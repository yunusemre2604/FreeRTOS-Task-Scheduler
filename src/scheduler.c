/* src/scheduler.c */

// asprintf fonksiyonu için gereklidir
#define _GNU_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

SimTask taskList[MAX_TASKS];
int taskCount = 0;
int globalTime = 0;

extern void vGenericTask(void *pvParameters);

// Renk paleti
const char* colors[] = {ANSI_BLUE, ANSI_RED, ANSI_GREEN, ANSI_YELLOW, ANSI_MAGENTA, ANSI_CYAN};

// Çıktı Fonksiyonu
void PrintTaskInfo(SimTask* task, char* statusMsg)
{
    printf("%s%10.4f sn %-30s (id:%04d  oncelik:%d  kalan sure:%d sn)%s\n",
           task->color,
           (float)globalTime,       
           statusMsg,               
           task->id,
           task->currentPriority,
           task->remainingTime,
           ANSI_RESET);
    fflush(stdout);
}

void ReadTasksFromFile(const char* filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Dosya okuma hatasi");
        exit(1);
    }
    
    int arrival, prio, burst;
    int id_counter = 0;

    // giris.txt formatı: varış, öncelik, süre
    while (fscanf(file, "%d, %d, %d", &arrival, &prio, &burst) == 3) {
        if(taskCount >= MAX_TASKS) break;

        taskList[taskCount].id = id_counter;
        
        // İsimlendirme task1, task2... şeklinde
        snprintf(taskList[taskCount].taskName, 20, "task%d", id_counter + 1);
        
        taskList[taskCount].arrivalTime = arrival;
        taskList[taskCount].initialPriority = prio;
        taskList[taskCount].currentPriority = prio;
        taskList[taskCount].burstTime = burst;
        taskList[taskCount].remainingTime = burst;
        
        // Başlangıçta son aktivite zamanı = varış zamanı
        taskList[taskCount].lastActiveTime = arrival;

        taskList[taskCount].state = STATE_WAITING;
        taskList[taskCount].handle = NULL;
        
        strcpy(taskList[taskCount].color, colors[id_counter % 6]);
        
        id_counter++;
        taskCount++;
    }
    fclose(file);
    printf("[Init] %d gorev dosyadan okundu.\n", taskCount);
}

