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

#include "ipmi_errors.hpp"
#include "ipmi_interface.hpp"
#include "ipmi_handler.hpp"

constexpr int ipmiOEMNetFn = 62;
constexpr int ipmiOEMCmd = 62;

namespace host_tool
{

void updaterMain(UpdateHandlerInterface* updater, const std::string& imagePath,
                 const std::string& signaturePath)
{
    auto ipmi = host_tool::IpmiHandler::CreateIpmiHandler();
    std::vector<std::uint8_t> request, reply;

    try
    {
        /* Send over the firmware image. */
        std::fprintf(stderr, "Sending over the firmware image.\n");
        updater->sendFile(imagePath);

    }
    catch (...)
    {
        std::fprintf(stderr, "Sending file fail.\n");
        throw;
    }
}

} // namespace host_tool
