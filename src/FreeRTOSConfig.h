/* src/FreeRTOSConfig.h */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* --------------------------------------------------------------------
 * PC SIMÜLASYONU VE SCHEDULER PROJESİ İÇİN TEMEL AYARLAR
 * -------------------------------------------------------------------- */

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0
#define configTICK_RATE_HZ                      ( 1000 ) /* 1 ms tick */

/* * ÖNEMLİ: PC'de "printf" ve string işlemleri çok fazla Stack kullanır.
 * Varsayılan 128 değeri SegFault (Çökme) yaratır. 
 * Bunu 1024 yaparak çökmeyi engelliyoruz.
 */
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 1024 )

/* * Heap Boyutu: Dinamik görev oluşturma ve 'asprintf' kullanımı için 
 * yeterli alan. 1MB ayırıyoruz.
 */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 1024 * 1024 ) )

#define configMAX_TASK_NAME_LEN                 ( 20 )

/* * Öncelik Seviyesi:
 * Senin 4 seviyen + Idle Task + Controller Task için en az 10 yapıyoruz.
 */
#define configMAX_PRIORITIES                    ( 10 )

#define configUSE_TRACE_FACILITY                1
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configCHECK_FOR_STACK_OVERFLOW          0 
#define configUSE_RECURSIVE_MUTEXES             1
#define configQUEUE_REGISTRY_SIZE               20
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_ALTERNATIVE_API               0
#define configUSE_QUEUE_SETS                    1
#define configUSE_TASK_NOTIFICATIONS            1

/* Software Timer Ayarları */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                20
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* Dahil Edilecek Opsiyonel Fonksiyonlar */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1 
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1 
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetSchedulerState          1

/* Hata Ayıklama (Assert) */
extern void vAssertCalled( unsigned long ulLine, const char * const pcFileName );
#define configASSERT( x ) if( ( x ) == 0 ) vAssertCalled( __LINE__, __FILE__ )

#endif /* FREERTOS_CONFIG_H */