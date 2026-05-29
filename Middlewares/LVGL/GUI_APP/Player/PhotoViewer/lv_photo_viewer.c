/**
 * @file lv_photo_viewer.c
 * @brief 相册浏览器实现
 *
 * 布局（800x480）：
 *   顶部 50px：工具栏（返回键 + 标题 + 图片计数）
 *   中间区域：图片居中显示，自适应缩放
 *   左右两侧：圆形箭头按钮（悬浮在图片上）
 *   底部 30px：文件名标签
 *
 * 图片目录：SD卡 S:/PHOTO/（支持 .bmp .jpg .jpeg）
 * 图片通过 LVGL lv_img 控件显示，路径格式 "S:/PHOTO/xxx.jpg"
 */

#include "lv_photo_viewer.h"
#include "../MainPage/assets/icon_draw.h"
#include "font_sdram.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * 配置
 * ============================================================ */
/* 扫描目录列表：只扫 PHOTO 子目录 */
static const char * const SCAN_DIRS[] = {
    "S:/PHOTO",     /* PHOTO子目录 */
    NULL
};
#define MAX_PHOTOS          64      /* 减少：256→64，节省 24KB */
#define MAX_PATH_LEN        96      /* 减少：128→96，节省 8KB */

/* 箭头按钮尺寸 */
#define ARROW_BTN_SIZE      56
#define ARROW_BTN_MARGIN    16

/* ============================================================
 * 内部状态
 * ============================================================ */
static lv_obj_t *g_screen       = NULL;
static lv_obj_t *g_title_label  = NULL;
static lv_obj_t *g_count_label  = NULL;   /* "1 / 12" */
static lv_obj_t *g_img_obj      = NULL;   /* lv_img 控件 */
static lv_obj_t *g_name_label   = NULL;   /* 底部文件名 */
static lv_obj_t *g_btn_prev     = NULL;   /* 左箭头 */
static lv_obj_t *g_btn_next     = NULL;   /* 右箭头 */
static lv_obj_t *g_hint_label   = NULL;   /* 无图片提示 */

/* 图片文件列表 */
static char g_photo_paths[MAX_PHOTOS][MAX_PATH_LEN];
static int  g_photo_count = 0;
static int  g_current_idx = 0;

/* 当前图片路径（lv_img_set_src 需要持久有效的字符串） */
static char g_current_path[MAX_PATH_LEN];

/* ============================================================
 * 前向声明
 * ============================================================ */
static void show_photo(int idx);
static void prev_btn_cb(lv_event_t *e);
static void next_btn_cb(lv_event_t *e);
static void return_btn_cb(lv_event_t *e);
static void update_arrow_visibility(void);

/* ============================================================
 * 工具：判断是否是支持的图片格式
 * ============================================================ */
static bool is_image_file(const char *name)
{
    const char *ext = strrchr(name, '.');
    if (!ext) return false;
    /* 不区分大小写比较 */
    char lower[8] = {0};
    for (int i = 0; i < 7 && ext[i]; i++) {
        char c = ext[i];
        lower[i] = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
    }
    return (strcmp(lower, ".bmp")  == 0 ||
            strcmp(lower, ".jpg")  == 0 ||
            strcmp(lower, ".jpeg") == 0);
}

/* ============================================================
 * 扫描SD卡图片目录
 * ============================================================ */

/* 字符串比较用于排序 */
static int path_compare(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

static int scan_photos(void)
{
    g_photo_count = 0;
    DIR dir;
    FILINFO fno;

    for (int d = 0; SCAN_DIRS[d] != NULL && g_photo_count < MAX_PHOTOS; d++) {
        FRESULT fr = f_opendir(&dir, SCAN_DIRS[d]);
        if (fr != FR_OK) {
            continue;
        }

        while (g_photo_count < MAX_PHOTOS) {
            if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == 0) break;
            if (fno.fattrib & AM_DIR) continue;
            if (!is_image_file(fno.fname)) continue;

            const char *dir_path = SCAN_DIRS[d];
            if (dir_path[strlen(dir_path) - 1] == '/') {
                snprintf(g_photo_paths[g_photo_count], MAX_PATH_LEN,
                         "%s%s", dir_path, fno.fname);
            } else {
                snprintf(g_photo_paths[g_photo_count], MAX_PATH_LEN,
                         "%s/%s", dir_path, fno.fname);
            }
            g_photo_count++;
        }
        f_closedir(&dir);
    }

    /* 按文件名排序，保证顺序一致 */
    if (g_photo_count > 1) {
        qsort(g_photo_paths, g_photo_count, MAX_PATH_LEN, path_compare);
    }

    return g_photo_count;
}

