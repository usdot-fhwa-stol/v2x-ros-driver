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

#include "v2x_ros_driver/v2x_ros_driver_node.h"
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/schema.h>
#include <fstream>

namespace V2XDriverApplication
{

namespace std_ph = std::placeholders;

Node::Node(const rclcpp::NodeOptions &options)
    : carma_ros2_utils::CarmaLifecycleNode(options)
{
    config_ = Config();

    // ── Existing parameters ──
    config_.listening_port = declare_parameter<int>("listening_port", config_.listening_port);
    config_.v2x_radio_listening_port = declare_parameter<int>("v2x_radio_listening_port", config_.v2x_radio_listening_port);
    config_.v2x_radio_address = declare_parameter<std::string>("v2x_radio_address", config_.v2x_radio_address);

    // ── New parameters ──
    config_.protocol = declare_parameter<std::string>("protocol", config_.protocol);
    config_.broker_address = declare_parameter<std::string>("broker_address", config_.broker_address);
    config_.broker_port = declare_parameter<int>("broker_port", config_.broker_port);
    config_.client_id = declare_parameter<std::string>("client_id", config_.client_id);
    config_.qos_default = declare_parameter<int>("qos_default", config_.qos_default);
    config_.reconnect_interval_sec = declare_parameter<int>("reconnect_interval_sec", config_.reconnect_interval_sec);
    config_.mqtt_topic_prefix = declare_parameter<std::string>("mqtt_topic_prefix", config_.mqtt_topic_prefix);
}

rcl_interfaces::msg::SetParametersResult Node::parameter_update_callback(const std::vector<rclcpp::Parameter> &parameters)
{
    auto error   = update_params<int>({{"listening_port", config_.listening_port}}, parameters);
    auto error_2 = update_params<int>({{"v2x_radio_listening_port", config_.v2x_radio_listening_port}}, parameters);
    auto error_3 = update_params<std::string>({{"v2x_radio_address", config_.v2x_radio_address}}, parameters);
    auto error_4 = update_params<std::string>({{"protocol", config_.protocol}}, parameters);
    auto error_5 = update_params<std::string>({{"broker_address", config_.broker_address}}, parameters);
    auto error_6 = update_params<int>({{"broker_port", config_.broker_port}}, parameters);
    auto error_7 = update_params<std::string>({{"client_id", config_.client_id}}, parameters);
    auto error_8 = update_params<int>({{"qos_default", config_.qos_default}}, parameters);
    auto error_9 = update_params<int>({{"reconnect_interval_sec", config_.reconnect_interval_sec}}, parameters);
    auto error_10 = update_params<std::string>({{"mqtt_topic_prefix", config_.mqtt_topic_prefix}}, parameters);

    rcl_interfaces::msg::SetParametersResult result;
    result.successful = !error && !error_2 && !error_3 && !error_4 && !error_5
                     && !error_6 && !error_7 && !error_8 && !error_9 && !error_10;
    return result;
}


std::string Node::uint8_vector_to_hex_string(const std::vector<uint8_t>& v) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto it = v.begin(); it != v.end(); it++) {
        ss << std::setw(2) << static_cast<unsigned>(*it);
    }
    return ss.str();
}

// ────────────────────────────────────────────────────────────────
//  Radio client factory
// ────────────────────────────────────────────────────────────────

void Node::createRadioClient()
{
    if (config_.protocol == "mqtt")
    {
        RCLCPP_INFO(this->get_logger(), "Creating MQTT radio client");
        auto mqtt_client = std::make_unique<MqttRadioClient>();
        mqtt_client->setMqttConfig(config_.client_id,
                                   config_.qos_default,
                                   config_.reconnect_interval_sec,
                                   config_.mqtt_topic_prefix);
        radio_client_ = std::move(mqtt_client);
    }
    else
    {
        RCLCPP_INFO(this->get_logger(), "Creating UDP radio client");
        radio_client_ = std::make_unique<UdpRadioClient>();
    }
}

// ────────────────────────────────────────────────────────────────
//  Lifecycle: configure
// ────────────────────────────────────────────────────────────────

