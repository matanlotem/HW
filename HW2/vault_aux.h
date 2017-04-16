#ifndef VAULT_AUX_H_
#define VAULT_AUX_H_

#include <sys/types.h>

/* Check if two strings are equal
 *
 * @param str1 - pointer to first string
 * @param str2 - pointer to second string
 *
 * @return 1 if equal, 0 otherwise
 */
int streq(char* str1, char* str2);

/* Convert string to lowercase in place
 * affects only characters in A-Z
 *
 * @param str - pointer to string to be converted to lowercase
 */
void strToLower(char* str);

/* Parse size string given in a format of integer followed by unit (B/K/M/G)
 *
 * @param sizeStr - string containing formatted size
 *
 * @return size in bytes
 * 		   0 if format invalid (non digit characters / wrong units)
 */
ssize_t parseSize(char* sizeStr);

/* Format size in bytes to integer followed by unit (B/K/M/G)
 *
 * @param sizeStr - return parameter - string containing formatted size
 * @param psize - size in bytes to be formatted
 */
void formatSize(char* sizeStr, ssize_t psize);

/* Copies data from one file descriptor to another through a buffer
 *
 * @param fromFd - file descriptor of file to copy from
 * 				   must be open for read and set to correct offset
 * @param toFd - file descriptor of file to copy to
 * 				 must be open for write and set to correct offset
 * @param dataSize - amount of data to copy in bytes
 *
 * @return 0 for success, -1 for failure
 */
int copyData (int fromFd, int toFd, ssize_t dataSize);


#endif /* VAULT_AUX_H_ */
