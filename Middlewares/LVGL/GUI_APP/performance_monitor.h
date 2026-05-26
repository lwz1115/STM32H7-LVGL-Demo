/**
 * @file performance_monitor.h
 * @brief 性能监控工具 - 监控FPS、CPU、内存使用
 */

#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

/**
 * @brief 性能统计数据结构
 */
typedef struct {
    uint32_t fps;              /* 当前FPS */
    uint32_t cpu_usage;        /* CPU使用率(%) */
    uint32_t mem_used;         /* 已使用内存(字节) */
    uint32_t mem_free;         /* 空闲内存(字节) */
    uint32_t mem_frag;         /* 内存碎片率(%) */
    uint32_t render_time_ms;   /* 渲染时间(ms) */
} perf_stats_t;

/**
 * @brief 初始化性能监控
 */
void perf_monitor_init(void);

/**
 * @brief 更新性能统计（每帧调用）
 */
void perf_monitor_update(void);

/**
 * @brief 获取性能统计数据
 * @param stats 输出统计数据
 */
void perf_monitor_get_stats(perf_stats_t * stats);

/**
 * @brief 打印性能统计到串口
 */
void perf_monitor_print(void);

/**
 * @brief 创建性能监控UI（屏幕上显示）
 * @param parent 父对象
 * @return 监控UI对象
 */
lv_obj_t * perf_monitor_create_ui(lv_obj_t * parent);

/**
 * @brief 销毁性能监控UI
 */
void perf_monitor_destroy_ui(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*PERFORMANCE_MONITOR_H*/
