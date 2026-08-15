# OmniCar 全向移动车（STM32 下位机固件）

全向移动车的 **STM32 下位机**固件工程。MCU 为 **STM32F407VET6**（LQFP100，168 MHz），经 **CAN** 与搭载激光雷达的 **Linux 上位机**（RK3562）通信。当前进度：**v0.6（电机调试）**。

> 面向人类读者的进度/注意/引脚速览。详细架构、构建、CubeMX 再生成规则等见 [`CLAUDE.md`](CLAUDE.md)。

## 项目进度

| 版本          | 内容                                                                          |
| ----------- | --------------------------------------------------------------------------- |
| v0.0–v0.1   | 选型：linux 板卡、STM32 核心板、激光雷达、CAN 收发器（见 `采购清单.md`）                             |
| v0.2        | 迁移到 CubeMX HAL 工程，搭建五层架构骨架                                                  |
| v0.3–v0.3.1 | 接入 **CMake + GCC** 主工具链；实现 **`BSP/led`**（板载 LED 每 500 ms 翻转）                |
| v0.4        | 实现 **`BSP/uart` + `Middleware/log`**（USART2 日志打印串口 + 分级日志，115200 8N1）       |
| v0.4.2      | 移植 **FreeRTOS**（CMSIS-RTOS V2），`App_Loop()` 由默认任务周期调用；时间基准由 SysTick 切到 TIM6 |
| v0.5        | 补入核心板 Altium PCB 设计资料                                                       |
| v0.5.1      | 定稿电机 / CAN / IMU **引脚分配**（见下）                                               |
| v0.5.2      | 绘制并下单**模块转接板**（立创 EDA Pro，Gerber 在 `资料/自制转接板资料/`）                           |
| v0.5.3      | 工具链统一为 **CMake + GCC**（Windows 弃用并删除 EIDE）；CubeMX 配置电机 **TIM3 PWM + 方向 GPIO**，进入电机调试   |

**已实现**：`BSP/led`、`BSP/uart` + `Middleware/log`、FreeRTOS 调度器。**仍为 stub**：`motor` / `can` / `encoder` / `ICM20948` 驱动，`can_protocol` / `kinematics` / `pid` / `attitude` / `controller` / 模式状态机 / 指令处理。

## 硬件与引脚分配

时钟：HSE 8 MHz → PLL → **168 MHz SYSCLK**（APB1 42 MHz / APB2 84 MHz）。

### 已接线

| 模块              | MCU 引脚    | 信号                        | 说明                    |
| --------------- | --------- | ------------------------- | --------------------- |
| 板载 LED          | PA1       | `boardLED`（开漏输出）          | 低电平点亮                 |
| 日志串口（USB 转 TTL） | PA2 / PA3 | `USART2_TX` / `USART2_RX` | **115200** 8N1，无流控，共地 |

### 待接线（规划已定，v0.5.1 定稿）

| 模块                | 功能        | MCU 引脚          | 复用/资源                       |
| ----------------- | --------- | --------------- | --------------------------- |
| 电机 1（520 编码电机）    | PWM       | **PA6**         | TIM3_CH1                    |
|                   | 编码器 A / B | **PE9 / PE11**  | TIM1_CH1 / CH2              |
|                   | 方向 A / B  | **PE13 / PE14** | GPIO 输出                     |
| 电机 2              | PWM       | **PA7**         | TIM3_CH2                    |
|                   | 编码器 A / B | **PC6 / PC7**   | TIM8_CH1 / CH2              |
|                   | 方向 A / B  | **PA4 / PA5**   | GPIO 输出                     |
| 电机 3              | PWM       | **PB0**         | TIM3_CH3                    |
|                   | 编码器 A / B | **PD12 / PD13** | TIM4_CH1 / CH2              |
|                   | 方向 A / B  | **PD14 / PD15** | GPIO 输出                     |
| CAN ↔ 上位机（RK3562） | RX / TX   | **PD0 / PD1**   | CAN1（AF9），需外接收发器（如 TJA1050） |
| ICM20948 九轴 IMU   | SCL / SDA | **PB8 / PB9**   | I2C1（AF4）                   |

