// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>
#include <memory>

#include "nav2_util/robot_utils.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav2_util/node_utils.hpp"

#include "bt_nav2_ergocub/is_goal_reached.hpp"

namespace ergocub_nav2_nodes
{

GoalReachedConditionModded::GoalReachedConditionModded(
  const std::string & condition_name,
  const BT::NodeConfiguration & conf)
: BT::ConditionNode(condition_name, conf),
  initialized_(false),
  global_frame_("map"),
  robot_base_frame_("base_link")
{
  getInput("global_frame", global_frame_);
  getInput("robot_base_frame", robot_base_frame_);
}

GoalReachedConditionModded::~GoalReachedConditionModded()
{
  cleanup();
}

void GoalReachedConditionModded::initialize()
{
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

  nav2_util::declare_parameter_if_not_declared(
    node_, "goal_reached_tol",
    rclcpp::ParameterValue(0.3));
  node_->get_parameter_or<double>("goal_reached_tol", goal_reached_tol_, 0.20);
  tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");

  node_->get_parameter("transform_tolerance", transform_tolerance_);

  goal_state_pub_=node_->create_publisher<std_msgs::msg::Bool>("/is_goal_reached", 10);

  initialized_ = true;
}

BT::NodeStatus GoalReachedConditionModded::tick()
{
  if (!initialized_) {
    initialize();
  }
  std_msgs::msg::Bool msg;
  if (isGoalReached()) {
    msg.data = true;
    goal_state_pub_->publish(msg);
    RCLCPP_INFO(node_->get_logger(), "[GoalReachedConditionModded] Goal Reached!");
    return BT::NodeStatus::SUCCESS;
  }
    msg.data = false;
    goal_state_pub_->publish(msg);
  return BT::NodeStatus::FAILURE;
}

bool GoalReachedConditionModded::isGoalReached()
{
  geometry_msgs::msg::PoseStamped current_pose;

  if (!nav2_util::getCurrentPose(current_pose, *tf_, global_frame_, robot_base_frame_, transform_tolerance_))
  {
    RCLCPP_DEBUG(node_->get_logger(), "Current robot pose is not available.");
    return false;
  }

  geometry_msgs::msg::PoseStamped goal;
  getInput("goal", goal);
  double dx = goal.pose.position.x - current_pose.pose.position.x;
  double dy = goal.pose.position.y - current_pose.pose.position.y;
  std::cout << "[GoalReachedConditionModded] Goal X: " << goal.pose.position.x << " Y: " << goal.pose.position.y << std::endl;
  std::cout << "[GoalReachedConditionModded] Current Pose X: " << current_pose.pose.position.x << " Y: " << current_pose.pose.position.y << std::endl;
  std::cout << "[GoalReachedConditionModded] Distance: " << std::sqrt(dx * dx + dy * dy) << std::endl;
  std::cout << "[GoalReachedConditionModded] Global Frame: " << global_frame_ << " Robot Frame: " << robot_base_frame_ << std::endl;
  return (dx * dx + dy * dy) <= (goal_reached_tol_ * goal_reached_tol_);
}

}  // namespace ergocub_nav2_nodes

#include "behaviortree_cpp_v3/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<ergocub_nav2_nodes::GoalReachedConditionModded>("GoalReachedConditionModded");
}
