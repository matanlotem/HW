#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 2233
#define BUFFER_SIZE 1024
#define NUM_LISTENERS 10
#define NUM_CHARS 128

#define SOCKET_CREATE_ERROR "Error creating socket\n"
#define SOCKET_REUSE_ERROR "Error setting socket reuse\n"
#define SOCKET_READ_ERROR "Error reading from socket: %s\n"
#define SOCKET_WRITE_ERROR "Error writing to socket: %s\n"
#define BIND_ERROR "Bind failed: %s\n"
#define LISTEN_ERROR "Listen failed: %s\n"
#define ACCEPT_ERROR "Accept failed: %s\n"


pthread_mutex_t lock;
int global_stats[NUM_CHARS] = {0};
int thread_counter = 0;
int done = 0;

void printConnectionDetails(int connfd) {
	struct sockaddr_in my_addr, peer_addr;
	socklen_t addrsize = sizeof(struct sockaddr_in);
	getsockname(connfd, (struct sockaddr*) &my_addr,   &addrsize);
	getpeername(connfd, (struct sockaddr*) &peer_addr, &addrsize);
	printf("Server: Client connected.\n"
		   "\t\tClient IP: %s Client Port: %d\n"
		   "\t\tServer IP: %s Server Port: %d\n",
		   inet_ntoa( peer_addr.sin_addr ),
		   ntohs(     peer_addr.sin_port ),
		   inet_ntoa( my_addr.sin_addr   ),
		   ntohs(     my_addr.sin_port   ) );

}


int setupListener() {
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


int processConnection(int connfd, int *stats) {
	char buffer[BUFFER_SIZE];
	int len, bytes_read, total_bytes = 0, printable_bytes = 0;

	// send HI to client to say server is ready
	sprintf(buffer,"HI");
	if (write(connfd, buffer, strlen(buffer)) < 0) {
		printf(SOCKET_WRITE_ERROR, strerror(errno));
		return -1;
	}

	// get length from client and send ack
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
		for (int i=0; i < bytes_read; i++) {
			if (isprint(buffer[i])) {
				printable_bytes++;
				stats[buffer[i]]++;
			}
		}
		total_bytes += bytes_read;
	}

	// write number of printable bytes to client
	sprintf(buffer,"%d ",printable_bytes);
	write(connfd,buffer,strlen(buffer));
	return 0;
}


void* handleClient(void *connfd_ptr) {
	int connfd = *((int*) connfd_ptr);
	int stats[NUM_CHARS] = {0};

	// raise thread counter
	pthread_mutex_lock(&lock);
	thread_counter++;
	pthread_mutex_unlock(&lock);

	// process connection
	printConnectionDetails(connfd);
	int res = processConnection(connfd, stats);
	close(connfd);

	// update global stats
	if (res != -1) {
		pthread_mutex_lock(&lock);
		for (int i=0; i<NUM_CHARS; i++)
			global_stats[i] += stats[i];
		pthread_mutex_unlock(&lock);
	}

	// lower thread counter
	pthread_mutex_lock(&lock);
	thread_counter--;
	pthread_mutex_unlock(&lock);

	pthread_exit(NULL);
}

int main (int argc, char* argv[]) {
	pthread_mutex_init(&lock, NULL);

	int listenfd = setupListener(), connfd, rc;
	pthread_t thread;

	if (listenfd != -1) {
		while (!done) {
			connfd = accept(listenfd, NULL, NULL);
			if(connfd < 0) {
				printf(ACCEPT_ERROR, strerror(errno));
				return -1;
			}
			else {
				rc = pthread_create(&thread, NULL, handleClient, (void *) &connfd);
			}
		}
	}

	close(listenfd);

	return 0;
}
