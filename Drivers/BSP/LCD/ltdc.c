/**
 ****************************************************************************************************

 ****************************************************************************************************
 */

#include "./BSP/LCD/ltdc.h"
#include "./BSP/LCD/lcd.h"
#include "./SYSTEM/delay/delay.h"


LTDC_HandleTypeDef  g_ltdc_handle;       /* LTDC��� */
DMA2D_HandleTypeDef g_dma2d_handle;      /* DMA2D��� */

#if !(__ARMCC_VERSION >= 6010050)                                                     /* ����AC6����������ʹ��AC5������ʱ */

/* ���ݲ�ͬ����ɫ��ʽ,����֡�������� */
#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888 || LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
    uint32_t ltdc_lcd_framebuf[1280][800] __attribute__((at(LTDC_FRAME_BUF_ADDR)));   /* ����������ֱ���ʱ,LTDC�����֡���������С */
#else
    uint16_t ltdc_lcd_framebuf[1280][800] __attribute__((at(LTDC_FRAME_BUF_ADDR)));   /* ����������ֱ���ʱ,LTDC�����֡���������С */
//    uint16_t ltdc_lcd_framebuf[1280][800] __attribute__((at(LTDC_FRAME_BUF_ADDR + 1280 * 800 * 2)));/* ʹ��LTDC��2ʱʹ�ã�Ĭ��ʹ��LTDC��1�� */
#endif

#else      /* ʹ��AC6������ʱ */

/* ���ݲ�ͬ����ɫ��ʽ,����֡�������� - �޸�Ϊʵ����Ļ�ߴ� 800×480 */
#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888 || LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
    uint32_t ltdc_lcd_framebuf[800][480] __attribute__((section(".bss.ARM.__at_0XC0000000")));  /* 800×480��Ļ��֡����768KB */
#else
    uint16_t ltdc_lcd_framebuf[800][480] __attribute__((section(".bss.ARM.__at_0XC0000000")));  /* 800×480��Ļ��֡����768KB */
#endif

#endif

uint32_t *g_ltdc_framebuf[2];              /* LTDC LCD֡��������ָ��,����ָ���Ӧ��С���ڴ����� */
_ltdc_dev lcdltdc;                         /* ����LCD LTDC����Ҫ���� */

/**
 * @brief       LTDC����
 * @param       sw   : 1 ��,0���ر�
 * @retval      ��
 */
void ltdc_switch(uint8_t sw)
{
    if (sw)
    {
        __HAL_LTDC_ENABLE(&g_ltdc_handle);
    }
    else
    {
        __HAL_LTDC_DISABLE(&g_ltdc_handle);
    }
}

/**
 * @brief       LTDC����ָ����
 * @param       layerx       : 0,��һ��; 1,�ڶ���
 * @param       sw           : 1 ��;   0�ر�
 * @retval      ��
 */
void ltdc_layer_switch(uint8_t layerx, uint8_t sw)
{
    if (sw) 
    {
        __HAL_LTDC_LAYER_ENABLE(&g_ltdc_handle, layerx);
    }
    else
    {
        __HAL_LTDC_LAYER_DISABLE(&g_ltdc_handle, layerx);
    }

    __HAL_LTDC_RELOAD_CONFIG(&g_ltdc_handle);
}

/**
 * @brief       LTDCѡ���
 * @param       layerx   : ���;0,��һ��; 1,�ڶ���;
 * @retval      ��
 */
void ltdc_select_layer(uint8_t layerx)
{
    lcdltdc.activelayer = layerx;
}

/**
 * @brief       LTDC��ʾ��������
 * @param       dir          : 0,������1,����
 * @retval      ��
 */
void ltdc_display_dir(uint8_t dir)
{
    lcdltdc.dir = dir;    /* ��ʾ���� */

    if (dir == 0)         /* ���� */
    {
        lcdltdc.width = lcdltdc.pheight;
        lcdltdc.height = lcdltdc.pwidth;
    }
    else if (dir == 1)    /* ���� */
    {
        lcdltdc.width = lcdltdc.pwidth;
        lcdltdc.height = lcdltdc.pheight;
    }
}

/**
 * @brief       LTDC���㺯��
 * @param       x,y         : д������
 * @param       color       : ��ɫֵ
 * @retval      ��
 */
void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color)
{ 
#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888 || LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
    if (lcdltdc.dir)   /* ���� */
    {
        *(uint32_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * y + x)) = color;
    }
    else               /* ���� */
    {
        *(uint32_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y)) = color; 
    }
