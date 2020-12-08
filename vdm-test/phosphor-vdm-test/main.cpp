/**
 * Copyright 2017 Google Inc.
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

#include <fcntl.h>
#include <getopt.h>
#include <systemd/sd-event.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/source/io.hpp>
#include <thread>
#include <cstring>
#include <asm/ioctl.h>
#include <sys/ioctl.h> 

#include <time.h>

#include <sys/types.h>
#include <sys/time.h>
#include <poll.h>
#include  "vdm_module.h"

static const char* vdmFilename = "/dev/vdm";
static uint32_t mtu = 80;
#define DEFAULT_RX_BUFF_SIZE 64 * 1024

typedef enum
{
	Mode_Default = 0,
	Mode_Send_Only = 1,
	Mode_Receive_Only,
} Mode_t;

Mode_t WorkingMode;

/* PCI vdm definitions */
struct pci_hdr {
	uint16_t rid;
	uint16_t tid;
	uint8_t pad;
	uint8_t len;
    uint8_t type;
};

struct mctp_pcie_packet {
	struct {
		uint32_t hdr[4];
		uint8_t *payload;
	} data;
	uint32_t size;
};

int prepare_pci_header(uint32_t* data, struct pci_hdr *hdr)
{
    if (data && hdr) {
        uint32_t VDM_VENDOR_ID = 0x1ab4;
        uint32_t PCIE_HEADER0_FMT = (0x03 << 5);
        uint32_t PCIE_HEADER0_TYPE = (0x10 | hdr->type);
        uint32_t PCIE_HEADER0_ATTR = (0x00 << 4) << 16;
        uint32_t PCIE_HEADER0_LENGTH = hdr->len << 24;

        uint32_t PCIE_HEADER1_TAG = (hdr->pad << 4) << 16;
        uint32_t PCIE_HEADER1_VED = 0x7f << 24;

        uint32_t PCIE_HEADER2_VENDORID =  (((VDM_VENDOR_ID >> 8) & 0xff) << 16) | ((VDM_VENDOR_ID & 0xff) << 24 );

        data[0] = PCIE_HEADER0_FMT | PCIE_HEADER0_TYPE | PCIE_HEADER0_ATTR | PCIE_HEADER0_LENGTH;
        data[1] = ((hdr->rid >> 8) & 0xff) | ((hdr->rid & 0xff) << 8) | PCIE_HEADER1_TAG | PCIE_HEADER1_VED;
        data[2] = ((hdr->tid >> 8) & 0xff) | ((hdr->tid & 0xff) << 8) | PCIE_HEADER2_VENDORID;
        return 0;
    }
    return -1;
}


