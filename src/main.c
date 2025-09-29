#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "https_client.h"

#define WEB_API_TASK_STACK_SIZE 2048U

#define LCD_TASK_STACK_SIZE 2048

int main() {
    stdio_init_all();

    (get_steam_user_data_ptr())->mutex = xSemaphoreCreateMutex();
    
    TaskHandle_t lcd_task_handle;
    xTaskCreate(lcd_task, "LCD Task", LCD_TASK_STACK_SIZE, NULL, 1, &lcd_task_handle);
    UBaseType_t lcd_task_uxCoreAffinityMask = 1 << 0; // Set core affinity to core 0
    vTaskCoreAffinitySet(lcd_task_handle, lcd_task_uxCoreAffinityMask);

    TaskHandle_t web_api_task_handle;
    xTaskCreate(https_client_task, "Web API Task", WEB_API_TASK_STACK_SIZE, NULL, 1, &web_api_task_handle);
    UBaseType_t web_api_task_uxCoreAffinityMask = 1 << 1; // Set core affinity to core 1
    vTaskCoreAffinitySet(web_api_task_handle, web_api_task_uxCoreAffinityMask);

    vTaskStartScheduler();

    return 0;
}