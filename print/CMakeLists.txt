cmake_minimum_required(VERSION 3.13)
include(../utils.cmake)
project(print C)

set(CMAKE_C_STANDARD 11)

create_meal_library(PRIVATE math macros platform)