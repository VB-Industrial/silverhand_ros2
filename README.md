# silverhand_arm_control

ROS 2 Jazzy package for the Silverhand arm control layer.

Package:
- `silverhand_arm_control`

This repository intentionally contains only the lower and middle control layers:
- `ros2_control`
- hardware interface
- controller bringup

Robot geometry, meshes, and RViz model viewing now live in `silverhand_arm_model`.
Upper-level planning remains in `silverhand_system_bringup`.

## Prerequisites

- Ubuntu `24.04`
- ROS 2 `Jazzy`
- `libcxxcanard` cloned into the same workspace

Install required ROS packages:

```bash
sudo apt-get update
sudo apt-get install -y \
  ros-jazzy-ros2-control \
  ros-jazzy-controller-manager \
  ros-jazzy-joint-trajectory-controller \
  ros-jazzy-joint-state-broadcaster \
  ros-jazzy-robot-state-publisher \
  ros-jazzy-hardware-interface \
  ros-jazzy-pluginlib \
  ros-jazzy-rclcpp \
  ros-jazzy-rclcpp-lifecycle \
  ros-jazzy-xacro \
  ros-jazzy-urdf
```

## Clone

Clone into a workspace that already contains `libcxxcanard`:

```bash
cd ~/silver_ws/src
git clone https://github.com/VB-Industrial/libcxxcanard.git
git clone <repo-url>
```

`libcxxcanard` should live next to the package:

```bash
/home/r/silver_ws/src/libcxxcanard
```

## Notes

- `libcxxcanard` is a separate workspace prerequisite.
- Bringup defaults to `use_mock_hardware:=true`, so the stack can be launched without CAN hardware.
- The control description includes `silverhand_arm_model` and appends the `ros2_control` system block.

## Workspace Layout

This repository can be built standalone:

```bash
/home/r/projects/silverhand_arm_control
```

or together with upper-level repositories inside a common workspace:

```bash
/home/r/silver_ws/src/silverhand_arm_control
/home/r/silver_ws/src/silverhand_arm_model
/home/r/silver_ws/src/silverhand_system_bringup
```

## Build

Standalone:

```bash
cd /home/r/projects/silverhand_arm_control
source /opt/ros/jazzy/setup.bash
colcon build
source install/setup.bash
```

Shared workspace:

```bash
cd /home/r/silver_ws
source /opt/ros/jazzy/setup.bash
colcon build
source install/setup.bash
```

## Packages Check

```bash
ros2 pkg list | rg silverhand
```

Expected packages from this repository:
- `silverhand_arm_control`

## Launch

Mock hardware:

```bash
ros2 launch silverhand_arm_control silverhand_arm_bringup.launch.py
```

Equivalent explicit mock launch:

```bash
ros2 launch silverhand_arm_control silverhand_arm_bringup.launch.py use_mock_hardware:=true
```

Real hardware:

```bash
ros2 launch silverhand_arm_control silverhand_arm_bringup.launch.py use_mock_hardware:=false can_iface:=can0 node_id:=100
```

## Helper scripts

Для типового запуска есть helper-скрипты:

```bash
cd /home/r/silver_ws/src/silverhand_arm_control
./scripts/start_arm_mock.sh
./scripts/start_arm_real.sh
```

Поддерживаемые переменные окружения:

- `ROS_WS` — путь к workspace, по умолчанию определяется как `~/silver_ws`
- `ROS_DISTRO` — по умолчанию `jazzy`
- `SILVERHAND_ARM_CAN_IFACE` — по умолчанию `can0`
- `SILVERHAND_ARM_NODE_ID` — по умолчанию `100`

## systemd

В репозитории лежит user-service template:

- `systemd/user/silverhand-arm-control@.service`

Экземпляры:

- `mock`
- `real`

Установка:

```bash
mkdir -p ~/.config/systemd/user
cp /home/r/silver_ws/src/silverhand_arm_control/systemd/user/silverhand-arm-control@.service ~/.config/systemd/user/
systemctl --user daemon-reload
```

Запуск:

```bash
systemctl --user enable --now silverhand-arm-control@mock.service
```

или:

```bash
systemctl --user enable --now silverhand-arm-control@real.service
```

Автозапуск без логина:

```bash
loginctl enable-linger "$USER"
```

Полезные команды:

```bash
systemctl --user status silverhand-arm-control@mock.service
journalctl --user -u silverhand-arm-control@mock.service -f
systemctl --user restart silverhand-arm-control@mock.service
systemctl --user disable --now silverhand-arm-control@mock.service
```

## Parameters

Launch arguments:
- `use_mock_hardware`
- `can_iface`
- `node_id`

Defaults:
- `use_mock_hardware:=true`
- `can_iface:=can0`
- `node_id:=100`

## Integration

Upper-level packages such as `silverhand_system_bringup` depend on:
- `silverhand_arm_model`
- `silverhand_arm_control`

If built separately, source order must be:

```bash
source /opt/ros/jazzy/setup.bash
source /path/to/silverhand_arm_model/install/setup.bash
source /path/to/silverhand_arm_control/install/setup.bash
source /path/to/upper_layer/install/setup.bash
```
