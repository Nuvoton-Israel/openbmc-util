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

#include "p2a.hpp"

#include "data.hpp"
#include "flags.hpp"
#include "pci.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace host_tool
{



bool P2aDataHandler::sendContents(const std::string& input)
{
    PciDevice result;
    PciFilter filter;
    bool found = false;
    pciaddr_t bar;
    bool returnValue = false;
    int inputFd = -1;
    std::int64_t fileSize;
    std::unique_ptr<std::uint8_t[]> readBuffer;

    std::uint32_t P2aOffset;
    std::uint32_t p2aLength;

        int bytesRead = 0;
        std::uint32_t offset = 0;

    for (auto device : PCIDeviceList)
    {
        filter.vid = device.VID;
        filter.did = device.DID;

        /* Find the PCI device entry we want. */
        auto output = pci->getPciDevices(filter);
        for (const auto& d : output)
        {
            std::fprintf(stderr, "sendContents: [0x%x 0x%x] \n", d.vid, d.did);

            /* Verify it's a memory-based bar. */
            bar = d.bars[device.bar];

            if ((bar & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO)
            {
                /* We want it to not be IO-based access. */
                continue;
            }

            /* For now capture the entire device even if we're only using BAR0
             */
            result = d;
            found = true;
			break;
        }

        if (found)
        {
            std::fprintf(stderr, "sendContents: Find [0x%x 0x%x] \n", device.VID, device.DID);
            std::fprintf(stderr, "sendContents: bar%u[0x%x] \n", device.bar,
                         (unsigned int)bar);
            P2aOffset = device.Offset;
            p2aLength = device.Length;
            break;
        }
    }

    if (!found)
    {
		std::fprintf(stderr, "sendContents: not found\n");
        return false;
    }

    std::fprintf(stderr, "\n");


    /* For data blocks in 64kb, stage data, and send blob write command. */
    inputFd = sys->open(input.c_str(), 0);
    if (inputFd < 0)
    {
        std::fprintf(stderr, "Unable to open file: '%s'\n", input.c_str());
        goto exit;
    }

    fileSize = sys->getSize(input.c_str());
    if (fileSize == 0)
    {
        std::fprintf(stderr, "sendContents: Zero-length file, or other file access error\n");
        goto exit;
    }

    progress->start(fileSize);

    readBuffer = std::make_unique<std::uint8_t[]>(p2aLength);
    if (nullptr == readBuffer)
    {
        std::fprintf(stderr, "sendContents: Unable to allocate memory for read buffer.\n");
        goto exit;
    }
	do
	{
		bytesRead = sys->read(inputFd, readBuffer.get(), p2aLength);
		if (bytesRead > 0)
		{
			/* TODO: Will likely need to store an rv somewhere to know when
			 * we're exiting from failure.
			 */
			if (!io->write(bar + P2aOffset, bytesRead, readBuffer.get()))
			{
				std::fprintf(stderr,
							 "Failed to write to region in memory!\n");
				goto exit;
			}

			/* Ok, so the data is staged, now send the blob write with the
			 * details.
			 */
			struct ipmi_flash::ExtChunkHdr chunk;
			chunk.length = bytesRead;
			std::vector<std::uint8_t> chunkBytes(sizeof(chunk));
			std::memcpy(chunkBytes.data(), &chunk, sizeof(chunk));

			offset += bytesRead;
			progress->updateProgress(bytesRead);
		}
	} while (bytesRead > 0);

    /* defaults to failure. */
    returnValue = true;

exit:

    /* close input file. */
    if (inputFd != -1)
    {
        sys->close(inputFd);
    }
    return returnValue;
}

} // namespace host_tool
