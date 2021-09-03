cmake_minimum_required(VERSION 3.17)
project(yadfa)

set(CMAKE_CXX_STANDARD 14)

add_executable(yadfa
        yadfa.cpp
        tests.cpp
        driver.cpp)
