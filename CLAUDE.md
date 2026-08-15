# CLAUDE.md

本文件为 Claude Code（claude.ai/code）在本仓库工作时提供的指引，**全文中文**，方便遇到问题时人工检查。

## 项目概述

OmniCar 是全向移动车的 **STM32 下位机**固件。MCU 为 **STM32F407VET6**（LQFP100），通过 CAN 与搭载激光雷达的 **Linux 上位机**通信（上位机板卡型号见 `采购清单.md`，v0.4.1 更换过）。v0.2 提交了五层架构（Core / BSP / Middleware / Motion / App）；**v0.4.2 移植 FreeRTOS（CMSIS-RTOS V2），`App_Loop()` 改由 RTOS 默认任务周期调用**。目前大部分模块仍是只声明接口的 stub；已落地：**`BSP/led`**（boardLED 每 500 ms 翻转一次，v0.3.1）、**`BSP/uart` + `Middleware/log`**（USART2 日志打印串口 + 分级日志中间件，v0.4）、**FreeRTOS 调度器**（v0.4.2）。**最新 v0.5** 未改动代码，仅补入核心板 Altium PCB 设计资料（见「硬件」节）。

**CMake + GCC 构建是主工具链**（v0.3 提交）：CubeMX 目标为 `CMake`（`.ioc`：`TargetToolchain=CMake`、`LibraryCopy=1`），供应商 HAL/CMSIS 目录已裁剪到只保留被编译的模块，五层源码已接入 `CMakeLists.txt`。该构建在 Linux 主机上编译链接通过（见下方「构建」）。**µVision 工程（`MDK-ARM/OmniCar.uvprojx`）不维护** —— Windows 端只用 VSCode + EIDE。

## 硬件 / 时钟 / 引脚

- MCU：STM32F407VETx，LQFP100。核心板原理图：`资料/STM32F407VET6核心板原理图.pdf`；v0.5 又补入该核心板的 Altium PCB 设计文件（`STM32_F4VX_M.PrjPcb` 及 PCB 制版图、封装定位图等，同在 `资料/`）。
- 引脚接线总表：仓库根目录 `引脚分配.md`（已接线的照实填写，未接线的标「待定」）。
- HSE 8 MHz → PLL（M=8, N=336, P=2）→ **168 MHz SYSCLK**；APB1 = 42 MHz，APB2 = 84 MHz（见 `Core/Src/main.c` 的 `SystemClock_Config()`）。
- **时间基准（v0.4.2 起）：SysTick 归 FreeRTOS，HAL 的 `HAL_GetTick()`/`HAL_Delay()` 改由 TIM6 驱动** —— `Core/Src/stm32f4xx_hal_timebase_tim.c` 提供 `HAL_InitTick` 实现，`main.c` 的 `HAL_TIM_PeriodElapsedCallback()` 里对 TIM6 调 `HAL_IncTick()`。`Middleware/log` 时间戳仍取 `HAL_GetTick()`。
- 引脚：**PA1 = `boardLED`** —— 开漏输出带内部上拉（`GPIO_MODE_OUTPUT_OD`），定义于 `Core/Inc/main.h`。目前是唯一的用户 GPIO；**PA2/PA3 = `USART2_TX/RX`**（复用 AF7）为日志打印串口（115200 8N1），已接线（见 `引脚分配.md`）。

## 五层架构

应用代码位于 `OmniCar/` 下，分五层。构建时把 `App/`、`BSP/`、`Middleware/`、`Motion/` 四个目录作为 include 根（EIDE 工程同样镜像），模块用相对路径 include，例如 `#include "App/main/app_main.h"`（`main.c` 就是这么写的）：

