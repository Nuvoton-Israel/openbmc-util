/*
 * Copyright 2019 Google Inc.
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

#include "pci.hpp"

extern "C" {
#include <pci/pci.h>
} // extern "C"

#include <cstring>
#include <optional>
#include <vector>

namespace host_tool
{

std::vector<PciDevice>
    PciUtilImpl::getPciDevices(std::optional<PciFilter> filter)
{
    PciFilter test;
    bool check = false;
    std::vector<PciDevice> results;

    if (filter.has_value())
    {
        test = filter.value();
        check = true;
    }

    pci_scan_bus(pacc);
    struct pci_dev* dev;

    for (dev = pacc->devices; dev; dev = dev->next)
    {
        PciDevice item;

        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);

        item.bus = dev->bus;
        item.dev = dev->dev;
        item.func = dev->func;
        item.vid = dev->vendor_id;
        item.did = dev->device_id;
        std::memcpy(item.bars, dev->base_addr, sizeof(dev->base_addr));

        if (check)
        {
            if (test.vid == dev->vendor_id && test.did == dev->device_id)
            {
                results.push_back(item);
            }
        }
        else
        {
            results.push_back(item);
        }
    }

    return results;
}

} // namespace host_tool
