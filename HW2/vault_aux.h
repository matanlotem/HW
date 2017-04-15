#ifndef VAULT_AUX_H_
#define VAULT_AUX_H_

#include <sys/types.h>

int streq(char* str1, char* str2);

ssize_t parseSize(char* sizeStr);

int formatSize(char* sizeStr, ssize_t psize);

void strToLower(char* str);

int copyData (int fromFd, int toFd, ssize_t dataSize);


#endif /* VAULT_AUX_H_ */
