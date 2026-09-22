# Repository Guidelines

## Project Structure & Module Organization

`OmniCar` is a firmware and ROS 2 monorepo. The active firmware is `stm32f103rct6_proj/`, targeting STM32F103RCT6; `stm32_proj/` is the retired STM32F407VET6 implementation and should only be used for migration reference. ROS 2 packages belong under `ros2_ws/src/`. Do not commit generated `build/`, `install/`, or `log/` directories.

Hand-written firmware follows four layers: `App/` owns the main loop and state handling, `Motion/` contains control algorithms, `Middleware/` provides reusable services, and `BSP/` owns HAL-facing drivers. `Core/`, `Drivers/`, `Middlewares/`, `startup_stm32f103xe.s`, and `cmake/stm32cubemx/` are CubeMX or vendor managed. Keep dependencies flowing downward and put generated-file edits inside `/* USER CODE BEGIN */` blocks. Register new hand-written sources in `stm32f103rct6_proj/CMakeLists.txt` under `OMNICAR_LAYER_SOURCES`.

## Build, Test, and Development Commands

Run firmware commands from `stm32f103rct6_proj/`:

```sh
./32build.sh Debug --clean
cmake --preset Debug
cmake --build --preset Debug
./32flash.sh Debug --adapter-speed 100
```

The build requires CMake, Ninja, and `arm-none-eabi-*`; outputs are under `build/Debug/`. Flash only when the task explicitly requests hardware programming and the artifact has been reviewed. The flash script uses OpenOCD, ST-Link, verification, and reset.

## Coding Style & Naming Conventions

Firmware is C11. Use four-space indentation, Linux-style braces, nearby Chinese/English comment style, lowercase paired `.c/.h` filenames, and module-prefixed APIs such as `BSP_LED_Init()` and `App_Loop()`. Follow the formatting rules in `stm32_proj/.clang-format` until the active project gains its own copy. Include headers from the project root, for example `#include "BSP/led/led.h"`, and preserve documented headers plus `extern "C"` guards.

## Testing Guidelines

There is no firmware unit-test framework or CI. A clean cross-compile is the required automated check. For hardware changes, record the wiring, expected behavior, observed serial output, and any unverified physical behavior. Run package-specific `colcon build` and tests for ROS 2 changes.

## Diagnostic Evidence & Reporting

Reports for builds, flashing, serial/CAN tests, and hardware detection must include the exact command, the relevant output, and what that output proves. Trim unrelated noise, distinguish direct observations from inference, and state what was not tested or changed; do not report only “passed” or “detected.” For K1 Mini USB-CAN checks, include evidence from `lsusb`, `lsusb -t`, `udevadm info --query=property --name=/dev/ttyACM0`, `ip -details link show type can`, and `fuser -v /dev/ttyACM0` so CDC serial devices are not confused with SocketCAN interfaces.

## Commit & Pull Request Guidelines

Use focused, action-oriented Chinese subjects; scoped prefixes such as `docs:` are allowed when requested. Do not add manual version prefixes like `v0.7.7`. Pull requests should identify affected layers, CubeMX changes, build results, hardware observations, and remaining unverified items.
