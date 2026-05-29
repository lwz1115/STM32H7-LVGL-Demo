/**
 ****************************************************************************************************

 ****************************************************************************************************
 */

#ifndef _SYS_H
#define _SYS_H

#include "stm32h7xx.h"
#include "core_cm7.h"
#include "stm32h7xx_hal.h"


/**
 * SYS_SUPPORT_OS���ڶ���ϵͳ�ļ����Ƿ�֧��OS
 * 0,��֧��OS
 * 1,֧��OS (uC/OS)
 * 
 * ע�⣺����Ŀʹ�� FreeRTOS������ʹ�� uC/OS
 * FreeRTOS �����Լ����ӳٺ��� vTaskDelay()
 * ���������������Ϊ 0��ʹ�ü򵥵��ӳٺ���
 */
#define SYS_SUPPORT_OS         0


#define      ON      1
#define      OFF     0
#define      Write_Through()    do{ *(__IO uint32_t*)0XE000EF9C = 1UL << 2; }while(0)     /* Cache͸дģʽ */

void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset);                       /* �����ж�ƫ���� */
void sys_cache_enable(void);                                                              /* ʹ��STM32H7��L1-Cahce */
uint8_t sys_stm32_clock_init(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq); /* ����ϵͳʱ�� */


/* ����Ϊ��ຯ�� */
void sys_wfi_set(void);             /* ִ��WFIָ�� */
void sys_intx_disable(void);        /* �ر������ж� */
void sys_intx_enable(void);         /* ���������ж� */
void sys_msr_msp(uint32_t addr);    /* ����ջ����ַ */

#endif

