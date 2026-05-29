/**
 * @file lv_text_reader.c
 * @brief 文本阅读器 - 文件列表 + 点击打开
 *
 * 编码说明：
 *   自动检测文件编码（UTF-8 BOM / UTF-8 无BOM / GBK）
 *   GBK 通过 ff_oem2uni() 转换为 UTF-8 后传给 LVGL
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
#define SD_ROOT_PATH        "S:/TEXT/"
#define MAX_FILES           32      /* 减少：64→32，节省 12KB */
#define MAX_FILENAME_LEN    96      /* 减少：128→96，节省 2KB */
#define MAX_FILENAME_UTF8   192     /* 减少：256→192，节省 2KB */
#define READ_BUF_SIZE       4096    /* 读取缓冲区 */
#define UTF8_BUF_SIZE       12288   /* UTF-8缓冲区，GBK最坏3倍膨胀：4096*3 */
#define PAGE_SIZE           4096    /* 每次读取4KB */

/* ============================================================
 * 内部状态
 * ============================================================ */
typedef enum { VIEW_FILE_LIST = 0, VIEW_FILE_CONTENT } reader_view_t;

static lv_obj_t      *g_screen       = NULL;
static lv_obj_t      *g_title_label  = NULL;
static lv_obj_t      *g_content_area = NULL;
static reader_view_t  g_view         = VIEW_FILE_LIST;

static char g_filenames[MAX_FILES][MAX_FILENAME_UTF8];
static char g_filenames_raw[MAX_FILES][MAX_FILENAME_LEN];
static int  g_file_count = 0;

/* 大缓冲区放到D2 SRAM（0x30000000，288KB），避免占用D1 SRAM */
static char    g_read_buf[READ_BUF_SIZE + 1] __attribute__((section(".RAM_D2")));
static char    g_utf8_buf[UTF8_BUF_SIZE]     __attribute__((section(".RAM_D2")));

static FIL     g_current_file;
static bool    g_file_opened = false;
static FSIZE_t g_file_size   = 0;
static FSIZE_t g_current_pos = 0;
static char    g_current_filename[MAX_FILENAME_LEN];
static lv_obj_t *g_text_label = NULL;

/* 文件编码类型（第一页检测后保存，后续页直接使用） */
typedef enum { ENC_UNKNOWN = 0, ENC_GBK, ENC_UTF8 } file_enc_t;
static file_enc_t g_file_enc = ENC_UNKNOWN;

/* UTF-8 分页时，上一页末尾可能有不完整的多字节序列，保存在这里 */
static uint8_t g_gbk_carry      = 0;    /* GBK：上页遗留的高字节 */
static uint8_t g_utf8_carry[4]  = {0};  /* UTF-8：上页遗留的不完整字节 */
static int     g_utf8_carry_len = 0;    /* 遗留字节数 */

/* ============================================================
 * 编码转换
 * ============================================================ */

/* GBK双字节 → UTF-8，写入dst，返回写入字节数 */
static int gbk_char_to_utf8(unsigned char hi, unsigned char lo, char *dst)
{
    WCHAR uc = ff_oem2uni(((WCHAR)hi << 8) | lo, 936);
    if (uc == 0 || uc == 0xFFFF) {
        dst[0] = '?';
        return 1;
    }
    if (uc < 0x80) {
        dst[0] = (char)uc;
        return 1;
    } else if (uc < 0x800) {
        dst[0] = (char)(0xC0 | (uc >> 6));
        dst[1] = (char)(0x80 | (uc & 0x3F));
        return 2;
    } else {
        dst[0] = (char)(0xE0 | (uc >> 12));
        dst[1] = (char)(0x80 | ((uc >> 6) & 0x3F));
        dst[2] = (char)(0x80 | (uc & 0x3F));
        return 3;
    }
}

/* 纯GBK转UTF-8（不做编码检测）
 * carry_in:  上一页遗留的高字节（0=无）
 * carry_out: 本页末尾遗留的高字节（0=无）*/
