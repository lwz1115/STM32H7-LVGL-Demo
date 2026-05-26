/**
 * @file ds1302.c
 * @brief DS1302实时时钟驱动实现文件
 */

#include "ds1302.h"
#include <stdio.h>
#include <string.h>

/* 引脚操作宏定义 */
#define DS1302_CLK_HIGH()   HAL_GPIO_WritePin(DS1302_CLK_GPIO_PORT, DS1302_CLK_PIN, GPIO_PIN_SET)
#define DS1302_CLK_LOW()    HAL_GPIO_WritePin(DS1302_CLK_GPIO_PORT, DS1302_CLK_PIN, GPIO_PIN_RESET)
#define DS1302_DAT_HIGH()   HAL_GPIO_WritePin(DS1302_DAT_GPIO_PORT, DS1302_DAT_PIN, GPIO_PIN_SET)
#define DS1302_DAT_LOW()    HAL_GPIO_WritePin(DS1302_DAT_GPIO_PORT, DS1302_DAT_PIN, GPIO_PIN_RESET)
#define DS1302_RST_HIGH()   HAL_GPIO_WritePin(DS1302_RST_GPIO_PORT, DS1302_RST_PIN, GPIO_PIN_SET)
#define DS1302_RST_LOW()    HAL_GPIO_WritePin(DS1302_RST_GPIO_PORT, DS1302_RST_PIN, GPIO_PIN_RESET)
#define DS1302_DAT_READ()   HAL_GPIO_ReadPin(DS1302_DAT_GPIO_PORT, DS1302_DAT_PIN)

/* 星期字符串数组 */
static const char* weekday_strings[] = {
    "",         /* 0 - 无效 */
    "星期一",   /* 1 */
    "星期二",   /* 2 */
    "星期三",   /* 3 */
    "星期四",   /* 4 */
    "星期五",   /* 5 */
    "星期六",   /* 6 */
    "星期日"    /* 7 */
};

/**
 * @brief 微秒级延时（软件延时）
 */
static void ds1302_delay_us(uint32_t us)
{
    uint32_t i;
    for(i = 0; i < us * 10; i++) {
        __NOP();
    }
}

/**
 * @brief 设置DAT引脚为输出模式
 */
static void ds1302_dat_output_mode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_DAT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS1302_DAT_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief 设置DAT引脚为输入模式
 */
static void ds1302_dat_input_mode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_DAT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS1302_DAT_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief 向DS1302写入一个字节
 * @param dat 要写入的数据
 */
static void ds1302_write_byte(uint8_t dat)
{
    uint8_t i;
    ds1302_dat_output_mode();
    
    for(i = 0; i < 8; i++) {
        DS1302_CLK_LOW();
        ds1302_delay_us(1);
        
        if(dat & 0x01) {
            DS1302_DAT_HIGH();
        } else {
            DS1302_DAT_LOW();
        }
        
        dat >>= 1;
        DS1302_CLK_HIGH();
        ds1302_delay_us(1);
    }
}

/**
 * @brief 从DS1302读取一个字节
 * @return 读取到的数据
 */
static uint8_t ds1302_read_byte(void)
{
    uint8_t i, dat = 0;
    ds1302_dat_input_mode();
    
    for(i = 0; i < 8; i++) {
        DS1302_CLK_LOW();
        ds1302_delay_us(1);
        
        dat >>= 1;
        if(DS1302_DAT_READ()) {
            dat |= 0x80;
        }
        
        DS1302_CLK_HIGH();
        ds1302_delay_us(1);
    }
    
    return dat;
}

/**
 * @brief 向DS1302指定地址写入数据
 * @param addr 寄存器地址
 * @param dat 要写入的数据
 */
static void ds1302_write_reg(uint8_t addr, uint8_t dat)
{
    DS1302_RST_LOW();
    DS1302_CLK_LOW();
    ds1302_delay_us(2);
    
    DS1302_RST_HIGH();
    ds1302_delay_us(2);
    
    ds1302_write_byte(addr);
    ds1302_write_byte(dat);
    
    DS1302_RST_LOW();
    DS1302_CLK_HIGH();
    ds1302_delay_us(2);
}

/**
 * @brief 从DS1302指定地址读取数据
 * @param addr 寄存器地址
 * @return 读取到的数据
 */
static uint8_t ds1302_read_reg(uint8_t addr)
{
    uint8_t dat;
    
    DS1302_RST_LOW();
    DS1302_CLK_LOW();
    ds1302_delay_us(2);
    
    DS1302_RST_HIGH();
    ds1302_delay_us(2);
    
    ds1302_write_byte(addr);
    dat = ds1302_read_byte();
    
    DS1302_RST_LOW();
    DS1302_CLK_HIGH();
    ds1302_delay_us(2);
    
    return dat;
}

/**
 * @brief BCD码转十进制
 */
