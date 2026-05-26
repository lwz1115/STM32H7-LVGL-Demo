/**
 * @file app_icons.c
 * @brief 应用图标 - 预渲染Canvas实现（修复版�?
 *
 * 修复的两个问题：
 * 1. 黑框：LV_IMG_CF_TRUE_COLOR 不支持透明，改�?lv_img 对象显示
 *    预渲染结果，canvas 本身隐藏，只保留像素数据�?
 *    实际方案：canvas 背景设为与按钮相同颜色（不透明），消除黑框�?
 *    更简单方案：直接�?lv_obj 子对象绘制，不用 canvas�?
 *
 * 2. 返回后图标消失：rendered[] 只保护像素内容，每次重新 set_buffer
 *    绑定到新建的 canvas 对象上，缓冲区数据不丢失�?
 */

#include "app_icons.h"
#include "lvgl.h"
#include <math.h>

/* 每个图标的像素缓冲区（RGB565，60×60×2=7200字节）*/
#define ICON_PX (APP_ICON_SIZE * APP_ICON_SIZE)

static lv_color_t buf_text   [ICON_PX];
static lv_color_t buf_photo  [ICON_PX];
static lv_color_t buf_music  [ICON_PX];
static lv_color_t buf_weather[ICON_PX];
static lv_color_t buf_ai     [ICON_PX];
static lv_color_t buf_wifi   [ICON_PX];
static lv_color_t buf_temp   [ICON_PX];
static lv_color_t buf_set    [ICON_PX];
static lv_color_t buf_paint  [ICON_PX];

/* 像素内容是否已绘�?*/
static uint8_t rendered[9] = {0};

/* 按钮背景色（�?lv_player_main.c 中一致），用于填�?canvas 背景消除黑框 */
static const uint32_t btn_bg_colors[9] = {
    0x3D7EBF, 0x2E8B57, 0x8B3A8B,
    0x1A6EA8, 0x1A5276, 0x117A65,
    0xB03A2E, 0x4A4A4A, 0x7D3C98
};

/* ---- 绘图�?---- */
#define CANVAS_RECT(cv,x,y,w,h,col) do{ \
    lv_draw_rect_dsc_t _r; lv_draw_rect_dsc_init(&_r); \
    _r.bg_color=lv_color_hex(col); _r.bg_opa=LV_OPA_COVER; \
    _r.radius=0; _r.border_width=0; \
    lv_canvas_draw_rect(cv,x,y,w,h,&_r);}while(0)

#define CANVAS_RRECT(cv,x,y,w,h,r,col) do{ \
    lv_draw_rect_dsc_t _r; lv_draw_rect_dsc_init(&_r); \
    _r.bg_color=lv_color_hex(col); _r.bg_opa=LV_OPA_COVER; \
    _r.radius=(r); _r.border_width=0; \
    lv_canvas_draw_rect(cv,x,y,w,h,&_r);}while(0)

#define CANVAS_CIRCLE(cv,x,y,d,col) do{ \
    lv_draw_rect_dsc_t _r; lv_draw_rect_dsc_init(&_r); \
    _r.bg_color=lv_color_hex(col); _r.bg_opa=LV_OPA_COVER; \
    _r.radius=LV_RADIUS_CIRCLE; _r.border_width=0; \
    lv_canvas_draw_rect(cv,x,y,d,d,&_r);}while(0)

#define CANVAS_RRECT_BORDER(cv,x,y,w,h,r,bg,bw,bc) do{ \
    lv_draw_rect_dsc_t _r; lv_draw_rect_dsc_init(&_r); \
    _r.bg_color=lv_color_hex(bg); _r.bg_opa=LV_OPA_COVER; \
    _r.radius=(r); _r.border_width=(bw); \
    _r.border_color=lv_color_hex(bc); _r.border_opa=LV_OPA_COVER; \
    lv_canvas_draw_rect(cv,x,y,w,h,&_r);}while(0)

