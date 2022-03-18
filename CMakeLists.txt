cmake_minimum_required(VERSION 3.21)
project(dclvideoplayer C)

set(CMAKE_C_STANDARD 17)

option(STATIC_LIB "Build only static library" OFF)
set(installPath ${CMAKE_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (MSVC)
    message(MSVC Detected)
    set(CMAKE_C_FLAGS "/MT")
    set(CMAKE_C_FLAGS_RELEASE "/MT")
    set(CMAKE_C_FLAGS_DEBUG "/MTd")
endif()

if(STATIC_LIB)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()
message(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++")
else()
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -lpthread -lm -lz")
endif()

add_library(${PROJECT_NAME} SHARED src/decoder.c src/player.c src/framequeue.c)

if (STATIC_LIB)
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

    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::LIBAV)
else()
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
    find_library(AVCODEC_LIBRARY avcodec)

    find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
    find_library(AVFORMAT_LIBRARY avformat)

    find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
    find_library(AVUTIL_LIBRARY avutil)

    find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
    find_library(AVDEVICE_LIBRARY avdevice)

    find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
    find_library(SWRESAMPLE_LIBRARY swresample)

    find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
    find_library(SWSCALE_LIBRARY swscale)

    set(AVLIBRARIES ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${SWRESAMPLE_LIBRARY} ${SWSCALE_LIBRARY})

    target_include_directories(${PROJECT_NAME} PUBLIC ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${AVLIBRARIES})

    if (WIN32)
        file(GLOB AVDLLS
                "${CMAKE_PREFIX_PATH}/bin/*.dll"
        )
        message(AVDLLS ${AVDLLS})
        install(FILES ${AVDLLS} DESTINATION ${installPath})
    else()
        set (resolvedLibraries "")
        foreach (_file ${AVLIBRARIES})
            get_filename_component(_resolvedFile "${_file}" REALPATH)
            list (APPEND resolvedLibraries "${_resolvedFile}")
        endforeach()
        install(FILES ${resolvedLibraries} DESTINATION ${installPath})
    endif()
endif()

add_executable(${PROJECT_NAME}Test src/main.c)
target_link_libraries(${PROJECT_NAME}Test PRIVATE ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION ${installPath})