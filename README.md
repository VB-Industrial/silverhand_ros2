# silverhand_ros2

ROS 2 Jazzy workspace for the Silverhand arm lower and middle layers:

- `silverhand_arm_description`: Silverhand robot description, meshes and `ros2_control` xacro files.
- `silverhand_arm_hardware`: `ros2_control` hardware plugins for Cyphal/CAN transport.
- `silverhand_arm_bringup`: `robot_state_publisher`, `ros2_control_node` and `joint_trajectory_controller` bringup.

This repository intentionally does not contain the upper layer:

- no `MoveIt 2`
- no RViz planning setup
- no planning pipelines or MoveIt launch files

That upper layer is expected to live in a separate repository later and depend on these packages.

## Notes

- The robot kinematics and geometry are based on the working Silverhand URDF from `Raspberry_Cyphal_ruka/URDF`.
- The Cyphal transport is vendored as the `third_party/libcxxcanard` git submodule.
- Bringup defaults to `use_mock_hardware:=true` so the stack can be launched without CAN hardware.

## Clone

```bash
git clone --recurse-submodules <repo-url>
```

If the repository is already cloned:

```bash
git submodule update --init --recursive
```

## Example

```bash
ros2 launch silverhand_arm_bringup silverhand_arm_bringup.launch.py
```

For real hardware:

```bash
ros2 launch silverhand_arm_bringup silverhand_arm_bringup.launch.py use_mock_hardware:=false can_iface:=can0 node_id:=100
```
