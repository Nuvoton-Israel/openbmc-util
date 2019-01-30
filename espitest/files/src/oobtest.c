/* Copyright (c) 2018, Nuvoton Corporation */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <poll.h>

#define OOBIOC_BASE 'O'
#define OOB_GET             _IOR(OOBIOC_BASE, 1, struct oob_msg)
#define OOB_SEND            _IOW(OOBIOC_BASE, 2, struct oob_msg)

struct oob_msg {
    char data[80];
    unsigned int len;
};

int main(int argc, char **argv)
{
	int devfd;
	struct pollfd fds;
	struct oob_msg msg;

	devfd = open("/dev/npcm7xx-espi-oob", O_RDWR);
	if (devfd == -1) {
		fprintf(stderr, "Can't open device\n");
		return 0;
	}
#if 1
	/* Get PCH Temperature */
	msg.len = 4;

	//msg.data[0] = 7;//pktlen
	//msg.data[1] = 0x21;//cycle type
	//msg.data[2] = 0;//len[11:8]
	//msg.data[3] = 4;//len[7:0]

	msg.data[0] = 2;//dest slave addr:01h
	msg.data[1] = 1;//command code:01h
	msg.data[2] = 1;//byte count
	msg.data[3] = 0x1F;//source slave addr:0Fh
#endif
#if 0
	/* Test */
	msg.len = 6;
	msg.data[0] = 1;
	msg.data[1] = 2;
	msg.data[2] = 3;
	msg.data[3] = 4;
	msg.data[4] = 5;
	msg.data[5] = 6;
#endif
	if (ioctl(devfd, OOB_SEND, &msg) < 0) {
		perror("ioctl OOB_SEND");
		return 0;
	}

	fds.fd = devfd;
	fds.events = POLLIN;
	//fprintf(stderr, "wait for VW state changed\n");
	if (poll(&fds, 1, -1) == -1) {
		perror("poll");
		goto quit;
	}

	if (fds.revents & POLLIN) {
		int i;

		if (ioctl(devfd, OOB_GET, &msg) < 0) {
			perror("ioctl OOB_GET");
			return 0;
		}
		printf("msg len = %d\n", msg.len);
		for (i = 0; i < msg.len + 3; i++)
			printf("oob: 0x%02x\n", msg.data[i]);

	} else if (fds.revents & POLLHUP) {
		printf("eSPI hang up\n");
	}

quit:
	fprintf(stderr, "quit..\n");

	close(devfd);
	return 0;
}
