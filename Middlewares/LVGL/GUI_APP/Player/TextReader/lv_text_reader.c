/**
 * @file lv_text_reader.c
 * @brief 文本阅读器 - 文件列表 + 点击打开
 *
 * 编码说明：
 *   Windows 复制的 .txt 文件默认是 GBK 编码
 *   LVGL 显示需要 UTF-8 编码
 *   读取文件后自动将 GBK 转换为 UTF-8
 */

#include "lv_text_reader.h"
#include "lvgl.h"
#include "../MainPage/assets/icon_draw.h"
#include "font_sdram.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * 配置
 * ============================================================ */
#define SD_ROOT_PATH        "S:/"
#define MAX_FILES           64
#define MAX_FILENAME_LEN    128     /* 原始文件名长度（GBK编码） */
#define MAX_FILENAME_UTF8   256     /* UTF-8 文件名长度（中文需要3倍空间） */
#define READ_BUF_SIZE       4096    /* 单次读取块大小 */
#define UTF8_BUF_SIZE       8192    /* UTF-8 转换缓冲区 */
#define PAGE_SIZE           2048    /* 每页显示的字节数（GBK编码） */

/* ============================================================
 * 内部状态
 * ============================================================ */
typedef enum {
    VIEW_FILE_LIST = 0,     /* 文件列表界面 */
    VIEW_FILE_CONTENT,      /* 文件内容界面 */
} reader_view_t;

static lv_obj_t    *g_screen       = NULL;
static lv_obj_t    *g_title_label  = NULL;
static lv_obj_t    *g_content_area = NULL;  /* 列表或文本的容器 */
static reader_view_t g_view        = VIEW_FILE_LIST;

/* 文件列表 */
static char g_filenames[MAX_FILES][MAX_FILENAME_UTF8];     /* UTF-8 显示用 */
static char g_filenames_raw[MAX_FILES][MAX_FILENAME_LEN];  /* 原始GBK，用于打开文件 */
static int  g_file_count = 0;

/* 文件内容缓冲区（静态分配，避免堆碎片） */
static char g_read_buf[READ_BUF_SIZE];

/* 分页阅读状态 */
static FIL  g_current_file;         /* 当前打开的文件 */
static bool g_file_opened = false;  /* 文件是否打开 */
static FSIZE_t g_file_size = 0;     /* 文件总大小 */
static FSIZE_t g_current_pos = 0;   /* 当前读取位置 */
static char g_current_filename[MAX_FILENAME_LEN]; /* 当前文件名（GBK） */
static lv_obj_t *g_text_label = NULL; /* 文本显示标签 */

/* ============================================================
 * GBK/GB2312 转 UTF-8 - 简化版本
 * 直接转换，不依赖复杂的码表
 * ============================================================ */

static int gbk_to_utf8(const char *gbk, int gbk_len, char *utf8, int utf8_max)
{
    int i = 0, j = 0;
    
    /* 检查 UTF-8 BOM */
    if (gbk_len >= 3 &&
        (unsigned char)gbk[0] == 0xEF &&
        (unsigned char)gbk[1] == 0xBB &&
        (unsigned char)gbk[2] == 0xBF) {
        /* 已经是 UTF-8，直接复制（去掉BOM） */
        int copy_len = gbk_len - 3;
        if (copy_len >= utf8_max) copy_len = utf8_max - 1;
        memcpy(utf8, gbk + 3, copy_len);
        utf8[copy_len] = '\0';
        return copy_len;
    }

    /* GBK → UTF-8 转换 */
    while (i < gbk_len && j < utf8_max - 4) {
        unsigned char c = (unsigned char)gbk[i];

        if (c < 0x80) {
            /* ASCII 字符 */
            utf8[j++] = gbk[i++];
        } else if (i + 1 < gbk_len) {
            /* GBK 双字节字符 */
            unsigned char c2 = (unsigned char)gbk[i + 1];
            
            /* 使用 FatFS 的转换函数 */
            WCHAR gbk_code = ((WCHAR)c << 8) | c2;
            WCHAR uc = ff_oem2uni(gbk_code, 936);
            
            if (uc != 0 && uc != 0xFFFF) {
                /* 转换成功，Unicode → UTF-8 */
                if (uc < 0x80) {
                    utf8[j++] = (char)uc;
                } else if (uc < 0x800) {
                    utf8[j++] = (char)(0xC0 | (uc >> 6));
                    utf8[j++] = (char)(0x80 | (uc & 0x3F));
                } else {
                    utf8[j++] = (char)(0xE0 | (uc >> 12));
                    utf8[j++] = (char)(0x80 | ((uc >> 6) & 0x3F));
                    utf8[j++] = (char)(0x80 | (uc & 0x3F));
                }
                i += 2;
            } else {
                /* 转换失败，保留原始字节 */
                utf8[j++] = gbk[i++];
            }
        } else {
            /* 最后一个字节 */
            utf8[j++] = gbk[i++];
        }
    }
    utf8[j] = '\0';
    return j;
}

