#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>

#define PIPE_NAME "/tmp/counter_%d"
#define SLEEP_TIME 1
////#define SLEEP_TIME 1
#define STR_LEN 1025

#define COUNTER_USAGE_ERROR "Usage: counter <character> <filename> <offset> <length>\n"
#define FILE_READ_ERROR "Error reading file: %s\n"
#define FILE_MAP_ERROR "Error mapping file to memory: %s\n"
#define UNMAP_ERROR "Error un-mapping file: %s\n"
#define PIPE_CREATE_ERROR "Error creating named pipe: %s\n"
#define PIPE_OPEN_ERROR "Error opening named pipe: %s\n"
#define PIPE_WRITE_ERROR "Error writing to named pipe: %s\n"
#define PIPE_DEL_ERROR "Error deleting named pipe: %s\n"
#define SIG_SEND_ERROR "Error sending signal: %s\n"


void count(char character, char* filename, off_t offset, off_t length) {
	int fd, pipeFd;
	char* arr;
	char pipeName[STR_LEN];
	off_t mapOffset = (offset / sysconf(_SC_PAGE_SIZE)) * sysconf(_SC_PAGE_SIZE);
	off_t mapLength = offset - mapOffset + length;

	// open file for reading
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf(FILE_READ_ERROR, strerror(errno));
		return;
	}

	// map file
	////printf("%s %lu %lu %d\n", filename, offset, length, fd);
	arr = (char*) mmap(NULL, mapLength, PROT_READ , MAP_PRIVATE, fd, mapOffset);
	if (arr == MAP_FAILED) {
		printf(FILE_MAP_ERROR, strerror(errno));
		close(fd);
		return;
	}

	// count
	off_t counter = 0;
	for (int i=offset - mapOffset; i<mapLength; i++)
		if (arr[i] == character)
			counter++;

	// create pipe
	sprintf(pipeName, PIPE_NAME, getpid());
	if (mkfifo(pipeName, 0777) == -1) {
		printf(PIPE_CREATE_ERROR, strerror(errno));
		goto CLEANUP;
	}

	// send signal to dispatcher
	if (kill(getppid(), SIGUSR1) == -1) {
		printf (SIG_SEND_ERROR, strerror(errno));
		printf("counter_%d: %lu\n",getpid(), offset);
		goto CLEANUP;
	}

	// open pipe
	pipeFd = open(pipeName, O_WRONLY);
	if (pipeFd == -1) {
		printf(PIPE_OPEN_ERROR, strerror(errno));
		goto CLEANUP;
	}

	// write counter to pipe
	if (write(pipeFd, &counter, sizeof(off_t)) == -1)
		printf(PIPE_WRITE_ERROR, strerror(errno));

	sleep(SLEEP_TIME);
	CLEANUP:
	// unmap file
	if (munmap(arr, mapLength) == -1)
		printf(UNMAP_ERROR, strerror(errno));
	close(fd);

	// close and remove pipe
	if (pipeFd >= 0) close(pipeFd);
	if (unlink(pipeName) == -1) // delete pipe
		printf(PIPE_DEL_ERROR, strerror(errno));
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
 * Convert digit string to offset
 */
off_t toOffset(char* str) {
	off_t offset = 0;
	for (int i=0; i<strlen(str); i++)
		offset = offset * 10 + (str[i] - '0');
	return offset;
}

int main(int argc, char* argv[]) {
	// validate arguments
	if (argc < 5 || strlen(argv[1]) != 1 || !isOffset(argv[3]) || !isOffset(argv[4])) {
		printf(COUNTER_USAGE_ERROR);
		return -1;
	}

	//count
	count(argv[1][0], argv[2], toOffset(argv[3]), toOffset(argv[4]));

	return 0;
}
