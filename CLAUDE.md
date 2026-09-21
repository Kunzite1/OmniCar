# CLAUDE.md

本文件为 Claude Code 在 OmniCar 仓库工作时提供当前工程上下文。仓库级约束以根目录 `AGENTS.md` 为准；本文件补充硬件、生成代码和板级验证信息。

## 项目入口

- `stm32f103rct6_proj/`：当前 STM32 下位机固件，目标为 STM32F103RCT6（Cortex-M3、256 KiB Flash、48 KiB SRAM）。
- `stm32_proj/`：已停用的 STM32F407VET6 固件，仅用于追溯旧实现和迁移差异，不在其中继续开发新功能。
- `ros2_ws/`：KICKPI K1 Mini 上运行的 ROS 2 工作区；上位机计划经 USB-CAN/SocketCAN 与下位机通信。
- `资料/`：旧核心板、转接板和其他硬件资料。
- `docs/引脚分配.md`：旧 F407 转接板引脚表；当前 F103 引脚以 `stm32f103rct6_proj/README.md` 和 `.ioc` 为准。
- `docker/`：K1 Mini 的 ROS Humble 容器方案。

不要提交 `stm32f103rct6_proj/build/`，也不要提交 `ros2_ws/build/`、`install/` 或 `log/`。

## 当前固件状态

截至 2026-09-21，F407 到 F103 的第一阶段迁移已完成。当前实现包括 LED、USART3 异步日志、TIM3 三路 PWM、电机安全停止、CAN1 收发与协议骨架、FreeRTOS CMSIS-RTOS V2，以及 I2C1 的 CubeMX/HAL 初始化。

上电后默认任务每秒翻转 PA8 LED 并发送 CAN ID `0x101` 心跳；CAN 指令任务接收 `0x2FF` 后以 `0x2FE` echo。日志任务每 200 ms 批量输出，默认等级为 INFO。编码器、ICM20948 数据驱动、PID、姿态、控制器和速度闭环仍为骨架或待实现状态。

I2C1 已配置为 PB6/PB7、100 kHz、7 位地址、允许时钟拉伸，未启用 DMA 或中断。`BSP/ICM20948/icm20948.c/.h` 已加入构建，但没有设备地址探测、`WHO_AM_I` 或数据读取，不能把 HAL 初始化成功等同于传感器通信成功。

最近一次板级复测结果：Debug 构建通过，Flash 使用 35,560 B，RAM 使用 22,168 B；ST-Link/OpenOCD 在约 3.25 V 目标电压下完成编程、校验和复位；CH340 COM8 以 115200 8N1 收到 11 条完整启动日志，时钟、USART3、TIM3、CAN、应用和 default/log/CAN 三个任务均成功启动，观察窗口内无 WARN/ERROR。ICM20948、PWM 波形、CAN 物理链路和电机负载行为仍待专门验证。

## 硬件与引脚

- HSE 8 MHz，SYSCLK/HCLK 72 MHz，APB1 36 MHz，APB2 72 MHz。
- PA8：板载 LED，推挽输出，高电平点亮。
- PB10/PB11：USART3 TX/RX，115200 8N1。
- PB6/PB7：I2C1 SCL/SDA；需要 4.7 kΩ 上拉到 3.3 V，且不能与板载屏幕同时使用。
- PA11/PA12：CAN1 RX/TX，500 kbit/s；同时连接板载 Type-C DM/DP，CAN 工作时不要连接 Type-C 数据线。
- PC6/PC7/PC8：TIM3 CH1/2/3 全重映射，20 kHz。
- PC4/PC5、PB12/PB13、PB14/PB15：三路电机方向 GPIO。
- PA2/PA3：未使用的 Flash/TF 片选，保持高电平。
- PA13/PA14：Serial Wire，禁止改为 `No Debug`。

当前系统板原理图为 `stm32f103rct6_proj/docs/双TypeCF103RCT6原理图.pdf`。

## 软件结构

活动固件保持分层依赖：`App -> Motion/Middleware -> BSP -> HAL`。

