cmake_minimum_required(VERSION 4.1)
project(tdcf_test)

set(CMAKE_CXX_STANDARD 17)

find_package(tdcf REQUIRED)

add_executable(tdcf_test main.cpp
        Matrix.hpp)
