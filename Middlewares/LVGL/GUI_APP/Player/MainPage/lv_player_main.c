/**
 * @file lv_player_main.c
 * @brief Player主界面实现文件
 *
 * 时钟说明：
 *   当前使用软件计时，初始时间写死为 2026-05-19 15:01:55（星期二）。
 *   每秒由 LVGL 定时器自增，精度依赖 lv_tick_inc() 的准确性。
 *   后续接入 ESP32-S3 NTP 后，只需调用 clock_set_time() 校准即可。
 */

#include "lv_player_main.h"
#include "lvgl.h"
#include "assets/icon_draw.h"
#include "../TextReader/lv_text_reader.h"
#include "../TextReader/assets/app_icons.h"
#include "font_sdram.h"
#include <stdio.h>

/* -------------------------------------------------------
 * 软件时钟结构体
 * ------------------------------------------------------- */
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;       /* 日 */
    uint8_t  weekday;   /* 1=周一 ... 7=周日 */
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} sw_clock_t;

/* 每月天数（非闰年） */
static const uint8_t days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

static sw_clock_t g_clock = {
    .year    = 2026,
    .month   = 5,
    .day     = 19,
    .weekday = 2,   /* 2026-05-19 是星期二 */
    .hour    = 15,
    .minute  = 1,
    .second  = 55
};

/* -------------------------------------------------------
 * 星期字符串（放在文件顶部，clock_set_time 和 clock_update_cb 都能用）
 * ------------------------------------------------------- */
static const char *weekday_str[] = {
    "", "\xe6\x98\x9f\xe6\x9c\x9f\xe4\xb8\x80",  /* 星期一 */
       "\xe6\x98\x9f\xe6\x9c\x9f\xe4\xba\x8c",   /* 星期二 */
       "\xe6\x98\x9f\xe6\x9c\x9f\xe4\xb8\x89",   /* 星期三 */
       "\xe6\x98\x9f\xe6\x9c\x9f\xe5\x9b\x9b",   /* 星期四 */
       "\xe6\x98\x9f\xe6\x9c\x9f\xe4\xba\x94",   /* 星期五 */
       "\xe6\x98\x9f\xe6\x9c\x9f\xe5\x85\xad",   /* 星期六 */
       "\xe6\x98\x9f\xe6\x9c\x9f\xe6\x97\xa5"    /* 星期日 */
};

static const char *weekday_short[] = {
    "", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

/* -------------------------------------------------------
 * 全局 UI 对象指针
 * ------------------------------------------------------- */
static lv_obj_t   *main_screen    = NULL;
static lv_obj_t   *status_bar     = NULL;
static lv_obj_t   *battery_icon   = NULL;
static lv_obj_t   *wifi_icon      = NULL;
static lv_obj_t   *data_icon      = NULL;
static lv_obj_t   *g_date_label   = NULL;
static lv_obj_t   *g_week_label   = NULL;
static lv_obj_t   *g_clock_label  = NULL;
static lv_obj_t   *g_sb_label     = NULL;
static lv_timer_t *g_clock_timer  = NULL;

/* -------------------------------------------------------
 * 设置时钟时间（从RTC同步时调用）
 * ------------------------------------------------------- */
void clock_set_time(uint16_t year, uint8_t month, uint8_t day, 
                    uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second)
{
    g_clock.year    = year;
    g_clock.month   = month;
    g_clock.day     = day;
    g_clock.weekday = weekday;
    g_clock.hour    = hour;
    g_clock.minute  = minute;
    g_clock.second  = second;

    /* 立即更新显示 */
    if(g_clock_label != NULL) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour, minute, second);
        lv_label_set_text(g_clock_label, buf);
    }
    if(g_date_label != NULL) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d\xe5\xb9\xb4%d\xe6\x9c\x88%d\xe6\x97\xa5", year, month, day);
        lv_label_set_text(g_date_label, buf);
    }
    if(g_week_label != NULL) {
        lv_label_set_text(g_week_label, weekday_str[weekday]);
    }
    if(g_sb_label != NULL) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d-%02d %s", month, day, weekday_short[weekday]);
        lv_label_set_text(g_sb_label, buf);
    }
}

/* -------------------------------------------------------
 * 软件时钟自增（每秒调用一次）
 * ------------------------------------------------------- */
