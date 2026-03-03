#pragma once
/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2017, Torc Robotics, LLC
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Torc Robotics, LLC nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <boost/asio.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/lambda/lambda.hpp>

#include <iostream>
#include <iomanip>

#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <vector>

#include "j2735_v2x_msgs/msg/byte_array.hpp"
#include "carma_msgs/msg/system_alert.hpp"
#include "carma_driver_msgs/srv/send_message.hpp"
#include "std_srvs/srv/trigger.hpp"

#include <rclcpp/rclcpp.hpp>
#include <carma_ros2_utils/carma_lifecycle_node.hpp>

#include <map>
#include <set>

#include "v2x_ros_driver/base_radio_client.h"
#include "v2x_ros_driver/udp_radio_client.h"
#include "v2x_ros_driver/mqtt_radio_client.h"
#include "v2x_ros_driver/v2x_ros_driver_config.h"

#include <ament_index_cpp/get_package_share_directory.hpp>


namespace V2XDriverApplication
{

/**
 * @class V2XDriverApplication::Node
 * @brief Is the class responsible for the ROS v2x radio driver.
 *
 * Now uses a polymorphic BaseRadioClient pointer so that the transport
 * (UDP or MQTT) can be selected at runtime via the "protocol" parameter.
 */
class Node : public carma_ros2_utils::CarmaLifecycleNode
{

private:
    struct WaveConfigStruct
    {
        std::string name, psid, dsrc_msg_id, channel, priority;

        WaveConfigStruct(){};
        WaveConfigStruct(const std::string &name,
                         const std::string &psid,
                         const std::string &dsrc_msg_id,
                         const std::string &channel,
                         const std::string &priority) : name(name),
                                         psid(psid),
                                         dsrc_msg_id(dsrc_msg_id),
                                         channel(channel),
                                         priority(priority) {}
    };

public:

    /**
     * \brief Node constructor
     */
    explicit Node(const rclcpp::NodeOptions &);

private:

    // Node configuration
    Config config_;

    // ROS interfaces
    carma_ros2_utils::SubPtr<carma_driver_msgs::msg::ByteArray> comms_sub_;
    carma_ros2_utils::PubPtr<carma_driver_msgs::msg::ByteArray> comms_pub_;
    carma_ros2_utils::ServicePtr<carma_driver_msgs::srv::SendMessage> comms_srv_;

    // New services
    carma_ros2_utils::ServicePtr<std_srvs::srv::Trigger> reconnect_srv_;
    carma_ros2_utils::ServicePtr<std_srvs::srv::Trigger> get_status_srv_;

    std::deque<std::shared_ptr<std::vector<uint8_t>>> send_msg_queue_;
    bool connecting_ = false;
    std::shared_ptr<std::thread> connect_thread_;

    std::vector<WaveConfigStruct> wave_cfg_items_;

    std::unique_ptr<BaseRadioClient> radio_client_;
    boost::system::error_code radio_client_error_;
    uint32_t queue_size_ = 100;

    /**
     * @brief function callback for dynamic parameter updates
     */
    rcl_interfaces::msg::SetParametersResult parameter_update_callback(const std::vector<rclcpp::Parameter> &parameters);

    carma_ros2_utils::CallbackReturn handle_on_configure(const rclcpp_lifecycle::State &);

    /**
     * @brief Called by the base DriverApplication class prior to Spin
     */
    virtual void pre_spin();

    virtual void handle_on_shutdown();

    /**
     * @brief Handles messages received from the radio client
     */
    void onMessageReceivedHandler(const std::vector<uint8_t> &data, uint16_t id);

    /**
     * @brief Packs an outgoing message into WAVE text format (UDP only).
     */
    std::vector<uint8_t> packMessage(carma_driver_msgs::msg::ByteArray message);

    /**
     * @brief Handles outbound messages from the ROS network.
     * Protocol-aware: packs WAVE format for UDP, sends raw for MQTT.
     */
    void onOutboundMessage(carma_driver_msgs::msg::ByteArray::UniquePtr message);

    /**
     * @brief Message sending service
     */
    void sendMessageSrv(const std::shared_ptr<rmw_request_id_t> header,
                        const std::shared_ptr<carma_driver_msgs::srv::SendMessage::Request> req,
                        std::shared_ptr<carma_driver_msgs::srv::SendMessage::Response> res);

    /**
     * @brief Reconnect service callback
     */
    void reconnectSrv(const std::shared_ptr<rmw_request_id_t> header,
                      const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
                      std::shared_ptr<std_srvs::srv::Trigger::Response> res);

    /**
     * @brief Status service callback
     */
    void getStatusSrv(const std::shared_ptr<rmw_request_id_t> header,
                      const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
                      std::shared_ptr<std_srvs::srv::Trigger::Response> res);

    /**
     * @brief Sends a message from the queue of outbound messages
     */
    void sendMessageFromQueue();

    /**
     * @brief Loads the wave file
     */
    void loadWaveConfig(const std::string& fileName);

    /**
     * @brief converts a uint8_t vector to an ascii hex representation
     */
    std::string uint8_vector_to_hex_string(const std::vector<uint8_t>& v);

    /**
     * @brief Creates and configures the appropriate radio client based on config_.protocol
     */
    void createRadioClient();
};

}