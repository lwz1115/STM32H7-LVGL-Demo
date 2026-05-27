/**
 * @file lv_conf.h
 * Configuration file for v8.2.0
 */

/*
 * Copy this file as `lv_conf.h`
 * 1. simply next to the `lvgl` folder
 * 2. or any other places and
 *    - define `LV_CONF_INCLUDE_SIMPLE`
 *    - add the path as include path
 */

/* clang-format off */
#if 1 /*Set it to "1" to enable content*/

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*********************************************************************************

                                        颜色设置
                                        
 ***********************************************************************************/

 /* 颜色深度: 1(1字节每像素), 8(RGB332), 16(RGB565), 32(ARGB8888) */
#define LV_COLOR_DEPTH                      16

/* 对于2字节的RGB565颜色格式，交换两个字节的内容(例如，如果通过SPI发送) */
#define LV_COLOR_16_SWAP                    0


/* 1: 启用屏幕透明度
 * 用于OSD(屏上显示)或其他GUI时使用
 * 注意: 当' LV_COLOR_DEPTH = 32 '时启用透明色，背景透明度通过: `style.body.opa = ...`设置*/
#define LV_COLOR_SCREEN_TRANSP              0

/* 为了颜色混合性能，添加一个混合偏移量，该值有助于GPU或CPU进行颜色混合时对舍入误差进行补偿
 * 0:禁用; 64:约x.75; 128:约一半; 192:约x.25; 254:约小于一个 */
#define LV_COLOR_MIX_ROUND_OFS              (LV_COLOR_DEPTH == 32 ? 0: 128)

/* 用于透明度混合的像素值(作为透明色) */
#define LV_COLOR_CHROMA_KEY                 lv_color_hex(0x00ff00)         /* 绿色 */



/*********************************************************************************

                                        内存设置
                                        
 ***********************************************************************************/

/* 0: 使用标准 `lv_mem_alloc()` 和 `lv_mem_free()`*/
#define LV_MEM_CUSTOM                       0
#if LV_MEM_CUSTOM == 0
    /* `lv_mem_alloc()`可用的总堆大小(单位:字节)(最少 >= 2kB)
     * 字体bin加载需要约3MB，必须放在SDRAM里
     * SDRAM布局:
     *   0xC0000000 ~ 0xC017FFFF  LVGL显示双缓冲 (800*480*2*2 = 1.5MB)
     *   0xC0200000 ~ 0xC05FFFFF  字体bin原始数据 (4MB，font_sdram.c使用)
     *   0xC0600000 ~ 0xC1FFFFFF  LVGL堆 (26MB，lv_mem_alloc使用)
     */
    #define LV_MEM_SIZE                     (26U * 1024U * 1024U)  /*[字节] 26MB SDRAM堆*/

    /* 把LVGL堆放到SDRAM，避免占用内部SRAM */
    #define LV_MEM_ADR                      0xC0600000UL           /* SDRAM 6MB偏移处 */
    /* 替换内存分配函数，当LV_MEM_ADR为0时有效 */
    #if LV_MEM_ADR == 0
        //#define LV_MEM_POOL_INCLUDE your_alloc_library  /* 可选: 包含自动分配内存函数的文件 */
        //#define LV_MEM_POOL_ALLOC   your_alloc          /* 可选: 替换为您的内存函数 */
    #endif

#else       /*LV_MEM_CUSTOM*/
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /* 用于分配函数的标准头文件 */
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif     /*LV_MEM_CUSTOM*/

/* 当内存分配失败时，在失败前尝试释放内存的次数
 * 如果仍然失败，将使用默认值，但显示警告信息时使用
 * 建议如果需要，可以覆盖默认的转储函数。 */
#define LV_MEM_BUF_MAX_NUM                  16

/* 如果LVGL使用标准 `memcpy` 和 `memset` 而不是LVGL自己的函数
 * (标准函数可能在特定平台实现更快，但会消耗更多代码空间) */
#define LV_MEMCPY_MEMSET_STD                0



/*********************************************************************************

                                        HAL 设置
                                        
 ***********************************************************************************/
 
/* 默认显示刷新周期，单位为毫秒 */
#define LV_DISP_DEF_REFR_PERIOD             8      /*[ms]*/  /* 优化：从4ms改为8ms，降低CPU负载 */