#else
    if (lcdltdc.dir)   /* ���� */
    {
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * y + x)) = color;
    }
    else              /* ���� */
    {
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y)) = color; 
    }
#endif
}

/**
 * @brief       LTDC���㺯��
 * @param       x,y       : ��ȡ�������
 * @retval      ����ֵ:��ɫֵ
 */
uint32_t ltdc_read_point(uint16_t x, uint16_t y)
{ 
#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888 || LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
    if (lcdltdc.dir)   /* ���� */
    {
        return *(uint32_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * y + x));
    }
    else               /* ���� */
    {
        return *(uint32_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y)); 
    }
#else
    if (lcdltdc.dir)   /* ���� */
    {
        return *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * y + x));
    }
    else               /* ���� */
    {
        return *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y)); 
    }
#endif 
}

/**
 * @brief       LTDC������,DMA2D���
 * @note       (sx,sy),(ex,ey):�����ζԽ�����,�����СΪ:(ex - sx + 1) * (ey - sy + 1)
 *              ע��:sx,ex,���ܴ���lcddev.width - 1; sy,ey,���ܴ���lcddev.height - 1
 * @param       sx,sy       : ��ʼ����
 * @param       ex,ey       : ��������
 * @param       color       : ������ɫ
 * @retval      ��
 */
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{ 
    uint32_t psx, psy, pex, pey;        /* ��LCD���Ϊ��׼������ϵ,����������仯���仯 */
    uint32_t timeout = 0; 
    uint16_t offline;
    uint32_t addr; 

    /* ����ϵת�� */
    if (lcdltdc.dir)                    /* ���� */
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else                                /* ���� */
    {
        if (ex >= lcdltdc.pheight)
        {
            ex = lcdltdc.pheight - 1;   /* ���Ʒ�Χ */
        }

        if (sx >= lcdltdc.pheight)
        {
            sx = lcdltdc.pheight - 1;   /* ���Ʒ�Χ */
        }
        psx = sy;
        psy = lcdltdc.pheight - ex - 1;
        pex = ey;
        pey = lcdltdc.pheight - sx - 1;
    }

    offline = lcdltdc.pwidth - (pex - psx + 1);
    addr = ((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * psy + psx));

    __HAL_RCC_DMA2D_CLK_ENABLE();                              /* ʹ��DM2Dʱ�� */

    DMA2D->CR &= ~(DMA2D_CR_START);                            /* ��ֹͣDMA2D */
    DMA2D->CR = DMA2D_R2M;                                     /* �Ĵ������洢��ģʽ */
    DMA2D->OPFCCR = LTDC_PIXFORMAT;                            /* ������ɫ��ʽ */
    DMA2D->OOR = offline;                                      /* ������ƫ��  */

    DMA2D->OMAR = addr;                                        /* ����洢����ַ */
    DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16);    /* �趨�����Ĵ��� */
    DMA2D->OCOLR = color;                                      /* �趨�����ɫ�Ĵ��� */
    DMA2D->CR |= DMA2D_CR_START;                               /* ����DMA2D */

    while ((DMA2D->ISR & (DMA2D_FLAG_TC)) == 0)                /* �ȴ�������� */
    {
        timeout++;
        if (timeout > 0X1FFFFF)break;                          /* ��ʱ�˳� */
    } 
    DMA2D->IFCR |= DMA2D_FLAG_TC;                              /* ���������ɱ�־ */
}

///* ʹ��DMA2D��ص�HAL����ʹ��DMA2D����(���Ƽ�) */
///**
// * @brief       ��ָ����������䵥����ɫ
// * @param       (sx,sy),(ex,ey)  : �����ζԽ�����,�����СΪ:(ex-sx+1)*(ey-sy+1)
// * @param       color            : Ҫ������ɫ
// * @retval      ��
// */
//void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
//{
//    uint32_t psx,psy,pex,pey;   /* ��LCD���Ϊ��׼������ϵ,����������仯���仯 */
//    uint32_t timeout = 0; 
//    uint16_t offline;
//    uint32_t addr;  
//
//    if (ex >= lcdltdc.width)
//    {
//        ex = lcdltdc.width - 1;
//    }
//
//    if (ey >= lcdltdc.height)
//    {
//        ey = lcdltdc.height - 1;
//    }
//
//    /* ����ϵת�� */
//    if (lcdltdc.dir)   /* ���� */
//    {
//        psx = sx; 
//        psy = sy;
//        pex = ex;
//        pey = ey;
//    }
//    else             /* ���� */
//    {
//        psx = sy;
//        psy = lcdltdc.pheight - ex - 1;
//        pex = ey;
//        pey = lcdltdc.pheight - sx - 1;
//    }

