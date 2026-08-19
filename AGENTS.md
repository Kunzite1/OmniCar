# Repository Guidelines

## Project Structure & Module Organization

This is the `OmniCar` monorepo. STM32 firmware lives under `stm32_proj/` and targets the STM32F407VET6. ROS 2 packages belong under `ros2_ws/src/`; never commit the generated `ros2_ws/build/`, `ros2_ws/install/`, or `ros2_ws/log/` directories. Repository-level documentation and hardware references remain at the root, including `资料/`, `引脚分配.md`, and `采购清单.md`.

The firmware's hand-written code follows a layered layout: `App/` contains the main loop and state handling, `Motion/` contains control and kinematics, `Middleware/` provides reusable services, and `BSP/` owns hardware-facing drivers. `Core/`, `Drivers/`, `startup_stm32f407xx.s`, and `cmake/stm32cubemx/` are STM32CubeMX or vendor-managed.

Keep application dependencies flowing downward; BSP is the layer that directly calls HAL peripherals. Put CubeMX-managed edits inside `/* USER CODE BEGIN */` blocks so regeneration preserves them. When adding a firmware module, register its source in `stm32_proj/CMakeLists.txt` (the `OMNICAR_LAYER_SOURCES` list).

## Build, Test, and Development Commands

Run firmware commands from `stm32_proj/`:

```sh
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build
cmake --build build --target clean
```

The build requires `arm-none-eabi-*` tools and produces `build/OmniCar.elf`, `.bin`, and `.hex`. The checked-in presets use Ninja; the explicit configure command works with the host's default generator. Do not automate flashing: contributors should use the VS Code `flash` task or OpenOCD manually after reviewing the artifact.

## Coding Style & Naming Conventions

Firmware code is C11. Follow `stm32_proj/.clang-format`: four-space indentation, Linux-style braces, and no fixed column limit. Match nearby Chinese/English comments. Use module-prefixed public APIs such as `BSP_LED_Init()` and `App_Loop()`, lowercase module filenames, and paired `.c`/`.h` files. Include project headers from a layer root, for example `#include "BSP/led/led.h"`. Preserve the existing documented header and `extern "C"` guard style. Follow each ROS 2 package's language conventions and keep package dependencies declared in its manifest.

## Testing Guidelines

There is currently no unit-test framework or CI. Treat a clean firmware cross-compile as the required automated check. When ROS 2 packages are present, run their relevant `colcon build` and tests. For hardware changes, document manual board verification—peripheral, wiring, expected behavior, and observed result—in the pull request. Never commit generated firmware or ROS 2 build output.

## Commit & Pull Request Guidelines

History uses versioned Chinese subjects such as `v0.4：实现 USART2 日志打印串口与日志分级中间件`; follow `vX.Y[.Z]：简明描述`. Keep commits focused. Pull requests should summarize affected layers, mention CubeMX or dual-toolchain configuration changes, link relevant issues, and list build and hardware-test results. Include logs or screenshots only when they clarify device behavior.

## Human Learning Notes

Maintain `资料/agent汇报.md` alongside substantive repository work, hardware diagnostics, builds, or configuration investigations. Write it in concise Chinese for a human learning embedded development—not as an internal agent log. Each dated entry should state the outcome first, then cover what was done, how it was determined, the commands actually used, verification results, and the next unresolved step.

Group commands by execution environment (development host, K1 Mini, STM32 tools), and explain what important output means instead of pasting full logs. Clearly distinguish read-only inspection from file changes, device configuration, builds, and flashing. Never record passwords, tokens, private keys, or other secrets. Link relevant repository files when useful, avoid repeated background material, and update an existing entry when continuing the same task rather than creating fragmented notes.
