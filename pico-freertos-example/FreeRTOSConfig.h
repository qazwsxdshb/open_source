#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

extern void vAssertCalled(const char *file, int line);

#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#if defined(PICO_RP2350)
#define configCPU_CLOCK_HZ                      150000000UL
#define configENABLE_MPU                        0
#define configENABLE_TRUSTZONE                  0
#define configRUN_FREERTOS_SECURE_ONLY          1
#define configENABLE_FPU                        1
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    16
#else
#define configCPU_CLOCK_HZ                      125000000UL
#endif
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                256
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

#define configNUMBER_OF_CORES                   1
#define configTICK_CORE                         0

#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_TASK_NOTIFICATIONS            1

#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            256

#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#if defined(PICO_RP2350)
#define configTOTAL_HEAP_SIZE                   (192 * 1024)
#else
#define configTOTAL_HEAP_SIZE                   (128 * 1024)
#endif
#define configAPPLICATION_ALLOCATED_HEAP        0

#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0

#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0

#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xSemaphoreGetMutexHolder        1

#define configASSERT(condition) \
    do { \
        if (!(condition)) { \
            vAssertCalled(__FILE__, __LINE__); \
        } \
    } while (0)

#endif
