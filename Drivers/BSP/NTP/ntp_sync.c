/**
 * @file ntp_sync.c
 * @brief NTP时间同步 + STM32H743 内置RTC驱动
 *
 * 工作流程：
 *  1. ntp_sync_init()  → 初始化 USART2 接收 + 启动内置RTC
 *  2. ntp_sync_process() → 主循环调用，收到ESP32-S3时间帧后写入RTC
 *  3. ntp_sync_get_time() → 直接读RTC寄存器，精度 ±2~5秒/月
 *
 * 断电保持：VBAT 引脚接 3.3V 或纽扣电池后，断电时间继续走
 * 首次上电：RTC无有效时间，等ESP32-S3发来NTP时间后自动校准
 */

#include "ntp_sync.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * 内部变量
 * ============================================================ */
UART_HandleTypeDef  g_ntp_uart_handle;
RTC_HandleTypeDef   g_rtc_handle;          /* 内置RTC句柄，供外部读取 */

static uint8_t  s_rx_byte;
static char     s_rx_buf[NTP_RX_BUF_SIZE];     /* 接收中缓冲区（中断写入） */
static char     s_frame_buf[NTP_RX_BUF_SIZE];  /* 完整帧缓冲区（主循环读取） */
static uint8_t  s_rx_index    = 0;
static bool     s_frame_ready = false;

static ntp_status_t s_status      = NTP_STATUS_NEVER;
static uint32_t     s_sync_count  = 0;

/* RTC备份寄存器魔数，用于判断RTC是否已初始化过 */
#define RTC_MAGIC_VALUE     0xA5A5A5A5U
#define RTC_BACKUP_REG      RTC_BKP_DR0

/* ============================================================
 * 内置RTC初始化
 * ============================================================ */
static void rtc_init(void)
{
    /* STM32H7 的 PWR 时钟默认开启，无需手动使能 */
    HAL_PWR_EnableBkUpAccess();

    /* 先检查备份寄存器，如果RTC已经有有效时间就不重新初始化 */
    __HAL_RCC_RTC_ENABLE();
    g_rtc_handle.Instance = RTC;

    uint32_t magic = HAL_RTCEx_BKUPRead(&g_rtc_handle, RTC_BACKUP_REG);
    if (magic == RTC_MAGIC_VALUE) {
        printf("RTC: 检测到有效时间，直接使用\r\n");
        s_status = NTP_STATUS_OK;
        return;
    }

    /* 首次上电，初始化RTC，直接使用LSI（内部低速振荡器）
     * 不等待LSE，避免没有外部晶振时死循环
     * LSI精度约 ±1~2% ，对于NTP每分钟校时足够用 */
    printf("RTC: 首次上电，使用LSI初始化...\r\n");

    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);

    g_rtc_handle.Init.HourFormat     = RTC_HOURFORMAT_24;
    g_rtc_handle.Init.AsynchPrediv   = 127;
    g_rtc_handle.Init.SynchPrediv    = 249;  /* LSI约32kHz: 32000/(127+1)/(249+1) ≈ 1Hz */
    g_rtc_handle.Init.OutPut         = RTC_OUTPUT_DISABLE;
    g_rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    g_rtc_handle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    HAL_RTC_Init(&g_rtc_handle);

    printf("RTC: LSI初始化完成，等待NTP校时\r\n");
    s_status = NTP_STATUS_NEVER;
}

/* ============================================================
 * 将时间写入内置RTC
 * ============================================================ */
static void rtc_write_time(const ntp_time_t *t)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    sTime.Hours   = t->hour;
    sTime.Minutes = t->minute;
    sTime.Seconds = t->second;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    sDate.Year    = (uint8_t)(t->year - 2000);  /* RTC存储后两位年份 */
    sDate.Month   = t->month;
    sDate.Date    = t->date;
    /* RTC星期: 1=周一...7=周日，与ntp_time_t一致 */
    sDate.WeekDay = t->day;

    HAL_RTC_SetTime(&g_rtc_handle, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&g_rtc_handle, &sDate, RTC_FORMAT_BIN);

    /* 写入魔数，标记RTC已有有效时间 */
    HAL_RTCEx_BKUPWrite(&g_rtc_handle, RTC_BACKUP_REG, RTC_MAGIC_VALUE);
}

/* ============================================================
 * 串口初始化
 * ============================================================ */
void ntp_sync_init(void)
{
    /* USART2 配置 - GPIO和时钟由 HAL_UART_MspInit 自动处理 */
    g_ntp_uart_handle.Instance          = NTP_UART;
    g_ntp_uart_handle.Init.BaudRate     = NTP_UART_BAUDRATE;
    g_ntp_uart_handle.Init.WordLength   = UART_WORDLENGTH_8B;
    g_ntp_uart_handle.Init.StopBits     = UART_STOPBITS_1;
    g_ntp_uart_handle.Init.Parity       = UART_PARITY_NONE;
    g_ntp_uart_handle.Init.Mode         = UART_MODE_TX_RX;
    g_ntp_uart_handle.Init.HwFlowCtl   = UART_HWCONTROL_NONE;
    HAL_UART_Init(&g_ntp_uart_handle);  /* 内部自动调用 HAL_UART_MspInit */

    /* 启动接收中断 */
    HAL_UART_Receive_IT(&g_ntp_uart_handle, &s_rx_byte, 1);

    printf("NTP串口初始化: USART2 PA2(TX) PA3(RX) %dbps\r\n", NTP_UART_BAUDRATE);

    /* --- 内置RTC初始化 --- */
    rtc_init();
}

