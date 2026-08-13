# Repository Guidelines

## Project Structure & Module Organization

Firmware lives under `OmniCar/` and targets the STM32F407VET6. Hand-written code follows a layered layout: `App/` contains the main loop and state handling, `Motion/` contains control and kinematics, `Middleware/` provides reusable services, and `BSP/` owns hardware-facing drivers. `Core/`, `Drivers/`, `startup_stm32f407xx.s`, and `cmake/stm32cubemx/` are STM32CubeMX or vendor-managed. Hardware references are in `资料/`, while pin assignments are documented in `引脚分配.md`.

Keep application dependencies flowing downward; BSP is the layer that directly calls HAL peripherals. Put CubeMX-managed edits inside `/* USER CODE BEGIN */` blocks so regeneration preserves them. When adding a module, register its source in `OmniCar/CMakeLists.txt` and mirror it in `OmniCar/MDK-ARM/.eide/eide.yml`.

## Build, Test, and Development Commands

Run commands from `OmniCar/`:

```sh
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build
cmake --build build --target clean
```

The build requires `arm-none-eabi-*` tools and produces `build/OmniCar.elf`, `.bin`, and `.hex`. The checked-in presets use Ninja; the explicit configure command works with the host's default generator. Do not automate flashing: contributors should use the VS Code `flash` task or OpenOCD manually after reviewing the artifact.

## Coding Style & Naming Conventions

Code is C11. Follow `OmniCar/MDK-ARM/.clang-format`: four-space indentation, Linux-style braces, and no fixed column limit. Match nearby Chinese/English comments. Use module-prefixed public APIs such as `BSP_LED_Init()` and `App_Loop()`, lowercase module filenames, and paired `.c`/`.h` files. Include project headers from a layer root, for example `#include "BSP/led/led.h"`. Preserve the existing documented header and `extern "C"` guard style.

## Testing Guidelines

There is currently no unit-test framework or CI. Treat a clean cross-compile as the required automated check. For hardware changes, document manual board verification—peripheral, wiring, expected behavior, and observed result—in the pull request. Never commit generated `build/` output.

## Commit & Pull Request Guidelines

History uses versioned Chinese subjects such as `v0.4：实现 USART2 日志打印串口与日志分级中间件`; follow `vX.Y[.Z]：简明描述`. Keep commits focused. Pull requests should summarize affected layers, mention CubeMX or dual-toolchain configuration changes, link relevant issues, and list build and hardware-test results. Include logs or screenshots only when they clarify device behavior.
