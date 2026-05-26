#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/SDRAM/sdram.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/TIMER/btim.h"

/* LVGL */
#include "lvgl.h"
#include "lv_port_indev_template.h"
#include "lv_port_disp_template.h"
#include "lv_player.h"


int main(void)
{
    sys_cache_enable();                                         /* 使能 L1-Cache */
    HAL_Init();                                                 /* 初始化 HAL 库 */
    sys_stm32_clock_init(192, 5, 2, 4);                         /* 配置时钟, 480Mhz */
    delay_init(480);                                            /* 延时初始化 */
    usart_init(115200);                                         /* 串口初始化 */
    mpu_memory_protection();                                    /* 保护内存存储区域 */
    led_init();                                                 /* 初始化 LED */
    key_init();                                                 /* 初始化 KEY */
    sdram_init();                                               /* 初始化 SDRAM */
    btim_timx_int_init(100-1,2400-1);                           /* 初始化定时器 */

    lv_init();                                                  /* lvgl 系统初始化 */
    lv_port_disp_init();                                        /* lvgl 显示接口初始化，需要在 lv_init() 之后调用 */
    lv_port_indev_init();                                       /* lvgl 输入接口初始化，需要在 lv_init() 之后调用 */
    
    lv_player();                                                /* 播放 Player 界面 */
    
    while (1)
    {
        lv_task_handler();
        delay_ms(5);
    }
}

