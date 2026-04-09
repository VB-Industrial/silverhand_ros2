import os
import yaml

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def load_profile(profile_name):
    package_path = get_package_share_directory("silverhand_arm_control")
    profile_path = os.path.join(package_path, "config", "hardware_profiles.yaml")
    fallback_profiles = {
        "ros_control": {
            "can_iface": "can0",
            "node_id": 100,
            "queue_len": 1000,
        }
    }
    if not os.path.exists(profile_path):
        return fallback_profiles[profile_name]
    with open(profile_path, "r", encoding="utf-8") as file:
        profiles = yaml.safe_load(file)["profiles"]
    return profiles[profile_name]


def generate_launch_description():
    ros_control_profile = load_profile("ros_control")
    use_mock_hardware = LaunchConfiguration("use_mock_hardware")
    can_iface = LaunchConfiguration("can_iface")
    node_id = LaunchConfiguration("node_id")
    queue_len = LaunchConfiguration("queue_len")
    controller_manager_name = "/arm_controller_manager"

    description_file = PathJoinSubstitution(
        [FindPackageShare("silverhand_arm_control"), "urdf", "silverhand.urdf.xacro"]
    )
    controllers_file = PathJoinSubstitution(
        [FindPackageShare("silverhand_arm_control"), "config", "controllers.yaml"]
    )

    robot_description_content = Command(
        [
            PathJoinSubstitution([FindExecutable(name="xacro")]),
            " ",
            description_file,
            " ",
            "use_mock_hardware:=",
            use_mock_hardware,
            " ",
            "can_iface:=",
            can_iface,
            " ",
            "node_id:=",
            node_id,
            " ",
            "queue_len:=",
            queue_len,
        ]
    )
    robot_description = {"robot_description": robot_description_content}

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[robot_description],
    )

    ros2_control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        name="arm_controller_manager",
        output="screen",
        parameters=[robot_description, controllers_file],
        remappings=[("/controller_manager/robot_description", "/robot_description")],
    )

    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_state_broadcaster", "--controller-manager", controller_manager_name],
        output="screen",
    )

    arm_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["arm_controller", "--controller-manager", controller_manager_name],
        output="screen",
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "use_mock_hardware",
                default_value="true",
                description="Use ros2_control mock hardware instead of the Cyphal transport.",
            ),
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
            robot_state_publisher,
            ros2_control_node,
            joint_state_broadcaster_spawner,
            arm_controller_spawner,
        ]
    )
