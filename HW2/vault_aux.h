#ifndef VAULT_AUX_H_
#define VAULT_AUX_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int streq(char* str1, char* str2);

ssize_t parseSize(char* sizeStr);
int formatSize(char* sizeStr, ssize_t psize);

int parseCmnd(char* cmndArg, char* cmnd, ssize_t cmndSize);
int copyData (int fromFd, int toFd, ssize_t dataSize);


#endif /* VAULT_AUX_H_ */
