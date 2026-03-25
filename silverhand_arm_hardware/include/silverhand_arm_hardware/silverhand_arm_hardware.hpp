#ifndef SILVERHAND_ARM_HARDWARE__SILVERHAND_ARM_HARDWARE_HPP_
#define SILVERHAND_ARM_HARDWARE__SILVERHAND_ARM_HARDWARE_HPP_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/hardware_component_interface.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp_lifecycle/node_interfaces/lifecycle_node_interface.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "silverhand_arm_hardware/visibility_control.h"

namespace silverhand_arm_hardware
{

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

namespace detail
{
struct CyphalRuntime;
}  // namespace detail

class SILVERHAND_ARM_HARDWARE_PUBLIC SilverhandArmSystem
  : public hardware_interface::SystemInterface
{
public:
  ~SilverhandArmSystem() override;

  CallbackReturn on_init(
    const hardware_interface::HardwareComponentInterfaceParams & params) override;

  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_cleanup(const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  CallbackReturn start_transport();

  void stop_transport();

  void spin_transport_once();

  void send_heartbeat();

  void send_joint_command(std::size_t joint_index, float position, float velocity);

  std::vector<double> joint_position_command_;
  std::vector<double> joint_velocity_command_;
  std::vector<double> joint_position_state_;
  std::vector<double> joint_velocity_state_;
  std::unique_ptr<detail::CyphalRuntime> runtime_;
  std::string can_iface_{"can0"};
  std::uint16_t node_id_{100};
  std::size_t queue_len_{1000};
  bool is_active_{false};
};

}  // namespace silverhand_arm_hardware

#endif  // SILVERHAND_ARM_HARDWARE__SILVERHAND_ARM_HARDWARE_HPP_