/* 默认输入设备读取周期(单位为毫秒) */
#define LV_INDEV_DEF_READ_PERIOD            4     /*[ms]*/

/* 使用自动的tick来源
 * 如果启用，则需要通过 `lv_tick_inc()` 定期提供tick值 */
#define LV_TICK_CUSTOM                      0
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE          "FreeRTOS.h"                /* 操作系统时间函数的标准头文件 */
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR    (xTaskGetTickCount())       /* 获取当前系统时间的表达式(单位:毫秒) */
#endif   /*LV_TICK_CUSTOM*/


/* 显示设备默认每英寸点数(dpi)
 * 用于缩放显示对象的大小并获取正确的缩放值
 * (通常为130) */
#define LV_DPI_DEF                          130     /*[px/inch]*/



/*********************************************************************************

                                        绘图设置
                                        
 ***********************************************************************************/
/*-------------
 * 1. 图形绘制
 *-----------*/


/* 启用复杂的图形绘制
 * 例如圆角、阴影、渐变等需要复杂处理的功能
 * 如果禁用，所有这些功能将无法使用 */
#define LV_DRAW_COMPLEX                     1
#if LV_DRAW_COMPLEX != 0

    /* 绘制阴影时水平扩展屏幕缓冲的大小
     * LV_SHADOW_CACHE_SIZE表示每个阴影渲染时会调用 `圆角半径 + 模糊大小`
     * 最大内存约为 LV_SHADOW_CACHE_SIZE^2 字节 */
    #define LV_SHADOW_CACHE_SIZE            0

    /* 用于圆角绘制的缓存大小
     * 每个四分之一圆使用约1/4个圆的内存进行缓存。
     * 注意: 该值*4等于圆的内存(如果缓存启用，也会缓存阴影)
     * 0: 禁用缓存 */
    #define LV_CIRCLE_CACHE_SIZE            4
    
#endif /*LV_DRAW_COMPLEX*/

/* 图像缓存大小。
 * 在图像渲染时，LVGL将其保存在RAM中，以便下次使用时不需再次解码。
 * 0: 禁用图像缓存 */
#define LV_IMG_CACHE_DEF_SIZE               0  /* 优化：无背景图片，禁用缓存节省内存 */

/* 渐变停止点的最大数量
 * 如果使用更多，则成本较高，该值较小可以减少内存消耗
 * 每个停止点开销为(sizeof(lv_color_t) + 1)字节 */
#define LV_GRADIENT_MAX_STOPS               2

/* 渐变缓存大小
 * 当LVGL绘制渐变时，会保存渐变结果以便后续重用
 * 如果启用，LV_GRAD_CACHE_DEF_SIZE指定存储多少个渐变项。
 * 0: 禁用缓存 */
#define LV_GRAD_CACHE_DEF_SIZE              0

/* 启用渐变抖动
 * 如果像素尺寸较小(如32px或更小)，可以改善渐变的外观
 * 注意: 对于24位颜色深度，通过抖动可以显著提高质量，但可能会降低性能 */
#define LV_DITHER_GRADIENT                  0
#if LV_DITHER_GRADIENT
    /* 抖动错误扩散算法
     * 如果启用，渐变效果更好但需要更多CPU时间
     * 如果禁用，仅对32位颜色深度有效(如果24位或32位颜色深度有效) */
    #define LV_DITHER_ERROR_DIFFUSION       0
#endif

/* 旋转显示时使用的最大缓冲区大小
 * 该值指定旋转显示时临时存储的数据量 */
#define LV_DISP_ROT_MAX_BUF                 (10*1024)

/*-------------
 * 2. GPU
 *-----------*/

/* 使用STM32的DMA2D(Chrom Art) GPU加速 */
#define LV_USE_GPU_STM32_DMA2D              1
#if LV_USE_GPU_STM32_DMA2D
    /* 配置对应CMSIS头文件名
       例如: stm32f769xx.h 或 stm32f429xx.h */
    #define LV_GPU_DMA2D_CMSIS_INCLUDE "stm32h7xx.h"
#endif

