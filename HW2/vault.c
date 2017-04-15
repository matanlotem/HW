#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "vault_aux.h"
#include "vault_catalog.h"
#include "vault_files.h"
#include "vault_consts.h"


int main (int argc, char** argv) {
	Catalog catalog = NULL;
	int res = -1;

	/*** timing code from moodle ***/
	struct timeval start_time, end_time;
	long seconds, useconds;
	double mtime;
	gettimeofday(&start_time, NULL);
	/*** timing code end ***/

	// command to lowercase
	if (argc >= 3) strToLower(argv[2]);

	// validate argument number
	if (argc < 3) {
		printf(ARG_NUM_ERR);
		printf(USAGE_ERR);
	}

	/*** INIT VAULT ***/
	else if (streq(argv[2],INIT_CMND)) {
		// validate arguments
		if (argc < 4)
			printf(INIT_SIZE_ERR);

		else {
			// parse size
			ssize_t vaultSize = parseSize(argv[3]);
			if (vaultSize <= 0)
				printf(INIT_SIZE_ERR);

			// run init
			else
				res = initVault(argv[1], vaultSize);
		}
	}

	else if (streq(argv[2],LIST_CMND) || streq(argv[2],ADD_CMND) ||
			 streq(argv[2],RM_CMND) || streq(argv[2],FETCH_CMND) ||
			 streq(argv[2],DEFRAG_CMND) || streq(argv[2],STATUS_CMND)) {

		char msg[1024] = "";

		// open vault
		Catalog catalog = NULL;
		int vaultFd;
		int updateCatalog = 0;
		catalog = openVault(argv[1], &vaultFd);
		if (catalog == NULL)
			res = -1;

		/*** LIST VAULT ***/
		else if (streq(argv[2],LIST_CMND))
			res = listVault(catalog);

		/*** GET VAULT STATUS ***/
		else if (streq(argv[2],STATUS_CMND))
			res = getVaultStatus(catalog);

		/*** MANIPULATE VAULT FILES ***/
		else if (streq(argv[2],ADD_CMND) || streq(argv[2],RM_CMND) || streq(argv[2],FETCH_CMND)) {
			// validate arguments
			if (argc < 4)
				printf(NO_FILENAME_ERR);

			// run command
			else if (streq(argv[2],ADD_CMND))
				res = addVaultFile(argv[3], vaultFd, catalog, &updateCatalog, msg);
			else if (streq(argv[2],RM_CMND))
				res = rmVaultFile(argv[3], vaultFd, catalog, &updateCatalog, msg);
			else if (streq(argv[2],FETCH_CMND))
				res = fetchVaultFile(argv[3], vaultFd, catalog, msg);
		}

		/*** DEFRAG VAULT ***/
		else if (streq(argv[2],DEFRAG_CMND))
			res = defragVault(argv[1], vaultFd, catalog, &updateCatalog, msg);

		// close vault
		if (closeVault(vaultFd, catalog, updateCatalog) == -1)
			res = -1;
		// success messages
		else if (res != -1 && strlen(msg)>0)
			printf("%s",msg);

	}
	else {
		printf(INVALID_CMND_ERR);
		res = -1;
	}

	/*** timing code from moodle ***/
	gettimeofday(&end_time, NULL);
	seconds  = end_time.tv_sec  - start_time.tv_sec;
	useconds = end_time.tv_usec - start_time.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0);
	printf("Elapsed time: %.3f milliseconds\n", mtime);
	/*** timing code end ***/

	return res;
}
