# STM32F103RCT6 固件工程

这是 OmniCar 当前使用的 STM32 下位机工程，目标芯片为 STM32F103RCT6（Cortex-M3、256 KB Flash、48 KB SRAM）。工程由 STM32CubeMX 生成基础代码，使用 CMake、Ninja 和 GNU Arm Embedded Toolchain 构建，并运行 FreeRTOS CMSIS-RTOS V2。

当前已完成从旧 STM32F407VET6 工程迁移的第一阶段工作，并配置 I2C1 供 ICM20948 使用。Debug 全量构建、ST-Link 烧录校验和 USART3 启动日志均已验证；日志确认 72 MHz 时钟、TIM3 PWM、CAN 控制器及 default/log/CAN 三个 FreeRTOS 任务均正常启动。LED、PWM 波形、CAN 物理链路、ICM20948 通信和电机行为仍需后续实物验证。

返回[仓库总览](../README.md)。

## 当前硬件配置

| 功能 | 引脚或参数 | 说明 |
| --- | --- | --- |
| 外部晶振 | PD0 / PD1 | 8 MHz HSE |
| 系统时钟 | 72 MHz | APB1 36 MHz，APB2 72 MHz |
| 板载 LED | PA8 | 推挽输出，高电平点亮 |
| 日志串口 | PB10 / PB11 | USART3 TX/RX，115200 8N1，无流控 |
| CAN1 | PA11 / PA12 | RX/TX，500 kbit/s，RX0 中断 |
| ICM20948 九轴 IMU | PB6 / PB7 | I2C1 SCL/SDA，硬件 I2C，默认引脚无需重映射 |
| 电机 PWM | PC6 / PC7 / PC8 | TIM3 CH1/CH2/CH3 全重映射，20 kHz |
| 电机 1 方向 | PC4 / PC5 | 普通推挽输出 |
| 电机 2 方向 | PB12 / PB13 | 普通推挽输出 |
| 电机 3 方向 | PB14 / PB15 | 普通推挽输出 |
| 外置 Flash / TF 卡片选 | PA2 / PA3 | 当前不使用，保持高电平避免误选中 |
| SWD | PA13 / PA14 | 必须保持 Serial Wire |

电机相关引脚中，PC6～PC8、PB10～PB15 同时连接板载 LCD 接口。本项目不使用 LCD，因此可以复用这些引脚；如果以后启用 LCD，必须重新规划电机和日志串口引脚。

IMU 选用 I2C1 的默认引脚 PB6/PB7，不需要开 AFIO 重映射。这两个脚同时也连到板载屏幕排针，**使用 IMU 时不要同时接屏幕**；后续若要启用屏幕，需要连同电机、日志串口一起重新规划。总线侧 SDA/SCL 需 4.7 kΩ 上拉到 3.3 V（多数 ICM20948 模块板上自带，没有则外接）。CubeMX 已将 I2C1 配置为 100 kHz 标准模式、7 位地址和允许时钟拉伸；当前未启用 DMA 或 I2C 中断。ICM20948 驱动仍为接口骨架，尚未执行地址应答或 `WHO_AM_I` 检查。

## 重要硬件注意事项

- PA11/PA12 同时连接 CAN1 RX/TX 和板载 Type-C 的 DM/DP。使用 CAN 时不要插 Type-C 数据线，也不要将该 Type-C 作为本项目供电接口，避免 USB 主机与 CAN 收发器争用信号线。
- CubeMX 的 `System Core > SYS > Debug` 必须选择 `Serial Wire`，不要改成 `No Debug`，否则程序启动后可能立即释放 SWD 引脚，导致 ST-Link 难以重新连接。
- 若 SWD 再次失联，优先降低适配器速率并尝试 Under Reset 连接；必要时拉高 BOOT0 后执行全片擦除，再恢复从 Flash 启动。
- 首次验证电机时应断开电机电源或架空车轮，先确认 PWM 频率、方向 GPIO 和停止状态，再带载测试。
- 原理图见 [`docs/双TypeCF103RCT6原理图.pdf`](docs/双TypeCF103RCT6原理图.pdf)，更完整的迁移判断见[迁移计划](docs/F407迁移到F103RCT6计划.md)。

## Windows Git Bash 构建

### 工具要求

Git Bash 的 `PATH` 中需要包含：

- `cmake`
- `ninja`
- `arm-none-eabi-gcc`
- `arm-none-eabi-objcopy`
- `arm-none-eabi-size`
- `openocd`（仅烧录时需要）

在本目录执行：

```sh
./32build.sh
```

默认构建 Debug。常用参数如下：

```sh
./32build.sh Debug --clean
./32build.sh Release --jobs 8
```

产物位于：

```text
build/Debug/stm32f103rct6_proj.elf
build/Debug/stm32f103rct6_proj.hex
build/Debug/stm32f103rct6_proj.bin
```

也可以直接使用 CMake preset：

```sh
cmake --preset Debug
cmake --build --preset Debug
```

## ST-Link 烧录

`32flash.sh` 使用 OpenOCD 的 `interface/stlink.cfg` 和 `target/stm32f1x.cfg`。默认先调用构建脚本，再烧录并校验 ELF：

```sh
./32flash.sh Debug --adapter-speed 100
```

确认命令但不连接芯片、不写入 Flash：

```sh
./32flash.sh Debug --no-build --dry-run --adapter-speed 100
```

已经确认产物是最新版本时，可以跳过构建：

```sh
./32flash.sh Debug --no-build --adapter-speed 100
```