/* 使用NXP的PXP GPU(i.MX RT系列) */
#define LV_USE_GPU_NXP_PXP                  0
#if LV_USE_GPU_NXP_PXP
    /*1: 使用PXP的自动初始化(lv_gpu_nxp_pxp_osa.c)
     *   确保处理器支持FreeRTOS内核
     *   需要在lv_init()之前调用lv_gpu_nxp_pxp_init()并成功
     *   前提是SDK_OS_FREE_RTOS宏已定义。
     *   如果不需要，可以不调用lv_gpu_nxp_pxp_init()
     *0: 不使用自动初始化，需要在lv_init()之前手动调用lv_gpu_nxp_pxp_init()
    */
    #define LV_USE_GPU_NXP_PXP_AUTO_INIT    0
#endif

/* 使用NXP的VG-Lite GPU(i.MX RT系列) */
#define LV_USE_GPU_NXP_VG_LITE              0

/* 使用SDL GPU加速API */
#define LV_USE_GPU_SDL                      0
#if LV_USE_GPU_SDL
    #define LV_GPU_SDL_INCLUDE_PATH <SDL2/SDL.h>
    /* 缓存大小，用于纹理缓存等，如果使用VRAM，可能需要8MB */
    #define LV_GPU_SDL_LRU_SIZE (1024 * 1024 * 8)
    /* 如果SDL版本>=2.0.6支持自动混合模式 */
    #define LV_GPU_SDL_CUSTOM_BLEND_MODE (SDL_VERSION_ATLEAST(2, 0, 6))
#endif

/*-------------
 * 3. 日志
 *-----------*/

/* 启用日志记录 */
#define LV_USE_LOG                          0
#if LV_USE_LOG

    /*日志记录详细程度:
    *LV_LOG_LEVEL_TRACE       记录所有信息(最详细)
    *LV_LOG_LEVEL_INFO        记录重要信息
    *LV_LOG_LEVEL_WARN        记录可能恢复的错误
    *LV_LOG_LEVEL_ERROR       记录致命错误
    *LV_LOG_LEVEL_USER        记录用户自定义消息
    *LV_LOG_LEVEL_NONE        不记录任何信息*/
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /*1: 使用'printf'打印日志;
     *0: 需要使用 'lv_log_register_print_cb()' 注册自定义打印回调 */
    #define LV_LOG_PRINTF                   0

    /* 启用/禁用LV_LOG_TRACE日志的详细信息 */
    #define LV_LOG_TRACE_MEM                1
    #define LV_LOG_TRACE_TIMER              1
    #define LV_LOG_TRACE_INDEV              1
    #define LV_LOG_TRACE_DISP_REFR          1
    #define LV_LOG_TRACE_EVENT              1
    #define LV_LOG_TRACE_OBJ_CREATE         1
    #define LV_LOG_TRACE_LAYOUT             1
    #define LV_LOG_TRACE_ANIM               1

#endif  /*LV_USE_LOG*/

/*-------------
 * 4. 断言
 *-----------*/

/* 启用断言可检查代码中的边界和一致性，可以减小代码大小
 * 提示: 如果LV_USE_LOG启用，断言失败时会打印详细信息 */
#define LV_USE_ASSERT_NULL                  1   /* 检查指针是否为NULL(基本检查，建议启用) */
#define LV_USE_ASSERT_MALLOC                1   /* 检查内存分配是否成功(基本检查，建议启用) */
#define LV_USE_ASSERT_STYLE                 0   /* 检查样式是否有效(如果经常遇到样式问题可以启用) */
#define LV_USE_ASSERT_MEM_INTEGRITY         0   /* 检查lv_mem内部结构是否损坏(如果内存问题可以启用) */
#define LV_USE_ASSERT_OBJ                   0   /* 检查对象指针是否有效(如果对象问题可以启用) */

/* 断言失败时的处理函数 */
#define LV_ASSERT_HANDLER_INCLUDE           <stdint.h>
#define LV_ASSERT_HANDLER while(1);         /* 死循环 */

/*-------------
 * 5. 调试
 *-----------*/

/* 1: 显示CPU使用率和FPS */
#define LV_USE_PERF_MONITOR                 1
#if LV_USE_PERF_MONITOR
    #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
#endif

/* 1: 显示内存使用情况
 * 注意: 需要 LV_MEM_CUSTOM = 0*/
