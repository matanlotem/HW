#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "message_slot.h"


int main(int argc, char* argv[]) {
	// check args
	int res;
	if (argc < 3) {
		printf(SENDER_USAGE_ERROR);
		return -1;
	}

	// open device
	int fd = open("/dev/"DEVICE_FILE_NAME, O_RDWR);
	if (fd < 0) {
		printf(DEVICE_OPEN_ERROR, strerror(errno));
		return -1;
	}

	// send channel selection IOCTL
	int channel_index = atoi(argv[1]), len = strlen(argv[2]);
	res = ioctl(fd, IOCTL_SET_CHANNEL, channel_index);
	if (res < 0)
		printf(SET_CHANNEL_ERROR, strerror(errno));
	else {
		// write to device
		res = write(fd, argv[2], len);
		if (res < 0)
			printf(DEVICE_WRITE_ERROR, strerror(errno));
		// success
		else
			printf(SENDER_SUCCESS_MSG, len, channel_index);
	}

	close(fd);
	return res;
}
