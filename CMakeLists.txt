cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_live_chip_tester C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall -Wextra
        -O4
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(pico_live_chip_tester
               pico_live_chip_tester.c
               )

target_link_libraries(pico_live_chip_tester PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_pwm
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(pico_live_chip_tester 1)
pico_enable_stdio_uart(pico_live_chip_tester 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(pico_live_chip_tester)

# add url
pico_set_program_url(pico_live_chip_tester "https://blog.qiqitori.com/")

pico_generate_pio_header(pico_live_chip_tester ${CMAKE_CURRENT_LIST_DIR}/pico_live_chip_tester.pio)

target_sources(pico_live_chip_tester PRIVATE pico_live_chip_tester.c)

pico_add_extra_outputs(pico_live_chip_tester)