//    offline = lcdltdc.pwidth - (pex - psx + 1);
//    addr =((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * psy + psx));
//
//    if (LTDC_PIXFORMAT == LTDC_PIXEL_FORMAT_RGB565)  /* �����RGB565��ʽ�Ļ���Ҫ������ɫת������16bitת��Ϊ32bit�� */
//    {
//        color = ((color & 0X0000F800) << 8) | ((color & 0X000007E0) << 5 ) | ((color & 0X0000001F) << 3);
//    }
//
//    /* ����DMA2D��ģʽ */
//    g_dma2d_handle.Instance = DMA2D;
//    g_dma2d_handle.Init.Mode = DMA2D_R2M;                                                    /* �ڴ浽�洢��ģʽ */
//    g_dma2d_handle.Init.ColorMode = LTDC_PIXFORMAT;                                          /* ������ɫ��ʽ */
//    g_dma2d_handle.Init.OutputOffset = offline;                                              /* ���ƫ��  */
//    HAL_DMA2D_Init(&g_dma2d_handle);                                                         /* ��ʼ��DMA2D */
//
//    HAL_DMA2D_ConfigLayer(&g_dma2d_handle, lcdltdc.activelayer);                             /* ������ */
//    HAL_DMA2D_Start(&g_dma2d_handle, color,(uint32_t)addr,pex - psx + 1,pey - psy + 1);      /* �������� */
//    HAL_DMA2D_PollForTransfer(&g_dma2d_handle, 1000);                                        /* �������� */
//
//    while((__HAL_DMA2D_GET_FLAG( &g_dma2d_handle,DMA2D_FLAG_TC) == 0) && (timeout < 0X5000)) /* �ȴ�DMA2D��� */
//    {
//        timeout++;
//    }
//    __HAL_DMA2D_CLEAR_FLAG(&g_dma2d_handle,DMA2D_FLAG_TC);                                   /* ���������ɱ�־ */
//}

/**
 * @brief       ��ָ�����������ָ����ɫ��,DMA2D���
 * @note        �˺�����֧��uint16_t,RGB565��ʽ����ɫ�������.
 *              (sx,sy),(ex,ey):�����ζԽ�����,�����СΪ:(ex - sx + 1) * (ey - sy + 1)
 *              ע��:sx,ex,���ܴ���lcddev.width - 1; sy,ey,���ܴ���lcddev.height - 1
 * @param       sx,sy       : ��ʼ����
 * @param       ex,ey       : ��������
 * @param       color       : ������ɫ�����׵�ַ
 * @retval      ��
 */
void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint32_t psx, psy, pex, pey;   /* ��LCD���Ϊ��׼������ϵ,����������仯���仯 */
    uint32_t timeout = 0; 
    uint16_t offline;
    uint32_t addr;
  
    /* ����ϵת�� */
    if (lcdltdc.dir)               /* ���� */
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else                          /* ���� */
    {
        psx = sy;
        psy = lcdltdc.pheight - ex - 1;
        pex = ey;
        pey = lcdltdc.pheight - sx - 1;
    }

    offline = lcdltdc.pwidth - (pex - psx + 1);
    addr = ((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] + lcdltdc.pixsize * (lcdltdc.pwidth * psy + psx));

    __HAL_RCC_DMA2D_CLK_ENABLE();                             /* ʹ��DM2Dʱ�� */

    DMA2D->CR &= ~(DMA2D_CR_START);                           /* ��ֹͣDMA2D */
    DMA2D->CR = DMA2D_M2M;                                    /* �洢�����洢��ģʽ */
    DMA2D->FGPFCCR = LTDC_PIXFORMAT;                          /* ������ɫ��ʽ */
    DMA2D->FGOR = 0;                                          /* ǰ������ƫ��Ϊ0 */
    DMA2D->OOR = offline;                                     /* ������ƫ�� */

    DMA2D->FGMAR = (uint32_t)color;                           /* Դ��ַ */
    DMA2D->OMAR = addr;                                       /* ����洢����ַ */
    DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16);   /* �趨�����Ĵ��� */
    DMA2D->CR |= DMA2D_CR_START;                              /* ����DMA2D */

    while((DMA2D->ISR & (DMA2D_FLAG_TC)) == 0)                /* �ȴ�������� */
    {
        timeout++;
        if (timeout > 0X1FFFFF)break;                         /* ��ʱ�˳� */
    } 
    DMA2D->IFCR |= DMA2D_FLAG_TC;                             /* ���������ɱ�־ */
}  