uint8_t bcd_to_dec(uint8_t bcd)
{
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

/**
 * @brief 十进制转BCD码
 */
uint8_t dec_to_bcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

/**
 * @brief DS1302初始化
 */
void ds1302_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 使能GPIO时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* 配置CLK引脚 */
    GPIO_InitStruct.Pin = DS1302_CLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS1302_CLK_GPIO_PORT, &GPIO_InitStruct);
    
    /* 配置RST引脚 */
    GPIO_InitStruct.Pin = DS1302_RST_PIN;
    HAL_GPIO_Init(DS1302_RST_GPIO_PORT, &GPIO_InitStruct);
    
    /* 配置DAT引脚（初始为输出） */
    GPIO_InitStruct.Pin = DS1302_DAT_PIN;
    HAL_GPIO_Init(DS1302_DAT_GPIO_PORT, &GPIO_InitStruct);
    
    /* 初始化引脚状态 */
    DS1302_CLK_LOW();
    DS1302_RST_LOW();
    DS1302_DAT_LOW();
    
    /* 关闭写保护 */
    ds1302_write_reg(DS1302_WP_WRITE, 0x00);
    
    /* 启动时钟（清除CH位） */
    uint8_t sec = ds1302_read_reg(DS1302_SECOND_READ);
    if(sec & 0x80) {
        ds1302_write_reg(DS1302_SECOND_WRITE, sec & 0x7F);
    }
}

/**
 * @brief 写入时间到DS1302
 * @param time 时间结构体指针
 */
void ds1302_write_time(const ds1302_time_t *time)
{
    /* 关闭写保护 */
    ds1302_write_reg(DS1302_WP_WRITE, 0x00);
    
    /* 写入时间数据 */
    ds1302_write_reg(DS1302_SECOND_WRITE, dec_to_bcd(time->second));
    ds1302_write_reg(DS1302_MINUTE_WRITE, dec_to_bcd(time->minute));
    ds1302_write_reg(DS1302_HOUR_WRITE, dec_to_bcd(time->hour));
    ds1302_write_reg(DS1302_DATE_WRITE, dec_to_bcd(time->date));
    ds1302_write_reg(DS1302_MONTH_WRITE, dec_to_bcd(time->month));
    ds1302_write_reg(DS1302_DAY_WRITE, dec_to_bcd(time->day));
    ds1302_write_reg(DS1302_YEAR_WRITE, dec_to_bcd(time->year % 100));
    
    /* 开启写保护 */
    ds1302_write_reg(DS1302_WP_WRITE, 0x80);
}

/**
 * @brief 从DS1302读取时间
 * @param time 时间结构体指针
 */
void ds1302_read_time(ds1302_time_t *time)
{
    uint8_t temp;
    
    temp = ds1302_read_reg(DS1302_SECOND_READ);
    time->second = bcd_to_dec(temp & 0x7F);
    
    temp = ds1302_read_reg(DS1302_MINUTE_READ);
    time->minute = bcd_to_dec(temp & 0x7F);
    
    temp = ds1302_read_reg(DS1302_HOUR_READ);
    time->hour = bcd_to_dec(temp & 0x3F);
    
    temp = ds1302_read_reg(DS1302_DATE_READ);
    time->date = bcd_to_dec(temp & 0x3F);
    
    temp = ds1302_read_reg(DS1302_MONTH_READ);
    time->month = bcd_to_dec(temp & 0x1F);
    
    temp = ds1302_read_reg(DS1302_DAY_READ);
    time->day = bcd_to_dec(temp & 0x07);
    
    temp = ds1302_read_reg(DS1302_YEAR_READ);
    time->year = 2000 + bcd_to_dec(temp);
}

/**
 * @brief 获取时间字符串 (HH:MM:SS格式)
 * @param buf 缓冲区指针
 * @param buf_size 缓冲区大小
 */
void ds1302_get_time_string(char *buf, uint8_t buf_size)
{
    ds1302_time_t time;
    ds1302_read_time(&time);
    snprintf(buf, buf_size, "%02d:%02d:%02d", time.hour, time.minute, time.second);
}

/**
 * @brief 获取日期字符串 (YYYY年MM月DD日格式)
 * @param buf 缓冲区指针
 * @param buf_size 缓冲区大小
 */
void ds1302_get_date_string(char *buf, uint8_t buf_size)
{
    ds1302_time_t time;
    ds1302_read_time(&time);
    snprintf(buf, buf_size, "%d年%d月%d日", time.year, time.month, time.date);
}

/**
 * @brief 获取星期字符串
 * @param day 星期数 (1-7)
 * @return 星期字符串指针
 */
const char* ds1302_get_weekday_string(uint8_t day)
{
    if(day >= 1 && day <= 7) {
        return weekday_strings[day];
    }
    return weekday_strings[0];
}

