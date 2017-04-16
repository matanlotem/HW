#ifndef VAULT_FILES_H_
#define VAULT_FILES_H_

#include "vault_catalog.h"

/* Add file to vault under the following restrictions
 *   - no file of same name already in vault
 *   - file can be fit in vault in up to 3 fragments
 *
 * On failure rolls back catalog and tries to wipe delimiters -
 * if this fails vault may be corrupted for the tester.
 *
 * @param filePath - path of file to be added to vault
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 * @param catalog - vault meta-data
 * @param updateCatalog - return parameter - on success is set to 1 to flush
 * 						  catalog to vault file when done. Otherwise set to 0.
 * @param msg - return parameter - formatted success message on success
 * 				otherwise left untouched
 *
 * @return 0 for success, -1 for failure
 */
int addVaultFile(char* filePath, int vaultFd, Catalog catalog, int* updateCatalog, char* msg);

/* Remove file from vault by file name (if file in vault)
 * Lazy remove - only removes from catalog and wipes delimiters.
 *
 * If delimiter wipe fails, vault data might be corrupted for the tester.
 * The catalog is rolled back as if the file was not deleted.
 *
 * @param fileName - name of file in vault to be removed (fails if does not exist)
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 * @param catalog - vault meta-data
 * @param updateCatalog - return parameter - on success is set to 1 to flush
 * 						  catalog to vault file when done. Otherwise set to 0.
 * @param msg - return parameter - formatted success message on success
 * 				otherwise left untouched
 *
 * @return 0 for success, -1 for failure
 */
int rmVaultFile(char* fileName, int vaultFd, Catalog catalog, int* updateCatalog, char* msg);

/* Fetch file from vault by file name (if file in vault)
 * Creates a file in working directory with name and permissions as set in vault.
 * File content matches that of the original file added to the vault.
 * If fails during copy, tries to remove the file from working directory.
 *
 * @param fileName - name of file in vault to be fetched (fails if does not exist)
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 * @param catalog - vault meta-data
 * @param msg - return parameter - formatted success message on success
 * 				otherwise left untouched
 *
 * @return 0 for success, -1 for failure
 */
int fetchVaultFile(char* fileName, int vaultFd, Catalog catalog, char* msg);

/* Defragment vault - shift all data blocks to close gaps between them.
 * Shifts first block to sit just after the catalog.
 * For convenience wipes delimiters before copy and returns them at the end.
 * If something during the defragmention process fails, vault file will
 * probably be corrupted and unfixable.
 *
 * @param vaultFileName - path of vault file - to open a second read only file descriptor
 * @param vaultFd - file descriptor of vault file - must be open for read/write
 * @param catalog - vault meta-data
 * @param updateCatalog - return parameter - on success is set to 1 to flush
 * 						  catalog to vault file when done. Otherwise set to 0.
 * @param msg - return parameter - formatted success message on success
 * 				otherwise left untouched
 *
 * @return 0 for success, -1 for failure
 */
int defragVault(char* vaultFileName, int vaultFd, Catalog catalog, int* updateCatalog, char* msg);

#endif /* VAULT_FILES_H_ */