#define CANVAS_LINE(cv,x1,y1,x2,y2,col,lw) do{ \
    lv_draw_line_dsc_t _l; lv_draw_line_dsc_init(&_l); \
    _l.color=lv_color_hex(col); _l.width=(lw); _l.opa=LV_OPA_COVER; \
    lv_point_t _p[2]={{(lv_coord_t)(x1),(lv_coord_t)(y1)}, \
                       {(lv_coord_t)(x2),(lv_coord_t)(y2)}}; \
    lv_canvas_draw_line(cv,_p,2,&_l);}while(0)

/**
 * 创建 canvas，绑定缓冲区，首次用按钮背景色填充（消除黑框�?
 * idx: 图标索引 0-8，用于取对应背景色和 rendered 标志
 */
static lv_obj_t *make_canvas(lv_obj_t *parent, lv_coord_t size,
                              lv_color_t *buf, uint8_t idx)
{
    lv_obj_t *cv = lv_canvas_create(parent);
    /* 每次都重新绑定（canvas对象是新建的，必须重新绑定缓冲区�?*/
    lv_canvas_set_buffer(cv, buf, size, size, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(cv, size, size);
    /* canvas 本身不需要边�?背景 */
    lv_obj_set_style_border_width(cv, 0, 0);
    lv_obj_set_style_pad_all(cv, 0, 0);
    lv_obj_clear_flag(cv, LV_OBJ_FLAG_SCROLLABLE);

    if(!rendered[idx]) {
        /* 首次：用按钮背景色填充，这样"透明"区域显示按钮颜色而非黑色 */
        lv_canvas_fill_bg(cv, lv_color_hex(btn_bg_colors[idx]), LV_OPA_COVER);
    }
    /* 非首次：缓冲区像素内容已有，直接复用，不清空不重�?*/
    return cv;
}

/* ================================================================
 * 0. 文本阅读�?- 书本+折角+文字横线
 * ================================================================ */
lv_obj_t *draw_text_reader_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_text, 0);
    if(rendered[0]) return cv;   /* 已绘制，直接返回复用缓冲区的canvas */
    rendered[0] = 1;
    int s = size;
    CANVAS_RRECT_BORDER(cv, s*10/60, s*8/60,  s*40/60, s*46/60, 3, 0xFFFFFF, 2, 0xDDDDDD);
    CANVAS_RRECT(cv, s*38/60, s*8/60,  s*12/60, s*12/60, 0, 0xBBBBBB);
    CANVAS_LINE(cv,  s*38/60, s*8/60,  s*50/60, s*20/60, 0xFFFFFF, 2);
    CANVAS_RECT(cv,  s*10/60, s*8/60,  3, s*46/60, 0xAAAAAA);
    for(int i = 0; i < 5; i++) {
        int lw = (i==4) ? s*18/60 : s*26/60;
        CANVAS_RRECT(cv, s*15/60, s*22/60+i*(s*6/60), lw, 2, 1, 0x999999);
    }
    return cv;
}

/* ================================================================
 * 1. 图片浏览�?- 相框+风景
 * ================================================================ */
lv_obj_t *draw_photo_browser_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_photo, 1);
    if(rendered[1]) return cv;
    rendered[1] = 1;
    int s = size;
    CANVAS_RRECT_BORDER(cv, s*6/60,  s*10/60, s*48/60, s*42/60, 4, 0xFFFFFF, 3, 0xEEEEEE);
    CANVAS_RRECT(cv, s*9/60,  s*13/60, s*42/60, s*20/60, 2, 0x87CEEB);
    CANVAS_RRECT(cv, s*9/60,  s*33/60, s*42/60, s*16/60, 2, 0x5DBB63);
    CANVAS_CIRCLE(cv, s*38/60, s*15/60, s*10/60, 0xFFD700);
    lv_point_t m1[4]={{s*9/60,s*48/60},{s*23/60,s*28/60},{s*37/60,s*48/60},{s*9/60,s*48/60}};
    lv_draw_rect_dsc_t rd; lv_draw_rect_dsc_init(&rd);
    rd.bg_color=lv_color_hex(0x4A7A4A); rd.bg_opa=LV_OPA_COVER;
    lv_canvas_draw_polygon(cv, m1, 4, &rd);
    lv_point_t m2[4]={{s*30/60,s*48/60},{s*42/60,s*33/60},{s*51/60,s*48/60},{s*30/60,s*48/60}};
    rd.bg_color=lv_color_hex(0x3A6A3A);
    lv_canvas_draw_polygon(cv, m2, 4, &rd);
    return cv;
}

