/**
 * @file ds1302.h
 * @brief DS1302实时时钟驱动头文件
 * @note DS1302是一款低功耗实时时钟芯片，通过3线串行接口与MCU通信
 */

#ifndef __DS1302_H
#define __DS1302_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

/* DS1302引脚定义 - 根据实际硬件连接修改 */
#define DS1302_CLK_GPIO_PORT    GPIOB
#define DS1302_CLK_PIN          GPIO_PIN_9
#define DS1302_DAT_GPIO_PORT    GPIOB
#define DS1302_DAT_PIN          GPIO_PIN_8
#define DS1302_RST_GPIO_PORT    GPIOB
#define DS1302_RST_PIN          GPIO_PIN_7

/* DS1302寄存器地址定义 */
#define DS1302_SECOND_WRITE     0x80    /* 秒 写 */
#define DS1302_SECOND_READ      0x81    /* 秒 读 */
#define DS1302_MINUTE_WRITE     0x82    /* 分 写 */
#define DS1302_MINUTE_READ      0x83    /* 分 读 */
#define DS1302_HOUR_WRITE       0x84    /* 时 写 */
#define DS1302_HOUR_READ        0x85    /* 时 读 */
#define DS1302_DATE_WRITE       0x86    /* 日 写 */
#define DS1302_DATE_READ        0x87    /* 日 读 */
#define DS1302_MONTH_WRITE      0x88    /* 月 写 */
#define DS1302_MONTH_READ       0x89    /* 月 读 */
#define DS1302_DAY_WRITE        0x8A    /* 星期 写 */
#define DS1302_DAY_READ         0x8B    /* 星期 读 */
#define DS1302_YEAR_WRITE       0x8C    /* 年 写 */
#define DS1302_YEAR_READ        0x8D    /* 年 读 */
#define DS1302_WP_WRITE         0x8E    /* 写保护 写 */
#define DS1302_WP_READ          0x8F    /* 写保护 读 */

/* 时间结构体 */
typedef struct {
    uint16_t year;      /* 年 (2000-2099) */
    uint8_t  month;     /* 月 (1-12) */
    uint8_t  date;      /* 日 (1-31) */
    uint8_t  day;       /* 星期 (1-7, 1=星期一) */
    uint8_t  hour;      /* 时 (0-23) */
    uint8_t  minute;    /* 分 (0-59) */
    uint8_t  second;    /* 秒 (0-59) */
} ds1302_time_t;

/* 函数声明 */
void ds1302_init(void);
void ds1302_write_time(const ds1302_time_t *time);
void ds1302_read_time(ds1302_time_t *time);
void ds1302_get_time_string(char *buf, uint8_t buf_size);
void ds1302_get_date_string(char *buf, uint8_t buf_size);
const char* ds1302_get_weekday_string(uint8_t day);

/* BCD码转换函数 */
uint8_t bcd_to_dec(uint8_t bcd);
uint8_t dec_to_bcd(uint8_t dec);

#endif /* __DS1302_H */
