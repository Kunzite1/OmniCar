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
set(CMAKE_PROJECT_NAME stm32f103rct6_proj)

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

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# ----------------------------------------------------------------------------
# OmniCar application layers (App / BSP / Middleware / Motion)
# Keep the project root as an include root for includes such as
# `#include "App/main/app_main.h"`.
# ----------------------------------------------------------------------------
set(OMNICAR_LAYER_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/App
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware
    ${CMAKE_CURRENT_SOURCE_DIR}/Motion
)

set(OMNICAR_LAYER_SOURCES
    # App
    ${CMAKE_CURRENT_SOURCE_DIR}/App/main/app_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/mode/app_mode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/cmd_handler/app_cmd_handler.c
    # BSP
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/can/can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/encoder/encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/ICM20948/icm20948.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/led/led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/motor/motor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/uart/uart.c
    # Middleware
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/can_protocol/can_protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/log/log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/math/math.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/ringbuffer/ringbuffer.c
    # Motion
    ${CMAKE_CURRENT_SOURCE_DIR}/Motion/attitude/attitude.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Motion/controller/controller.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Motion/kinematics/kinematics.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Motion/pid/pid.c
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${OMNICAR_LAYER_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${OMNICAR_LAYER_INCLUDE_DIRS}
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

    # Add user defined libraries
)

# Generate images used by programmers after linking.
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Generating HEX/BIN images"
)
