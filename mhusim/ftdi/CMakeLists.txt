cmake_minimum_required(VERSION 3.14)
project(ftdi)

set(CMAKE_CXX_STANDARD 14)

# Finde required packages
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )
find_package(FT4222 REQUIRED)

include_directories(inc)

set (INCS
        inc/i2cm.h
        )

set (SRCS
        src/i2cm.c
        src/main.c
        )

set(LIBS
        ${FT4222_LIBRARY}
        )

add_executable(mhusim
        ${SRCS}
        ${INCS}
        )

target_link_libraries(mhusim ${LIBS})
target_include_directories(mhusim SYSTEM PUBLIC /usr/local/include/)