- **Core/** —— CubeMX 管理的代码：`main.c`、`gpio.c`、`usart.c`（USART2）、`freertos.c`（RTOS 任务创建）、`stm32f4xx_it.c`、`stm32f4xx_hal_msp.c`、`stm32f4xx_hal_timebase_tim.c`（TIM6 时间基准）、`system_stm32f4xx.c`、`FreeRTOSConfig.h`（RTOS 配置），以及为 GNU 工具链添加的 newlib stub `syscalls.c`/`sysmem.c`。只有 `/* USER CODE BEGIN/END */` 块能在再生成时存活（见下）。
- **BSP/** —— 板级外设驱动：`led`（板载 LED）、`motor`（520 编码电机，PWM + 方向）、`can`（CAN 收发器 ↔ Linux 上位机）、`encoder`（电机转速反馈）、`uart`（日志打印串口）、`ICM20948`（9 轴 IMU，I2C）。`led` 与 `uart` 已实现（其余仍为 stub）；`uart` 内定义了 `_write` 覆盖 `syscalls.c` 的 weak 实现，使标准 `printf()` 也统一走 USART2。
- **Middleware/** —— 可复用服务：`log`（日志分级宏——唯一经 `BSP/uart` 驱动串口的模块；输出级别在编译期由 `LOG_LEVEL` 过滤，默认 `LOG_LEVEL_INFO`，低于该级别的 `LOG_*` 调用被编译剔除）、`can_protocol`（与上位机的 CAN 报文组帧）、`ringbuffer`、`math`（向量 / 四元数）。`FreeRTOS/` 空占位（`.gitkeep`）已无意义——真实 RTOS 源码在 CubeMX 的 `Middlewares/Third_Party/FreeRTOS/`（见下）。
- **Motion/** —— 运动学与控制：`kinematics`（三轮全向：vx, vy, ω → 各轮转速）、`pid`（轮速 PID）、`attitude`（IMU 姿态，互补滤波 / Mahony）、`controller`（指令执行 / 轮速环调度）。
- **App/** —— 应用逻辑与状态机：`main`（入口）、`mode`（工作模式状态机）、`cmd_handler`（上位机指令处理）。

**FreeRTOS（v0.4.2）：** 经 CubeMX 以 CMSIS-RTOS V2 方式接入，源码在 `Middlewares/Third_Party/FreeRTOS/Source/`（含 GCC `ARM_CM4F` 与 RVDS `ARM_CM4F` 双 port、`heap_4`、`cmsis_os2.c`）。关键配置在 `Core/Inc/FreeRTOSConfig.h`：**堆 32 KB**（`configTOTAL_HEAP_SIZE=32768`）、tick 1000 Hz、`configENABLE_FPU=0`、默认任务栈 256 字（1 KB）。**SysTick 归 RTOS 所有**，HAL 时间基准已切到 TIM6（见「时钟」）。任务创建在 `Core/Src/freertos.c` 的 `MX_FREERTOS_Init()` 里（CubeMX 生成，新增任务写在 `USER CODE BEGIN RTOS_THREADS` 块内）；堆栈溢出 / malloc 失败 hook 均已挂上（`configCHECK_FOR_STACK_OVERFLOW=2`）。

**入口接线：** `Core/Src/main.c`：`HAL_Init()` → `SystemClock_Config()` → `MX_GPIO_Init()` → `MX_USART2_UART_Init()` → `App_Init()`（`USER CODE BEGIN 2`）→ `osKernelInitialize()` → `MX_FREERTOS_Init()` → `osKernelStart()`。启动后控制权交给调度器：`freertos.c` 的 `StartDefaultTask()` 里 `for(;;) App_Loop();`。main() 末尾的 `while(1)` 循环在调度器正常启动后不会执行（CubeMX 保留的死代码）；`App_Loop()` 里的延时已改用 `osDelay()`（`app_main.c` include `cmsis_os2.h`）。App 驱动下方各层；**BSP 是唯一直接调用 HAL 外设的层** —— 例外：`Middleware/log` 为取时间戳直接调用 `HAL_GetTick()`，并经由 `BSP/uart` 打印。

每个模块都有手写的头文件，其 doc 注释用中文说明用途（如 `BSP/can/can.h` → 与上位机通信的 CAN 收发驱动）；新增模块时沿用该头文件风格并带 `extern "C"` 守卫。

## 构建

有两条构建路径：**Linux 端 CMake + GCC（主）** 与 **Windows 端 VSCode + EIDE**。无单元测试框架、无 CI —— 固件以编译通过为主要自动化检查，硬件行为靠手动验证。

### CMake + GCC（主）

CubeMX 生成的 CMake 工程（`cmake_minimum_required 3.22`，C11）。在 `OmniCar/` 下执行（Makefiles 生成器，`build/` 目录已配置好）：

```
cmake --build build    # 构建 → build/OmniCar.elf（并生成 .bin/.hex）
```

全新检出时先配置一次（Makefiles 生成器）：`cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake`（`cmake --preset Debug` 依赖 Ninja，本机未安装，勿用；`CMakePresets.json` 已配好 Ninja + 工具链，供装了 Ninja 的环境用）。

- 工具链：**GNU arm-none-eabi**（`cmake/gcc-arm-none-eabi.cmake`；需保证 `arm-none-eabi-*` 在 PATH），Cortex-M4 `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`，Debug `-O0 -g3` / Release `-Os`，链接 `STM32F407xx_FLASH.ld`，`--specs=nano.specs`，`--gc-sections`。另有 `cmake/starm-clang.cmake`（LLVM ST ARM）存在但未被 preset 选用。
- `cmake/stm32cubemx/CMakeLists.txt`（CubeMX 生成）提供源码：Core + `startup_stm32f407xx.s`（根目录的 GCC 启动文件）+ 裁剪后的 HAL 驱动集 + **`FreeRTOS` OBJECT 库**（`FreeRTOS_Src`：`cmsis_os2.c`、`heap_4`、GCC `ARM_CM4F` port 等，v0.4.2 起）。**不要手工改这个文件。**
- **五层（App/BSP/Middleware/Motion）通过 `CMakeLists.txt` 里的 `OMNICAR_LAYER_SOURCES` / `OMNICAR_LAYER_INCLUDE_DIRS` 接入构建**。include 根为项目根 + 四个层目录（与 EIDE 的 `..`/`../App` 等一致）——这就是 `#include "App/main/app_main.h"` 能解析的原因。**新增模块必须加进 `OMNICAR_LAYER_SOURCES`**（并同步 EIDE 的 `eide.yml`）。
- include 根顺序重要：项目根排最前，保证 `App/...` 风格的路径优先于层目录里 `App/App/...` 双前缀被解析。
- **Linux 上烧录/调试用 openocd + ST-Link**（本机 `stlink-tools` 1.7.0 的 `st-flash` 有 `libusb_set_option` 符号错误，已损坏）。`OmniCar/.vscode/` 下有 `tasks.json`（`build`、`flash`、`clean`）和 `launch.json`（cortex-debug + openocd）；VSCode 工作区入口是 `OmniCar/OmniCar.code-workspace`。命令行烧录等价：
  `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/OmniCar.elf verify reset exit"`
  CMake 构建默认只产 `build/OmniCar.elf`，`POST_BUILD` 的 objcopy 步骤还会生成 `.bin`/`.hex`。
