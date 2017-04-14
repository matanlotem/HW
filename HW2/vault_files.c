#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include "vault_files.h"
#include "vault_catalog.h"
#include "vault_consts.h"
#include "vault_aux.h"

int wipeDelim(off_t blockOffset, ssize_t blockSize, int vaultFd) {
	// TODO: validate block is not too small
	// try wiping delimiters to prevent data corruption (only for the tester)
	if ((lseek(vaultFd, blockOffset, SEEK_SET) == -1) || // wipe start delimiter
		(write(vaultFd, DELIM_WIPE, strlen(DELIM_WIPE)) != strlen(DELIM_WIPE)) ||
		(lseek(vaultFd, blockOffset + blockSize - strlen(DELIM_WIPE), SEEK_SET) == -1) || // wipe end delimiter
		(write(vaultFd, DELIM_WIPE, strlen(DELIM_WIPE)) != strlen(DELIM_WIPE))) {
		// data might have been corrupted
		printf(WIPE_DELIM_ERR, strerror(errno));
		return -1;
	}
	return 0;
}

/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** **********     ADD    ********** ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */

/* scan gaps
 *   - smallest gap that fits all data if exists
 *   - otherwise largest gap
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
			(newBlock->blockSize > writeSize && gapSize < newBlock->blockSize && newBlock->blockSize >= writeSize)) {
			gapBlockId = blockId;
			newBlock->blockSize = gapSize;
			newBlock->blockOffset = prevEndOffset;
		}

		prevEndOffset = catalog->blocks[blockId].blockOffset + catalog->blocks[blockId].blockSize;
	}

	return gapBlockId;
}

void logBlockToGap(VaultBlock newBlock, short gapBlockId, ssize_t writeSize, Catalog catalog) {
	if (writeSize < newBlock.blockSize)
		newBlock.blockSize = writeSize;

	// shift succeeding blocks right
	for (int i=gapBlockId+1; i<catalog->numBlocks; i++) {
		catalog->blocks[i] = catalog->blocks[i-1];
		catalog->fat[catalog->blocks[i].fatEntryId].blockId[catalog->blocks[i].blockNum] = i;
	}
	// write block to blocks and fat
	catalog->blocks[gapBlockId] = newBlock;
	catalog->fat[newBlock.fatEntryId].blockId[newBlock.blockNum] = gapBlockId;
	catalog->numBlocks++;
}

int writeBlock(int vaultFd, int fileFd, VaultBlock newBlock) {
	// validate block is large enough for both delimiters
	if (strlen(DELIM_START) + strlen(DELIM_END) > newBlock.blockSize) {
		printf(SHORT_BLOCK_ERR);
		return -1;
	}

	// set position to start of block
	if (lseek(vaultFd, newBlock.blockOffset, SEEK_SET) == -1) {
		printf(VAULT_SEEK_ERR, strerror(errno));
		return -1;
	}

	ssize_t tmpSize;
	// write start delimiter
	tmpSize = write(vaultFd, DELIM_START, strlen(DELIM_START));
	if (tmpSize != strlen(DELIM_START)) {
		printf(VAULT_FWRITE_ERR, strerror(errno));
		return -1;
	}

	// copy file to block
	if (copyData(fileFd, vaultFd, newBlock.blockSize - strlen(DELIM_START) - strlen(DELIM_END)) == -1) {
		printf(ADD_BLOCK_COPY_ERR);
		return -1;
	}

	// write end delimiter
	tmpSize = write(vaultFd, DELIM_END, strlen(DELIM_END));
	if (tmpSize != strlen(DELIM_END)) {
		printf(VAULT_FWRITE_ERR, strerror(errno));
		return -1;
	}

	return 0;
}

int addVaultFile(char* vaultFileName, char* filePath) {
	// open vault
	Catalog catalog = NULL;
	int vaultFd;
	catalog = openVault(vaultFileName, &vaultFd);
	if (catalog == NULL)
		return -1;

	// check max files
	if (catalog->numFiles == MAX_VAULT_FILES) {
		printf(MAX_FILE_ERR);
		closeVault(vaultFd, catalog, 0);
		return -1;
	}

	// get filename
	char *fileName = strrchr(filePath,'/');
	if (fileName == NULL) fileName = filePath;
	else fileName++;

	// check if file with same name already exists in vault
	if (getFATEntryId(fileName, catalog) >= 0) {
		printf(SAME_FNAME_ERR);
		closeVault(vaultFd, catalog, 0);
		return -1;
	}

	// get file stats
	struct stat fileStats;
	if (stat(filePath,&fileStats) == -1) {
		printf(FILE_STATS_ERR, strerror(errno));
		closeVault(vaultFd, catalog, 0);
		return -1;
	}

	// add file to fat
	short fatEntryId = catalog->numFiles++;
	strcpy(catalog->fat[fatEntryId].fileName,fileName);
	catalog->fat[fatEntryId].filePerm = fileStats.st_mode;
	catalog->fat[fatEntryId].fileSize = fileStats.st_size;
	struct timeval insertionTime;
	gettimeofday(&insertionTime,NULL);
	catalog->fat[fatEntryId].insertionTime = insertionTime.tv_sec;
	catalog->modificationTime = insertionTime.tv_sec;

	// add blocks
	ssize_t writeSize = catalog->fat[fatEntryId].fileSize;
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
		closeVault(vaultFd, catalog, 0);
		return -1;
	}

	// write data to blocks;
	// open file for read
	int fileFd = -1;
	fileFd = open(filePath, O_RDONLY);
	if (fileFd == -1) {
		printf(FILE_OPEN_ERR, strerror(errno));
		closeVault(vaultFd, catalog, 0);
		return -1;
	}
	// write data
	blockNum = 0;
	while (blockNum < VAULT_BLOCK_NUM) {
		if (catalog->fat[fatEntryId].blockId[blockNum] != -1) { // check block is not empty
			VaultBlock newBlock = catalog->blocks[catalog->fat[fatEntryId].blockId[blockNum]];
			if (writeBlock(vaultFd, fileFd, newBlock) == -1) {
				// error writing block
				close(fileFd);
				// wipe blocks to prevent data corruption
				for (int i=blockNum; i>=0; i--) {
					newBlock = catalog->blocks[catalog->fat[fatEntryId].blockId[i]];
					if (wipeDelim(newBlock.blockOffset, newBlock.blockSize, vaultFd) == -1)
						// failed wiping some of the blocks
						printf(DATA_CORRUPTION_ERR);
				}
				closeVault(vaultFd, catalog, 0);
				return -1;
			}
		blockNum++;
		}
		// go to end if reached empty block
		else blockNum = VAULT_BLOCK_NUM;
	}

	close(fileFd);
	closeVault(vaultFd, catalog, 1);
	printf(ADD_SUCCESS_MSG, fileName);
	return 0;
}

/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** **********   REMOVE   ********** ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */

