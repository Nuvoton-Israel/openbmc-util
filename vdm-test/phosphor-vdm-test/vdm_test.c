 #include <ctype.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <stdint.h>
 #include <string.h>
#include <asm/ioctl.h>
#include <sys/ioctl.h>                       /* For ioctl */
#include <sys/time.h>
#include <fcntl.h>
#include <poll.h>

 #include "vdm_module.h"

typedef enum
{
	Mode_Receive=0,
	Mode_Send,
	Mode_Receive_Poll,
} Mode_t;


const char *cmd_usage =
		"usage : \n"
		"vdm_test --bus XX --dev XX --func XX [--rxbufsize kb_size] [--txbufsize kb_size] --mode mode_str --route route_mode --data XXXX\n "
		"XX - number \n"
		"kb_size - size in kbytes \n"
		"mode_str - {receive,send,receive_poll} \n";

#define DEFAULT_RX_BUFF_SIZE 64*1024
#define DEFAULT_TX_BUFF_SIZE 64*1024

bdf_arg_t  bdf;
uint32_t rxbufersize=DEFAULT_RX_BUFF_SIZE;
uint32_t txbufersize=DEFAULT_TX_BUFF_SIZE;
Mode_t WorkingMode;
int DataIdx;

#define SEND_DATA_BUFF_SIZE  100
uint8_t data_to_send[SEND_DATA_BUFF_SIZE+1];

unsigned long get_time() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        unsigned long ret = tv.tv_usec;
        ret /= 1000;
        ret += (tv.tv_sec * 1000);
        return ret;
}

int parse_arguments(int argc, char **argv)
{
	int              i = 0;
    uint8_t route_mode = 0;

    if(argc < 6)
    {
		printf("%s", cmd_usage);
        return -1;
    }

    //Initialize Data count
    DataIdx = 0;

	for (i = 1; i < argc; i+=2)
	{
		if (i+1>=argc) abort();

		//printf("arg = %s \n",argv[i]);

		if(0 == strcmp(argv[i],"--bus"))
		{
			bdf.bus=atoi(argv[i+1]);
			printf("bus = %d \n", bdf.bus );
		}
		else if(0 == strcmp(argv[i],"--dev"))
		{
			bdf.device=atoi(argv[i+1]);
			printf("device = %d \n", bdf.device );
		}
		else if(0 == strcmp(argv[i],"--func"))
		{
			bdf.function=atoi(argv[i+1]);
			printf("function = %d \n", bdf.function );
		}
		else if(0 == strcmp(argv[i],"--rxbufsize"))
		{
			rxbufersize=atoi(argv[i+1])*1024;
			printf("rxbufersize = %d \n", rxbufersize );
		}
		else if(0 == strcmp(argv[i],"--txbufsize"))
		{
			txbufersize=atoi(argv[i+1])*1024;
			printf("txbufersize = %d \n", txbufersize );
		}
		else if(0 == strcmp(argv[i],"--mode"))
		{
			if(0 == strcmp(argv[i+1],"receive"))
			{
				WorkingMode=Mode_Receive;
			}
			else if(0 == strcmp(argv[i+1],"receive_poll"))
			{
				WorkingMode=Mode_Receive_Poll;
			}
			else if(0 == strcmp(argv[i+1],"send"))
			{
				WorkingMode=Mode_Send;
			}
			printf("WorkingMode = %d \n", WorkingMode );
		}
		else if(0 == strcmp(argv[i],"--route"))
		{
			route_mode = atoi(argv[i+1]);
			printf("route_mode = %d \n", route_mode );
		}
		else if(0 == strcmp(argv[i],"--data"))
		{
            int  DataLen = 0, error = 0;
            char *data = NULL, payload[255];
            data = argv[i+1]; 
            printf("Data %s\n", data);
            DataLen = strlen(argv[i+1]);
            printf("DataLen = %d, Total No of bytes = %d\n", DataLen, DataLen/2);
            if(((DataLen/2) > SEND_DATA_BUFF_SIZE) || ((DataLen%2) != 0))
            {
                error = 1;
                printf("Error : Data Length is not byte aligned or is Max!!!\n");
            }

            int ch      = 0;
            int CurrIdx = 0;
            
            //route mode : As per the driver
            data_to_send[DataIdx] = (0x10 | route_mode);
            printf("PCIe-VDM Routing mode : data_to_send[%d] : %x\n", DataIdx, data_to_send[DataIdx]);
            DataIdx++; 

            while((DataLen > 0) && (error == 0))
            {
                unsigned int  number = 0;
                int j = 0;
                printf("CurrIdx : %d, j : %d, DataLen : %d,", CurrIdx, j, DataLen);
                while((DataLen >= 2) && (j < 2))
                {
                    ch = data[CurrIdx + j];
                    printf(" ch = %c, ", ch);
                    if(('0' <= ch && ch <= '9'))
                    {
                        number = number * 16;
                        number = number + (ch - '0');
                    }
                    else if('A' <= ch && ch <= 'F') 
                    {
                        number = number * 16;
                        number = number + (0x0A + (ch - 'A'));
                    }
                    else if('a' <= ch && ch <= 'f')
                    {
                        number = number * 16;
                        number = number + (0x0A + (ch - 'a'));
                    }
                    else
                    {
                        printf("Unknown Hex value : character = %c\n", ch);
                        error = 1;
                        break;
                    }
                    j++; 
                }
                data_to_send[DataIdx] = number;
                printf("data_to_send[%d] : %x\n", DataIdx, number);
                CurrIdx+=2; 
                DataIdx++;
                DataLen-=2;
            }

		//	strncpy(data_to_send,argv[i+1],SEND_DATA_BUFF_SIZE);
        printf("Data len : %d\n", DataIdx);
		}

	}
    return 0;
}

