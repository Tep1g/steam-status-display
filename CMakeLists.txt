cmake_minimum_required(VERSION 3.13...3.27)

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(LV_CONF_BUILD_DISABLE_DEMOS 1)
set(LV_CONF_BUILD_DISABLE_EXAMPLES 1)

# Set custom lvgl config path
add_compile_definitions(
    LV_CONF_INCLUDE_SIMPLE
    LV_CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config/lvgl/lv_conf.h"
)

# Pull in SDK (must be before project)
set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/pico-sdk)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Pull in FreeRTOS
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/FreeRTOS-Kernel)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Create an interface library for freertos config
add_library(freertos_config INTERFACE)

# Add the freertos config directory as an include path
target_include_directories(freertos_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/config/freertos)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config/lwip)

# Pull in the LVGL library
add_subdirectory(submodules/lvgl)

project(app C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_subdirectory(src)