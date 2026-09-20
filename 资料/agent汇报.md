# Agent 汇报

## 2026-09-20：STM32F103RCT6 第一阶段代码迁移

结论：已完成 F103RCT6 的 CubeMX 外设配置、代码生成、业务代码迁移和首次板级日志自检。Git Bash DEBUG 构建、ST-Link 烧录校验、USART3 日志及 FreeRTOS 持续运行均正常；后续仍需随小车装配验证 LED、PWM 波形、CAN 物理链路和电机行为。

### 做了什么

- 只读校对了 STM32F103RCT6 的 CubeMX 器件数据库，确认 F1 工程中的外设配置名为 `CAN`，引脚信号名为 `CAN_RX/CAN_TX`；生成后的 HAL 句柄为 `hcan`。
- 修改 [stm32f103rct6_proj.ioc](../stm32f103rct6_proj/stm32f103rct6_proj.ioc)，配置 8 MHz HSE、72 MHz SYSCLK、USART3、CAN、TIM3 PWM、TIM6 HAL 时基、Serial Wire 和 FreeRTOS。
- 通过 STM32CubeMX 6.18.0 重新生成 F1 HAL、FreeRTOS `ARM_CM3` port 和 CMake 源文件清单。
- 将原 F407 工程的 `App/`、`BSP/`、`Middleware/`、`Motion/` 迁入 F103 工程；日志改为 USART3 PB10/PB11，LED 改为 PA8 高有效，CAN 改为 PA11/PA12，电机 PWM 改为 PC6/PC7/PC8，方向改为 PC4/PC5、PB12～PB15。
- 在 [CMakeLists.txt](../stm32f103rct6_proj/CMakeLists.txt) 注册所有手写模块，并在 FreeRTOS 的 CubeMX USER CODE 区域接入应用初始化、默认任务和 CAN 指令任务。
- 维护 [32build.sh](../stm32f103rct6_proj/32build.sh) 与 [32flash.sh](../stm32f103rct6_proj/32flash.sh)；烧录脚本支持 `--adapter-speed` 和 `--dry-run`，默认烧录前重新构建。
- 重构 `Middleware/log`：日志等级由 `log.h` 中单个宏控制，支持 DEBUG/INFO/WARN/ERROR/NONE；路径裁剪为 `proj/相对路径:函数名():`。后续将同步串口输出改为 16 项队列和低优先级 `logTask`，调用方只格式化并零等待入队，日志任务每 200 ms 批量发送；任务创建与 default/CAN 任务一起集中在 CubeMX 的 `MX_FREERTOS_Init()` USER CODE 区域。
- 为 UART、TIM3 PWM、CAN 初始化和 FreeRTOS 任务创建补充返回值检查；CAN ISR 只累计接收/丢帧数据，不执行阻塞日志。
- `32flash.sh` 原先在加载 target 配置前设置速率，实际被 target 默认值覆盖；已调整参数顺序并确认 OpenOCD 真正采用 100 kHz。
- 更新 [迁移计划](../stm32f103rct6_proj/docs/F407迁移到F103RCT6计划.md)，记录已完成项目和待上板验证项。

### 关键判断

