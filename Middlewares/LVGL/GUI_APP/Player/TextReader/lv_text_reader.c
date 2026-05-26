/**
 * @file lv_text_reader.c
 * @brief 文本阅读器界面实现 - 从SD卡读取文本并显示
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_text_reader.h"
#include "lvgl.h"
#include "../MainPage/assets/icon_draw.h"  /* 使用返回图标 */
#include "ff.h"  /* FatFS文件系统 */
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define TEXT_BUFFER_SIZE    (4096)  /* 文本缓冲区大小 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void return_btn_event_cb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t *text_reader_screen = NULL;  /* 文本阅读器屏幕对象 */
static lv_obj_t *text_area = NULL;           /* 文本显示区域 */
static lv_obj_t *status_label = NULL;        /* 状态标签 */
static char text_buffer[TEXT_BUFFER_SIZE];   /* 文本缓冲区 */

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 返回按钮点击事件回调函数
 */
static void return_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        /* 销毁文本阅读器界面 */
        lv_text_reader_destroy();
        
        /* 重新加载主界面 */
        extern void lv_player(void);
        lv_player();
    }
}

/**
 * @brief 创建文本阅读器界面
 */
lv_obj_t * lv_text_reader_create(lv_obj_t * parent_screen)
{
    /* 如果文本阅读器已存在，先销毁 */
    if(text_reader_screen != NULL) {
        lv_text_reader_destroy();
    }
    
    /* 获取屏幕尺寸 */
    lv_coord_t screen_w = lv_disp_get_hor_res(NULL);
    lv_coord_t screen_h = lv_disp_get_ver_res(NULL);
    
    /* 创建文本阅读器屏幕对象 */
    text_reader_screen = lv_obj_create(NULL);
    lv_obj_set_size(text_reader_screen, screen_w, screen_h);
    lv_obj_set_style_pad_all(text_reader_screen, 0, 0);
    lv_obj_set_style_border_width(text_reader_screen, 0, 0);
    lv_obj_set_style_outline_width(text_reader_screen, 0, 0);
    lv_obj_set_style_radius(text_reader_screen, 0, 0);
    lv_obj_clear_flag(text_reader_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 设置背景颜色 - 浅灰色适合阅读 */
    lv_obj_set_style_bg_color(text_reader_screen, lv_color_hex(0xf5f5f5), 0);
    lv_obj_set_style_bg_opa(text_reader_screen, LV_OPA_COVER, 0);
    
    /* 创建顶部工具栏 */
    lv_obj_t *toolbar = lv_obj_create(text_reader_screen);
    lv_obj_set_size(toolbar, screen_w, 55);  /* 高度降低一半 */
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_pad_all(toolbar, 3, 0);  /* 内边距也相应减小 */
    lv_obj_set_style_border_width(toolbar, 0, 0);
    lv_obj_set_style_radius(toolbar, 0, 0);
    lv_obj_set_style_bg_color(toolbar, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_bg_opa(toolbar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(toolbar, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 创建返回图标按钮 - 左上角 */
    lv_obj_t *return_btn = draw_return_icon(toolbar);
    lv_obj_align(return_btn, LV_ALIGN_LEFT_MID, 3, 0);
    lv_obj_add_flag(return_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(return_btn, return_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    /* 创建标题标签 */
    lv_obj_t *title_label = lv_label_create(toolbar);
    lv_label_set_text(title_label, "Text Reader");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);  /* 字体稍小 */
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);
    
    /* 创建状态标签 - 显示文件名或状态信息 */
    status_label = lv_label_create(text_reader_screen);
    lv_label_set_text(status_label, "No file loaded");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_12, 0);  /* 字体稍小 */
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 10, 60);  /* 位置调整 */
    
    /* 创建文本显示区域 */
    text_area = lv_textarea_create(text_reader_screen);
    lv_obj_set_size(text_area, screen_w - 20, screen_h - 95);  /* 高度调整 */
    lv_obj_align(text_area, LV_ALIGN_TOP_LEFT, 10, 80);  /* 位置调整 */
    
    /* 设置文本区域样式 */
    lv_obj_set_style_bg_color(text_area, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_color(text_area, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_border_width(text_area, 1, 0);
    lv_obj_set_style_radius(text_area, 5, 0);
    lv_obj_set_style_pad_all(text_area, 10, 0);
    lv_obj_set_style_text_color(text_area, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_font(text_area, &lv_font_montserrat_16, 0);
    
    /* 设置为只读模式 */
    lv_textarea_set_text(text_area, "");
    lv_obj_clear_flag(text_area, LV_OBJ_FLAG_CLICKABLE);
    
    /* 加载屏幕 */
    lv_scr_load(text_reader_screen);
    
    return text_reader_screen;
}

/**
 * @brief 从SD卡加载文本文件
 * @param file_path 文件路径，格式如 "0:/test.txt"
 * @return 0:成功, -1:失败
 */
int lv_text_reader_load_file(const char * file_path)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    
    /* 检查文本区域是否已创建 */
    if (text_area == NULL) {
        return -1;
    }
    
    /* 检查文件路径是否有效 */
    if (file_path == NULL || file_path[0] == '\0') {
        lv_label_set_text(status_label, "Error: Invalid file path");
        lv_textarea_set_text(text_area, "Invalid file path provided.");
        return -1;
    }
    
    /* 打开文件 */
    res = f_open(&file, file_path, FA_READ);
    if (res != FR_OK) {
        /* 详细的错误信息 */
        static char error_msg[256];
        const char *error_desc;
        
        switch(res) {
            case FR_NO_FILE:
                error_desc = "File not found";
                break;
            case FR_NO_PATH:
                error_desc = "Path not found";
                break;
            case FR_INVALID_NAME:
                error_desc = "Invalid file name";
                break;
            case FR_DENIED:
                error_desc = "Access denied";
                break;
            case FR_NOT_READY:
                error_desc = "SD card not ready";
                break;
            case FR_DISK_ERR:
                error_desc = "Disk error";
                break;
            default:
                error_desc = "Unknown error";
                break;
        }
        
        snprintf(error_msg, sizeof(error_msg), 
                "Error: Cannot open file\n\n"
                "File: %s\n"
                "Error code: %d (%s)\n\n"
                "Please check:\n"
                "- SD card is inserted\n"
                "- File exists in SD card\n"
                "- File path is correct\n"
                "- SD card is formatted (FAT32)",
                file_path, res, error_desc);
        
        lv_label_set_text(status_label, "Error: Cannot open file");
        lv_textarea_set_text(text_area, error_msg);
        return -1;
    }
    
    /* 获取文件大小 */
    FSIZE_t file_size = f_size(&file);
    
    /* 读取文件内容 */
    memset(text_buffer, 0, TEXT_BUFFER_SIZE);
    
    /* 如果文件大于缓冲区，只读取缓冲区大小 */
    UINT to_read = (file_size < TEXT_BUFFER_SIZE - 1) ? file_size : (TEXT_BUFFER_SIZE - 1);
    
    res = f_read(&file, text_buffer, to_read, &bytes_read);
    
    /* 关闭文件 */
    f_close(&file);
    
    if (res != FR_OK) {
        static char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), 
                "Failed to read file.\n"
                "Error code: %d\n"
                "Bytes read: %u", res, bytes_read);
        lv_label_set_text(status_label, "Error: Cannot read file");
        lv_textarea_set_text(text_area, error_msg);
        return -1;
    }
    
    /* 显示文件内容 */
    text_buffer[bytes_read] = '\0';  /* 确保字符串结束 */
    lv_textarea_set_text(text_area, text_buffer);
    
    /* 更新状态标签 */
    static char status_text[128];
    if (file_size > TEXT_BUFFER_SIZE - 1) {
        snprintf(status_text, sizeof(status_text), 
                "File: %s (%u/%u bytes shown)", 
                file_path, bytes_read, (unsigned int)file_size);
    } else {
        snprintf(status_text, sizeof(status_text), 
                "File: %s (%u bytes)", 
                file_path, bytes_read);
    }
    lv_label_set_text(status_label, status_text);
    
    return 0;
}

/**
 * @brief 列出SD卡指定目录中的文本文件
 * @param dir_path 目录路径，格式如 "0:/"
 * @return 找到的文件数量，-1表示失败
 */
int lv_text_reader_list_files(const char * dir_path)
{
    DIR dir;
    FILINFO fno;
    FRESULT res;
    int file_count = 0;
    static char file_list[TEXT_BUFFER_SIZE];
    
    /* 检查文本区域是否已创建 */
    if (text_area == NULL) {
        return -1;
    }
    
    /* 检查目录路径是否有效 */
    if (dir_path == NULL || dir_path[0] == '\0') {
        lv_label_set_text(status_label, "Error: Invalid directory path");
        lv_textarea_set_text(text_area, "Invalid directory path provided.");
        return -1;
    }
    
    /* 打开目录 */
    res = f_opendir(&dir, dir_path);
    if (res != FR_OK) {
        static char error_msg[256];
        const char *error_desc;
        
        switch(res) {
            case FR_NO_PATH:
                error_desc = "Path not found";
                break;
            case FR_INVALID_NAME:
                error_desc = "Invalid path name";
                break;
            case FR_NOT_READY:
                error_desc = "SD card not ready";
                break;
            case FR_DISK_ERR:
                error_desc = "Disk error";
                break;
            case FR_NOT_ENABLED:
                error_desc = "Volume not mounted (FR_NOT_ENABLED)";
                break;
            case FR_NO_FILESYSTEM:
                error_desc = "No valid FAT volume found";
                break;
            default:
                error_desc = "Unknown error";
                break;
        }
        
        snprintf(error_msg, sizeof(error_msg), 
                "Error: Cannot open directory\n\n"
                "Path: %s\n"
                "Error code: %d (%s)\n\n"
                "Please check:\n"
                "- SD card is inserted\n"
                "- SD card is formatted (FAT32)\n"
                "- SD card is mounted (f_mount)\n"
                "- Path format is correct",
                dir_path, res, error_desc);
        
        lv_label_set_text(status_label, "Error: Cannot open directory");
        lv_textarea_set_text(text_area, error_msg);
        return -1;
    }
    
    /* 初始化文件列表 */
    snprintf(file_list, sizeof(file_list), "Text files in %s:\n\n", dir_path);
    int list_len = strlen(file_list);
    
    /* 读取目录内容 */
    for (;;) {
        res = f_readdir(&dir, &fno);
        
        /* 读取错误或到达目录末尾 */
        if (res != FR_OK || fno.fname[0] == 0) {
            break;
        }
        
        /* 跳过目录 */
        if (fno.fattrib & AM_DIR) {
            continue;
        }
        
        /* 检查是否是文本文件（.txt扩展名） */
        const char *ext = strrchr(fno.fname, '.');
        if (ext != NULL && (strcmp(ext, ".txt") == 0 || strcmp(ext, ".TXT") == 0)) {
            /* 添加到文件列表 */
            int remaining = TEXT_BUFFER_SIZE - list_len - 1;
            int written = snprintf(file_list + list_len, remaining, 
                                  "%d. %s (%lu bytes)\n", 
                                  file_count + 1, fno.fname, (unsigned long)fno.fsize);
            
            if (written > 0 && written < remaining) {
                list_len += written;
                file_count++;
            } else {
                /* 缓冲区已满 */
                snprintf(file_list + list_len, remaining, "\n... (list truncated)");
                break;
            }
        }
    }
    
    /* 关闭目录 */
    f_closedir(&dir);
    
    /* 显示结果 */
    if (file_count == 0) {
        snprintf(file_list + list_len, TEXT_BUFFER_SIZE - list_len, 
                "\nNo .txt files found.\n\n"
                "Please add text files to the SD card.");
    }
    
    lv_textarea_set_text(text_area, file_list);
    
    /* 更新状态标签 */
    static char status_text[128];
    snprintf(status_text, sizeof(status_text), 
            "Found %d text file(s) in %s", file_count, dir_path);
    lv_label_set_text(status_label, status_text);
    
    return file_count;
}

/**
 * @brief 清空文本内容
 */
void lv_text_reader_clear(void)
{
    if (text_area != NULL) {
        lv_textarea_set_text(text_area, "");
    }
    
    if (status_label != NULL) {
        lv_label_set_text(status_label, "No file loaded");
    }
}

/**
 * @brief 销毁文本阅读器界面
 */
void lv_text_reader_destroy(void)
{
    if (text_reader_screen != NULL) {
        lv_obj_del(text_reader_screen);
        text_reader_screen = NULL;
        text_area = NULL;
        status_label = NULL;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
