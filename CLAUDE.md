# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

OmniCar is the STM32 lower-level controller (下位机) firmware for an omni-directional car. The MCU is an **STM32F407VET6** (LQFP100), paired via CAN with an RK3562 Linux board (上位机) that carries a laser lidar. A five-layer architecture skeleton (Core / BSP / Middleware / Motion / App) was committed in v0.2; every layer module is still a stub declaring its intended interface.

**The repo is mid-migration to a CMake + GCC build** (uncommitted working-tree changes on top of v0.2): CubeMX now targets `CMake` (`.ioc`: `TargetToolchain=CMake`, `LibraryCopy=1`), a CMake build system + GNU toolchain file + linker script + GCC startup file were added, the vendor HAL/CMSIS tree was trimmed to only the compiled modules, and the five-layer sources are wired into `CMakeLists.txt`. The build is verified to compile and link cleanly on the Linux host (see Build below).

## Hardware / clock / pins

- MCU: STM32F407VETx, LQFP100. Core board schematic: `资料/STM32F407VET6核心板原理图.pdf`.
- HSE 8 MHz → PLL (M=8, N=336, P=2) → **168 MHz SYSCLK**; APB1 = 42 MHz, APB2 = 84 MHz (see `SystemClock_Config()` in `Core/Src/main.c`).
- Pins: **PA1 = `boardLED`** — open-drain output with pull-up (`GPIO_MODE_OUTPUT_OD`), defined in `Core/Inc/main.h`. This is currently the only user GPIO.

## Five-layer architecture

The application is split into five layers under `OmniCar/`. The build adds `App/`, `BSP/`, `Middleware/`, `Motion/` as include roots (in the Keil project), so modules are included by that relative path — e.g. `#include "App/main/app_main.h"` (as `main.c` does):

- **Core/** — CubeMX-managed HAL code: `main.c`, `gpio.c`, `stm32f4xx_it.c`, `stm32f4xx_hal_msp.c`, `system_stm32f4xx.c`, plus newlib stubs `syscalls.c`/`sysmem.c` (added for the GNU toolchain). Only the `/* USER CODE BEGIN/END */` blocks survive regeneration (see below).
- **BSP/** — board-support drivers wrapping peripherals: `led` (board LED), `motor` (520 encoded motor, PWM + direction), `can` (CAN transceiver ↔ RK3562), `encoder` (motor speed feedback), `uart` (log serial), `ICM20948` (9-axis IMU, I2C).
- **Middleware/** — hardware-independent services: `can_protocol` (CAN message framing for RK3562), `ringbuffer`, `math` (vector / quaternion). `FreeRTOS/` is an empty placeholder (`.gitkeep`), not yet integrated.
- **Motion/** — kinematics + control: `kinematics` (3-wheel omni: vx, vy, ω → per-wheel speed), `pid` (wheel-speed PID), `attitude` (IMU attitude, complementary filter / Mahony), `controller` (command execution / wheel-speed loop scheduling).
- **App/** — application logic / state machines: `main` (entry), `mode` (work-mode state machine), `cmd_handler` (upper-machine command handling).

**Entry-point wiring:** `Core/Src/main.c` (inside `USER CODE` blocks) includes `App/main/app_main.h`, calls `App_Init()` after `MX_GPIO_Init()`, then calls `App_Loop()` on every iteration of the `while(1)` loop. App drives the layers below; BSP is the only layer that talks to HAL peripherals directly.

Each module has a hand-written header whose doc comment states its purpose in Chinese (e.g. `BSP/can/can.h` → 与 RK3562 通信的 CAN 收发驱动); follow this header style and the `extern "C"` guard when adding new modules.

## Build

Two toolchains exist; the project is moving to CMake. There is no unit-test framework or CI — this is bare-metal firmware; "running tests" does not apply.

### CMake + GCC (new, primary — work in progress)

CubeMX-generated CMake build (`cmake_minimum_required 3.22`, C11, Ninja). Run from `OmniCar/`:

```
cmake --preset Debug          # configure → build/Debug (toolchain cmake/gcc-arm-none-eabi.cmake)
cmake --build --preset Debug  # builds build/Debug/OmniCar.elf
```

- Toolchain: **GNU arm-none-eabi** (`cmake/gcc-arm-none-eabi.cmake`; `arm-none-eabi-*` must be on PATH), Cortex-M4 `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`, Debug `-O0 -g3` / Release `-Os`, links against `STM32F407xx_FLASH.ld` with `--specs=nano.specs`, `--gc-sections`. An alternative `cmake/starm-clang.cmake` (LLVM ST ARM) is present but not selected by the presets.
- `cmake/stm32cubemx/CMakeLists.txt` (CubeMX-generated) supplies the sources: Core + `startup_stm32f407xx.s` (the root-level GCC startup) + a trimmed HAL driver set. Do not hand-edit this file.
- **The five layers (App/BSP/Middleware/Motion) are wired into the build** via `OMNICAR_LAYER_SOURCES` / `OMNICAR_LAYER_INCLUDE_DIRS` in `CMakeLists.txt`. Include roots are the project root **plus** the four layer dirs (mirroring EIDE's `..`/`../App` etc.) — this is why `#include "App/main/app_main.h"` resolves. **New modules must be added to `OMNICAR_LAYER_SOURCES`** (and to the Keil/EIDE project separately).
- The include-root order matters: the project root comes first, so `App/...`-style paths resolve before the layer dirs' `App/App/...` double-prefix would be tried.
- **Flashing/debug on Linux uses openocd + ST-Link** (the `stlink-tools` 1.7.0 `st-flash` binary is currently broken on this host — `libusb_set_option` symbol error). `OmniCar/.vscode/` has `tasks.json` (`build`, `flash`, `clean`) and `launch.json` (cortex-debug + openocd). Flash CLI equivalent:
  `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/OmniCar.elf verify reset exit"`.
  The CMake build only produces `build/OmniCar.elf` unless the `POST_BUILD` objcopy step runs — it now generates `.bin`/`.hex` too.

