/**
 * @file ntp_sync.h
 * @brief NTP时间同步 + STM32H743 内置RTC
 *
 * 数据帧格式（ESP32-S3发送）:
 *   $TIME,年,月,日,星期,时,分,秒#\r\n
 *   例: $TIME,2025,01,15,3,10,30,00#\r\n
 *   星期: 1=周一 ... 7=周日
 *
 * 串口: USART2，PA2(TX) PA3(RX)，115200bps
 * RTC:  内置硬件RTC，LSE 32.768kHz，精度 ±2~5秒/月
 */

#ifndef __NTP_SYNC_H
#define __NTP_SYNC_H

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * 串口配置 - USART2: PA2=TX, PA3=RX
 * ============================================================ */
#define NTP_UART                    USART2
#define NTP_UART_IRQn               USART2_IRQn
#define NTP_UART_CLK_ENABLE()       __HAL_RCC_USART2_CLK_ENABLE()
#define NTP_UART_TX_GPIO_PORT       GPIOA
#define NTP_UART_TX_PIN             GPIO_PIN_2
#define NTP_UART_TX_AF              GPIO_AF7_USART2
#define NTP_UART_RX_GPIO_PORT       GPIOA
#define NTP_UART_RX_PIN             GPIO_PIN_3
#define NTP_UART_RX_AF              GPIO_AF7_USART2
#define NTP_UART_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define NTP_UART_BAUDRATE           115200

/* 接收缓冲区大小 */
#define NTP_RX_BUF_SIZE             64

/* 帧尾标识 */
#define NTP_FRAME_TAIL              '#'

/* ============================================================
 * 时间结构体
 * ============================================================ */
typedef struct {
    uint16_t year;      /* 年 (2000-2099) */
    uint8_t  month;     /* 月 (1-12) */
    uint8_t  date;      /* 日 (1-31) */
    uint8_t  day;       /* 星期 (1=周一 ... 7=周日) */
    uint8_t  hour;      /* 时 (0-23) */
    uint8_t  minute;    /* 分 (0-59) */
    uint8_t  second;    /* 秒 (0-59) */
} ntp_time_t;

/* NTP同步状态 */
typedef enum {
    NTP_STATUS_NEVER  = 0,  /* 从未同步过（RTC无有效时间） */
    NTP_STATUS_OK     = 1,  /* 已同步（RTC正在走时） */
    NTP_STATUS_ERROR  = 2,  /* 解析错误 */
} ntp_status_t;

/* ============================================================
 * 函数声明
 * ============================================================ */
void         ntp_sync_init(void);       /* 初始化串口 + RTC，在main中调用一次 */
void         ntp_sync_process(void);    /* 主循环调用，处理收到的时间帧 */
bool         ntp_sync_is_synced(void);  /* RTC是否有有效时间 */
ntp_status_t ntp_sync_get_status(void);
uint32_t     ntp_sync_get_count(void);  /* NTP校时次数 */
bool         ntp_sync_get_time(ntp_time_t *out_time); /* 读RTC当前时间 */

/* usart.c 的 HAL_UART_RxCpltCallback 中转发调用 */
void ntp_uart_rx_callback(void);

/* 外部句柄 */
extern UART_HandleTypeDef g_ntp_uart_handle;
extern RTC_HandleTypeDef  g_rtc_handle;

#endif /* __NTP_SYNC_H */