carma_ros2_utils::CallbackReturn Node::handle_on_configure(const rclcpp_lifecycle::State &) {

    RCLCPP_INFO_STREAM(this->get_logger(), "v2x_ros_driver trying to configure");

    config_ = Config();

    // Load all parameters
    get_parameter<int>("listening_port", config_.listening_port);
    get_parameter<int>("v2x_radio_listening_port", config_.v2x_radio_listening_port);
    get_parameter<std::string>("v2x_radio_address", config_.v2x_radio_address);
    get_parameter<std::string>("protocol", config_.protocol);
    get_parameter<std::string>("broker_address", config_.broker_address);
    get_parameter<int>("broker_port", config_.broker_port);
    get_parameter<std::string>("client_id", config_.client_id);
    get_parameter<int>("qos_default", config_.qos_default);
    get_parameter<int>("reconnect_interval_sec", config_.reconnect_interval_sec);
    get_parameter<std::string>("mqtt_topic_prefix", config_.mqtt_topic_prefix);

    RCLCPP_INFO_STREAM(this->get_logger(), "Loaded params: " << config_);

    // Create the appropriate radio client based on protocol
    createRadioClient();

    pre_spin();

    std::string package_share_directory = ament_index_cpp::get_package_share_directory("v2x_ros_driver");
    std::string wave_cfg_file = package_share_directory + "/etc/wave.json";
    loadWaveConfig(wave_cfg_file);

    add_on_set_parameters_callback(std::bind(&Node::parameter_update_callback, this, std_ph::_1));

    // Configure radio client with wave config
    radio_client_->set_wave_file_path(wave_cfg_file);

    // Setup connection handlers
    radio_client_error_.clear();
    radio_client_->onConnect.connect([this](){});
    radio_client_->onDisconnect.connect([this](){});
    radio_client_->onError.connect([this](const boost::system::error_code& err){ radio_client_error_ = err; });

    // Comms Subscriber
    comms_sub_ = create_subscription<carma_driver_msgs::msg::ByteArray>(
        "outbound_binary_msg", queue_size_,
        std::bind(&Node::onOutboundMessage, this, std_ph::_1));

    // Comms Publisher
    comms_pub_ = create_publisher<carma_driver_msgs::msg::ByteArray>("inbound_binary_msg", queue_size_);

    // Existing send service
    comms_srv_ = create_service<carma_driver_msgs::srv::SendMessage>(
        "send", std::bind(&Node::sendMessageSrv, this, std_ph::_1, std_ph::_2, std_ph::_3));

    // New services
    reconnect_srv_ = create_service<std_srvs::srv::Trigger>(
        "v2x_driver/reconnect", std::bind(&Node::reconnectSrv, this, std_ph::_1, std_ph::_2, std_ph::_3));
    get_status_srv_ = create_service<std_srvs::srv::Trigger>(
        "v2x_driver/get_status", std::bind(&Node::getStatusSrv, this, std_ph::_1, std_ph::_2, std_ph::_3));

    // Wire up message received signal
    radio_client_->onMessageReceived.connect(
        [this](std::vector<uint8_t> const &msg, uint16_t id) { onMessageReceivedHandler(msg, id); });

    sendMessageFromQueue();

    return CallbackReturn::SUCCESS;
}

// ────────────────────────────────────────────────────────────────
//  Inbound message handler
// ────────────────────────────────────────────────────────────────

void Node::onMessageReceivedHandler(const std::vector<uint8_t> &data, uint16_t id) {
    auto it = std::find_if(wave_cfg_items_.begin(), wave_cfg_items_.end(),
        [id](const WaveConfigStruct& entry) {
            return entry.dsrc_msg_id == std::to_string(id);
        });

    carma_driver_msgs::msg::ByteArray msg;
    msg.header.stamp = this->now();
    msg.header.frame_id = "";
    msg.message_type = it != wave_cfg_items_.end() ? it->name : "Unknown";
    msg.content = data;
    comms_pub_->publish(msg);

    RCLCPP_DEBUG_STREAM(this->get_logger(),
        "Application received Data: " << data.size() << " bytes, message: "
        << uint8_vector_to_hex_string(data));
}

// ────────────────────────────────────────────────────────────────
//  Outbound message packing (UDP / WAVE format only)
// ────────────────────────────────────────────────────────────────

