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

#include <iostream>
#include <iomanip>
#include <functional>
#include <dirent.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/schema.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "v2x_ros_driver/v2x_radio_client.h"
namespace V2XDriverApplication
{

namespace
{

// Decode a COER length determinant at index i.
// Short form (b < 0x80): the byte itself is the length.
// Long form  (b & 0x7F): low 7 bits give the number of trailing length octets.
// Returns false on overrun / unsupported width.
bool coerLength(const std::vector<uint8_t> &buf, size_t i,
                size_t &value, size_t &n_bytes)
{
    if (i >= buf.size()) { return false; }
    uint8_t b = buf[i];
    if (b < 0x80)
    {
        value = b;
        n_bytes = 1;
        return true;
    }
    size_t n = b & 0x7F;
    if (n == 0 || n > sizeof(size_t) || i + 1 + n > buf.size()) { return false; }
    size_t v = 0;
    for (size_t k = 0; k < n; ++k) { v = (v << 8) | buf[i + 1 + k]; }
    value = v;
    n_bytes = 1 + n;
    return true;
}

// Interpret buf[s:] as an IEEE 1609.2 Ieee1609Dot2Data and, if it ultimately
// carries a J2735 MessageFrame as its unsecuredData, return that MessageFrame.
//
//   03 80 <len> <MessageFrame>             content = unsecuredData  -> MessageFrame
//   03 81 <hashId> <preamble> <data> ...   content = signedData     -> recurse into
//                                                                       tbsData.payload.data
//
// All multi-byte fields are COER-encoded. Returns false for anything that does
// not parse as the above, so non-1609.2 data is left untouched by the caller.
bool extractIeee1609Dot2Payload(const std::vector<uint8_t> &buf, size_t s,
                                std::vector<uint8_t> &out)
{
    if (s + 2 > buf.size() || buf[s] != 0x03) { return false; } // protocolVersion == 3
    uint8_t content = buf[s + 1];

    if (content == 0x80) // unsecuredData (Opaque) -> this is the MessageFrame
    {
        size_t len = 0, n = 0;
        if (!coerLength(buf, s + 2, len, n)) { return false; }
        size_t start = s + 2 + n;
        if (len == 0 || start + len > buf.size()) { return false; }
        out.assign(buf.begin() + start, buf.begin() + start + len);
        return true;
    }

    if (content == 0x81) // signedData -> recurse into the signed payload's data field
    {
        // SignedData ::= SEQUENCE { hashId(1), tbsData, signer, signature }
        // tbsData    ::= SEQUENCE { payload SignedDataPayload, headerInfo }
        // SignedDataPayload preamble: the 'data' OPTIONAL is present when bit 0x40 is set.
        size_t i = s + 2;
        i += 1;                       // skip hashId enumeration (1 byte)
        if (i >= buf.size()) { return false; }
        uint8_t preamble = buf[i++];  // SignedDataPayload preamble byte
        if (!(preamble & 0x40)) { return false; } // 'data' field absent -> no MessageFrame here
        return extractIeee1609Dot2Payload(buf, i, out);
    }

    return false;
}

} // namespace

V2XRadioClient::V2XRadioClient() :
    running_(false)
{}


V2XRadioClient::~V2XRadioClient() {
    try{
        close();
    }catch(...){}
}

bool V2XRadioClient::connect(const std::string &remote_address, unsigned short remote_port,
                            unsigned short local_port) {
    boost::system::error_code ignored_ec;
    return connect(remote_address, remote_port, local_port, ignored_ec);
}

bool V2XRadioClient::connect(const std::string &remote_address,
                                unsigned short remote_port,
                                 unsigned short local_port,
                                 boost::system::error_code &ec)
{
    //If we are already connected return false
    if(running_) return false;

    //Get remote endpoint
    try
    {
        remote_udp_ep_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(remote_address),remote_port);
    }catch(std::exception e)
    {
        ec = boost::asio::error::invalid_argument;
        throw e;
    }

    ec.clear();

    io_.reset(new boost::asio::io_service());
    output_strand_.reset(new boost::asio::io_service::strand(*io_));

