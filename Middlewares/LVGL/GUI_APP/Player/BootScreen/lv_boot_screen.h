/**
 * @file lv_boot_screen.h
 * @brief 系统启动画面
 *
 * 在字体加载期间显示进度，避免白屏等待
 */

#ifndef __LV_BOOT_SCREEN_H
#define __LV_BOOT_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief 创建并显示启动画面
 *        必须在 lv_init() + lv_port_disp_init() 之后调用
 *        调用后立即执行一次 lv_task_handler() 让画面渲染出来
 */
void lv_boot_screen_show(void);

/**
 * @brief 更新进度条和状态文字
 * @param progress  进度 0~100
 * @param msg       状态文字（ASCII，不超过32字符）
 */
void lv_boot_screen_update(uint8_t progress, const char *msg);

/**
 * @brief 销毁启动画面（切换到主界面前调用）
 */
void lv_boot_screen_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* __LV_BOOT_SCREEN_H */
