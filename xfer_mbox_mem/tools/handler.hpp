#pragma once

#include "interface.hpp"

#include <string>

namespace host_tool
{

class UpdateHandlerInterface
{
  public:
    virtual ~UpdateHandlerInterface() = default;


    /**
     * Send the file contents at path to the blob id, target.
     *
     * @param[in] target - the blob id
     * @param[in] path - the source file path
     */
    virtual void sendFile(const std::string& path) = 0;

};

/** Object that actually handles the update itself. */
class UpdateHandler : public UpdateHandlerInterface
{
  public:
    UpdateHandler(DataInterface* handler) :
        handler(handler)
    {
    }

    ~UpdateHandler() = default;

    /**
     * @throw ToolException on failure.
     */
    void sendFile(const std::string& path) override;

  private:
    DataInterface* handler;
};

} // namespace host_tool
