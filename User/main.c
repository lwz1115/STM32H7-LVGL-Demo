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

    /* SD卡初始化 + FatFS挂载（驱动字母 S:） */
    if (sd_init() == SD_OK) {
        if (f_mount(&g_fatfs, "S:", 1) == FR_OK) {
            printf("SD卡挂载成功\r\n");
        } else {
            printf("FatFS挂载失败\r\n");
        }
    } else {
        printf("SD卡初始化失败\r\n");
    }

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    /* 从SD卡加载完整中文字体到SDRAM */
    font_sdram_init();

    lv_player();

    while (1)
    {
        lv_task_handler();
        ntp_sync_process();
        delay_ms(5);
    }
}
