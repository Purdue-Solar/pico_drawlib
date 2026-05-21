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

cmake_minimum_required(VERSION 3.12)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Must come before project()
include(pico_sdk_import.cmake)

project(artemis26-steering-wheel C CXX ASM)

# Must come after project()
pico_sdk_init()

# ---------------------------------------------------------------------------
# pico_canlib — pulled in from the submodule.
# add_subdirectory() runs lib/pico_canlib/CMakeLists.txt, which defines the
# pico_canlib target. Because we are a parent project, the standalone guard
# in that file skips its project()/pico_sdk_init() calls automatically.
# ---------------------------------------------------------------------------
add_subdirectory(lib/pico_canlib)
# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(artemis26-steering-wheel
    main.cpp
    src/accelerator.cpp
    src/matrix.cpp
)

pico_set_program_name(artemis26-steering-wheel "artemis26-steering-wheel")
pico_set_program_version(artemis26-steering-wheel "0.1")

pico_enable_stdio_uart(artemis26-steering-wheel 0)
pico_enable_stdio_usb(artemis26-steering-wheel 1)

# PRIVATE: inc/ is only for this target, not propagated to anything that
# links against it (nothing does, since this is the top-level executable).
target_include_directories(artemis26-steering-wheel PRIVATE
    inc/
)

# Linking pico_canlib here automatically pulls in:
#   - lib/pico_canlib/Inc/ (from pico_canlib's PUBLIC include)
#   - pico_stdlib and hardware_spi (from pico_canlib's PUBLIC link libraries)
target_link_libraries(artemis26-steering-wheel PRIVATE
    pico_canlib
    pico_stdlib
    hardware_adc
)

pico_add_extra_outputs(artemis26-steering-wheel)