#define LV_USE_MEM_MONITOR                  0

/* 1: 在显示刷新时启用调试，显示刷新区域或类似调试信息 */
#define LV_USE_REFR_DEBUG                   0

/* 使用自定义的(v)snprintf函数 */
#define LV_SPRINTF_CUSTOM                   0
#if LV_SPRINTF_CUSTOM
    #define LV_SPRINTF_INCLUDE  <stdio.h>
    #define lv_snprintf         snprintf
    #define lv_vsnprintf        vsnprintf
#else   /*LV_SPRINTF_CUSTOM*/
    #define LV_SPRINTF_USE_FLOAT            0
#endif  /*LV_SPRINTF_CUSTOM*/

#define LV_USE_USER_DATA                    1

/* 垃圾回收(GC)支持
 * 如果lvgl使用支持GC的环境，需要在GC时注册相关对象
 * 默认情况下不使用 */
#define LV_ENABLE_GC                        0
#if LV_ENABLE_GC != 0
    #define LV_GC_INCLUDE "gc.h"                           /* 包含垃圾回收的头文件 */
#endif /*LV_ENABLE_GC*/

 
 
/*********************************************************************************

                                        编译器设置
                                        
 ***********************************************************************************/
/* 大端系统: 1表示大端 */
#define LV_BIG_ENDIAN_SYSTEM                0

/* 为 ' lv_tick_inc ' 添加属性(例如__attribute__((...))) */
#define LV_ATTRIBUTE_TICK_INC

/* 为 ' lv_timer_handler ' 添加属性(例如__attribute__((...))) */
#define LV_ATTRIBUTE_TIMER_HANDLER

/* 为 ' lv_disp_flush_ready ' 添加属性(例如__attribute__((...))) */
#define LV_ATTRIBUTE_FLUSH_READY

/* 内存分配的最小对齐要求
 * 默认值为1，使用1字节对齐即可。 */
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE         1

/* 内存对齐属性(例如__attribute__((aligned(4)))) */
#define LV_ATTRIBUTE_MEM_ALIGN

/* 大型常量属性(例如__attribute__((section(".rodata")))) */
#define LV_ATTRIBUTE_LARGE_CONST

/* 大型RAM数组属性(例如__attribute__((section(".bss")))) */
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/* 快速内存属性(例如放置于RAM中) */
#define LV_ATTRIBUTE_FAST_MEM

/* 用于GPU访问的内存属性(例如DMA可访问的内存) */
#define LV_ATTRIBUTE_DMA

/* 如果LV_<CONST>常量需要作为int值导出，用于Micropython等其他环境
 * 不影响LVGL自己的API */
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /* 抑制GCC警告 */

/* 支持大坐标(>= 32k..32k)和小坐标(-4M..4M)使用int32_t vs int16_t
 * 如果坐标可能超出-32k..32k范围，建议选择启用。 */
#define LV_USE_LARGE_COORD                  0




/*********************************************************************************

                                        字体设置
                                        
 ***********************************************************************************/
/* 启用内置字体。默认字体为ASCII字符，bpp = 4
 * 字体来源: https://fonts.google.com/specimen/Montserrat */
#define LV_FONT_MONTSERRAT_8                0
#define LV_FONT_MONTSERRAT_10               0
#define LV_FONT_MONTSERRAT_12               1
#define LV_FONT_MONTSERRAT_14               1
#define LV_FONT_MONTSERRAT_16               1
#define LV_FONT_MONTSERRAT_18               0
#define LV_FONT_MONTSERRAT_20               1
#define LV_FONT_MONTSERRAT_22               0
#define LV_FONT_MONTSERRAT_24               1
#define LV_FONT_MONTSERRAT_26               0
#define LV_FONT_MONTSERRAT_28               1
#define LV_FONT_MONTSERRAT_30               0
#define LV_FONT_MONTSERRAT_32               0
#define LV_FONT_MONTSERRAT_34               0
#define LV_FONT_MONTSERRAT_36               0
#define LV_FONT_MONTSERRAT_38               0
#define LV_FONT_MONTSERRAT_40               0
#define LV_FONT_MONTSERRAT_42               0
#define LV_FONT_MONTSERRAT_44               0
#define LV_FONT_MONTSERRAT_46               0
#define LV_FONT_MONTSERRAT_48               1