static char g_utf8_buf[UTF8_BUF_SIZE];

/* ============================================================
 * 前向声明
 * ============================================================ */
static void show_file_list(void);
static void show_file_content(const char *filename);
static void return_btn_cb(lv_event_t *e);
static void file_item_cb(lv_event_t *e);

/* ============================================================
 * 创建顶部工具栏（返回按钮 + 标题）
 * ============================================================ */
static void create_toolbar(lv_obj_t *parent, lv_coord_t w)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, w, 50);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 返回按钮 */
    lv_obj_t *btn = draw_return_icon(bar);
    lv_obj_align(btn, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, return_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 标题 */
    g_title_label = lv_label_create(bar);
    lv_label_set_text(g_title_label, "Text Reader");
    lv_obj_set_style_text_color(g_title_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(g_title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(g_title_label, LV_ALIGN_CENTER, 0, 0);
}

/* ============================================================
 * 返回按钮回调
 * ============================================================ */
static void return_btn_cb(lv_event_t *e)
{
    if (g_view == VIEW_FILE_CONTENT) {
        /* 文件内容 → 文件列表 */
        show_file_list();
    } else {
        /* 文件列表 → 主界面 */
        lv_text_reader_destroy();
        extern void lv_player(void);
        lv_player();
    }
}

/* ============================================================
 * 文件列表项点击回调
 * ============================================================ */
static void file_item_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < g_file_count) {
        show_file_content(g_filenames_raw[idx]);  /* 用原始GBK文件名打开 */
    }
}

/* ============================================================
 * 显示文件列表
 * ============================================================ */