`--adapter-speed` 的单位是 kHz。连接稳定后可以逐步提高速率；排查连接故障时先使用 100 kHz。

## 最近一次板级验证

2026-09-21 使用 ST-Link 和 CH340 USB 串口完成复测：

- Debug 全量构建通过，Flash 使用 35,560 B（13.57%），RAM 使用 22,168 B（45.10%）。
- OpenOCD 识别 STM32F103 高密度器件和 256 KiB Flash，目标电压约 3.25 V；烧录、校验和复位均成功。
- COM8 以 115200 8N1 完整收到 11 条启动日志；时钟、USART3、TIM3、CAN、应用初始化及三个 FreeRTOS 任务均成功，观察窗口内没有 `WARN` 或 `ERROR`。
- `MX_I2C1_Init()` 位于应用日志之前；能够进入应用并输出日志说明 HAL I2C 初始化未进入 `Error_Handler()`。这不代表 ICM20948 已应答，传感器通信仍需驱动实现后单独验证。

## 日志配置

日志等级只在 [`Middleware/log/log.h`](Middleware/log/log.h) 中设置：

```c
#define LOG_LEVEL LOG_LEVEL_INFO
```

可选等级为 `LOG_LEVEL_DEBUG`、`LOG_LEVEL_INFO`、`LOG_LEVEL_WARN`、`LOG_LEVEL_ERROR` 和 `LOG_LEVEL_NONE`。低于当前等级的日志会在编译期移除；`NONE` 会关闭包括 ERROR 在内的全部日志。日常默认使用 INFO，需要健康摘要和详细收发信息时可临时改为 DEBUG。

日志输出使用工程相对路径和函数名，不暴露开发机绝对路径：

```text
[3] [INFO] proj/App/main/app_main.c:App_Init(): application initialization complete
```

INFO 仅记录启动和关键事件；DEBUG 每 10 秒输出一次 heap、日志丢弃数和 CAN 统计。正常的每秒心跳、LED 翻转和中断过程不逐次打印。

`LOG_*` 在调用任务中完成格式化后，以零等待方式把最多 192 字节的完整日志复制到 16 项 FreeRTOS 队列。低优先级 `logTask` 由 CubeMX 的 `MX_FREERTOS_Init()` 与其他任务统一创建，每 200 ms 批量排空队列，再通过 USART3 轮询发送；因此业务任务不会等待串口。队列未就绪、队列已满或串口发送失败都会增加 dropped 计数。日志接口不能在中断中调用；应用代码也不应直接使用会绕过队列和等级过滤的 `printf()`。

## 软件结构

| 目录 | 职责 |
| --- | --- |
| `App/` | 应用入口、状态机和 CAN 指令分发 |
| `Motion/` | 运动学、PID、姿态和控制器 |
| `Middleware/` | 日志、CAN 协议、数学和环形缓冲服务 |
| `BSP/` | LED、UART、CAN、电机、编码器和 IMU 板级驱动 |
| `Core/` | CubeMX 生成的启动、外设初始化、中断和 FreeRTOS 接线 |
| `Drivers/`、`Middlewares/` | STM32 HAL、CMSIS 和 FreeRTOS 第三方源码 |
| `cmake/` | GNU Arm 工具链与 CubeMX 生成的 CMake 源文件清单 |

依赖方向应从 `App` 向下经过 `Motion`、`Middleware` 到 `BSP`；只有 BSP 直接操作 HAL 外设。新增手写模块后，需要把源文件加入顶层 `CMakeLists.txt` 的 `OMNICAR_LAYER_SOURCES`。

## FreeRTOS 与当前行为

- FreeRTOS 使用 GCC `ARM_CM3` port、CMSIS-RTOS V2、`heap_4` 和 16 KB heap。
- SysTick 供 FreeRTOS 使用，HAL 时间基准由 TIM6 提供。
- CAN RX0 中断把报文送入 FreeRTOS 队列，指令任务负责解析和应答。
- 日志使用 16 项异步队列和 768 字节低优先级发送任务栈，每 200 ms 批量输出。
- 上电后电机保持停止；默认任务每秒翻转 PA8 LED，并发送 CAN ID `0x101` 心跳。
- 收到 CAN ID `0x2FF` 测试帧后，以 `0x2FE` 返回 echo。
- I2C1 已初始化；编码器、ICM20948、PID、姿态和控制器模块仍处于接口骨架或待实现状态，速度指令 `0x201` 尚未接入闭环控制。

## CubeMX 再生成

`stm32f103rct6_proj.ioc` 是芯片和外设配置的唯一来源。重新生成前后需要特别检查：

1. SYS Debug 仍为 `Serial Wire`。
2. HSE 8 MHz 和 72 MHz 时钟树未变化。
3. FreeRTOS 仍使用 Cortex-M3 的 `ARM_CM3` port，CAN 中断优先级仍满足 FromISR API 要求。
4. I2C1 仍使用 PB6/PB7、100 kHz，并生成 `Core/Src/i2c.c` 与 HAL I2C 源文件。
5. 所有手写内容位于 CubeMX 的 `USER CODE` 块内。
6. 顶层 `CMakeLists.txt` 中的手写模块列表仍完整。

不要提交 `build/` 目录。CubeMX 生成后应至少运行一次：

```sh
./32build.sh Debug --clean
```

## 相关文档

- [F407 迁移到 F103RCT6 的计划、引脚依据和验证清单](docs/F407迁移到F103RCT6计划.md)
- [F103RCT6 系统板原理图](docs/双TypeCF103RCT6原理图.pdf)
- [旧 F407 工程说明](../stm32_proj/README.md)
