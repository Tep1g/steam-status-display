#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
// #include "lcd.h"
// #include "https_client.h"

static void task_1(void *pvParameters) {
    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);

    while(1) {
        sleep_ms(1000);
        gpio_put(16, 1);

        sleep_ms(1000);
        gpio_put(16, 0);
    }
}

static void task_2(void *pvParameters) {
    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);

    while(1) {
        sleep_ms(1000);
        gpio_put(17, 1);

        sleep_ms(1000);
        gpio_put(17, 0);
    }
}

int main() {
    stdio_init_all();

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