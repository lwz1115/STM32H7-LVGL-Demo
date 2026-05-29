/**
 ****************************************************************************************************

 ****************************************************************************************************
 */
 
#include "./BSP/MPU/mpu.h"
#include "./BSP/LED/led.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
 
 
 /**
 * @brief       ����ĳ�������MPU����
 * @param       baseaddr:MPU��������Ļ�ַ(�׵�ַ)
 *              size:MPU��������Ĵ�С(������32�ı���,��λΪ�ֽ�),�����õ�ֵ�ο�:CORTEX_MPU_Region_Size
 *              rnum:MPU���������,��Χ:0~7,���֧��8����������,�����õ�ֵ�ο���CORTEX_MPU_Region_Number
 *              ap:����Ȩ��,���ʹ�ϵ����:�����õ�ֵ�ο���CORTEX_MPU_Region_Permission_Attributes
 *              MPU_REGION_NO_ACCESS,�޷��ʣ���Ȩ&�û������ɷ��ʣ�
 *              MPU_REGION_PRIV_RW,��֧����Ȩ��д����
 *              MPU_REGION_PRIV_RW_URO,��ֹ�û�д���ʣ���Ȩ�ɶ�д���ʣ�
 *              MPU_REGION_FULL_ACCESS,ȫ���ʣ���Ȩ&�û����ɷ��ʣ�
 *              MPU_REGION_PRIV_RO,��֧����Ȩ������
 *              MPU_REGION_PRIV_RO_URO,ֻ������Ȩ&�û���������д��
 *              ���:STM32F7����ֲ�.pdf,4.6��,Table 89.
 *              sen:�Ƿ���������;MPU_ACCESS_NOT_SHAREABLE,������;MPU_ACCESS_SHAREABLE,����
 *              cen:�Ƿ�����cache;MPU_ACCESS_NOT_CACHEABLE,������;MPU_ACCESS_CACHEABLE,����
 *              ben:�Ƿ���������;MPU_ACCESS_NOT_BUFFERABLE,������;MPU_ACCESS_BUFFERABLE,����
 * @retval      0,�ɹ�.
 *              ����,����.
 */
uint8_t mpu_set_protection(uint32_t baseaddr, uint32_t size, uint32_t rnum, uint8_t ap, uint8_t sen, uint8_t cen, uint8_t ben)
{
    MPU_Region_InitTypeDef mpu_initure;

    HAL_MPU_Disable();                                        /* ����MPU֮ǰ�ȹر�MPU,��������Ժ���ʹ��MPU */

    mpu_initure.Enable = MPU_REGION_ENABLE;                   /* ʹ�ܸñ������� */
    mpu_initure.Number = rnum;                                /* ���ñ������� */
    mpu_initure.BaseAddress = baseaddr;                       /* ���û�ַ */
    mpu_initure.Size = size;                                  /* ���ñ��������С */
    mpu_initure.SubRegionDisable = 0X00;                      /* ��ֹ������ */
    mpu_initure.TypeExtField = MPU_TEX_LEVEL0;                /* ����������չ��Ϊlevel0 */
    mpu_initure.AccessPermission = (uint8_t)ap;               /* ���÷���Ȩ��, */
    mpu_initure.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;  /* ����ָ�����(������ȡָ��) */
    mpu_initure.IsShareable = sen;                            /* �Ƿ���? */
    mpu_initure.IsCacheable = cen;                            /* �Ƿ�cache? */
    mpu_initure.IsBufferable = ben;                           /* �Ƿ񻺳�? */
    HAL_MPU_ConfigRegion(&mpu_initure);                       /* ����MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);                   /* ����MPU */
    return 0;
}

/**
 * @brief       ������Ҫ�����Ĵ洢��
 * @param       ��
 * @note        ����Բ��ִ洢�������MPU����,������ܵ��³��������쳣
 *              ����MCU������ʾ,����ͷ�ɼ����ݳ����ȵ�����...
 */
void mpu_memory_protection(void)
{
    /* D1 SRAM 512KB：可缓存，用于代码数据和 FreeRTOS 堆 */
    mpu_set_protection(0x24000000,
                       MPU_REGION_SIZE_512KB,
                       MPU_REGION_NUMBER1,
                       MPU_REGION_FULL_ACCESS,
                       MPU_ACCESS_NOT_SHAREABLE,
                       MPU_ACCESS_CACHEABLE,
                       MPU_ACCESS_BUFFERABLE);

    /* SDRAM 32MB：可缓存+可缓冲，用于 LTDC 帧缓冲和 LVGL 堆 */
    mpu_set_protection(0xC0000000,
                       MPU_REGION_SIZE_32MB,
                       MPU_REGION_NUMBER2,
                       MPU_REGION_FULL_ACCESS,
                       MPU_ACCESS_NOT_SHAREABLE,
                       MPU_ACCESS_CACHEABLE,
                       MPU_ACCESS_BUFFERABLE);

    /* D2 SRAM 256KB：不可缓存，用于 LVGL draw buffer（DMA2D 直接访问）
     * 必须 non-cacheable，否则 DMA2D 写入后 CPU 读到的是 Cache 旧数据 */
    mpu_set_protection(0x30000000,
                       MPU_REGION_SIZE_256KB,
                       MPU_REGION_NUMBER3,
                       MPU_REGION_FULL_ACCESS,
                       MPU_ACCESS_NOT_SHAREABLE,
                       MPU_ACCESS_NOT_CACHEABLE,
                       MPU_ACCESS_BUFFERABLE);
}