- **工作约定：Claude 不执行烧录操作**（openocd program / st-flash / VSCode `flash` 任务一律不代跑），只负责 `cmake --build` 编译与排错；编译通过即交付，烧录由用户手动完成（VSCode `flash` 任务或上述命令行）。

### VSCode + EIDE（Windows 端）

Windows 端唯一工具是 **VSCode + EIDE**（`MDK-ARM/.vscode/tasks.json`：`build`/`rebuild`/`clean`/`flash`/`build and flash`）。`eide.yml` 的 uploader 是 **ST-Link**（SWD，base `0x08000000`，speed 4000）；调试用 `cortex-debug`。输出在 `MDK-ARM/build/`（已 gitignore）。**EIDE 用的编译器是 Keil AC5**（`eide.yml`：`toolchain: AC5`，FreeRTOS 走 `RVDS/ARM_CM4F` port）——µVision 工程虽不维护，但 Windows 端实际以 Keil 工具链编译，**新增代码须同时兼容 GCC 与 AC5**（C99）。

**µVision（`MDK-ARM/OmniCar.uvprojx`）本项目不使用、不维护、不提及** —— 不要管它、不要同步它、不要建议用它。他人要用源码请自行配置或参考 EIDE 配置教程。

### 双端开发配置同步（工作约定）

