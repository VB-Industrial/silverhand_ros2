import os
import yaml

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def load_profile(profile_name):
    package_path = get_package_share_directory("silverhand_arm_control")
    profile_path = os.path.join(package_path, "config", "hardware_profiles.yaml")
    with open(profile_path, "r", encoding="utf-8") as file:
        profiles = yaml.safe_load(file)["profiles"]
    return profiles[profile_name]


def generate_launch_description():
    ros_control_profile = load_profile("ros_control")
    can_iface = LaunchConfiguration("can_iface")
    node_id = LaunchConfiguration("node_id")
    queue_len = LaunchConfiguration("queue_len")

    base_launch = PathJoinSubstitution(
        [FindPackageShare("silverhand_arm_control"), "launch", "silverhand_arm_bringup.launch.py"]
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "can_iface",
                default_value=str(ros_control_profile["can_iface"]),
                description="Linux CAN interface used by the Cyphal transport.",
            ),
            DeclareLaunchArgument(
                "node_id",
                default_value=str(ros_control_profile["node_id"]),
                description="Cyphal node id for the ros2_control hardware node.",
            ),
            DeclareLaunchArgument(
                "queue_len",
                default_value=str(ros_control_profile["queue_len"]),
                description="Cyphal queue length for the ros2_control hardware node.",
            ),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(base_launch),
                launch_arguments={
                    "use_mock_hardware": "false",
                    "can_iface": can_iface,
                    "node_id": node_id,
                    "queue_len": queue_len,
                }.items(),
            ),
        ]
    )
