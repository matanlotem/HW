#ifndef VAULT_FILES_H_
#define VAULT_FILES_H_

#include "vault_catalog.h"

int addVaultFile(char* filePath, int vaultFd, Catalog catalog, int* updateCatalog);

int rmVaultFile(char* fileName, int vaultFd, Catalog catalog, int* updateCatalog);

int fetchVaultFile(char* fileName, int vaultFd, Catalog catalog);

int defragVault(char* vaultFileName, int vaultFd, Catalog catalog, int* updateCatalog);

#endif /* VAULT_FILES_H_ */