static void sw_clock_tick(sw_clock_t *c)
{
    c->second++;
    if(c->second < 60) return;
    c->second = 0;
    c->minute++;
    if(c->minute < 60) return;
    c->minute = 0;
    c->hour++;
    if(c->hour < 24) return;
    c->hour = 0;
    /* 日期进位 */
    uint8_t max_day = days_in_month[c->month];
    /* 闰年2月 */
    if(c->month == 2) {
        uint16_t y = c->year;
        if((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) max_day = 29;
    }
    c->day++;
    c->weekday = c->weekday % 7 + 1;   /* 1-7 循环 */
    if(c->day <= max_day) return;
    c->day = 1;
    c->month++;
    if(c->month <= 12) return;
    c->month = 1;
    c->year++;
}

/* -------------------------------------------------------
 * 时钟定时器回调 - 每秒直接读RTC，精度高且上电即显示正确时间
 * ------------------------------------------------------- */
static void clock_update_cb(lv_timer_t *timer)
{
    if(g_clock_label == NULL) return;

    ntp_time_t t;
    if(ntp_sync_get_time(&t)) {
        /* RTC有有效时间，直接显示 */
        char buf[32];

        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hour, t.minute, t.second);
        lv_label_set_text(g_clock_label, buf);

        /* 同步内部软件时钟（供日期进位逻辑使用） */
        g_clock.year    = t.year;
        g_clock.month   = t.month;
        g_clock.day     = t.date;
        g_clock.weekday = t.day;
        g_clock.hour    = t.hour;
        g_clock.minute  = t.minute;
        g_clock.second  = t.second;

        /* 日期和星期（每分钟更新一次，减少刷新开销） */
        if(t.second == 0) {
            if(g_date_label != NULL) {
                snprintf(buf, sizeof(buf), "%d\xe5\xb9\xb4%d\xe6\x9c\x88%d\xe6\x97\xa5",
                         t.year, t.month, t.date);
                lv_label_set_text(g_date_label, buf);
            }
            if(g_week_label != NULL) {
                lv_label_set_text(g_week_label, weekday_str[t.day]);
            }
            if(g_sb_label != NULL) {
                snprintf(buf, sizeof(buf), "%02d-%02d %s",
                         t.month, t.date, weekday_short[t.day]);
                lv_label_set_text(g_sb_label, buf);
            }
        }
    } else {
        /* RTC尚未同步，软件自增保持界面不卡住 */
        sw_clock_tick(&g_clock);
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                 g_clock.hour, g_clock.minute, g_clock.second);
        lv_label_set_text(g_clock_label, buf);
    }
}

/* -------------------------------------------------------
 * 应用图标点击回调
 * ------------------------------------------------------- */
static void app_icon_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    uint32_t app_id = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    switch(app_id) {
        case 0:  /* 文本阅读器 */
            lv_text_reader_create(NULL);
            lv_text_reader_list_files("S:");
            break;
        default:
            break;
    }
}

/* -------------------------------------------------------
 * lv_player() - 主界面入口
 * ------------------------------------------------------- */