/**
 * @brief       LTCD����
 * @param       color          : ��ɫֵ
 * @retval      ��
 */
void ltdc_clear(uint32_t color)
{
    ltdc_fill(0, 0, lcdltdc.width - 1, lcdltdc.height - 1, color);
}

/**
 * @brief       LTDCʱ��(Fdclk)���ú���
 * @param       pll3n     : PLL3��Ƶϵ��(PLL��Ƶ),           ȡֵ��Χ:4~512.
 * @param       pll3m     : PLL3Ԥ��Ƶϵ��(��PLL֮ǰ�ķ�Ƶ),  ȡֵ��Χ:2~63.
 * @param       pll3r     : PLL3��r��Ƶϵ��(PLL֮��ķ�Ƶ),   ȡֵ��Χ:1~128.
 *
 * @note        Fvco = Fs * (pll3n / pll3m);
 *              Fr = Fvco / pll3r = Fs * (pll3n / (pll3m * pll3r));
 *              Fdclk = Fr;
 *              ����:
 *              Fvco: VCOƵ��
 *              Fr: PLL3��r��Ƶ���ʱ��Ƶ��
 *              Fs: PLL3����ʱ��Ƶ��,������HSI,CSI,HSE��.
 *
 *              LTDC,����һ������pll3m = 25, pll3n = 300, ����,���Եõ�Fvco = 300Mhz
 *              Ȼ��,ֻ ��Ҫͨ���޸�pll3r, ��ƥ�䲻ͬ��Һ����ʱ�򼴿�.

 *              ����:�ⲿ����Ϊ25M, pllm = 25 ��ʱ��, Fs = 25Mhz�� pllm��Ƶ��Ƶ�� Ϊ1Mhz.
 *              ����: Ҫ�õ�33M��LTDCʱ��, ���������: pll3n = 300, pllm = 25, pll3r = 9
 *              Fdclk= ((25 / 25) * 300) / 9 = 33 Mhz
 * @retval      0, �ɹ�;
 *              ����, ʧ��;
 */
uint8_t ltdc_clk_set(uint32_t pll3n, uint32_t pll3m, uint32_t pll3r)
{
    RCC_PeriphCLKInitTypeDef periphclk_initure;

    /* LTDC�������ʱ�ӣ���Ҫ�����Լ���ʹ�õ�LCD�����ֲ������ã� */
    periphclk_initure.PeriphClockSelection = RCC_PERIPHCLK_LTDC;     /* LTDCʱ�� */
    periphclk_initure.PLL3.PLL3M = pll3m;
    periphclk_initure.PLL3.PLL3N = pll3n;
    periphclk_initure.PLL3.PLL3P = 2;
    periphclk_initure.PLL3.PLL3Q = 2;
    periphclk_initure.PLL3.PLL3R = pll3r;

    if (HAL_RCCEx_PeriphCLKConfig(&periphclk_initure) == HAL_OK)     /* ��������ʱ�� */
    {
        return 0;                                                    /* �ɹ� */
    }
    else 
    {
        return 1;                                                    /* ʧ�� */
    }
}

/**
 * @brief       LTDC�㴰������, ������LCD�������ϵΪ��׼
 * @note        �˺���������ltdc_layer_parameter_config֮��������.����,�����õĴ���ֵ���������ĳ�
 *              ��ʱ,GRAM�Ĳ���(��/д�㺯��),ҲҪ���ݴ��ڵĿ����������޸�,������ʾ������(�����̾�δ���޸�).
 * @param       layerx      : 0,��һ��; 1,�ڶ���;
 * @param       sx, sy      : ��ʼ����
 * @param       width,height: ���Ⱥ͸߶�
 * @retval      ��
 */
void ltdc_layer_window_config(uint8_t layerx, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    HAL_LTDC_SetWindowPosition(&g_ltdc_handle, sx, sy, layerx);     /* ���ô��ڵ�λ�� */
    HAL_LTDC_SetWindowSize(&g_ltdc_handle, width, height, layerx);  /* ���ô��ڴ�С */
}

