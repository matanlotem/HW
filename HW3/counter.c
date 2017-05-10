#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#define COUNTER_USAGE_ERROR "Usage: counter <character> <filename> <offset> <length>\n"
#define FILE_READ_ERROR "Error reading file: %s\n"
#define FILE_MAP_ERROR "Error mapping file to memory: %s\n"
#define UNMAP_ERROR "Error un-mapping file: %s\n"
#define PIPE_CREATE_ERROR "Error creating named pipe: %s\n"
#define PIPE_OPEN_ERROR "Error opening named pipe: %s\n"
#define PIPE_WRITE_ERROR "Error writing to named pipe: %s\n"
#define PIPE_DEL_ERROR "Error deleting named pipe: %s\n"


#define STR_LEN 1025
#define PIPE_NAME "/tmp/counter_%d"

void count(char character, char* filename, off_t offset, off_t length) {
	int fd, pipeFd;
	char* arr;
	char pipeName[STR_LEN];

	// open file for reading
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf(FILE_READ_ERROR, strerror(errno));
		return;
	}

	// map file
	arr = (char*) mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
	if (arr == MAP_FAILED) {
		printf(FILE_MAP_ERROR, strerror(errno));
		close(fd);
		return;
	}

	// count
	off_t counter = 0;
	for (int i=0; i<length; i++)
		if (arr[i] == character)
			counter++;

	// create and open pipe
	sprintf(pipeName, PIPE_NAME, getpid());
	if (mkfifo(pipeName, 0777) == -1) {
		printf(PIPE_CREATE_ERROR, strerror(errno));
		goto CLEANUP;
	}
	pipeFd = open(pipeName, O_WRONLY);
	if (pipeFd == -1) {
		printf(PIPE_OPEN_ERROR, strerror(errno));
		goto CLEANUP;
	}

	//TODO: send signal to dispatcher

	// write counter to pipe
	if (write(pipeFd, sizeof(off_t), counter) == -1)
		printf(PIPE_WRITE_ERROR, strerror(errno));

	sleep(1);
	CLEANUP:
	// unmap file
	if (munmap(arr, length) == -1) {
		printf(UNMAP_ERROR, strerror(errno));
		return;
	}
	close(fd);

	// close and remove pipe
	if (pipeFd >= 0) close(pipeFd);
	if (unlink(pipeName) == -1) // delete pipe
		printf(PIPE_DEL_ERROR, strerrno(errno));
}

/*
 * Check if string consists only of digits
 */
int isOffset(char* str) {
	if (strlen(str) == 0)
		return 0;

	for (int i=0; i<strlen(str); i++)
		if (str[i] < '0' || str[i] > '9')
			return 0;
	return 1;
}

/*
 * Convert string to offset
 * returns -1 if string is not only digits
 */
off_t toOffset(char* str) {
	if (isOffset(str)) {
		off_t offset = 0;
		for (int i=strlen(str)-1; i>=0; i--)
			offset = offset * 10 + (str[i] - '0');
		return offset;
	}
	return -1;
}

int main(int argc, char* argv[]) {
	if (argc < 5 || strlen(argv[1]) != 1 || !isOffset(argv[3]) || !isOffset(argv[4])) {
		printf(COUNTER_USAGE_ERROR);
		return -1;
	}
	count(argv[1][0], argv[2], toOffset(argv[3]), toOffset(argv[4]));
	return 0;
}
