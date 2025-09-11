#include "st7796.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "lvgl.h"

static struct ST7796 {
        lv_display_t *disp;
        spi_inst_t *spi;
        int cs_gpio;
        int dcx_gpio;
        int rst_gpio;
        int dma_channel;
        uint dma_irq_index;
        dma_channel_config *dma_config;
        bool bus_busy;
} st7796;

/* Send short command to the LCD. This function shall wait until the transaction finishes. */
static void st7796_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size)
{
        LV_UNUSED(disp);
        while(st7796.bus_busy);
        st7796.bus_busy = true;

        gpio_put(st7796.dcx_gpio, 0);
        gpio_put(st7796.cs_gpio, 0);
        spi_set_format(st7796.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        int ret = spi_write_blocking(st7796.spi, cmd, cmd_size);
        if (ret == (int)cmd_size) {
                gpio_put(st7796.dcx_gpio, 1); 
                spi_write_blocking(st7796.spi, param, param_size);
                gpio_put(st7796.cs_gpio, 1);
        }
        st7796.bus_busy = false;
}

/* Send large array of pixel data to the LCD. If necessary, this function has to do the byte-swapping. This function can do the transfer in the background. */
static void st7796_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size)
{
        LV_UNUSED(disp);
        while(st7796.bus_busy);
        st7796.bus_busy = true;
        
        gpio_put(st7796.cs_gpio, 0);
        gpio_put(st7796.dcx_gpio, 0);

        spi_set_format(st7796.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        int ret = spi_write_blocking(st7796.spi, cmd, cmd_size);
        if (ret == (int)cmd_size) {
                gpio_put(st7796.dcx_gpio, 1);
                spi_set_format(st7796.spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
                dma_channel_configure(
                        st7796.dma_channel,
                        st7796.dma_config,
                        &spi_get_hw(st7796.spi)->dr,
                        param,
                        (uint)(param_size / 2),
                        true
                );
        }
}

void st7796_send_color_callback()
{
        if (dma_irqn_get_channel_status(st7796.dma_irq_index, st7796.dma_channel)) {
                dma_irqn_acknowledge_channel(st7796.dma_irq_index, st7796.dma_channel);
                gpio_put(st7796.cs_gpio, 1);
                st7796.bus_busy = false;
                lv_display_flush_ready(st7796.disp);
        }
}

lv_disp_t *st7796_init(
        uint32_t st7796_hor_res, 
        uint32_t st7796_ver_res, 
        lv_lcd_flag_t st7796_flag, 
        spi_inst_t *spi, 
        int cs_gpio, 
        int dcx_gpio, 
        int rst_gpio, 
        int dma_channel, 
        uint dma_irq_index, 
        dma_channel_config *dma_config
) {
        st7796.spi = spi;
        st7796.cs_gpio = cs_gpio;
        st7796.dcx_gpio = dcx_gpio;
        st7796.rst_gpio = rst_gpio;
        st7796.dma_channel = dma_channel;
        st7796.dma_irq_index = dma_irq_index;
        st7796.dma_config = dma_config;
        st7796.bus_busy = false;

        // Reset the LCD
        gpio_put(st7796.cs_gpio, 0);
        gpio_put(st7796.dcx_gpio, 1);

        gpio_put(st7796.rst_gpio, 1);
        sleep_ms(100);
        gpio_put(st7796.rst_gpio, 0);
        sleep_ms(100);
        gpio_put(st7796.rst_gpio, 1);
        sleep_ms(100);

        gpio_put(st7796.cs_gpio, 1);
        sleep_ms(10);

        st7796.disp = lv_st7796_create(st7796_hor_res, st7796_ver_res, st7796_flag, st7796_send_cmd, st7796_send_color);

        return st7796.disp;
}