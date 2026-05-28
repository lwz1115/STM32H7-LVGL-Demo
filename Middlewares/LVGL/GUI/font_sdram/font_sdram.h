/**
 * @file font_sdram.h
 * @brief 从SD卡加载字体bin文件到SDRAM，供LVGL使用
 *
 * 使用方法：
 *   1. 把 simhei_16.bin 复制到SD卡根目录
 *   2. 在 lv_init() 之后调用 font_sdram_init()
 *   3. 用 font_sdram_get() 获取字体指针，传给 lv_obj_set_style_text_font()
 *
 * SDRAM布局（0xC0000000 起，32MB）：
 *   0xC0000000 ~ 0xC0FFFFFF  LVGL显示缓冲区（由lv_port_disp使用，约2MB）
 *   0xC0200000 ~ 0xC0600000  字体数据区（最大4MB，实际约2.6MB）
 */

#ifndef __FONT_SDRAM_H
#define __FONT_SDRAM_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/* SDRAM中字体数据的存放起始地址（避开LVGL显示缓冲区）
 * LVGL双缓冲：800*480*2字节*2 = 1,536,000 字节 ≈ 1.5MB
 * 从2MB偏移处开始存字体，留足余量 */
#define FONT_SDRAM_BASE_ADDR    (0xC0000000UL + 2 * 1024 * 1024)   /* 0xC0200000 */
#define FONT_SDRAM_MAX_SIZE     (4 * 1024 * 1024)                   /* 最大4MB */

/* SD卡上的字体文件路径 */
#define FONT_BIN_PATH           "S:/FONT/simhei_16.bin"

/**
 * @brief 初始化：从SD卡读取字体bin到SDRAM，并注册到LVGL
 * @retval true: 成功, false: 失败（SD卡无文件或SDRAM不足）
 */
bool font_sdram_init(void);

/**
 * @brief 设置进度回调（可选），在加载过程中更新进度条
 * @param cb  回调函数指针，参数为进度0~100和状态字符串
 *            传 NULL 取消回调
 */
typedef void (*font_progress_cb_t)(uint8_t progress, const char *msg);
void font_sdram_set_progress_cb(font_progress_cb_t cb);

/**
 * @brief 获取已加载的中文字体指针
 * @retval 字体指针，失败时返回 &lv_font_montserrat_16（回退字体）
 */
const lv_font_t *font_sdram_get(void);

/**
 * @brief 是否已成功加载
 */
bool font_sdram_is_loaded(void);

#endif /* __FONT_SDRAM_H */
