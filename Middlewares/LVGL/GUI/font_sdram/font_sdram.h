/**
 * @file font_sdram.h
 * @brief 从SD卡加载字体bin文件，供LVGL使用
 *
 * 使用方法：
 *   1. 把 simhei_16.bin 复制到SD卡 /FONT/ 目录
 *   2. 在 lv_init() + lv_fs_fatfs_init() 之后调用 font_sdram_init()
 *   3. 用 font_sdram_get() 获取字体指针
 *
 * 加载流程：
 *   FatFs 顺序读取整个 bin → lv_mem_alloc 临时缓冲区（SDRAM堆）
 *   → 内存文件系统驱动 → lv_font_load 从内存解析（避免SD卡随机IO乱码）
 *   → 释放临时缓冲区（字体结构体已独立分配）
 */

#ifndef __FONT_SDRAM_H
#define __FONT_SDRAM_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/* SD卡上的字体文件路径 */
#define FONT_BIN_PATH           "S:/FONT/simhei_16.bin"

/**
 * @brief 初始化：从SD卡读取字体bin并解析，注册到LVGL
 * @retval true: 成功, false: 失败
 */
bool font_sdram_init(void);

/**
 * @brief 获取已加载的中文字体指针
 * @retval 字体指针，失败时返回 &lv_font_montserrat_16
 */
const lv_font_t *font_sdram_get(void);

/**
 * @brief 是否已成功加载
 */
bool font_sdram_is_loaded(void);

#endif /* __FONT_SDRAM_H */