### Keil MDK-ARM (legacy, still functional)

Open `OmniCar/MDK-ARM/OmniCar.uvprojx` in µVision (MDK V5.32). ARM Compiler 5 (AC5), target `STM32F407VETx`, output `OmniCar/MDK-ARM/OmniCar/OmniCar.axf`. The project compiles Core + `App/main` (the entry) + the trimmed HAL set, using `MDK-ARM/startup_stm32f407xx.s` (the AC5 startup, distinct from the root GCC one).

The VSCode+EIDE project (`MDK-ARM/.vscode/tasks.json`: `build`, `rebuild`, `clean`, `flash`, `build and flash`) also still works and is the current path for **flashing** — `eide.yml` uploader is **ST-Link** (SWD, base `0x08000000`, speed 4000); debugging uses `cortex-debug`. Output goes to `MDK-ARM/build/` (gitignored).

## STM32CubeMX regeneration (important)

- `OmniCar/OmniCar.ioc` is the single source of truth for peripheral config. It was generated with **CubeMX 6.17.0** and the **STM32Cube FW_F4 V1.28.3** HAL package. Currently only GPIO / RCC / SysTick are configured.
- The `.ioc` now has `ProjectManager.TargetToolchain=CMake` and `ProjectManager.LibraryCopy=1` (HAL sources copied into the project). **Regenerating restores the full HAL/CMSIS source tree** that was trimmed out (those deleted files come back as modified/untracked — expected).
- Regenerating code from the `.ioc` **overwrites everything outside the `/* USER CODE BEGIN */ … /* USER CODE END */` blocks** in CubeMX-managed files (`main.c`, `gpio.c`, `stm32f4xx_it.c`, `stm32f4xx_hal_msp.c`, `*.h`). All user logic must live inside those markers.
- New peripherals added in CubeMX appear as `MX_<peripheral>_Init()` functions that must be called from `main()`; headers are included in `main.h`/`main.c` `USER CODE BEGIN Includes` regions.
- `.gitignore` prunes `Drivers/CMSIS/` down to only the build-needed parts (`Include/`, `Device/ST/STM32F4xx/`) — expected, don't commit the rest.

## Conventions

- Formatting: `.clang-format` in `MDK-ARM/` (Microsoft base, 4-space indent, Linux braces, no column limit); the Keil VSCode workspace configures clangd (`--header-insertion=never`) and EIDE for IntelliSense.
- Comments/commit messages are a mix of Chinese and English — match whatever is nearby. Commit messages follow `vX.Y：中文描述` (e.g. `v0.2：迁移到CubeMX HAL工程，搭建五层架构骨架`).
