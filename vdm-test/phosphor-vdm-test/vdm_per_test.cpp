#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <asm/ioctl.h>
#include <sys/ioctl.h>                       /* For ioctl */
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include "vdm_module.h"

#define DEFAULT_RX_BUFF_SIZE 64*1024
#define DEFAULT_TX_BUFF_SIZE 128*1024

bdf_arg_t  bdf;
uint32_t rxbufersize=DEFAULT_RX_BUFF_SIZE;
uint32_t txbufersize=DEFAULT_TX_BUFF_SIZE, txsize;
int DataIdx;

#define SEND_DATA_BUFF_SIZE  8192
uint8_t data_to_send[SEND_DATA_BUFF_SIZE+1];


#define MTU 80 //increased by 4 

struct mctp_pcie_packet {
	struct {
		uint32_t hdr[3]; //pcie header
		uint32_t payload[(MTU - 12)>>2];
	} data;
	uint32_t size;
};


int main (int argc, char **argv)
{
    long ret;
    int i, writelen, cnt;
    int fd = -1;
    struct timeval t0,t1;
    double diff;
    uint64_t total = 0;
    mctp_pcie_packet pkt;

    for (i = 1; i < argc; i+=2)
    {
	    if (i+1>=argc) abort();

	    if(0 == strcmp(argv[i],"--bus"))
	    {
		    bdf.bus=atoi(argv[i+1]);
	    }
	    else if(0 == strcmp(argv[i],"--dev"))
	    {
		    bdf.device=atoi(argv[i+1]);
	    }
	    else if(0 == strcmp(argv[i],"--func"))
	    {
		    bdf.function=atoi(argv[i+1]);
	    }
	    else if(0 == strcmp(argv[i],"--txbufsize"))
	    {
		    txbufersize=atoi(argv[i+1]);
	    }
	    else if(0 == strcmp(argv[i],"--txsize"))
	    {
		    txsize=atoi(argv[i+1]);
	    }
    }

    fd = open("/dev/vdm",  O_RDWR, 0);
    if(fd < 0)
    {
        printf(" Opening VDM device Failed\n");
        return -1;
    }

    ret = ioctl(fd, PCIE_VDM_SET_BDF, &bdf);
    if (ret < 0) {
        printf("ioctl failed. Return code: %ld \n", ret );
        return ret;
    }

    ret = ioctl(fd, PCIE_VDM_SET_TRANSMIT_BUFFER_SIZE , txbufersize);
    if (ret < 0) {
        printf("ioctl failed. Return code: %ld \n", ret );
        return ret;
    }


    if (txsize < MTU)
        writelen = txsize;
    else
        writelen = MTU;

    cnt = txsize /MTU;
    if (txsize % MTU)
        cnt+=1;

    printf("bus = %d device = %d function=%d txbufersize=%d txsize=%d\n",bdf.bus,bdf.device,bdf.function,txbufersize, txsize);


    data_to_send[0]=0x12;

    for (i=1 ; i < writelen + 1 ; i++) {
	    data_to_send[i] = i % 256;
    }

    gettimeofday(&t0,NULL);
    for (i = 0; i < cnt; i++) {
        int wrrten = 0;
        wrrten = (writelen + 1) - 12; //+ byte 0 - pcie header 
  
        ret = write(fd, data_to_send, wrrten);    
        if (ret < 0) {
            printf("write failed. Return code: %ld \n", ret );
            return ret;
        }

        total+=(ret - 1 + 12);  //- byte 0 + pcie header 
    }
    gettimeofday(&t1,NULL);
    diff=(t1.tv_sec - t0.tv_sec)*1000 + (t1.tv_usec - t0.tv_usec)/1000.0;

   // uint64_t total = (writelen * cnt);
    double sperb = (float)(total * 1000)/diff;

    printf("MTU %d byte\n", sizeof(pkt.data));
    printf("Total send (including pci header) %lld byte\n", total);
    printf("Test measure %10.5f msec\n", diff);
    printf("Performance %10.5f MB/s\n", sperb/1024/1024);
    
    close(fd);
}
