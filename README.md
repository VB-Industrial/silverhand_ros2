# silverhand_ros2

ROS 2 Jazzy workspace for the Silverhand arm lower and middle layers.

Packages:
- `silverhand_arm_description`
- `silverhand_arm_hardware`
- `silverhand_arm_bringup`

This repository intentionally contains only the lower and middle layers:
- robot description
- `ros2_control`
- hardware interface
- controller bringup

It does not contain:
- `MoveIt 2`
- RViz planning setup
- upper-level planning launch files

Those are expected to live in a separate repository such as `silverhand_moveit2`.

## Prerequisites

- Ubuntu `24.04`
- ROS 2 `Jazzy`
- git submodules enabled

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

Clone with submodules:

```bash
git clone --recurse-submodules <repo-url>
```

If the repository is already cloned:

```bash
git submodule update --init --recursive
```

## Notes

- The robot kinematics and geometry are based on the working Silverhand URDF from `Raspberry_Cyphal_ruka/URDF`.
- The Cyphal transport is vendored as the `third_party/libcxxcanard` git submodule.
- Bringup defaults to `use_mock_hardware:=true`, so the stack can be launched without CAN hardware.

## Workspace Layout

This repository can be built standalone:

```bash
/home/r/projects/silverhand_ros2
```

or together with upper-level repositories inside a common workspace:

```bash
/home/r/silver_ws/src/silverhand_ros2
/home/r/silver_ws/src/silverhand_moveit2
```

## Build

Standalone:

```bash
cd /home/r/projects/silverhand_ros2
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
- `silverhand_arm_description`
- `silverhand_arm_hardware`
- `silverhand_arm_bringup`

## Launch

Mock hardware:

```bash
ros2 launch silverhand_arm_bringup silverhand_arm_bringup.launch.py
```

Equivalent explicit mock launch:

```bash
ros2 launch silverhand_arm_bringup silverhand_arm_bringup.launch.py use_mock_hardware:=true
```

Real hardware:

```bash
ros2 launch silverhand_arm_bringup silverhand_arm_bringup.launch.py use_mock_hardware:=false can_iface:=can0 node_id:=100
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

Upper-level packages such as `silverhand_moveit2` depend on:
- `silverhand_arm_description`
- `silverhand_arm_bringup`

If built separately, source order must be:

```bash
source /opt/ros/jazzy/setup.bash
source /path/to/silverhand_ros2/install/setup.bash
source /path/to/upper_layer/install/setup.bash
```
