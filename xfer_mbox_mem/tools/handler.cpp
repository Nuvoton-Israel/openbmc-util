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

#include "handler.hpp"

#include "flags.hpp"
#include "status.hpp"
#include "tool_errors.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace host_tool
{

void UpdateHandler::sendFile(const std::string& path)
{
    if (!handler->sendContents(path))
    {
        /* Need to close the session on failure, or it's stuck open (until the
         * blob handler timeout is implemented, and even then, why make it wait.
         */
        throw ToolException("Failed to send contents of " + path);
    }
}

} // namespace host_tool
