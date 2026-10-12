cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(mouse_jiggler C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(mouse_jiggler
    src/main.c
    src/usb_descriptors.c
    src/bootsel_button.c
)

pico_generate_pio_header(mouse_jiggler ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

target_include_directories(mouse_jiggler PRIVATE src)

target_link_libraries(mouse_jiggler
    pico_stdlib
    hardware_pio
    hardware_sync
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(mouse_jiggler)
