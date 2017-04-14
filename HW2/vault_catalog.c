#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>


#include "vault_catalog.h"
#include "vault_aux.h"
#include "vault_consts.h"



int initVault(char* vaultFileName, ssize_t vaultSize) {
	int res = -1;
	// create catalog
	Catalog catalog = NULL;
	catalog = (Catalog) malloc(sizeof(*catalog));
	if (catalog == NULL) {
		printf(ALLOC_ERR);
		return -1;
	}
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
	//TODO: set permissions
	//TODO: check vault file size is not too small
	vaultFd = open(vaultFileName,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (vaultFd < 0) {
		printf(VAULT_CREATION_ERR, strerror(errno));
		goto INIT_END;
	}

	// write catalog to file
	if (write(vaultFd,catalog, sizeof(*catalog)) != sizeof(*catalog)) {
		printf(CATALOG_WRITE_ERR, strerror(errno));
		goto INIT_END;
	}

	// strech file
	if (lseek(vaultFd, vaultSize-1, SEEK_SET) == -1) {
		printf(VAULT_STRECH_ERR, strerror(errno));
		goto INIT_END;
	}
	if (write(vaultFd, "", 1) < 0) {
		printf(CATALOG_WRITE_ERR, strerror(errno));
		goto INIT_END;
	}

	res = 0;
	INIT_END:
	if (vaultFd >=0) close(vaultFd);
	free(catalog);
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


int listVault(char* vaultFileName) {
	// open vault
	Catalog catalog = NULL;
	int vaultFd;
	catalog = openVault(vaultFileName, &vaultFd);
	if (catalog == NULL)
		return -1;

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

	closeVault(vaultFd, catalog, 0);
	return 0;

}

int getFATEntryId(char* fileName, Catalog catalog) {
	for (int i=0; i<catalog->numFiles; i++)
		if (streq(fileName,catalog->fat[i].fileName))
			return i;
	return -1;
}
