#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#include "vault_files.h"
#include "vault_catalog.h"
#include "vault_consts.h"
#include "vault_aux.h"

/* Writes data to vault file at given offset
 * Assumes data is a NULL terminated string
 *
 * @param data - a NULL terminated string to write to vault file
 * @param offset - offset to write data
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 *
 * @return 0 for success, -1 for failure
 */
int writeAtOffset(char* data, off_t offset, int vaultFd) {
	if (lseek(vaultFd, offset, SEEK_SET) == -1) {
		printf(VAULT_SEEK_ERR, strerror(errno));
		return -1;
	}
	if (write(vaultFd, data, strlen(data)) != strlen(data)) {
		printf(VAULT_FWRITE_ERR, strerror(errno));
		return -1;
	}
	return 0;
}

/* Add delimiters to block
 * If block size is smaller than delimiters then fails
 *
 * @param vaultBlock - block meta-data (size and offset)
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 *
 * @return 0 for success, -1 for failure
 */
int addDelim(VaultBlock vaultBlock, int vaultFd) {
	// validate block is not too small for both delimiters
	if (vaultBlock.blockSize < strlen(DELIM_START) + strlen(DELIM_END)) {
		printf(SHORT_BLOCK_ERR);
		return -1;
	}

	if ((writeAtOffset(DELIM_START, vaultBlock.blockOffset, vaultFd) == -1) ||
		(writeAtOffset(DELIM_END, vaultBlock.blockOffset +
				vaultBlock.blockSize - strlen(DELIM_END), vaultFd) == -1)) {
		printf(ADD_DELIM_ERR);
		return -1;
	}
	return 0;
}

/* Overwrite delimiters of block with wipe characters
 * If block size is smaller than delimiter, does not wipe (to prevent overflows)
 * and returns success.
 *
 * @param vaultBlock - block meta-data (size and offset)
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 *
 * @return 0 for success, -1 for failure
 */
int wipeDelim(VaultBlock vaultBlock, int vaultFd) {
	// validate block is not too small - prevent overflows
	if (vaultBlock.blockSize < strlen(DELIM_WIPE))
		return 0;

	// wipe delimiters
	if ((writeAtOffset(DELIM_WIPE, vaultBlock.blockOffset, vaultFd) == -1) ||
		(writeAtOffset(DELIM_WIPE, vaultBlock.blockOffset +
				vaultBlock.blockSize - strlen(DELIM_WIPE), vaultFd) == -1)) {
		printf(WIPE_DELIM_ERR, strerror(errno));
		return -1;
	}

	return 0;
}

/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** **********     ADD    ********** ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */

/* Find best gap to fit block in according to the following heuristic:
 *   - smallest gap that fits all data if exists
 *   - otherwise largest gap
 *
 * @param newBlock - return parameter - will hold the parameters of the
 * 					 best fitting gap: gap size, gap offset
 * @param writeSize - size of data to be fitted into block including delimiters
 * @param catalog - vault meta-data
 *
 * @return index of block with best gap before it
 */
short findGap(VaultBlock *newBlock, ssize_t writeSize, Catalog catalog) {
	short gapBlockId = -1;

	ssize_t gapSize = 0;
	off_t prevEndOffset = sizeof(*catalog); // first gap - from just after catalog

	for (int blockId=0; blockId <= catalog->numBlocks; blockId++) {
		// calculate current gap
		if (blockId < catalog->numBlocks)
			gapSize = catalog->blocks[blockId].blockOffset - prevEndOffset;
		else // last gap from last block to end of vault
			gapSize = catalog->vaultSize - prevEndOffset;

		// check if current gap is better
		if ((newBlock->blockSize < writeSize && gapSize > newBlock->blockSize) ||
			(newBlock->blockSize > writeSize && gapSize < newBlock->blockSize && gapSize >= writeSize)) {
			gapBlockId = blockId;
			newBlock->blockSize = gapSize;
			newBlock->blockOffset = prevEndOffset;
		}

		prevEndOffset = catalog->blocks[blockId].blockOffset + catalog->blocks[blockId].blockSize;
	}

	return gapBlockId;
}

/* Write block meta-data to catalog (without inserting data into vault).
 * Shifts blocks in catalog array to make room for block according to index
 * (blocks are sorted by offset)
 *
 * @param newBlock - block meta-data (with size according to gap to be fitted in)
 * @param gapBlockId - index of block in catalog array (according offset)
 * @param writeSize - actual size of block data including delimiters
 * @param catalog - vault meta-data
 */