int main(int argc, char* argv[])
{
    int rc = 0, len = 0, i, cnt, timeout_msecs = -1;
    uint8_t *buf;
    uint32_t txsize = mtu, writelen, btu, rxsize = DEFAULT_RX_BUFF_SIZE, txloop = 100,  txdelay = 100;
    uint64_t total = 0;
    struct pollfd fds;
    struct mctp_pcie_packet pkt;
    struct pci_hdr pci;
    struct timeval t0, t1;
    double sperb, diff;
    ssize_t written = 0;

    bdf_arg_t bdf = {};

    for (i = 1; i < argc; i+=2) {
        if (i + 1 >= argc) abort();

        if (!strcmp(argv[i],"--bus"))
		    bdf.bus =atoi(argv[i+1]);
	    else if(!strcmp(argv[i],"--dev"))
		    bdf.device=atoi(argv[i+1]);
	    else if(!strcmp(argv[i],"--func"))
		    bdf.function=atoi(argv[i+1]);
	    else if(!strcmp(argv[i],"--txsize"))
		    txsize = atoi(argv[i+1]);
	    else if(!strcmp(argv[i],"--txloop"))
		    txloop = atoi(argv[i+1]);
	    else if(!strcmp(argv[i],"--txdelay"))
		    txdelay = atoi(argv[i+1]);
	    else if(!strcmp(argv[i],"--rxbufsize"))
		    rxsize = atoi(argv[i+1]);
	    else if(!strcmp(argv[i],"--mtu"))
		    mtu = atoi(argv[i+1]);
        else if(!strcmp(argv[i],"--mode"))
		{
			if(!strcmp(argv[i+1],"rx"))
				WorkingMode = Mode_Receive_Only;
			else if(!strcmp(argv[i+1],"tx"))
				WorkingMode = Mode_Send_Only;
		}        
    }

    fds.fd = open(vdmFilename, O_RDWR);
    if (fds.fd  < 0)
    {
        fprintf(stderr, "Unable to open: %s\n", vdmFilename);
        return -1;
    }

    rc = ioctl(fds.fd, PCIE_VDM_REINIT , 0);
    if (rc < 0)
        printf("ioctl PCIE_VDM_REINIT failed. Return code: %d \n", rc);

    rc = ioctl(fds.fd, PCIE_VDM_SET_RECEIVE_BUFFER_SIZE , rxsize);
    if (rc < 0)
        printf("ioctl PCIE_VDM_SET_RECEIVE_BUFFER_SIZE failed. Return code: %d \n", rc);

    rc = ioctl(fds.fd, PCIE_VDM_SET_BDF , &bdf);
    if (rc < 0)
        printf("ioctl failed. Return code: %d \n", rc );

    btu = mtu - sizeof(pkt.data.hdr);
    buf = (uint8_t*)malloc(mtu);

    if (WorkingMode != Mode_Receive_Only) {

        for (i = 0; i < (int)btu; i++)
            buf[i + sizeof(pkt.data.hdr)] = rand() % 256;

        memset(&pci, 0 ,sizeof(pci));
        pci.rid = (0 << 8) | (0 << 3) | 0x00;
        pci.tid = (bdf.bus << 8) | (bdf.device << 3) | bdf.function;
        pci.type = 2;
        pci.pad = 4 - (btu % 4);
        pci.len = (btu >> 2);

        if (btu < 4)
            pci.len = 1;
        else if (len % 4)
            pci.len += 1;
        prepare_pci_header((uint32_t *)buf, &pci);

        if (txsize < mtu)
            writelen = txsize;
        else
            writelen = mtu;

        cnt = txsize /mtu;
        if (txsize % mtu)
            cnt+=1;

        for (uint32_t loop = 0; loop < txloop; loop++) {
            gettimeofday(&t0 ,NULL);
            total = 0;

            for (i = 0; i < cnt; i++) {
                written = write(fds.fd , (void *)buf,  writelen);
                if (written < 0) {
                    int ret, error;
                    ret = ioctl(fds.fd, PCIE_VDM_GET_ERRORS , &error);
                    if (ret < 0)
                        printf("ioctl PCIE_VDM_GET_ERRORS failed. Return code: %d \n", ret);

                    fprintf(stderr, "write error %d i %d\n", error, i);

                    ret = ioctl(fds.fd, PCIE_VDM_CLEAR_ERRORS , error);
                    if (ret < 0)
                        printf("ioctl PCIE_VDM_CLEAR_ERRORS failed. Return code: %d \n", ret);
                    i--;
                } else {
                    total += written;
                }
            }

            gettimeofday(&t1,NULL);

            diff=(t1.tv_sec - t0.tv_sec)*1000 + (t1.tv_usec - t0.tv_usec)/1000.0;
            sperb = (double)(total * 1000)/diff;

            printf("MTU %d byte\n", mtu);
            printf("Test measure send %lld byte %10.5f msec\n", total, diff);
            printf("Performance %10.5f MB/s\n", sperb/1024/1024);
            usleep(txdelay * 1000);
        }
    }

    if (WorkingMode == Mode_Send_Only)
        goto out;

    fds.events = POLLIN | POLLRDNORM;
    len = 0;

    while(1) {
        uint32_t *rxbuf;

        printf("Waiting on Rx....\n");    
        rc = poll(&fds, 1, timeout_msecs);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                 fprintf(stderr, "poll error");
                break;
            }
        }

        /* process internal fd first */
        if (fds.revents) {
            rc = read(fds.fd, buf, mtu);
            if (rc <= 0) {
                int error = 0, ret;
                //fprintf(stderr, "Error reading from tty device %d\n", rc);

                ret = ioctl(fds.fd, PCIE_VDM_GET_ERRORS , &error);
                if (ret < 0)
                    printf("ioctl PCIE_VDM_GET_ERRORS failed. Return code: %d \n", ret);

                fprintf(stderr, "Error reading from tty device %d\n", error);

                ret = ioctl(fds.fd, PCIE_VDM_CLEAR_ERRORS , error);
                if (ret < 0)
                    printf("ioctl PCIE_VDM_CLEAR_ERRORS failed. Return code: %d \n", ret);
                continue;
               // PCIE_VDM_GET_ERRORS
                //rc = -1;
                //break;
            }
            len = (rc >> 2) + (rc % 4);
            total+=len;
            rxbuf = (uint32_t *)buf;
            for (i = 0; i < len; i++)
                fprintf(stderr, "read 0x%x\n", rxbuf[i]);
        }
        fprintf(stderr, "Total receive %lld bytes\n", total *4);
    }
out:
    close(fds.fd);
    free(buf);
    return rc;
}