工程维护两套并列的开发/构建配置，共用同一份五层源码：

- **Linux 端**：`OmniCar/.vscode/`（CMake + GCC，tasks：`build`/`flash`/`clean`，openocd + ST-Link 烧录）。
- **Windows 端**：`MDK-ARM/.vscode/` + `MDK-ARM/.eide/eide.yml`（VSCode + EIDE，tasks：`build`/`rebuild`/`clean`/`flash`/`build and flash`，ST-Link 烧录）。

约定：**在任一端开发时，凡是会影响另一端的改动，都要同步更新另一端配置**，不要只改一端：

- 新增/删除模块源文件 → 同步修改 `CMakeLists.txt` 的 `OMNICAR_LAYER_SOURCES` 与 `MDK-ARM/.eide/eide.yml` 的 `virtualFolder`（µVision 工程不维护，无需同步）。
- 修改 `tasks.json` / `launch.json` / 烧录参数 → 将改动镜像到另一端的 `.vscode/`（两端工具不同，功能不一致处注明取舍）。
- 两端的 include 根保持一致：项目根 + `App/`/`BSP/`/`Middleware/`/`Motion/`。

## STM32CubeMX 再生成（重要）

- `OmniCar/OmniCar.ioc` 是外设配置的唯一真相。它由 **CubeMX 6.18.1** 和 **STM32Cube FW_F4 V1.28.3** HAL 包生成。当前配置了 GPIO / RCC / **FREERTOS（CMSIS_V2）** / **TIM6（HAL 时间基准）** / USART2（日志串口）/ SWD 调试（PA13/PA14）。
- `.ioc` 现在是 `ProjectManager.TargetToolchain=CMake`、`ProjectManager.LibraryCopy=1`（HAL 源码拷入工程）。**再生成会还原被裁剪的完整 HAL/CMSIS 源码树**（被删的文件会以修改/未跟踪形式回来 —— 预期行为）。
- 从 `.ioc` 再生成会**覆盖 CubeMX 管理文件中 `/* USER CODE BEGIN */ … /* USER CODE END */` 块之外的一切**（`main.c`、`gpio.c`、`stm32f4xx_it.c`、`stm32f4xx_hal_msp.c`、`*.h`）。所有用户逻辑必须写在块内。
- 在 CubeMX 新增外设后，`main()` 中会出现 `MX_<外设>_Init()` 函数且必须调用；头文件在 `main.h`/`main.c` 的 `USER CODE BEGIN Includes` 区域 include。
- FreeRTOS 侧同样受 CubeMX 管理：**新增任务 / 改堆大小 / 改 hook 在 CubeMX 的 FREERTOS 中间件里配置，或把代码写在 `freertos.c` / `FreeRTOSConfig.h` 的 `USER CODE` 块内**；再生成会把这些文件还原成完整版。
- `.gitignore` 把 `Drivers/CMSIS/` 裁剪到只保留构建所需部分（`Include/`、`Device/ST/STM32F4xx/`）——预期行为，不要把其余部分提交。

## 约定

- 格式化：`.clang-format` 位于 `MDK-ARM/`（Microsoft 基础，4 空格缩进，Linux 大括号风格，无列宽限制）；EIDE 的 VSCode 工作区（`MDK-ARM/.vscode/`）配置了 clangd（`--header-insertion=never`）和 EIDE 用于 IntelliSense。
- 注释/提交消息中英文混排 —— 跟着附近内容保持一致。提交消息遵循 `vX.Y：中文描述`（如 `v0.2：迁移到CubeMX HAL工程，搭建五层架构骨架`）。
- 根目录另有 **`AGENTS.md`**（仓库级规范：构建命令、编码/命名约定、提交与 PR 指南、人工验证要求），与本文件互为补充；两者冲突时以本文件为准。
- 遇到问题时以本文件为第一参考；Claude 工作时的约束（不代跑烧录、双端同步、µVision 不维护）都在上面写明。
