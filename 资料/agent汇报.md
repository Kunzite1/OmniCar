# Agent 汇报

## 2026-09-20：STM32F103RCT6 第一阶段代码迁移

结论：已完成 F103RCT6 的 CubeMX 外设配置、代码生成和 OmniCar 四层业务代码迁移，Git Bash Debug 全量构建通过；本轮没有实际烧录，下一步是按 LED、USART3、PWM、CAN 的顺序上板验证。

### 做了什么

- 只读校对了 STM32F103RCT6 的 CubeMX 器件数据库，确认 F1 工程中的外设配置名为 `CAN`，引脚信号名为 `CAN_RX/CAN_TX`；生成后的 HAL 句柄为 `hcan`。
- 修改 [stm32f103rct6_proj.ioc](../stm32f103rct6_proj/stm32f103rct6_proj.ioc)，配置 8 MHz HSE、72 MHz SYSCLK、USART3、CAN、TIM3 PWM、TIM6 HAL 时基、Serial Wire 和 FreeRTOS。
- 通过 STM32CubeMX 6.18.0 重新生成 F1 HAL、FreeRTOS `ARM_CM3` port 和 CMake 源文件清单。
- 将原 F407 工程的 `App/`、`BSP/`、`Middleware/`、`Motion/` 迁入 F103 工程；日志改为 USART3 PB10/PB11，LED 改为 PA8 高有效，CAN 改为 PA11/PA12，电机 PWM 改为 PC6/PC7/PC8，方向改为 PC4/PC5、PB12～PB15。
- 在 [CMakeLists.txt](../stm32f103rct6_proj/CMakeLists.txt) 注册所有手写模块，并在 FreeRTOS 的 CubeMX USER CODE 区域接入应用初始化、默认任务和 CAN 指令任务。
- 维护 [32build.sh](../stm32f103rct6_proj/32build.sh) 与 [32flash.sh](../stm32f103rct6_proj/32flash.sh)；烧录脚本支持 `--adapter-speed` 和 `--dry-run`，默认烧录前重新构建。
- 更新 [迁移计划](../stm32f103rct6_proj/docs/F407迁移到F103RCT6计划.md)，记录已完成项目和待上板验证项。

### 关键判断

- CAN 时钟是 APB1 的 36 MHz。`Prescaler=4`、`1+BS1(15)+BS2(2)=18 TQ`，所以波特率为 `36 MHz / 4 / 18 = 500 kbit/s`，采样点约为 88.9%。
- TIM3 时钟为 72 MHz。`Prescaler=0`、`Period=3599`，所以 PWM 为 `72 MHz / 3600 = 20 kHz`。
- CAN RX0 中断优先级为 5，与 FreeRTOS 的 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5` 匹配，可在 HAL 回调中使用 `xQueueSendFromISR()`。
- F103RCT6 只有 48 KB SRAM，因此 FreeRTOS heap 从原 F407 方案降为 16 KB；链接结果显示 RAM 使用 22,056 B（44.87%），仍有余量，但上板后应继续检查任务栈水位。

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
```

### 验证结果

- CubeMX 成功生成 CAN、TIM3 PWM、USART3、TIM6 和 FreeRTOS 代码。
- `32build.sh Debug --clean` 完成 56 个编译/链接步骤，无编译错误；生成 `stm32f103rct6_proj.elf/.bin/.hex`。
- 链接统计：Flash 31,128 B / 256 KB（11.87%），RAM 22,056 B / 48 KB（44.87%）。
- `32flash.sh` dry-run 正确选择 `interface/stlink.cfg`、`target/stm32f1x.cfg`、100 kHz 适配器速率及 Debug ELF。该命令没有连接 ST-Link，也没有改变芯片 Flash。
- 仓库基线曾跟踪 `stm32f103rct6_proj/build/` 中的 66 个生成文件；本次提交已将它们从 Git 索引移除并补充 `.gitignore`，本地构建文件仍保留。

### 下一步

1. 实际烧录后确认 PA8 每秒翻转、USART3 115200 8N1 输出启动日志。
2. 不接电机负载，测量 PC6/PC7/PC8 是否为 20 kHz，并确认六路方向 GPIO 上电为低。
3. CAN 工作时不要插 Type-C；接好 CAN 总线终端电阻后验证 0x101 心跳和 0x2FF/0x2FE echo。
4. 上板运行一段时间后读取 FreeRTOS high-water mark，再决定是否调整 16 KB heap 和任务栈。
5. 后续提交继续保持 `stm32f103rct6_proj/build/` 不进入版本控制。

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
