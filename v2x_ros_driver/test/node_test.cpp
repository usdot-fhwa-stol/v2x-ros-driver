
/*------------------------------------------------------------------------------
* Copyright (C) 2020-2025 LEIDOS.
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

------------------------------------------------------------------------------*/

#include <iostream>
#include <functional>
#include "v2x_ros_driver/v2x_radio_client.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/schema.h>
#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include <ament_index_cpp/get_package_share_directory.hpp>

TEST(V2XRadioClient, testSocket)
{
    boost::asio::ip::tcp::endpoint remote_endpoint = boost::asio::ip::tcp::endpoint( boost::asio::ip::address_v4::from_string( "192.168.88.40" ), 5398 );
    ASSERT_EQ("192.168.88.40",remote_endpoint.address().to_string());
}

TEST(V2XRadioClient,testConnection)
{
    V2XDriverApplication::V2XRadioClient v2x_radio_client_;
    boost::system::error_code ec;

    ASSERT_FALSE(v2x_radio_client_.connected());
    auto result = v2x_radio_client_.connect("192.168.88.40", 1516, 5398, ec);
    ASSERT_TRUE(result);
    ASSERT_EQ(0,ec.value());
    ASSERT_EQ("Success",ec.message());

    ASSERT_TRUE(v2x_radio_client_.connected());
    result = v2x_radio_client_.connect("192.168.88.40", 1516, 5398);
    ASSERT_FALSE(result);

    v2x_radio_client_.close();
    ASSERT_FALSE(v2x_radio_client_.connected());
    result = v2x_radio_client_.connect("192.168.88.40", 1516, 5398);
    ASSERT_TRUE(result);
}