void vSchedulerController(void *pvParameters)
{
    (void) pvParameters;
    
    // Dinamik string için pointer
    char *msg = NULL; 

    // ROUND ROBIN HAFIZASI (Prio 3 için)
    static int lastPrio3Index = -1;

    for(;;)
    {
        // --- 1. ZAMANAŞIMI (TIMEOUT) KONTROLÜ ---
        // Son aktivite zamanına göre kontrol
        for(int i=0; i<taskCount; i++) {
            if(taskList[i].state != STATE_TERMINATED && taskList[i].state != STATE_WAITING) {
                
                if ((globalTime - taskList[i].lastActiveTime) >= 20) {
                    
                    asprintf(&msg, "%s zamanasimi", taskList[i].taskName);
                    PrintTaskInfo(&taskList[i], msg);
                    free(msg); 
                    
                    if(taskList[i].handle != NULL) {
                        vTaskDelete(taskList[i].handle);
                    }
                    taskList[i].state = STATE_TERMINATED;
                }
            }
        }

        // --- 2. YENİ GÖREVLERİ AL ---
        for(int i=0; i<taskCount; i++) {
            if(taskList[i].state == STATE_WAITING && taskList[i].arrivalTime <= globalTime) {
                // Görevi oluştur
                xTaskCreate(vGenericTask, taskList[i].taskName, configMINIMAL_STACK_SIZE, 
                            NULL, tskIDLE_PRIORITY + 1, &taskList[i].handle);
                
                if(taskList[i].handle != NULL) {
                    vTaskSuspend(taskList[i].handle); // Hemen durdur
                }
                
                taskList[i].state = STATE_READY;
                // Yeni geldiği için lastActiveTime zaten arrivalTime

                asprintf(&msg, "%s basladi", taskList[i].taskName);
                PrintTaskInfo(&taskList[i], msg);
                free(msg);
            }
        }

        // --- 3. ÇALIŞACAK GÖREVİ SEÇ (ALGORİTMA) ---
        SimTask *selectedTask = NULL;

        // ADIM A: Önce Prio 0, 1 ve 2'ye bak (FCFS / Priority)
        for(int p=0; p<3; p++) {
            for(int i=0; i<taskCount; i++) {
                if(taskList[i].state == STATE_READY && taskList[i].currentPriority == p) {
                    selectedTask = &taskList[i];
                    goto TASK_FOUND;
                }
            }
        }

        // ADIM B: Prio 3 için ROUND ROBIN uygula
        if(selectedTask == NULL) {
            int count = 0;
            int i = (lastPrio3Index + 1) % taskCount; 

            while(count < taskCount) {
                if(taskList[i].state == STATE_READY && taskList[i].currentPriority == 3) {
                    selectedTask = &taskList[i];
                    lastPrio3Index = i; // Hafızayı güncelle
                    goto TASK_FOUND;
                }
                i = (i + 1) % taskCount;
                count++;
            }
        }

        TASK_FOUND:

        if(selectedTask != NULL) {
            // --- 4. GÖREVİ YÜRÜT ---
            asprintf(&msg, "%s yurutuluyor", selectedTask->taskName);
            PrintTaskInfo(selectedTask, msg);
            free(msg);

            // FreeRTOS taskını uyandır
            if(selectedTask->handle != NULL) {
                vTaskResume(selectedTask->handle);
            }
            
            // Simülasyon Hızı
            vTaskDelay(pdMS_TO_TICKS(75));
            
            globalTime++;
            selectedTask->remainingTime--;
            
            // Son Aktivite zamanını güncelle
            selectedTask->lastActiveTime = globalTime;
            
            // Görevi durdur
            if(selectedTask->handle != NULL) {
                vTaskSuspend(selectedTask->handle);
            }

            // --- 5. DURUM GÜNCELLEME ---
            if(selectedTask->remainingTime <= 0) {
                asprintf(&msg, "%s sonlandi", selectedTask->taskName);
                PrintTaskInfo(selectedTask, msg);
                free(msg);
                
                if(selectedTask->handle != NULL) {
                    vTaskDelete(selectedTask->handle);
                    selectedTask->handle = NULL;
                }
                selectedTask->state = STATE_TERMINATED;
            } 
            else {
                // --- İSTEDİĞİN DÜZELTME BURADA YAPILDI ---
                
                // Durum 1: Öncelik düşebiliyorsa (1 ve 2 ise)
                if(selectedTask->currentPriority > 0 && selectedTask->currentPriority < 3) {
                    selectedTask->currentPriority++; 
                    asprintf(&msg, "%s askida (Prio Dusuruldu)", selectedTask->taskName);
                    PrintTaskInfo(selectedTask, msg);
                    free(msg);
                } 
                // Durum 2: Zaten en alttaysa (Prio 3) yine de askıya alındı mesajı yaz
                else if(selectedTask->currentPriority == 3) {
                    asprintf(&msg, "%s askida", selectedTask->taskName);
                    PrintTaskInfo(selectedTask, msg);
                    free(msg);
                }
                
                selectedTask->state = STATE_READY;
            }

        } else {
            // IDLE
            globalTime++;
            vTaskDelay(pdMS_TO_TICKS(75));
        }

        // ÇIKIŞ KONTROLÜ
        int active = 0;
        for(int i=0; i<taskCount; i++) {
            if(taskList[i].state != STATE_TERMINATED) active++;
        }
        if(active == 0) {
            printf("\n--- Tum gorevler tamamlandi ---\n");
            exit(0);
        }
    }
}

void Scheduler_Init(void) { }

void Scheduler_Start(void) {
    xTaskCreate(vSchedulerController, "Controller", configMINIMAL_STACK_SIZE * 4, NULL, configMAX_PRIORITIES - 1, NULL);
    vTaskStartScheduler();
} 