void logBlockToGap(VaultBlock newBlock, short gapBlockId, ssize_t writeSize, Catalog catalog) {
	if (writeSize < newBlock.blockSize)
		newBlock.blockSize = writeSize;

	// shift succeeding blocks right
	for (int i=catalog->numBlocks; i > gapBlockId; i--) {
		catalog->blocks[i] = catalog->blocks[i-1];
		catalog->fat[catalog->blocks[i].fatEntryId].blockId[catalog->blocks[i].blockNum] = i;
	}
	// write block to blocks and fat
	catalog->blocks[gapBlockId] = newBlock;
	catalog->fat[newBlock.fatEntryId].blockId[newBlock.blockNum] = gapBlockId;
	catalog->numBlocks++;
}

/* Read block data from file and write it to vault.
 * Add delimiters at start and end of block.
 *
 * @param newBlock - block meta-data
 * @param fileId - file descriptor of file to read block from - must be open for read
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 *
 * @return 0 for success, -1 for failure
 */
int writeBlock(VaultBlock newBlock, int vaultFd, int fileFd) {
	// validate block is large enough for both delimiters
	if (strlen(DELIM_START) + strlen(DELIM_END) > newBlock.blockSize) {
		printf(SHORT_BLOCK_ERR);
		return -1;
	}

	// set position to start of block (+start delimiter
	if (lseek(vaultFd, newBlock.blockOffset + strlen(DELIM_START), SEEK_SET) == -1) {
		printf(VAULT_SEEK_ERR, strerror(errno));
		return -1;
	}

	ssize_t tmpSize;
	// copy file to block
	if (copyData(fileFd, vaultFd, newBlock.blockSize - strlen(DELIM_START) - strlen(DELIM_END)) == -1) {
		printf(ADD_BLOCK_COPY_ERR);
		return -1;
	}

	// write delimiters
	return addDelim(newBlock, vaultFd);
}

/* Add file to vault */
int addVaultFile(char* filePath, int vaultFd, Catalog catalog, int* updateCatalog, char* msg) {
	// set for rollback
	*updateCatalog = 0;

	// check max files
	if (catalog->numFiles == MAX_VAULT_FILES) {
		printf(MAX_FILE_ERR);
		return -1;
	}

	// get filename
	char *fileName = strrchr(filePath,'/');
	if (fileName == NULL) fileName = filePath;
	else fileName++;

	// check if file with same name already exists in vault
	if (getFATEntryId(fileName, catalog) >= 0) {
		printf(SAME_FNAME_ERR);
		return -1;
	}

	// get file stats
	struct stat fileStats;
	if (stat(filePath,&fileStats) == -1) {
		printf(FILE_STATS_ERR, strerror(errno));
		return -1;
	}

	// find location to fit file alphabetically and shift entries to make room
	short fatEntryId = 0;
	while (fatEntryId < catalog->numFiles &&
			strcmp(fileName, catalog->fat[fatEntryId].fileName) > 0)
		fatEntryId++;
	for (int i=catalog->numFiles; i > fatEntryId; i--) {
		catalog->fat[i] = catalog->fat[i-1]; // shift entries right
		// fix blocks->fat pointers
		for (int j=0; j<VAULT_BLOCK_NUM; j++)
			if (catalog->fat[i].blockId[j] != -1)
				catalog->blocks[catalog->fat[i].blockId[j]].fatEntryId = i;
	}
	catalog->numFiles++;

	// add file to fat
	FATEntry *fatEntry = &(catalog->fat[fatEntryId]);
	strcpy(fatEntry->fileName,fileName);
	fatEntry->filePerm = fileStats.st_mode;
	fatEntry->fileSize = fileStats.st_size;
	for (int j=0; j<VAULT_BLOCK_NUM; j++)
		fatEntry->blockId[j] = -1;
	struct timeval insertionTime;
	gettimeofday(&insertionTime,NULL);
	fatEntry->insertionTime = insertionTime.tv_sec;
	catalog->modificationTime = insertionTime.tv_sec;


	// add blocks
	ssize_t writeSize = fatEntry->fileSize;
	ssize_t delimPadding = strlen(DELIM_START) + strlen(DELIM_END);
	short blockNum = 0, gapBlockId =-1;
	while (writeSize > 0 && blockNum < VAULT_BLOCK_NUM) {
		VaultBlock newBlock = {fatEntryId, blockNum, 0, 0};
		gapBlockId = findGap(&newBlock, writeSize + delimPadding, catalog);

		// log block to gap
		if (newBlock.blockSize > delimPadding && gapBlockId != -1) {
			logBlockToGap(newBlock, gapBlockId, writeSize + delimPadding, catalog);
			writeSize -= (newBlock.blockSize - delimPadding);
			blockNum++;
		}
		else // no gap found
			blockNum = VAULT_BLOCK_NUM;
	}

	// file does not fit in vault
	if (writeSize > 0) {
		printf(CANNOT_FIT_ERR);
		return -1;
	}

	// write data to blocks;
	// open file for read
	int fileFd = -1;
	fileFd = open(filePath, O_RDONLY);
	if (fileFd == -1) {
		printf(FILE_OPEN_ERR, strerror(errno));
		return -1;
	}
	// write data
	blockNum = 0;
	while (blockNum < VAULT_BLOCK_NUM) {
		if (fatEntry->blockId[blockNum] != -1) { // check block is not empty
			VaultBlock newBlock = catalog->blocks[fatEntry->blockId[blockNum]];
			if (writeBlock(newBlock, vaultFd, fileFd) == -1) {
				// error writing block
				close(fileFd);
				// wipe blocks to prevent data corruption
				for (int i=blockNum; i>=0; i--) {
					newBlock = catalog->blocks[fatEntry->blockId[i]];
					if (wipeDelim(newBlock, vaultFd) == -1)
						// failed wiping some of the blocks
						printf(DATA_CORRUPTION_ERR);
				}
				return -1;
			}
		blockNum++;
		}
		// go to end if reached empty block
		else blockNum = VAULT_BLOCK_NUM;
	}

	close(fileFd);
	*updateCatalog = 1;
	sprintf(msg,ADD_SUCCESS_MSG, fileName);
	return 0;
}

