#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* -----------------------------------------------------------
 * Application specific definitions for the LPC1768 Cortex-M3.
 * ----------------------------------------------------------*/

#define configUSE_PREEMPTION        1   // Allows high priority tasks to interrupt low priority ones
#define configUSE_IDLE_HOOK         0
#define configUSE_TICK_HOOK         0
#define configCPU_CLOCK_HZ          ( ( unsigned long ) 100000000 ) // LPC1768 runs at 100 MHz
#define configTICK_RATE_HZ          ( ( TickType_t ) 1000 )         // 1ms RTOS Tick (Time slice)
#define configMAX_PRIORITIES        ( 5 )                           // Priority levels 0 to 4
#define configMINIMAL_STACK_SIZE    ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE       ( ( size_t ) ( 10 * 1024 ) )    // 10KB of RAM allocated for OS
#define configMAX_TASK_NAME_LEN     ( 16 )
#define configUSE_TRACE_FACILITY    0
#define configUSE_16_BIT_TICKS      0
#define configIDLE_SHOULD_YIELD     1

/* Co-routine definitions (Not used in our project) */
#define configUSE_CO_ROUTINES       0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Enable standard FreeRTOS API functions */
#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_vTaskDelete             1
#define INCLUDE_vTaskCleanUpResources   0
#define INCLUDE_vTaskSuspend            1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelay              1   // Required for our vTaskDelay() timing

/* Cortex-M3 specific Interrupt Priorities */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS         __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS         5   /* 32 priority levels */
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         0x1f
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* * CRITICAL STEP FOR KEIL & CMSIS:
 * Map the FreeRTOS interrupt handlers to the standard CMSIS hardware names.
 * Without this, the RTOS will crash immediately upon booting!
 */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
