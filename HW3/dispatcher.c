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
#define GIVEUP_ERROR "Retried several times, giving up\n"
#define OUTPUT_MSG "The character '%c' appears %lu times in %s\n"

// structure containing counter state and parameters
typedef struct counter_state {
	off_t offset;
	off_t length;
	off_t count;
	int pid;
} counterState;

// state of all counters - in order to account for missing counters
counterState COUNTERS[MAX_NUM_COUNTERS];
int numCounters = 0;

/*
 * Handle counter signal (when a counter is ready to return its result)
 * reads result from named pipe and saves to COUNTERS state.
 */
void counterSignalHandler (int signum, siginfo_t* info, void* ptr) {
	char pipeName[STR_LEN];
	sprintf(pipeName, PIPE_NAME, info->si_pid);
	////printf("Received signal from %d\n",info->si_pid);

	// get index in COUNTERS for specific pid
	int counterID = 0;
	for (counterID = 0; counterID < numCounters; counterID++)
		if (COUNTERS[counterID].pid == info->si_pid) break;

	// open pipe
	int pipeFd = open(pipeName, O_RDONLY);
	if (pipeFd == -1) {
		printf(PIPE_OPEN_ERROR, strerror(errno));
		return;
	}

	// read counter from pipe
	if (read(pipeFd, &(COUNTERS[counterID].count), sizeof(off_t)) != sizeof(off_t)) {
		printf(PIPE_READ_ERROR, strerror(errno));
		return;
	}
	close(pipeFd);

	// set counter state to done
	COUNTERS[counterID].pid = -1;
}


/*
 * Split file into blocks and write to COUNTERS state
 * returns 0 on success, -1 on failure
 */
int prepareCounters(char* filename) {
	// get file size (and check if exists)
	struct stat st;
	if (stat(filename, &st) == -1) {
		printf(FILE_NOT_FOUND_ERROR, strerror(errno));
		return -1;
	}
	off_t N = st.st_size;

	// calculate number of counters to run and number of characters each counter processes
	int pageSize = sysconf(_SC_PAGE_SIZE);
	off_t blockSize, numPages = N/pageSize + (N%pageSize>0);
	if (numPages <= 2) // for small files run one counter
		blockSize = 2 * pageSize;
	else if (numPages < MAX_NUM_COUNTERS) // for medium files run a small number counters
		blockSize = pageSize;
	else // for large files run MAX_NUM_COUNTERS counters
		blockSize = pageSize * (numPages/MAX_NUM_COUNTERS + (numPages%MAX_NUM_COUNTERS>0));

	// prepare counters
	numCounters = 0;
	counterState state;
	for (off_t offset=0; offset<N; offset+=blockSize) {
		COUNTERS[numCounters].pid = 0;
		COUNTERS[numCounters].count = 0;
		COUNTERS[numCounters].offset = offset;
		// prevent overflow of last counter
		if (offset + blockSize > N)
			COUNTERS[numCounters].length = N-offset;
		else
			COUNTERS[numCounters].length = blockSize;
		numCounters++;
	}

	return 0;
}

/*
 * Dispatches a counter on a block of given length and starting offset
 * returns (to dispatcher) counter pid on success, -1 on failure
 */
int dispatchCounter(char character, char* filename, off_t offset, off_t length) {
	// set arguments for counter
	char offsetStr[STR_LEN], lengthStr[STR_LEN];
	char* args[] = {COUNTER_EXE, &character, filename, offsetStr, lengthStr, NULL};
	sprintf(offsetStr, "%lu", offset);
	sprintf(lengthStr, "%lu", length);

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
	else // successful fork - return child pid
		return f;
}

/*
 * Dispatch all counters that are not marked as done, that is have a pid != -1
 * Save pid for later reference (to mark as done)
 * Wait on all counters to finish running
 */
int dispatchWaitingCounters(char character, char* filename) {
	int res = 0;

	// run counters waiting to be run and save pid
	for (int i=0; i<numCounters; i++) {
		if (COUNTERS[i].pid != -1) {
			COUNTERS[i].pid = dispatchCounter(character, filename, COUNTERS[i].offset, COUNTERS[i].length);

			// if dispatch fails, do not run more counters
			if (COUNTERS[i].pid == -1) {
				res = -1;
				break;
			}
		}
	}

	// wait on counters, ignore syscall interrupts
	while (wait(NULL) != -1 || errno == EINTR);

	return res;
}

/*
 * Check if all counters are marked as done, that is pid == -1
 */
int allDone() {
	for (int i=0; i<numCounters; i++)
		if (COUNTERS[i].pid != -1)
			return 0;
	return 1;
}

/*
 * Counts number of occurrences of some character (argv[1]) in a file (argv[2])
 * In case of signal misses tries again several times.
 */
int main (int argc, char** argv) {
	// validate arguments
	if (argc < 3 || strlen(argv[1]) != 1) {
		printf(DISPATCHER_USAGE_ERROR);
		return -1;
	}
	char character = argv[1][0];
	char* filename = argv[2];

	// decide how many counters will be run on on what block size
	if (prepareCounters(filename) == -1)
		return -1;

	// register signal handler (code from recitation)
	struct sigaction new_action;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_sigaction = counterSignalHandler;
	new_action.sa_flags = SA_SIGINFO;
	if (0 != sigaction(SIGUSR1, &new_action, NULL)) {
		printf(SIG_REGISTER_ERROR, strerror(errno));
		return -1;
	}

	// Try dispatching counters. If error occurs during dispatching then quit.
	// If some counter does not return (probably due to signal miss),
	// run again all non returning counters.
	// Retries at most as number of counters - for the case of signal miss we get
	// at least one successful counter per a run.
	int retries = 0;
	while (!allDone() && retries < numCounters) {
		if (dispatchWaitingCounters(character, filename) == -1)
			return -1;; // quit if dispatch fails
		retries++;
	}

	// when done
	if (allDone()) { // success - accumulate and print output
		off_t totalCount = 0;
		for (int i=0; i<numCounters; i++)
			totalCount += COUNTERS[i].count;

		printf(OUTPUT_MSG, character, totalCount, filename);
		return 0;
	}
	else { // gave up on retries
		printf(GIVEUP_ERROR);
		return -1;
	}
}