static int do_gbk_to_utf8(const char *src, int src_len, char *dst, int dst_max,
                           uint8_t carry_in, uint8_t *carry_out)
{
    int i = 0, j = 0;
    if(carry_out) *carry_out = 0;

    /* 处理上页遗留的高字节 */
    if(carry_in != 0 && src_len > 0) {
        unsigned char c2 = (unsigned char)src[0];
        if(c2 >= 0x40 && c2 != 0xFF) {
            char tmp[4];
            int n = gbk_char_to_utf8(carry_in, c2, tmp);
            for(int k = 0; k < n && j < dst_max - 4; k++) dst[j++] = tmp[k];
        } else {
            dst[j++] = '?';
        }
        i = 1; /* 已消耗第一个字节 */
    }

    while(i < src_len && j < dst_max - 4) {
        unsigned char c = (unsigned char)src[i];
        if(c < 0x80) {
            dst[j++] = src[i++];
        } else if(c >= 0x81) {
            if(i + 1 < src_len) {
                unsigned char c2 = (unsigned char)src[i + 1];
                if(c2 >= 0x40 && c2 != 0xFF) {
                    char tmp[4];
                    int n = gbk_char_to_utf8(c, c2, tmp);
                    for(int k = 0; k < n && j < dst_max - 4; k++) dst[j++] = tmp[k];
                    i += 2;
                } else {
                    dst[j++] = '?';
                    i++;
                }
            } else {
                /* 高字节在页末，留给下一页 */
                if(carry_out) *carry_out = c;
                i++;
                break;
            }
        } else {
            dst[j++] = '?';
            i++;
        }
    }
    dst[j] = '\0';
    return j;
}

/**
 * @brief 自动检测编码并转换（用于第一页，同时设置 g_file_enc）
 */
static int convert_first_page(const char *src, int src_len, char *dst, int dst_max)
{
    g_gbk_carry      = 0;
    g_utf8_carry_len = 0;

    /* 1. UTF-8 BOM */
    if (src_len >= 3 &&
        (unsigned char)src[0] == 0xEF &&
        (unsigned char)src[1] == 0xBB &&
        (unsigned char)src[2] == 0xBF) {
        g_file_enc = ENC_UTF8;
        int n = src_len - 3;
        if (n >= dst_max) n = dst_max - 1;
        memcpy(dst, src + 3, n);
        dst[n] = '\0';
        /* 检查末尾是否有不完整的UTF-8序列 */
        goto utf8_trim;
    }

    /* 2. 检测 UTF-8 without BOM */
    {
        int scan = (src_len < 512) ? src_len : 512;
        for (int k = 0; k < scan - 2; k++) {
            unsigned char b0 = (unsigned char)src[k];
            unsigned char b1 = (unsigned char)src[k + 1];
            if (b0 >= 0xE0 && b0 <= 0xEF && (b1 & 0xC0) == 0x80) {
                unsigned char b2 = (unsigned char)src[k + 2];
                if ((b2 & 0xC0) == 0x80) {
                    g_file_enc = ENC_UTF8;
                    int n = src_len;
                    if (n >= dst_max) n = dst_max - 1;
                    memcpy(dst, src, n);
                    dst[n] = '\0';
                    goto utf8_trim;
                }
            }
            if (b0 >= 0xC2 && b0 <= 0xDF && (b1 & 0xC0) == 0x80) {
                g_file_enc = ENC_UTF8;
                int n = src_len;
                if (n >= dst_max) n = dst_max - 1;
                memcpy(dst, src, n);
                dst[n] = '\0';
                goto utf8_trim;
            }
        }
    }

    /* 3. GBK */
    g_file_enc = ENC_GBK;
    return do_gbk_to_utf8(src, src_len, dst, dst_max, 0, &g_gbk_carry);

utf8_trim:
    /* 检查 dst 末尾是否有不完整的 UTF-8 多字节序列，截断并保存到 carry */
    {
        int len = (int)strlen(dst);
        /* 从末尾往前最多扫 4 个字节，找到序列头字节 */
        for (int i = len - 1; i >= 0 && i >= len - 4; i--) {
            unsigned char c = (unsigned char)dst[i];
            if (c >= 0xF0) {
                /* 4字节序列头，需要后面3个续字节 */
                if (len - 1 - i < 3) {
                    g_utf8_carry_len = len - i;
                    memcpy(g_utf8_carry, dst + i, g_utf8_carry_len);
                    dst[i] = '\0';
                }
                break;
            } else if (c >= 0xE0) {
                /* 3字节序列头，需要后面2个续字节 */
                if (len - 1 - i < 2) {
                    g_utf8_carry_len = len - i;
                    memcpy(g_utf8_carry, dst + i, g_utf8_carry_len);
                    dst[i] = '\0';
                }
                break;
            } else if (c >= 0xC0) {
                /* 2字节序列头，需要后面1个续字节 */
                if (len - 1 - i < 1) {
                    g_utf8_carry_len = 1;
                    g_utf8_carry[0] = c;
                    dst[i] = '\0';
                }
                break;
            } else if (c < 0x80) {
                /* ASCII字节，序列完整，不需要截断 */
                break;
            }
            /* c 是 0x80-0xBF（续字节），继续往前找序列头 */
        }
        return (int)strlen(dst);
    }
}

