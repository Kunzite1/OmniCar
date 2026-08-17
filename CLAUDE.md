# CLAUDE.md

本文件为 Claude Code（claude.ai/code）在本仓库工作时提供的指引，**全文中文**，方便遇到问题时人工检查。

## 项目概述

OmniCar 是全向移动车的 **STM32 下位机**固件。MCU 为 **STM32F407VET6**（LQFP100），通过 CAN 与搭载激光雷达的 **Linux 上位机**通信（上位机板卡型号见 `采购清单.md`，v0.4.1 更换过）。v0.2 提交了五层架构（Core / BSP / Middleware / Motion / App）；**v0.4.2 移植 FreeRTOS（CMSIS-RTOS V2），`App_Loop()` 改由 RTOS 默认任务周期调用**。目前大部分模块仍是只声明接口的 stub；已落地：**`BSP/led`**（boardLED 每 500 ms 翻转一次，v0.3.1）、**`BSP/uart` + `Middleware/log`**（USART2 日志打印串口 + 分级日志中间件，v0.4）、**FreeRTOS 调度器**（v0.4.2）。**v0.6 提交**：v0.5 补入核心板 Altium PCB 设计资料，v0.5.1 定稿电机/CAN/IMU 引脚分配（`引脚分配.md`「待接线」节），v0.5.2 绘制并下单了**模块转接板**（立创 EDA Pro 工程与 Gerber 在 `资料/自制转接板资料/`），**v0.6 进入电机调试并统一工具链**：CubeMX 已配置 TIM3 三路 PWM（PA6/PA7/PB0）+ 6 路方向 GPIO（PE13/14、PA4/5、PD14/15），Windows 端弃用 EIDE 改用同一套 CMake + GCC（µVision/EIDE 配置已删除），`BSP/motor` 仍为 stub（见「硬件」节）。

**CMake + GCC 构建是主工具链**（v0.3 提交）：CubeMX 目标为 `CMake`（`.ioc`：`TargetToolchain=CMake`、`LibraryCopy=1`），供应商 HAL/CMSIS 目录已裁剪到只保留被编译的模块，五层源码已接入 `CMakeLists.txt`。该构建在 Linux / Windows 上均编译链接通过（见下方「构建」）。Windows 端与 Linux 端共用同一套 CMake + GCC 构建；**µVision 与 EIDE 配置均已删除（v0.6）**。

## 硬件 / 时钟 / 引脚

- MCU：STM32F407VETx，LQFP100。`资料/` 分两处：核心板资料在 `资料/STM32F407VET6核心板资料/`（原理图 PDF、PCB 制版图、Altium 设计文件 `STM32_F4VX_M.PrjPcb` 及封装定位图）；模块转接板资料在 `资料/自制转接板资料/`（立创 EDA Pro 工程 `ProPrj_OmniCar转接板_*.epro2`、Gerber `Gerber_PCB1_*.zip`、原理图 PNG）。
- 引脚接线总表：仓库根目录 `引脚分配.md`（已接线的照实填写，未接线的标「待定」）。
- HSE 8 MHz → PLL（M=8, N=336, P=2）→ **168 MHz SYSCLK**；APB1 = 42 MHz，APB2 = 84 MHz（见 `Core/Src/main.c` 的 `SystemClock_Config()`）。
- **时间基准（v0.4.2 起）：SysTick 归 FreeRTOS，HAL 的 `HAL_GetTick()`/`HAL_Delay()` 改由 TIM6 驱动** —— `Core/Src/stm32f4xx_hal_timebase_tim.c` 提供 `HAL_InitTick` 实现，`main.c` 的 `HAL_TIM_PeriodElapsedCallback()` 里对 TIM6 调 `HAL_IncTick()`。`Middleware/log` 时间戳仍取 `HAL_GetTick()`。
- 引脚：**PA1 = `boardLED`** —— 开漏输出带内部上拉（`GPIO_MODE_OUTPUT_OD`），定义于 `Core/Inc/main.h`。目前是唯一的用户 GPIO；**PA2/PA3 = `USART2_TX/RX`**（复用 AF7）为日志打印串口（115200 8N1），已接线（见 `引脚分配.md`）。v0.5.1 已规划全部待接线（见 `引脚分配.md`「待接线」节）。**v0.6 起 CubeMX 已配置 TIM3 PWM（PA6/PA7/PB0，CH1/2/3）+ 6 路方向 GPIO（PE13/14、PA4/5、PD14/15）**，`BSP/motor` 待实现；编码器（TIM1/8/4）、CAN1（PD0/PD1）、IMU I2C1（PB8/PB9）**尚未在 `.ioc` 配置**，接线时需在 CubeMX 中新增。

