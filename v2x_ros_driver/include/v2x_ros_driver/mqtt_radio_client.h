#pragma once
/*
 * Copyright (C) 2022-2025 LEIDOS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <mosquitto.h>
#include <atomic>
#include <string>
#include <memory>
#include <vector>
#include <thread>

#include "v2x_ros_driver/base_radio_client.h"

namespace V2XDriverApplication
{

/**
 * @brief MqttRadioClient implements the MQTT transport for V2X communication,
 * targeting the Ettifos OBU which hosts its own Mosquitto-based MQTT broker.
 *
 * Incoming messages: subscribes to  Ettifos/V2X/ind/J2735/#
 * Outgoing messages: publishes to   Ettifos/V2X/req/J2735/{PSID}
 *
 * All received payloads are routed through BaseRadioClient::process() for
 * the same validation applied to UDP messages.
 */
class MqttRadioClient : public BaseRadioClient
{
public:
    MqttRadioClient();
    ~MqttRadioClient() override;

    // MQTT-specific configuration

    /**
     * @brief Set MQTT-specific parameters before calling connect().
     * @param client_id    MQTT client identifier (empty = auto-generated)
     * @param qos          Default QoS level (0, 1, or 2)
     * @param reconnect_interval_sec  Seconds between reconnect attempts
     * @param mqtt_topic_prefix  Base topic prefix (default "Ettifos/V2X")
     */
    void setMqttConfig(const std::string &client_id,
                       int qos,
                       int reconnect_interval_sec,
                       const std::string &mqtt_topic_prefix = "Ettifos/V2X");

    // BaseRadioClient abstract

    /**
     * @brief Connects to the MQTT broker on the Ettifos OBU.
     * @param remote_address  Broker hostname / IP
     * @param remote_port     Broker port (typically 1883)
     * @param local_port      Unused for MQTT (ignored)
     * @param ec              Set on failure
     */
    bool connect(const std::string &remote_address, unsigned short remote_port,
                 unsigned short local_port, boost::system::error_code &ec) override;

    using BaseRadioClient::connect;

    void close() override;

    bool connected() const override { return connected_.load(); }

    /**
     * @brief Publish a raw UPER-encoded V2X message to the broker.
     * The first two bytes are treated as the DSRCmsgID, which is used
     * to look up the correct PSID and build the MQTT topic.
     */
    bool sendV2xMessage(const std::shared_ptr<std::vector<uint8_t>> &message) override;

private:

    // Mosquitto callbacks

    static void onConnectCb(struct mosquitto *mosq, void *userdata, int rc);
    static void onDisconnectCb(struct mosquitto *mosq, void *userdata, int rc);
    static void onMessageCb(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg);

    void handleConnect(int rc);
    void handleDisconnect(int rc);
    void handleMessage(const struct mosquitto_message *msg);

    /** @brief Subscribe to all configured inbound topics */
    void subscribeToTopics();

    /** @brief Build the outbound MQTT topic for a given hex PSID string */
    std::string buildPublishTopic(const std::string &psid_hex) const;

    /** @brief Build the inbound subscription topic (wildcard) */
    std::string buildSubscribeTopic() const;

    struct mosquitto *mosq_{nullptr};
    std::atomic<bool> connected_{false};

    // Config (set via setMqttConfig before connect)
    std::string client_id_;
    int qos_{0};
    int reconnect_interval_sec_{5};
    std::string mqtt_topic_prefix_{"Ettifos/V2X"};

    // Broker endpoint (set in connect())
    std::string broker_address_;
    int broker_port_{1883};

    // Mosquitto library init reference count (thread-safe)
    static std::atomic<int> lib_ref_count_;
};

}