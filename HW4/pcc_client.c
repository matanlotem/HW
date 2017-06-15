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

#define USAGE_ERROR "Number of bytes to transfer not provided\n"
#define SOCKET_CREATE_ERROR "Error creating socket\n"
#define CONNECT_ERROR "Connect Failed: %s\n"
#define INPUT_OPEN_ERROR "Error opening input file: %s\n"
#define INPUT_READ_ERROR "Error reading from input file: %s\n"
#define SOCKET_READ_ERROR "Error reading from socket: %s\n"
#define SOCKET_WRITE_ERROR "Error writing to socket: %s\n"
#define OUTPUT_MSG "%lld printable characters out of %lld total characters\n"


/*
 * Opens file (by filename) for reading
 * Returns file descriptor on success, -1 on failure.
 */
int openFile(const char* filename) {
	int inputfd = open(filename, O_RDONLY);
	if (inputfd < 0) {
		printf(INPUT_OPEN_ERROR, strerror(errno));
		return -1;
	}
	return inputfd;
}


/*
 * Connects to server at given address and port
 * Returns socket file descriptor on success, -1 on failure
 */
int connectToServer(const char* address, int port) {
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
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = inet_addr(address);

	// connect
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		printf(CONNECT_ERROR, strerror(errno));
		return -1;
	}

	return sockfd;
}


/*
 * Reads len bytes from input file, sends them to server, and
 * receives from server the number of printable bytes sent.
 * Uses the following protocol:
 *   SERVER -> CLIENT: 'HI'				lets client know server is ready
 *   CLIENT -> SERVER: len				so server knows how much to read
 *   SERVER -> CLIENT: 'THANKS'			prevents data overflowing into len
 *   CLIENT -> SERVER: data 			in chunks
 *   SERVER -> CLIENT: num printable
 * Returns number of printable bytes on success, -1 on failure
 */
long long transaction(long long len, int inputfd, int sockfd) {
	char buffer[BUFFER_SIZE];

	// wait for server to say HI
	if (read(sockfd, buffer, BUFFER_SIZE) < 0) {
		printf(SOCKET_READ_ERROR, strerror(errno));
		return -1;
	}

	// send data length to server
	sprintf(buffer,"%lld ", len);
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
	int buffer_size = BUFFER_SIZE;
	long long data_sent = 0;
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

	// get number of characters from server
	if (read(sockfd, buffer, BUFFER_SIZE) < 0) {
		printf(SOCKET_READ_ERROR, strerror(errno));
		return -1;
	}

	return atoll(buffer);
}


/*
 * 1. Gets number of characters to send to server as input
 * 2. Reads characters from /dev/urandom and sends them to server
 * 3. Receives number of printable characters from server and prints it
 */
int main (int argc, char* argv[]) {
	if (argc < 2) {
		printf(USAGE_ERROR);
		return -1;
	}
	long long len = atoll(argv[1]);

	// open connection and file
	int sockfd = connectToServer(SERVER_ADDRESS, SERVER_PORT);
	int inputfd = openFile(INPUT_FILE);

	// communicate with server and print output on success
	if (sockfd != -1 && inputfd != -1) {
		long long count = transaction (len, inputfd, sockfd);
		if (count >= 0) printf(OUTPUT_MSG, count, len);
	}

	// close all
	close(sockfd);
	close(inputfd);
	return 0;

}
