#include "silverhand_arm_control/silverhand_arm_hardware.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"
#include "silverhand_arm_control/joint_ports.hpp"

#include "cyphal/allocators/o1/o1_allocator.h"
#include "cyphal/cyphal.h"
#include "cyphal/providers/LinuxCAN.h"
#include "cyphal/subscriptions/subscription.h"
#include "reg/udral/physics/kinematics/rotation/Planar_0_1.h"
#include "uavcan/node/Heartbeat_1_0.h"

TYPE_ALIAS(HBeat, uavcan_node_Heartbeat_1_0)
TYPE_ALIAS(JointStateMsg, reg_udral_physics_kinematics_rotation_Planar_0_1)

namespace
{

void error_handler()
{
  std::exit(EXIT_FAILURE);
}

std::uint64_t micros_64()
{
  using namespace std::chrono;
  return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

UtilityConfig g_utilities(micros_64, error_handler);

std::string get_string_param(
  const hardware_interface::HardwareInfo & info,
  const std::string & key,
  const std::string & default_value)
{
  const auto it = info.hardware_parameters.find(key);
  if (it == info.hardware_parameters.end()) {
    return default_value;
  }
  return it->second;
}

std::uint16_t get_uint16_param(
  const hardware_interface::HardwareInfo & info,
  const std::string & key,
  const std::uint16_t default_value)
{
  const auto it = info.hardware_parameters.find(key);
  if (it == info.hardware_parameters.end()) {
    return default_value;
  }

  try {
    return static_cast<std::uint16_t>(std::stoi(it->second));
  } catch (const std::exception &) {
    RCLCPP_WARN(
      rclcpp::get_logger("silverhand_arm_control"),
      "Failed to parse hardware parameter '%s', using default value %u",
      key.c_str(), default_value);
    return default_value;
  }
}

std::size_t get_size_t_param(
  const hardware_interface::HardwareInfo & info,
  const std::string & key,
  const std::size_t default_value)
{
  const auto it = info.hardware_parameters.find(key);
  if (it == info.hardware_parameters.end()) {
    return default_value;
  }

  try {
    return static_cast<std::size_t>(std::stoul(it->second));
  } catch (const std::exception &) {
    RCLCPP_WARN(
      rclcpp::get_logger("silverhand_arm_control"),
      "Failed to parse hardware parameter '%s', using default value %zu",
      key.c_str(), default_value);
    return default_value;
  }
}

}  // namespace

namespace silverhand_arm_control::detail
{

class JointStateReader;

struct CyphalRuntime
{
  std::shared_ptr<CyphalInterface> interface;
  std::unique_ptr<JointStateReader> joint_state_reader;
  std::array<float, kJointCount> joint_position{};
  std::array<float, kJointCount> joint_velocity{};
  std::uint32_t uptime{0};
  std::size_t heartbeat_counter{0};
  CanardTransferID heartbeat_transfer_id{0};
  CanardTransferID command_transfer_id{0};
};

class JointStateReader : public AbstractSubscription<JointStateMsg>
{
public:
  JointStateReader(const InterfacePtr & interface, CyphalRuntime * runtime)
  : AbstractSubscription<JointStateMsg>(interface, silverhand_arm_control::kAgentJointStatePort),
    runtime_(runtime)
  {
  }

