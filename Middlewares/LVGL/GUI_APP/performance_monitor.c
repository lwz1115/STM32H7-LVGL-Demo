/**
 * @file performance_monitor.c
 * @brief 性能监控工具实现
 */

#include "performance_monitor.h"
#include "stm32h7xx_hal.h"
#include <stdio.h>

/* 静态变量 */
static uint32_t frame_count = 0;
static uint32_t last_time_ms = 0;
static perf_stats_t current_stats = {0};
static lv_obj_t * monitor_label = NULL;

/**
 * @brief 初始化性能监控
 */
void perf_monitor_init(void)
{
    frame_count = 0;
    last_time_ms = HAL_GetTick();
}

/**
 * @brief 更新性能统计
 */
void perf_monitor_update(void)
{
    frame_count++;
    uint32_t current_time_ms = HAL_GetTick();
    uint32_t elapsed_ms = current_time_ms - last_time_ms;
    
    /* 每秒更新一次统计 */
    if(elapsed_ms >= 1000) {
        /* 计算FPS */
        current_stats.fps = (frame_count * 1000) / elapsed_ms;
        
        /* 获取LVGL内存使用情况 */
        lv_mem_monitor_t mem_mon;
        lv_mem_monitor(&mem_mon);
        current_stats.mem_used = mem_mon.total_size - mem_mon.free_size;
        current_stats.mem_free = mem_mon.free_size;
        current_stats.mem_frag = mem_mon.frag_pct;
        
        /* 估算CPU使用率（基于LVGL任务时间） */
        current_stats.cpu_usage = (current_stats.render_time_ms * 100) / elapsed_ms;
        if(current_stats.cpu_usage > 100) current_stats.cpu_usage = 100;
        
        /* 重置计数器 */
        frame_count = 0;
        last_time_ms = current_time_ms;
        current_stats.render_time_ms = 0;
    }
}

/**
 * @brief 获取性能统计数据
 */
void perf_monitor_get_stats(perf_stats_t * stats)
{
    if(stats != NULL) {
        *stats = current_stats;
    }
}

/**
 * @brief 打印性能统计到串口
 */
void perf_monitor_print(void)
{
    printf("=== Performance Stats ===\n");
    printf("FPS: %lu\n", current_stats.fps);
    printf("CPU: %lu%%\n", current_stats.cpu_usage);
    printf("Memory Used: %lu bytes\n", current_stats.mem_used);
    printf("Memory Free: %lu bytes\n", current_stats.mem_free);
    printf("Memory Frag: %lu%%\n", current_stats.mem_frag);
    printf("========================\n");
}

/**
 * @brief 性能监控UI更新定时器回调
 */
static void perf_monitor_update_cb(lv_timer_t * timer)
{
    if(monitor_label != NULL) {
        static char buf[64];
        snprintf(buf, sizeof(buf), 
                "FPS: %lu\nCPU: %lu%%\nMEM: %lu KB",
                current_stats.fps,
                current_stats.cpu_usage,
                current_stats.mem_used / 1024);
        lv_label_set_text(monitor_label, buf);
    }
}

/**
 * @brief 创建性能监控UI
 */
lv_obj_t * perf_monitor_create_ui(lv_obj_t * parent)
{
    if(monitor_label != NULL) {
        return monitor_label;
    }
    
    /* 创建半透明背景 */
    lv_obj_t * bg = lv_obj_create(parent);
    lv_obj_set_size(bg, 150, 100);
    lv_obj_align(bg, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_opa(bg, LV_OPA_70, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(bg, 1, 0);
    lv_obj_set_style_border_color(bg, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(bg, 5, 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 创建文本标签 */
    monitor_label = lv_label_create(bg);
    lv_label_set_text(monitor_label, "FPS: --\nCPU: --%\nMEM: -- KB");
    lv_obj_set_style_text_color(monitor_label, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(monitor_label, &lv_font_montserrat_12, 0);
    lv_obj_center(monitor_label);
    
    /* 创建定时器定期更新显示 */
    lv_timer_create(perf_monitor_update_cb, 500, NULL);  /* 每500ms更新一次 */
    
    return bg;
}

/**
 * @brief 销毁性能监控UI
 */
void perf_monitor_destroy_ui(void)
{
    if(monitor_label != NULL) {
        lv_obj_t * parent = lv_obj_get_parent(monitor_label);
        if(parent != NULL) {
            lv_obj_del(parent);
        }
        monitor_label = NULL;
    }
}
