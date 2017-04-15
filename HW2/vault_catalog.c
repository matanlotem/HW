#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "vault_catalog.h"
#include "vault_consts.h"
#include "vault_aux.h"

int initVault(char* vaultFileName, ssize_t vaultSize) {
	int res = 0;
	// create catalog
	Catalog catalog = NULL;
	// validate vault is large enough
	if (sizeof(*catalog) > vaultSize) {
		printf(VAULT_SIZE_ERR);
		return -1;
	}
	// allocate catalog
	catalog = (Catalog) malloc(sizeof(*catalog));
	if (catalog == NULL) {
		printf(ALLOC_ERR);
		return -1;
	}

	// initialize vault attributes
	catalog->vaultSize = vaultSize;
	catalog->numFiles = 0;
	catalog->numBlocks = 0;

	// nullify all fat entries and blocks
	for (int i=0; i < MAX_VAULT_FILES; i++) {
		for (int j=0; j < VAULT_BLOCK_NUM; j++) {
			catalog->fat[i].blockId[j] = -1;
			catalog->blocks[i*VAULT_BLOCK_NUM + j].fatEntryId = -1;
		}
	}

	// time
	struct timeval creationTime;
	gettimeofday(&creationTime,NULL);
	catalog->creationTime = creationTime.tv_sec;
	catalog->modificationTime = catalog->creationTime;

	// create vault file
	int vaultFd = -1;
	vaultFd = open(vaultFileName,O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (vaultFd < 0) {
		printf(VAULT_CREATION_ERR, strerror(errno));
		res = -1;
	}

	// write catalog to file
	else if (write(vaultFd,catalog, sizeof(*catalog)) != sizeof(*catalog)) {
		printf(CATALOG_WRITE_ERR, strerror(errno));
		res = -1;
	}

	// strech file
	else if (lseek(vaultFd, vaultSize-1, SEEK_SET) == -1 ||
		write(vaultFd, "", 1) < 0) {
		printf(VAULT_STRECH_ERR, strerror(errno));
		res = -1;
	}

	if (vaultFd >=0) close(vaultFd);
	free(catalog);
	if (res != -1)
		printf(INIT_SUCCESS_MSG);
	return res;
}

Catalog openVault(char* vaultFileName, int *vaultFd) {
	// open vault file
	*vaultFd = -1;
    *vaultFd = open(vaultFileName,O_RDWR);
    if (*vaultFd < 0) {
        printf(VAULT_OPEN_ERR, strerror(errno));
        return NULL;
    }

    // allocate catalog
	Catalog catalog = NULL;
	catalog = (Catalog) malloc(sizeof(*catalog));
	if (catalog == NULL) {
		printf(ALLOC_ERR);
		closeVault(*vaultFd, catalog, 0);
		return NULL;
	}

	// read catalog
    ssize_t readSize = read(*vaultFd, catalog, sizeof(*catalog));
    if (readSize != sizeof(*catalog)) {
    	if (readSize == -1)
    		printf(CATALOG_READ_ERR, strerror(errno));
    	else
    		printf(CATALOG_READ_ERR, "");
    	closeVault(*vaultFd, catalog, 0);
    	return NULL;
    }

    return catalog;
}


int closeVault(int vaultFd, Catalog catalog, int updateCatalog) {
	int res = 0;
	if (updateCatalog) {
		if (catalog == NULL || vaultFd < 0) {
			printf(CATALOG_WRITE_ERR, "");
			res = -1;
		}
		else {
			if (lseek(vaultFd, 0, SEEK_SET) == -1) {
				printf(VAULT_SEEK_ERR, strerror(errno));
				res = -1;
			}
			else if (write(vaultFd,catalog, sizeof(*catalog)) != sizeof(*catalog)) {
				printf(CATALOG_WRITE_ERR, strerror(errno));
				res = -1;
			}
		}
	}
	if (catalog != NULL) free(catalog);
	if (vaultFd >= 0) close(vaultFd);
	return res;
}


int listVault(Catalog catalog) {
	// get length of longest file name
	int fnameLen = 0;
	for (int i=0; i<catalog->numFiles; i++)
		if (strlen(catalog->fat[i].fileName) > fnameLen)
			fnameLen = strlen(catalog->fat[i].fileName);

	// format and print
	char sizeStr[10], dateStr[30];
	for (int i=0; i<catalog->numFiles; i++) {
		formatSize(sizeStr, catalog->fat[i].fileSize);
		strftime(dateStr, 30, "%A %b %d %H:%M:%S %Y", localtime(&(catalog->fat[i].insertionTime)));
		printf("%-*s%-8s%.4o%32s\n",fnameLen+4, catalog->fat[i].fileName, sizeStr,
				catalog->fat[i].filePerm & 0777, dateStr);
	}

	return 0;
}

int getVaultStatus(Catalog catalog) {
	// calculate status
	ssize_t totalSize = 0;
	double fragRatio = 0;
	if (catalog->numBlocks > 0) {
		for (int blockId=0; blockId < catalog->numBlocks; blockId++)
			totalSize += catalog->blocks[blockId].blockSize;

		fragRatio = 1 - ((double) totalSize) / (catalog->blocks[catalog->numBlocks-1].blockOffset +
				catalog->blocks[catalog->numBlocks-1].blockSize - catalog->blocks[0].blockOffset);
	}

	// output status
	printf(NUM_FILES_MSG, catalog->numFiles);
	printf(TOTAL_SIZE_MSG, totalSize);
	printf(FRAG_RATIO_MSG, fragRatio);
	//printFAT(catalog);
	//printBlocks(catalog);

	return 0;
}

int getFATEntryId(char* fileName, Catalog catalog) {
	for (int i=0; i<catalog->numFiles; i++)
		if (streq(fileName,catalog->fat[i].fileName))
			return i;
	return -1;
}

/*** DEBUG PRINT METHODS ***/
void printVaultBlock(VaultBlock vaultBlock) {
	printf("fat: %d (%d) \tsize: %d\toffset: %d\n", (int) vaultBlock.fatEntryId, vaultBlock.blockNum,
			(int) vaultBlock.blockSize, (int) vaultBlock.blockOffset);
}

void printFATEntry(FATEntry fatEntry) {
	printf("num: %s\tsize: %d\tperm: %.4o\n", fatEntry.fileName,
			(int) fatEntry.fileSize, fatEntry.filePerm & 0777);
}

void printBlocks(Catalog catalog) {
	printf("num blocks: %d\t\tstart\toffset: %d\n",catalog->numBlocks, sizeof(*catalog));
	for (int i=0; i<catalog->numBlocks; i++)
		printVaultBlock(catalog->blocks[i]);
	printf("\t\t\tlast\toffset: %d\n",(int) catalog->vaultSize);
}

void printFAT(Catalog catalog) {
	printf("num files: %d\n",catalog->numFiles);
	for (int i=0; i<catalog->numFiles; i++)
		printFATEntry(catalog->fat[i]);
}
