/*
 * Copyright (C) 2022-2026 LEIDOS.
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

inline uint16_t toMsgId(uint8_t byte1, uint8_t byte2){
    return (static_cast<uint16_t>(byte1) << 8) | static_cast<uint16_t>(byte2);
}

inline uint16_t msgLength(const std::shared_ptr<std::vector<uint8_t>> &data, size_t offset){
    return static_cast<uint16_t>(data->end() - (data->begin() + offset));
}

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
    auto data_size = data->size();

    if (data_size < short_frame_)
    {
        RCLCPP_WARN_STREAM(logger_, "Received empty or insufficient data, nothing to process.");
        return;
    }

    for (size_t i = 0; i < data_size-1; i++){
        // find the presumed beginning of the message frame by hitting one of the 
        // valid message id listed in config
        auto msg_id = toMsgId(entry[i], entry[i + 1]);
        if (!IsValidMsgID(std::to_string(msg_id))){
            continue;
        }

        auto start_index{i+2};  // skip the two bytes for frame id
    
        if (msgLength(data, start_index) > 16383) { // lengths greater than 16383 (0x3FFF) are encoded by splitting up the message into discrete chunks, each with its own length marker
            RCLCPP_WARN_STREAM(logger_, "BaseRadioClient::process() : discarding received message with length field longer than 16383.");
            break;
        }

        if (!isValidMsgSize(data, start_index)) {
            RCLCPP_WARN_STREAM(logger_, "Expected size of possible MessageFrame does not match actual data size. Checking next index.");
            continue;
        }

        bool shouldProcess = !isPossiblePSID(std::to_string(msg_id)) ||
                             !isValidMsgAssumingBSMPSID(start_index, entry);

        if (shouldProcess) {
            onMessageReceived(*(data+start_index), msg_id);
            break;
        } else {
            RCLCPP_WARN_STREAM(logger_, "PSID found, parsing rest of data for MessageID.");
            continue;
        }
    }
}

//  Message-size validation

bool BaseRadioClient::isValidMsgSize(const std::shared_ptr<const std::vector<uint8_t>> &data, size_t start_index)
{
    // use explicit first 2 bits of the first byte to determine if it is short form, 
    // long form, or fragmented form (ignored for now).

    if(msgLength(data, start_index)<1){return false};

    uint8_t first_byte = data->at(0);

    uint16_t expected_size{0}, actual_size{0};

    if ((first_byte >> 7)==0){  // first bit 0, short form
        // skip the first byte before computing the length
        expected_size = msgLength(data, start_index+1);
        actual_size = first_byte;
        RCLCPP_DEBUG_STREAM(logger_, "Short form message encountered.");
    }
    else if ((first_byte >> 6)== 0b10) // first 2 bits 10, long form
    {   
        // Allows creation of msg_size from two bytes. e.g., [0 20 129 9 ...], we want to combine bytes 2 and 3 from this vector to create msg_size = 0x0109.
        // msg_vec[2] is hex value of 129 (0x81) and msg_vec[3] is hex value of 9 (0x09).
        // Per IEEE 1609.3, we will always have a value of "8" as the most significant value in the message size, if payload size > 128 bytes.
        // So, we bitwise AND (&) the value of msg_vec[2] with 0x7f (01111111) to drop the most significant bit, turning 0x81 (10000001) to 0x01 (00000001).
        // Because we need to merge two bytes, we cast the values to an unsigned 16-bit integer.
        // First byte is shifted to the left 8 bits (0x0001 is now 0x0100), followed by a bitwise OR (|) with msg_vec[3], combining 0x0100 and 0x0009 to create 0x0109.

        actual_size = (static_cast<uint16_t>(data->at(start_index) & 0x7F) << 8) | data->at(start_index+1);
        expected_size = msgLength(data, start_index+2);
        
        RCLCPP_DEBUG_STREAM(logger_, "Long form message encountered.");
    }
    else if ((first_byte >> 6)== 0b11){  // fragmented form, not supported
        RCLCPP_WARN_STREAM(logger_, "Fragmented form encountered, discard for now.")
    }
    else {
        RCLCPP_DEBUG_STREAM(logger_, "Skipping unknow size encoding.");
    }
    
    RCLCPP_DEBUG_STREAM(logger_, "Size match at index " << start_index <<": " << expected_size==actual_size << " | expected size: " << expected_size << " actual size: " << actual_size);
    
    return (actual_size>0) & (expected_size==actual_size);
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

bool BaseRadioClient::isValidMsgAssumingBSMPSID(const std::shared_ptr<std::vector<uint8_t>> &data), size_t start_index)
{
    for (auto i = start_index; i < start_index + 6; i++)
    {
        auto element_id = toMsgId(data->at(i), data->at(i+1));
        if (element_id == 0x0380) // 0x0380
        {
            auto element_id_index = i;
            for (auto j = element_id_index; j < element_id_index + 6; j++)
            {
                auto possible_msg_id = toMsgId(data->at(j), data->at[j+1]);
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