## 五层架构

应用代码位于 `stm32_proj/` 下，分五层。构建时把 `App/`、`BSP/`、`Middleware/`、`Motion/` 四个目录作为 include 根（见「构建」节的 `OMNICAR_LAYER_INCLUDE_DIRS`），模块用相对路径 include，例如 `#include "App/main/app_main.h"`（`main.c` 就是这么写的）：

- **Core/** —— CubeMX 管理的代码：`main.c`、`gpio.c`、`usart.c`（USART2）、`freertos.c`（RTOS 任务创建）、`stm32f4xx_it.c`、`stm32f4xx_hal_msp.c`、`stm32f4xx_hal_timebase_tim.c`（TIM6 时间基准）、`system_stm32f4xx.c`、`FreeRTOSConfig.h`（RTOS 配置），以及为 GNU 工具链添加的 newlib stub `syscalls.c`/`sysmem.c`。只有 `/* USER CODE BEGIN/END */` 块能在再生成时存活（见下）。
- **BSP/** —— 板级外设驱动：`led`（板载 LED）、`motor`（520 编码电机，PWM + 方向）、`can`（CAN 收发器 ↔ Linux 上位机）、`encoder`（电机转速反馈）、`uart`（日志打印串口）、`ICM20948`（9 轴 IMU，I2C）。`led` 与 `uart` 已实现（其余仍为 stub）；`uart` 内定义了 `_write` 覆盖 `syscalls.c` 的 weak 实现，使标准 `printf()` 也统一走 USART2。
- **Middleware/** —— 可复用服务：`log`（日志分级宏——唯一经 `BSP/uart` 驱动串口的模块；输出级别在编译期由 `LOG_LEVEL` 过滤，默认 `LOG_LEVEL_INFO`，低于该级别的 `LOG_*` 调用被编译剔除）、`can_protocol`（与上位机的 CAN 报文组帧）、`ringbuffer`、`math`（向量 / 四元数）。`FreeRTOS/` 空占位（`.gitkeep`）已无意义——真实 RTOS 源码在 CubeMX 的 `Middlewares/Third_Party/FreeRTOS/`（见下）。
- **Motion/** —— 运动学与控制：`kinematics`（三轮全向：vx, vy, ω → 各轮转速）、`pid`（轮速 PID）、`attitude`（IMU 姿态，互补滤波 / Mahony）、`controller`（指令执行 / 轮速环调度）。
- **App/** —— 应用逻辑与状态机：`main`（入口）、`mode`（工作模式状态机）、`cmd_handler`（上位机指令处理）。

**FreeRTOS（v0.4.2）：** 经 CubeMX 以 CMSIS-RTOS V2 方式接入，源码在 `Middlewares/Third_Party/FreeRTOS/Source/`（含 GCC `ARM_CM4F` port、`heap_4`、`cmsis_os2.c`）。关键配置在 `Core/Inc/FreeRTOSConfig.h`：**堆 32 KB**（`configTOTAL_HEAP_SIZE=32768`）、tick 1000 Hz、`configENABLE_FPU=0`、默认任务栈 256 字（1 KB）。**SysTick 归 RTOS 所有**，HAL 时间基准已切到 TIM6（见「时钟」）。任务创建在 `Core/Src/freertos.c` 的 `MX_FREERTOS_Init()` 里（CubeMX 生成，新增任务写在 `USER CODE BEGIN RTOS_THREADS` 块内）；堆栈溢出 / malloc 失败 hook 均已挂上（`configCHECK_FOR_STACK_OVERFLOW=2`）。

**入口接线：** `Core/Src/main.c`：`HAL_Init()` → `SystemClock_Config()` → `MX_GPIO_Init()` → `MX_USART2_UART_Init()` → `MX_TIM3_Init()` → `App_Init()`（`USER CODE BEGIN 2`）→ `osKernelInitialize()` → `MX_FREERTOS_Init()` → `osKernelStart()`。启动后控制权交给调度器：`freertos.c` 的 `StartDefaultTask()` 里 `for(;;) App_Loop();`。main() 末尾的 `while(1)` 循环在调度器正常启动后不会执行（CubeMX 保留的死代码）；`App_Loop()` 里的延时已改用 `osDelay()`（`app_main.c` include `cmsis_os2.h`）。App 驱动下方各层；**BSP 是唯一直接调用 HAL 外设的层** —— 例外：`Middleware/log` 为取时间戳直接调用 `HAL_GetTick()`，并经由 `BSP/uart` 打印。

每个模块都有手写的头文件，其 doc 注释用中文说明用途（如 `BSP/can/can.h` → 与上位机通信的 CAN 收发驱动）；新增模块时沿用该头文件风格并带 `extern "C"` 守卫。

## 构建

唯一构建路径：**CMake + GCC**（Linux / Windows 通用）。无单元测试框架、无 CI —— 固件以编译通过为主要自动化检查，硬件行为靠手动验证。

### CMake + GCC（主）

CubeMX 生成的 CMake 工程（`cmake_minimum_required 3.22`，C11）。在 `stm32_proj/` 下执行（Makefiles 生成器，`build/` 目录已配置好）：

```
cmake --build build    # 构建 → build/OmniCar.elf（并生成 .bin/.hex）
```

全新检出时先配置一次（Makefiles 生成器）：`cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake`。`CMakePresets.json` 另配了 Ninja 生成器（`cmake --preset Debug`），装了 Ninja 的环境可用，产物在 `build/Debug/`。

- 工具链：**GNU arm-none-eabi**（`cmake/gcc-arm-none-eabi.cmake`；需保证 `arm-none-eabi-*` 在 PATH），Cortex-M4 `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`，Debug `-O0 -g3` / Release `-Os`，链接 `STM32F407xx_FLASH.ld`，`--specs=nano.specs`，`--gc-sections`。另有 `cmake/starm-clang.cmake`（LLVM ST ARM）存在但未被 preset 选用。
- `cmake/stm32cubemx/CMakeLists.txt`（CubeMX 生成）提供源码：Core + `startup_stm32f407xx.s`（根目录的 GCC 启动文件）+ 裁剪后的 HAL 驱动集 + **`FreeRTOS` OBJECT 库**（`FreeRTOS_Src`：`cmsis_os2.c`、`heap_4`、GCC `ARM_CM4F` port 等，v0.4.2 起）。**不要手工改这个文件。**
- **五层（App/BSP/Middleware/Motion）通过 `CMakeLists.txt` 里的 `OMNICAR_LAYER_SOURCES` / `OMNICAR_LAYER_INCLUDE_DIRS` 接入构建**。include 根为项目根 + 四个层目录——这就是 `#include "App/main/app_main.h"` 能解析的原因。**新增模块必须加进 `OMNICAR_LAYER_SOURCES`**。
- include 根顺序重要：项目根排最前，保证 `App/...` 风格的路径优先于层目录里 `App/App/...` 双前缀被解析。
- **烧录/调试用 openocd + ST-Link**（Linux / Windows 通用；Linux 上 `stlink-tools` 的 `st-flash` 有 `libusb_set_option` 符号错误，已损坏，勿用）。`stm32_proj/.vscode/` 下有 `tasks.json`（`build`、`flash`、`clean`）和 `launch.json`（cortex-debug + openocd）；VSCode 工作区入口是 `stm32_proj/OmniCar.code-workspace`。命令行烧录等价：
  `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/OmniCar.elf verify reset exit"`
  CMake 构建默认只产 `build/OmniCar.elf`，`POST_BUILD` 的 objcopy 步骤还会生成 `.bin`/`.hex`。