/**
 * @brief 根据已知编码转换（用于第2页及以后）
 */
static int convert_page(const char *src, int src_len, char *dst, int dst_max)
{
    if (g_file_enc == ENC_UTF8) {
        /* 先把上页遗留的不完整字节拼到本页开头 */
        static char tmp[UTF8_BUF_SIZE] __attribute__((section(".RAM_D2")));
        int carry = g_utf8_carry_len;
        int total = carry + src_len;
        if (total >= (int)sizeof(tmp)) total = (int)sizeof(tmp) - 1;

        if (carry > 0) {
            memcpy(tmp, g_utf8_carry, carry);
            memcpy(tmp + carry, src, total - carry);
            tmp[total] = '\0';
            g_utf8_carry_len = 0;
            src     = tmp;
            src_len = total;
        }

        int n = src_len;
        if (n >= dst_max) n = dst_max - 1;
        memcpy(dst, src, n);
        dst[n] = '\0';

        /* 检查末尾是否有不完整序列 */
        int len = n;
        for (int i = len - 1; i >= 0 && i >= len - 4; i--) {
            unsigned char c = (unsigned char)dst[i];
            if (c >= 0xF0) {
                if (len - 1 - i < 3) { g_utf8_carry_len = len - i; memcpy(g_utf8_carry, dst + i, g_utf8_carry_len); dst[i] = '\0'; }
                break;
            } else if (c >= 0xE0) {
                if (len - 1 - i < 2) { g_utf8_carry_len = len - i; memcpy(g_utf8_carry, dst + i, g_utf8_carry_len); dst[i] = '\0'; }
                break;
            } else if (c >= 0xC0) {
                if (len - 1 - i < 1) { g_utf8_carry_len = 1; g_utf8_carry[0] = c; dst[i] = '\0'; }
                break;
            } else if (c < 0x80) {
                break;
            }
        }
        return (int)strlen(dst);
    }
    /* GBK：传入上页遗留的高字节，输出本页遗留的高字节 */
    return do_gbk_to_utf8(src, src_len, dst, dst_max, g_gbk_carry, &g_gbk_carry);
}

/**
 * @brief 文件名GBK→UTF-8
 */
static int filename_to_utf8(const char *src, int src_len, char *dst, int dst_max)
{
    return do_gbk_to_utf8(src, src_len, dst, dst_max, 0, NULL);
}

/* ============================================================
 * 前向声明
 * ============================================================ */
static void show_file_list(void);
static void show_file_content(const char *filename);
static void return_btn_cb(lv_event_t *e);
static void file_item_cb(lv_event_t *e);

/* ============================================================
 * 工具栏
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

    lv_obj_t *btn = draw_return_icon(bar);
    lv_obj_align(btn, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, return_btn_cb, LV_EVENT_CLICKED, NULL);

    g_title_label = lv_label_create(bar);
    lv_label_set_text(g_title_label, "Text Reader");
    lv_obj_set_style_text_color(g_title_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(g_title_label, font_sdram_get(), 0);
    lv_obj_align(g_title_label, LV_ALIGN_CENTER, 0, 0);
}

/* ============================================================
 * 回调
 * ============================================================ */
static void return_btn_cb(lv_event_t *e)
{
    if (g_view == VIEW_FILE_CONTENT) {
        show_file_list();
    } else {
        lv_text_reader_destroy();
        extern void lv_player(void);
        lv_player();
    }
}

static void file_item_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < g_file_count) {
        show_file_content(g_filenames_raw[idx]);
    }
}

