cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME weather-station)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# !!! LV_CONF_PATH and LV_BUILD_CONF_PATH is set in `cmake/gcc-arm-none-eabi.cmake`

set(CONFIG_LV_BUILD_DEMOS OFF)
set(CONFIG_LV_BUILD_EXAMPLES OFF)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF)

# Dodaj podprojekt LVGL (katalog 'lvgl' w repo)
# To zbuduje target biblioteczny 'lvgl'
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdParty/mirrors/lvgl)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/epd3in7_driver.c
    Core/Src/epd3in7_lvgl_adapter.c
    Core/Src/lv_font_opensans_thin_14.c
    Core/Src/lv_font_opensans_regular_16.c
    Core/Src/lv_font_opensans_regular_24.c
    Core/Src/lv_font_opensans_bold_numbers_72.c
    Core/Src/renderer.c
    Core/Src/bmpxx80.c
    Core/Src/bat_nimh_calc.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/3rdParty/inc
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lvgl
    # Add user defined libraries
)

# LVGL FLASH/RAM optimization
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -Wl,--gc-sections)

# float in printf/snprintf
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -u _printf_float)