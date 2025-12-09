/* src/scheduler.h */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"

// Renk Kodları
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_WHITE   "\x1b[37m"

#define MAX_TASKS 50

typedef enum {
    STATE_WAITING,
    STATE_READY,
    STATE_RUNNING,
    STATE_SUSPENDED,
    STATE_TERMINATED
} TaskState;

typedef struct {
    int id;
    
    // Dinamik İsimlendirme Alanları
    char displayName[20]; 
    int nameAssigned;     
    
    char taskName[20];    
    int arrivalTime;
    int initialPriority;
    int currentPriority;
    int burstTime;
    int remainingTime;
    int lastActiveTime;     

    // YENİ: Kuyruğa giriş zamanı (Sıralama için)
    int queueEntryTime;

    TaskState state;
    TaskHandle_t handle;
    char color[10];
} SimTask;

void Scheduler_Init(void);
void Scheduler_Start(void);
void ReadTasksFromFile(const char* filename);

#endif /* SCHEDULER_H */