/* ============================================================
 * 文件列表
 * ============================================================ */
static void show_file_list(void)
{
    g_view = VIEW_FILE_LIST;
    lv_label_set_text(g_title_label, "Text Reader");
    lv_obj_clean(g_content_area);

    g_file_count = 0;
    DIR dir;
    FILINFO fno;
    if (f_opendir(&dir, SD_ROOT_PATH) == FR_OK) {
        while (g_file_count < MAX_FILES) {
            if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == 0) break;
            if (fno.fattrib & AM_DIR) continue;
            const char *ext = strrchr(fno.fname, '.');
            if (ext && strcasecmp(ext, ".txt") == 0) {
                strncpy(g_filenames_raw[g_file_count], fno.fname, MAX_FILENAME_LEN - 1);
                g_filenames_raw[g_file_count][MAX_FILENAME_LEN - 1] = '\0';
                filename_to_utf8(fno.fname, strlen(fno.fname),
                                 g_filenames[g_file_count], MAX_FILENAME_UTF8);
                g_file_count++;
            }
        }
        f_closedir(&dir);
    }


    if (g_file_count == 0) {
        lv_obj_t *hint = lv_label_create(g_content_area);
        lv_label_set_text(hint, "SD\xe5\x8d\xa1\xe6\xa0\xb9\xe7\x9b\xae\xe5\xbd\x95\xe6\xb2\xa1\xe6\x9c\x89 .txt \xe6\x96\x87\xe4\xbb\xb6");
        lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(hint, font_sdram_get(), 0);
        lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);
        return;
    }

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
        lv_obj_set_style_bg_color(item, lv_color_hex(0xe8f4fd), LV_STATE_PRESSED);

        lv_obj_t *icon = draw_file_icon(item);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *name_lbl = lv_label_create(item);
        lv_label_set_text(name_lbl, g_filenames[i]);
        lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name_lbl, lv_pct(70));
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(0x2c3e50), 0);
        lv_obj_set_style_text_font(name_lbl, font_sdram_get(), 0);
        lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 36, 0);

        lv_obj_t *arrow = lv_label_create(item);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(arrow, lv_color_hex(0xaaaaaa), 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, 0, 0);

        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(item, file_item_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

/* ============================================================
 * 文件内容
 * ============================================================ */
static void close_current_file(void)
{
    if (g_file_opened) {
        f_close(&g_current_file);
        g_file_opened = false;
    }
    g_gbk_carry      = 0;
    g_utf8_carry_len = 0;
    g_file_enc       = ENC_UNKNOWN;
}

static void scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    lv_coord_t bottom = lv_obj_get_scroll_bottom(cont);
    if (bottom < 200 && g_file_opened && g_current_pos < g_file_size) {
        /* 加载下一页，直接替换内容（不拼接，避免内存溢出） */
        UINT bytes_read = 0;
        memset(g_read_buf, 0, READ_BUF_SIZE);
        UINT to_read = (g_file_size - g_current_pos < PAGE_SIZE) ?
                       (UINT)(g_file_size - g_current_pos) : PAGE_SIZE;
        if (f_read(&g_current_file, g_read_buf, to_read, &bytes_read) == FR_OK && bytes_read > 0) {
            g_current_pos += bytes_read;
            g_read_buf[bytes_read] = '\0';
            convert_page(g_read_buf, (int)bytes_read, g_utf8_buf, UTF8_BUF_SIZE);
            if (g_text_label) {
                lv_label_set_text(g_text_label, g_utf8_buf);
                /* 滚动回顶部显示新内容 */
                lv_obj_scroll_to_y(cont, 0, LV_ANIM_OFF);
            }
        }
    }
}

