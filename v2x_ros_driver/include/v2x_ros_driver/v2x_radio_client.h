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

#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include "v2x_ros_driver/udp_listener.h"
namespace V2XDriverApplication
{

class V2XRadioClient
{

public:
    /**
     * @brief Initializes the V2X radio client, sets up i/o services
     */
    V2XRadioClient();
    ~V2XRadioClient();

    /**
    * @brief Connects the driver to the radio at the provided IPv4 address and Port
    * @param remote_address IPv4 address of radio
    * @param remote_port Port of client service
    * @param local_port Local port to receive incoming data
    * @param ec Error code set during connect
    * @return True on sucessful connect, false otherwise
    */
    bool connect(const std::string &remote_address,unsigned short remote_port,
                        unsigned short local_port,boost::system::error_code &ec);

    bool connect(const std::string &remote_address, unsigned short remote_port,
                 unsigned short local_port);

    /**
    * @brief Closes connection - closes all sockets, and stops all threads used for io and processing, blocks until threads are closed
    */
    void close();

    /**
    * @brief Returns connected state
    * @return Returns true if connected, false otherwise
    */
    inline bool connected() { return running_; }

    /**
    * @brief Signaled when client is connected to a V2X radio device
    */
    boost::signals2::signal<void()> onConnect;

    /**
    * @brief Signaled when client is disconnected from hardware
    */
    boost::signals2::signal<void()> onDisconnect;

    /**
     * @brief Signaled when client experiences a fatal error. Client should be closed and reconnected
     */
    boost::signals2::signal<void(const boost::system::error_code&)> onError;

    /**
    * @brief Signaled when message received
    */
    boost::signals2::signal<void(std::vector<uint8_t> const &, uint16_t id)> onMessageReceived;

    /**
     * @brief Sends a UDP message
     * @param message Message vector to be sent
     */
    bool sendV2xMessage(const std::shared_ptr<std::vector<uint8_t>> &message);

    /**
     * @brief Validate possible DSRCmsgID
     * @param msg_id Message ID under test
     */
    bool IsValidMsgID(const std::string &msg_id);

    /**
     * @brief Load list of valid dsrc_msg_ids
     * @param fileName Configuration file to be loaded
     */
    void loadWaveConfigIds(const std::string &fileName);

    /**
     * @brief Set wave config path
     * @param path Path where configuration file is located
    */
    void set_wave_file_path(const std::string &path);

    /**
     * @brief Checks actual message vector size against size specified in MessageFrame
     * @param msg_vec Full message vector under test
     * @param start_index Starting index for detected payload within message vector
     * @param entry Recevied data reference used to generate test vectors
     */
    bool isValidMsgSize(const std::vector<uint8_t> &msg_vec, size_t start_index, const std::vector<uint8_t> &entry);
    
    /**
     * @brief Checks msg_id against Provider Service ID (PSID) list to make sure a Wireless Access in Vehicular Environments (WAVE) 
     * Short Message Protocol (WSMP) T-Header with valid PSID is not accidentally used instead of WSM Data (MessageFrame + Payload).
     * IEEE 1609.3 (2020) - Section 8.3 Figure 33 and Section 8.3.3 Table 21, Figure 36 (TPID = 0). 
     * @param msg_id Message ID under test
     */
    bool isValidPSID(const std::string &msg_id);

private:
    rclcpp::Logger logger_{rclcpp::get_logger("v2x_radio_client")};

    std::unique_ptr<boost::asio::io_service> io_;
    std::unique_ptr<boost::asio::io_service::strand> output_strand_;
    std::shared_ptr<boost::asio::io_service::work> work_;

    std::shared_ptr<std::thread> io_thread_;
    volatile bool running_;
    std::vector<std::string> wave_cfg_dsrc_msg_ids_;
    std::vector<std::string> wave_cfg_psids_;
    std::string wave_file_path;

    std::unique_ptr<boost::asio::ip::udp::socket> udp_out_socket_;
    boost::asio::ip::udp::endpoint remote_udp_ep_;
    std::unique_ptr<cav::UDPListener> udp_listener_;

    /** @brief WAVE Service Advertisement (WSA) frame size = 2 bytes for J2735 payloads > 127 bytes. IEEE 1609.3 (2020) - 8.1.3.
     * Add 2 bytes for DSRCmsgID. */
    static const int long_frame_ = 4;
    /** @brief WAVE Service Advertisement (WSA) frame size = 1 byte for J2735 payloads < 128 bytes. IEEE 1609.3 (2020) - 8.1.3.
     * Add 2 bytes for DSRCmsgID. */
    static const int short_frame_ = 3;

    /**
    * @brief Maintains the process thread.
    * This will parse through the incoming UDP packets and runs various tests to find a
    * valid J2735 message.
    * @param data Incoming data to be processed.
    */
    void process(const std::shared_ptr<const std::vector<uint8_t>> &data);
};

}
