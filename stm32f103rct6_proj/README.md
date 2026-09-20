# STM32F103RCT6 固件工程

这是 OmniCar 当前使用的 STM32 下位机工程，目标芯片为 STM32F103RCT6（Cortex-M3、256 KB Flash、48 KB SRAM）。工程由 STM32CubeMX 生成基础代码，使用 CMake、Ninja 和 GNU Arm Embedded Toolchain 构建，并运行 FreeRTOS CMSIS-RTOS V2。

当前已完成从旧 STM32F407VET6 工程迁移的第一阶段工作，Debug 全量构建通过。迁移后固件仍需按 LED、日志串口、PWM、CAN 的顺序完成板上验证。

返回[仓库总览](../README.md)。

## 当前硬件配置

| 功能 | 引脚或参数 | 说明 |
| --- | --- | --- |
| 外部晶振 | PD0 / PD1 | 8 MHz HSE |
| 系统时钟 | 72 MHz | APB1 36 MHz，APB2 72 MHz |
| 板载 LED | PA8 | 推挽输出，高电平点亮 |
| 日志串口 | PB10 / PB11 | USART3 TX/RX，115200 8N1，无流控 |
| CAN1 | PA11 / PA12 | RX/TX，500 kbit/s，RX0 中断 |
| 电机 PWM | PC6 / PC7 / PC8 | TIM3 CH1/CH2/CH3 全重映射，20 kHz |
| 电机 1 方向 | PC4 / PC5 | 普通推挽输出 |
| 电机 2 方向 | PB12 / PB13 | 普通推挽输出 |
| 电机 3 方向 | PB14 / PB15 | 普通推挽输出 |
| 外置 Flash / TF 卡片选 | PA2 / PA3 | 当前不使用，保持高电平避免误选中 |
| SWD | PA13 / PA14 | 必须保持 Serial Wire |

电机相关引脚中，PC6～PC8、PB10～PB15 同时连接板载 LCD 接口。本项目不使用 LCD，因此可以复用这些引脚；如果以后启用 LCD，必须重新规划电机和日志串口引脚。

## 重要硬件注意事项

- PA11/PA12 同时连接 CAN1 RX/TX 和板载 Type-C 的 DM/DP。使用 CAN 时不要插 Type-C 数据线，也不要将该 Type-C 作为本项目供电接口，避免 USB 主机与 CAN 收发器争用信号线。
- CubeMX 的 `System Core > SYS > Debug` 必须选择 `Serial Wire`，不要改成 `No Debug`，否则程序启动后可能立即释放 SWD 引脚，导致 ST-Link 难以重新连接。
- 若 SWD 再次失联，优先降低适配器速率并尝试 Under Reset 连接；必要时拉高 BOOT0 后执行全片擦除，再恢复从 Flash 启动。
- 首次验证电机时应断开电机电源或架空车轮，先确认 PWM 频率、方向 GPIO 和停止状态，再带载测试。
- 原理图见 [`docs/Schematic+Prints.pdf`](docs/Schematic+Prints.pdf)，更完整的迁移判断见[迁移计划](docs/F407迁移到F103RCT6计划.md)。

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
- 上电后电机保持停止；默认任务每秒翻转 PA8 LED，并发送 CAN ID `0x101` 心跳。
- 收到 CAN ID `0x2FF` 测试帧后，以 `0x2FE` 返回 echo。
- 编码器、ICM20948、PID、姿态和控制器模块仍处于接口骨架或待实现状态；速度指令 `0x201` 尚未接入闭环控制。

## CubeMX 再生成

`stm32f103rct6_proj.ioc` 是芯片和外设配置的唯一来源。重新生成前后需要特别检查：

1. SYS Debug 仍为 `Serial Wire`。
2. HSE 8 MHz 和 72 MHz 时钟树未变化。
3. FreeRTOS 仍使用 Cortex-M3 的 `ARM_CM3` port，CAN 中断优先级仍满足 FromISR API 要求。
4. 所有手写内容位于 CubeMX 的 `USER CODE` 块内。
5. 顶层 `CMakeLists.txt` 中的手写模块列表仍完整。

不要提交 `build/` 目录。CubeMX 生成后应至少运行一次：

```sh
./32build.sh Debug --clean
```

## 相关文档

- [F407 迁移到 F103RCT6 的计划、引脚依据和验证清单](docs/F407迁移到F103RCT6计划.md)
- [F103RCT6 系统板原理图](docs/Schematic+Prints.pdf)
- [仓库开发与排查记录](../资料/agent汇报.md)
- [旧 F407 工程说明](../stm32_proj/README.md)
