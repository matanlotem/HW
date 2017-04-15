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
	int res = -1;
	char cmnd[10];

	/*** timing code from moodle ***/
	struct timeval start_time, end_time;
	long seconds, useconds;
	double mtime;
	gettimeofday(&start_time, NULL);
	/*** timing code end ***/

	if (argc < 3) {
		printf(ARG_NUM_ERR);
		printf(USAGE_ERR);
	}

	// command to lowercase
	else if (parseCmnd(argv[2], cmnd, sizeof(cmnd)) == -1)
		printf(INVALID_CMND_ERR);

	/*** INIT VAULT ***/
	else if (streq(cmnd,INIT_CMND)) {
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

	else if (streq(cmnd,LIST_CMND) || streq(cmnd,ADD_CMND) ||
			 streq(cmnd,RM_CMND) || streq(cmnd,FETCH_CMND) ||
			 streq(cmnd,DEFRAG_CMND) || streq(cmnd,STATUS_CMND)) {

		// open vault
		Catalog catalog = NULL;
		int vaultFd;
		int updateCatalog = 0;
		catalog = openVault(argv[1], &vaultFd);
		if (catalog == NULL)
			res = -1;

		/*** LIST VAULT ***/
		else if (streq(cmnd,LIST_CMND))
			res = listVault(catalog);

		/*** MANIPULATE VAULT FILES ***/
		else if (streq(cmnd,ADD_CMND) || streq(cmnd,RM_CMND) || streq(cmnd,FETCH_CMND)) {
			// validate arguments
			if (argc < 4)
				printf(NO_FILENAME_ERR);

			// run command
			else if (streq(cmnd,ADD_CMND))
				res = addVaultFile(argv[3], vaultFd, catalog, &updateCatalog);
			else if (streq(cmnd,RM_CMND))
				res = rmVaultFile(argv[3], vaultFd, catalog, &updateCatalog);
			else if (streq(cmnd,FETCH_CMND))
				res = fetchVaultFile(argv[3], vaultFd, catalog);
		}

		/*** DEFRAG VAULT ***/
		else if (streq(cmnd,DEFRAG_CMND))
			res = defragVault(argv[1], vaultFd, catalog, &updateCatalog);

		/*** GET VAULT STATUS ***/
		else if (streq(cmnd,STATUS_CMND))
			res = getVaultStatus(catalog);

		// close vault
		if (closeVault(vaultFd, catalog, updateCatalog) == -1)
			res = -1;
	}
	else {
		printf(INVALID_CMND_ERR);
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
