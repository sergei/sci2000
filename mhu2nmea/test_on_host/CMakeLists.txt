cmake_minimum_required(VERSION 3.26)
project(test_on_host)

set(CMAKE_CXX_STANDARD 17)

add_executable(test_on_host
        ../main/AWAComputer.cpp
        ../main/AWAComputer.h
        ../main/LowPassFilter.cpp
        ../main/LowPassFilter.h

        main.cpp
)
