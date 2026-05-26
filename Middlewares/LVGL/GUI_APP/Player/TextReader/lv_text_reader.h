/**
 * @file lv_text_reader.h
 * @brief 文本阅读器界面头文件 - 从SD卡读取文本并显示
 */

#ifndef LV_TEXT_READER_H
#define LV_TEXT_READER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief 创建文本阅读器界面
 * @param parent_screen 父屏幕对象（如果为NULL则创建新屏幕）
 * @return 返回创建的文本阅读器屏幕对象
 */
lv_obj_t * lv_text_reader_create(lv_obj_t * parent_screen);

/**
 * @brief 从SD卡加载文本文件
 * @param file_path 文件路径（例如："0:/test.txt"）
 * @return 0:成功, -1:失败
 */
int lv_text_reader_load_file(const char * file_path);

/**
 * @brief 列出SD卡指定目录中的文本文件
 * @param dir_path 目录路径（例如："0:/"）
 * @return 找到的文件数量，-1表示失败
 */
int lv_text_reader_list_files(const char * dir_path);

/**
 * @brief 清空文本内容
 */
void lv_text_reader_clear(void);

/**
 * @brief 销毁文本阅读器界面
 */
void lv_text_reader_destroy(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_TEXT_READER_H */