/**
 * @brief       LTDC�������������
 * @note        �˺���,������ltdc_layer_window_config֮ǰ����.
 * @param       layerx      : 0,��һ��; 1,�ڶ���;
 * @param       bufaddr     : ����ɫ֡������ʼ��ַ
 * @param       pixformat   : ��ɫ��ʽ. 0,ARGB8888; 1,RGB888; 2,RGB565; 3,ARGB1555; 4,ARGB4444; 5,L8; 6;AL44; 7;AL88
 * @param       alpha       : ����ɫAlphaֵ, 0,ȫ͸��;255,��͸��
 * @param       alpha0      : Ĭ����ɫAlphaֵ, 0,ȫ͸��;255,��͸��
 * @param       bfac1       : ���ϵ��1, 4(100),�㶨��Alpha; 6(101),����Alpha*�㶨Alpha
 * @param       bfac2       : ���ϵ��2, 5(101),�㶨��Alpha; 7(111),����Alpha*�㶨Alpha
 * @param       bkcolor     : ��Ĭ����ɫ,32λ,��24λ��Ч,RGB888��ʽ
 * @retval      ��
 */
void ltdc_layer_parameter_config(uint8_t layerx, uint32_t bufaddr, uint8_t pixformat, uint8_t alpha, uint8_t alpha0, uint8_t bfac1, uint8_t bfac2, uint32_t bkcolor)
{
    LTDC_LayerCfgTypeDef playercfg;

    playercfg.WindowX0 = 0;                                            /* ������ʼX���� */
    playercfg.WindowY0 = 0;                                            /* ������ʼY���� */
    playercfg.WindowX1 = lcdltdc.pwidth;                               /* ������ֹX���� */
    playercfg.WindowY1 = lcdltdc.pheight;                              /* ������ֹY���� */
    playercfg.PixelFormat = pixformat;                                 /* ���ظ�ʽ */
    playercfg.Alpha = alpha;                                           /* Alphaֵ���ã�0~255,255Ϊ��ȫ��͸�� */
    playercfg.Alpha0 = alpha0;                                         /* Ĭ��Alphaֵ */
    playercfg.BlendingFactor1 = (uint32_t)bfac1 << 8;                  /* ���ò���ϵ�� */
    playercfg.BlendingFactor2 = (uint32_t)bfac2 << 8;                  /* ���ò���ϵ�� */
    playercfg.FBStartAdress = bufaddr;                                 /* ���ò���ɫ֡������ʼ��ַ */
    playercfg.ImageWidth = lcdltdc.pwidth;                             /* ������ɫ֡�������Ŀ��� */
    playercfg.ImageHeight = lcdltdc.pheight;                           /* ������ɫ֡�������ĸ߶� */
    playercfg.Backcolor.Red = (uint8_t)(bkcolor & 0X00FF0000) >> 16;   /* ������ɫ��ɫ���� */
    playercfg.Backcolor.Green = (uint8_t)(bkcolor & 0X0000FF00) >> 8;  /* ������ɫ��ɫ���� */
    playercfg.Backcolor.Blue = (uint8_t)bkcolor & 0X000000FF;          /* ������ɫ��ɫ���� */
    HAL_LTDC_ConfigLayer(&g_ltdc_handle, &playercfg, layerx);          /* ������ѡ�еĲ� */
}  

/**
 * @brief       LTDC��ȡ���ID
 * @note        ����LCD RGB�ߵ����λ(R7,G7,B7)��ʶ�����ID
 *              PG6 = R7(M0); PI2 = G7(M1); PI7 = B7(M2);
 *              M2:M1:M0
 *              0 :0 :0     4.3 ��480*272  RGB��,ID = 0X4342
 *              0 :0 :1     7   ��800*480  RGB��,ID = 0X7084
 *              0 :1 :0     7   ��1024*600 RGB��,ID = 0X7016
 *              0 :1 :1     7   ��1280*800 RGB��,ID = 0X7018
 *              1 :0 :0     4.3 ��800*480  RGB��,ID = 0X4384
 *              1 :0 :1     10.1��1280*800 RGB��,ID = 0X1018
 * @param       ��
 * @retval      0, �Ƿ�; 
 *              ����, LCD ID
 */