/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** **********   REMOVE   ********** ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */

/* Remove block from vault - lazy remove - wipes delimiters and updates catalog.
 * Shifts blocks in catalog to leave no gaps in array (only in catalog, not in vault)
 *
 * @param blockId - index of block to be removed.
 * 				    if -1 then block not in use so does nothing and returns success
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 * @param catalog - vault meta-data
 *
 * @return 0 for success, -1 for failure
 */
int rmBlock(short blockId, int vaultFd, Catalog catalog) {
	if (blockId == -1) // block not used
		return 0;
	VaultBlock vaultBlock = catalog->blocks[blockId];

	// update catalog
	for (int i=blockId; i<catalog->numBlocks-1; i++) { // shift blocks left
		catalog->blocks[i] = catalog->blocks[i+1];
		catalog->fat[catalog->blocks[i].fatEntryId].blockId[catalog->blocks[i].blockNum] = i;
	}
	// delete last block
	catalog->fat[vaultBlock.fatEntryId].blockId[vaultBlock.blockNum] = -1;
	catalog->blocks[catalog->numBlocks-1].fatEntryId = -1;
	catalog->numBlocks --;

	// wipe delimiters
	return wipeDelim(vaultBlock, vaultFd);
}

/* Remove file from vault by file name (if file in vault) */
int rmVaultFile(char* fileName, int vaultFd, Catalog catalog, int* updateCatalog, char* msg) {
	// set for rollback
	*updateCatalog = 0;

	// check if exists
	int fatEntryId = getFATEntryId(fileName, catalog);
	if (fatEntryId < 0) {
		printf(MISSING_FNAME_ERR);
		return -1;
	}

	// delete blocks
	for (int blockNum = 0; blockNum < VAULT_BLOCK_NUM; blockNum++) {
		if(rmBlock(catalog->fat[fatEntryId].blockId[blockNum], vaultFd, catalog) == -1) {
			// failed removing block - cannot be fixed
			printf(DATA_CORRUPTION_ERR);
			return -1;
		}
	}

	// delete fat entry
	for (int i=fatEntryId; i< catalog->numFiles-1; i++) {
		catalog->fat[i] = catalog->fat[i+1]; // shift entries left
		// fix blocks->fat pointers
		for (int j=0; j<VAULT_BLOCK_NUM; j++)
			if (catalog->fat[i].blockId[j] != -1)
				catalog->blocks[catalog->fat[i].blockId[j]].fatEntryId = i;
	}
	// nullify last entry
	for (int j=0; j<VAULT_BLOCK_NUM; j++)
		catalog->fat[catalog->numFiles-1].blockId[j] = -1;
	catalog->numFiles --;

	*updateCatalog = 1;
	sprintf(msg, RM_SUCCESS_MSG, fileName);
	return 0;
}

