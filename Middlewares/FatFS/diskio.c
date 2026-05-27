/**
 ******************************************************************************
 * @file    diskio.c
 * @brief   FatFS底层磁盘I/O接口 - STM32H7 SDIO版本
 ******************************************************************************
 */

#include "ff.h"
#include "diskio.h"
#include "./BSP/SDCARD/sdcard.h"
#include "./BSP/NTP/ntp_sync.h"
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
            /* STM32H7 D-Cache 要求 DMA 缓冲区 4 字节对齐
             * FatFs 有时传入非对齐地址，需要用临时缓冲区中转 */
            if ((uint32_t)buff & 0x03)
            {
                static uint8_t scratch[512] __attribute__((aligned(4)));
                while (count--)
                {
                    if (sd_read_disk(scratch, sector++, 1) != SD_OK)
                    {
                        res = RES_ERROR;
                        break;
                    }
                    memcpy(buff, scratch, 512);
                    buff += 512;
                    res = RES_OK;
                }
            }
            else if (sd_read_disk(buff, sector, count) == SD_OK)
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
                    /* FatFs 要求返回以扇区为单位的擦除块大小，SD卡固定为 1 */
                    *(DWORD *)buff = 1;
                    res = RES_OK;
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
 * @brief  获取当前时间（用于文件时间戳，从RTC读取）
 */
DWORD get_fattime(void)
{
    ntp_time_t t;
    if (ntp_sync_get_time(&t)) {
        return ((DWORD)(t.year - 1980) << 25)
             | ((DWORD)t.month        << 21)
             | ((DWORD)t.date         << 16)
             | ((DWORD)t.hour         << 11)
             | ((DWORD)t.minute       << 5)
             | ((DWORD)t.second       >> 1);
    }
    /* RTC未同步时返回固定时间 */
    return ((DWORD)(2026 - 1980) << 25) | ((DWORD)1 << 21) | ((DWORD)1 << 16);
}
