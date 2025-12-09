/* src/scheduler.c */

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

// Dinamik İsimlendirme Sayacı
int dynamicNameCounter = 1;

extern void vGenericTask(void *pvParameters);

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

    while (fscanf(file, "%d, %d, %d", &arrival, &prio, &burst) == 3) {
        if(taskCount >= MAX_TASKS) break;

        taskList[taskCount].id = id_counter;
        snprintf(taskList[taskCount].taskName, 20, "T_ID%d", id_counter);
        taskList[taskCount].displayName[0] = '\0';
        taskList[taskCount].nameAssigned = 0;

        taskList[taskCount].arrivalTime = arrival;
        taskList[taskCount].initialPriority = prio;
        taskList[taskCount].currentPriority = prio;
        taskList[taskCount].burstTime = burst;
        taskList[taskCount].remainingTime = burst;
        taskList[taskCount].lastActiveTime = arrival;
        
        // YENİ: Başlangıçta kuyruk zamanı = varış zamanı
        taskList[taskCount].queueEntryTime = arrival;

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
    
    char *msg = NULL; 
    static int lastScheduledTaskId = -1;

    for(;;)
    {
        // --- 1. ZAMANAŞIMI KONTROLÜ ---
        for(int i=0; i<taskCount; i++) {
            if(taskList[i].state != STATE_TERMINATED && taskList[i].state != STATE_WAITING) {
                if ((globalTime - taskList[i].lastActiveTime) >= 20) {
                    
                    if(taskList[i].nameAssigned == 0) {
                        snprintf(taskList[i].displayName, 20, "task%d", dynamicNameCounter++);
                        taskList[i].nameAssigned = 1;
                    }

                    asprintf(&msg, "%s zamanasimi", taskList[i].displayName);
                    PrintTaskInfo(&taskList[i], msg);
                    free(msg); 
                    
                    if(taskList[i].handle != NULL) vTaskDelete(taskList[i].handle);
                    taskList[i].state = STATE_TERMINATED;
                }
            }
        }

        // --- 2. YENİ GÖREVLERİ AL ---
        for(int i=0; i<taskCount; i++) {
            if(taskList[i].state == STATE_WAITING && taskList[i].arrivalTime <= globalTime) {
                xTaskCreate(vGenericTask, "Generic", configMINIMAL_STACK_SIZE, 
                            NULL, tskIDLE_PRIORITY + 1, &taskList[i].handle);
                if(taskList[i].handle != NULL) vTaskSuspend(taskList[i].handle);
                
                taskList[i].state = STATE_READY;
                // Yeni gelen görev için queueEntryTime zaten arrivalTime olarak ayarlı
            }
        }

        // --- 3. ÇALIŞACAK GÖREVİ SEÇ ---
        SimTask *selectedTask = NULL;

        // ADIM A: Prio 0, 1, 2 (Burayı BOZMADIK, ID sırasına göre FCFS)
        for(int p=0; p<3; p++) {
            for(int i=0; i<taskCount; i++) {
                if(taskList[i].state == STATE_READY && taskList[i].currentPriority == p) {
                    selectedTask = &taskList[i];
                    goto TASK_FOUND;
                }
            }
        }

        // ADIM B: Prio 3 (Round Robin - ARTIK FIFO MANTIĞI İLE ÇALIŞIYOR)
        // Burada ID sırasına değil, Kuyruğa Giriş Zamanına (queueEntryTime) bakıyoruz.
        if(selectedTask == NULL) {
            SimTask *bestCandidate = NULL;
            int minTime = 2147483647; // Max Int

            for(int i=0; i<taskCount; i++) {
                if(taskList[i].state == STATE_READY && taskList[i].currentPriority == 3) {
                    // En küçük (en eski) zaman damgasını bul
                    if(taskList[i].queueEntryTime < minTime) {
                        minTime = taskList[i].queueEntryTime;
                        bestCandidate = &taskList[i];
                    }
                    // Zamanlar eşitse ID'ye bak (Stabilite için)
                    else if(taskList[i].queueEntryTime == minTime) {
                        if(bestCandidate == NULL || taskList[i].id < bestCandidate->id) {
                            bestCandidate = &taskList[i];
                        }
                    }
                }
            }
            selectedTask = bestCandidate;
        }

        TASK_FOUND:

        if(selectedTask != NULL) {
            
            // İsim Atama
            if(selectedTask->nameAssigned == 0) {
                snprintf(selectedTask->displayName, 20, "task%d", dynamicNameCounter++);
                selectedTask->nameAssigned = 1;
            }

            // --- 4. GÖREVİ YÜRÜT ---
            if(selectedTask->id != lastScheduledTaskId) {
                 asprintf(&msg, "%s basladi", selectedTask->displayName);
                 PrintTaskInfo(selectedTask, msg);
                 free(msg);
            } 
            else {
                 asprintf(&msg, "%s yurutuluyor", selectedTask->displayName);
                 PrintTaskInfo(selectedTask, msg);
                 free(msg);
            }

            lastScheduledTaskId = selectedTask->id;

            if(selectedTask->handle != NULL) vTaskResume(selectedTask->handle);
            
            vTaskDelay(pdMS_TO_TICKS(75));
            
            globalTime++;
            selectedTask->remainingTime--;
            selectedTask->lastActiveTime = globalTime;
            
            if(selectedTask->handle != NULL) vTaskSuspend(selectedTask->handle);

            // --- 5. DURUM GÜNCELLEME ---
            if(selectedTask->remainingTime <= 0) {
                asprintf(&msg, "%s sonlandi", selectedTask->displayName);
                PrintTaskInfo(selectedTask, msg);
                free(msg);
                
                if(selectedTask->handle != NULL) {
                    vTaskDelete(selectedTask->handle);
                    selectedTask->handle = NULL;
                }
                selectedTask->state = STATE_TERMINATED;
                lastScheduledTaskId = -1; 
            } 
            else {
                // YENİ: Görev çalıştı, sırasını savdı. 
                // Kuyruğun en arkasına geçmesi için zaman damgasını güncelliyoruz.
                // Bu sayede Round Robin döngüsü sağlanır.
                selectedTask->queueEntryTime = globalTime;

                // FEEDBACK
                if(selectedTask->currentPriority > 0 && selectedTask->currentPriority < 3) {
                    selectedTask->currentPriority++; 
                    asprintf(&msg, "%s askida (Prio Dusuruldu)", selectedTask->displayName);
                    PrintTaskInfo(selectedTask, msg);
                    free(msg);
                } 
                else if(selectedTask->currentPriority == 3) {
                    asprintf(&msg, "%s askida", selectedTask->displayName);
                    PrintTaskInfo(selectedTask, msg);
                    free(msg);
                }
                
                selectedTask->state = STATE_READY;
            }

        } else {
            // IDLE
            globalTime++;
            lastScheduledTaskId = -1;
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