TEST(V2XRadioClient,testValidateMsgId)
{
    V2XDriverApplication::V2XRadioClient v2x_radio_client_;
    //No valid msg_id loaded yet
    uint16_t msg_id = 20;
    ASSERT_FALSE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));


    std::string package_share_directory = ament_index_cpp::get_package_share_directory("v2x_ros_driver");

    //load wrong wave config file
    v2x_radio_client_.set_wave_file_path(package_share_directory + "/etc/wave_invalid.json");
    ASSERT_FALSE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));

    //read list of valid msg_id from correct wave.json file
    v2x_radio_client_.set_wave_file_path(package_share_directory + "/etc/wave.json");
    ASSERT_TRUE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
    msg_id = 31;
    ASSERT_TRUE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
    msg_id = 18;
    ASSERT_TRUE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
    msg_id = 19;
    ASSERT_TRUE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
    msg_id = 240;
    ASSERT_TRUE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
    msg_id = 241;
    ASSERT_TRUE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
    msg_id = 242;
    ASSERT_TRUE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
    msg_id = 243;
    ASSERT_TRUE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
    msg_id = 896; //invalid msg_id
    ASSERT_FALSE(v2x_radio_client_.IsValidMsgID(std::to_string(msg_id)));
}
TEST(V2XRadioClient,testsendV2xMessage)
{
    V2XDriverApplication::V2XRadioClient v2x_radio_client_;
    std::shared_ptr<std::vector<uint8_t>> messagePtr;
    std::vector<uint8_t> message =
     {0, 243, 124, 29, 89, 212, 226, 212, 58, 179, 169, 197, 168,
     193, 131, 6, 12, 24, 48, 96, 193, 131, 6, 12, 24, 48, 96, 181, 131, 6, 12, 22, 176, 96, 193, 130, 214, 12,
     24, 48, 90, 193, 131, 6, 12, 24, 48, 96, 193, 131, 6, 12, 24, 48, 96, 193, 131, 6, 12, 24, 48, 96, 193, 131,
     6, 12, 24, 48, 54, 167, 46, 184, 245, 201, 221, 207, 134, 92, 125, 52, 239, 220, 24, 118, 211, 186, 254, 238,
     187, 113, 101, 228, 233, 140, 106, 178, 163, 200, 137, 89, 204, 57, 144, 168, 56, 112, 205, 154, 204, 120, 121,
     114, 211, 151, 149, 253, 216, 118, 229, 117, 26, 108, 58, 112, 106, 101, 198};
    messagePtr= std::make_shared<std::vector<uint8_t> >(std::move(message));
    ASSERT_FALSE(v2x_radio_client_.sendV2xMessage(messagePtr));
    auto result = v2x_radio_client_.connect("192.168.88.40", 1516, 5398);
    ASSERT_TRUE(result);
    ASSERT_TRUE(v2x_radio_client_.connected());
    ASSERT_TRUE(v2x_radio_client_.sendV2xMessage(messagePtr));
}
TEST(V2XRadioClient,testIsValidMsgSize)
{
    V2XDriverApplication::V2XRadioClient v2x_radio_client_;
    size_t start_index = 0;

    // Case 1: Message vector too short (msg_vec.size() < 3)
    {
        std::vector<uint8_t> msg_vec = {0, 1};  // Only 2 bytes
        std::vector<uint8_t> entry = {0, 1};
        ASSERT_FALSE(v2x_radio_client_.isValidMsgSize(msg_vec, start_index, entry));
    }

    // Case 2: Short frame (msg_vec.size() < 128)
    // In this case, msg_vec[2] holds the payload length
    {
        std::vector<uint8_t> msg_vec = 
        {0, 32, 59, 3, 128, 56, 0, 20, 53, 84, 110, 235, 28, 175, 212, 133, 230, 190, 195, 243, 28, 
            231, 55, 89, 11, 207, 4, 132, 128, 0, 112, 0, 128, 0, 253, 125, 55, 208, 0, 127, 255, 0, 
            0, 101, 144, 160, 0, 56, 192, 0, 255, 117, 63, 233, 47, 1, 255, 250, 255, 254, 200, 0};

        std::vector<uint8_t> valid_entry = 
        {0, 32, 59, 3, 128, 56, 0, 20, 53, 84, 110, 235, 28, 175, 212, 133, 230, 190, 195, 243, 28, 
            231, 55, 89, 11, 207, 4, 132, 128, 0, 112, 0, 128, 0, 253, 125, 55, 208, 0, 127, 255, 0, 
            0, 101, 144, 160, 0, 56, 192, 0, 255, 117, 63, 233, 47, 1, 255, 250, 255, 254, 200, 0};
        ASSERT_TRUE(v2x_radio_client_.isValidMsgSize(msg_vec, start_index, valid_entry));

        std::vector<uint8_t> invalid_entry = 
        {1, 2, 32, 59, 3, 128, 56, 0, 20, 53, 84, 110, 235, 28, 175, 212, 133, 230, 190, 195, 243, 28, 
            231, 55, 89, 11, 207, 4, 132, 128, 0, 112, 0, 128, 0, 253, 125, 55, 208, 0, 127, 255, 0, 
            0, 101, 144, 160, 0, 56, 192, 0, 255, 117, 63, 233, 47, 1, 255, 250, 255, 254, 200, 0};
        ASSERT_FALSE(v2x_radio_client_.isValidMsgSize(msg_vec, start_index, invalid_entry));
    }

    // Case 3: Long frame (msg_vec.size() > 127)
    // In this case, msg_vec[2] and msg_vec[3] hold the payload length
    {
        std::vector<uint8_t> msg_vec =
        {0, 18, 129, 145, 56, 0, 48, 0, 33, 136, 122, 1, 70, 103, 163, 112, 58, 131, 73, 205, 17, 244, 2, 220, 40, 32, 8, 36, 0, 0, 0, 0, 175, 147, 193, 130, 
            196, 187, 200, 128, 24, 144, 0, 0, 0, 2, 173, 110, 197, 11, 12, 237, 162, 0, 66, 64, 0, 0, 0, 10, 115, 185, 112, 43, 227, 205, 136, 0, 137, 0, 0, 0, 
            0, 40, 146, 224, 112, 175, 239, 42, 72, 43, 8, 0, 0, 0, 0, 145, 227, 50, 136, 210, 214, 115, 192, 176, 100, 0, 5, 15, 36, 20, 132, 0, 0, 0, 0, 70, 
            99, 142, 4, 103, 127, 90, 224, 88, 100, 0, 1, 7, 18, 9, 194, 0, 0, 0, 0, 34, 19, 192, 98, 51, 131, 174, 184, 44, 54, 0, 0, 131, 73, 4, 161, 0, 0, 0,
            0, 16, 114, 93, 25, 25, 199, 215, 172, 86, 29, 0, 0, 65, 107, 17, 32, 0, 32, 194, 64, 82, 64, 0, 0, 0, 7, 139, 186, 40, 15, 197, 20, 31, 21, 136, 
            192, 0, 32, 10, 193, 8, 0, 16, 8, 64, 24, 200, 0, 0, 0, 1, 226, 238, 88, 195, 249, 125, 24, 68, 1, 204, 128, 0, 0, 0, 30, 53, 100, 16, 63, 168, 81, 
            0, 144, 33, 16, 0, 0, 0, 1, 117, 250, 202, 14, 101, 101, 23, 21, 130, 64, 0, 48, 26, 193, 200, 0, 24, 16, 144, 37, 16, 0, 0, 0, 1, 108, 234, 159, 
            142, 98, 37, 10, 5, 129, 192, 0, 48, 41, 32, 82, 32, 0, 0, 0, 2, 199, 196, 214, 28, 192, 202, 20, 11, 2, 128, 0, 96, 98, 64, 180, 64, 0, 0, 0, 3, 
            222, 195, 208, 230, 20, 80, 160, 88, 138, 0, 0, 131, 136, 6, 41, 0, 0, 0, 0, 37, 143, 54, 0, 205, 242, 26, 32, 26, 164, 0, 0, 0, 0, 144, 228, 199, 
            195, 59, 8, 104, 128, 114, 144, 0, 0, 0, 2, 53, 34, 195, 12, 223, 36, 36, 130, 12, 128, 0, 0, 0, 12, 205, 205, 240, 16, 207, 49, 224, 44, 26, 0, 2, 
            2, 137, 3, 217, 0, 0, 0, 0, 25, 172, 25, 132, 33, 165, 101, 209, 88, 60, 0, 4, 4, 44, 56, 128, 2, 2, 68, 4, 92, 128, 0, 0, 0, 25, 154, 30, 32, 33, 
            79, 98, 32};

        std::vector<uint8_t> valid_entry =
        {0, 18, 129, 145, 56, 0, 48, 0, 33, 136, 122, 1, 70, 103, 163, 112, 58, 131, 73, 205, 17, 244, 2, 220, 40, 32, 8, 36, 0, 0, 0, 0, 175, 147, 193, 130, 
            196, 187, 200, 128, 24, 144, 0, 0, 0, 2, 173, 110, 197, 11, 12, 237, 162, 0, 66, 64, 0, 0, 0, 10, 115, 185, 112, 43, 227, 205, 136, 0, 137, 0, 0, 0, 
            0, 40, 146, 224, 112, 175, 239, 42, 72, 43, 8, 0, 0, 0, 0, 145, 227, 50, 136, 210, 214, 115, 192, 176, 100, 0, 5, 15, 36, 20, 132, 0, 0, 0, 0, 70, 
            99, 142, 4, 103, 127, 90, 224, 88, 100, 0, 1, 7, 18, 9, 194, 0, 0, 0, 0, 34, 19, 192, 98, 51, 131, 174, 184, 44, 54, 0, 0, 131, 73, 4, 161, 0, 0, 0,
            0, 16, 114, 93, 25, 25, 199, 215, 172, 86, 29, 0, 0, 65, 107, 17, 32, 0, 32, 194, 64, 82, 64, 0, 0, 0, 7, 139, 186, 40, 15, 197, 20, 31, 21, 136, 
            192, 0, 32, 10, 193, 8, 0, 16, 8, 64, 24, 200, 0, 0, 0, 1, 226, 238, 88, 195, 249, 125, 24, 68, 1, 204, 128, 0, 0, 0, 30, 53, 100, 16, 63, 168, 81, 
            0, 144, 33, 16, 0, 0, 0, 1, 117, 250, 202, 14, 101, 101, 23, 21, 130, 64, 0, 48, 26, 193, 200, 0, 24, 16, 144, 37, 16, 0, 0, 0, 1, 108, 234, 159, 
            142, 98, 37, 10, 5, 129, 192, 0, 48, 41, 32, 82, 32, 0, 0, 0, 2, 199, 196, 214, 28, 192, 202, 20, 11, 2, 128, 0, 96, 98, 64, 180, 64, 0, 0, 0, 3, 
            222, 195, 208, 230, 20, 80, 160, 88, 138, 0, 0, 131, 136, 6, 41, 0, 0, 0, 0, 37, 143, 54, 0, 205, 242, 26, 32, 26, 164, 0, 0, 0, 0, 144, 228, 199, 
            195, 59, 8, 104, 128, 114, 144, 0, 0, 0, 2, 53, 34, 195, 12, 223, 36, 36, 130, 12, 128, 0, 0, 0, 12, 205, 205, 240, 16, 207, 49, 224, 44, 26, 0, 2, 
            2, 137, 3, 217, 0, 0, 0, 0, 25, 172, 25, 132, 33, 165, 101, 209, 88, 60, 0, 4, 4, 44, 56, 128, 2, 2, 68, 4, 92, 128, 0, 0, 0, 25, 154, 30, 32, 33, 
            79, 98, 32};
        ASSERT_TRUE(v2x_radio_client_.isValidMsgSize(msg_vec, start_index, valid_entry));

        std::vector<uint8_t> invalid_entry =
        {1, 2, 18, 129, 145, 56, 0, 48, 0, 33, 136, 122, 1, 70, 103, 163, 112, 58, 131, 73, 205, 17, 244, 2, 220, 40, 32, 8, 36, 0, 0, 0, 0, 175, 147, 193, 130, 
            196, 187, 200, 128, 24, 144, 0, 0, 0, 2, 173, 110, 197, 11, 12, 237, 162, 0, 66, 64, 0, 0, 0, 10, 115, 185, 112, 43, 227, 205, 136, 0, 137, 0, 0, 0, 
            0, 40, 146, 224, 112, 175, 239, 42, 72, 43, 8, 0, 0, 0, 0, 145, 227, 50, 136, 210, 214, 115, 192, 176, 100, 0, 5, 15, 36, 20, 132, 0, 0, 0, 0, 70, 
            99, 142, 4, 103, 127, 90, 224, 88, 100, 0, 1, 7, 18, 9, 194, 0, 0, 0, 0, 34, 19, 192, 98, 51, 131, 174, 184, 44, 54, 0, 0, 131, 73, 4, 161, 0, 0, 0,
            0, 16, 114, 93, 25, 25, 199, 215, 172, 86, 29, 0, 0, 65, 107, 17, 32, 0, 32, 194, 64, 82, 64, 0, 0, 0, 7, 139, 186, 40, 15, 197, 20, 31, 21, 136, 
            192, 0, 32, 10, 193, 8, 0, 16, 8, 64, 24, 200, 0, 0, 0, 1, 226, 238, 88, 195, 249, 125, 24, 68, 1, 204, 128, 0, 0, 0, 30, 53, 100, 16, 63, 168, 81, 
            0, 144, 33, 16, 0, 0, 0, 1, 117, 250, 202, 14, 101, 101, 23, 21, 130, 64, 0, 48, 26, 193, 200, 0, 24, 16, 144, 37, 16, 0, 0, 0, 1, 108, 234, 159, 
            142, 98, 37, 10, 5, 129, 192, 0, 48, 41, 32, 82, 32, 0, 0, 0, 2, 199, 196, 214, 28, 192, 202, 20, 11, 2, 128, 0, 96, 98, 64, 180, 64, 0, 0, 0, 3, 
            222, 195, 208, 230, 20, 80, 160, 88, 138, 0, 0, 131, 136, 6, 41, 0, 0, 0, 0, 37, 143, 54, 0, 205, 242, 26, 32, 26, 164, 0, 0, 0, 0, 144, 228, 199, 
            195, 59, 8, 104, 128, 114, 144, 0, 0, 0, 2, 53, 34, 195, 12, 223, 36, 36, 130, 12, 128, 0, 0, 0, 12, 205, 205, 240, 16, 207, 49, 224, 44, 26, 0, 2, 
            2, 137, 3, 217, 0, 0, 0, 0, 25, 172, 25, 132, 33, 165, 101, 209, 88, 60, 0, 4, 4, 44, 56, 128, 2, 2, 68, 4, 92, 128, 0, 0, 0, 25, 154, 30, 32, 33, 
            79, 98, 32};
        ASSERT_FALSE(v2x_radio_client_.isValidMsgSize(msg_vec, start_index, invalid_entry));
    }
}

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    bool success = RUN_ALL_TESTS();

    return success;
}
