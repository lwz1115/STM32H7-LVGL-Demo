/**
 * @file icon_draw.c
 * @brief 使用LVGL绘图API绘制电池和返回图标
 */

#include "icon_draw.h"
#include "lvgl.h"

/**
 * @brief 绘制电池图标（黑色，100%电量）
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_battery_icon(lv_obj_t * parent)
{
    /* 创建画布容器 */
    lv_obj_t * canvas_obj = lv_obj_create(parent);
    lv_obj_set_size(canvas_obj, 30, 16);  /* 电池图标大小 30x16 */
    lv_obj_set_style_bg_opa(canvas_obj, LV_OPA_TRANSP, 0);  /* 透明背景 */
    lv_obj_set_style_border_width(canvas_obj, 0, 0);
    lv_obj_set_style_pad_all(canvas_obj, 0, 0);
    lv_obj_clear_flag(canvas_obj, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 绘制电池外壳 - 主体矩形 */
    lv_obj_t * battery_body = lv_obj_create(canvas_obj);
    lv_obj_set_size(battery_body, 24, 14);
    lv_obj_align(battery_body, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(battery_body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(battery_body, 2, 0);
    lv_obj_set_style_border_color(battery_body, lv_color_hex(0xffffff), 0);  /* 白色边框 */
    lv_obj_set_style_radius(battery_body, 2, 0);
    lv_obj_set_style_pad_all(battery_body, 0, 0);
    lv_obj_clear_flag(battery_body, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 绘制电池正极（右侧小凸起） */
    lv_obj_t * battery_tip = lv_obj_create(canvas_obj);
    lv_obj_set_size(battery_tip, 3, 8);
    lv_obj_align(battery_tip, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(battery_tip, lv_color_hex(0xffffff), 0);  /* 白色填充 */
    lv_obj_set_style_bg_opa(battery_tip, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(battery_tip, 0, 0);
    lv_obj_set_style_radius(battery_tip, 1, 0);
    lv_obj_set_style_pad_all(battery_tip, 0, 0);
    lv_obj_clear_flag(battery_tip, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 绘制电量填充（100%电量） */
    lv_obj_t * battery_fill = lv_obj_create(battery_body);
    lv_obj_set_size(battery_fill, 18, 8);  /* 内部填充，留2px边距 */
    lv_obj_align(battery_fill, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(battery_fill, lv_color_hex(0xffffff), 0);  /* 白色填充 */
    lv_obj_set_style_bg_opa(battery_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(battery_fill, 0, 0);
    lv_obj_set_style_radius(battery_fill, 1, 0);
    lv_obj_set_style_pad_all(battery_fill, 0, 0);
    lv_obj_clear_flag(battery_fill, LV_OBJ_FLAG_SCROLLABLE);
    
    return canvas_obj;
}

/**
 * @brief 绘制返回图标（黑色箭头）- 50x50大小
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_return_icon(lv_obj_t * parent)
{
    /* 创建画布容器 */
    lv_obj_t * canvas_obj = lv_obj_create(parent);
    lv_obj_set_size(canvas_obj, 50, 50);  /* 返回图标大小 50x50，降低一半 */
    lv_obj_set_style_bg_opa(canvas_obj, LV_OPA_TRANSP, 0);  /* 透明背景 */
    lv_obj_set_style_border_width(canvas_obj, 0, 0);
    lv_obj_set_style_pad_all(canvas_obj, 0, 0);
    lv_obj_clear_flag(canvas_obj, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 使用line对象绘制箭头，避免使用transform API */
    static lv_point_t line_points[7];
    
    /* 定义箭头路径：< 形状 - 按比例缩小到50x50 */
    line_points[0].x = 33;  /* 横线右端 */
    line_points[0].y = 25;
    line_points[1].x = 13;  /* 横线左端/箭头尖端 */
    line_points[1].y = 25;
    line_points[2].x = 23;  /* 箭头上斜线 */
    line_points[2].y = 15;
    line_points[3].x = 13;  /* 回到箭头尖端 */
    line_points[3].y = 25;
    line_points[4].x = 23;  /* 箭头下斜线 */
    line_points[4].y = 35;
    
    /* 创建线条对象 */
    lv_obj_t * arrow_line = lv_line_create(canvas_obj);
    lv_line_set_points(arrow_line, line_points, 5);
    lv_obj_set_style_line_width(arrow_line, 4, 0);  /* 线宽4px，降低一半 */
    lv_obj_set_style_line_color(arrow_line, lv_color_hex(0x000000), 0);  /* 黑色 */
    lv_obj_set_style_line_rounded(arrow_line, true, 0);  /* 圆角端点 */
    lv_obj_center(arrow_line);
    
    return canvas_obj;
}

/**
 * @brief 绘制WiFi图标（黑色，3格信号）
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_wifi_icon(lv_obj_t * parent)
{
    /* 创建画布容器 */
    lv_obj_t * canvas_obj = lv_obj_create(parent);
    lv_obj_set_size(canvas_obj, 20, 16);  /* WiFi图标大小 20x16 */
    lv_obj_set_style_bg_opa(canvas_obj, LV_OPA_TRANSP, 0);  /* 透明背景 */
    lv_obj_set_style_border_width(canvas_obj, 0, 0);
    lv_obj_set_style_pad_all(canvas_obj, 0, 0);
    lv_obj_clear_flag(canvas_obj, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 绘制WiFi信号弧线 - 使用圆弧对象 */
    /* 第一层（最外层，最弱信号） */
    lv_obj_t * arc1 = lv_arc_create(canvas_obj);
    lv_obj_set_size(arc1, 18, 18);
    lv_obj_align(arc1, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_arc_set_bg_angles(arc1, 200, 340);
    lv_arc_set_value(arc1, 100);
    lv_arc_set_range(arc1, 0, 100);
    lv_obj_set_style_arc_width(arc1, 2, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc1, lv_color_hex(0xffffff), LV_PART_INDICATOR);  /* 白色 */
    lv_obj_set_style_arc_width(arc1, 0, LV_PART_MAIN);
    lv_obj_remove_style(arc1, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc1, LV_OBJ_FLAG_CLICKABLE);
    
    /* 第二层（中间信号） */
    lv_obj_t * arc2 = lv_arc_create(canvas_obj);
    lv_obj_set_size(arc2, 12, 12);
    lv_obj_align(arc2, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_arc_set_bg_angles(arc2, 210, 330);
    lv_arc_set_value(arc2, 100);
    lv_arc_set_range(arc2, 0, 100);
    lv_obj_set_style_arc_width(arc2, 2, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc2, lv_color_hex(0xffffff), LV_PART_INDICATOR);  /* 白色 */
    lv_obj_set_style_arc_width(arc2, 0, LV_PART_MAIN);
    lv_obj_remove_style(arc2, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc2, LV_OBJ_FLAG_CLICKABLE);
    
    /* 第三层（最内层，最强信号） */
    lv_obj_t * arc3 = lv_arc_create(canvas_obj);
    lv_obj_set_size(arc3, 6, 6);
    lv_obj_align(arc3, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_arc_set_bg_angles(arc3, 220, 320);
    lv_arc_set_value(arc3, 100);
    lv_arc_set_range(arc3, 0, 100);
    lv_obj_set_style_arc_width(arc3, 2, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc3, lv_color_hex(0xffffff), LV_PART_INDICATOR);  /* 白色 */
    lv_obj_set_style_arc_width(arc3, 0, LV_PART_MAIN);
    lv_obj_remove_style(arc3, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc3, LV_OBJ_FLAG_CLICKABLE);
    
    /* 绘制中心点 */
    lv_obj_t * center_dot = lv_obj_create(canvas_obj);
    lv_obj_set_size(center_dot, 3, 3);
    lv_obj_align(center_dot, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(center_dot, lv_color_hex(0xffffff), 0);  /* 白色 */
    lv_obj_set_style_bg_opa(center_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(center_dot, 0, 0);
    lv_obj_set_style_radius(center_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(center_dot, LV_OBJ_FLAG_SCROLLABLE);
    
    return canvas_obj;
}

/**
 * @brief 绘制流量数据图标（黑色，5G信号柱状图）
 * @param parent 父对象
 * @return 返回创建的图标对象
 */
lv_obj_t * draw_data_icon(lv_obj_t * parent)
{
    /* 创建画布容器 */
    lv_obj_t * canvas_obj = lv_obj_create(parent);
    lv_obj_set_size(canvas_obj, 18, 16);  /* 数据图标大小 18x16 */
    lv_obj_set_style_bg_opa(canvas_obj, LV_OPA_TRANSP, 0);  /* 透明背景 */
    lv_obj_set_style_border_width(canvas_obj, 0, 0);
    lv_obj_set_style_pad_all(canvas_obj, 0, 0);
    lv_obj_clear_flag(canvas_obj, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 绘制5个竖条，从左到右依次变高 */
    /* 第1个竖条（最短） */
    lv_obj_t * bar1 = lv_obj_create(canvas_obj);
    lv_obj_set_size(bar1, 2, 4);
    lv_obj_align(bar1, LV_ALIGN_BOTTOM_LEFT, 1, -1);
    lv_obj_set_style_bg_color(bar1, lv_color_hex(0xffffff), 0);  /* 白色 */
    lv_obj_set_style_bg_opa(bar1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar1, 0, 0);
    lv_obj_set_style_radius(bar1, 1, 0);
    lv_obj_clear_flag(bar1, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 第2个竖条 */
    lv_obj_t * bar2 = lv_obj_create(canvas_obj);
    lv_obj_set_size(bar2, 2, 7);
    lv_obj_align(bar2, LV_ALIGN_BOTTOM_LEFT, 4, -1);
    lv_obj_set_style_bg_color(bar2, lv_color_hex(0xffffff), 0);  /* 白色 */
    lv_obj_set_style_bg_opa(bar2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar2, 0, 0);
    lv_obj_set_style_radius(bar2, 1, 0);
    lv_obj_clear_flag(bar2, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 第3个竖条 */
    lv_obj_t * bar3 = lv_obj_create(canvas_obj);
    lv_obj_set_size(bar3, 2, 10);
    lv_obj_align(bar3, LV_ALIGN_BOTTOM_LEFT, 7, -1);
    lv_obj_set_style_bg_color(bar3, lv_color_hex(0xffffff), 0);  /* 白色 */
    lv_obj_set_style_bg_opa(bar3, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar3, 0, 0);
    lv_obj_set_style_radius(bar3, 1, 0);
    lv_obj_clear_flag(bar3, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 第4个竖条 */
    lv_obj_t * bar4 = lv_obj_create(canvas_obj);
    lv_obj_set_size(bar4, 2, 13);
    lv_obj_align(bar4, LV_ALIGN_BOTTOM_LEFT, 10, -1);
    lv_obj_set_style_bg_color(bar4, lv_color_hex(0xffffff), 0);  /* 白色 */
    lv_obj_set_style_bg_opa(bar4, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar4, 0, 0);
    lv_obj_set_style_radius(bar4, 1, 0);
    lv_obj_clear_flag(bar4, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 第5个竖条（最高） */
    lv_obj_t * bar5 = lv_obj_create(canvas_obj);
    lv_obj_set_size(bar5, 2, 14);
    lv_obj_align(bar5, LV_ALIGN_BOTTOM_LEFT, 13, -1);
    lv_obj_set_style_bg_color(bar5, lv_color_hex(0xffffff), 0);  /* 白色 */
    lv_obj_set_style_bg_opa(bar5, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar5, 0, 0);
    lv_obj_set_style_radius(bar5, 1, 0);
    lv_obj_clear_flag(bar5, LV_OBJ_FLAG_SCROLLABLE);
    
    return canvas_obj;
}