/* 其他专用字体 */
#define LV_FONT_MONTSERRAT_12_SUBPX         0
#define LV_FONT_MONTSERRAT_28_COMPRESSED    0  /* bpp = 3 */
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW    0  /* 希伯来语、波斯语、阿拉伯语等支持`unicode`范围 */
#define LV_FONT_SIMSUN_16_CJK               1  /* 1000多个常用CJK字符集字体 */

/* 默认的UNSCII字体 */
#define LV_FONT_UNSCII_8                    0
#define LV_FONT_UNSCII_16                   0

/* 自定义字体声明
 * 每个自定义字体需要使用LV_FONT_DECLARE(my_font_1)进行声明
 * 然后每处需要时包含该声明 */
#define LV_FONT_CUSTOM_DECLARE

/* 默认字体，当对象显示文本时使用的默认字体。 */
#define LV_FONT_DEFAULT                     &lv_font_montserrat_14

/* 如果字体大小>16像素，字形索引可能超过256个字符
 * 如果字体大小>16像素，可能需要启用此选项
 * 如果启用，字形索引将存储为uint16_t而不是uint8_t
 * 但会占用更多内存 */
#define LV_FONT_FMT_TXT_LARGE               0

/* 启用压缩字体(如lv_font_montserrat_28_compressed) */
#define LV_USE_FONT_COMPRESSED              0

/* 启用字体亚像素渲染 */
#define LV_USE_FONT_SUBPX                   0
#if LV_USE_FONT_SUBPX
    /* 亚像素渲染顺序RGB或BGR
     * 使用RGB顺序显示时设置为0，BGR顺序显示时设置为1 */
    #define LV_FONT_SUBPX_BGR               0  /* 0: RGB;1: BGR顺序 */
#endif

/*********************************************************************************

                                        文本设置
                                        
 ***********************************************************************************/
/**
 * 选择字符编码。
 * 如果IDE支持UTF-8，请使用UTF-8编码。
 * 否则使用ASCII编码。
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/* 用于自动换行的字符(如标点符号) */
#define LV_TXT_BREAK_CHARS                  " ,.;:-_"

/* 当文本行长度超过最大值时启用自动换行
 * 如果值<=0则使用默认值，否则使用该值 */
#define LV_TXT_LINE_BREAK_LONG_LEN          0

/* 换行时最小前缀长度
 * 当使用LV_TXT_LINE_BREAK_LONG_LEN时有效 */
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3

/* 换行时最小后缀长度
 * 当使用LV_TXT_LINE_BREAK_LONG_LEN时有效 */
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/* 文本颜色命令前缀字符 */
#define LV_TXT_COLOR_CMD                    "#"

/* 支持双向文本(BiDi)
 * 允许处理从左到右(LTR)和从右到左(RTL)的文本混合
 * 参考: https://www.w3.org/International/articles/inline-bidi-markup/uba-basics */
#define LV_USE_BIDI                         0
#if LV_USE_BIDI
    /* 默认基本方向:
    *`LV_BASE_DIR_LTR` 从左到右
    *`LV_BASE_DIR_RTL` 从右到左
    *`LV_BASE_DIR_AUTO` 自动检测 */
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/* 支持阿拉伯/波斯语字符形状处理
 * 根据字符位置改变字符形状以正确连接 */
#define LV_USE_ARABIC_PERSIAN_CHARS         0



/*********************************************************************************

                                        部件设置
                                        
 ***********************************************************************************/
/* 部件列表: https://docs.lvgl.io/latest/en/html/widgets/index.html */

#define LV_USE_ARC                          1

#define LV_USE_ANIMIMG                      1

#define LV_USE_BAR                          1

#define LV_USE_BTN                          1

#define LV_USE_BTNMATRIX                    1

#define LV_USE_CANVAS                       1

#define LV_USE_CHECKBOX                     1

#define LV_USE_DROPDOWN                     1   /* 依赖: lv_label */

#define LV_USE_IMG                          1   /* 依赖: lv_label */

#define LV_USE_LABEL                        1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION         1  /* 支持文本选择 */
    #define LV_LABEL_LONG_TXT_HINT          1  /* 当文本显示提示超出显示范围时显示 */
