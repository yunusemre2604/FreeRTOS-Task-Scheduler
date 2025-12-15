/* src/scheduler.c */

#define _GNU_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

// Linked List Başlangıç İşaretçisi
SimTask *taskListHead = NULL;
int globalTime = 0;
int dynamicNameCounter = 1;

extern void vGenericTask(void *pvParameters);

const char* colors[] = {ANSI_BLUE, ANSI_RED, ANSI_GREEN, ANSI_YELLOW, ANSI_MAGENTA, ANSI_CYAN};

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

// Listeye eleman ekleme
void AddTaskToLinkedList(SimTask *newTask) {
    if (taskListHead == NULL) {
        taskListHead = newTask;
    } else {
        SimTask *temp = taskListHead;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newTask;
    }
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
    int count = 0;

    while (fscanf(file, "%d, %d, %d", &arrival, &prio, &burst) == 3) {
        
        SimTask *newTask = (SimTask*)malloc(sizeof(SimTask));
        if (newTask == NULL) {
            perror("Bellek hatasi");
            exit(1);
        }

        newTask->id = id_counter;
        snprintf(newTask->taskName, 20, "T_ID%d", id_counter);
        newTask->displayName[0] = '\0';
        newTask->nameAssigned = 0;

        newTask->arrivalTime = arrival;
        newTask->initialPriority = prio;
        newTask->currentPriority = prio;
        newTask->burstTime = burst;
        newTask->remainingTime = burst;
        newTask->lastActiveTime = arrival;
        
        // Başlangıçta kuyruk zamanı = varış zamanı
        newTask->queueEntryTime = arrival;

        newTask->state = STATE_WAITING;
        newTask->handle = NULL;
        newTask->next = NULL; 
        
        strcpy(newTask->color, colors[id_counter % 6]);
        
        AddTaskToLinkedList(newTask);

        id_counter++;
        count++;
    }
    fclose(file);
    printf("[Init] %d gorev dinamik bellege (Linked List) yuklendi.\n", count);
}

void vSchedulerController(void *pvParameters)
{
    (void) pvParameters;
    
    char *msg = NULL; 
    static int lastScheduledTaskId = -1;

    for(;;)
    {
        // --- 1. ZAMANAŞIMI KONTROLÜ ---
        SimTask *iterator = taskListHead;
        int activeTasks = 0;

        while(iterator != NULL) {
            
            if(iterator->state != STATE_TERMINATED) activeTasks++;

            if(iterator->state != STATE_TERMINATED && iterator->state != STATE_WAITING) {
                if ((globalTime - iterator->lastActiveTime) >= 20) {
                    
                    if(iterator->nameAssigned == 0) {
                        snprintf(iterator->displayName, 20, "task%d", dynamicNameCounter++);
                        iterator->nameAssigned = 1;
                    }

                    asprintf(&msg, "%s zamanasimi", iterator->displayName);
                    PrintTaskInfo(iterator, msg);
                    free(msg); 
                    
                    if(iterator->handle != NULL) vTaskDelete(iterator->handle);
                    iterator->state = STATE_TERMINATED;
                }
            }
            iterator = iterator->next; 
        }

        // ÇIKIŞ KONTROLÜ
        if(activeTasks == 0) {
            printf("\n--- Tum gorevler tamamlandi ---\n");
            SimTask *current = taskListHead;
            while(current != NULL) {
                SimTask *next = current->next;
                free(current);
                current = next;
            }
            exit(0);
        }

        // --- 2. YENİ GÖREVLERİ AL ---
        iterator = taskListHead;
        while(iterator != NULL) {
            if(iterator->state == STATE_WAITING && iterator->arrivalTime <= globalTime) {
                xTaskCreate(vGenericTask, "Generic", configMINIMAL_STACK_SIZE, 
                            NULL, tskIDLE_PRIORITY + 1, &iterator->handle);
                
                if(iterator->handle != NULL) vTaskSuspend(iterator->handle);
                
                iterator->state = STATE_READY;
                // Yeni gelen görevin queueEntryTime'ı zaten arrivalTime olarak ayarlı
            }
            iterator = iterator->next;
        }

        // --- 3. ÇALIŞACAK GÖREVİ SEÇ ---
        SimTask *selectedTask = NULL;
        iterator = taskListHead;

        while(iterator != NULL) {
            if(iterator->state == STATE_READY) {
                int isBetter = 0;

                if(selectedTask == NULL) {
                    isBetter = 1;
                } else {
                    // Kriter 1: Düşük Öncelik Değeri (0, 1, 2...)
                    if(iterator->currentPriority < selectedTask->currentPriority) {
                        isBetter = 1;
                    } 
                    // Kriter 2: Öncelikler Eşitse
                    else if(iterator->currentPriority == selectedTask->currentPriority) {
                        
                        // HİBRİT MANTIK:
                        // Eğer öncelik yüksekse (0, 1, 2) -> ID sıralaması (Eski düzen korunsun)
                        if(iterator->currentPriority < 3) {
                            if(iterator->id < selectedTask->id) {
                                isBetter = 1;
                            }
                        }
                        // Eğer öncelik düşükse (3, 4, 5...) -> KUYRUK ZAMANI (Round Robin FIFO)
                        else {
                            if(iterator->queueEntryTime < selectedTask->queueEntryTime) {
                                isBetter = 1;
                            }
                            else if(iterator->queueEntryTime == selectedTask->queueEntryTime) {
                                if(iterator->id < selectedTask->id) { // Zaman eşitse ID'ye bak
                                    isBetter = 1;
                                }
                            }
                        }
                    }
                }

                if(isBetter) {
                    selectedTask = iterator;
                }
            }
            iterator = iterator->next;
        }

        // TASK FOUND
        if(selectedTask != NULL) {
            
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
                // KRİTİK DÜZELTME: 
                // Görev çalıştı ve sırasını savdı. Artık kuyruğun en sonuna geçmeli.
                // Zaman damgasını 'şu an' yapıyoruz. Böylece Round Robin'de en arkaya geçer.
                selectedTask->queueEntryTime = globalTime;

                // Feedback (Öncelik düşürme)
                if(selectedTask->currentPriority > 0) {
                    selectedTask->currentPriority++; 
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
    }
}

void Scheduler_Init(void) { }

void Scheduler_Start(void) {
    xTaskCreate(vSchedulerController, "Controller", configMINIMAL_STACK_SIZE * 4, NULL, configMAX_PRIORITIES - 1, NULL);
    vTaskStartScheduler();
}