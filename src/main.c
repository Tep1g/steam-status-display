#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
// #include "lcd.h"
// #include "https_client.h"

static SemaphoreHandle_t mutex;

static void task_1(void *pvParameters) {
    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);

    gpio_init(15);
    gpio_set_dir(15, GPIO_IN);
    gpio_pull_up(15);

    while(1) {
        if (gpio_get(15) == 0) {
            if (xSemaphoreTake(mutex, 0) == pdTRUE) {
                gpio_put(16, 1);
                while(gpio_get(15) == 0);
                gpio_put(16, 0);
                xSemaphoreGive(mutex);
            }
        }
    }
}

static void task_2(void *pvParameters) {
    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);

    gpio_init(14);
    gpio_set_dir(14, GPIO_IN);
    gpio_pull_up(14);

    while(1) {
        if (gpio_get(14) == 0) {
            if (xSemaphoreTake(mutex, 0) == pdTRUE) {
                gpio_put(17, 1);
                while(gpio_get(14) == 0);
                gpio_put(17, 0);
                xSemaphoreGive(mutex);
            }
        }
    }
}

int main() {
    stdio_init_all();

    mutex = xSemaphoreCreateMutex();
    
    TaskHandle_t task_1_handle;
    xTaskCreate(task_1, "task_1", configMINIMAL_STACK_SIZE, NULL, 1, &task_1_handle);
    UBaseType_t lcd_task_uxCoreAffinityMask = 1 << 0; // Set core affinity to core 0
    vTaskCoreAffinitySet(task_1_handle, lcd_task_uxCoreAffinityMask);

    TaskHandle_t task_2_handle;
    xTaskCreate(task_2, "task_2", configMINIMAL_STACK_SIZE, NULL, 1, &task_2_handle);
    UBaseType_t web_api_task_uxCoreAffinityMask = 1 << 1; // Set core affinity to core 1
    vTaskCoreAffinitySet(task_2_handle, web_api_task_uxCoreAffinityMask);

    vTaskStartScheduler();

    return 0;
}