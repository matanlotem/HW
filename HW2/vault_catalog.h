#ifndef VAULT_CATALOG_H_
#define VAULT_CATALOG_H_

#include <sys/types.h>
#include "vault_consts.h"

typedef struct fat_entry_t FATEntry;
typedef struct vault_block_t VaultBlock;
typedef struct catalog_t* Catalog;

struct fat_entry_t {
	char fileName[MAX_VAULT_FNAME + 1];
	ssize_t fileSize;
	mode_t filePerm;
	time_t insertionTime;
	short blockId[VAULT_BLOCK_NUM];
};

struct vault_block_t {
	short fatEntryId;
	short blockNum;
	ssize_t blockSize;
	off_t blockOffset;
};


struct catalog_t {
	ssize_t vaultSize;
	time_t creationTime;
	time_t modificationTime;
	short numFiles;
	short numBlocks;
	FATEntry fat[MAX_VAULT_FILES];
	VaultBlock blocks[MAX_VAULT_FILES*VAULT_BLOCK_NUM];
};

/* Initialize vault - creates a new vault of specified size.
 *
 * @param vaultFileName - path to create vault file
 * @param vaultSize - request vault file size in bytes
 * 		fails if size is too small to contain vault meta-data.
 *
 * @return 0 for success, -1 for failure
 */
int initVault(char* vaultFileName, ssize_t vaultSize);

/* Opens vault for for read-write and loads meta-data.
 *
 * @param vaultFileName - path of vault file
 * @param vaultFd - return parameter - pointer to file descriptor of vault file
 * 					on success is open for read/write
 *
 * @return loaded vault meta-data (catalog) on success
 * 		   NULL on failure
 */
Catalog openVault(char* vaultFileName, int *vaultFd);

/* Closes vault at end of invocation.
 * Updates meta-data in vault file and closes file.
 *
 * @param vaultFd - file descriptor of vault file
 * @param catalog - vault meta-data
 * @param updateCatalog - 1 to flush catalog to vault file,
 * 						  0 to leave vault file catalog untouched.
 *
 * @return 0 for success, -1 for failure
 */
int closeVault(int vaultFd, Catalog catalog, int updateCatalog);

/* Outputs a list of the files in the vault:
 * size, permissions and insertion date
 *
 * @param catalog - vault meta-data
 *
 * @return 0 for success, -1 for failure
 */
int listVault(Catalog catalog);

/* Outputs vault status:
 *   - number of files in vault
 *   - total size of all files in vault (including delimiters)
 *   - fragmentation ratio - (total size) / (consumed size) where
 *     "consumed size" is the distance between the start of the
 *     first file to the end of the last file in the vault.
 *
 * @param catalog - vault meta-data
 *
 * @return 0 for success, -1 for failure
 */
int getVaultStatus(Catalog catalog);

/* Get the index of the fat entry of a file by name
 *
 * @param fileName - name of file to search for in fat
 * @param catalog - vault meta-data
 *
 * @return - fat entry index in fat if file name in fat
 * 			 -1 if file name not in fat
 */
int getFATEntryId(char* fileName, Catalog catalog);


/*** DEBUG PRINT METHODS ***/

/* Print attributes of a specific vault block:
 * 	 - fat entry id + block number of fat entry (fragment number)
 * 	 - block size
 * 	 - block offset in vault file
 *
 * @param vaultBlock - the block at interest
 */
void printVaultBlock(VaultBlock vaultBlock);

/* Print attributes of a specific fat entry:
 * 	 - file name
 * 	 - file size
 * 	 - file permissions
 *
 * @param fatEntry - the fat entry at interest
 */
void printFATEntry(FATEntry fatEntry);

/* Print list of all vault blocks using printVaultBlock.
 * Print first and last allowed file offsets (just after catalog and end of vault file)
 *
 * @param catalog - vault meta-data
 */
void printBlocks(Catalog catalog);

/* Print list of all vault files (fat entries) using printFATEntry
 *
 * @param catalog - vault meta-data
 */
void printFAT(Catalog catalog);

#endif /* VAULT_CATALOG_H_ */
