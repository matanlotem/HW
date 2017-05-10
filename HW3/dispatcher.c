#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stats.h>
#include <errno.h>

#define DISPATCHER_USAGE_ERROR "Usage: dispatcher <character> <filename>\n"
#define FILE_NOT_FOUND_ERROR "Could not get file size: %s\n"
#define PIPE_OPEN_ERROR "Error opening named pipe: %s\n"
#define PIPE_READ_ERROR "Error reading from named pipe: %s\n"
#define OUTPUT_MSG "The character '%c' appears %d times in %s\n"

#define MIN_M 1024
#define STR_LEN 1025
#define PIPE_NAME "/tmp/counter_%d"

off_t getPipeCounter(pid_t pid) {
	char pipeName[STR_LEN];
	sprintf(pipeName, PIPE_NAME, pid);

	// open pipe
	int pipeFd = open(pipeName, O_RDONLY);
	if (pipeFd == -1) {
		printf(PIPE_OPEN_ERROR, strerror(errno));
		return -1;
	}

	// read counter from pipe
	off_t counter;
	if (read(pipeFd, &counter, sizeof(off_t)) != sizeof(off_t)) {
		printf(PIPE_READ_ERROR, strerror(errno));
		counter = -1;
	}

	close(pipeFd);
	return counter;
}

int main (int argc, char** argv) {
	// validate arguments
	if (argc < 3 || strlen(argv[1]) != 1) {
		printf(DISPATCHER_USAGE_ERROR);
		return -1;
	}

	// get file size (and check if exists)
	struct stat st;
	if (stat(argv[2], &st) == -1) {
		printf(FILE_NOT_FOUND_ERROR, strerror(errno));
		return -1;
	}
	off_t N = st.st_size();
	off_t M = sqrt(N);

	// for small file run only only counter
	if (M <= MIN_M) {
		//TODO: run counter
	}
	// for large file run ~M counters
	else {
		off_t startOffset = 0;
		while (startOffset < N) {
			// prevent overflow of last counter
			if (N-startOffset < M)
				M = N-startOffset;

			//TODO: run counter

			startOffset += M;
		}
	}

	off_t counter = 0;
	//TODO: wait on counters and signals and sum pipe counters
	//counter += getPipeCounter(pipePid);

	printf(OUTPUT_MSG, argv[1], counter, argv[2]);

}
