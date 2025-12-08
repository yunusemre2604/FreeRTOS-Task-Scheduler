/* src/main.c */
#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
    printf("ASSERT ERROR! File: %s Line: %ld\n", pcFileName, ulLine);
    exit(-1);
}

int main(int argc, char *argv[])
{
    printf("==========================================\n");
    printf("   FreeRTOS 4-Level Scheduler Simulator   \n");
    printf("==========================================\n");

    Scheduler_Init();
    
    // Varsayılan dosya giris.txt, argüman varsa onu kullan
    const char* filename = "giris.txt";
    if(argc > 1) {
        filename = argv[1];
    }
    
    ReadTasksFromFile(filename);

    Scheduler_Start();

    return 0;
}