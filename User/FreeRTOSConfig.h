/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS配置文件 - STM32H743 @ 480MHz
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ============================================================
 * 基础配置
 * ============================================================ */
#define configUSE_PREEMPTION                    1   /* 使用抢占式调度 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0   /* 不使用硬件优化（CM7通用） */
#define configUSE_TICKLESS_IDLE                 0   /* 不使用低功耗tickless */
#define configCPU_CLOCK_HZ                      480000000UL  /* 480MHz */
#define configTICK_RATE_HZ                      1000         /* 1ms tick */
#define configMAX_PRIORITIES                    32
#define configMINIMAL_STACK_SIZE                128  /* 最小栈大小（word） */
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configSTACK_DEPTH_TYPE                  uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* ============================================================
 * 内存分配
 * ============================================================ */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   (32 * 1024)  /* 32KB FreeRTOS堆 */
#define configAPPLICATION_ALLOCATED_HEAP        0  /* 使用默认堆分配 */

/* ============================================================
 * Hook 函数
 * ============================================================ */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0   /* lv_tick_inc 由 TIM6 中断提供，不需要 TickHook */
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* ============================================================
 * 运行时统计
 * ============================================================ */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* ============================================================
 * 协程（不使用）
 * ============================================================ */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         2

/* ============================================================
 * 软件定时器
 * ============================================================ */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)

/* ============================================================
 * 中断优先级配置（Cortex-M7）
 * STM32H7 使用 4 位优先级，NVIC_PRIORITYGROUP_4
 * ============================================================ */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

#define configKERNEL_INTERRUPT_PRIORITY     ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - 4) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - 4) )

/* ============================================================
 * API 函数映射
 * ============================================================ */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1


#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              1

/* ============================================================
 * Cortex-M7 特定配置
 * ============================================================ */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif

/* 把 FreeRTOS 的 SVC/PendSV 中断处理函数映射到 HAL 期望的名字 */
/* SysTick_Handler 在 stm32h7xx_it.c 中实现，调用 xPortSysTickHandler() */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler

#endif /* FREERTOS_CONFIG_H */