/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** **********   FETCH    ********** ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */

/* Read block from vault and write it to file
 *
 * @param blockId - index of block to be removed.
 * 				    if -1 then block not in use so does nothing and returns success
 * @param fileId - file descriptor of file to write block to - must be open for write
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 * @param catalog - vault meta-data
 *
 * @return 0 for success, -1 for failure
 */
int readBlock(short blockId, int fileFd, int vaultFd, Catalog catalog) {
	if (blockId == -1) // block not used
		return 0;
	VaultBlock vaultBlock = catalog->blocks[blockId];

	if (lseek(vaultFd, vaultBlock.blockOffset + strlen(DELIM_START), SEEK_SET) == -1) {
		printf(VAULT_SEEK_ERR, strerror(errno));
		return -1;
	}

	// copy block to file
	return copyData(vaultFd, fileFd, vaultBlock.blockSize - strlen(DELIM_START) - strlen(DELIM_END));
}

/* Fetch file from vault by file name (if file in vault) */
int fetchVaultFile(char* fileName, int vaultFd, Catalog catalog, char *msg) {

	// check if filename exists
	int fatEntryId = getFATEntryId(fileName, catalog);
	if (fatEntryId < 0) {
		printf(MISSING_FNAME_ERR);
		return -1;
	}

	// create file
	int fileFd = -1;
	fileFd = open(fileName,O_WRONLY | O_CREAT | O_TRUNC, catalog->fat[fatEntryId].filePerm);
	if (fileFd < 0) {
		printf(FETCH_CREATE_ERR, strerror(errno));
		return -1;
	}

	// copy data from blocks to file
	for (int blockNum = 0; blockNum < VAULT_BLOCK_NUM; blockNum++) {
		if(readBlock(catalog->fat[fatEntryId].blockId[blockNum], fileFd, vaultFd, catalog) == -1) {
			// failed copying block
			printf(FETCH_BLOCK_ERR);
			close(fileFd);
			if (unlink(fileName) == -1) // delete file
				printf(DEL_FETCH_FILE_ERR, strerror(errno));
			return -1;
		}
	}

	close(fileFd);
	sprintf(msg, FETCH_SUCCESS_MSG, fileName);
	return 0;
}

/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** **********   DEFRAG   ********** ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */

/* Defragment vault - shift all data blocks to close gaps between them. */
int defragVault(char* vaultFileName, int vaultFd, Catalog catalog, int* updateCatalog, char* msg) {
	// set for rollback
	*updateCatalog = 0;
	int res = 0;

	// open vault read-only
	int vaultReadFd = -1;
	vaultReadFd = open(vaultFileName, O_RDONLY);
	if (vaultReadFd == -1) {
		printf(VAULT_OPEN_ERR, strerror(errno));
		return -1;
	}

	VaultBlock *vaultBlock = NULL;
	off_t prevEndOffset = sizeof(*catalog); // first offset - from just after catalog
	for (int blockId=0; blockId < catalog->numBlocks; blockId++) {
		vaultBlock = &(catalog->blocks[blockId]);

		// close gap
		if (vaultBlock->blockOffset - prevEndOffset > 0) {
			// remove delimeters before move
			if (wipeDelim(*vaultBlock, vaultFd) == -1) {
				printf(DEFRAG_DELIM_ERR);
				res = -1;
			}
			// go to start of block
			else if ((lseek(vaultFd, prevEndOffset, SEEK_SET) == -1) ||
				(lseek(vaultReadFd, vaultBlock->blockOffset, SEEK_SET) == -1)) {
				printf(VAULT_SEEK_ERR, strerror(errno));
				res = -1;
			}
			// move block
			else if (copyData(vaultReadFd, vaultFd, vaultBlock->blockSize) == -1) {
				printf(MOVE_BLOCK_COPY_ERR);
				res = -1;
			}
			// fix catalog
			vaultBlock->blockOffset = prevEndOffset;
			// return delimiters
			if (res != -1 && addDelim(*vaultBlock, vaultFd) == -1) {
				printf(DEFRAG_DELIM_ERR);
				res = -1;
			}

			// error moving block
			if (res == -1) {
				printf(DATA_CORRUPTION_ERR);
				close(vaultReadFd);
				return -1;
			}


		}

		prevEndOffset += vaultBlock->blockSize;
	}

	*updateCatalog = 1;
	sprintf(msg, DEFRAG_SUCCESS_MSG);
	return res;
}