#endif

#define LV_USE_LINE                         1

#define LV_USE_ROLLER                       1  /* 依赖: lv_label */
#if LV_USE_ROLLER
    #define LV_ROLLER_INF_PAGES             7  /* 当滚动时无限滚动的页面数 */
#endif

#define LV_USE_SLIDER                       1  /* 依赖: lv_bar*/

#define LV_USE_SWITCH                       1

#define LV_USE_TEXTAREA                     1  /* 依赖: lv_label*/
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME   1500    /*ms*/
#endif

#define LV_USE_TABLE                        1


/*********************************************************************************

                                        扩展部件
                                        
 ***********************************************************************************/
/*-----------
 * 1. 扩展部件
 *----------*/
#define LV_USE_CALENDAR                     1
#if LV_USE_CALENDAR
    #define LV_CALENDAR_WEEK_STARTS_MONDAY  0
    #if LV_CALENDAR_WEEK_STARTS_MONDAY
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
    #else
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
    #endif

    #define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March",  "April", "May",  "June", "July", "August", "September", "October", "November", "December"}
    #define LV_USE_CALENDAR_HEADER_ARROW    1
    #define LV_USE_CALENDAR_HEADER_DROPDOWN 1
#endif  /*LV_USE_CALENDAR*/

#define LV_USE_CHART                        1

#define LV_USE_COLORWHEEL                   1

#define LV_USE_IMGBTN                       1

#define LV_USE_KEYBOARD                     1

#define LV_USE_LED                          1

#define LV_USE_LIST                         1

#define LV_USE_MENU                         1

#define LV_USE_METER                        1

#define LV_USE_MSGBOX                       1

#define LV_USE_SPINBOX                      1

#define LV_USE_SPINNER                      1

#define LV_USE_TABVIEW                      1

#define LV_USE_TILEVIEW                     1

#define LV_USE_WIN                          1

#define LV_USE_SPAN                         1
#if LV_USE_SPAN
    /* 用于span的摘要堆栈大小，用于获取部分文本并显示 */
    #define LV_SPAN_SNIPPET_STACK_SIZE      64
#endif

/*-----------
 * 2. 主题
 *----------*/

/* 默认主题，支持多种样式和动画 */
#define LV_USE_THEME_DEFAULT                1
#if LV_USE_THEME_DEFAULT

    /* 0: 浅色主题;1: 深色主题 */
    #define LV_THEME_DEFAULT_DARK           0

    /* 1: 启用主题生长效果 */
    #define LV_THEME_DEFAULT_GROW           1

    /* 默认主题动画时间[ms] */
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif /*LV_USE_THEME_DEFAULT*/

/* 基础主题，非常轻量 */
#define LV_USE_THEME_BASIC                  1

/* 单色主题，仅使用黑白颜色 */
#define LV_USE_THEME_MONO                   1

/*-----------
 * 3. 布局
 *----------*/

/* 支持Flexbox布局 */
#define LV_USE_FLEX                         1

/* 支持Grid布局 */
#define LV_USE_GRID                         1

/*---------------------
 * 4. 文件系统
 *--------------------*/

/* 标准C的stdio文件系统接口 */
 
/* STDIO */
#define LV_USE_FS_STDIO            0
#if LV_USE_FS_STDIO
    #define LV_FS_STDIO_LETTER      '\0'        /* 驱动字母(例如 'S' 表示 "S:") */
    #define LV_FS_STDIO_PATH        ""          /* 设置默认的工作目录，可以是绝对路径 */
    #define LV_FS_STDIO_CACHE_SIZE  0           /* >0时对lv_fs_read()进行缓存，缓存相应大小数据 */
#endif

/* POSIX标准接口(open, read, write等) */
#define LV_USE_FS_POSIX             0
#if LV_USE_FS_POSIX
    #define LV_FS_POSIX_LETTER      '\0'        /* 驱动字母(例如 'S' 表示 "S:") */
    #define LV_FS_POSIX_PATH        ""          /* 设置默认的工作目录，可以是绝对路径 */
    #define LV_FS_POSIX_CACHE_SIZE  0           /* >0时对lv_fs_read()进行缓存，缓存相应大小数据 */
#endif

