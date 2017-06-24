#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILE_NAME "simple_message_slot"

int main() {
	int file_desc = open("/dev/"DEVICE_FILE_NAME, 0);
	printf("USER: %d\n",file_desc);
	close(file_desc);
	return 0;
}
