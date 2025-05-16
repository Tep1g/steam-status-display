set(SRC_FILES
    "main.c"
    "lcd_task/st7796/st7796.c"
    "lcd_task/lcd.c"
    "{CMAKE_CURRENT_SOURCE_DIR}/../submodules/tiny-json/tiny-json.c"
)

set(HEADER_DIRS 
    "lcd_task/st7796"
    "lcd_task"
    "{CMAKE_CURRENT_SOURCE_DIR}/../submodules/tiny-json/tiny-json.h"
)

add_executable(app ${SRC_FILES})

target_include_directories(app PRIVATE ${HEADER_DIRS})

# pull in common dependencies
target_link_libraries(app
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_spi
    hardware_dma
    hardware_irq
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    freertos_config
    lvgl
    lwipopts
)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(app)

# enable usb output, disable uart output
pico_enable_stdio_usb(app 1)
pico_enable_stdio_uart(app 0)