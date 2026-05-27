#ifndef LV_PLAYER_MAIN_H
#define LV_PLAYER_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "./BSP/NTP/ntp_sync.h"     /* 直接读RTC获取时间 */

void lv_player(void);
void lv_player_destroy(void);
lv_obj_t * _lv_player_main_create(lv_obj_t * parent);

/* NTP校时后同步屏幕时钟（main.c调用） */
void clock_set_time(uint16_t year, uint8_t month, uint8_t day,
                    uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PLAYER_MAIN_H*/
