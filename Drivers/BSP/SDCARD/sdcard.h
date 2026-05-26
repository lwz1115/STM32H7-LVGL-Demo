/**
 ******************************************************************************
 * @file    sdcard.h
 * @author  正点原子团队
 * @version V1.0
 * @date    2024-01-01
 * @brief   SD卡驱动头文件
 ******************************************************************************
 */

#ifndef __SDCARD_H
#define __SDCARD_H

#include "./SYSTEM/sys/sys.h"

/* SD卡引脚定义 
 * SDIO接口引脚:
 * SDIOD0  - PC8
 * SDIOD1  - PC9
 * SDIOD2  - PC10
 * SDIOD3  - PC11
 * SDIOSCK - PC12
 * SDIOCMD - PD2
 */

/* SD卡类型定义 */
#define SD_CARD_SDSC                    0   /* SD Standard Capacity (SDSC) */
#define SD_CARD_SDHC_SDXC               1   /* SD High Capacity (SDHC) / SD eXtended Capacity (SDXC) */
#define SD_CARD_SECURED                 3   /* SD Secured */

/* SD卡错误代码 */
#define SD_OK                           0   /* 成功 */
#define SD_ERROR                        1   /* 错误 */
#define SD_TIMEOUT                      2   /* 超时 */
#define SD_INVALID_PARAMETER            3   /* 参数错误 */

/* SD卡信息结构体 */
typedef struct
{
    uint32_t CardType;                      /* SD卡类型 */
    uint32_t CardVersion;                   /* SD卡版本 */
    uint32_t Class;                         /* SD卡类别 */
    uint32_t RelCardAdd;                    /* SD卡相对地址 */
    uint32_t BlockNbr;                      /* SD卡块数量 */
    uint32_t BlockSize;                     /* SD卡块大小 */
    uint32_t LogBlockNbr;                   /* SD卡逻辑块数量 */
    uint32_t LogBlockSize;                  /* SD卡逻辑块大小 */
    uint32_t CardCapacity;                  /* SD卡容量(MB) */
} SD_CardInfo;

/* 全局变量声明 */
extern SD_HandleTypeDef hsd1;               /* SD卡句柄 */
extern HAL_SD_CardInfoTypeDef SDCardInfo;   /* SD卡信息 */

/* 函数声明 */
uint8_t sd_init(void);                                          /* 初始化SD卡 */
uint8_t sd_get_card_info(SD_CardInfo *cardinfo);                /* 获取SD卡信息 */
uint8_t sd_read_disk(uint8_t *buf, uint32_t sector, uint32_t cnt);     /* 读SD卡 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t sector, uint32_t cnt);    /* 写SD卡 */
uint8_t sd_get_card_state(void);                                /* 获取SD卡状态 */

#endif /* __SDCARD_H */