    //build the udp listener, this class listens on address::port and sends packets through onReceive
    try
    {
        if(udp_listener_){
            udp_listener_.reset(nullptr);
        }
        udp_listener_.reset(new cav::UDPListener(*io_,local_port));
    }catch(boost::system::system_error e)
    {
        ec = e.code();
        return false;
    }
    catch(std::exception e)
    {
        RCLCPP_ERROR_STREAM(logger_, "V2XRadioClient::connect threw exception : " << e.what());
        return false;
    };

    //connect signals
    udp_listener_->onReceive.connect([this](const std::shared_ptr<const std::vector<uint8_t>> &data){process(data);});

    udp_out_socket_.reset(new boost::asio::ip::udp::socket(*io_,remote_udp_ep_.protocol()));

    work_.reset(new boost::asio::io_service::work(*io_));
    // run the io service
    udp_listener_->start();
    io_thread_.reset(new std::thread([this]()
                                     {
                                         boost::system::error_code err;
                                         io_->run(err);
                                         if(err)
                                         {
                                             onError(err);
                                         }
                                     }));
    running_ = true;

    onConnect();
    return true;
}

void V2XRadioClient::close() {
    if(!running_) return;
    running_ = false;
    work_.reset();
    io_->stop();
    io_thread_->join();
    udp_listener_->stop();
    udp_out_socket_.reset();
    onDisconnect();
}

// J3224 SDSM (and other) messages may arrive wrapped in an IEEE 1609.2 security
// envelope. This recovers the bare J2735 MessageFrame from that envelope so the
// existing validation in process() can handle it. Returns false (leaving 'out'
// untouched) for data that is not 1609.2-wrapped, so un-wrapped traffic that
// already worked is unaffected.
bool V2XRadioClient::stripIeee1609Dot2Header(const std::vector<uint8_t> &in,
                                             std::vector<uint8_t> &out)
{
    // The IEEE 1609.2 envelope may sit behind a transport/capture header whose
    // length varies by source: the raw radio feed prepends only a few bytes of
    // framing, but the Commsignia "capture" feed prepends ~48 bytes of metadata
    // (record type, sequence, timestamps, interface info) before the message.
    // So scan the whole datagram for the start of the SPDU rather than assuming
    // a fixed offset or a small window. A successful strip requires both a
    // well-formed envelope AND that the recovered MessageFrame begins with a
    // configured DSRCmsgID; that guard keeps capture-metadata records, bare
    // MessageFrames, and certificate/signature bytes from ever being
    // misidentified and altered.
    for (size_t s = 0; s + 3 < in.size(); ++s)
    {
        std::vector<uint8_t> frame;
        if (!extractIeee1609Dot2Payload(in, s, frame)) { continue; }
        if (frame.size() < 3) { continue; }

        auto msg_id = (static_cast<uint16_t>(frame[0]) << 8) | static_cast<uint16_t>(frame[1]);
        if (IsValidMsgID(std::to_string(msg_id)))
        {
            RCLCPP_DEBUG_STREAM(logger_, "V2XRadioClient::process() : IEEE 1609.2 security "
                                         "header detected; stripped to inner MessageFrame ("
                                         << frame.size() << " bytes).");
            out = std::move(frame);
            return true;
        }
    }
    return false;
}