/* ============================================================
 * 解析时间帧: $TIME,2025,01,15,3,10,30,00#
 * ============================================================ */
static bool parse_time_frame(const char *frame, ntp_time_t *t)
{
    if (strncmp(frame, "$TIME,", 6) != 0) return false;

    int year, month, date, day, hour, minute, second;
    int n = sscanf(frame + 6, "%d,%d,%d,%d,%d,%d,%d",
                   &year, &month, &date, &day, &hour, &minute, &second);

    if (n != 7)                     return false;
    if (year < 2000 || year > 2099) return false;
    if (month < 1   || month > 12)  return false;
    if (date  < 1   || date  > 31)  return false;
    if (day   < 1   || day   > 7)   return false;
    if (hour  < 0   || hour  > 23)  return false;
    if (minute < 0  || minute > 59) return false;
    if (second < 0  || second > 59) return false;

    t->year   = (uint16_t)year;
    t->month  = (uint8_t)month;
    t->date   = (uint8_t)date;
    t->day    = (uint8_t)day;
    t->hour   = (uint8_t)hour;
    t->minute = (uint8_t)minute;
    t->second = (uint8_t)second;
    return true;
}

/* ============================================================
 * 处理接收到的完整帧（主循环调用）
 * ============================================================ */
void ntp_sync_process(void)
{
    if (!s_frame_ready) return;

    /* 从独立帧缓冲区读取，不受中断影响 */
    char frame_copy[NTP_RX_BUF_SIZE];
    memcpy(frame_copy, s_frame_buf, NTP_RX_BUF_SIZE);
    s_frame_ready = false;

    printf("NTP收到: %s\r\n", frame_copy);

    ntp_time_t t;
    if (!parse_time_frame(frame_copy, &t)) {
        printf("NTP解析失败\r\n");
        return;
    }

    /* 写入内置RTC */
    rtc_write_time(&t);
    s_status = NTP_STATUS_OK;
    s_sync_count++;

    printf("NTP校时成功 #%lu → RTC: %04d-%02d-%02d %02d:%02d:%02d 周%d\r\n",
           s_sync_count, t.year, t.month, t.date,
           t.hour, t.minute, t.second, t.day);
}

/* ============================================================
 * 查询接口
 * ============================================================ */
bool ntp_sync_is_synced(void)       { return (s_status == NTP_STATUS_OK); }
ntp_status_t ntp_sync_get_status(void) { return s_status; }
uint32_t ntp_sync_get_count(void)   { return s_sync_count; }

/* ============================================================
 * 读取当前时间（直接读RTC寄存器，硬件走时，精度高）
 * ============================================================ */
bool ntp_sync_get_time(ntp_time_t *out_time)
{
    if (s_status != NTP_STATUS_OK) return false;

    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    /* 必须先读Time再读Date，否则RTC影子寄存器不会更新 */
    HAL_RTC_GetTime(&g_rtc_handle, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&g_rtc_handle, &sDate, RTC_FORMAT_BIN);

    out_time->year   = 2000 + sDate.Year;
    out_time->month  = sDate.Month;
    out_time->date   = sDate.Date;
    out_time->day    = sDate.WeekDay;   /* 1=周一...7=周日 */
    out_time->hour   = sTime.Hours;
    out_time->minute = sTime.Minutes;
    out_time->second = sTime.Seconds;

    return true;
}

/* ============================================================
 * UART接收中断（由 usart.c 的 HAL_UART_RxCpltCallback 转发）
 * ============================================================ */
void ntp_uart_rx_callback(void)
{
    char c = (char)s_rx_byte;

    /* 检测帧头 '$'，重置接收缓冲区 */
    if (c == '$') {
        s_rx_index = 0;
        memset(s_rx_buf, 0, NTP_RX_BUF_SIZE);
    }

    /* 存入缓冲区 */
    if (s_rx_index < NTP_RX_BUF_SIZE - 1) {
        s_rx_buf[s_rx_index++] = c;
        s_rx_buf[s_rx_index]   = '\0';
    } else {
        /* 缓冲区溢出，重置 */
        s_rx_index = 0;
        memset(s_rx_buf, 0, NTP_RX_BUF_SIZE);
    }

    /* 检测帧尾 '#'，帧长度至少要有 "$TIME,x#" = 8个字符 */
    if (c == NTP_FRAME_TAIL && s_rx_index >= 8) {
        /* 立刻把完整帧复制到独立缓冲区，不受后续 \r\n 影响 */
        memcpy(s_frame_buf, s_rx_buf, NTP_RX_BUF_SIZE);
        s_frame_ready = true;
        /* 不清零 s_rx_index，等下一个 '$' 来时再重置 */
    }

    HAL_UART_Receive_IT(&g_ntp_uart_handle, &s_rx_byte, 1);
}


