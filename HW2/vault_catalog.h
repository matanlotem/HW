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


int initVault(char* vaultFileName, ssize_t vaultSize);

Catalog openVault(char* vaultFileName, int *vaultFd);

int closeVault(int vaultFd, Catalog catalog, int updateCatalog);

int listVault(Catalog catalog);

int getVaultStatus(Catalog catalog);

int getFATEntryId(char* fileName, Catalog catalog);

void printVaultBlock(VaultBlock vaultBlock);
void printFATEntry(FATEntry fatEntry);
void printBlocks(Catalog catalog);
void printFAT(Catalog catalog);

#endif /* VAULT_CATALOG_H_ */
