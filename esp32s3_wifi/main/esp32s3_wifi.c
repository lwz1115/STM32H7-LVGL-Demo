/**
 * @file esp32s3_wifi.c
 * @brief ESP32-S3 WiFi连接 + NTP时间同步 + 串口发送给STM32
 *
 * 功能说明：
 *  1. 连接WiFi (SSID: lwz, 密码: 88888888)
 *  2. 通过NTP服务器获取北京时间 (UTC+8)
 *  3. 开机同步后立即发送，之后每60秒发送一次
 *  4. 发送格式: $TIME,2026,05,26,2,22,47,49#\r\n
 *              (年,月,日,星期,时,分,秒)
 *
 * 串口连接：
 *  ESP32-S3 GPIO17(TX) --> STM32 PA3(USART2 RX)
 *  ESP32-S3 GPIO18(RX) <-- STM32 PA2(USART2 TX)
 *  GND -------------------- GND
 *  波特率: 115200
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "driver/uart.h"

/* ============================================================
 * 用户配置区
 * ============================================================ */
#define WIFI_SSID           "lwz"
#define WIFI_PASSWORD       "88888888"
#define NTP_SERVER1         "ntp.aliyun.com"
#define NTP_SERVER2         "pool.ntp.org"
#define NTP_SYNC_INTERVAL   3600            /* NTP同步间隔(秒) */
#define SEND_INTERVAL_SEC   60              /* 发送时间给STM32的间隔(秒) */

/* 串口配置 - UART1, GPIO17=TX, GPIO18=RX */
#define UART_PORT_NUM       UART_NUM_1
#define UART_BAUD_RATE      115200
#define UART_TX_PIN         17
#define UART_RX_PIN         18
#define UART_BUF_SIZE       256

/* ============================================================ */

static const char *TAG = "ESP32S3_NTP";

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static int  s_retry_num   = 0;
#define WIFI_MAX_RETRY      10

static bool s_time_synced = false;

/* ============================================================
 * 串口初始化
 * ============================================================ */
static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = UART_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    /* 正确顺序：先配置参数和引脚，再安装驱动 */
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "串口初始化: UART%d TX=GPIO%d RX=GPIO%d %dbps",
             UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
}

/* ============================================================
 * 发送时间数据给STM32
 * 格式: $TIME,年,月,日,星期,时,分,秒#\r\n
 * 星期: 1=周一 ... 7=周日
 * ============================================================ */
static void send_time_to_stm32(void)
{
    time_t now;
    struct tm timeinfo;
    char tx_buf[64];

    time(&now);
    /* 用 localtime_r，依赖 setenv("TZ","CST-8",1) 设置的时区 */
    localtime_r(&now, &timeinfo);

    /* tm_wday: 0=周日,1=周一...6=周六 → 转为 1=周一...7=周日 */
    uint8_t weekday = (timeinfo.tm_wday == 0) ? 7 : timeinfo.tm_wday;

    int len = snprintf(tx_buf, sizeof(tx_buf),
                       "$TIME,%04d,%02d,%02d,%d,%02d,%02d,%02d#\r\n",
                       timeinfo.tm_year + 1900,
                       timeinfo.tm_mon + 1,
                       timeinfo.tm_mday,
                       weekday,
                       timeinfo.tm_hour,
                       timeinfo.tm_min,
                       timeinfo.tm_sec);

    uart_write_bytes(UART_PORT_NUM, tx_buf, len);
    ESP_LOGI(TAG, "已发送时间给STM32: %s", tx_buf);
}

/* ============================================================
 * NTP时间同步回调
 * ============================================================ */
static void ntp_sync_callback(struct timeval *tv)
{
    s_time_synced = true;
    ESP_LOGI(TAG, "NTP时间同步成功！");
    /* 同步成功后立即发送一次 */
    send_time_to_stm32();
}

/* ============================================================
 * 初始化SNTP
 * ============================================================ */
static void sntp_init_custom(void)
{
    ESP_LOGI(TAG, "初始化NTP客户端: %s / %s", NTP_SERVER1, NTP_SERVER2);

    /* 设置时区为北京时间 UTC+8 */
    setenv("TZ", "CST-8", 1);
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER1);
    esp_sntp_setservername(1, NTP_SERVER2);
    sntp_set_time_sync_notification_cb(ntp_sync_callback);
    sntp_set_sync_interval(NTP_SYNC_INTERVAL * 1000);
    esp_sntp_init();
}

/* ============================================================
 * WiFi事件处理
 * ============================================================ */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "正在连接WiFi: %s ...", WIFI_SSID);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "WiFi断开，第%d次重连...", s_retry_num);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "WiFi连接失败，已达最大重连次数(%d)", WIFI_MAX_RETRY);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi连接成功！IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ============================================================
 * WiFi初始化
 * ============================================================ */
static bool wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 等待连接结果（最多30秒） */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(30000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "成功连接到WiFi: %s", WIFI_SSID);
        return true;
    } else {
        ESP_LOGE(TAG, "连接WiFi失败: %s", WIFI_SSID);
        return false;
    }
}

/* ============================================================
 * 定时发送任务：先发送，再等待
 * ============================================================ */
static void time_send_task(void *pvParameters)
{
    ESP_LOGI(TAG, "定时发送任务启动，间隔: %d 秒", SEND_INTERVAL_SEC);

    while (1) {
        if (s_time_synced) {
            send_time_to_stm32();
        } else {
            ESP_LOGW(TAG, "NTP尚未同步，跳过本次发送");
        }
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_SEC * 1000));
    }
}

/* ============================================================
 * 主函数
 * ============================================================ */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  H7-MediaCube ESP32-S3 NTP时间同步");
    ESP_LOGI(TAG, "========================================");

    /* 1. 初始化NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. 初始化串口（GPIO17=TX，GPIO18=RX） */
    uart_init();

    /* 3. 连接WiFi */
    bool wifi_ok = wifi_init_sta();
    if (!wifi_ok) {
        ESP_LOGE(TAG, "WiFi连接失败，3秒后重启...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
        return;
    }

    /* 4. 启动NTP同步（含时区设置） */
    sntp_init_custom();

    /* 5. 等待NTP同步（最多60秒） */
    ESP_LOGI(TAG, "等待NTP时间同步...");
    int wait_count = 0;
    while (!s_time_synced && wait_count < 60) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        wait_count++;
        if (wait_count % 5 == 0) {
            ESP_LOGI(TAG, "等待NTP同步... (%d秒)", wait_count);
        }
    }

    if (!s_time_synced) {
        ESP_LOGW(TAG, "NTP同步超时，后台继续重试");
    }

    /* 6. 创建定时发送任务 */
    xTaskCreate(time_send_task, "time_send", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "初始化完成，每隔%d秒向STM32发送时间", SEND_INTERVAL_SEC);
}
