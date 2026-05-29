/**
 * @file includes.h
 * @brief OS相关头文件汇总（FreeRTOS版本）
 *
 * delay.c 在 SYS_SUPPORT_OS=1 时会 include 此文件
 * 原本是为 UCOS 设计的，这里适配 FreeRTOS
 *
 * delay.c 里会定义 delay_osschedlock() 和 delay_osschedunlock() 函数，
 * 函数体里调用 OSSchedLock/OSSchedUnlock（UCOS风格）。
 * 我们通过宏把 UCOS 的函数名重定向到 FreeRTOS 的 API。
 *
 * 注意：delay.c 里的函数体是：
 *   void delay_osschedlock(void)  { OSSchedLock(); }
 *   void delay_osschedunlock(void){ OSSchedUnlock(); }
 * 所以只需要把 OSSchedLock/OSSchedUnlock 定义成 FreeRTOS 对应的函数即可。
 */

#ifndef __INCLUDES_H
#define __INCLUDES_H

#include "FreeRTOS.h"
#include "task.h"

/* delay.c 里用到的 OS 状态宏 */
#define delay_osrunning         (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
#define delay_ostickspersec     configTICK_RATE_HZ
#define delay_osintnesting      0   /* FreeRTOS 不需要，设为0 */

/* delay.c 里的 delay_osschedlock/unlock 函数体调用 OSSchedLock/OSSchedUnlock
 * 把这两个 UCOS 函数名映射到 FreeRTOS 的调度锁 API */
#define OSSchedLock()           vTaskSuspendAll()
#define OSSchedUnlock()         xTaskResumeAll()

#endif /* __INCLUDES_H */