  void handler(
    const reg_udral_physics_kinematics_rotation_Planar_0_1 & message,
    CanardRxTransfer * transfer) override
  {
    if (runtime_ == nullptr) {
      return;
    }

    const auto remote_node_id = transfer->metadata.remote_node_id;
    if (remote_node_id == 0 || remote_node_id > silverhand_arm_control::kJointCount) {
      return;
    }

    const auto index = static_cast<std::size_t>(remote_node_id - 1);
    runtime_->joint_position[index] = message.angular_position.radian;
    runtime_->joint_velocity[index] = message.angular_velocity.radian_per_second;
  }

private:
  CyphalRuntime * runtime_;
};

}  // namespace silverhand_arm_control::detail

namespace silverhand_arm_control
{

SilverhandArmSystem::~SilverhandArmSystem() = default;

CallbackReturn SilverhandArmSystem::on_init(
  const hardware_interface::HardwareInfo & hardware_info)
{
  if (hardware_interface::SystemInterface::on_init(hardware_info) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }

  if (info_.joints.size() != kJointCount) {
    RCLCPP_ERROR(
      get_logger(),
      "Expected %zu joints for SilverhandArmSystem, got %zu",
      kJointCount, info_.joints.size());
    return CallbackReturn::ERROR;
  }

  can_iface_ = get_string_param(info_, "can_iface", "vcan1.0");
  node_id_ = get_uint16_param(info_, "node_id", 100);
  queue_len_ = get_size_t_param(info_, "queue_len", 1000);

  const auto joint_count = info_.joints.size();
  joint_position_command_.assign(joint_count, 0.0);
  joint_velocity_command_.assign(joint_count, 0.0);
  joint_position_state_.assign(joint_count, 0.0);
  joint_velocity_state_.assign(joint_count, 0.0);
  runtime_ = std::make_unique<detail::CyphalRuntime>();
  is_active_ = false;

  RCLCPP_INFO(
    get_logger(),
    "Initialized hardware '%s' with can_iface=%s node_id=%u queue_len=%zu",
    info_.name.c_str(), can_iface_.c_str(), node_id_, queue_len_);

  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandArmSystem::start_transport()
{
  stop_transport();

  try {
    runtime_ = std::make_unique<detail::CyphalRuntime>();
    runtime_->interface = CyphalInterface::create_heap<LinuxCAN, O1Allocator>(
      node_id_, can_iface_.c_str(), queue_len_, g_utilities);
    runtime_->joint_state_reader =
      std::make_unique<detail::JointStateReader>(runtime_->interface, runtime_.get());
  } catch (const std::exception & error) {
    RCLCPP_ERROR(get_logger(), "Failed to start Cyphal transport: %s", error.what());
    stop_transport();
    return CallbackReturn::ERROR;
  }

  if (!runtime_->interface || !runtime_->interface->is_up()) {
    RCLCPP_ERROR(
      get_logger(), "Cyphal transport is not available on interface '%s'", can_iface_.c_str());
    stop_transport();
    return CallbackReturn::ERROR;
  }

  RCLCPP_INFO(
    get_logger(),
    "Started Cyphal transport for '%s' on %s with node_id=%u",
    info_.name.c_str(), can_iface_.c_str(), node_id_);
  return CallbackReturn::SUCCESS;
}

void SilverhandArmSystem::stop_transport()
{
  if (runtime_) {
    runtime_->joint_state_reader.reset();
    runtime_->interface.reset();
  }
}

CallbackReturn SilverhandArmSystem::on_configure(const rclcpp_lifecycle::State & /*previous_state*/)
{
  return start_transport();
}

CallbackReturn SilverhandArmSystem::on_activate(const rclcpp_lifecycle::State & /*previous_state*/)
{
  if (!runtime_ || !runtime_->interface) {
    RCLCPP_ERROR(get_logger(), "Cannot activate hardware '%s' without transport", info_.name.c_str());
    return CallbackReturn::ERROR;
  }

  runtime_->heartbeat_counter = 0;
  runtime_->uptime = 0;
  runtime_->heartbeat_transfer_id = 0;
  runtime_->command_transfer_id = 0;
  is_active_ = true;
  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandArmSystem::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  is_active_ = false;
  return CallbackReturn::SUCCESS;
}

CallbackReturn SilverhandArmSystem::on_cleanup(const rclcpp_lifecycle::State & /*previous_state*/)
{
  is_active_ = false;
  stop_transport();
  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> SilverhandArmSystem::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  state_interfaces.reserve(info_.joints.size() * 2U);

  for (std::size_t i = 0; i < info_.joints.size(); ++i) {
    state_interfaces.emplace_back(
      info_.joints[i].name,
      hardware_interface::HW_IF_POSITION,
      &joint_position_state_[i]);
    state_interfaces.emplace_back(
      info_.joints[i].name,
      hardware_interface::HW_IF_VELOCITY,
      &joint_velocity_state_[i]);
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> SilverhandArmSystem::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
  command_interfaces.reserve(info_.joints.size() * 2U);

  for (std::size_t i = 0; i < info_.joints.size(); ++i) {
    command_interfaces.emplace_back(
      info_.joints[i].name,
      hardware_interface::HW_IF_POSITION,
      &joint_position_command_[i]);
    command_interfaces.emplace_back(
      info_.joints[i].name,
      hardware_interface::HW_IF_VELOCITY,
      &joint_velocity_command_[i]);
  }

  return command_interfaces;
}

void SilverhandArmSystem::spin_transport_once()
{
  if (runtime_ && runtime_->interface) {
    runtime_->interface->loop();
  }
}

void SilverhandArmSystem::send_heartbeat()
{
  if (!runtime_ || !runtime_->interface) {
    return;
  }

  HBeat::Type heartbeat_msg = {
    .uptime = runtime_->uptime,
    .health = {uavcan_node_Health_1_0_NOMINAL},
    .mode = {uavcan_node_Mode_1_0_OPERATIONAL}
  };

  runtime_->interface->send_msg<HBeat>(
    &heartbeat_msg,
    uavcan_node_Heartbeat_1_0_FIXED_PORT_ID_,
    &runtime_->heartbeat_transfer_id);
  ++runtime_->uptime;
}

void SilverhandArmSystem::send_joint_command(
  const std::size_t joint_index, const float position, const float velocity)
{
  if (!runtime_ || !runtime_->interface || joint_index >= kJointCount) {
    return;
  }

  ++runtime_->command_transfer_id;
  reg_udral_physics_kinematics_rotation_Planar_0_1 command = {
    .angular_position = position,
    .angular_velocity = velocity,
    .angular_acceleration = 0.0F
  };

  runtime_->interface->send_msg<JointStateMsg>(
    &command,
    kJointCommandPorts[joint_index],
    &runtime_->command_transfer_id);
}

hardware_interface::return_type SilverhandArmSystem::read(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!runtime_ || !runtime_->interface) {
    return hardware_interface::return_type::ERROR;
  }

  spin_transport_once();

  const auto copy_count = std::min(joint_position_state_.size(), runtime_->joint_position.size());
  for (std::size_t i = 0; i < copy_count; ++i) {
    joint_position_state_[i] = runtime_->joint_position[i];
    joint_velocity_state_[i] = runtime_->joint_velocity[i];
  }

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type SilverhandArmSystem::write(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!is_active_ || !runtime_ || !runtime_->interface) {
    return hardware_interface::return_type::OK;
  }

  for (std::size_t i = 0; i < joint_position_command_.size() && i < kJointCount; ++i) {
    send_joint_command(
      i,
      static_cast<float>(joint_position_command_[i]),
      static_cast<float>(joint_velocity_command_[i]));
  }

  ++runtime_->heartbeat_counter;
  if (runtime_->heartbeat_counter >= 1000U) {
    send_heartbeat();
    runtime_->heartbeat_counter = 0U;
  }

  spin_transport_once();

  return hardware_interface::return_type::OK;
}

}  // namespace silverhand_arm_control

PLUGINLIB_EXPORT_CLASS(
  silverhand_arm_control::SilverhandArmSystem,
  hardware_interface::SystemInterface)
