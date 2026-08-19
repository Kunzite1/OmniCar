# Agent 汇报

本文件记录 Agent 完成的关键工作，供人工复习嵌入式开发过程。只保留结论、方法、关键命令和下一步，不记录密码或冗长原始日志。

## 2026-08-19：STM32 结构梳理与 K1 Mini CAN 排查

### 结论

- 本批 CAN/CubeMX/仓库文档杂项改动统一按 v0.7.4 收口；该版只表示 CAN 自检代码编译就绪，不表示物理链路已打通。
- STM32 的 CAN1（PD0/PD1、500 kbit/s）、中断接收队列、1 Hz 心跳和 echo 已有最小代码，且本次 GCC 交叉编译通过；但尚无物理 CAN 总线实测证据。
- ROS 2 侧只有 `test` 包的 `/hello` 计数发布节点，没有 CAN 节点、容器配置或业务协议实现；本次 `colcon build` 通过，2 项风格测试通过。
- 已通过 SSH 对 K1 Mini 做只读检查。系统安装了 `candump`、`cansend`，内核也启用了 `CONFIG_CANFD_ROCKCHIP=y`，但当前没有 `can0/can1` 网卡。
- 根因是运行中设备树的 `can@fe570000`、`can@fe580000`、`can@fe590000` 三个节点全部为 `disabled`，不是 Docker 或 `can-utils` 缺失。
- 本地 `kickpi_sdk/kernel` 指向 Linux 6.1.141，而板卡运行 Linux 5.10.160。可以用 SDK 查资料和修改源码，但不能把不同内核版本的产物直接混用。
- KICKPI 官方支持表将旧 Linux SDK 标为不再维护，而 `linux-kernel-6.1` 明确维护 K1 Mini；本地 SDK 也有该板的 Ubuntu/Debian defconfig。因此长期方案应迁移整套匹配的 6.1 镜像，而不是把 6.1 的 DTB 或模块塞进现有 5.10 系统。

### 做了什么、怎么判断

