cmake_minimum_required(VERSION 3.13)
include(../utils.cmake)
project(basket C)

set(CMAKE_C_STANDARD 11)

create_meal_library(PUBLIC def alloc PRIVATE assert list_pool memory macros math)