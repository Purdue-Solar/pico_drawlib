# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(artemis26-steering-wheel C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(artemis26-steering-wheel "main.c" "xl2515.c" )

pico_set_program_name(artemis26-steering-wheel "artemis26-steering-wheel")
pico_set_program_version(artemis26-steering-wheel "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(artemis26-steering-wheel 0)
pico_enable_stdio_usb(artemis26-steering-wheel 1)

# Add the standard library to the build
target_link_libraries(artemis26-steering-wheel
        pico_stdlib
        hardware_spi)

# Add the standard include files to the build
target_include_directories(artemis26-steering-wheel PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(artemis26-steering-wheel)