- CAN 时钟是 APB1 的 36 MHz。`Prescaler=4`、`1+BS1(15)+BS2(2)=18 TQ`，所以波特率为 `36 MHz / 4 / 18 = 500 kbit/s`，采样点约为 88.9%。
- TIM3 时钟为 72 MHz。`Prescaler=0`、`Period=3599`，所以 PWM 为 `72 MHz / 3600 = 20 kHz`。
- CAN RX0 中断优先级为 5，与 FreeRTOS 的 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5` 匹配，可在 HAL 回调中使用 `xQueueSendFromISR()`。
- F103RCT6 只有 48 KB SRAM，因此 FreeRTOS heap 从原 F407 方案降为 16 KB；当前 DEBUG 链接结果显示 RAM 使用 22,088 B（44.94%），仍有余量，但上板后应继续检查任务栈水位。

### 实际执行的命令

开发主机（PowerShell 调用 STM32CubeMX CLI）：

```text
java.exe -Duser.home=C:\Users\admin -jar STM32CubeMX.exe -q .cubemx_generate.script
```

开发主机（Windows Git Bash，CMake/Ninja/Arm GNU/OpenOCD 已加入本次 shell 的 `PATH`）：

```sh
cd /c/Users/admin/Documents/OmniCar/stm32f103rct6_proj
./32build.sh Debug --clean
./32flash.sh Debug --no-build --dry-run --adapter-speed 100
./32flash.sh Debug --no-build --adapter-speed 100
```

开发主机（PowerShell，异步日志复测）：

```text
使用 System.IO.Ports.SerialPort 独占打开 COM8，参数为 115200 8N1
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "adapter speed 100" -c init -c "reset run" -c shutdown
```

### 验证结果

- CubeMX 成功生成 CAN、TIM3 PWM、USART3、TIM6 和 FreeRTOS 代码。
- 首次日志改动全量构建时发现 `app_main.c` 缺少 HAL 主头文件，补充显式 include 后构建通过；生成 `stm32f103rct6_proj.elf/.bin/.hex`。
- 首次同步日志 DEBUG 链接统计为 Flash 35,776 B / 256 KB、RAM 22,088 B / 48 KB。改为异步队列并统一由 `MX_FREERTOS_Init()` 创建任务后，INFO 构建为 Flash 34,700 B（13.24%）、RAM 22,080 B（44.92%）；`Log_Write` 静态栈占用 348 B，`Log_Task` 为 216 B，日志任务配置 768 B 栈。
- 日志队列和任务从 16 KB FreeRTOS heap 动态分配，预计比原互斥量方案多使用约 4 KB；按首次上板剩余 13,728 B 推算仍有约 9.7 KB，实际值需下次烧录后由 DEBUG 健康摘要确认。
- OpenOCD 两次完成 Programming、Verified OK 和 reset；识别 STM32F1 Cortex-M3、256 KiB Flash，目标电压约 3.24～3.25 V。修正脚本后实际 SWD 时钟为 100 kHz。
- CH340 枚举为 COM8。USART3 日志完整显示 72/72/36/72 MHz 时钟、20 kHz TIM3 参数、CAN 启动、defaultTask/canTask 创建和进入；日志路径为 `proj/...:函数名():`。
- 异步日志版本重新烧录、校验并复位成功；串口完整收到 11 条预期启动日志，顺序与调用顺序一致，`logTask` 创建信息和三个任务入口日志均正常，没有乱码、交叉、截断或缺行。当前 INFO 等级不输出健康摘要，因此本次未直接测量异步版本的 dropped 和剩余 heap。
- 约 9 秒健康日志显示 FreeRTOS 剩余 heap 13,728 B、日志丢弃 0、CAN 为 LISTENING。未连接 ACK 对端时约 19 秒出现一次发送邮箱警告，后续仅在 DEBUG 摘要累计，不持续刷 WARN。
- 仓库基线曾跟踪 `stm32f103rct6_proj/build/` 中的 66 个生成文件；本次提交已将它们从 Git 索引移除并补充 `.gitignore`，本地构建文件仍保留。

### 下一步

1. 装车前目视确认 PA8 每秒翻转；使用示波器或逻辑分析仪测量 PC6/PC7/PC8 是否为 20 kHz，并确认六路方向 GPIO 上电为低。
2. CAN 工作时不要插 Type-C；接好 CAN 总线终端电阻和 ACK 对端后验证 0x101 心跳及 0x2FF/0x2FE echo。
3. 装车后再验证电机方向、编码器、IMU 和闭环控制；当前日志自检不能替代这些电气与机械验证。
4. 异步日志启动顺序已验证；需要评估压力时临时启用 DEBUG，继续观察 dropped、运行时剩余 heap 和任务 stack high-water mark，再决定是否调整 16 项队列及 768 B 日志任务栈。
5. 长时间运行后继续观察各任务 stack high-water mark；日常保持 INFO，需要详细排障时再临时启用 DEBUG。

## 2026-09-20：README 分层整理

结论：根 README 已收敛为项目概述和导航；STM32F103RCT6、ROS 2 与旧 STM32F407 工程分别维护自己的详细 README。旧 F407 工程已明确标记为“暂时废弃”，当前不删除，待 F103 完成关键外设上板验收后再归档并从主开发线移除。

### 做了什么

- 重写根目录 [README.md](../README.md)，只保留系统定位、子工程入口、通信关系和资料导航。
- 新增 [F103 固件 README](../stm32f103rct6_proj/README.md)，记录实际引脚、Git Bash 构建、OpenOCD 烧录、FreeRTOS、CubeMX 和 Type-C 冲突等注意事项。
- 新增 [ROS 2 工作区 README](../ros2_ws/README.md)，记录现有测试包、colcon 构建测试和 SocketCAN 联调方式。
- 新增 [旧 F407 工程 README](../stm32_proj/README.md)，明确该工程暂时废弃，仅用于迁移对照和历史追溯。

### 怎么判断

- F103 工程已编译通过，但迁移后的 LED、USART3、PWM、CAN 和基础电机行为尚未完成整套上板验收，因此现在直接删除 F407 工程会过早失去方便的对照基线。
- 两套 CubeMX、HAL 和 FreeRTOS 源码长期并存会增加仓库体积，也容易让开发者误改、误构建旧工程；F407 不适合永久保留在主开发线。
- 推荐在 F103 关键链路验收后，为最后可用的 F407 状态建立 Git 标签或归档分支，再删除 `stm32_proj/`。Git 历史仍可恢复旧工程，无需在主线永久复制一套源码。

### 实际使用的命令

开发主机（只读检查与文档校验）：

```text
rg --files -g 'README*' -g 'readme*'
rg --files stm32f103rct6_proj -g '!build/**'
rg --files ros2_ws
git diff --check
Test-Path <README 中引用的目标文件>
```

### 验证结果与下一步

- README 中引用的工程文档、原理图、协议头文件和历史引脚表均存在。
- `git diff --check` 未发现空白错误；本次仅修改文档，没有重新构建或烧录固件。
- F103 完成关键外设与基础电机上板验收后，再执行 F407 工程归档和删除，不在当前迁移阶段删除。