uint16_t ltdc_panelid_read(void)
{
    uint8_t idx = 0;

    GPIO_InitTypeDef gpio_init_struct;

    __HAL_RCC_GPIOG_CLK_ENABLE();                               /* ʹ��GPIOGʱ�� */
    __HAL_RCC_GPIOI_CLK_ENABLE();                               /* ʹ��GPIOIʱ�� */
    
    gpio_init_struct.Pin = GPIO_PIN_6;                          /* PG6 */
    gpio_init_struct.Mode = GPIO_MODE_INPUT;                    /* ���� */
    gpio_init_struct.Pull = GPIO_PULLUP;                        /* ���� */
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;                   /* ���� */
    HAL_GPIO_Init(GPIOG, &gpio_init_struct);                    /* ��ʼ�� */
    
    gpio_init_struct.Pin = GPIO_PIN_2 | GPIO_PIN_7;             /* PI2,7 */
    HAL_GPIO_Init(GPIOI, &gpio_init_struct);                    /* ��ʼ�� */

    delay_us(2);
    idx = (uint8_t)HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_6);         /* ��ȡM0 */
    idx|= (uint8_t)HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_2) << 1;    /* ��ȡM1 */
    idx|= (uint8_t)HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_7) << 2;    /* ��ȡM2 */

    switch (idx)
    {
        case 0 :
            return 0X4342;                    /* 4.3����,480*272�ֱ��� */
        
        case 1 :
            return 0X7084;                    /* 7  ����,800*480�ֱ��� */
        
        case 2 :
            return 0X7016;                    /* 7  ����,1024*600�ֱ��� */
        
        case 3 :
            return 0X7018;                    /* 7  ����,1280*800�ֱ��� */
        
        case 4 :
            return 0X4384;                    /* 4.3����,800*480�ֱ��� */
        
        case 5 :
            return 0X1018;                    /* 10.1����,1280*800�ֱ��� */
        
        default :
            return 0;
    }
}

/**
 * @brief       LTDC��ʼ������
 * @param       ��
 * @retval      ��
 */
