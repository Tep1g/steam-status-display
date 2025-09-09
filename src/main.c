#include "pico/stdlib.h"
// #include "FreeRTOS.h"
// #include "task.h"
// #include "lcd.h"
// #include "https_client.h"

int main() {
    stdio_init_all();

    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);

    while(1) {
        sleep_ms(1000);
        gpio_put(16, 1);

        sleep_ms(1000);
        gpio_put(16, 0);
    }

    return 0;
}