/* Windows标准API接口 */
#define LV_USE_FS_WIN32             0
#if LV_USE_FS_WIN32
    #define LV_FS_WIN32_LETTER      '\0'        /* 驱动字母(例如 'S' 表示 "S:") */
    #define LV_FS_WIN32_PATH        ""          /* 设置默认的工作目录，可以是绝对路径 */
    #define LV_FS_WIN32_CACHE_SIZE  0           /* >0时对lv_fs_read()进行缓存，缓存相应大小数据 */
#endif

/* FATFS文件系统接口 */
#define LV_USE_FS_FATFS             1
#if LV_USE_FS_FATFS
    #define LV_FS_FATFS_LETTER      'S'         /* 驱动字母(例如 'S' 表示 "S:") */
    #define LV_FS_FATFS_CACHE_SIZE  0           /* >0时对lv_fs_read()进行缓存，缓存相应大小数据 */
#endif

/* PNG图像解码支持 */
#define LV_USE_PNG                          1

/* BMP图像解码支持 */
#define LV_USE_BMP                          1

/* JPEG图像解码支持(单帧版本) */
#define LV_USE_SJPG                         1

/* GIF图像解码支持 */
#define LV_USE_GIF                          0

/* 二维码生成支持 */
#define LV_USE_QRCODE                       0

/* FreeType字体渲染支持 */
#define LV_USE_FREETYPE                     0
#if LV_USE_FREETYPE
    /* FreeType缓存大小(字节)，-1表示无限制 */
    #define LV_FREETYPE_CACHE_SIZE          (16 * 1024)
    #if LV_FREETYPE_CACHE_SIZE >= 0
        /* 1: 使用位图缓存; 0: 使用轮廓缓存
         * 位图缓存占用内存较小(通常<256)
         * 轮廓缓存渲染效果更好但更耗内存 */
        #define LV_FREETYPE_SBIT_CACHE      0
        /* 缓存的FT_Face/FT_Size对象数量，设置为0表示不缓存
           (0: 不缓存) */
        #define LV_FREETYPE_CACHE_FT_FACES  0
        #define LV_FREETYPE_CACHE_FT_SIZES  0
    #endif
#endif

/* Rlottie矢量动画支持 */
#define LV_USE_RLOTTIE                      0

/* FFmpeg音视频支持 */
#define LV_USE_FFMPEG                       0
#if LV_USE_FFMPEG
    /* 转储FFmpeg av_log信息到stderr */
    #define LV_FFMPEG_AV_DUMP_FORMAT        0
#endif

/*-----------
 * 5. 其他功能
 *----------*/

/* 1: 支持屏幕截图API */
#define LV_USE_SNAPSHOT                     1

/* 1: 支持Monkey测试工具 */
#define LV_USE_MONKEY                       0

/* 1: 支持Grid导航模式 */
#define LV_USE_GRIDNAV                      0

/*********************************************************************************

                                        示例
                                        
 ***********************************************************************************/
/* 启用内置示例 */
#define LV_BUILD_EXAMPLES                   1

/*===================
 * 演示程序
 ====================*/

/* 小部件演示，显示各种小部件，确保LV_MEM_SIZE足够大 */
#define LV_USE_DEMO_WIDGETS                 0
#if LV_USE_DEMO_WIDGETS
#define LV_DEMO_WIDGETS_SLIDESHOW           0
#endif

/* 键盘和编码器演示，显示键盘和编码器的小部件演示 */
#define LV_USE_DEMO_KEYPAD_AND_ENCODER      0

/* 基准测试演示，显示标准基准测试演示 */
#define LV_USE_DEMO_BENCHMARK               0

/* 压力测试演示，显示压力测试演示 */
#define LV_USE_DEMO_STRESS                  0

/* 音乐演示，显示音乐播放器演示 */
#define LV_USE_DEMO_MUSIC                   1
#if LV_USE_DEMO_MUSIC
# define LV_DEMO_MUSIC_SQUARE               0
# define LV_DEMO_MUSIC_LANDSCAPE            0
# define LV_DEMO_MUSIC_ROUND                0
# define LV_DEMO_MUSIC_LARGE                0
# define LV_DEMO_MUSIC_AUTO_PLAY            0
#endif

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/

#endif /*End of "Content enable"*/

