#ifndef VAULT_FILES_H_
#define VAULT_FILES_H_

#include "vault_catalog.h"

int addVaultFile(char* vaultFileName, char* filePath);

int rmVaultFile(char* vaultFileName, char* fileName);

int fetchVaultFile(char* vaultFileName, char* fileName);


#endif /* VAULT_FILES_H_ */