/* 错误提示 label（全局，方便清除） */
static lv_obj_t *g_err_label = NULL;

/* ============================================================
 * 显示指定索引的图片
 * ============================================================ */
static void show_photo(int idx)
{
    if (idx < 0 || idx >= g_photo_count) return;
    g_current_idx = idx;

    /* 清除上次的错误提示 */
    if (g_err_label != NULL) {
        lv_obj_del(g_err_label);
        g_err_label = NULL;
    }

    /* 更新路径，扩展名转小写 */
    strncpy(g_current_path, g_photo_paths[idx], MAX_PATH_LEN - 1);
    g_current_path[MAX_PATH_LEN - 1] = '\0';
    char *ext = strrchr(g_current_path, '.');
    if (ext) {
        for (char *p = ext; *p; p++) {
            if (*p >= 'A' && *p <= 'Z') *p += 32;
        }
    }

    /* 更新标签 */
    char buf[32];
    snprintf(buf, sizeof(buf), "%d / %d", idx + 1, g_photo_count);
    lv_label_set_text(g_count_label, buf);

    const char *fname = strrchr(g_current_path, '/');
    fname = fname ? fname + 1 : g_current_path;
    lv_label_set_text(g_name_label, fname);

    /* 先读图片头，获取原始尺寸，用于计算缩放 */
    lv_img_header_t header;

    /* 诊断：直接用 FATFS 读文件前16字节，确认文件可读且是标准JPEG */
    {
        lv_fs_file_t f;
        uint8_t magic[4] = {0};
        uint32_t rn = 0;
        lv_fs_res_t fres = lv_fs_open(&f, g_current_path, LV_FS_MODE_RD);
        if (fres == LV_FS_RES_OK) {
            lv_fs_read(&f, magic, 4, &rn);
            lv_fs_close(&f);
            printf("[PHOTO] file open OK, magic=%02X %02X %02X %02X rn=%d\r\n",
                   magic[0], magic[1], magic[2], magic[3], (int)rn);
        } else {
            printf("[PHOTO] file open FAILED res=%d\r\n", (int)fres);
        }
    }

    lv_res_t info_res = lv_img_decoder_get_info(g_current_path, &header);
    printf("[PHOTO] %s  info=%d  %dx%d\r\n",
           g_current_path, (int)info_res, (int)header.w, (int)header.h);

    /* 加载新图，不缩放，先确认能显示 */
    lv_img_set_zoom(g_img_obj, 256);
    lv_img_set_offset_x(g_img_obj, 0);
    lv_img_set_offset_y(g_img_obj, 0);
    lv_img_set_angle(g_img_obj, 0);
    lv_img_set_src(g_img_obj, g_current_path);

    lv_obj_align(g_img_obj, LV_ALIGN_CENTER, 0, 0);
    update_arrow_visibility();
}

/* ============================================================
 * 更新箭头按钮可见性
 * ============================================================ */
