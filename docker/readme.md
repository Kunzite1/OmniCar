# 板卡humble容器
RK3568-K1MINI板卡，厂商仅提供UBUNTU20版本镜像，需要在板卡里跑一个ROS2-HUMBLE容器。
在板卡使用以下指令获取官方humble镜像
```bash
docker pull ros:humble-ros-base
```
然后使用 `docker run` 启动容器，注意使用 `-v`、`--rm`、`--network host` 等参数。
---

# PC端容器（用于编译内核相关）
- 暂无
