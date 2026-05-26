#ifndef LV_PLAYER_MAIN_H
#define LV_PLAYER_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void lv_player(void);
void lv_player_destroy(void);  /* 添加销毁函数声明 */
lv_obj_t * _lv_player_main_create(lv_obj_t * parent);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PLAYER_MAIN_H*/
