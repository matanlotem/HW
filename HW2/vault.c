#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include "vault_aux.h"
#include "vault_catalog.h"
#include "vault_files.h"
#include "vault_consts.h"





int main (int argc, char** argv) {
	Catalog catalog = NULL;
	int res;

	if (argc < 3) {
		printf(ARG_NUM_ERR);
		printf(USAGE_ERR);
		return -1;
	}

	// command to lowercase
	char cmnd[10];
	if (parseCmnd(argv[2], cmnd, sizeof(cmnd)) == -1) {
		printf(INVALID_CMND_ERR);
		return -1;
	}

	/*** INIT VAULT ***/
	if (streq(cmnd,INIT_CMND)) {
		// validate arguments
		if (argc < 4) {
			printf(INIT_SIZE_ERR);
			return -1;
		}

		// parse size
		ssize_t vaultSize = parseSize(argv[3]);
		if (vaultSize <= 0) {
			printf(INIT_SIZE_ERR);
			return -1;
		}

		return initVault(argv[1], vaultSize);
	}

	/*** LIST VAULT ***/
	else if (streq(cmnd,LIST_CMND)) {
		return listVault(argv[1]);
	}

	/*** MANIPULATE VAULT FILES ***/
	else if (streq(cmnd,ADD_CMND) || streq(cmnd,RM_CMND) || streq(cmnd,FETCH_CMND)) {
		// validate arguments
		if (argc < 4) {
			printf(NO_FILENAME_ERR);
			return -1;
		}

		// run command
		if (streq(cmnd,ADD_CMND))
			res = addVaultFile(argv[1], argv[3]);
		else if (streq(cmnd,RM_CMND))
			res = rmVaultFile(argv[1], argv[3]);
		else if (streq(cmnd,FETCH_CMND))
			res = fetchVaultFile(argv[1], argv[3]);

		return res;
	}

	/*** DEFRAG VAULT ***/
	//TODO
	/*else if (streq(cmnd,DEFRAG_CMND)) {
		if (loadVault(argv[1], &catalog) == -1) return -1;
	}*/

	/*** GET VAULT STATS ***/
	//TODO
	/*else if (streq(cmnd,STATUS_CMND)) {
		if (loadVault(argv[1], &catalog) == -1) return -1;
	}*/
	// TODO: timings
	printf(INVALID_CMND_ERR);
	return -1;
}
