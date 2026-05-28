/**
 * @file lv_boot_screen.c
 * @brief 系统启动画面实现
 *
 * 布局（800x480）：
 *   深色背景
 *   中央：项目名称大字
 *   下方：进度条 + 状态文字
 *   底部：版本号
 */

#include "lv_boot_screen.h"
#include <string.h>

/* ============================================================
 * 内部对象
 * ============================================================ */
static lv_obj_t *s_screen    = NULL;
static lv_obj_t *s_bar       = NULL;   /* 进度条 */
static lv_obj_t *s_msg_label = NULL;   /* 状态文字 */

/* ============================================================
 * 公开接口
 * ============================================================ */

void lv_boot_screen_show(void)
{
    if (s_screen != NULL) return;

    lv_coord_t sw = lv_disp_get_hor_res(NULL);
    lv_coord_t sh = lv_disp_get_ver_res(NULL);

    /* 背景 */
    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, sw, sh);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x1a2535), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 项目名称 */
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, "H7-MediaCube");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -60);

    /* 副标题 */
    lv_obj_t *sub = lv_label_create(s_screen);
    lv_label_set_text(sub, "STM32H743 + LVGL v8");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x7f8c8d), 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 0);

    /* 进度条背景 */
    lv_obj_t *bar_bg = lv_obj_create(s_screen);
    lv_obj_set_size(bar_bg, 400, 6);
    lv_obj_set_style_bg_color(bar_bg, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_bg_opa(bar_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_bg, 0, 0);
    lv_obj_set_style_radius(bar_bg, 3, 0);
    lv_obj_align(bar_bg, LV_ALIGN_CENTER, 0, 60);

    /* 进度条 */
    s_bar = lv_bar_create(s_screen);
    lv_obj_set_size(s_bar, 400, 6);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x3498db), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar, 3, 0);
    lv_obj_set_style_radius(s_bar, 3, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(s_bar, 0, 0);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_align(s_bar, LV_ALIGN_CENTER, 0, 60);

    /* 状态文字 */
    s_msg_label = lv_label_create(s_screen);
    lv_label_set_text(s_msg_label, "Initializing...");
    lv_obj_set_style_text_font(s_msg_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_msg_label, lv_color_hex(0x95a5a6), 0);
    lv_obj_align(s_msg_label, LV_ALIGN_CENTER, 0, 85);

    /* 版本号 */
    lv_obj_t *ver = lv_label_create(s_screen);
    lv_label_set_text(ver, "v1.0.0");
    lv_obj_set_style_text_font(ver, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ver, lv_color_hex(0x4a5568), 0);
    lv_obj_align(ver, LV_ALIGN_BOTTOM_RIGHT, -20, -15);

    lv_scr_load(s_screen);
}

void lv_boot_screen_update(uint8_t progress, const char *msg)
{
    if (s_bar != NULL) {
        lv_bar_set_value(s_bar, progress, LV_ANIM_OFF);
    }
    if (s_msg_label != NULL && msg != NULL) {
        lv_label_set_text(s_msg_label, msg);
    }
    /* 强制刷新一帧，让进度可见 */
    lv_task_handler();
}

void lv_boot_screen_destroy(void)
{
    if (s_screen != NULL) {
        lv_obj_del(s_screen);
        s_screen    = NULL;
        s_bar       = NULL;
        s_msg_label = NULL;
    }
}
