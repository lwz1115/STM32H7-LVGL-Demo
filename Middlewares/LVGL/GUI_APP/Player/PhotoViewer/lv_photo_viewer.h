/**
 * @file lv_photo_viewer.h
 * @brief 相册浏览器 - 从SD卡读取图片并显示
 *
 * 支持格式：BMP、JPG（LVGL内置解码器）
 * 图片路径：SD卡根目录下的 /PHOTO/ 文件夹
 * 操作方式：左右圆形箭头切换图片
 */

#ifndef LV_PHOTO_VIEWER_H
#define LV_PHOTO_VIEWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief 创建相册浏览器界面
 * @return 创建的屏幕对象
 */
lv_obj_t *lv_photo_viewer_create(void);

/**
 * @brief 销毁相册浏览器界面，释放资源
 */
void lv_photo_viewer_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_PHOTO_VIEWER_H */
