#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 2233
#define BUFFER_SIZE 1024
#define NUM_LISTENERS 10
#define NUM_CHARS 128

// output messages
#define SIG_REGISTER_ERROR "Error registering signal handle: %s\n"
#define SOCKET_CREATE_ERROR "Error creating socket\n"
#define SOCKET_REUSE_ERROR "Error setting socket reuse\n"
#define SOCKET_READ_ERROR "Error reading from socket: %s\n"
#define SOCKET_WRITE_ERROR "Error writing to socket: %s\n"
#define BIND_ERROR "Bind failed: %s\n"
#define LISTEN_ERROR "Listen failed: %s\n"
#define ACCEPT_ERROR "Accept failed: %s\n"
#define THREAD_CREATE_ERROR "Thread creation failed\n"
#define SERVER_UP_MSG "SERVER IS UP\n"
#define SERVER_DOWN_MSG "SERVER IS DOWN, Waiting for %d threads to finish\n"
#define STATS_COUNTER_MSG "Total bytes read: %lld\n"
#define STATS_HEADER_MSG "CHAR\tSTATS\n"
#define STATS_DATA_MSG "%c\t%lld\n"

// global variables
pthread_mutex_t lock;
pthread_cond_t counter_cv;
long long global_stats[NUM_CHARS] = {0};
long long global_bytes_read = 0;
int thread_counter = 0;


int setupListener() {
	// create socket
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		printf(SOCKET_CREATE_ERROR);
		return -1;
	}
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int) ) < 0) {
		printf(SOCKET_REUSE_ERROR);
		close(listenfd);
		return -1;
	}

	// set connection parameters
	struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind
	if(bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) != 0) {
		printf(BIND_ERROR, strerror(errno));
		close(listenfd);
		return -1;
	}

	// listen
	if(listen(listenfd, NUM_LISTENERS) != 0) {
		printf(LISTEN_ERROR, strerror(errno));
		close(listenfd);
		return -1;
	}

	return listenfd;
}


int processConnection(int connfd, long long *stats) {
	char buffer[BUFFER_SIZE];
	long long len, bytes_read, total_bytes = 0, printable_bytes = 0;

	// send HI to client to say server is ready
	sprintf(buffer,"HI");
	if (write(connfd, buffer, strlen(buffer)) < 0) {
		printf(SOCKET_WRITE_ERROR, strerror(errno));
		return -1;
	}

	// get length from client
	if (read(connfd, buffer, BUFFER_SIZE) > 0)
		len = atoi(buffer);
	else {
		printf(SOCKET_READ_ERROR, strerror(errno));
		return -1;
	}

	// send THANKS to client to say server is ready for data
	sprintf(buffer,"THANKS");
	if (write(connfd, buffer, strlen(buffer)) < 0) {
		printf(SOCKET_WRITE_ERROR, strerror(errno));
		return -1;
	}

	// read data from client and count printable bytes
	while (total_bytes < len) {
		bytes_read = read(connfd, buffer, BUFFER_SIZE);
		if (bytes_read <= 0) {
			printf(SOCKET_READ_ERROR, strerror(errno));
			return -1;
		}
		for (int i=0; i < bytes_read; i++) {
			if (isprint(buffer[i])) {
				printable_bytes++;
				stats[(int) buffer[i]]++;
			}
		}
		total_bytes += bytes_read;
	}

	// write number of printable bytes to client
	sprintf(buffer,"%lld ",printable_bytes);
	write(connfd,buffer,strlen(buffer));
	return total_bytes;
}


void* handleClient(void *connfd_ptr) {
	int connfd = *((int*) connfd_ptr);
	long long stats[NUM_CHARS] = {0};

	// raise thread counter
	pthread_mutex_lock(&lock);
	thread_counter++;
	pthread_mutex_unlock(&lock);

	// process connection
	int bytes_read = processConnection(connfd, stats);
	close(connfd);

	// update global stats
	if (bytes_read >= 0) {
		pthread_mutex_lock(&lock);
		for (int i=0; i<NUM_CHARS; i++)
			global_stats[i] += stats[i];
		global_bytes_read += bytes_read;
		pthread_mutex_unlock(&lock);
	}

	// lower thread counter and signal thread is done
	pthread_mutex_lock(&lock);
	thread_counter--;
	pthread_cond_signal(&counter_cv);
	pthread_mutex_unlock(&lock);
	pthread_exit(NULL);
}


// does nothing - signal only breaks loop
void signalHandler() {}


int main (int argc, char* argv[]) {
	// register signal handler
	struct sigaction new_action;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_sigaction = signalHandler;
	new_action.sa_flags = SA_SIGINFO;
	if (0 != sigaction(SIGINT, &new_action, NULL)) {
		printf(SIG_REGISTER_ERROR, strerror(errno));
		return -1;
	}

	// setup listener
	int listenfd = setupListener();
	if (listenfd == -1)
		return -1;
	printf(SERVER_UP_MSG);

	// setup locks
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init (&counter_cv, NULL);

	// accept loop - breaks on SIGINT or error
	pthread_t thread;
	while (1) {
		// accept
		int connfd = accept(listenfd, NULL, NULL);
		if(connfd < 0) {
			if (errno != 4) // ignore SIGINT
				printf(ACCEPT_ERROR, strerror(errno));
			break;
		}
		// create thread
		if (pthread_create(&thread, NULL, handleClient, (void *) &connfd)) {
			printf(THREAD_CREATE_ERROR);
			break;
		}
	}
	close(listenfd);

	// wait for all threads to terminate and prints output stats
	pthread_mutex_lock(&lock);
	printf (SERVER_DOWN_MSG, thread_counter);
	while (thread_counter > 0) // wait
		pthread_cond_wait(&counter_cv, &lock);
	// print stats
	printf(STATS_COUNTER_MSG, global_bytes_read);
	printf(STATS_HEADER_MSG);
	for (int i=0; i < NUM_CHARS; i++)
		if (isprint(i)) printf(STATS_DATA_MSG, i, global_stats[i]);
	pthread_mutex_unlock(&lock);

	// destroy locks
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&counter_cv);

	return 0;
}