- `App/`：应用初始化、主循环、模式和 CAN 指令分发。
- `Motion/`：运动学、PID、姿态和控制器。
- `Middleware/`：日志、CAN 协议、数学和环形缓冲服务。
- `BSP/`：LED、UART、CAN、电机、编码器和 ICM20948 板级驱动；BSP 是常规业务代码直接调用 HAL 外设的层。
- `Core/`：CubeMX 生成的入口、GPIO、CAN、I2C、TIM、USART、中断和 FreeRTOS 接线。
- `Drivers/`、`Middlewares/`：STM32CubeF1 HAL、CMSIS 和 FreeRTOS 第三方源码。

手写源文件通过顶层 `CMakeLists.txt` 的 `OMNICAR_LAYER_SOURCES` 接入；新增模块必须同步登记。项目根和四个手写层目录是 include 根，使用 `#include "App/main/app_main.h"` 一类路径。

FreeRTOS 使用 GCC `ARM_CM3` port、`heap_4` 和 16 KiB heap。SysTick 供调度器使用，HAL tick 由 TIM6 提供。`main()` 依次初始化 GPIO、USART3、TIM3、CAN1、I2C1，再初始化内核并调用 `MX_FREERTOS_Init()`；`App_Init()` 和三个任务在该阶段创建，随后启动调度器。

## 构建、烧录与日志

从 `stm32f103rct6_proj/` 执行：

```sh
./32build.sh Debug --clean
./32build.sh Release --jobs 8
cmake --preset Debug
cmake --build --preset Debug
```

产物位于 `build/Debug/stm32f103rct6_proj.{elf,hex,bin}`。固件没有单元测试或 CI，干净交叉编译是最低自动化检查。

仅在任务明确要求烧录且已检查产物时执行：

```sh
./32flash.sh Debug --adapter-speed 100
```

脚本使用 `interface/stlink.cfg`、`target/stm32f1x.cfg`，执行 program、verify、reset。串口日志使用 USART3 115200 8N1；硬件验证需记录端口、接线、预期结果、实际日志和仍未验证的物理行为。

ROS 2 源码位于 `ros2_ws/src/`，在 `ros2_ws/` 下使用 `colcon build` 和 `colcon test`；新包依赖写入各自 `package.xml`。

## STM32CubeMX 再生成

`stm32f103rct6_proj/stm32f103rct6_proj.ioc` 是当前外设配置唯一来源，使用 STM32CubeMX 6.18.0 和 STM32Cube FW_F1 V1.8.7。当前配置包括 RCC、SYS/SWD、GPIO、USART3、I2C1、TIM3、TIM6、CAN1 和 FreeRTOS。

- CubeMX 管理文件的手写内容必须位于 `/* USER CODE BEGIN */` 块内。
- 再生成会更新 `Core/`、HAL/CMSIS/FreeRTOS 文件、`.mxproject` 和 `cmake/stm32cubemx/CMakeLists.txt`；先检查差异，避免提交纯换行符噪声。
- 保持 Serial Wire、8 MHz HSE/72 MHz 时钟树、CAN 中断优先级 5、I2C1 PB6/PB7 100 kHz，以及 Cortex-M3 `ARM_CM3` port。
- 再生成后确认 `Core/Src/i2c.c`、HAL I2C 源文件和所有手写层源文件仍在 CMake 清单中，然后执行一次 Debug 干净构建。

## 代码与文档约定

固件使用 C11、四空格缩进、Linux 大括号风格和模块前缀 API。活动工程尚无独立 `.clang-format`，暂时沿用 `stm32_proj/.clang-format` 的格式规则，但不要批量重排 CubeMX 或供应商源码。

提交主题使用简洁中文，不加手工版本号前缀；按用户要求可使用 `docs:` 等作用域前缀。修改引脚、外设或验证状态时，至少同步根 `README.md`、活动固件 README 和迁移计划。旧 F407 文档必须明确保持“历史/停用”标记，避免与当前 F103 配置混用。
