# car_control

`car_control` contains the K1 Mini side of OmniCar control. The current scope is
the CAN communication layer; motion and vehicle business logic will be added as
separate components later.

## Architecture

- `CanFrame.msg` is the ROS boundary. Producers publish frames to
  `/car_control/can/tx`; received frames are published on `/car_control/can/rx`.
- `BoundedQueue<car_can_frame_t>` is the only path into hardware transmission.
  The ROS subscription and future in-process business components call
  `enqueue_tx()`, while one I/O worker owns and drains the queue.
- `can_protocol.c` is a C11 codec for the DaMiao USB-CAN CDC protocol: 30-byte
  transmit frames, 16-byte reports, and the CAN bitrate command.
- `CanCommunicationNode` exclusively owns `/dev/ttyACM0`; other code must not
  open the device directly.

The default adapter serial rate is 921600 baud. CAN bitrate index `3` selects
500 kbit/s. The node accepts standard/extended data or remote frames with a
maximum DLC of eight.

Connect STM32 `PA12/CAN_TX` to the transceiver `CTX/TXD` input and
`PA11/CAN_RX` to `CRX/RXD`; these signals are not crossed like a UART. Connect
CAN_H to CAN_H, CAN_L to CAN_L, and share ground between all three devices.
Power the transceiver at the voltage required by its part and keep any EN/STB
pin in normal mode. With both end terminators fitted, the unpowered resistance
between CAN_H and CAN_L is approximately 60 ohms.

## Build in the K1 Mini Humble Container

From the K1 Mini host:

```sh
docker run --rm -it --network host \
  --device=/dev/ttyACM0:/dev/ttyACM0 \
  -v /root/OmniCar/ros2_ws:/ws -w /ws \
  ros:humble-ros-base bash
```

Inside the container:

```sh
source /opt/ros/humble/setup.bash
colcon build --packages-select car_control
source install/setup.bash
ros2 run car_control can_communication_node \
  --ros-args --params-file src/car_control/config/can.yaml
```

## Wiring Test

The STM32 sends heartbeat ID `0x101` once per second. Observe it with:

```sh
ros2 topic echo /car_control/can/rx
```

Send an eight-byte echo request (`0x2FF`, decimal 767) through the TX queue:

```sh
ros2 topic pub --once /car_control/can/tx car_control/msg/CanFrame \
  "{id: 767, dlc: 8, is_extended: false, is_remote: false, \
  data: [66, 75, 49, 83, 84, 77, 51, 50]}"
```

A valid bidirectional link produces response ID `0x2FE` (decimal 766) with the
same payload. The node status line must show increasing `heartbeat` and `echo`,
with zero `queue_drop`, `write_err`, and `adapter_err`.

Run the hardware-independent codec and queue tests with:

```sh
colcon test --packages-select car_control
colcon test-result --verbose
```
