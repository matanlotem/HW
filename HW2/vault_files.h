#ifndef VAULT_FILES_H_
#define VAULT_FILES_H_

#include "vault_catalog.h"

int addVaultFile(char* filePath, int vaultFd, Catalog catalog, int* updateCatalog, char* msg);

int rmVaultFile(char* fileName, int vaultFd, Catalog catalog, int* updateCatalog, char* msg);

int fetchVaultFile(char* fileName, int vaultFd, Catalog catalog, char* msg);

int defragVault(char* vaultFileName, int vaultFd, Catalog catalog, int* updateCatalog, char* msg);

#endif /* VAULT_FILES_H_ */
