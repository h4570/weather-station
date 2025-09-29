cmake_minimum_required(VERSION 3.22)
project(shared C)

# Option A: headers-only
add_library(shared INTERFACE)
target_include_directories(shared INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Option B: kod wspólny jako STATIC (jeśli masz .c)
# add_library(shared STATIC src/foo.c src/bar.c)
# target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)