uint8_t received_packet_buffer[64*1024];

int main (int argc, char **argv)
{
	static int packet_count=0;
    long ret;
    int packet_length;
    int timeout = 0;
    int fd = -1;
    struct pollfd fds[1];
    int i,j=0,start=0;
    long start_time;

    if(parse_arguments(argc, argv) != 0)
        return -1;

    //Test
    //return 0;
    fd = open("/dev/vdm",  O_RDWR, 0);
    fds[0].fd = fd;
    fds[0].events = POLLIN | POLLRDNORM;

    if(fd < 0)
    {
        printf(" Opening VDM device Failed\n");
        return -1;
    }

    ret = ioctl(fd, PCIE_VDM_SET_BDF , &bdf);
	printf(" vdm_test : cmd = %d \n",PCIE_VDM_SET_BDF);

    if (ret < 0)
        printf("ioctl failed. Return code: %d \n", ret );

    ret = ioctl(fd, PCIE_VDM_SET_TRANSMIT_BUFFER_SIZE , txbufersize);
	printf(" vdm_test : cmd = %d \n",PCIE_VDM_SET_TRANSMIT_BUFFER_SIZE);
    if (ret < 0)
        printf("ioctl failed. Return code: %d \n", ret );

    ret = ioctl(fd, PCIE_VDM_SET_RECEIVE_BUFFER_SIZE , rxbufersize);
	printf(" vdm_test : cmd = %d \n",PCIE_VDM_SET_RECEIVE_BUFFER_SIZE);
    if (ret < 0)
        printf("ioctl failed. Return code: %d \n", ret );

    if(Mode_Send==WorkingMode)
    {
    	//ret = write(fd,data_to_send,strlen(data_to_send));
        //printf("VDM App Tx Data : %s\n", data_to_send); 
    	ret = write(fd, data_to_send, DataIdx);
        printf("Tx Done with ret val : %d\n", ret);
        printf("Waiting on Rx for 15s....\n");
		while(1)
		{
			ret=read(fd,&received_packet_buffer[0],4);
			if(4 == ret)
			{
				packet_length=  (received_packet_buffer[2] << 8) + received_packet_buffer[3]  + 4;// 4 for pcie header length
				printf("packet %d with length  %d \n", packet_count++ ,packet_length);
				packet_length=packet_length*4 ;// convert to bytes
				ret=read(fd,&received_packet_buffer[4],packet_length - 4 );
//				printf("first data =  0x%x 0x%x 0x%x 0x%x\n",received_packet_buffer[16],received_packet_buffer[17],
//							received_packet_buffer[18],received_packet_buffer[19]);
//				printf("second data =  0x%x 0x%x 0x%x 0x%x\n",received_packet_buffer[20],received_packet_buffer[21],
//							received_packet_buffer[22],received_packet_buffer[23]);
                timeout = 0;
			}
			else
			{
				sleep(1);
                timeout++;
			}
            if(timeout >= 15)
            {
                printf("Rx Timeout : %d\n", timeout);
                break;
            }
		}
    }
    else if(Mode_Receive_Poll==WorkingMode)
    {
	    i=0;
		while(j<95000)
		{
			fds[0].events = POLLIN | POLLRDNORM;
			poll(fds, 1, -1);
			if (!start) {
				start = 1;
				start_time = get_time();
			}
			if (fds[0].revents & POLLIN)
			{
				#if 1
				ret=read(fd,&received_packet_buffer[0],4);
				if(4 == ret)
				{
					i++;
					if (i==5000){
						j=j+i;
						//printf("%d\n",j);
						i=0;
					}
					/*printf("Header -> 0x%x 0x%x 0x%x 0x%x\n", received_packet_buffer[0], received_packet_buffer[1],
					       received_packet_buffer[2],received_packet_buffer[3]);*/
					packet_length=  (received_packet_buffer[2] << 8) + received_packet_buffer[3]  + 4;// 4 for pcie header length
					//printf("poll : packet %d with length  %d \n", packet_count++ ,packet_length);
					packet_length=packet_length*4 ;// convert to bytes
					ret=read(fd,&received_packet_buffer[4],packet_length - 4 );
					/*for (i = 16 ; i < packet_length ; i+=4)
						printf("Address %d =  0x%x 0x%x 0x%x 0x%x\n", i - 16, received_packet_buffer[i], received_packet_buffer[i+1],
									received_packet_buffer[i+2],received_packet_buffer[i+3]);*/
				}
				#endif
			}
			else
				printf("read stuck %d event 0x%x\n",i,fds[0].revents);
		}
		printf ("It took %ld ms to count to 10^8.\n",get_time() - start_time);
    }
    else if(Mode_Receive==WorkingMode)
    {
		while(1)
		{
			ret=read(fd,&received_packet_buffer[0],4);
			if(4 == ret)
			{
				printf("Header -> 0x%x 0x%x 0x%x 0x%x\n", received_packet_buffer[0], received_packet_buffer[1],
				       received_packet_buffer[2],received_packet_buffer[3]);
				packet_length = (received_packet_buffer[2] << 8) + received_packet_buffer[3]  + 4;// 4 for pcie header length
				printf("packet %d with length  %d \n", packet_count++ ,packet_length);
				packet_length=packet_length*4 ;// convert to bytes
				ret=read(fd,&received_packet_buffer[4],packet_length - 4 );
				for (i = 16 ; i < packet_length ; i+=4) {
					printf("Address %d =  0x%x 0x%x 0x%x 0x%x\n", i - 16, received_packet_buffer[i], received_packet_buffer[i+1],
								received_packet_buffer[i+2],received_packet_buffer[i+3]);
				}
			}
			else
			{
				sleep(5);
			}
		}
    }
    close(fd);
}

