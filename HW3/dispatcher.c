#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define PIPE_NAME "/tmp/counter_%d"
#define COUNTER_EXE "./counter"
#define MIN_M 1
////#define MIN_M 1024
#define STR_LEN 1025

#define DISPATCHER_USAGE_ERROR "Usage: dispatcher <character> <filename>\n"
#define FILE_NOT_FOUND_ERROR "Error getting file size: %s\n"
#define SIG_REGISTER_ERROR "Error registering signal handle: %s\n"
#define PIPE_OPEN_ERROR "Error opening named pipe: %s\n"
#define PIPE_READ_ERROR "Error reading from named pipe: %s\n"
#define FORK_ERROR "Error creating fork: %s\n"
#define EXECV_ERROR "Error executing counter: %s\n"
#define OUTPUT_MSG "The character '%c' appears %lu times in %s\n"

// counter
off_t totalCounter = 0;

/*
 * handle signals
 */
void counterSignalHandler (int signum, siginfo_t* info, void* ptr) {
	char pipeName[STR_LEN];
	sprintf(pipeName, PIPE_NAME, info->si_pid);

	// open pipe
	int pipeFd = open(pipeName, O_RDONLY);
	if (pipeFd == -1) {
		printf(PIPE_OPEN_ERROR, strerror(errno));
		return;
	}

	// read counter from pipe
	off_t counter;
	if (read(pipeFd, &counter, sizeof(off_t)) != sizeof(off_t)) {
		printf(PIPE_READ_ERROR, strerror(errno));
		return;
	}
	close(pipeFd);

	totalCounter += counter;
}


int dispatchCounter(char character, char* filename, off_t offset, off_t length) {
	// set arguments for counter
	char offsetStr[STR_LEN], lengthStr[STR_LEN];
	char* args[] = {COUNTER_EXE, &character, filename, offsetStr, lengthStr, NULL};
	sprintf(offsetStr, "%lu", (long int) offset);
	sprintf(lengthStr, "%lu", (long int) length);

	// fork
	int f = fork();
	if (f<0) { // fork error
		printf(FORK_ERROR, strerror(errno));
		return -1;
	}
	else if (f==0) { // child process
		execv(args[0],args);
		printf(EXECV_ERROR, strerror(errno));
		return -1;
	}

	return 0;
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
	off_t N = st.st_size;
	off_t M = sqrt(N);

	// register signal handler (code from recitation)
	struct sigaction new_action;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_sigaction = counterSignalHandler;
	new_action.sa_flags = SA_SIGINFO;
	if (0 != sigaction(SIGUSR1, &new_action, NULL)) {
		printf(SIG_REGISTER_ERROR, strerror(errno));
		return -1;
	}

	// for small file run only only counter
	if (M <= MIN_M) {
		//TODO: run counter
		dispatchCounter(argv[1][0], argv[2], 0, N);
	}
	// for large file run ~M counters
	else {
		off_t offset = 0;
		while (offset < N) {
			// prevent overflow of last counter
			if (N-offset < M)
				M = N-offset;

			//TODO: run counter - what happens when fails?
			if (dispatchCounter(argv[1][0], argv[2], offset, M) == -1) {
				break;
			}

			offset += M;
		}
	}

	// wait on counters, ignore syscall interrupts
	int status;
	while (wait(&status) != -1 || errno == EINTR);

	// print output
	printf(OUTPUT_MSG, argv[1][0], totalCounter, argv[2]);

	return 0;
}
