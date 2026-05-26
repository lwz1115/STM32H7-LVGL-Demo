/**
 ******************************************************************************
 * @file    diskio.c
 * @brief   FatFS底层磁盘I/O接口 - STM32H7 SDIO版本
 ******************************************************************************
 */

#include "ff.h"
#include "diskio.h"
#include "./BSP/SDCARD/sdcard.h"
#include <string.h>

/* 物理驱动器编号 */
#define DEV_SD      0   /* SD卡 */

/**
 * @brief  获取磁盘状态
 * @param  pdrv: 物理驱动器编号
 * @retval 磁盘状态
 */
DSTATUS disk_status(BYTE pdrv)
{
    DSTATUS stat = STA_NOINIT;
    
    switch (pdrv)
    {
        case DEV_SD:
            if (sd_get_card_state() == SD_OK)
            {
                stat = 0;  /* 正常 */
            }
            break;
            
        default:
            stat = STA_NOINIT;
            break;
    }
    
    return stat;
}

/**
 * @brief  初始化磁盘
 * @param  pdrv: 物理驱动器编号
 * @retval 磁盘状态
 */
DSTATUS disk_initialize(BYTE pdrv)
{
    DSTATUS stat = STA_NOINIT;
    
    switch (pdrv)
    {
        case DEV_SD:
            if (sd_init() == SD_OK)
            {
                stat = 0;  /* 初始化成功 */
            }
            break;
            
        default:
            stat = STA_NOINIT;
            break;
    }
    
    return stat;
}

/**
 * @brief  读取扇区
 * @param  pdrv: 物理驱动器编号
 * @param  buff: 数据缓冲区
 * @param  sector: 起始扇区
 * @param  count: 扇区数量
 * @retval 结果
 */
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    DRESULT res = RES_PARERR;
    
    switch (pdrv)
    {
        case DEV_SD:
            if (sd_read_disk(buff, sector, count) == SD_OK)
            {
                res = RES_OK;
            }
            else
            {
                res = RES_ERROR;
            }
            break;
            
        default:
            res = RES_PARERR;
            break;
    }
    
    return res;
}

/**
 * @brief  写入扇区
 * @param  pdrv: 物理驱动器编号
 * @param  buff: 数据缓冲区
 * @param  sector: 起始扇区
 * @param  count: 扇区数量
 * @retval 结果
 */
#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    DRESULT res = RES_PARERR;
    
    switch (pdrv)
    {
        case DEV_SD:
            if (sd_write_disk((uint8_t *)buff, sector, count) == SD_OK)
            {
                res = RES_OK;
            }
            else
            {
                res = RES_ERROR;
            }
            break;
            
        default:
            res = RES_PARERR;
            break;
    }
    
    return res;
}
#endif

/**
 * @brief  磁盘I/O控制
 * @param  pdrv: 物理驱动器编号
 * @param  cmd: 控制命令
 * @param  buff: 数据缓冲区
 * @retval 结果
 */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    DRESULT res = RES_PARERR;
    SD_CardInfo cardinfo;
    
    switch (pdrv)
    {
        case DEV_SD:
            switch (cmd)
            {
                case CTRL_SYNC:
                    res = RES_OK;
                    break;
                    
                case GET_SECTOR_COUNT:
                    if (sd_get_card_info(&cardinfo) == SD_OK)
                    {
                        *(DWORD *)buff = cardinfo.LogBlockNbr;
                        res = RES_OK;
                    }
                    else
                    {
                        res = RES_ERROR;
                    }
                    break;
                    
                case GET_SECTOR_SIZE:
                    *(WORD *)buff = 512;
                    res = RES_OK;
                    break;
                    
                case GET_BLOCK_SIZE:
                    if (sd_get_card_info(&cardinfo) == SD_OK)
                    {
                        *(DWORD *)buff = cardinfo.LogBlockSize;
                        res = RES_OK;
                    }
                    else
                    {
                        res = RES_ERROR;
                    }
                    break;
                    
                default:
                    res = RES_PARERR;
                    break;
            }
            break;
            
        default:
            res = RES_PARERR;
            break;
    }
    
    return res;
}

/**
 * @brief  获取当前时间（用于文件时间戳）
 * @param  无
 * @retval 当前时间（FAT格式）
 */
DWORD get_fattime(void)
{
    /* 返回当前时间
     * bit31:25 - 年份(0-127, 从1980年开始)
     * bit24:21 - 月份(1-12)
     * bit20:16 - 日期(1-31)
     * bit15:11 - 小时(0-23)
     * bit10:5  - 分钟(0-59)
     * bit4:0   - 秒/2(0-29)
     */
    
    /* 这里返回固定时间2024-01-01 00:00:00
     * 实际应用中应该从RTC获取真实时间 */
    return ((DWORD)(2024 - 1980) << 25)  /* 年份: 2024 */
         | ((DWORD)1 << 21)              /* 月份: 1 */
         | ((DWORD)1 << 16)              /* 日期: 1 */
         | ((DWORD)0 << 11)              /* 小时: 0 */
         | ((DWORD)0 << 5)               /* 分钟: 0 */
         | ((DWORD)0 >> 1);              /* 秒: 0 */
}