/* ================================================================
 * 2. 音乐播放�?- 唱片+播放三角
 * ================================================================ */
lv_obj_t *draw_music_player_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_music, 2);
    if(rendered[2]) return cv;
    rendered[2] = 1;
    int s = size;
    CANVAS_CIRCLE(cv, s*8/60,  s*8/60,  s*44/60, 0x111111);
    CANVAS_CIRCLE(cv, s*12/60, s*12/60, s*36/60, 0x333333);
    CANVAS_CIRCLE(cv, s*16/60, s*16/60, s*28/60, 0x111111);
    CANVAS_CIRCLE(cv, s*22/60, s*22/60, s*16/60, 0xFFFFFF);
    CANVAS_CIRCLE(cv, s*25/60, s*25/60, s*10/60, 0x222222);
    CANVAS_CIRCLE(cv, s*28/60, s*28/60, s*4/60,  0xFFFFFF);
    lv_point_t tri[4]={{s*32/60,s*34/60},{s*32/60,s*54/60},{s*54/60,s*44/60},{s*32/60,s*34/60}};
    lv_draw_rect_dsc_t rd; lv_draw_rect_dsc_init(&rd);
    rd.bg_color=lv_color_hex(0xFFFFFF); rd.bg_opa=LV_OPA_COVER;
    lv_canvas_draw_polygon(cv, tri, 4, &rd);
    CANVAS_RECT(cv, s*4/60, s*4/60, 3, s*14/60, 0xFFFFFF);
    CANVAS_CIRCLE(cv, s*1/60, s*16/60, s*6/60, 0xFFFFFF);
    return cv;
}

/* ================================================================
 * 3. 天气 - 太阳+�?�?
 * ================================================================ */
lv_obj_t *draw_weather_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_weather, 3);
    if(rendered[3]) return cv;
    rendered[3] = 1;
    int s = size;
    CANVAS_CIRCLE(cv, s*8/60, s*4/60, s*24/60, 0xFFCC00);
    for(int i=0;i<8;i++){
        float a=i*3.14159f/4.0f;
        int cx=s*20/60, cy=s*16/60;
        CANVAS_LINE(cv, cx+(int)(s*13/60*0.72f*cosf(a)), cy+(int)(s*13/60*0.72f*sinf(a)),
                        cx+(int)(s*13/60*cosf(a)),       cy+(int)(s*13/60*sinf(a)), 0xFFCC00, 2);
    }
    CANVAS_CIRCLE(cv, s*12/60, s*28/60, s*20/60, 0xFFFFFF);
    CANVAS_CIRCLE(cv, s*24/60, s*22/60, s*24/60, 0xFFFFFF);
    CANVAS_CIRCLE(cv, s*36/60, s*28/60, s*18/60, 0xFFFFFF);
    CANVAS_RECT(cv,   s*12/60, s*36/60, s*42/60, s*12/60, 0xFFFFFF);
    for(int i=0;i<3;i++)
        CANVAS_LINE(cv, s*(16+i*10)/60, s*50/60, s*(14+i*10)/60, s*58/60, 0x4499FF, 2);
    return cv;
}

/* ================================================================
 * 4. AI对话 - 双气�?
 * ================================================================ */
