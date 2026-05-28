#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/SDRAM/sdram.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/TIMER/btim.h"
#include "./BSP/NTP/ntp_sync.h"
#include "./BSP/SDCARD/sdcard.h"    /* SD卡驱动 */
#include "ff.h"                     /* FatFS */
#include "lvgl.h"
#include "lv_port_indev_template.h"
#include "lv_port_disp_template.h"
#include "lv_player.h"
#include "lv_player_main.h"
#include "font_sdram.h"             /* SDRAM字体加载 */
#include "lv_boot_screen.h"         /* 启动画面 */

static FATFS g_fatfs;   /* FatFS文件系统对象，全局保持挂载状态 */

int main(void)
{
    sys_cache_enable();
    HAL_Init();
    sys_stm32_clock_init(192, 5, 2, 4);
    delay_init(480);
    usart_init(115200);
    mpu_memory_protection();
    led_init();
    key_init();
    sdram_init();
    btim_timx_int_init(100-1, 2400-1);
    ntp_sync_init();

    /* ---- LVGL 初始化 ---- */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_fs_fatfs_init();             /* 注册LVGL FatFS文件系统驱动 */

    /* ---- 显示启动画面，避免白屏 ---- */
    lv_boot_screen_show();
    lv_task_handler();              /* 立即渲染一帧，让启动画面出现 */

    /* ---- SD卡初始化 ---- */
    lv_boot_screen_update(10, "Mounting SD card...");
    if (sd_init() == SD_OK) {
        if (f_mount(&g_fatfs, "S:", 1) == FR_OK) {
            printf("SD卡挂载成功\r\n");
            lv_boot_screen_update(30, "SD card OK");
        } else {
            printf("FatFS挂载失败\r\n");
            lv_boot_screen_update(30, "SD mount failed");
        }
    } else {
        printf("SD卡初始化失败\r\n");
        lv_boot_screen_update(30, "SD card error");
    }

    /* ---- 加载中文字体（Step1: SD卡读取，Step2: 内存解析） ---- */
    lv_boot_screen_update(40, "Reading font from SD...");
    /* font_sdram_init 内部会自动更新进度，这里只是给一个起始提示 */
    font_sdram_init();
    lv_boot_screen_update(90, "Font ready");

    /* ---- 启动主界面 ---- */
    lv_boot_screen_update(100, "Starting...");
    lv_task_handler();
    delay_ms(300);                  /* 短暂停留让用户看到100% */

    lv_boot_screen_destroy();
    lv_player();

    while (1)
    {
        lv_task_handler();
        ntp_sync_process();
        delay_ms(5);
    }
}
