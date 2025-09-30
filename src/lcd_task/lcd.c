#include "lcd.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "FreeRTOS.h"
#include "https_client.h"
#include "st7796.h"

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

static struct steam_user_data_t *user_data;

static bool updating_lv_objects = false;

static lv_obj_t *lv_user_name_label;
static lv_obj_t *lv_avatar_icon_img;
static lv_obj_t *lv_game_icon_img;
static lv_image_dsc_t lv_game_icon_dsc;
static lv_image_dsc_t lv_avatar_icon_dsc;

static void update_avatar_icon() {
    if (user_data->avatar_icon_changed) {
        lv_obj_invalidate(lv_avatar_icon_img);
        user_data->avatar_icon_changed = false;
    }
}

static void update_game_icon() {
    switch (user_data->game_icon_state) {
        case GAME_ICON_SWITCHED:
            lv_obj_invalidate(lv_game_icon_img);
            break;

        case GAME_ICON_SET:
            lv_game_icon_img = lv_image_create(lv_screen_active());
            lv_obj_align(lv_game_icon_img, LV_ALIGN_RIGHT_MID, 0, 0);
            lv_image_set_src(lv_game_icon_img, &lv_game_icon_dsc);
            break;

        case GAME_ICON_CLEARED:
            lv_obj_delete(lv_game_icon_img);
            break;

        case GAME_ICON_NO_CHANGE:
            break;

        default:
            break;
    }
}

static void update_user_name() {
    if(user_data->display_name_changed) {
        lv_label_set_text(lv_user_name_label, user_data->display_name);
    }
}

static void update_lv_objects() {
    updating_lv_objects = true;
    xSemaphoreTake(user_data->mutex, portMAX_DELAY);
    if(user_data->data_is_ready) {
        update_avatar_icon();
        update_game_icon();
        update_user_name();
        user_data->data_is_ready = false;
    }
    xSemaphoreGive(user_data->mutex);
    updating_lv_objects = false;
}

static bool display_timer_callback() {
    if (!updating_lv_objects) {
        lv_timer_handler();
    }
    return true;
}

void lcd_task(void *pvParameters) {
    lv_init();

    spi_init(spi0, 40000000);
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
    
    lv_color_t *buf1 = NULL;
    lv_color_t *buf2 = NULL;

    uint32_t buf_size = st7796_hor_res * st7796_ver_res / 10 * lv_color_format_get_size(lv_display_get_color_format(lv_st7796));

    buf1 = lv_malloc(buf_size);
    buf2 = lv_malloc(buf_size);

    lv_display_set_buffers(lv_st7796, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_100, 0);
    
    user_data = get_steam_user_data_ptr();
    lv_user_name_label = lv_label_create(lv_screen_active());
    lv_obj_align(lv_user_name_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_avatar_icon_img = lv_image_create(lv_screen_active());
    lv_obj_align(lv_avatar_icon_img, LV_ALIGN_LEFT_MID, 0, 0);
    lv_avatar_icon_dsc.header.cf = LV_COLOR_FORMAT_RAW;
    lv_avatar_icon_dsc.header.w = 64;
    lv_avatar_icon_dsc.header.h = 64;
    lv_avatar_icon_dsc.data = user_data->avatar_icon_jpg;
    lv_avatar_icon_dsc.data_size = (uint32_t)(*(user_data->avatar_icon_size));
    lv_image_set_src(lv_avatar_icon_img, &lv_avatar_icon_dsc);
    
    lv_game_icon_dsc.header.cf = LV_COLOR_FORMAT_RAW;
    lv_game_icon_dsc.header.w = 32;
    lv_game_icon_dsc.header.h = 32;
    lv_game_icon_dsc.data = user_data->game_icon_jpg;
    lv_game_icon_dsc.data_size = (uint32_t)(*(user_data->game_icon_size));

    struct repeating_timer display_timer;
    add_repeating_timer_ms(10, display_timer_callback, NULL, &display_timer);

    while (1) {
        update_lv_objects();
    }
}