std::vector<uint8_t> Node::packMessage(carma_driver_msgs::msg::ByteArray message) {
    std::stringstream ss;
    auto wave_item = std::find_if(wave_cfg_items_.begin(), wave_cfg_items_.end(),
        [&message](const WaveConfigStruct& entry) {
            return entry.name == message.message_type;
        });

    WaveConfigStruct cfg;
    if (wave_item == wave_cfg_items_.end())
    {
        RCLCPP_WARN_STREAM(this->get_logger(),
            "No wave config entry for type: " << message.message_type << ", using defaults");
        cfg.name = message.message_type;
        cfg.channel = "CCH";
        cfg.priority = "1";
        cfg.dsrc_msg_id = std::to_string((message.content[0] << 8) | message.content[1]);
        cfg.psid = cfg.dsrc_msg_id;
    }
    else
    {
        cfg = *wave_item;
    }

    ss << "Version=0.7" << std::endl;
    ss << "Type=" << cfg.name << std::endl;
    ss << "PSID=" << cfg.psid << std::endl;
    ss << "Priority=" << cfg.priority << std::endl;
    ss << "TxMode=ALT" << std::endl;
    ss << "TxChannel=" << cfg.channel << std::endl;
    ss << "TxInterval=0" << std::endl;
    ss << "DeliveryStart=" << std::endl;
    ss << "DeliveryStop=" << std::endl;
    ss << "Signature=False" << std::endl;
    ss << "Encryption=False" << std::endl;
    ss << "Payload=" << uint8_vector_to_hex_string(message.content) << std::endl;

    std::string str = ss.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

// ────────────────────────────────────────────────────────────────
//  Outbound message handler (protocol-aware)
// ────────────────────────────────────────────────────────────────

void Node::onOutboundMessage(carma_driver_msgs::msg::ByteArray::UniquePtr message) {
    if (!radio_client_->connected())
    {
        RCLCPP_WARN_STREAM(this->get_logger(),
            "Outbound message received but node is not connected to V2X Radio");
        return;
    }

    std::shared_ptr<std::vector<uint8_t>> message_content;

    if (config_.protocol == "mqtt")
    {
        // MQTT: send raw UPER-encoded content (MqttRadioClient determines topic from DSRCmsgID)
        message_content = std::make_shared<std::vector<uint8_t>>(
            message->content.begin(), message->content.end());
    }
    else
    {
        // UDP: pack into WAVE text format (existing behavior)
        message_content = std::make_shared<std::vector<uint8_t>>(
            std::move(packMessage(*message)));
    }

    send_msg_queue_.push_back(std::move(message_content));
    sendMessageFromQueue();
}

// ────────────────────────────────────────────────────────────────
//  Message queue processing
// ────────────────────────────────────────────────────────────────

void Node::sendMessageFromQueue() {
    if (!send_msg_queue_.empty()) {
        RCLCPP_DEBUG_STREAM(this->get_logger(),
            "Sending message: " << std::string(
                send_msg_queue_.front()->begin(), send_msg_queue_.front()->end()));
        bool success = radio_client_->sendV2xMessage(send_msg_queue_.front());
        send_msg_queue_.pop_front();
        if (!success) {
            RCLCPP_WARN_STREAM(this->get_logger(), "Message send failed");
        } else {
            RCLCPP_DEBUG_STREAM(this->get_logger(), "Message successfully sent from queue");
        }
    }
}

// ────────────────────────────────────────────────────────────────
//  Service handlers
// ────────────────────────────────────────────────────────────────

void Node::sendMessageSrv(const std::shared_ptr<rmw_request_id_t> header,
                           const std::shared_ptr<carma_driver_msgs::srv::SendMessage::Request> req,
                           const std::shared_ptr<carma_driver_msgs::srv::SendMessage::Response> res)
{
    if (!radio_client_->connected())
    {
        RCLCPP_WARN_STREAM(this->get_logger(),
            "Outbound message received but node is not connected to V2X Radio");
        res->error_status = 1;
        return;
    }

    const carma_driver_msgs::msg::ByteArray::UniquePtr message =
        carma_driver_msgs::msg::ByteArray::UniquePtr(
            new carma_driver_msgs::msg::ByteArray(req->message_to_send));

    std::shared_ptr<std::vector<uint8_t>> message_data;

    if (config_.protocol == "mqtt")
    {
        message_data = std::make_shared<std::vector<uint8_t>>(
            message->content.begin(), message->content.end());
    }
    else
    {
        message_data = std::make_shared<std::vector<uint8_t>>(
            std::move(packMessage(*message)));
    }

    bool success = radio_client_->sendV2xMessage(message_data);
    if (success) {
        RCLCPP_DEBUG_STREAM(this->get_logger(), "SendMessage service returned success");
        res->error_status = 0;
    } else {
        RCLCPP_WARN_STREAM(this->get_logger(), "SendMessage service returned failure");
        res->error_status = 1;
    }
}

void Node::reconnectSrv(const std::shared_ptr<rmw_request_id_t> /*header*/,
                         const std::shared_ptr<std_srvs::srv::Trigger::Request> /*req*/,
                         std::shared_ptr<std_srvs::srv::Trigger::Response> res)
{
    RCLCPP_INFO(this->get_logger(), "Reconnect service called");
    radio_client_->close();
    radio_client_error_.clear();
    connecting_ = false;  // Allow pre_spin to reconnect
    pre_spin();
    res->success = true;
    res->message = "Reconnect initiated for protocol: " + config_.protocol;
}

void Node::getStatusSrv(const std::shared_ptr<rmw_request_id_t> /*header*/,
                         const std::shared_ptr<std_srvs::srv::Trigger::Request> /*req*/,
                         std::shared_ptr<std_srvs::srv::Trigger::Response> res)
{
    bool is_connected = radio_client_->connected();
    res->success = is_connected;
    res->message = "protocol=" + config_.protocol
                 + ", connected=" + (is_connected ? "true" : "false");
    if (radio_client_error_)
    {
        res->message += ", last_error=" + radio_client_error_.message();
    }
}

// ────────────────────────────────────────────────────────────────
//  Connection management
// ────────────────────────────────────────────────────────────────

void Node::pre_spin()
{
    if (radio_client_error_)
    {
        RCLCPP_ERROR_STREAM(this->get_logger(),
            "Radio Client Error : " << radio_client_error_.message());
        radio_client_->close();
        radio_client_error_.clear();
    }

    if (!connecting_ && !radio_client_->connected())
    {
        connecting_ = true;
        if (connect_thread_)
            connect_thread_->join();

        connect_thread_.reset(new std::thread([this]()
        {
            boost::system::error_code ec;

            if (config_.protocol == "mqtt")
            {
                RCLCPP_INFO_STREAM(this->get_logger(),
                    "Attempting MQTT connection to broker at "
                    << config_.broker_address << ":" << config_.broker_port);
                try {
                    if (!radio_client_->connect(config_.broker_address,
                                                static_cast<unsigned short>(config_.broker_port),
                                                0 /*unused*/, ec))
                    {
                        RCLCPP_WARN_STREAM(this->get_logger(),
                            "Failed to connect to MQTT broker, err: " << ec.message());
                    }
                } catch (std::exception &e) {
                    RCLCPP_ERROR_STREAM(this->get_logger(),
                        "Exception connecting to MQTT broker: " << e.what()
                        << " error_code: " << ec.message());
                }
            }
            else
            {
                RCLCPP_INFO_STREAM(this->get_logger(),
                    "Attempting UDP connection to OBU at "
                    << config_.v2x_radio_address << ":" << config_.v2x_radio_listening_port);
                RCLCPP_INFO_STREAM(this->get_logger(),
                    "Local port: " << config_.listening_port);
                try {
                    if (!radio_client_->connect(config_.v2x_radio_address,
                                                config_.v2x_radio_listening_port,
                                                config_.listening_port, ec))
                    {
                        RCLCPP_WARN_STREAM(this->get_logger(),
                            "Failed to connect, err: " << ec.message());
                    }
                } catch (std::exception &e) {
                    RCLCPP_ERROR_STREAM(this->get_logger(),
                        "Exception connecting to v2x radio: " << e.what()
                        << " error_code: " << ec.message());
                    RCLCPP_ERROR_STREAM(this->get_logger(),
                        "Config:\n\tv2x_radio_address:" << config_.v2x_radio_address
                        << "\n\tv2x_radio_listening_port:" << config_.v2x_radio_listening_port
                        << "\n\tlistening_port:" << config_.listening_port);
                }
            }

            connecting_ = false;
        }));
    }
}

// ────────────────────────────────────────────────────────────────
//  Wave config loading
// ────────────────────────────────────────────────────────────────

void Node::loadWaveConfig(const std::string &fileName)
{
    RCLCPP_DEBUG_STREAM(this->get_logger(), "Loading wave config");

    const char* schema = "{\n"
        " \"$schema\":\"http://json-schema.org/draft-06/schema\",\n"
        " \"title\":\"Wave Config Schema\",\n"
        " \"description\":\"A simple schema to describe DSRC/Wave messages\",\n"
        "  \"type\": \"array\",\n"
        "  \"items\": {\n"
        "    \"type\": \"object\",\n"
        "    \"properties\": {\n"
        "      \"name\": {\n"
        "        \"description\": \"message type - abbreviated name\",\n"
        "        \"type\": \"string\"\n"
        "      },\n"
        "      \"psid\": {\n"
        "        \"description\": \"psid assigned to message type in decimal\",\n"
        "        \"type\": \"string\"\n"
        "      },\n"
        "      \"dsrc_msg_id\": {\n"
        "        \"description\": \"J2735 DSRC id assigned to message type in decimal\",\n"
        "        \"type\": \"string\"\n"
        "      },\n"
        "      \"channel\": {\n"
        "        \"description\": \"DSRC radio channel assigned to message type in decimal\",\n"
        "        \"type\": \"string\"\n"
        "      },\n"
        "      \"priority\": {\n"
        "        \"description\": \"WSM Priotiy to use assigned to message type in decimal\",\n"
        "        \"type\":\"string\"\n"
        "      }\n"
        "    },\n"
        "    \"required\":[\"name\",\"psid\",\"dsrc_msg_id\",\"channel\",\"priority\"]"
        "  }\n"
        "}\n";

    std::ifstream file;
    try
    {
        file.open(fileName);
    }
    catch (std::exception& e)
    {
        RCLCPP_ERROR_STREAM(this->get_logger(),
            "Unable to open file : " << fileName << ", exception: " << e.what());
        return;
    }

    rapidjson::Document sd;
    if (sd.Parse(schema).HasParseError())
    {
        RCLCPP_ERROR_STREAM(this->get_logger(), "Invalid Wave Config Schema");
        return;
    }

    rapidjson::SchemaDocument schemaDocument(sd);
    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw(file);
    if (doc.ParseStream(isw).HasParseError())
    {
        RCLCPP_ERROR_STREAM(this->get_logger(), "Error Parsing Wave Config");
        return;
    }

    rapidjson::SchemaValidator validator(schemaDocument);
    if (!doc.Accept(validator))
    {
        RCLCPP_ERROR_STREAM(this->get_logger(), "Wave Config improperly formatted");
        return;
    }

    for (auto& it : doc.GetArray())
    {
        auto entry = it.GetObject();
        wave_cfg_items_.emplace_back(entry["name"].GetString(),
                                     entry["psid"].GetString(),
                                     entry["dsrc_msg_id"].GetString(),
                                     entry["channel"].GetString(),
                                     entry["priority"].GetString());
    }
}

// ────────────────────────────────────────────────────────────────
//  Shutdown
// ────────────────────────────────────────────────────────────────

void Node::handle_on_shutdown()
{
    RCLCPP_INFO_STREAM(this->get_logger(), "Shutdown signal received from DriverApplication");
    if (connect_thread_)
    {
        RCLCPP_INFO_STREAM(this->get_logger(), "Cleaning up connection thread");
        connect_thread_->join();
        connect_thread_.reset();
    }

    RCLCPP_INFO_STREAM(this->get_logger(), "Closing connection to radio");
    if (radio_client_)
    {
        radio_client_->close();
    }
}

} // namespace V2XDriverApplication

#include "rclcpp_components/register_node_macro.hpp"

// Register the component with class_loader
RCLCPP_COMPONENTS_REGISTER_NODE(V2XDriverApplication::Node)