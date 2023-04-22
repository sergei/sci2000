# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# Set path to ESP32 library obtained from https://github.com/UncleRus/esp-idf-lib
set(EXTRA_COMPONENT_DIRS
        ../idf-components
        )

execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DGIT_HASH=\"${GIT_HASH}\"")

include($ENV{IDF_PATH}/tools/cmake/project.cmake )
project(imu2nmea)
