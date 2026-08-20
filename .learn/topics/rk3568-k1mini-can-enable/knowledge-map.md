# RK3568 K1 Mini 开启 CAN 设备

> 0/23 mastered · 0% complete

## 全链路心智模型

- 🔵 **一帧 CAN 数据的端到端旅程** (in progress)
  - 用户程序与 SocketCAN
  - Linux 网络接口与内核驱动
  - RK3568 CAN 控制器
  - 收发器与 CAN\_H、CAN\_L
- ⚪ **CAN 控制器、收发器与总线的分工** (unexplored)
  - 数字逻辑侧 RXD、TXD
  - 差分物理侧 CAN\_H、CAN\_L
  - 为什么开发板引脚不能直接接 CAN 总线
- ⚪ **经典 CAN、CAN FD 与波特率** (unexplored)
  - 本项目为何使用经典 CAN 500 kbit/s
  - 总线两端参数一致的必要性
  - Linux 中 fd off 的含义
- ⚪ **终端电阻、共地与安全接线** (unexplored)
  - 总线两端各 120 Ω
  - 断电测得约 60 Ω 的原因
  - CAN\_H、CAN\_L、GND 的接线检查

## RK3568 引脚与设备树

- ⚪ **RK3568 CAN 控制器与 K1 Mini 40Pin** (unexplored)
  - SoC 内 CAN 控制器与板级引脚的关系
  - CAN1 M0 对应物理 3、5 脚
  - 控制器编号不等于 Linux 接口编号
- ⚪ **引脚复用与 CAN1 M0、I2C3 M0 冲突** (unexplored)
  - 一个物理引脚的多种功能
  - 为什么启用 CAN1 前必须禁用 I2C3
  - M0 代表的复用组
- ⚪ **DTS、DTB 与设备树节点** (unexplored)
  - DTS 源文本与 DTB 二进制
  - 设备树如何描述不可自动枚举的硬件
  - 节点地址与属性
- ⚪ **status、pinctrl 与 phandle** (unexplored)
  - okay 与 disabled
  - pinctrl-0 如何选择 CAN1 M0 引脚组
  - phandle 如何引用另一个节点
- ⚪ **内核配置、驱动与 can0 的出现条件** (unexplored)
  - CONFIG\_CANFD\_ROCKCHIP
  - 设备树节点匹配内核驱动
  - 驱动探测成功后注册 SocketCAN 网络接口

## K1 Mini 启动与镜像结构

- ⚪ **从上电到 Linux 的启动链** (unexplored)
  - BootROM、MiniLoader、U-Boot、内核与 rootfs
  - 设备树在启动链中的传递位置
- ⚪ **Rockchip update.img 的嵌套结构** (unexplored)
  - update.img 与 firmware.img
  - 参数、loader、boot、recovery、rootfs 等组件
  - 为何本项目只替换 boot.img
- ⚪ **FIT boot.img 与外置数据** (unexplored)
  - ITS 描述文件
  - kernel、基础 DTB 与 resource.img
  - 0x800 起始位置、对齐与 64 MiB 分区限制
- ⚪ **resource.img 与 13 份硬件 DTB 的选择** (unexplored)
  - 屏幕变体与硬件 DTB
  - U-Boot 根据屏幕或 SARADC 选择 DTB
  - 为何只改 FIT 基础 DTB 不会生效

## 安全定制镜像

- ⚪ **基线镜像与 SHA-256 身份校验** (unexplored)
  - 为什么固定官方 2026-05-27 multi 镜像
  - 哈希用于确认字节级输入身份
  - 固定偏移只能配合已核验基线
- ⚪ **Rockchip 解包与打包工具的职责** (unexplored)
  - rkImageMaker
  - afptool
  - mkimage、dtc 与 resource\_tool
- ⚪ **DTB 反编译、节点修改与重新编译** (unexplored)
  - dtc 的输入输出格式
  - 启用 CAN1、切换 pinctrl、禁用 I2C3
  - 同步处理基础 DTB 与全部硬件 DTB
- ⚪ **逐段读懂 prepare\_can1m0.sh** (unexplored)
  - 严格模式、临时目录与自动清理
  - 解包、提取、修改、重建、替换与总封装
  - 结构断言、13 份计数、尺寸检查与失败即停止
- ⚪ **重建 FIT、resource.img 与 update.img** (unexplored)
  - 重打 resource.img
  - 根据 boot.its 重建 FIT
  - 替换 boot.img 并封装可刷写镜像
  - 输出哈希与可重复性边界

## 刷机、验收与排错

- ⚪ **Loader、Maskrom 与 MSC 模式** (unexplored)
  - 三种 USB 状态的用途
  - 为什么刷机前必须读取并确认模式
  - Recovery 键与进入 Maskrom 的基本思路
- ⚪ **完整刷写、风险边界与恢复策略** (unexplored)
  - upgrade\_tool LD、UF 与 RD
  - 刷写期间不可断电
  - 保留官方镜像并通过 Maskrom 回刷
  - 为何不随意执行 EF
- ⚪ **读取运行中的设备树并确认驱动探测** (unexplored)
  - sysfs 中 CAN1 与 I2C3 的 status
  - 区分镜像里改了与运行时真正生效
  - 用内核日志和网络接口交叉验证
- ⚪ **SocketCAN 接口命名与 ip link 配置** (unexplored)
  - CAN1 为什么可能注册为 can0
  - down、设置 500000、fd off、restart-ms、up
  - 接口详细状态与错误计数
- ⚪ **candump、cansend 与分层故障定位** (unexplored)
  - 监听 STM32 的 0x101 心跳
  - 发送 0x2FF 并检查 0x2FE 回显
  - 接口不存在、无法 UP、无帧、错误增长的分层排查
