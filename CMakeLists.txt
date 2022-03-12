cmake_minimum_required(VERSION 3.21)
project(dclvideoplayer C)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_C_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

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

set(installPath ${CMAKE_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR})
install(TARGETS ${PROJECT_NAME} DESTINATION ${installPath})