cmake_minimum_required(VERSION 3.21)
project(dclvideoplayer C)

set(CMAKE_C_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

add_library(${PROJECT_NAME} SHARED src/decoder.c src/player.c)

target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::LIBAV)

add_executable(${PROJECT_NAME}Test src/main.c)
target_link_libraries(${PROJECT_NAME}Test PRIVATE ${PROJECT_NAME})