1. 阅读 STM32 的目录、CMake 源文件清单、`main.c` 和 `freertos.c`，确认初始化顺序和两个 RTOS 任务。
2. 登录 K1 Mini，检查网络接口、CAN 工具、内核配置、平台驱动和实时设备树状态。
3. 绕过 `.gitignore` 检查本地 KICKPI SDK，定位 K1 Mini DTS、RK3568 pinctrl 和 Rockchip CAN 驱动文档。
4. 核对 Git 基线与未提交工作区，将 README 中过早的“打通 CAN 链路”修正为“自检代码就绪，硬件待验收”。
5. 在开发主机构建当前 STM32 固件和 ROS 2 测试包，未烧录 STM32，未修改 K1 Mini、SDK 或设备树。本轮尝试重新 SSH 核对板卡时认证失败，因此 K1 结论仍以同日前次只读实测为准。
6. 对照 KICKPI 官方的 [Linux SDK 支持表](https://doc.kickpi.com/products/sdk_compilation/rk356x-rk3588/linux_sdk/)、[CAN 说明](https://doc.kickpi.com/products/peripherals_interfaces/can/)和[镜像下载页](https://doc.kickpi.com/products/download/rk_download/)，确认 K1 Mini 的维护内核线、原生 CAN 外接收发器要求和官方镜像入口。

### 关键命令

开发主机上的源码检查：

```sh
find stm32_proj -maxdepth 3 -type d | sort
rg --files stm32_proj | sort
sed -n '1,240p' stm32_proj/CMakeLists.txt
rg -n '^#include' stm32_proj/{App,Motion,Middleware,BSP} -g '*.[ch]'
rg --no-ignore -n '&can[012]|can[012]m[01]_pins' kickpi_sdk/kernel-6.1
git status --short --branch
cmake --build build                         # 在 stm32_proj/ 执行
colcon build --symlink-install              # 在 ros2_ws/ 执行
colcon test && colcon test-result --verbose
```

v0.7.4 收口时重新验证：STM32 固件编译成功，FLASH 使用 36,688 B（7.00%），RAM 使用 38,144 B（29.10%）；ROS 2 `test` 包构建成功，2 项测试零失败。测试仅有 Python 依赖的弃用接口警告，不影响通过结果。

K1 Mini 上的只读检查：

```sh
ssh root@192.168.1.102
ip -br link
ip -details -statistics link show type can
command -v candump
command -v cansend
zcat /proc/config.gz | grep -E '^(CONFIG_CAN|CONFIG_CANFD_ROCKCHIP)'
find /sys/firmware/devicetree/base -type d -iname '*can*'
```

设备树节点的 `status` 可用 `tr -d '\0' < <节点>/status` 读取。实测三路均输出 `disabled`；`/sys/bus/platform/drivers/rockchip_canfd` 存在，说明驱动已编入内核但没有可探测的启用节点。

### 引脚复用与下一步

K1 Mini 40Pin 可用的 CAN 组合如下，启用前必须停用占用同组引脚的默认外设：

| 控制器映射 | 40Pin 引脚 | 默认冲突 |
|---|---|---|
| CAN1 M0 | 3=RX，5=TX | I2C3 M0 |
| CAN1 M1 | 19=TX，23=RX | SPI3 M1 |
| CAN2 M0 | 22=RX，24=TX | I2C2 M1 |

长期首选方案是先备份当前 5.10 系统并准备串口、Recovery/Maskrom 恢复手段，在备用 TF 卡上启动官方 K1 Mini 6.1 完整镜像，先验证网络、USB、存储和 Docker。基线正常后，再确定实际接线，在 `kickpi_sdk/kernel-6.1/arch/arm64/boot/dts/rockchip/rk3568-kickpi-K1mini-extend-40pin.dtsi` 中关闭冲突外设、启用相应 CAN 节点并选择正确的 `pinctrl`；内核、DTB、模块和启动镜像必须来自同一套 6.1 构建。

若必须暂留 5.10，只能向板卡厂商取得与当前镜像精确对应的 K1 Mini 5.10 BSP、defconfig 和打包方法，再修改该版本设备树；通用 RK3568 5.10 源码不能视为可替代品。若只想尽快完成 ROS 与 STM32 通信里程碑，可先使用内核已支持的 USB-CAN 适配器，绕过板载 CAN 的设备树迁移。

设备树生效并重启后，先用 `ip -br link` 确认实际接口名，再以经典 CAN 500 kbit/s 联调 STM32：

```sh
ip link set can0 down
ip link set can0 type can bitrate 500000 fd off restart-ms 100
ip link set can0 up
ip -details -statistics link show can0
candump can0
cansend can0 2FF#A1B2C3D4
```

预期每秒收到 STM32 的 `101#...` 心跳；发送 `2FF#A1B2C3D4` 后收到 `2FE#...` echo。K1 Mini 和 STM32 两端都需要 CAN 收发器，CAN_H 接 CAN_H、CAN_L 接 CAN_L并共地，在总线两端各保留一个 120 Ω 终端电阻。

### 里程碑顺序

1. 优先在备用 TF 卡验证官方维护的 K1 Mini 6.1 镜像；只有取得当前镜像精确匹配的厂商 BSP 时，才继续保留 5.10。
2. 启用宿主机 CAN 网卡，先做 SocketCAN 本地回环，再以 `candump/cansend` 与 STM32 双向验证。
3. 在 ROS Humble 容器内只验证原始 SocketCAN 收发。CAN 是网络接口，不是 `/dev` 字符设备；宿主管理波特率时，容器只需获得对应网络命名空间和原始套接字权限。
4. 固化业务协议后再写 ROS 2 CAN 节点：明确 ID、DLC、字节序、单位、范围、周期、超时停车、序号和异常处理，并先用 `vcan` 测编解码。
5. 动作验证拆成“解析并打印”、“架空车轮低占空比”和“带看门狗的整车运动”。若验收闭环轮速，则必须先完成当前仍为 stub 的编码器、PID 和 controller。
