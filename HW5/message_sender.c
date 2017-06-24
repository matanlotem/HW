#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
	int file_desc = open("/dev/simple_message_slot", 0);
	printf("USER: %d\n",file_desc);
	close(file_desc);
	return 0;
}
