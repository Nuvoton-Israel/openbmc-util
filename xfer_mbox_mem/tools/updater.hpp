#pragma once

#include "handler.hpp"

#include <string>

namespace host_tool
{

/**
 * Attempt to update the BMC's firmware using the interface provided.
 *
 * @param[in] updater - update handler object.
 * @param[in] imagePath - the path to the image file.
 * @param[in] signaturePath - the path to the signature file.
 * @param[in] layoutType - the image update layout type (static/ubi/other)
 * @param[in] ignoreUpdate - determines whether to ignore the update status
 * @throws ToolException on failures.
 */
void updaterMain(UpdateHandlerInterface* updater, const std::string& imagePath,
                 const std::string& signaturePath);

} // namespace host_tool
