#pragma once

extern "C" {
#include <pci/pci.h>
} // extern "C"

#include <optional>
#include <vector>

namespace host_tool
{

/* The ASPEED AST2400 & AST2500 have the same VIDDID. */

/**
 * The PciDevice structure is a copy of the information to uniquely identify a
 * PCI device.
 */
struct PciDevice
{
    std::uint16_t vid;
    std::uint16_t did;
    std::uint8_t bus;
    std::uint8_t dev;
    std::uint8_t func;
    pciaddr_t bars[6];
};

/**
 * The PciFilter structure is a simple mechanism for filtering devices by their
 * vendor and/or device ids.
 */
struct PciFilter
{
    std::uint16_t vid;
    std::uint16_t did;
};

class PciUtilInterface
{
  public:
    virtual ~PciUtilInterface() = default;

    /**
     * Get a list of PCI devices from a system.
     *
     * @param[in] filter - optional filter for the list.
     * @return the list of devices.
     */
    virtual std::vector<PciDevice>
        getPciDevices(std::optional<PciFilter> filter = std::nullopt) = 0;
};

class PciUtilImpl : public PciUtilInterface
{
  public:
    PciUtilImpl()
    {
        pacc = pci_alloc();
        pci_init(pacc);
    }
    ~PciUtilImpl()
    {
        pci_cleanup(pacc);
    }

    std::vector<PciDevice>
        getPciDevices(std::optional<PciFilter> filter = std::nullopt) override;

  private:
    struct pci_access* pacc;
};

} // namespace host_tool
