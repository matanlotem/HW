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
	if (argc < 2) {
		printf(READER_USAGE_ERROR);
		return -1;
	}

	// open device
	int fd = open("/dev/"DEVICE_FILE_NAME, O_RDWR);
	if (fd < 0) {
		printf(DEVICE_OPEN_ERROR, strerror(errno));
		return -1;
	}

	// send channel selection IOCTL
	int channel_index = atoi(argv[1]);
	res = ioctl(fd, IOCTL_SET_CHANNEL, channel_index);
	if (res < 0)
		printf(SET_CHANNEL_ERROR, strerror(errno));
	else {
		// read from device
		char buffer[BUFFER_LEN];
		res = read(fd, buffer, BUFFER_LEN);
		if (res < 0)
			printf(DEVICE_READ_ERROR, strerror(errno));
		// success
		else {
			printf(READER_SUCCESS_MSG, channel_index);
			printf("%s\n", buffer);
		}
	}

	close(fd);
	return res;
}