static void show_file_content(const char *filename)
{
    g_view = VIEW_FILE_CONTENT;
    g_file_enc = ENC_UNKNOWN;

    close_current_file();

    /* 标题 */
    static char title_utf8[MAX_FILENAME_UTF8];
    filename_to_utf8(filename, strlen(filename), title_utf8, sizeof(title_utf8));
    lv_label_set_text(g_title_label, title_utf8);

    lv_obj_clean(g_content_area);

    /* 构造完整路径 */
    char path[MAX_FILENAME_LEN + 8];
    snprintf(path, sizeof(path), "%s%s", SD_ROOT_PATH, filename);
    strncpy(g_current_filename, filename, MAX_FILENAME_LEN - 1);

    /* 打开文件 */
    FRESULT res = f_open(&g_current_file, path, FA_READ);
    if (res != FR_OK) {
        lv_obj_t *err = lv_label_create(g_content_area);
        snprintf(g_utf8_buf, UTF8_BUF_SIZE, "Cannot open: %s (err=%d)", path, res);
        lv_label_set_text(err, g_utf8_buf);
        lv_obj_set_style_text_font(err, font_sdram_get(), 0);
        return;
    }

    g_file_opened = true;
    g_file_size   = f_size(&g_current_file);
    g_current_pos = 0;

    /* 滚动容器 */
    lv_obj_t *scroll_cont = lv_obj_create(g_content_area);
    lv_obj_set_size(scroll_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(scroll_cont, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(scroll_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scroll_cont, 0, 0);
    lv_obj_set_style_radius(scroll_cont, 0, 0);
    lv_obj_set_style_pad_all(scroll_cont, 12, 0);
    lv_obj_set_scroll_dir(scroll_cont, LV_DIR_VER);
    lv_obj_add_event_cb(scroll_cont, scroll_event_cb, LV_EVENT_SCROLL, NULL);

    /* 文本标签 */
    g_text_label = lv_label_create(scroll_cont);
    lv_label_set_long_mode(g_text_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_text_label, lv_pct(100));
    lv_obj_set_style_text_color(g_text_label, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_font(g_text_label, font_sdram_get(), 0);
    lv_obj_set_style_text_line_space(g_text_label, 4, 0);

    /* 读取第一页 */
    UINT bytes_read = 0;
    UINT to_read = (g_file_size < PAGE_SIZE) ? (UINT)g_file_size : PAGE_SIZE;
    res = f_read(&g_current_file, g_read_buf, to_read, &bytes_read);

    if (res == FR_OK && bytes_read > 0) {
        g_current_pos = bytes_read;
        g_read_buf[bytes_read] = '\0';

        int utf8_len = convert_first_page(g_read_buf, (int)bytes_read, g_utf8_buf, UTF8_BUF_SIZE);
        (void)utf8_len;

        lv_label_set_text(g_text_label, g_utf8_buf);
    } else {
        lv_label_set_text(g_text_label, "Read failed");
        close_current_file();
    }
}

/* ============================================================
 * 公开接口
 * ============================================================ */
lv_obj_t *lv_text_reader_create(lv_obj_t *parent_screen)
{
    if (g_screen != NULL) lv_text_reader_destroy();

    lv_coord_t sw = lv_disp_get_hor_res(NULL);
    lv_coord_t sh = lv_disp_get_ver_res(NULL);

    g_screen = lv_obj_create(NULL);
    lv_obj_set_size(g_screen, sw, sh);
    lv_obj_set_style_bg_color(g_screen, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(g_screen, 0, 0);
    lv_obj_set_style_border_width(g_screen, 0, 0);
    lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);

    create_toolbar(g_screen, sw);

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
    show_file_list();
    return g_screen;
}

int lv_text_reader_list_files(const char *dir_path)
{
    (void)dir_path;
    if (g_screen == NULL) return -1;
    show_file_list();
    return g_file_count;
}

int lv_text_reader_load_file(const char *file_path)
{
    if (g_screen == NULL) return -1;
    const char *name = strrchr(file_path, '/');
    name = name ? name + 1 : file_path;
    show_file_content(name);
    return 0;
}

void lv_text_reader_destroy(void)
{
    close_current_file();
    if (g_screen != NULL) {
        lv_obj_del(g_screen);
        g_screen       = NULL;
        g_title_label  = NULL;
        g_content_area = NULL;
        g_text_label   = NULL;
    }
    g_file_count = 0;
    g_view       = VIEW_FILE_LIST;
    g_file_enc   = ENC_UNKNOWN;
    g_gbk_carry  = 0;
    g_utf8_carry_len = 0;
    memset(g_filenames_raw, 0, sizeof(g_filenames_raw));
}

void lv_text_reader_clear(void)
{
    if (g_content_area != NULL) lv_obj_clean(g_content_area);
}
