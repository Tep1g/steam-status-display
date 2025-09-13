#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "st7796.h"
#include "lvgl.h"
#include "pico/cyw43_arch.h"
#include "http_client_util.h"
#include "tiny-json.h"
#include <string.h>
// #include "lcd.h"
// #include "https_client.h"

#define SSID ""
#define PASSWORD ""

#define MOCK_IO_EXAMPLE_HOSTNAME ""
#define MOCK_IO_URL_REQUEST ""

#define LVGL_TICK_PERIOD_MS 100

#define ST7796_SPI_SCK 2
#define ST7796_SPI_MOSI 3
#define ST7796_SPI_MISO 4
#define ST7796_SPI_CS 5
#define ST7796_SPI_DCX 6
#define ST7796_SPI_RST 7

const uint32_t st7796_hor_res = 320;
const uint32_t st7796_ver_res = 480;
const lv_lcd_flag_t st7796_flag = LV_LCD_FLAG_NONE;
const uint st7796_dma_irq_index = 0;

static void make_http_request(void *cb_arg, altcp_recv_fn recv_fn, char *host, char *url_request) {
    HTTP_REQUEST_T request = {0};
    request.callback_arg = cb_arg;
    request.hostname = host;
    request.url = url_request;
    request.headers_fn = http_client_header_callback;
    request.recv_fn = recv_fn;
    http_client_request_sync(cyw43_arch_async_context(), &request);
}

static void https_client_init() {

    printf("Initing cyw43 arch with country\n");
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA));

    printf("Enabling cyw43 wifi sta mode\n");
    cyw43_arch_enable_sta_mode();

    printf("Initing cyw43 wifi connection\n");
    while(cyw43_arch_wifi_connect_timeout_ms(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 4000)) {
        printf("Failed to connect, retrying\n");
    }
}

static void lv_tick_timer_callback(TimerHandle_t xTimer) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void task_1(void *pvParameters) {
    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);

    gpio_init(15);
    gpio_set_dir(15, GPIO_IN);
    gpio_pull_up(15);

    gpio_init(14);
    gpio_set_dir(14, GPIO_IN);
    gpio_pull_up(14);

    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);

    while(1) {
        if (gpio_get(15) == 0) {
            gpio_put(16, 1);
            while(gpio_get(15) == 0);
            gpio_put(16, 0);
        }
        if (gpio_get(14) == 0) {
            gpio_put(17, 1);
            while(gpio_get(14) == 0);
            gpio_put(17, 0);
        }
    }
}

static void task_2(void *pvParameters) {
    sleep_ms(6500);
    
    lv_init();
    lv_delay_set_cb(sleep_ms);

    spi_init(spi0, 12500000);
    spi_set_slave(spi0, false);
    gpio_set_function(ST7796_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(ST7796_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ST7796_SPI_MISO, GPIO_FUNC_SPI);
    gpio_init(ST7796_SPI_CS);
    gpio_init(ST7796_SPI_DCX);
    gpio_init(ST7796_SPI_RST);
    gpio_set_dir(ST7796_SPI_CS, GPIO_OUT);
    gpio_set_dir(ST7796_SPI_DCX, GPIO_OUT);
    gpio_set_dir(ST7796_SPI_RST, GPIO_OUT);

    const int st7796_dma_channel = dma_claim_unused_channel(true);
    dma_channel_config st7796_dma_config = dma_channel_get_default_config(st7796_dma_channel);
    channel_config_set_transfer_data_size(&st7796_dma_config, DMA_SIZE_16);
    channel_config_set_dreq(&st7796_dma_config, spi_get_dreq(spi0, true));
    channel_config_set_read_increment(&st7796_dma_config, true);
    channel_config_set_write_increment(&st7796_dma_config, false);

    dma_channel_set_irq0_enabled(st7796_dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, st7796_send_color_callback);
    irq_set_enabled(DMA_IRQ_0, true);

    lv_disp_t *lv_st7796 = st7796_init(
        st7796_hor_res, 
        st7796_ver_res, 
        st7796_flag, 
        spi0, 
        ST7796_SPI_CS, 
        ST7796_SPI_DCX, 
        ST7796_SPI_RST, 
        st7796_dma_channel, 
        st7796_dma_irq_index, 
        &st7796_dma_config
    );

    lv_display_set_rotation(lv_st7796, LV_DISPLAY_ROTATION_270);
    
    lv_color_t *buf1 = NULL;
    lv_color_t *buf2 = NULL;

    uint32_t buf_size = st7796_hor_res * st7796_ver_res / 10 * lv_color_format_get_size(lv_display_get_color_format(lv_st7796));

    buf1 = lv_malloc(buf_size);
    buf2 = lv_malloc(buf_size);

    lv_display_set_buffers(lv_st7796, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_100, 0);
    
    lv_obj_t *obj = lv_label_create(scr);
    lv_obj_set_align(obj, LV_ALIGN_CENTER);
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    lv_obj_set_width(obj, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(obj, lv_color_black(), 0);

    printf("Connecting to wifi\n");
    https_client_init();
    sleep_ms(50);
    printf("Connected to wifi\n");

    char resp_json_buf[30] = {0};
    struct steam_response_json resp_json = {
        .buf = resp_json_buf,
        .len = 0
    };

    printf("Making http request\n");

    make_http_request((void *)&resp_json, http_client_recv_json_callback, MOCK_IO_EXAMPLE_HOSTNAME, MOCK_IO_URL_REQUEST);

    printf("Received response\n");
    printf(resp_json_buf);
    printf("\n");

    printf("Converting response to JSON object\n");
    json_t json_mem[64];
    printf("1\n");
    const json_t *json_object = json_create(resp_json_buf, json_mem, 64);
    printf("2\n");
    const json_t *msg_property = json_getProperty(json_object, "message");
    printf("3\n");
    const char *msg = json_getValue(msg_property);
    printf("4\n");
    char msg_buffer[30] = {0};
    printf("5\n");
    strcpy(msg_buffer, msg);
    printf("Extracted message\n");
    lv_label_set_text(obj, msg_buffer);
    printf("Outputted to display\n");

    TimerHandle_t lv_tick_timer = xTimerCreate("lv_tick Timer", pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS), pdFALSE, (void *)0, lv_tick_timer_callback);
    xTimerStart(lv_tick_timer, 0);

    while (1) {
        lv_timer_handler();
        printf("Looping in task 2\n");
        sleep_ms(1000);
    }
}

int main() {
    stdio_init_all();
    
    TaskHandle_t task_1_handle;
    xTaskCreate(task_1, "task_1", configMINIMAL_STACK_SIZE*4, NULL, 1, &task_1_handle);
    UBaseType_t task_1_uxCoreAffinityMask = 1 << 1; // Set core affinity to core 0
    vTaskCoreAffinitySet(task_1_handle, task_1_uxCoreAffinityMask);

    TaskHandle_t task_2_handle;
    xTaskCreate(task_2, "task_2", configMINIMAL_STACK_SIZE*4, NULL, 2, &task_2_handle);
    UBaseType_t task_2_uxCoreAffinityMask = 1 << 0; // Set core affinity to core 1
    vTaskCoreAffinitySet(task_2_handle, task_2_uxCoreAffinityMask);

    vTaskStartScheduler();

    return 0;
}