接线约定：**模块 TX → 单片机 RX，模块 RX → 单片机 TX**。三路电机 PWM 共用 TIM3（CH1/2/3），编码器各用独立定时器。

> ⚠️ **TIM3 PWM + 6 路方向 GPIO 已配置（v0.5.3）**；编码器（TIM1/8/4）、CAN1、I2C1 **尚未在 CubeMX `.ioc` 中配置**——接线时需在 CubeMX 新增。

## 构建

工具链为 **CMake + GCC**（Linux / Windows 通用），CubeMX 生成 CMake 工程（`.ioc`：`TargetToolchain=CMake`）。Windows 端不再使用 EIDE。

```sh
cd OmniCar
# 全新检出先配置一次（Makefiles 生成器；Windows 必须显式 -G "Unix Makefiles"，Linux 可省）
cmake -B build -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
# 构建 → build/OmniCar.elf/.bin/.hex
cmake --build build
```

无单元测试、无 CI，**编译通过即视为主要自动化检查**；硬件行为靠手动验证。

### Windows 端：工具安装与构建烧录

Windows 上与 Linux 使用同一套 CMake 工程，命令一致。**推荐装 MSYS2 一次性装齐**（v0.5.3 实测）：

1. 装 **MSYS2**（如 D 盘），打开 **MSYS2 UCRT64** 终端执行：
   `pacman -S mingw-w64-ucrt-x86_64-arm-none-eabi-gcc mingw-w64-ucrt-x86_64-cmake make mingw-w64-ucrt-x86_64-openocd`
2. 把 `<msys>\ucrt64\bin` 与 `<msys>\usr\bin` 加进**用户** PATH（放最前）。
3. **别另装 Program Files 的 CMake 4.4+**——它会把 `-DCMAKE_TOOLCHAIN_FILE=…x.cmake` 的 `.cmake` 拆掉导致配置失败（系统 PATH 优先于用户 PATH，会被它抢先）；系统里已有就卸载：`winget uninstall Kitware.CMake`。
4. **ST-Link 驱动 STSW-LINK009**（ST 官网）——Windows 不装则 openocd 认不到 ST-Link。

装完验证一条链（PowerShell）：

```powershell
cmake --version; arm-none-eabi-gcc --version; make --version; openocd --version
```

构建与烧录（与 Linux 命令完全一致）：

```sh
cmake --build build
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/OmniCar.elf verify reset exit"
```

或用 VSCode 打开 `OmniCar/OmniCar.code-workspace`，运行 `build` / `flash` / `clean` 任务（终端 → 运行任务）。µVision 与 EIDE 配置均已删除。

## 注意事项

- **烧录由人工执行**：VSCode `flash` 任务（`OmniCar/`，openocd + ST-Link），或命令行 `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/OmniCar.elf verify reset exit"`。
- **µVision 工程与 EIDE 配置均已删除**——Windows 端与 Linux 共用同一套 CMake + GCC。
- **CubeMX 再生成**会覆盖 `/* USER CODE BEGIN */ … /* USER CODE END */` 块之外的一切（`main.c`、`gpio.c`、`freertos.c`、`*.h`），用户逻辑必须写在块内。
- **时间基准**：SysTick 归 FreeRTOS，HAL 的 `HAL_GetTick()`/`HAL_Delay()` 由 TIM6 驱动。
- 日志打印串口为 **USART2**（PA2/PA3），日志级别由 `LOG_LEVEL` 编译期过滤（默认 `LOG_LEVEL_INFO`）。
- 资料目录：核心板设计文件在 `资料/STM32F407VET6核心板资料/`，模块转接板工程/Gerber 在 `资料/自制转接板资料/`。
