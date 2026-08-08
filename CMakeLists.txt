# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
  set(USERHOME $ENV{USERPROFILE})
else()
  set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)

if (EXISTS ${picoVscode})
  include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_drawlib C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Collect all sources under Src/, then strip out:
#   - pico_canlib/  (built as a separate library above)
#   - steering_wheel_main.cpp  (standalone entry point, superseded by merged main.cpp)
file(GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
)
list(FILTER SRC_FILES EXCLUDE REGEX ".*/pico_canlib/.*")
list(FILTER SRC_FILES EXCLUDE REGEX ".*steering_wheel_main\\.cpp$")

add_library(pico_drawlib STATIC ${SRC_FILES})


# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico_drawlib 1)

target_include_directories(pico_drawlib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Inc
    ${CMAKE_CURRENT_LIST_DIR}/Src/mcufont_decoder
    # Src/pico_canlib/Inc is inherited via pico_canlib's PUBLIC include dirs
)

target_link_libraries(pico_drawlib PUBLIC
    pico_stdlib
    hardware_spi
    hardware_uart
    hardware_timer
    hardware_adc
    pico_canlib
    pico_multicore 
)

