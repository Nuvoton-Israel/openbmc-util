/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "updater.hpp"

#include "flags.hpp"
#include "handler.hpp"
#include "status.hpp"
#include "tool_errors.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <exception>

#include "ipmi_errors.hpp"
#include "ipmi_interface.hpp"
#include "ipmi_handler.hpp"

constexpr int ipmiOEMNetFn = 62;
constexpr int ipmiOEMCmd = 62;

namespace host_tool
{

// we will not handler extra large file error, and
// we do only support sending < 16K file via mailbox
unsigned int getFileSize(
        const std::string& imagePath, std::vector<std::uint8_t> &request)
{
    unsigned int image_size = 0;
    constexpr unsigned int maxImageSize = 0x4000;
    try
    {
        std::ifstream in(imagePath, std::ifstream::ate | std::ifstream::binary);
        image_size = in.tellg();
        if (image_size > maxImageSize || image_size == 0)
        {
            std::fprintf(stderr, "File size is too large!, %u\n", image_size);
            return 0;
        }
        std::fprintf(stdout, "image size: %u\n", image_size);
        // put the image size as byte array
        request.push_back(image_size & 0xFF);
        request.push_back(image_size >> 8 & 0xFF);
        request.push_back(image_size >> 16 & 0xFF);
        request.push_back(image_size >> 24 & 0xFF);
    }
    catch (const std::exception& e)
    {
         std::fprintf(stderr, "Get file size error, %s\n", e.what());
    }
    return image_size;
}

void updaterMain(UpdateHandlerInterface* updater, const std::string& imagePath,
                 const std::string& signaturePath)
{
    auto ipmi = host_tool::IpmiHandler::CreateIpmiHandler();
    std::vector<std::uint8_t> request={}, reply;

    try
    {
        /* Send over the firmware image. */
        std::fprintf(stderr, "Sending over the firmware image.\n");
        updater->sendFile(imagePath);
        try
        {
            // get image size first
            if (getFileSize(imagePath, request) == 0)
            {
                return;
            }
            std::fprintf(stderr, "sendPacket: send ipmi oem cmd (0x3e)\n");
            reply = ipmi->sendPacket(ipmiOEMNetFn, ipmiOEMCmd, request);
        }
        catch (const IpmiException& e)
        {
            std::fprintf(stderr, "sendPacket: send ipmi oem cmd error\n");
	    std::fprintf(stderr, "%s\n", e.what());
        }
    }
    catch (...)
    {
        std::fprintf(stderr, "Sending file fail.\n");
        throw;
    }
}

} // namespace host_tool
