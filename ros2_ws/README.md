# ROS 2 上位机工作区

这是 OmniCar 的 ROS 2 工作区，目标运行环境为 KICKPI K1 Mini（RK3568）上的 ROS 2 Humble。上位机后续负责传感器接入、任务控制和通过 USB-CAN 向 STM32 下位机发送运动指令。

当前工作区只有一个最小 `ament_python` 测试包，尚未实现正式的 CAN 控制节点。

返回[仓库总览](../README.md)。

## 当前包

| 包 | Python 模块 | 状态 |
| --- | --- | --- |
| `test` | `test_pkg` | 提供 `hello_publisher`，每秒向 `/hello` 发布递增字符串 |

包名与 Python 模块名不同：ROS 2 包名是 `test`，源码模块目录是 `test_pkg/`，不要把业务节点误放进用于代码检查的 `test/test/` 目录。

## 构建与运行

先准备 ROS 2 Humble 环境，然后从工作区目录执行：

```sh
source /opt/ros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash
ros2 run test hello_publisher
```

在另一个已经加载工作区环境的终端查看消息：

```sh
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 topic echo /hello
```

如果 K1 Mini 通过容器运行 ROS 2，先参考 [`../docker/readme.md`](../docker/readme.md) 准备容器环境，再在对应工作区中执行相同的 colcon 命令。

## 测试

当前包配置了 `ament_flake8` 和 `ament_pep257`：

```sh
colcon test --packages-select test
colcon test-result --verbose
```

## USB-CAN 联调

计划链路为：

```text
K1 Mini ── USB-CAN ── CAN_H/CAN_L ── CAN 收发器 ── STM32F103RCT6
```

USB-CAN 在 Linux 上应提供 SocketCAN 接口，例如 `can0`。STM32 当前使用 500 kbit/s：

```sh
sudo ip link set can0 down
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
ip -details link show can0
```

安装 `can-utils` 后可以进行固件链路自检：

```sh
candump can0
cansend can0 2FF#A1B2C3D4
```

预期每秒收到一次 `0x101` 心跳；发送 `0x2FF` 后应收到内容对应的 `0x2FE` echo。正式 ROS 2 CAN 节点尚未实现，目前这些命令仅用于 SocketCAN 和 STM32 固件联调。

## 工作区结构与约定

```text
ros2_ws/
├── README.md
└── src/
    └── test/
        ├── package.xml
        ├── setup.py
        ├── test_pkg/          # 节点源码
        └── test/              # flake8 / pep257 测试
```

- 新增 ROS 2 包放在 `src/<package_name>/`，依赖同步写入 `package.xml`。
- `build/`、`install/`、`log/` 是 colcon 生成目录，已在仓库根 `.gitignore` 中忽略，不要提交。
- 修改依赖或入口后，重新构建并重新加载 `install/setup.bash`。
- CAN 控制协议以 STM32 工程的 [`Middleware/can_protocol/can_protocol.h`](../stm32f103rct6_proj/Middleware/can_protocol/can_protocol.h) 为当前依据。