lv_obj_t *draw_ai_chat_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_ai, 4);
    if(rendered[4]) return cv;
    rendered[4] = 1;
    int s = size;
    CANVAS_RRECT(cv, s*4/60,  s*6/60,  s*38/60, s*24/60, 8, 0xFFFFFF);
    lv_point_t t1[4]={{s*8/60,s*28/60},{s*18/60,s*28/60},{s*4/60,s*38/60},{s*8/60,s*28/60}};
    lv_draw_rect_dsc_t rd; lv_draw_rect_dsc_init(&rd);
    rd.bg_color=lv_color_hex(0xFFFFFF); rd.bg_opa=LV_OPA_COVER;
    lv_canvas_draw_polygon(cv, t1, 4, &rd);
    for(int i=0;i<3;i++){
        int lw=(i==2)?s*14/60:s*24/60;
        CANVAS_RRECT(cv, s*9/60, s*11/60+i*(s*7/60), lw, 2, 1, 0x888888);
    }
    CANVAS_RRECT(cv, s*20/60, s*36/60, s*36/60, s*20/60, 6, 0x55AAFF);
    lv_point_t t2[4]={{s*40/60,s*54/60},{s*52/60,s*54/60},{s*56/60,s*60/60},{s*40/60,s*54/60}};
    rd.bg_color=lv_color_hex(0x55AAFF);
    lv_canvas_draw_polygon(cv, t2, 4, &rd);
    CANVAS_RRECT(cv, s*26/60, s*42/60, s*22/60, 2, 1, 0xFFFFFF);
    CANVAS_RRECT(cv, s*26/60, s*48/60, s*14/60, 2, 1, 0xFFFFFF);
    return cv;
}

/* ================================================================
 * 5. WiFi管理 - 扇形�?圆点
 * ================================================================ */
lv_obj_t *draw_wifi_mgr_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_wifi, 5);
    if(rendered[5]) return cv;
    rendered[5] = 1;
    int s = size;
    int cx = s/2;
    int cy = s*42/60;  /* 圆心在下�?*/
    /* 用大圆减小圆画弧环，再遮住下�?*/
    CANVAS_CIRCLE(cv, cx-s*22/60, cy-s*22/60, s*44/60, 0xFFFFFF);
    CANVAS_CIRCLE(cv, cx-s*17/60, cy-s*17/60, s*34/60, btn_bg_colors[5]);
    CANVAS_CIRCLE(cv, cx-s*14/60, cy-s*14/60, s*28/60, 0xFFFFFF);
    CANVAS_CIRCLE(cv, cx-s*10/60, cy-s*10/60, s*20/60, btn_bg_colors[5]);
    CANVAS_CIRCLE(cv, cx-s*7/60,  cy-s*7/60,  s*14/60, 0xFFFFFF);
    CANVAS_CIRCLE(cv, cx-s*4/60,  cy-s*4/60,  s*8/60,  btn_bg_colors[5]);
    /* 遮住下半部分 */
    CANVAS_RECT(cv, 0, cy, s, s-cy, btn_bg_colors[5]);
    /* 底部圆点 */
    CANVAS_CIRCLE(cv, cx-s*4/60, cy+s*2/60, s*8/60, 0xFFFFFF);
    return cv;
}

/* ================================================================
 * 6. 温湿�?- 温度�?水滴
 * ================================================================ */
