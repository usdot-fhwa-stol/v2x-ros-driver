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

#include "v2x_ros_driver/base_radio_client.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/schema.h>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace V2XDriverApplication
{

BaseRadioClient::BaseRadioClient() {}

BaseRadioClient::~BaseRadioClient() {}

bool BaseRadioClient::connect(const std::string &remote_address,
                              unsigned short remote_port,
                              unsigned short local_port)
{
    boost::system::error_code ignored_ec;
    return connect(remote_address, remote_port, local_port, ignored_ec);
}

//  Shared message processing

void BaseRadioClient::process(const std::shared_ptr<const std::vector<uint8_t>> &data)
{
    auto &entry = *data;

    if (entry.empty() || entry.size() < 3)
    {
        RCLCPP_WARN_STREAM(logger_, "Received empty or insufficient data, nothing to process.");
        return;
    }

    for (size_t i = 0; i < entry.size() - 3; i++)
    {
        auto msg_id = (static_cast<uint16_t>(entry[i]) << 8) | static_cast<uint16_t>(entry[i + 1]);
        if (!IsValidMsgID(std::to_string(msg_id))) { continue; }

        if ((i + short_frame_) >= entry.size()) {
            break;
        }

        auto start_index = i;
        std::vector<uint8_t> msg_vec(entry.begin() + start_index, entry.end());

        if (msg_vec.size() > 16383) {
            RCLCPP_WARN_STREAM(logger_, "BaseRadioClient::process() : discarding received message with length field longer than 16383.");
            break;
        }

        if (!isValidMsgSize(msg_vec, start_index, entry)) {
            RCLCPP_WARN_STREAM(logger_, "Size in possible MessageFrame does not match actual data size. Checking rest of data.");
            continue;
        }

        bool shouldProcess = !isPossiblePSID(std::to_string(msg_id)) ||
                             !isValidMsgAssumingBSMPSID(start_index, entry);

        if (shouldProcess) {
            onMessageReceived(msg_vec, msg_id);
            break;
        } else {
            RCLCPP_WARN_STREAM(logger_, "PSID found, parsing rest of data for MessageID.");
            continue;
        }
    }
}

//  Message-size validation

bool BaseRadioClient::isValidMsgSize(const std::vector<uint8_t> &msg_vec,
                                     size_t start_index,
                                     const std::vector<uint8_t> &entry)
{
    if (msg_vec.size() > 127)
    {
        auto tmp_start_index = start_index + long_frame_;
        std::vector<uint8_t> long_vec(entry.begin() + tmp_start_index, entry.end());
        auto msg_size = (static_cast<uint16_t>(msg_vec[2] & 0x7F) << 8) | msg_vec[3];
        return (msg_size == long_vec.size());
    }
    else if (msg_vec.size() < 128 && msg_vec.size() >= 3)
    {
        auto tmp_start_index = start_index + short_frame_;
        std::vector<uint8_t> short_vec(entry.begin() + tmp_start_index, entry.end());
        auto msg_size = msg_vec[2];
        return (msg_size == short_vec.size());
    }
    return false;
}

//  PSID / DSRCmsgID checks

bool BaseRadioClient::isPossiblePSID(const std::string &msg_id)
{
    if (wave_cfg_psids_.empty())
    {
        if (wave_file_path.empty()) return false;
        loadWaveConfigIds(wave_file_path);
    }

    for (const auto &psid : wave_cfg_psids_)
    {
        auto psid_value = std::stoi(psid, nullptr, 16);
        auto psidInt = std::to_string(psid_value);
        if (msg_id == psidInt) return true;
    }
    return false;
}

bool BaseRadioClient::isValidMsgAssumingBSMPSID(size_t start_index,
                                                 const std::vector<uint8_t> &entry)
{
    for (auto i = start_index; i < start_index + 6; i++)
    {
        auto element_id = (static_cast<uint16_t>(entry[i]) << 8) | static_cast<uint16_t>(entry[i+1]);
        if (element_id == 896) // 0x0380
        {
            auto element_id_index = i;
            for (auto j = element_id_index; j < element_id_index + 6; j++)
            {
                auto possible_msg_id = (static_cast<uint16_t>(entry[j]) << 8) | static_cast<uint16_t>(entry[j+1]);
                if (possible_msg_id == 20) return true; // BSM DSRCmsgID
            }
        }
    }
    return false;
}

bool BaseRadioClient::IsValidMsgID(const std::string &msg_id)
{
    if (wave_cfg_dsrc_msg_ids_.empty())
    {
        if (wave_file_path.empty()) return false;
        loadWaveConfigIds(wave_file_path);
    }

    for (const auto &dsrc_msg_id : wave_cfg_dsrc_msg_ids_)
    {
        if (msg_id == dsrc_msg_id) return true;
    }
    return false;
}

//  Wave config loading

void BaseRadioClient::set_wave_file_path(const std::string &path)
{
    wave_file_path = path;
}

void BaseRadioClient::loadWaveConfigIds(const std::string &fileName)
{
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
        "        \"description\": \"J2735 DSRCmsgID assigned to message type in decimal\",\n"
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
    catch (const std::ifstream::failure &e)
    {
        RCLCPP_ERROR_STREAM(logger_, "Unable to open file : " << fileName << ", exception: " << e.what());
        return;
    }

    rapidjson::Document sd;
    if (sd.Parse(schema).HasParseError())
    {
        RCLCPP_ERROR_STREAM(logger_, "Invalid Wave Config Schema");
        return;
    }

    rapidjson::SchemaDocument schemaDocument(sd);
    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw(file);
    if (doc.ParseStream(isw).HasParseError())
    {
        RCLCPP_ERROR_STREAM(logger_, "Error Parsing Wave Config");
        return;
    }

    rapidjson::SchemaValidator validator(schemaDocument);
    if (!doc.Accept(validator))
    {
        RCLCPP_ERROR_STREAM(logger_, "Wave Config improperly formatted");
        return;
    }

    for (auto &it : doc.GetArray())
    {
        auto entry = it.GetObject();
        wave_cfg_dsrc_msg_ids_.emplace_back(entry["dsrc_msg_id"].GetString());
        wave_cfg_psids_.emplace_back(entry["psid"].GetString());
    }
}

//  PSID lookup helper

std::string BaseRadioClient::psidForDsrcMsgId(const std::string &dsrc_msg_id) const
{
    for (size_t i = 0; i < wave_cfg_dsrc_msg_ids_.size(); ++i)
    {
        if (wave_cfg_dsrc_msg_ids_[i] == dsrc_msg_id)
        {
            return wave_cfg_psids_[i];
        }
    }
    return "";
}

}