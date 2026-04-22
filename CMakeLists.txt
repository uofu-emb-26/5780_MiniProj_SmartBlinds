cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32_uofu)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(gcc-arm-none-eabi)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})


# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Add STM32Cube libraries
add_subdirectory(Drivers)

include(flash_stm32)

# Smart Blinds executable
add_executable(smart_blinds
    Core/Src/main.c
    Core/Src/motor.c
    Core/Src/adc.c
    Core/Src/uart.c
    Core/Src/lcd.c
)

target_link_libraries(smart_blinds PRIVATE
    STM32_Drivers
    STM32_Discovery
    SEGGER_RTT
    ${TOOLCHAIN_LINK_LIBRARIES}
)

# Flash target: build and flash via `cmake --build build --target flash_smart_blinds`
flash_target(smart_blinds)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
