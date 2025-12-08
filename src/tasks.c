/* src/tasks.c */
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

// Görev sadece var olur, çıktı üretmez (Çıktıyı Controller üretir)
void vGenericTask(void *pvParameters)
{
    (void) pvParameters; 
    
    for(;;)
    {
        // İşlemci yükü simülasyonu
        // Controller askıya alana kadar burada döner
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}