void ltdc_init(void)
{
    uint16_t lcdid = 0;

    lcdid = ltdc_panelid_read();                /* ��ȡLCD���ID */
    if (lcdid == 0X4342)
    {
        lcdltdc.pwidth = 480;                   /* ������,��λ:���� */
        lcdltdc.pheight = 272;                  /* ���߶�,��λ:���� */
        lcdltdc.hsw = 1;                        /* ˮƽͬ������ */
        lcdltdc.vsw = 1;                        /* ��ֱͬ������ */
        lcdltdc.hbp = 40;                       /* ˮƽ���� */
        lcdltdc.vbp = 8;                        /* ��ֱ���� */
        lcdltdc.hfp = 5;                        /* ˮƽǰ�� */
        lcdltdc.vfp = 8;                        /* ��ֱǰ�� */
        ltdc_clk_set(300, 25, 33);              /* ��������ʱ�� 9Mhz */
        /* ������������. */
    }
    else if (lcdid == 0X7084)
    {
        lcdltdc.pwidth = 800;                   /* ������,��λ:���� */
        lcdltdc.pheight = 480;                  /* ���߶�,��λ:���� */
        lcdltdc.hsw = 1;                        /* ˮƽͬ������ */
        lcdltdc.vsw = 1;                        /* ��ֱͬ������ */
        lcdltdc.hbp = 46;                       /* ˮƽ���� */
        lcdltdc.vbp = 23;                       /* ��ֱ���� */
        lcdltdc.hfp = 210;                      /* ˮƽǰ�� */
        lcdltdc.vfp = 22;                       /* ��ֱǰ�� */
        ltdc_clk_set(300, 25, 9);               /* ��������ʱ�� 33M(�����˫��,��Ҫ����DCLK��:18.75Mhz  300/4/4,�Ż�ȽϺ�) */
    }
    else if (lcdid == 0X7016)
    {
        lcdltdc.pwidth = 1024;                  /* ������,��λ:���� */
        lcdltdc.pheight = 600;                  /* ���߶�,��λ:���� */
        lcdltdc.hsw = 20;                       /* ˮƽͬ������ */
        lcdltdc.vsw = 3;                        /* ��ֱͬ������ */
        lcdltdc.hbp = 140;                      /* ˮƽ���� */
        lcdltdc.vbp = 20;                       /* ��ֱ���� */
        lcdltdc.hfp = 160;                      /* ˮƽǰ�� */
        lcdltdc.vfp = 12;                       /* ��ֱǰ�� */
        ltdc_clk_set(300, 25, 6);               /* ��������ʱ��  40Mhz 6 */
        /* ������������.*/
    }
    else if (lcdid == 0X7018)
    {
        lcdltdc.pwidth = 1280;                  /* ������,��λ:���� */
        lcdltdc.pheight = 800;                  /* ���߶�,��λ:���� */
        /* ������������.*/
    }
    else if (lcdid == 0X4384)
    {
        lcdltdc.pwidth = 800;                   /* ������,��λ:���� */
        lcdltdc.pheight = 480;                  /* ���߶�,��λ:���� */
        lcdltdc.hbp = 88;                       /* ˮƽ���� */
        lcdltdc.hfp = 40;                       /* ˮƽǰ�� */
        lcdltdc.hsw = 48;                       /* ˮƽͬ������ */
        lcdltdc.vbp = 32;                       /* ��ֱ���� */
        lcdltdc.vfp = 13;                       /* ��ֱǰ�� */
        lcdltdc.vsw = 3;                        /* ��ֱͬ������ */
        ltdc_clk_set(300, 25, 9);               /* ��������ʱ�� 33M */
        /* ������������. */
    }
    else if (lcdid == 0X1018)                   /* 10.1��1280*800 RGB�� */
    {
        lcdltdc.pwidth = 1280;                  /* ������,��λ:���� */
        lcdltdc.pheight = 800;                  /* ���߶�,��λ:���� */
        lcdltdc.hbp = 140;                      /* ˮƽ���� */
        lcdltdc.hfp = 10;                       /* ˮƽǰ�� */
        lcdltdc.hsw = 10;                       /* ˮƽͬ������ */
        lcdltdc.vbp = 10;                       /* ��ֱ���� */
        lcdltdc.vfp = 10;                       /* ��ֱǰ�� */
        lcdltdc.vsw = 3;                        /* ��ֱͬ������ */
        ltdc_clk_set(300, 25, 6);               /* ��������ʱ��  45Mhz */
    } 

    lcddev.width = lcdltdc.pwidth;
    lcddev.height = lcdltdc.pheight;
    
#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888 || LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888 
    g_ltdc_framebuf[0] = (uint32_t*) &ltdc_lcd_framebuf;
    lcdltdc.pixsize = 4;                        /* ÿ������ռ4���ֽ� */
#else 
    lcdltdc.pixsize = 2;                        /* ÿ������ռ2���ֽ� */
    g_ltdc_framebuf[0] = (uint32_t*)&ltdc_lcd_framebuf;
//    g_ltdc_framebuf[1] = (uint32_t*)&ltdc_lcd_framebuf1;
#endif 
    /* LTDC���� */
    g_ltdc_handle.Instance = LTDC;
    g_ltdc_handle.Init.HSPolarity = LTDC_HSPOLARITY_AL;         /* ˮƽͬ������ */
    g_ltdc_handle.Init.VSPolarity = LTDC_VSPOLARITY_AL;         /* ��ֱͬ������ */
    g_ltdc_handle.Init.DEPolarity = LTDC_DEPOLARITY_AL;         /* ����ʹ�ܼ��� */
    g_ltdc_handle.State = HAL_LTDC_STATE_RESET;
    
    if (lcdid == 0X1018)
    {
        g_ltdc_handle.Init.PCPolarity = LTDC_PCPOLARITY_IIPC;   /* ����ʱ�Ӽ��� */
    }
    else 
    {
        g_ltdc_handle.Init.PCPolarity = LTDC_PCPOLARITY_IPC;    /* ����ʱ�Ӽ��� */
    }

    g_ltdc_handle.Init.HorizontalSync = lcdltdc.hsw - 1;                                            /* ˮƽͬ������ */
    g_ltdc_handle.Init.VerticalSync = lcdltdc.vsw - 1;                                              /* ��ֱͬ������ */
    g_ltdc_handle.Init.AccumulatedHBP = lcdltdc.hsw + lcdltdc.hbp - 1;                              /* ˮƽͬ�����ؿ��� */
    g_ltdc_handle.Init.AccumulatedVBP = lcdltdc.vsw + lcdltdc.vbp - 1;                              /* ��ֱͬ�����ظ߶� */
    g_ltdc_handle.Init.AccumulatedActiveW = lcdltdc.hsw + lcdltdc.hbp + lcdltdc.pwidth - 1;         /* ��Ч���� */
    g_ltdc_handle.Init.AccumulatedActiveH = lcdltdc.vsw + lcdltdc.vbp + lcdltdc.pheight - 1;        /* ��Ч�߶� */
    g_ltdc_handle.Init.TotalWidth = lcdltdc.hsw + lcdltdc.hbp + lcdltdc.pwidth + lcdltdc.hfp - 1;   /* �ܿ��� */
    g_ltdc_handle.Init.TotalHeigh = lcdltdc.vsw + lcdltdc.vbp + lcdltdc.pheight + lcdltdc.vfp - 1;  /* �ܸ߶� */
    g_ltdc_handle.Init.Backcolor.Red = 0;                                                           /* ��Ļ�������ɫ���� */
    g_ltdc_handle.Init.Backcolor.Green = 0;                                                         /* ��Ļ��������ɫ���� */
    g_ltdc_handle.Init.Backcolor.Blue = 0;                                                          /* ��Ļ����ɫ��ɫ���� */
    HAL_LTDC_Init(&g_ltdc_handle);

    /* ������ */
    ltdc_layer_parameter_config(0, (uint32_t)g_ltdc_framebuf[0], LTDC_PIXFORMAT, 255, 0, 6, 7, 0X000000);   /* ��������� */
    ltdc_layer_window_config(0, 0, 0, lcdltdc.pwidth, lcdltdc.pheight);                                     /* �㴰������,��LCD�������ϵΪ��׼,��Ҫ����޸�! */

    //ltdc_layer_parameter_config(1,(uint32_t)g_ltdc_framebuf[1],LTDC_PIXFORMAT,127,0,6,7,0X000000);        /* ��������� */
    //ltdc_layer_window_config(1,0,0,lcdltdc.pwidth,lcdltdc.pheight);                                       /* �㴰������,��LCD�������ϵΪ��׼,��Ҫ����޸�! */

//    ltdc_display_dir(0);                 /* Ĭ������ */
    ltdc_select_layer(0);                  /* ѡ���1�� */
    LTDC_BL(1);                            /* �������� */
    ltdc_clear(0XFFFFFFFF);                /* ���� */
}

