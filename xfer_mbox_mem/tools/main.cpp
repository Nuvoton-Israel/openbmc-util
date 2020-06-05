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


#include "io.hpp"
#include "p2a.hpp"
#include "pci.hpp"
#include "progress.hpp"
#include "tool_errors.hpp"
#include "updater.hpp"

/* Use CLI11 argument parser once in openbmc/meta-oe or whatever. */
#include <getopt.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <ipmi_handler.hpp>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#define IPMILPC "ipmilpc"
#define IPMIPCI "ipmipci"
#define IPMIBT "ipmibt"
#define IPMINET "ipminet"

namespace
{
const std::vector<std::string> interfaceList = {IPMINET, IPMIBT, IPMILPC,
                                                IPMIPCI};
} // namespace

void usage(const char* program)
{
    std::fprintf(
        stderr,
        "Usage: %s --command <command> --interface <interface> --image "
        "<image file> --sig <signature file>\n",
        program);

    std::fprintf(stderr, "interfaces: ");
    std::copy(interfaceList.begin(), interfaceList.end(),
              std::ostream_iterator<std::string>(std::cerr, ", "));
    std::fprintf(stderr, "\n");
}

bool checkCommand(const std::string& command)
{
    return (command == "update");
}

bool checkInterface(const std::string& interface)
{
    auto intf =
        std::find(interfaceList.begin(), interfaceList.end(), interface);
    return (intf != interfaceList.end());
}

int main(int argc, char* argv[])
{
    std::string command, interface, imagePath, signaturePath, type, host;

    while (1)
    {
        // clang-format off
        static struct option long_options[] = {
            {"command", required_argument, 0, 'c'},
            {"interface", required_argument, 0, 'i'},
            {"image", required_argument, 0, 'm'},
            {"sig", required_argument, 0, 's'},
            {0, 0, 0, 0}
        };
        // clang-format on

        int option_index = 0;
        int c = getopt_long(argc, argv, "c:i:m:s", long_options,
                            &option_index);
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 'c':
                command = std::string{optarg};
                if (!checkCommand(command))
                {
					printf("Here--1\n");
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }

                break;
            case 'i':
                interface = std::string{optarg};
                if (!checkInterface(interface))
                {
					printf("Here--2\n");
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'm':
                imagePath = std::string{optarg};
                break;
            case 's':
                signaturePath = std::string{optarg};
                break;
            default:
				printf("Here--3\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (command.empty())
    {
		printf("Here--4\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* They want to update the firmware. */
    if (command == "update")
    {
        if (interface.empty() || imagePath.empty() || signaturePath.empty())
        {
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }

#ifdef ENABLE_PPC
        const std::string ppcMemPath = "/sys/kernel/debug/powerpc/lpc/fw";
        host_tool::PpcMemDevice devmem(ppcMemPath);
#else
        host_tool::DevMemDevice devmem;
#endif
        host_tool::PciUtilImpl pci;
        host_tool::ProgressStdoutIndicator progress;

        std::unique_ptr<host_tool::DataInterface> handler;

        /* Input has already been validated in this case. */
        if (interface == IPMIPCI)
        {
            handler = std::make_unique<host_tool::P2aDataHandler>(
                &devmem, &pci, &progress);
        }

        if (!handler)
        {
            /* TODO(venture): use a custom exception. */
            std::fprintf(stderr, "Interface %s is unavailable\n",
                         interface.c_str());
            exit(EXIT_FAILURE);
        }

        /* The parameters are all filled out. */
        try
        {
            host_tool::UpdateHandler updater(handler.get());
            host_tool::updaterMain(&updater, imagePath, signaturePath);
        }
        catch (const host_tool::ToolException& e)
        {
            std::fprintf(stderr, "Exception received: %s\n", e.what());
            return -1;
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unexpected exception received: %s\n",
                         e.what());
            return -1;
        }
    }

    return 0;
}