static void update_arrow_visibility(void)
{
    if (g_photo_count <= 1) {
        lv_obj_add_flag(g_btn_prev, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_btn_next, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    /* 第一张：隐藏左箭头 */
    if (g_current_idx == 0) {
        lv_obj_add_flag(g_btn_prev, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(g_btn_prev, LV_OBJ_FLAG_HIDDEN);
    }

    /* 最后一张：隐藏右箭头 */
    if (g_current_idx == g_photo_count - 1) {
        lv_obj_add_flag(g_btn_next, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(g_btn_next, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ============================================================
 * 回调函数
 * ============================================================ */
static void prev_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (g_current_idx > 0) {
        show_photo(g_current_idx - 1);
    }
}

static void next_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (g_current_idx < g_photo_count - 1) {
        show_photo(g_current_idx + 1);
    }
}

static void return_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_photo_viewer_destroy();
    extern void lv_player(void);
    lv_player();
}

/* ============================================================
 * 创建圆形箭头按钮
 * ============================================================ */
static lv_obj_t *create_arrow_btn(lv_obj_t *parent, bool is_left,
                                   lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, ARROW_BTN_SIZE, ARROW_BTN_SIZE);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3498db), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_80, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_PRESS_LOCK);

    /* 箭头符号 */
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, is_left ? LV_SYMBOL_LEFT : LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

/* ============================================================
 * 公开接口
 * ============================================================ */
lv_obj_t *lv_photo_viewer_create(void)
{
    if (g_screen != NULL) lv_photo_viewer_destroy();

    /* 进入前清理内存 */
    lv_img_cache_invalidate_src(NULL);

    lv_coord_t sw = lv_disp_get_hor_res(NULL);
    lv_coord_t sh = lv_disp_get_ver_res(NULL);

    /* ---- 主屏幕 ---- */
    g_screen = lv_obj_create(NULL);
    lv_obj_set_size(g_screen, sw, sh);
    lv_obj_set_style_bg_color(g_screen, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(g_screen, 0, 0);
    lv_obj_set_style_border_width(g_screen, 0, 0);
    lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- 顶部工具栏（与文本阅读器一致） ---- */
    lv_obj_t *bar = lv_obj_create(g_screen);
    lv_obj_set_size(bar, sw, 50);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 返回按钮 */
    lv_obj_t *ret_btn = draw_return_icon(bar);
    lv_obj_align(ret_btn, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_add_flag(ret_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ret_btn, return_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 标题 */
    g_title_label = lv_label_create(bar);
    lv_label_set_text(g_title_label, "Photo Viewer");
    lv_obj_set_style_text_color(g_title_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(g_title_label, font_sdram_get(), 0);
    lv_obj_align(g_title_label, LV_ALIGN_CENTER, 0, 0);

    /* 图片计数（右侧） */
    g_count_label = lv_label_create(bar);
    lv_label_set_text(g_count_label, "0 / 0");
    lv_obj_set_style_text_color(g_count_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(g_count_label, &lv_font_montserrat_14, 0);
    lv_obj_align(g_count_label, LV_ALIGN_RIGHT_MID, -12, 0);

    /* ---- 底部文件名标签 ---- */
    g_name_label = lv_label_create(g_screen);
    lv_label_set_text(g_name_label, "");
    lv_label_set_long_mode(g_name_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(g_name_label, sw - 40);
    lv_obj_set_style_text_color(g_name_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(g_name_label, &lv_font_montserrat_14, 0);
    lv_obj_align(g_name_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* ---- 图片显示区域 ---- */
    lv_coord_t img_area_h = sh - 50 - 30;  /* 减去工具栏和底部标签 */
    lv_obj_t *img_area = lv_obj_create(g_screen);
    lv_obj_set_size(img_area, sw, img_area_h);
    lv_obj_align(img_area, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(img_area, lv_color_hex(0x0d0d1a), 0);
    lv_obj_set_style_bg_opa(img_area, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(img_area, 0, 0);
    lv_obj_set_style_radius(img_area, 0, 0);
    lv_obj_set_style_pad_all(img_area, 0, 0);
    lv_obj_clear_flag(img_area, LV_OBJ_FLAG_SCROLLABLE);

    /* lv_img 控件 */
    g_img_obj = lv_img_create(img_area);
    lv_obj_align(g_img_obj, LV_ALIGN_CENTER, 0, 0);

    /* ---- 左右箭头按钮（悬浮在图片区域上） ---- */
    g_btn_prev = create_arrow_btn(img_area, true,  prev_btn_cb);
    lv_obj_align(g_btn_prev, LV_ALIGN_LEFT_MID, ARROW_BTN_MARGIN, 0);

    g_btn_next = create_arrow_btn(img_area, false, next_btn_cb);
    lv_obj_align(g_btn_next, LV_ALIGN_RIGHT_MID, -ARROW_BTN_MARGIN, 0);

    lv_scr_load(g_screen);

    /* ---- 扫描图片 ---- */
    scan_photos();

    if (g_photo_count == 0) {
        /* 无图片提示 */
        g_hint_label = lv_label_create(img_area);
        lv_label_set_text(g_hint_label,
            "No photos found\n\n"
            "Put .jpg or .bmp files in:\n"
            "SD card root  or  /PHOTO/");
        lv_obj_set_style_text_color(g_hint_label, lv_color_hex(0x555577), 0);
        lv_obj_set_style_text_font(g_hint_label, font_sdram_get(), 0);
        lv_obj_set_style_text_align(g_hint_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(g_hint_label, LV_ALIGN_CENTER, 0, 0);

        lv_label_set_text(g_count_label, "0 / 0");
        lv_obj_add_flag(g_btn_prev, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_btn_next, LV_OBJ_FLAG_HIDDEN);
    } else {
        /* 显示第一张 */
        show_photo(0);
    }

    return g_screen;
}

void lv_photo_viewer_destroy(void)
{
    if (g_screen != NULL) {
        lv_obj_del(g_screen);
        g_screen      = NULL;
        g_title_label = NULL;
        g_count_label = NULL;
        g_img_obj     = NULL;
        g_name_label  = NULL;
        g_btn_prev    = NULL;
        g_btn_next    = NULL;
        g_hint_label  = NULL;
        g_err_label   = NULL;
    }
    g_photo_count = 0;
    g_current_idx = 0;
}
