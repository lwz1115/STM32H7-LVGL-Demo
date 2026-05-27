/**
 ******************************************************************************
 * @file    sdcard.c
 * @author  正点原子团队
 * @version V1.0
 * @date    2024-01-01
 * @brief   SD卡驱动实现文件
 ******************************************************************************
 */

#include "./BSP/SDCARD/sdcard.h"
#include "./SYSTEM/delay/delay.h"

/* 全局变量定义 */
SD_HandleTypeDef hsd1;                      /* SD卡句柄 */
HAL_SD_CardInfoTypeDef SDCardInfo;          /* SD卡信息 */

/**
 * @brief  初始化SD卡
 * @param  无
 * @retval 0: 成功, 其他: 失败
 */
uint8_t sd_init(void)
{
    uint8_t res = SD_OK;
    
    /* SDIO配置 */
    hsd1.Instance = SDMMC1;
    hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;              /* 上升沿 */
    hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;  /* 不使能省电模式 */
    hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;                      /* 4位数据线 */
    hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE; /* 不使能硬件流控 */
    hsd1.Init.ClockDiv = 8;                                     /* 时钟分频系数 - 降低到8以兼容老卡 */
    
    /* 初始化SDIO */
    if (HAL_SD_Init(&hsd1) != HAL_OK)
    {
        res = SD_ERROR;
    }
    
    return res;
}

/**
 * @brief  SDIO底层初始化（由HAL_SD_Init调用）
 * @param  hsd: SD卡句柄
 * @retval 无
 */
void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (hsd->Instance == SDMMC1)
    {
        /* 使能时钟 */
        __HAL_RCC_SDMMC1_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        
        /**
         * SDMMC1 GPIO配置
         * PC8  ------> SDMMC1_D0
         * PC9  ------> SDMMC1_D1
         * PC10 ------> SDMMC1_D2
         * PC11 ------> SDMMC1_D3
         * PC12 ------> SDMMC1_CK
         * PD2  ------> SDMMC1_CMD
         */
        
        /* 配置PC8-PC12 */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        
        /* 配置PD2 */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
        
        /* 配置SDMMC1中断 */
        HAL_NVIC_SetPriority(SDMMC1_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
    }
}

/**
 * @brief  SDIO底层反初始化（由HAL_SD_DeInit调用）
 * @param  hsd: SD卡句柄
 * @retval 无
 */
void HAL_SD_MspDeInit(SD_HandleTypeDef *hsd)
{
    if (hsd->Instance == SDMMC1)
    {
        /* 禁用时钟 */
        __HAL_RCC_SDMMC1_CLK_DISABLE();
        
        /* 反初始化GPIO */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
        
        /* 禁用中断 */
        HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
    }
}

/**
 * @brief  获取SD卡信息
 * @param  cardinfo: SD卡信息结构体指针
 * @retval 0: 成功, 其他: 失败
 */
uint8_t sd_get_card_info(SD_CardInfo *cardinfo)
{
    uint8_t res = SD_OK;
    
    /* 获取SD卡信息 */
    if (HAL_SD_GetCardInfo(&hsd1, &SDCardInfo) != HAL_OK)
    {
        res = SD_ERROR;
    }
    else
    {
        cardinfo->CardType = SDCardInfo.CardType;               /* 卡类型 */
        cardinfo->CardVersion = SDCardInfo.CardVersion;         /* 卡版本 */
        cardinfo->Class = SDCardInfo.Class;                     /* 卡类别 */
        cardinfo->RelCardAdd = SDCardInfo.RelCardAdd;           /* 卡相对地址 */
        cardinfo->BlockNbr = SDCardInfo.BlockNbr;               /* 块数量 */
        cardinfo->BlockSize = SDCardInfo.BlockSize;             /* 块大小 */
        cardinfo->LogBlockNbr = SDCardInfo.LogBlockNbr;         /* 逻辑块数量 */
        cardinfo->LogBlockSize = SDCardInfo.LogBlockSize;       /* 逻辑块大小 */
        cardinfo->CardCapacity = (uint32_t)((uint64_t)SDCardInfo.BlockNbr * SDCardInfo.BlockSize / 1024 / 1024); /* 容量(MB) */
    }
    
    return res;
}

/**
 * @brief  读取SD卡
 * @param  buf: 数据缓冲区
 * @param  sector: 起始扇区
 * @param  cnt: 扇区数量
 * @retval 0: 成功, 其他: 失败
 */
uint8_t sd_read_disk(uint8_t *buf, uint32_t sector, uint32_t cnt)
{
    uint8_t res = SD_OK;
    uint32_t timeout = 0;

    if (HAL_SD_ReadBlocks(&hsd1, buf, sector, cnt, 1000) != HAL_OK)
    {
        res = SD_ERROR;
    }
    else
    {
        timeout = 1000;
        while ((HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) && timeout)
        {
            timeout--;
            delay_ms(1);
        }

        if (timeout == 0)
        {
            res = SD_TIMEOUT;
        }
        else
        {
            /* H743 开启了 D-Cache，DMA 传输完成后必须刷新缓存
             * 否则 CPU 读到的是 Cache 中的旧数据而不是 DMA 写入的新数据 */
            SCB_InvalidateDCache_by_Addr((uint32_t *)buf, cnt * 512);
        }
    }

    return res;
}

/**
 * @brief  写入SD卡
 * @param  buf: 数据缓冲区
 * @param  sector: 起始扇区
 * @param  cnt: 扇区数量
 * @retval 0: 成功, 其他: 失败
 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t sector, uint32_t cnt)
{
    uint8_t res = SD_OK;
    uint32_t timeout = 0;
    
    /* 写入数据 */
    if (HAL_SD_WriteBlocks(&hsd1, buf, sector, cnt, 1000) != HAL_OK)
    {
        res = SD_ERROR;
    }
    else
    {
        /* 等待传输完成 */
        timeout = 1000;
        while ((HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) && timeout)
        {
            timeout--;
            delay_ms(1);
        }
        
        if (timeout == 0)
        {
            res = SD_TIMEOUT;
        }
    }
    
    return res;
}

/**
 * @brief  获取SD卡状态
 * @param  无
 * @retval 0: 正常, 1: 错误
 */
uint8_t sd_get_card_state(void)
{
    return ((HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER) ? SD_OK : SD_ERROR);
}

/**
 * @brief  SDMMC1中断服务函数
 * @param  无
 * @retval 无
 */
void SDMMC1_IRQHandler(void)
{
    HAL_SD_IRQHandler(&hsd1);
}
