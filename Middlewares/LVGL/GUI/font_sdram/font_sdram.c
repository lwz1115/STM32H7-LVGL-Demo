/**
 * @file font_sdram.c
 * @brief 字体加载优化版本
 *
 * 优化原理：
 *   lv_font_load() 内部对每个字符做 seek+read，20902个汉字 = 4万次SD卡随机IO，极慢。
 *
 *   优化方案：
 *   1. 用 FatFs 把整个 bin 文件一次性读入 SDRAM（顺序读，速度快）
 *   2. 注册一个"内存文件系统"驱动到 LVGL，驱动字母用 'M'
 *   3. 调用 lv_font_load("M:/font") 让 LVGL 从内存读取
 *      → 所有 seek/read 变成指针运算，速度提升 100x+
 *
 * SDRAM 布局：
 *   0xC0200000 ~ 0xC05FFFFF  字体 bin 数据（最大4MB）
 *   0xC0600000 ~             LVGL 堆
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

/* 内存文件句柄 */
typedef struct {
    const uint8_t *data;    /* 数据指针 */
    uint32_t       size;    /* 总大小 */
    uint32_t       pos;     /* 当前读取位置 */
} mem_file_t;

/* 只有一个字体文件，静态分配句柄 */
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

static void mem_fs_register(void)
{
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);
    drv.letter   = MEM_FS_LETTER;
    drv.open_cb  = mem_fs_open;
    drv.close_cb = mem_fs_close;
    drv.read_cb  = mem_fs_read;
    drv.seek_cb  = mem_fs_seek;
    drv.tell_cb  = mem_fs_tell;
    lv_fs_drv_register(&drv);
}

/* ============================================================
 * 内部状态
 * ============================================================ */
static lv_font_t *s_font   = NULL;
static bool       s_loaded = false;
static font_progress_cb_t s_progress_cb = NULL;

/* SDRAM 字体数据区 */
static uint8_t * const s_sdram_buf = (uint8_t *)FONT_SDRAM_BASE_ADDR;

/* ============================================================
 * 公开接口
 * ============================================================ */

void font_sdram_set_progress_cb(font_progress_cb_t cb)
{
    s_progress_cb = cb;
}

bool font_sdram_init(void)
{
    if (s_loaded) return true;

    printf("[font] 开始加载: %s\r\n", FONT_BIN_PATH);
    if (s_progress_cb) s_progress_cb(40, "Reading font...");

    /* ---- Step 1: FatFs 顺序读取整个 bin 到 SDRAM ---- */
    FIL fil;
    FRESULT res = f_open(&fil, FONT_BIN_PATH, FA_READ);
    if (res != FR_OK) {
        printf("[font] 打开失败 err=%d，请把 simhei_16.bin 放到SD卡根目录\r\n", res);
        if (s_progress_cb) s_progress_cb(40, "Font file not found!");
        return false;
    }

    FSIZE_t file_size = f_size(&fil);
    printf("[font] 文件大小: %lu 字节 (%.2f MB)\r\n",
           (uint32_t)file_size, (float)file_size / 1024.0f / 1024.0f);

    if (file_size > FONT_SDRAM_MAX_SIZE) {
        printf("[font] 文件超过 %d MB 限制\r\n", FONT_SDRAM_MAX_SIZE / 1024 / 1024);
        f_close(&fil);
        return false;
    }

    /* 大块读取，每次 32KB，最大化 SDMMC 吞吐 */
    #define READ_CHUNK  (32 * 1024)
    UINT  bytes_read = 0;
    UINT  total_read = 0;
    uint8_t *dst = s_sdram_buf;
    UINT  file_size_u = (UINT)file_size;

    while (total_read < file_size_u) {
        UINT chunk = (file_size_u - total_read > READ_CHUNK) ?
                     READ_CHUNK : file_size_u - total_read;
        res = f_read(&fil, dst, chunk, &bytes_read);
        if (res != FR_OK || bytes_read == 0) break;
        dst        += bytes_read;
        total_read += bytes_read;

        /* 更新进度：40%~75% 对应读取阶段 */
        if (s_progress_cb) {
            uint8_t pct = 40 + (uint8_t)((uint32_t)(total_read) * 35 / file_size_u);
            s_progress_cb(pct, "Reading font...");
        }
    }
    f_close(&fil);

    if (total_read != file_size_u) {
        printf("[font] 读取不完整: %u / %u\r\n", total_read, file_size_u);
        return false;
    }
    printf("[font] SD卡读取完成: %u 字节\r\n", total_read);

    /* ---- Step 2: 注册内存文件系统驱动 ---- */
    mem_fs_register();

    /* 设置内存文件句柄指向 SDRAM 数据 */
    s_mem_file.data = s_sdram_buf;
    s_mem_file.size = total_read;
    s_mem_file.pos  = 0;

    /* ---- Step 3: 从内存解析字体结构（极快，全内存操作） ---- */
    if (s_progress_cb) s_progress_cb(78, "Parsing font...");
    printf("[font] 从内存解析字体结构...\r\n");
    s_font = lv_font_load("M:/font");

    if (s_font == NULL) {
        printf("[font] 解析失败！bin 文件可能损坏\r\n");
        if (s_progress_cb) s_progress_cb(78, "Font parse failed!");
        return false;
    }

    s_loaded = true;
    if (s_progress_cb) s_progress_cb(90, "Font ready");
    printf("[font] 加载成功\r\n");
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
