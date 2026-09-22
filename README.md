# OmniCar 全向移动车

OmniCar 是一个全向移动车软硬件单仓库，包含 STM32 下位机固件、ROS 2 上位机工作区以及硬件参考资料。上位机负责感知与任务控制，下位机负责实时外设和运动控制，两者计划通过 500 kbit/s CAN 总线通信。

当前固件开发已从 STM32F407VET6 迁移到 STM32F103RCT6。F103 工程已完成第一阶段迁移、I2C1 配置和基础 CAN 自检协议；K1 Mini 侧已新增 ROS 2 `car_control` 功能包及达妙 USB-CAN 通信节点。两端代码均可构建，CAN 物理链路仍需完成实物验收。

## 子工程

| 入口 | 状态 | 内容 |
| --- | --- | --- |
| [STM32F103RCT6 固件](stm32f103rct6_proj/README.md) | 当前使用 | CubeMX、CMake、FreeRTOS、板级外设和运动控制代码 |
| [ROS 2 工作区](ros2_ws/README.md) | 持续开发 | K1 Mini 上位机功能包、构建测试和 CAN 联调说明 |
| [STM32F407VET6 固件](stm32_proj/README.md) | 暂时废弃 | 迁移前的旧固件，仅作历史参考 |

## 系统关系

```text
ROS 2 / K1 Mini
        │ USB
    USB-CAN 模块
        │ CAN_H / CAN_L，500 kbit/s
      CAN 收发器
        │ PA11 / PA12
STM32F103RCT6 ── 电机驱动 / 编码器 / IMU
```

K1-Mini开发板位号ETH0的网口，配置为静态IP: 192.168.1.102，可以使用网线ssh。
``` shell
ssh root@192.168.1.102  # 密码：root
```

ROS 2 工作区的 `car_control` 包通过 `/dev/ttyACM0` 直接使用达妙 USB-CAN CDC 协议；它不是 SocketCAN `can0`。发送者向 `/car_control/can/tx` 发布帧，通信节点统一排队并独占硬件发送，接收帧发布到 `/car_control/can/rx`。STM32 固件保留 `0x101` 心跳和 `0x2FF`/`0x2FE` echo 自检链路。

## 资料入口

- [F407 迁移到 F103RCT6 的计划与当前进度](stm32f103rct6_proj/docs/F407迁移到F103RCT6计划.md)
- [F103RCT6 系统板原理图](stm32f103rct6_proj/docs/双TypeCF103RCT6原理图.pdf)
- [旧 F407 转接板引脚分配](docs/引脚分配.md)

具体的环境要求、构建命令、烧录方式、引脚配置和注意事项统一记录在各子工程 README 中，根 README 不再重复维护这些细节。

## 仓库约定

- STM32 的 `build/` 以及 ROS 2 的 `build/`、`install/`、`log/` 均为本地生成目录，不纳入版本管理。
- CubeMX 管理文件中的手写代码必须放在 `/* USER CODE BEGIN */` 与 `/* USER CODE END */` 区域内。
- 硬件操作前先阅读对应工程 README 中的接线和安全说明。
