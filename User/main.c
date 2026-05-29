#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/SDRAM/sdram.h"
#include "./BSP/LED/led.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/NTP/ntp_sync.h"
#include "./BSP/SDCARD/sdcard.h"
#include "./BSP/TIMER/btim.h"
#include "ff.h"
#include "lvgl.h"
#include "lv_port_indev_template.h"
#include "lv_port_disp_template.h"
#include "lv_player.h"
#include "font_sdram.h"
#include "lv_boot_screen.h"
#include "FreeRTOS.h"
#include "task.h"

static FATFS g_fatfs;

/* ============================================================
 * LVGL 任务
 * 栈大小说明：
 *   SJPG/BMP 解码器调用栈较深，加上 LVGL 内部渲染栈，
 *   512 words(2KB) 会溢出导致卡死，需要至少 2048 words(8KB)。
 * ============================================================ */
#define LVGL_TASK_STACK     2048    /* 8KB，覆盖图片解码器调用深度 */
#define LVGL_TASK_PRIO      3

static void lvgl_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(5));
        lv_task_handler();
    }
}

/* ============================================================
 * NTP 任务
 * ============================================================ */
#define NTP_TASK_STACK      256     /* 1KB，仅串口解析，够用 */
#define NTP_TASK_PRIO       2

static void ntp_task(void *pvParameters)
{
    for (;;)
    {
        ntp_sync_process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ============================================================
 * FreeRTOS Hook
 * ============================================================ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("STACK OVERFLOW: %s\r\n", pcTaskName);
    while (1) { LED0_TOGGLE(); delay_ms(50); }
}

void vApplicationMallocFailedHook(void)
{
    printf("MALLOC FAILED\r\n");
    while (1) { LED0_TOGGLE(); LED1_TOGGLE(); delay_ms(100); }
}

/* ============================================================
 * main
 * ============================================================ */
int main(void)
{
    sys_cache_enable();
    HAL_Init();
    sys_stm32_clock_init(192, 5, 2, 4);
    delay_init(480);
    usart_init(115200);

    mpu_memory_protection();
    led_init();
    sdram_init();
    btim_timx_int_init(999, 239);   /* TIM6: 1ms tick for lv_tick_inc */
    ntp_sync_init();

    /* LVGL 初始化 */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_fs_fatfs_init();

    /* 启动画面 */
    lv_boot_screen_show();
    lv_task_handler();

    /* 挂载 SD 卡 */
    lv_boot_screen_update(10, "Mounting SD...");
    lv_task_handler();
    if (sd_init() == SD_OK && f_mount(&g_fatfs, "S:", 1) == FR_OK)
    {
        printf("SD OK\r\n");
        lv_boot_screen_update(30, "SD OK");
    }
    else
    {
        printf("SD fail\r\n");
        lv_boot_screen_update(30, "SD fail");
    }
    lv_task_handler();

    /* 加载字体 */
    lv_boot_screen_update(50, "Loading font...");
    lv_task_handler();
    font_sdram_init();
    lv_boot_screen_update(90, "Font ready");
    lv_task_handler();

    /* 进入主界面 */
    lv_boot_screen_update(100, "Starting...");
    lv_task_handler();
    delay_ms(200);
    lv_boot_screen_destroy();
    lv_player();
    lv_task_handler();

    /* 创建任务 */
    printf("Creating tasks...\r\n");
    if (xTaskCreate(lvgl_task, "LVGL", LVGL_TASK_STACK, NULL, LVGL_TASK_PRIO, NULL) != pdPASS ||
        xTaskCreate(ntp_task,  "NTP",  NTP_TASK_STACK,  NULL, NTP_TASK_PRIO,  NULL) != pdPASS)
    {
        printf("Task create FAIL\r\n");
        while (1) { LED0_TOGGLE(); delay_ms(500); }
    }

    printf("Starting scheduler...\r\n");
    LED1(0);
    vTaskStartScheduler();

    while (1) { LED0_TOGGLE(); delay_ms(100); }
}
