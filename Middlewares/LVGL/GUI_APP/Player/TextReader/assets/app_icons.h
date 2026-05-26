/**
 * @file app_icons.h
 * @brief 应用图标 - 预渲染Canvas版本
 *
 * 9个图标对应功能：
 *  0 - 文本阅读器   1 - 图片浏览器   2 - 音乐播放器
 *  3 - 天气         4 - AI对话       5 - WiFi管理
 *  6 - 温湿度       7 - 设置         8 - 画画
 */

#ifndef APP_ICONS_H
#define APP_ICONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* 图标尺寸（像素），与按钮内绘制区域匹配 */
#define APP_ICON_SIZE   60

/* 预渲染：将图标绘制到 lv_canvas，返回 canvas 对象
 * 调用一次后图像固化，不再消耗 CPU 重绘 */
lv_obj_t *draw_text_reader_icon   (lv_obj_t *parent, lv_coord_t size);
lv_obj_t *draw_photo_browser_icon (lv_obj_t *parent, lv_coord_t size);
lv_obj_t *draw_music_player_icon  (lv_obj_t *parent, lv_coord_t size);
lv_obj_t *draw_weather_icon       (lv_obj_t *parent, lv_coord_t size);
lv_obj_t *draw_ai_chat_icon       (lv_obj_t *parent, lv_coord_t size);
lv_obj_t *draw_wifi_mgr_icon      (lv_obj_t *parent, lv_coord_t size);
lv_obj_t *draw_temp_humidity_icon (lv_obj_t *parent, lv_coord_t size);
lv_obj_t *draw_settings_icon      (lv_obj_t *parent, lv_coord_t size);
lv_obj_t *draw_paint_icon         (lv_obj_t *parent, lv_coord_t size);

/* 兼容旧接口（lv_player_main.c 中仍使用的名称） */
#define draw_calculator_icon  draw_text_reader_icon
#define draw_palette_icon     draw_paint_icon
#define draw_video_icon       draw_photo_browser_icon
#define draw_photo_icon       draw_photo_browser_icon
#define draw_music_icon       draw_music_player_icon
#define draw_calendar_icon    draw_weather_icon
#define draw_text_icon        draw_text_reader_icon

#ifdef __cplusplus
}
#endif

#endif /* APP_ICONS_H */
