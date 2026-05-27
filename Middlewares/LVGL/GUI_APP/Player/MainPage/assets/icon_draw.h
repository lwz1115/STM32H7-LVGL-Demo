/**
 * @file icon_draw.h
 * @brief 图标绘制函数头文件
 */

#ifndef ICON_DRAW_H
#define ICON_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief 绘制电池图标（黑色，100%电量）
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_battery_icon(lv_obj_t * parent);

/**
 * @brief 绘制返回图标（黑色箭头）
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_return_icon(lv_obj_t * parent);

/**
 * @brief 绘制WiFi图标（黑色，3格信号）
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_wifi_icon(lv_obj_t * parent);

/**
 * @brief 绘制流量数据图标（黑色，5G信号柱状图）
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_data_icon(lv_obj_t * parent);

/**
 * @brief 绘制文件图标（文档图标）
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_file_icon(lv_obj_t * parent);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*ICON_DRAW_H*/
