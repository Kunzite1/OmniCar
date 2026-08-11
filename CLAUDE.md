# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

OmniCar is the STM32 lower-level controller (下位机) firmware for an omni-directional car. The MCU is an **STM32F407VET6** (LQFP100), and the broader system (see `采购清单.md` / purchase list) pairs it with an RK3562 Linux board (上位机), a laser lidar, and CAN transceivers. At present the firmware is a freshly-generated STM32CubeMX HAL skeleton: only GPIO, RCC, and SysTick are configured — no application logic exists yet in `main.c`.

This repo is mid-migration: it originally used the ST Standard Peripheral Library (`CORE/` + `FWLIB/` at the repo root, now deleted, uncommitted) and is being rebuilt as a CubeMX HAL project under `OmniCar/` (currently untracked).

## Hardware / clock / pins

- MCU: STM32F407VETx, LQFP100. Core board schematic: `资料/STM32F407VET6核心板原理图.pdf`.
- HSE 8 MHz → PLL (M=8, N=336, P=2) → **168 MHz SYSCLK**; APB1 = 42 MHz, APB2 = 84 MHz (see `SystemClock_Config()` in `Core/Src/main.c`).
- Pins: **PA1 = `boardLED`** — open-drain output with pull-up (`GPIO_MODE_OUTPUT_OD`), defined in `Core/Inc/main.h`. This is currently the only user GPIO.

## Directory layout

- `OmniCar/Core/Inc`, `OmniCar/Core/Src` — application code: `main.c`, `gpio.c`, `stm32f4xx_it.c`, `stm32f4xx_hal_msp.c`, `system_stm32f4xx.c`. **This is where user code lives.**
- `OmniCar/Drivers/` — STM32F4xx HAL driver + CMSIS (vendor code; normally not edited).
- `OmniCar/MDK-ARM/` — project/toolchain files: Keil `OmniCar.uvprojx`, EIDE config `.eide/eide.yml`, VSCode workspace + tasks, startup assembly `startup_stm32f407xx.s`.
- `采购清单.md` — hardware purchase list (Chinese). `资料/` — hardware reference docs.

## Build / flash — two toolchains, one codebase

**1. Keil MDK-ARM** — open `OmniCar/MDK-ARM/OmniCar.uvprojx` in µVision (MDK V5.32). Uses ARM Compiler 5 (AC5), target `STM32F407VETx`, output `OmniCar/MDK-ARM/OmniCar/OmniCar.axf`.

**2. VSCode + EIDE (Embedded IDE, `cl.eide` extension)** — the primary CLI-friendly path. Build/flash is driven through EIDE project commands defined in `.vscode/tasks.json`: `build`, `rebuild`, `clean`, `flash`, and `build and flash`. All require:
- ARM Compiler 5 (AC5) — EIDE invokes the ARMCC toolchain installed by Keil; the same AC5 toolchain config lives in `.eide/eide.yml` (`toolchain: AC5`).
- J-Link for `flash`/debug (see `uploadConfigMap.JLink` in `eide.yml`, speed 8000); debugging uses `cortex-debug`.

Output goes to `OmniCar/MDK-ARM/build/` (gitignored).

There is no unit-test framework or CI — this is bare-metal firmware; "running tests" does not apply.

## STM32CubeMX regeneration (important)

- `OmniCar/OmniCar.ioc` is the single source of truth for peripheral config. It was generated with **CubeMX 6.17.0** and the **STM32Cube FW_F4 V1.28.3** HAL package.
- Regenerating code from the `.ioc` **overwrites everything outside the `/* USER CODE BEGIN */ … /* USER CODE END */` blocks** in CubeMX-managed files (`main.c`, `gpio.c`, `stm32f4xx_it.c`, `stm32f4xx_hal_msp.c`, `*.h`). All user logic must live inside those markers.
- New peripherals added in CubeMX appear as `MX_<peripheral>_Init()` functions that must be called from `main()`; headers are included in `main.h`/`main.c` `USER CODE BEGIN Includes` regions.

## Conventions

- Formatting: `.clang-format` in `MDK-ARM/` (Microsoft base, 4-space indent, Linux braces, no column limit); VSCode workspace configures clangd (`--header-insertion=never`) and EIDE for IntelliSense.
- Comments/commit messages in this repo are a mix of Chinese and English — match whatever is nearby. Commit messages follow `vX.Y：中文描述` (e.g. `v0.1：选好了linux板卡、STM32下位机、激光雷达、can收发器`).
