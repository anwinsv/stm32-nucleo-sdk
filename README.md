# stm32-nucleo-sdk
SDK for STM32 Nucleo-G491RE integrated with FreeRTOS

# STM32G4 Enterprise FreeRTOS SDK

> **Note:** This project is currently a work in progress.

## Overview
This repository contains a custom, high-performance Software Development Kit (SDK) for the **STM32G491RE** (Cortex-M4F @ 170MHz). 

Hardware peripherals are driven via direct register manipulation (CMSIS) and wrapped seamlessly into **FreeRTOS** synchronization primitives. 

The driver architecture heavily mirrors enterprise SOC patterns, strictly decoupling the generic OS abstraction layer from the low-level silicon IP. This ensures maximum portability, thread safety, and execution speed.

## Tech Stack
* **Microcontroller:** STM32G491RE (NUCLEO-G491RE)
* **Operating System:** FreeRTOS (Custom Port, `heap_4.c`)
* **Build System:** CMake + Ninja
* **Toolchain:** `arm-none-eabi-gcc`
* **Hardware Access:** CMSIS (Direct Register Manipulation)

## Repository Architecture
The repository isolates vendor code from the OS and custom logic to allow for painless third-party updates.

## How to build?
Ensure cmake, ninja, and the ARM GCC toolchain are in your system PATH.

1. Configure the Build System:
cmake -G Ninja -B build "-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake"

2. Compile the Firmware:
ninja -C build

3. Flash:
Flash the generated build/nucleo-g491.elf (or .bin / .hex) via STM32CubeProgrammer or OpenOCD.

