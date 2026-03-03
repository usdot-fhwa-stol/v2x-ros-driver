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

#include <memory>
#include <thread>
#include <vector>

#include "v2x_ros_driver/base_radio_client.h"
#include "v2x_ros_driver/udp_listener.h"

namespace V2XDriverApplication
{

/**
 * @brief UdpRadioClient implements the UDP transport for V2X communication.
 * Refactored from the original V2XRadioClient; all shared validation logic
 * now lives in BaseRadioClient.
 */
class UdpRadioClient : public BaseRadioClient
{
public:
    UdpRadioClient();
    ~UdpRadioClient() override;

    // ── BaseRadioClient abstract overrides ──

    bool connect(const std::string &remote_address, unsigned short remote_port,
                 unsigned short local_port, boost::system::error_code &ec) override;

    using BaseRadioClient::connect; // pull in convenience overload

    void close() override;

    bool connected() const override { return running_; }

    bool sendV2xMessage(const std::shared_ptr<std::vector<uint8_t>> &message) override;

private:
    std::unique_ptr<boost::asio::io_service> io_;
    std::unique_ptr<boost::asio::io_service::strand> output_strand_;
    std::shared_ptr<boost::asio::io_service::work> work_;

    std::shared_ptr<std::thread> io_thread_;
    volatile bool running_;

    std::unique_ptr<boost::asio::ip::udp::socket> udp_out_socket_;
    boost::asio::ip::udp::endpoint remote_udp_ep_;
    std::unique_ptr<cav::UDPListener> udp_listener_;
};

} // namespace V2XDriverApplication