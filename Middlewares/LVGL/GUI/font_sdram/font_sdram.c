/**
 * @file font_sdram.c
 * @brief 从SD卡加载字体bin文件，通过LVGL文件系统接口解析，堆放在SDRAM
 *
 * 工作流程：
 *   1. lv_conf.h 中 LV_MEM_ADR = 0xC0600000，LV_MEM_SIZE = 26MB
 *      → LVGL 的 lv_mem_alloc() 直接从 SDRAM 分配，有足够空间放字体
 *   2. font_sdram_init() 调用 lv_font_load("S:/simhei_16.bin")
 *      → LVGL 通过 FatFs 读取 bin 文件，解析后把字模数据分配在 SDRAM 堆里
 *   3. font_sdram_get() 返回字体指针，供 lv_obj_set_style_text_font() 使用
 *
 * 前提：SD卡根目录必须有 simhei_16.bin（约2.6MB）
 */

#include "font_sdram.h"
#include "font/lv_font_loader.h"
#include <stdio.h>

/* ============================================================
 * 内部状态
 * ============================================================ */
static lv_font_t *s_font   = NULL;
static bool       s_loaded = false;

/* ============================================================
 * 公开接口
 * ============================================================ */

bool font_sdram_init(void)
{
    if (s_loaded) return true;

    printf("[font] 加载字体: %s\r\n", FONT_BIN_PATH);

    /* lv_font_load 通过 LVGL 文件系统读取 bin，
     * 字模数据用 lv_mem_alloc 分配（已指向 SDRAM，26MB 足够） */
    s_font = lv_font_load(FONT_BIN_PATH);

    if (s_font == NULL) {
        printf("[font] 加载失败！请确认 SD 卡根目录有 simhei_16.bin\r\n");
        return false;
    }

    s_loaded = true;
    printf("[font] 加载成功\r\n");
    return true;
}

const lv_font_t *font_sdram_get(void)
{
    if (s_loaded && s_font != NULL) {
        return s_font;
    }
    return &lv_font_montserrat_16;  /* 回退字体 */
}

bool font_sdram_is_loaded(void)
{
    return s_loaded;
}
