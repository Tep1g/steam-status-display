#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "https_client.h"

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName ) {
    ( void ) pxTask;
    ( void ) pcTaskName;

    configASSERT( ( volatile void * ) NULL );
}

void vApplicationMallocFailedHook( void ) {
    configASSERT( ( volatile void * ) NULL );
}

void vApplicationTickHook( void ) {}

void main() {
    stdio_init_all();

    (get_steam_user_data_ptr())->mutex = xSemaphoreCreateMutex();
    
    TaskHandle_t lcd_task_handle;
    xTaskCreate(lcd_task, "LCD Task", 2048, NULL, 1, &lcd_task_handle);
    UBaseType_t lcd_task_uxCoreAffinityMask = 1 << 0; // Set core affinity to core 0
    vTaskCoreAffinitySet(lcd_task_handle, lcd_task_uxCoreAffinityMask);
    vTaskStartScheduler();

    return 0;
}