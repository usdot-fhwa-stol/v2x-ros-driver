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

#include <iostream>
#include <vector>
#include <string>

namespace V2XDriverApplication
{

  /**
   * \brief Struct containing the algorithm configuration values for v2x_ros_driver
   */
  struct Config
  {
    // ── Protocol selection ──
    //! "udp" (default, existing Cohda/Commsignia/Kapsch) or "mqtt" (Ettifos)
    std::string protocol = "udp";

    // ── Existing UDP parameters ──
    std::string v2x_radio_address = "169.254.1.1";
    int v2x_radio_listening_port = 1516;
    int listening_port = 5398;

    // ── New MQTT parameters ──
    std::string broker_address = "localhost";
    int broker_port = 1883;
    std::string client_id = "";               // empty = auto-generated
    int qos_default = 0;                      // 0 = fire-and-forget
    int reconnect_interval_sec = 5;
    std::string mqtt_topic_prefix = "Ettifos/V2X";

    // Stream operator for this config
    friend std::ostream &operator<<(std::ostream &output, const Config &c)
    {
      output << "v2x_ros_driver::Config { " << std::endl
           << "protocol: " << c.protocol << std::endl
           << "listening_port: " << c.listening_port << std::endl
           << "v2x_radio_listening_port: " << c.v2x_radio_listening_port << std::endl
           << "v2x_radio_address: " << c.v2x_radio_address << std::endl
           << "broker_address: " << c.broker_address << std::endl
           << "broker_port: " << c.broker_port << std::endl
           << "client_id: " << c.client_id << std::endl
           << "qos_default: " << c.qos_default << std::endl
           << "reconnect_interval_sec: " << c.reconnect_interval_sec << std::endl
           << "mqtt_topic_prefix: " << c.mqtt_topic_prefix << std::endl
           << "}" << std::endl;

      return output;
    }
  };

}