static void show_file_list(void)
{
    g_view = VIEW_FILE_LIST;
    lv_label_set_text(g_title_label, "Text Reader");

    /* 清空内容区 */
    lv_obj_clean(g_content_area);

    /* 扫描 SD 卡根目录 */
    g_file_count = 0;
    DIR dir;
    FILINFO fno;
    if (f_opendir(&dir, SD_ROOT_PATH) == FR_OK) {
        while (g_file_count < MAX_FILES) {
            if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == 0) break;
            if (fno.fattrib & AM_DIR) continue;
            const char *ext = strrchr(fno.fname, '.');
            if (ext && (strcasecmp(ext, ".txt") == 0)) {
                /* 保存原始文件名（用于打开文件） */
                strncpy(g_filenames_raw[g_file_count], fno.fname, MAX_FILENAME_LEN - 1);
                g_filenames_raw[g_file_count][MAX_FILENAME_LEN - 1] = '\0';
                
                /* 文件名 GBK→UTF-8 转换后存储（用于显示） */
                gbk_to_utf8(fno.fname, strlen(fno.fname), 
                           g_filenames[g_file_count], MAX_FILENAME_UTF8);
                g_filenames[g_file_count][MAX_FILENAME_UTF8 - 1] = '\0';
                
                g_file_count++;
            }
        }
        f_closedir(&dir);
    }

    if (g_file_count == 0) {
        /* 没有文件 */
        lv_obj_t *hint = lv_label_create(g_content_area);
        lv_label_set_text(hint, "SD卡根目录没有 .txt 文件\n\n请将文本文件复制到SD卡根目录");
        lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
        lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* 创建文件列表 */
    for (int i = 0; i < g_file_count; i++) {
        lv_obj_t *item = lv_obj_create(g_content_area);
        lv_obj_set_size(item, lv_pct(100), 52);
        lv_obj_set_style_bg_color(item, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(item, 0, LV_PART_MAIN);
        lv_obj_set_style_border_color(item, lv_color_hex(0xe0e0e0), 0);
        lv_obj_set_style_border_side(item, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(item, 1, 0);
        lv_obj_set_style_radius(item, 0, 0);
        lv_obj_set_style_pad_left(item, 16, 0);
        lv_obj_set_style_pad_right(item, 16, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

        /* 按下效果 */
        lv_obj_set_style_bg_color(item, lv_color_hex(0xe8f4fd), LV_STATE_PRESSED);

        /* 文件图标 */
        lv_obj_t *icon = draw_file_icon(item);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

        /* 文件名标签 - 使用支持中文的字体 */
        lv_obj_t *name_lbl = lv_label_create(item);
        lv_label_set_text(name_lbl, g_filenames[i]);
        lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name_lbl, lv_pct(70));
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(0x2c3e50), 0);
        lv_obj_set_style_text_font(name_lbl, font_sdram_get(), 0);
        lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 36, 0);

        /* 箭头 */
        lv_obj_t *arrow = lv_label_create(item);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(arrow, lv_color_hex(0xaaaaaa), 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, 0, 0);

        /* 点击事件 */
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(item, file_item_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

/* ============================================================
 * 显示文件内容（优化版 - 分页加载）
 * ============================================================ */

/* 关闭当前打开的文件 */
static void close_current_file(void)
{
    if (g_file_opened) {
        f_close(&g_current_file);
        g_file_opened = false;
    }
}

/* 加载下一页内容 */
static bool load_next_page(void)
{
    if (!g_file_opened || g_current_pos >= g_file_size) {
        return false; /* 已到文件末尾 */
    }

    /* 读取一页数据 */
    UINT bytes_read = 0;
    memset(g_read_buf, 0, READ_BUF_SIZE);
    
    UINT to_read = (g_file_size - g_current_pos < PAGE_SIZE) ? 
                   (UINT)(g_file_size - g_current_pos) : PAGE_SIZE;
    
    FRESULT res = f_read(&g_current_file, g_read_buf, to_read, &bytes_read);
    if (res != FR_OK || bytes_read == 0) {
        return false;
    }

    g_current_pos += bytes_read;
    g_read_buf[bytes_read] = '\0';

    /* GBK → UTF-8 转换 */
    gbk_to_utf8(g_read_buf, (int)bytes_read, g_utf8_buf, UTF8_BUF_SIZE);

    /* 追加到现有文本 */
    if (g_text_label) {
        const char *old_text = lv_label_get_text(g_text_label);
        static char combined[UTF8_BUF_SIZE * 2];
        snprintf(combined, sizeof(combined), "%s%s", old_text, g_utf8_buf);
        lv_label_set_text(g_text_label, combined);
    }

    return true;
}

/* 滚动事件回调 - 接近底部时自动加载下一页 */
static void scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *scroll_cont = lv_event_get_target(e);
    lv_coord_t scroll_y = lv_obj_get_scroll_y(scroll_cont);
    lv_coord_t scroll_h = lv_obj_get_scroll_bottom(scroll_cont);
    
    /* 当滚动到距离底部 100px 时，加载下一页 */
    if (scroll_h - scroll_y < 100) {
        load_next_page();
    }
}

static void show_file_content(const char *filename)
{
    g_view = VIEW_FILE_CONTENT;

    /* 关闭之前打开的文件 */
    close_current_file();

    /* 标题显示 UTF-8 文件名（从原始GBK转换） */
    static char title_utf8[MAX_FILENAME_UTF8];
    gbk_to_utf8(filename, strlen(filename), title_utf8, sizeof(title_utf8));
    lv_label_set_text(g_title_label, title_utf8);

    /* 清空内容区 */
    lv_obj_clean(g_content_area);

    /* 构造完整路径（用原始GBK文件名） */
    char path[MAX_FILENAME_LEN + 8];
    snprintf(path, sizeof(path), "%s%s", SD_ROOT_PATH, filename);
    strncpy(g_current_filename, filename, MAX_FILENAME_LEN - 1);

    /* 打开文件 */
    FRESULT res = f_open(&g_current_file, path, FA_READ);
    if (res != FR_OK) {
        snprintf(g_utf8_buf, UTF8_BUF_SIZE, "无法打开文件: %s (错误 %d)", path, res);
        lv_obj_t *err_label = lv_label_create(g_content_area);
        lv_label_set_text(err_label, g_utf8_buf);
        return;
    }

    g_file_opened = true;
    g_file_size = f_size(&g_current_file);
    g_current_pos = 0;

    /* 创建滚动容器 */
    lv_obj_t *scroll_cont = lv_obj_create(g_content_area);
    lv_obj_set_size(scroll_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(scroll_cont, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(scroll_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scroll_cont, 0, 0);
    lv_obj_set_style_radius(scroll_cont, 0, 0);
    lv_obj_set_style_pad_all(scroll_cont, 12, 0);
    lv_obj_set_scroll_dir(scroll_cont, LV_DIR_VER);
    
    /* 添加滚动事件监听 */
    lv_obj_add_event_cb(scroll_cont, scroll_event_cb, LV_EVENT_SCROLL, NULL);

    /* 创建文本标签 */
    g_text_label = lv_label_create(scroll_cont);
    lv_label_set_long_mode(g_text_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_text_label, lv_pct(100));
    lv_obj_set_style_text_color(g_text_label, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_font(g_text_label, font_sdram_get(), 0);

    /* 加载第一页 */
    UINT bytes_read = 0;
    memset(g_read_buf, 0, READ_BUF_SIZE);
    
    UINT to_read = (g_file_size < PAGE_SIZE) ? (UINT)g_file_size : PAGE_SIZE;
    res = f_read(&g_current_file, g_read_buf, to_read, &bytes_read);
    
    if (res == FR_OK && bytes_read > 0) {
        g_current_pos = bytes_read;
        g_read_buf[bytes_read] = '\0';
        
        /* GBK → UTF-8 转换 */
        gbk_to_utf8(g_read_buf, (int)bytes_read, g_utf8_buf, UTF8_BUF_SIZE);
        lv_label_set_text(g_text_label, g_utf8_buf);
    } else {
        lv_label_set_text(g_text_label, "读取文件失败");
        close_current_file();
    }
}

/* ============================================================
 * 公开接口
 * ============================================================ */

/**
 * @brief 创建文本阅读器界面并显示文件列表
 */
lv_obj_t *lv_text_reader_create(lv_obj_t *parent_screen)
{
    if (g_screen != NULL) lv_text_reader_destroy();

    lv_coord_t sw = lv_disp_get_hor_res(NULL);
    lv_coord_t sh = lv_disp_get_ver_res(NULL);

    /* 主屏幕 */
    g_screen = lv_obj_create(NULL);
    lv_obj_set_size(g_screen, sw, sh);
    lv_obj_set_style_bg_color(g_screen, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(g_screen, 0, 0);
    lv_obj_set_style_border_width(g_screen, 0, 0);
    lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 工具栏 */
    create_toolbar(g_screen, sw);

    /* 内容区（工具栏下方） */
    g_content_area = lv_obj_create(g_screen);
    lv_obj_set_size(g_content_area, sw, sh - 50);
    lv_obj_align(g_content_area, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(g_content_area, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_style_bg_opa(g_content_area, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_content_area, 0, 0);
    lv_obj_set_style_radius(g_content_area, 0, 0);
    lv_obj_set_style_pad_all(g_content_area, 0, 0);
    lv_obj_set_flex_flow(g_content_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_content_area, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_scr_load(g_screen);

    /* 显示文件列表 */
    show_file_list();

    return g_screen;
}

/**
 * @brief 列出文件（兼容旧接口，直接调用 show_file_list）
 */
int lv_text_reader_list_files(const char *dir_path)
{
    (void)dir_path;
    if (g_screen == NULL) return -1;
    show_file_list();
    return g_file_count;
}

/**
 * @brief 加载并显示指定文件
 */
int lv_text_reader_load_file(const char *file_path)
{
    if (g_screen == NULL) return -1;
    /* 从路径中提取文件名 */
    const char *name = strrchr(file_path, '/');
    name = name ? name + 1 : file_path;
    show_file_content(name);
    return 0;
}

/**
 * @brief 销毁文本阅读器
 */
void lv_text_reader_destroy(void)
{
    /* 关闭打开的文件 */
    close_current_file();
    
    if (g_screen != NULL) {
        lv_obj_del(g_screen);
        g_screen       = NULL;
        g_title_label  = NULL;
        g_content_area = NULL;
        g_text_label   = NULL;
    }
    g_file_count = 0;
    g_view = VIEW_FILE_LIST;
    memset(g_filenames_raw, 0, sizeof(g_filenames_raw));
}

/**
 * @brief 清空内容
 */
void lv_text_reader_clear(void)
{
    if (g_content_area != NULL) lv_obj_clean(g_content_area);
}