- **工作约定：Claude 不执行烧录操作**（openocd program / st-flash / VSCode `flash` 任务一律不代跑），只负责 `cmake --build` 编译与排错；编译通过即交付，烧录由用户手动完成（VSCode `flash` 任务或上述命令行）。

### Windows 端（CMake + GCC，与 Linux 相同）

Windows 端与 Linux 端共用 `stm32_proj/` 下**同一套 CMake 工程**，命令完全一致（见上），无独立构建配置。实测可行的安装路径（v0.6 验证）：

- 装 **MSYS2**（例如 D 盘），在 **UCRT64 终端**里 `pacman -S mingw-w64-ucrt-x86_64-arm-none-eabi-gcc mingw-w64-ucrt-x86_64-cmake make mingw-w64-ucrt-x86_64-openocd`，一次装齐工具链。
- 把 `<msys>/ucrt64/bin` 与 `<msys>/usr/bin` 加进**用户** PATH（放在最前）。
- **不要另装 Program Files 的 CMake 4.4+**：其 `-DCMAKE_TOOLCHAIN_FILE=…x.cmake` 会把 `.cmake` 后缀拆掉导致配置失败（PowerShell 下必现，v0.6 实测）。因系统 PATH 优先于用户 PATH，若系统里已有高版本 CMake，卸载或从 PATH 移除（`winget uninstall Kitware.CMake`）。
- **ST-Link 驱动 STSW-LINK009**（ST 官网）——Windows 特有，**不装则 openocd 认不到 ST-Link**。

