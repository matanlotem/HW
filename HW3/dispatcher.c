#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

// dispatcher parameters
#define MAX_NUM_COUNTERS 16
#define NUM_RETRIES 3
#define PIPE_NAME "/tmp/counter_%d"
#define COUNTER_EXE "./counter"
#define STR_LEN 1025

// output messages
#define DISPATCHER_USAGE_ERROR "Usage: dispatcher <character> <filename>\n"
#define FILE_NOT_FOUND_ERROR "Error getting file size: %s\n"
#define SIG_REGISTER_ERROR "Error registering signal handle: %s\n"
#define PIPE_OPEN_ERROR "Error opening named pipe: %s\n"
#define PIPE_READ_ERROR "Error reading from named pipe: %s\n"
#define FORK_ERROR "Error creating fork: %s\n"
#define EXECV_ERROR "Error executing counter: %s\n"
#define MISSED_SIGNAL_ERROR "Missed some signals, trying again\n"
#define MISSED_SIGNAL_GIVEUP_ERROR "Missed some signals, giving up\n"
#define OUTPUT_MSG "The character '%c' appears %lu times in %s\n"

// counter
off_t totalCounter = 0;
int numCounters = 0;
off_t counterOffsets[MAX_NUM_COUNTERS];
off_t counterLengths[MAX_NUM_COUNTERS];
int counterPID[MAX_NUM_COUNTERS];

/*
 * Handle counter signal (when a counter is ready to return its result)
 * reads result from named pipe.
 */
void counterSignalHandler (int signum, siginfo_t* info, void* ptr) {
	numCounters--;

	char pipeName[STR_LEN];
	sprintf(pipeName, PIPE_NAME, info->si_pid);
	////printf("Received signal from %d\n",info->si_pid);

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

/*
 * Dispatches a counter on a block of given length and starting offset
 * returns 0 on success, -1 on failure
 */
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

/*
 * Split file into blocks and dispatch to counters.
 * returns 0 on success
 *        -1 for some general error
 *        -2 if missed some signals
 */
int dispatchAllCounters(char character, char* filename, off_t N) {
	int res = 0;

	// calculate number of counters to run and number of characters each counter processes
	int pageSize = sysconf(_SC_PAGE_SIZE);
	off_t numPages = N/pageSize + (N%pageSize>0), blockSize;
	if (numPages <= 2) // for small files run one counter
		blockSize = 2 * pageSize;
	if (numPages < MAX_NUM_COUNTERS) // for medium files run a small number counters
		blockSize = pageSize;
	else // for large files run MAX_NUM_COUNTERS counters
		blockSize = pageSize * (numPages/MAX_NUM_COUNTERS + (numPages%MAX_NUM_COUNTERS>0));

	// dispatch counters
	numCounters = 0;
	totalCounter = 0;
	for (off_t offset=0; offset<N; offset+=blockSize) {
		 // prevent overflow of last counter
		if (offset + blockSize > N)
			blockSize = N-offset;

		//run counter - if dispatch fails, do not run more counters
		if (dispatchCounter(character, filename, offset, blockSize) == -1) {
			res = -1;
			break;
		}

		numCounters++;
	}

	// wait on counters, ignore syscall interrupts
	while (wait(NULL) != -1 || errno == EINTR);
	// check all counters sent signals
	if (res == 0 && numCounters != 0)
		return -2;

	return res;
}

/*
 * Counts number of occurrences of some character (argv[1]) in a file (argv[2])
 * In case of signal missess tries again several times.
 */
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

	// register signal handler (code from recitation)
	struct sigaction new_action;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_sigaction = counterSignalHandler;
	new_action.sa_flags = SA_SIGINFO;
	if (0 != sigaction(SIGUSR1, &new_action, NULL)) {
		printf(SIG_REGISTER_ERROR, strerror(errno));
		return -1;
	}

	// try dispatching counters - retried several times if some counter failed returning a signal
	for (int i=0; i<NUM_RETRIES; i++) {
		switch (dispatchAllCounters(argv[1][0], argv[2], N)) {
		case 0: // success - print output
			printf(OUTPUT_MSG, argv[1][0], totalCounter, argv[2]);
			return 0;
			break;
		case -1: // some general error - exit
			return -1;
			break;
		case -2: // missed some signals - try again
			if (i<NUM_RETRIES-1)
				printf(MISSED_SIGNAL_ERROR);
			else {
				printf(MISSED_SIGNAL_GIVEUP_ERROR);
				return -1;
			}
			break;
		}
	}

	return 0;
}
