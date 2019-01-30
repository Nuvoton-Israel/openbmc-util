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

struct vw_cmd {
	int direction;
	char index;
	char data;
};
struct vw_state {
	unsigned int state[8];
};
#define VWIOC_BASE    'V'
#define VW_GET         		_IOR(VWIOC_BASE, 1, struct vw_cmd)
#define VW_PUT         		_IOW(VWIOC_BASE, 2, struct vw_cmd)
#define VW_GET_STATES       _IOR(VWIOC_BASE, 3, struct vw_state)

int main(int argc, char **argv)
{
	int devfd;
	struct vw_cmd vwcmd;
	struct vw_state vwstate;
	struct pollfd fds;

	devfd = open("/dev/npcm7xx-espi-vw", O_RDWR);
	if (devfd == -1) {
		fprintf(stderr, "Can't open device\n");
		return 0;
	}

	vwcmd.direction = 0;
	vwcmd.index = 3;
	vwcmd.data = 0;
	if (ioctl(devfd, VW_GET, &vwcmd) < 0) {
		perror("VW_GET error\n");
		return 0;
	}
	printf("index 3: data = 0x%x\n", vwcmd.data);
	sleep(3);
#if 1
	fds.fd = devfd;
	fds.events = POLLIN;
	//fprintf(stderr, "wait for VW state changed\n");
	if (poll(&fds, 1, -1) == -1) {
		perror("poll");
		goto quit;
	}

	//fprintf(stderr, "poll return\n");
	if (fds.revents & POLLIN) {
		int i;
		//fprintf(stderr, "VW state changed\n");

		if (ioctl(devfd, VW_GET_STATES, &vwstate) < 0) {
			perror("ioctl VW_GET_STATES");
			return 0;
		}
		sleep(2);
		for (i = 0; i < 8; i++) {
			fprintf(stderr, "VW state%d = 0x%08x\n", i, vwstate.state[i]);
		}
		sleep(2);

		vwcmd.direction = 0;
		vwcmd.index = 3;
		vwcmd.data = 0;
		if (ioctl(devfd, VW_GET, &vwcmd) < 0) {
			fprintf(stderr, "ioctl error\n");
			return 0;
		}
		sleep(2);
		printf("index 3: data = 0x%x\n", vwcmd.data);
	} else if (fds.revents & POLLHUP) {
		printf("eSPI hang up\n");
	}
#endif
quit:
	fprintf(stderr, "quit..\n");

	close(devfd);
	return 0;
}