void V2XRadioClient::process(const std::shared_ptr<const std::vector<uint8_t>> &data)
{
    // Check if data is empty or smaller than the minimum required bytes
    if (!data || data->size() < 3)
    {
        RCLCPP_WARN_STREAM(logger_, "Received empty or insufficient data, nothing to process.");
        return;
    }

    // J3224 SDSM and other messages may arrive inside an IEEE 1609.2 security
    // envelope (signed: header + certificate + signature). The validation below
    // expects a bare J2735 MessageFrame, so a wrapped message would fail
    // isValidMsgSize() -- the trailing certificate/signature bytes inflate the
    // frame -- and be discarded. If a security header is present, strip it down
    // to the inner MessageFrame first; if it is absent, 'entry' stays bound to
    // the original buffer and the logic below is unchanged, so un-wrapped
    // traffic is processed exactly as before.
    std::vector<uint8_t> stripped;
    const std::vector<uint8_t> &entry =
        stripIeee1609Dot2Header(*data, stripped) ? stripped : *data;

    for (size_t i = 0; i < entry.size() - 3; i++)
    {   // Leave 3 bytes after (for lsb of id, length byte 1, and either message body or length byte 2)
        // Generate a 16-bit message id from two bytes, e.g. [0 20 ...] = 0x0014, skip if it isn't a valid one
        auto msg_id = (static_cast<uint16_t>(entry[i]) << 8) | static_cast<uint16_t>(entry[i + 1]);
        if (!IsValidMsgID(std::to_string(msg_id))) { continue; }

        if ((i + short_frame_) >= entry.size()) {
            break; // Break if not enough data remaining
        }

        auto start_index = i;
        std::vector<uint8_t> msg_vec(entry.begin() + start_index, entry.end());

        // TODO lengths greater than 16383 (0x3FFF) are encoded by splitting up the message into discrete chunks, each with its own length
        // marker. It doesn't look like we'll be receiving anything that long
        if (msg_vec.size() > 16383) {
            RCLCPP_WARN_STREAM(logger_, "V2XRadioClient::process() : discarding received message with length field longer than 16383.");
            break;
        }

        // Second check for a valid message. Checks MessageFrame length field against actual payload size.
        if (!isValidMsgSize(msg_vec, start_index, entry)) {
            RCLCPP_WARN_STREAM(logger_, "Size in possible MessageFrame does not match actual data size. Checking rest of data.");
            continue;
        }

        // Determine if we should process the message or continue searching by checking for a valid message.
        // (WSMP) T-Header uses same format as WAVE Short Message (WSM) and precurses the WSM in some cases.
        // Make sure T-Header is not accidentally detected before actual message. This is done by checking detected msg_id against list of PSIDs.
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

bool V2XRadioClient::isValidMsgSize(const std::vector<uint8_t> &msg_vec, size_t start_index, const std::vector<uint8_t> &entry)
{
    // If message vector is 128 bytes or larger, length field will be 2 bytes.
    if (msg_vec.size() > 127)
    {
        auto tmp_start_index = start_index + long_frame_;
        std::vector<uint8_t> long_vec(entry.begin() + tmp_start_index, entry.end());
        // Allows creation of msg_size from two bytes. e.g., [0 20 129 9 ...], we want to combine bytes 2 and 3 from this vector to create msg_size = 0x0109.
        // msg_vec[2] is hex value of 129 (0x81) and msg_vec[3] is hex value of 9 (0x09).
        // Per IEEE 1609.3, we will always have a value of "8" as the most significant value in the message size, if payload size > 128 bytes.
        // So, we bitwise AND (&) the value of msg_vec[2] with 0x7f (01111111) to drop the most significant bit, turning 0x81 (10000001) to 0x01 (00000001).
        // Because we need to merge two bytes, we cast the values to an unsigned 16-bit integer.
        // First byte is shifted to the left 8 bits (0x0001 is now 0x0100), followed by a bitwise OR (|) with msg_vec[3], combining 0x0100 and 0x0009 to create 0x0109.
        auto msg_size = (static_cast<uint16_t>(msg_vec[2] & 0x7F) << 8) | msg_vec[3];
        if (msg_size == long_vec.size())
            {
                return true;
            }
        else
        {
            return false;
        }
    }
    // If message vector is smaller than 128 bytes, length field will be 1 byte. Additional check to make sure msg_vec[2] exists.
    else if (msg_vec.size() < 128 && msg_vec.size() >= 3)
    {
        auto tmp_start_index = start_index + short_frame_;
        std::vector<uint8_t> short_vec(entry.begin() + tmp_start_index, entry.end());
        auto msg_size = msg_vec[2];
        if (msg_size == short_vec.size())
            {
                return true;
            }
        else
        {
            return false;
        }
    }
    else { return false; }
}

bool V2XRadioClient::isPossiblePSID(const std::string &msg_id)
{
    if (this->wave_cfg_psids_.empty())
    {
        if (this->wave_file_path.empty())
        {
            return false;
        }
        loadWaveConfigIds(this->wave_file_path);
    }

    for (const auto &psid : this->wave_cfg_psids_)
    {
        // Convert the hex string to an integer
        auto psid_value = std::stoi(psid, nullptr, 16);
        // Convert the integer to its decimal string representation
        auto psidInt = std::to_string(psid_value);
        if (msg_id == psidInt)
        {
            return true;
        }
    }
    return false;
}

bool V2XRadioClient::isValidMsgAssumingBSMPSID(size_t start_index, const std::vector<uint8_t> &entry)
{
    // Valid element id will exist, at max, 5 bytes after a PSID
    for (auto i = start_index; i < start_index + 6; i++)
    {
        // Generate a 16-bit element id from two bytes, e.g. [03 128 ...] = 0x0380
        auto element_id = (static_cast<uint16_t>(entry[i]) << 8) | static_cast<uint16_t>(entry[i+1]);
        // Check if valid element id 896 (0x0380) exists after PSID and before DSRCmsgID
        if (element_id == 896)
        {
            auto element_id_index = i;
            // Valid DSRCmsgID will exist, at max, 5 bytes after the element id
            for (auto j = element_id_index; j < element_id_index + 6; j++)
            {
                // Generate a 16-bit message id from two bytes, e.g. [0 20 ...] = 0x0014
                auto possible_msg_id = (static_cast<uint16_t>(entry[j]) << 8) | static_cast<uint16_t>(entry[j+1]);
                // Check if BSM DSRCmsgID 20 (0x0014)
                if (possible_msg_id == 20)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool V2XRadioClient::IsValidMsgID(const std::string &msg_id)
{
    if (this->wave_cfg_dsrc_msg_ids_.empty())
    {
        if (this->wave_file_path.size() == 0)
        {
            return false;
        }
        loadWaveConfigIds(this->wave_file_path);
    }

    for (const auto &dsrc_msg_id : this->wave_cfg_dsrc_msg_ids_)
    {
        if (msg_id == dsrc_msg_id)
        {
            return true;
        }
    }
    return false;
}

void V2XRadioClient::set_wave_file_path(const std::string &path)
{
    this->wave_file_path = path;
}

void V2XRadioClient::loadWaveConfigIds(const std::string &fileName)
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
        return ;
    }

    rapidjson::Document sd;
    if(sd.Parse(schema).HasParseError())
    {
        RCLCPP_ERROR_STREAM(logger_, "Invalid Wave Config Schema");
        return ;
    }

    rapidjson::SchemaDocument schemaDocument(sd);
    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw(file);
    if(doc.ParseStream(isw).HasParseError())
    {
        RCLCPP_ERROR_STREAM(logger_, "Error Parsing Wave Config");
        return ;
    }

    rapidjson::SchemaValidator validator(schemaDocument);
    if(!doc.Accept(validator))
    {
        RCLCPP_ERROR_STREAM(logger_, "Wave Config improperly formatted");
        return;
    }

    for(auto& it : doc.GetArray())
    {
        auto entry = it.GetObject();
        wave_cfg_dsrc_msg_ids_.emplace_back(entry["dsrc_msg_id"].GetString());
        wave_cfg_psids_.emplace_back(entry["psid"].GetString());
    }
}

bool V2XRadioClient::sendV2xMessage(const std::shared_ptr<std::vector<uint8_t>> &message)
{
    if(!running_) return false;
    try {
        output_strand_->post([this,message]()
                             {
                                 try
                                 {
                                     udp_out_socket_->send_to(boost::asio::buffer(*message), remote_udp_ep_);
                                 }
                                 catch(boost::system::system_error error_code)
                                 {
                                     onError(error_code.code());
                                 }
                                 catch(...)
                                 {
                                     onError(boost::asio::error::fault);
                                 }
                             });
        return true;
    }
    catch (std::exception& e) {
        return false;
    }
}
}
