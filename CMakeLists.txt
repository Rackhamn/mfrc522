cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

project(RFID_Reader C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(rfid_reader
        src/test.c
	src/mfrc522.c
)
pico_enable_stdio_usb(rfid_reader 1)
pico_enable_stdio_uart(rfid_reader 1)

pico_add_extra_outputs(rfid_reader)

target_link_libraries(rfid_reader
        pico_stdlib
        # pico_cyw42_arch_none
        hardware_spi
        hardware_gpio
)

