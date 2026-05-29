/**
 * @file font_sdram.c
 * @brief 字体加载：先把bin文件读入LVGL堆，再从内存解析
 *
 * 为什么不直接用 lv_font_load(SD卡路径)：
 *   lv_font_load 内部逐字符 seek+read，3万字符 = 6万次SD卡随机IO
 *   FatFs 512字节扇区缓冲频繁切换，导致读取数据错位，字符映射乱码
 *
 * 正确做法：
 *   1. 用 FatFs 顺序读取整个 bin 到 lv_mem_alloc 分配的缓冲区（SDRAM堆）
 *   2. 注册内存文件系统驱动（驱动字母 'M'）
 *   3. lv_font_load("M:/font") 从内存读取，所有 seek/read 变成指针运算
 *   4. 解析完成后释放临时缓冲区（字体结构体已分配在堆里，不依赖原始bin数据）
 *
 * 注意：步骤4中，glyph_bitmap 数据是从 bin 文件里拷贝出来的（load_glyph 里
 *       lv_mem_alloc 了新的 glyph_bitmap），所以释放临时缓冲区是安全的。
 */

#include "font_sdram.h"
#include "font/lv_font_loader.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * 内存文件系统驱动（驱动字母 'M'）
 * ============================================================ */
#define MEM_FS_LETTER   'M'

typedef struct {
    const uint8_t *data;
    uint32_t       size;
    uint32_t       pos;
} mem_file_t;

static mem_file_t s_mem_file;

static void *mem_fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    (void)drv; (void)path; (void)mode;
    s_mem_file.pos = 0;
    return &s_mem_file;
}

static lv_fs_res_t mem_fs_close(lv_fs_drv_t *drv, void *file_p)
{
    (void)drv; (void)file_p;
    return LV_FS_RES_OK;
}

static lv_fs_res_t mem_fs_read(lv_fs_drv_t *drv, void *file_p,
                                void *buf, uint32_t btr, uint32_t *br)
{
    (void)drv;
    mem_file_t *f = (mem_file_t *)file_p;
    uint32_t available = f->size - f->pos;
    uint32_t to_read   = (btr < available) ? btr : available;
    memcpy(buf, f->data + f->pos, to_read);
    f->pos += to_read;
    if (br) *br = to_read;
    return LV_FS_RES_OK;
}

static lv_fs_res_t mem_fs_seek(lv_fs_drv_t *drv, void *file_p,
                                uint32_t pos, lv_fs_whence_t whence)
{
    (void)drv;
    mem_file_t *f = (mem_file_t *)file_p;
    uint32_t new_pos;
    switch (whence) {
        case LV_FS_SEEK_SET: new_pos = pos;              break;
        case LV_FS_SEEK_CUR: new_pos = f->pos + pos;    break;
        case LV_FS_SEEK_END: new_pos = f->size + pos;   break;
        default:             return LV_FS_RES_INV_PARAM;
    }
    if (new_pos > f->size) new_pos = f->size;
    f->pos = new_pos;
    return LV_FS_RES_OK;
}

static lv_fs_res_t mem_fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    (void)drv;
    mem_file_t *f = (mem_file_t *)file_p;
    if (pos_p) *pos_p = f->pos;
    return LV_FS_RES_OK;
}

static bool s_mem_fs_registered = false;

static void mem_fs_register(void)
{
    if (s_mem_fs_registered) return;
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);
    drv.letter   = MEM_FS_LETTER;
    drv.open_cb  = mem_fs_open;
    drv.close_cb = mem_fs_close;
    drv.read_cb  = mem_fs_read;
    drv.seek_cb  = mem_fs_seek;
    drv.tell_cb  = mem_fs_tell;
    lv_fs_drv_register(&drv);
    s_mem_fs_registered = true;
}

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

    /* ---- Step 1: 用 FatFs 顺序读取整个 bin 到 LVGL 堆 ---- */
    FIL fil;
    FRESULT res = f_open(&fil, FONT_BIN_PATH, FA_READ);
    if (res != FR_OK) {
        return false;
    }

    FSIZE_t file_size = f_size(&fil);
    if (file_size == 0 || file_size > 16 * 1024 * 1024) {
        f_close(&fil);
        return false;
    }

    /* 从 LVGL 堆分配临时缓冲区（SDRAM，足够大） */
    uint8_t *bin_buf = (uint8_t *)lv_mem_alloc((uint32_t)file_size);
    if (bin_buf == NULL) {
        f_close(&fil);
        return false;
    }

    /* 顺序读取，每次 32KB */
    UINT total_read = 0;
    UINT bytes_read = 0;
    uint8_t *dst = bin_buf;
    while (total_read < (UINT)file_size) {
        UINT chunk = ((UINT)file_size - total_read > 32768) ? 32768 : (UINT)file_size - total_read;
        res = f_read(&fil, dst, chunk, &bytes_read);
        if (res != FR_OK || bytes_read == 0) break;
        dst        += bytes_read;
        total_read += bytes_read;
    }
    f_close(&fil);

    if (total_read != (UINT)file_size) {
        lv_mem_free(bin_buf);
        return false;
    }

    /* ---- Step 2: 注册内存文件系统，指向刚读入的数据 ---- */
    mem_fs_register();
    s_mem_file.data = bin_buf;
    s_mem_file.size = total_read;
    s_mem_file.pos  = 0;

    /* ---- Step 3: 从内存解析字体结构（所有 seek/read 变成指针运算） ---- */
    s_font = lv_font_load("M:/font");

    /* ---- Step 4: 释放临时缓冲区（glyph_bitmap 已被 lv_font_load 拷贝到新分配的内存） ---- */
    lv_mem_free(bin_buf);
    s_mem_file.data = NULL;
    s_mem_file.size = 0;

    if (s_font == NULL) {
        return false;
    }

    s_loaded = true;
    return true;
}

const lv_font_t *font_sdram_get(void)
{
    if (s_loaded && s_font != NULL) {
        return s_font;
    }
    return &lv_font_montserrat_16;
}

bool font_sdram_is_loaded(void)
{
    return s_loaded;
}
