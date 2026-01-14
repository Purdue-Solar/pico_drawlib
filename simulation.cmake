message(STATUS "ITS TIME TO SIMULATE.")

project(pico_canlib C CXX ASM)
file(GLOB_RECURSE SRC_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Src/*.cpp"
    "${CMAKE_SOURCE_DIR}/Src/*.c"

    "${CMAKE_SOURCE_DIR}/Sim/*.c"
    "${CMAKE_SOURCE_DIR}/Src/*.cpp"
)
# find_package(raylib REQUIRED)

add_compile_options(-I/usr/local/include)
find_package(raylib REQUIRED)

add_executable(pico_canlib ${SRC_FILES})
target_include_directories(pico_canlib PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/Inc"
    "${CMAKE_CURRENT_LIST_DIR}/Sim"
)
target_link_libraries(pico_canlib PRIVATE raylib)
target_compile_definitions(pico_canlib PRIVATE SIMULATION)