/*
 * delete block
 * Remove block from block array and shift succeeding blocks left.
 * Fix fat blockIds.
 * Try to wipe delimiters (if fails then vault might be corrupted)
 *
 */
int rmBlock(short fatEntryId, short blockNum, int vaultFd, Catalog catalog) {
	short blockId = catalog->fat[fatEntryId].blockId[blockNum];
	if (blockId == -1) // block not used
		return 0;

	short blockOffset = catalog->blocks[blockId].blockOffset;
	short blockSize = catalog->blocks[blockId].blockSize;

	// update catalog
	for (int i=blockId; i<catalog->numBlocks-1; i++) { // shift blocks left
		catalog->blocks[i] = catalog->blocks[i+1];
		catalog->fat[catalog->blocks[i].fatEntryId].blockId[catalog->blocks[i].blockNum] = i;
	}
	// delete last block
	catalog->blocks[catalog->numBlocks-1].fatEntryId = -1;
	catalog->fat[fatEntryId].blockId[blockNum] = -1;
	catalog->numBlocks --;

	// wipe delimiters
	return wipeDelim(blockOffset, blockSize, vaultFd);
}

int rmVaultFile(char* vaultFileName, char* fileName) {
	// open vault
	Catalog catalog = NULL;
	int vaultFd;
	catalog = openVault(vaultFileName, &vaultFd);
	if (catalog == NULL)
		return -1;

	// check if exists
	int fatEntryId = getFATEntryId(fileName, catalog);
	if (fatEntryId < 0) {
		printf(MISSING_FNAME_ERR);
		closeVault(vaultFd, catalog, 0);
		return -1;
	}

	// delete blocks
	for (int blockNum = 0; blockNum < VAULT_BLOCK_NUM; blockNum++) {
		if(rmBlock(fatEntryId, blockNum, vaultFd, catalog) == -1) {
			// failed removing block - cannot be fixed
			printf(DATA_CORRUPTION_ERR);
			closeVault(vaultFd, catalog, 0);
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

	closeVault(vaultFd, catalog, 1);
	printf(RM_SUCCESS_MSG, fileName);
	return 0;
}

/* ********** ********** ********** ********** ********** ********** ********** */
/* ********** ********** **********   FETCH    ********** ********** ********** */
/* ********** ********** ********** ********** ********** ********** ********** */
int readBlock(short fatEntryId, short blockNum, int fileFd, int vaultFd, Catalog catalog) {
	short blockId = catalog->fat[fatEntryId].blockId[blockNum];
	if (blockId == -1) // block not used
		return 0;

	short blockOffset = catalog->blocks[blockId].blockOffset;
	short blockSize = catalog->blocks[blockId].blockSize;

	if (lseek(vaultFd, blockOffset + strlen(DELIM_START), SEEK_SET) == -1) {
		printf(VAULT_SEEK_ERR, strerror(errno));
		return -1;
	}

	// copy block to file
	return copyData(vaultFd, fileFd, blockSize - strlen(DELIM_START) - strlen(DELIM_END));
}

int fetchVaultFile(char* vaultFileName, char* fileName) {
	// open vault
	Catalog catalog = NULL;
	int vaultFd;
	catalog = openVault(vaultFileName, &vaultFd);
	if (catalog == NULL)
		return -1;

	// check if filename exists
	int fatEntryId = getFATEntryId(fileName, catalog);
	if (fatEntryId < 0) {
		printf(MISSING_FNAME_ERR);
		closeVault(vaultFd, catalog, 0);
		return -1;
	}

	// create file
	int fileFd = -1;
	fileFd = open(fileName,O_WRONLY | O_CREAT | O_TRUNC, catalog->fat[fatEntryId].filePerm);
	if (fileFd < 0) {
		printf(FETCH_CREATE_ERR, strerror(errno));
		closeVault(vaultFd, catalog, 0);
		return -1;
	}

	// copy data from blocks to file
	for (int blockNum = 0; blockNum < VAULT_BLOCK_NUM; blockNum++) {
		if(readBlock(fatEntryId, blockNum, fileFd, vaultFd, catalog) == -1) {
			// failed copying block
			printf(FETCH_BLOCK_ERR);
			close(fileFd);
			if (unlink(fileName) == -1) // delete file
				printf(DEL_FETCH_FILE_ERR, strerror(errno));
			closeVault(vaultFd, catalog, 0);
			return -1;
		}
	}

	close(fileFd);
	closeVault(vaultFd, catalog, 0);
	printf(FETCH_SUCCESS_MSG, fileName);
	return 0;
}