PowerShell 命令与 Linux 相同：`cmake -B build -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake` → `cmake --build build` → `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/OmniCar.elf verify reset exit"`。VSCode 打开 `stm32_proj/OmniCar.code-workspace`，`build`/`flash`/`clean` 任务即用。

**µVision 与 EIDE 配置均已删除（v0.6）** —— 不要同步、不要建议使用。他人要用源码请自行配置。

### 双端开发配置同步（工作约定）

Linux / Windows 共用 `stm32_proj/` 下同一份 `CMakeLists.txt`、`.ioc`、`.vscode` 与五层源码，**不存在两套并行构建配置**，因此没有「同步另一端」的动作：

- 新增/删除模块源文件 → 只需改 `CMakeLists.txt` 的 `OMNICAR_LAYER_SOURCES`。
- 修改 `tasks.json` / `launch.json` / 烧录参数 → 两端共用同一份，改动即两端生效。
- include 根（项目根 + `App/`/`BSP/`/`Middleware/`/`Motion/`）在 `OMNICAR_LAYER_INCLUDE_DIRS` 统一定义。
- CubeMX 再生成在任一平台执行，源码经 git 同步到另一端即可，无额外同步。

## STM32CubeMX 再生成（重要）

- `stm32_proj/OmniCar.ioc` 是外设配置的唯一真相。它由 **CubeMX 6.18.1** 和 **STM32Cube FW_F4 V1.28.3** HAL 包生成。当前配置了 GPIO / RCC / **FREERTOS（CMSIS_V2）** / **TIM6（HAL 时间基准）** / **TIM3（电机 PWM）** / USART2（日志串口）/ SWD 调试（PA13/PA14）。
- `.ioc` 现在是 `ProjectManager.TargetToolchain=CMake`、`ProjectManager.LibraryCopy=1`（HAL 源码拷入工程）。**再生成会还原被裁剪的完整 HAL/CMSIS 源码树**（被删的文件会以修改/未跟踪形式回来 —— 预期行为）。
- 从 `.ioc` 再生成会**覆盖 CubeMX 管理文件中 `/* USER CODE BEGIN */ … /* USER CODE END */` 块之外的一切**（`main.c`、`gpio.c`、`stm32f4xx_it.c`、`stm32f4xx_hal_msp.c`、`*.h`）。所有用户逻辑必须写在块内。
- 在 CubeMX 新增外设后，`main()` 中会出现 `MX_<外设>_Init()` 函数且必须调用；头文件在 `main.h`/`main.c` 的 `USER CODE BEGIN Includes` 区域 include。
- FreeRTOS 侧同样受 CubeMX 管理：**新增任务 / 改堆大小 / 改 hook 在 CubeMX 的 FREERTOS 中间件里配置，或把代码写在 `freertos.c` / `FreeRTOSConfig.h` 的 `USER CODE` 块内**；再生成会把这些文件还原成完整版。
- `.gitignore` 把 `Drivers/CMSIS/` 裁剪到只保留构建所需部分（`Include/`、`Device/ST/STM32F4xx/`）——预期行为，不要把其余部分提交。

## 约定

- 格式化：`.clang-format` 位于 `stm32_proj/`（Microsoft 基础，4 空格缩进，Linux 大括号风格，无列宽限制）；IntelliSense 用 C/C++ 扩展读取 `build/compile_commands.json`（见 `stm32_proj/.vscode/settings.json`）。
- 注释/提交消息中英文混排 —— 跟着附近内容保持一致。提交消息遵循 `vX.Y：中文描述`（如 `v0.2：迁移到CubeMX HAL工程，搭建五层架构骨架`）。
- 根目录另有 **`AGENTS.md`**（仓库级规范：构建命令、编码/命名约定、提交与 PR 指南、人工验证要求），与本文件互为补充；两者冲突时以本文件为准。
- 遇到问题时以本文件为第一参考；Claude 工作时的约束（不代跑烧录、两端共用同一套 CMake、µVision/EIDE 已删除）都在上面写明。
- `README.md` 是给人类读者的进度/引脚/注意速览（详细规则以本文件为准）；涉及这些内容时两端同步更新。
