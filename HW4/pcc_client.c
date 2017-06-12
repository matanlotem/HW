#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER_PORT 2233
#define SERVER_ADDRESS "127.0.0.1"
#define INPUT_FILE "/dev/urandom"
#define BUFFER_SIZE 1024

#define SOCKET_CREATE_ERROR "Error creating socket\n"
#define CONNECT_ERROR "Connect Failed: %s\n"
#define INPUT_OPEN_ERROR "Error opening input file: %s\n"
#define INPUT_READ_ERROR "Error reading from input file: %s\n"
#define SOCKET_READ_ERROR "Error reading from socket: %s\n"
#define SOCKET_WRITE_ERROR "Error writing to socket: %s\n"
#define OUTPUT_MSG "%d printable characters out of %d total characters\n"


int openFile() {
	int inputfd = open(INPUT_FILE, O_RDONLY);
	if (inputfd < 0) {
		printf(INPUT_OPEN_ERROR, strerror(errno));
		return -1;
	}
	return inputfd;
}


int connectToServer() {
	// create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		printf(SOCKET_CREATE_ERROR);
		return -1;
	}

	// set connection parameters
	struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

	// connect
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		printf(CONNECT_ERROR, strerror(errno));
		return -1;
	}

	return sockfd;
}

int readAndSend(int len, int inputfd, int sockfd) {
	char buffer[BUFFER_SIZE];
	int buffer_size = BUFFER_SIZE;
	int data_sent = 0;

	while (data_sent < len) {
		// do not overflow
		if (buffer_size > len - data_sent)
			buffer_size = len - data_sent;

		// read from input file
		if (read(inputfd, buffer, buffer_size) < 0) {
			printf(INPUT_READ_ERROR, strerror(errno));
			return -1;
		}

		// write to socket
		if (write(sockfd, buffer, buffer_size) < 0) {
			printf(SOCKET_WRITE_ERROR, strerror(errno));
			return -1;
		}
		data_sent += buffer_size;
	}
	return 0;
}


int transaction(int len, int inputfd, int sockfd) {
	char buffer[BUFFER_SIZE];

	// wait for server to say HI
	if (read(sockfd, buffer, BUFFER_SIZE) < 0) {
		printf(SOCKET_READ_ERROR, strerror(errno));
		return -1;
	}

	// send data length to server
	sprintf(buffer,"%d ", len);
	if (write(sockfd, buffer, strlen(buffer)) < 0) {
		printf(SOCKET_WRITE_ERROR, strerror(errno));
		return -1;
	}

	// wait for server to say THANKS
	if (read(sockfd, buffer, BUFFER_SIZE) < 0) {
		printf(SOCKET_READ_ERROR, strerror(errno));
		return -1;
	}

	// read data from file and send to server
	if (readAndSend(len, inputfd, sockfd) < 0)
		return -1;

	// get number of characters from server
	if (read(sockfd, buffer, BUFFER_SIZE) < 0) {
		printf(SOCKET_READ_ERROR, strerror(errno));
		return -1;
	}

	return atoi(buffer);
}

int main (int argc, char* argv[]) {
	if (argc < 2) {
		printf("TODO");
		return -1;
	}
	int len = atoi(argv[1]);

	// open connection and file
	int sockfd = connectToServer();
	int inputfd = openFile();

	// communicate with server and print output on success
	if (sockfd != -1 && inputfd != -1) {
		int count = transaction (len, inputfd, sockfd);
		if (count >= 0) printf(OUTPUT_MSG, count, len);
	}

	// close all
	close(sockfd);
	close(inputfd);
	return 0;

}