lv_obj_t *draw_temp_humidity_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_temp, 6);
    if(rendered[6]) return cv;
    rendered[6] = 1;
    int s = size;
    CANVAS_RRECT_BORDER(cv, s*10/60, s*6/60,  s*10/60, s*34/60, 5, 0xFFFFFF, 2, 0xCCCCCC);
    CANVAS_RRECT(cv, s*13/60, s*20/60, s*4/60, s*18/60, 2, 0xFF3333);
    CANVAS_CIRCLE(cv, s*7/60,  s*38/60, s*16/60, 0xFF3333);
    CANVAS_CIRCLE(cv, s*9/60,  s*40/60, s*12/60, 0xFF6666);
    for(int i=0;i<3;i++)
        CANVAS_RECT(cv, s*20/60, s*10/60+i*(s*9/60), s*5/60, 2, 0xCCCCCC);
    CANVAS_CIRCLE(cv, s*36/60, s*32/60, s*18/60, 0x3399FF);
    lv_point_t drop[4]={{s*45/60,s*18/60},{s*50/60,s*30/60},{s*45/60,s*42/60},{s*40/60,s*30/60}};
    lv_draw_rect_dsc_t rd; lv_draw_rect_dsc_init(&rd);
    rd.bg_color=lv_color_hex(0x3399FF); rd.bg_opa=LV_OPA_COVER;
    lv_canvas_draw_polygon(cv, drop, 4, &rd);
    CANVAS_CIRCLE(cv, s*38/60, s*34/60, s*5/60, 0x88CCFF);
    return cv;
}

/* ================================================================
 * 7. 设置 - 齿轮
 * ================================================================ */
lv_obj_t *draw_settings_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_set, 7);
    if(rendered[7]) return cv;
    rendered[7] = 1;
    int s = size;
    int cx=s/2, cy=s/2;
    CANVAS_CIRCLE(cv, cx-s*18/60, cy-s*18/60, s*36/60, 0xFFFFFF);
    /* 8个齿 */
    int ox[8]={0,1,1,1,0,-1,-1,-1};
    int oy[8]={-1,-1,0,1,1,1,0,-1};
    for(int i=0;i<8;i++){
        int tw=(ox[i]==0)?s*8/60:s*6/60;
        int th=(oy[i]==0)?s*12/60:s*8/60;
        int tx=cx+ox[i]*s*20/60-tw/2;
        int ty=cy+oy[i]*s*20/60-th/2;
        CANVAS_RRECT(cv, tx, ty, tw, th, 2, 0xFFFFFF);
    }
    CANVAS_CIRCLE(cv, cx-s*10/60, cy-s*10/60, s*20/60, btn_bg_colors[7]);
    CANVAS_CIRCLE(cv, cx-s*5/60,  cy-s*5/60,  s*10/60, 0xCCCCCC);
    return cv;
}

/* ================================================================
 * 8. 画画 - 调色�?画笔+彩色�?
 * ================================================================ */
lv_obj_t *draw_paint_icon(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *cv = make_canvas(parent, size, buf_paint, 8);
    if(rendered[8]) return cv;
    rendered[8] = 1;
    int s = size;
    CANVAS_RRECT_BORDER(cv, s*4/60, s*8/60, s*44/60, s*40/60, 20, 0xFFFFFF, 2, 0xCCCCCC);
    CANVAS_CIRCLE(cv, s*28/60, s*32/60, s*12/60, btn_bg_colors[8]);
    uint32_t dc[]={0xFF3333,0x33CC33,0x3366FF,0xFFCC00,0xFF66CC};
    int dx[]={s*10/60,s*22/60,s*10/60,s*22/60,s*16/60};
    int dy[]={s*14/60,s*14/60,s*26/60,s*26/60,s*20/60};
    for(int i=0;i<5;i++) CANVAS_CIRCLE(cv, dx[i], dy[i], s*8/60, dc[i]);
    lv_point_t pen[5]={{s*44/60,s*10/60},{s*56/60,s*4/60},{s*58/60,s*8/60},{s*46/60,s*14/60},{s*44/60,s*10/60}};
    lv_draw_rect_dsc_t rd; lv_draw_rect_dsc_init(&rd);
    rd.bg_color=lv_color_hex(0xFFDD88); rd.bg_opa=LV_OPA_COVER;
    lv_canvas_draw_polygon(cv, pen, 5, &rd);
    lv_point_t tip[4]={{s*44/60,s*10/60},{s*46/60,s*14/60},{s*40/60,s*20/60},{s*44/60,s*10/60}};
    rd.bg_color=lv_color_hex(0xFF6633);
    lv_canvas_draw_polygon(cv, tip, 4, &rd);
    return cv;
}