void lv_player(void)
{
    if(main_screen != NULL) lv_player_destroy();

    lv_coord_t screen_w = lv_disp_get_hor_res(NULL);
    lv_coord_t screen_h = lv_disp_get_ver_res(NULL);

    /* 主屏幕 */
    main_screen = lv_obj_create(NULL);
    lv_obj_set_size(main_screen, screen_w, screen_h);
    lv_obj_set_style_pad_all(main_screen, 0, 0);
    lv_obj_set_style_border_width(main_screen, 0, 0);
    lv_obj_set_style_outline_width(main_screen, 0, 0);
    lv_obj_set_style_radius(main_screen, 0, 0);
    lv_obj_clear_flag(main_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_bg_opa(main_screen, LV_OPA_COVER, 0);

    /* 状态栏 */
    status_bar = lv_obj_create(main_screen);
    lv_obj_set_size(status_bar, screen_w, 18);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_pad_all(status_bar, 0, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    battery_icon = draw_battery_icon(status_bar);
    lv_obj_align(battery_icon, LV_ALIGN_TOP_RIGHT, -10, 1);

    data_icon = draw_data_icon(status_bar);
    lv_obj_align_to(data_icon, battery_icon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

    wifi_icon = draw_wifi_icon(status_bar);
    lv_obj_align_to(wifi_icon, data_icon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

    g_sb_label = lv_label_create(status_bar);
    lv_label_set_text(g_sb_label, "05-19 Tue");
    lv_obj_set_style_text_color(g_sb_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(g_sb_label, &lv_font_montserrat_16, 0);
    lv_obj_align(g_sb_label, LV_ALIGN_LEFT_MID, 10, 0);

    /* 左侧时钟区域 */
    g_date_label = lv_label_create(main_screen);
    /* 2026年5月19日 */
    lv_label_set_text(g_date_label,
        "2026\xe5\xb9\xb4""5\xe6\x9c\x88""19\xe6\x97\xa5");
    lv_obj_set_style_text_color(g_date_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(g_date_label, font_sdram_get(), 0);
    lv_obj_align(g_date_label, LV_ALIGN_TOP_LEFT, 30, 30);

    g_week_label = lv_label_create(main_screen);
    /* 星期二 */
    lv_label_set_text(g_week_label, "\xe6\x98\x9f\xe6\x9c\x9f\xe4\xba\x8c");
    lv_obj_set_style_text_color(g_week_label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(g_week_label, font_sdram_get(), 0);
    lv_obj_align(g_week_label, LV_ALIGN_TOP_LEFT, 30, 55);

    g_clock_label = lv_label_create(main_screen);
    lv_label_set_text(g_clock_label, "15:01:55");
    lv_obj_set_style_text_color(g_clock_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(g_clock_label, &lv_font_montserrat_48, 0);
    lv_obj_align(g_clock_label, LV_ALIGN_TOP_LEFT, 30, 90);

    /* 右侧应用图标网格 */
    lv_obj_t *app_grid = lv_obj_create(main_screen);
    lv_obj_set_size(app_grid, 360, 360);
    lv_obj_align(app_grid, LV_ALIGN_RIGHT_MID, -30, 0);
    lv_obj_set_style_bg_opa(app_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(app_grid, 0, 0);
    lv_obj_set_style_pad_all(app_grid, 0, 0);
    lv_obj_clear_flag(app_grid, LV_OBJ_FLAG_SCROLLABLE);

    const uint32_t app_colors[] = {
        0x3D7EBF,  /* 0 文本阅读 */
        0x2E8B57,  /* 1 图片浏览 */
        0x8B3A8B,  /* 2 音乐播放 */
        0x1A6EA8,  /* 3 天气     */
        0x1A5276,  /* 4 AI对话   */
        0x117A65,  /* 5 WiFi管理 */
        0xB03A2E,  /* 6 温湿度   */
        0x4A4A4A,  /* 7 设置     */
        0x7D3C98   /* 8 画画     */
    };

    for(int row = 0; row < 3; row++) {
        for(int col = 0; col < 3; col++) {
            int index = row * 3 + col;

            lv_obj_t *app_btn = lv_btn_create(app_grid);
            lv_obj_set_size(app_btn, 100, 100);
            lv_obj_set_pos(app_btn, col * 120 + 10, row * 120 + 10);
            lv_obj_set_style_radius(app_btn, 18, 0);
            lv_obj_set_style_bg_color(app_btn, lv_color_hex(app_colors[index]), 0);
            lv_obj_set_style_shadow_width(app_btn, 12, 0);
            lv_obj_set_style_shadow_color(app_btn, lv_color_hex(0x000000), 0);
            lv_obj_set_style_shadow_opa(app_btn, LV_OPA_40, 0);
            lv_obj_set_style_shadow_ofs_y(app_btn, 4, 0);
            lv_obj_clear_flag(app_btn, LV_OBJ_FLAG_PRESS_LOCK);
            lv_obj_clear_flag(app_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
            lv_obj_add_event_cb(app_btn, app_icon_event_cb,
                                LV_EVENT_CLICKED, (void*)(uintptr_t)index);

            lv_obj_t *icon = NULL;
            switch(index) {
                case 0: icon = draw_text_reader_icon  (app_btn, 60); break;
                case 1: icon = draw_photo_browser_icon(app_btn, 60); break;
                case 2: icon = draw_music_player_icon (app_btn, 60); break;
                case 3: icon = draw_weather_icon      (app_btn, 60); break;
                case 4: icon = draw_ai_chat_icon      (app_btn, 60); break;
                case 5: icon = draw_wifi_mgr_icon     (app_btn, 60); break;
                case 6: icon = draw_temp_humidity_icon(app_btn, 60); break;
                case 7: icon = draw_settings_icon     (app_btn, 60); break;
                case 8: icon = draw_paint_icon        (app_btn, 60); break;
            }
            if(icon != NULL) lv_obj_center(icon);
        }
    }

    /* 启动时钟定时器，每1000ms自增一秒 */
    g_clock_timer = lv_timer_create(clock_update_cb, 1000, NULL);

    lv_scr_load(main_screen);
}

/* -------------------------------------------------------
 * lv_player_destroy() - 销毁主界面，释放所有资源
 * ------------------------------------------------------- */
void lv_player_destroy(void)
{
    if(g_clock_timer != NULL) {
        lv_timer_del(g_clock_timer);
        g_clock_timer = NULL;
    }
    g_date_label  = NULL;
    g_week_label  = NULL;
    g_clock_label = NULL;
    g_sb_label    = NULL;

    if(main_screen != NULL) {
        lv_obj_del(main_screen);
        main_screen  = NULL;
        status_bar   = NULL;
        battery_icon = NULL;
        wifi_icon    = NULL;
        data_icon    = NULL;
    }
}
