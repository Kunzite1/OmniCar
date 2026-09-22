# ROS 2 上位机工作区

这是 OmniCar 在 KICKPI K1 Mini（RK3568）上的 ROS 2 Humble 工作区。当前重点是通过达妙 USB-CAN 与 STM32F103RCT6 通信；后续业务控制代码继续放在 `car_control` 包内，并通过通信层提供的队列接口发送 CAN 帧。

返回[仓库总览](../README.md)。

## 当前功能包

| 包 | 状态 | 职责 |
| --- | --- | --- |
| [`car_control`](src/car_control/README.md) | 开发中 | CAN 节点、发送队列、达妙 CDC 协议打包；后续承载车辆业务组件 |

`car_control` 的通信节点独占 `/dev/ttyACM0`。ROS 发送者向 `/car_control/can/tx` 发布 `CanFrame`，节点校验后放入有界队列，由唯一 I/O 线程调用硬件发送；接收帧发布到 `/car_control/can/rx`。协议编码位于独立的 C11 文件 `src/can_protocol.c`。

## 设备识别

达妙模块枚举为 USB CDC 串口，并不会创建 SocketCAN 接口。板载 `can0` 是 RK3568 控制器，不要与 `/dev/ttyACM0` 混淆。排查时执行：

```sh
lsusb
lsusb -t
udevadm info --query=property --name=/dev/ttyACM0
ip -details link show type can
fuser -v /dev/ttyACM0
```

其中 `lsusb` 应显示 VID:PID `2e88:4603`，拓扑应使用 `cdc_acm`；启动节点前 `fuser` 不应显示占用进程。

## 容器构建与运行

主机未安装原生 Humble，使用现有 `ros:humble-ros-base` 镜像：

```sh
docker run --rm -it --network host \
  --device=/dev/ttyACM0:/dev/ttyACM0 \
  -v /root/OmniCar/ros2_ws:/ws -w /ws \
  ros:humble-ros-base bash
```

容器内执行：

```sh
source /opt/ros/humble/setup.bash
colcon build --packages-select car_control
source install/setup.bash
ros2 run car_control can_communication_node \
  --ros-args --params-file src/car_control/config/can.yaml
```

默认串口速率为 921600，CAN 波特率索引 `3` 对应 500 kbit/s。

## CAN 链路验证

STM32 每秒发送标准帧 `0x101`。查看接收话题：

```sh
ros2 topic echo /car_control/can/rx
```

通过发送队列发布 `0x2FF`：

```sh
ros2 topic pub --once /car_control/can/tx car_control/msg/CanFrame \
  "{id: 767, dlc: 8, is_extended: false, is_remote: false, \
  data: [66, 75, 49, 83, 84, 77, 51, 50]}"
```

链路正常时会收到 ID `0x2FE` 且数据相同。日志统计中的 `heartbeat`、`echo` 应递增，`queue_drop`、`write_err` 和 `adapter_err` 应保持为零。

## 工作区约定

- 新包放在 `src/<package_name>/`，依赖同步写入 `package.xml`。
- 不提交 `build/`、`install/`、`log/`。
- 修改接口或依赖后重新构建，并重新加载 `install/setup.bash`。
- CAN 业务协议以 STM32 的 [`can_protocol.h`](../stm32f103rct6_proj/Middleware/can_protocol/can_protocol.h) 为依据；达妙串口封装只处理传输，不掺入车辆业务语义。
