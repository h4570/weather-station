cmake_minimum_required(VERSION 3.22)

project(shared C)

add_library(shared STATIC src/shared_test.c)

target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)