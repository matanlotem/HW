#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include "vault_aux.h"
#include "vault_consts.h"

int streq(char* str1, char* str2) {
	return (strcmp(str1,str2) == 0);
}

ssize_t parseSize(char* sizeStr) {
    if (strlen(sizeStr) < 2)
        return 0;

    // validate digits and parse integer size
    ssize_t psize = 0;
    for (int i=0; i<strlen(sizeStr)-1; i++)
        if (sizeStr[i] < '0' || sizeStr[i] > '9')
            return 0;
        else
        	psize = psize * 10 + (sizeStr[i] - '0');

    // get units
    switch (sizeStr[strlen(sizeStr)-1]) {
        case 'G':
            psize *= 1024;
        case 'M':
            psize *= 1024;
        case 'K':
            psize *= 1024;
        case 'B':
            break;
        default:
            return 0;
    }

    return psize;
}

int formatSize(char* sizeStr, ssize_t psize) {
	// convert units
	char units[5] = "BKMG";
	int unitNum = 0;
	double tmpSize = psize;
	while (tmpSize > 1024 && unitNum < 3) {
		tmpSize /= 1024;
		unitNum++;
	}

	// round up (ceil)
	psize = (ssize_t) tmpSize;
	if (tmpSize - psize > 0)
		psize++;

	// format
	sprintf(sizeStr,"%d%c",psize, units[unitNum]);
	return 0;
}

void strToLower(char* str) {
	if (str != NULL)
		for (int i=0; i< (int) strlen(str); i++)
			if (str[i] >= 'A' && str[i] <= 'Z')
				str[i] = str[i] - 'A' + 'a';
}

/*
 * copies data from one file descriptor to another through a buffer
 * if fails, returns -1
 */
int copyData (int fromFd, int toFd, ssize_t dataSize) {
	ssize_t writeSize = 0, bufferSize = BUFFER_SIZE, tmpSize;
	char buffer[BUFFER_SIZE];

	while (writeSize < dataSize) {
		// don't read more than you can write
		if (bufferSize > dataSize - writeSize)
			bufferSize = dataSize - writeSize ;

		// read data
		tmpSize = read(fromFd, buffer, bufferSize);
		if (tmpSize != bufferSize) {
			printf(DATA_READ_ERR, strerror(errno));
			return -1;
		}

		// write data
		tmpSize = write(toFd, buffer, bufferSize);
		if (tmpSize != bufferSize) {
			printf(DATA_WRITE_ERR, strerror(errno));
			return -1;
		}

		writeSize += bufferSize;
	}

	return 0;
}