/**
 * @brief       LTDC�ײ�IO��ʼ����ʱ��ʹ��
 * @note        �˺����ᱻHAL_LTDC_Init()����
 * @param       hltdc:LTDC���
 * @retval      ��
 */
void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
    GPIO_InitTypeDef gpio_init_struct;

    __HAL_RCC_LTDC_CLK_ENABLE();                      /* ʹ��LTDCʱ�� */
    __HAL_RCC_DMA2D_CLK_ENABLE();                     /* ʹ��DMA2Dʱ�� */

    /* ������LTDC�źſ������� BL/DE/VSYNC/HSYNC/CLK�ȵ����� */
    LTDC_BL_GPIO_CLK_ENABLE();                        /* LTDC_BL��ʱ��ʹ�� */
    LTDC_DE_GPIO_CLK_ENABLE();                        /* LTDC_DE��ʱ��ʹ�� */
    LTDC_VSYNC_GPIO_CLK_ENABLE();                     /* LTDC_VSYNC��ʱ��ʹ�� */
    LTDC_HSYNC_GPIO_CLK_ENABLE();                     /* LTDC_HSYNC��ʱ��ʹ�� */
    LTDC_CLK_GPIO_CLK_ENABLE();                       /* LTDC_CLK��ʱ��ʹ�� */
    __HAL_RCC_GPIOH_CLK_ENABLE();                     /* GPIOHʱ��ʹ�� */
    
    /* ��ʼ��PB5���������� */
    gpio_init_struct.Pin = LTDC_BL_GPIO_PIN;          /* LTDC_BL����ģʽ���� */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;      /* ������� */
    gpio_init_struct.Pull = GPIO_PULLUP;              /* ���� */
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;         /* ���� */
    HAL_GPIO_Init(LTDC_BL_GPIO_PORT, &gpio_init_struct);
    
    gpio_init_struct.Pin = LTDC_DE_GPIO_PIN;          /* LTDC_DE����ģʽ���� */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;          /* ���� */
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;
    gpio_init_struct.Alternate = GPIO_AF14_LTDC;      /* ����ΪLTDC */
    HAL_GPIO_Init(LTDC_DE_GPIO_PORT, &gpio_init_struct);
    
    gpio_init_struct.Pin = LTDC_VSYNC_GPIO_PIN;       /* LTDC_VSYNC����ģʽ���� */
    HAL_GPIO_Init(LTDC_VSYNC_GPIO_PORT, &gpio_init_struct);
    
    gpio_init_struct.Pin = LTDC_HSYNC_GPIO_PIN;       /* LTDC_HSYNC����ģʽ���� */
    HAL_GPIO_Init(LTDC_HSYNC_GPIO_PORT, &gpio_init_struct);
    
    gpio_init_struct.Pin = LTDC_CLK_GPIO_PIN;         /* LTDC_CLK����ģʽ���� */
    HAL_GPIO_Init(LTDC_CLK_GPIO_PORT, &gpio_init_struct);

    /* ��ʼ��PG6,11 */
    gpio_init_struct.Pin = GPIO_PIN_6 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOG, &gpio_init_struct);
    
    /* ��ʼ��PH9,10,11,12,13,14,15 */
    gpio_init_struct.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
                     GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOH, &gpio_init_struct);
    
    /* ��ʼ��PI0,1,2,4,5,6,7 */
    gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5| \
                     GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOI, &gpio_init_struct); 
}

