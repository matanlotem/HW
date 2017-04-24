#ifndef VAULT_CONSTS_H_
#define VAULT_CONSTS_H_

#define _FILE_OFFSET_BITS 64

// vault parameters
#define MAX_VAULT_FILES 100
#define VAULT_BLOCK_NUM 3
#define MAX_VAULT_FNAME 256
#define DELIM_START "<<<<<<<<"
#define DELIM_END   ">>>>>>>>"
#define DELIM_WIPE  "00000000"
#define BUFFER_SIZE 4096

// vault commands
#define INIT_CMND "init"
#define LIST_CMND "list"
#define ADD_CMND "add"
#define RM_CMND "rm"
#define FETCH_CMND "fetch"
#define DEFRAG_CMND "defrag"
#define STATUS_CMND "status"
#define CMNDS_LIST INIT_CMND" | "LIST_CMND" | "ADD_CMND" | "RM_CMND" | "FETCH_CMND" | "DEFRAG_CMND" | "STATUS_CMND

// general errors
#define ALLOC_ERR "Allocation error\n"
#define DATA_READ_ERR "Error reading data: %s\n"
#define DATA_WRITE_ERR "Error writing data: %s\n"

// vault usage errors
#define ARG_NUM_ERR "Invalid number of arguments\n"
#define USAGE_ERR "Usage: ./vault <vault_file> <command> (<argument>)\n"
#define INVALID_CMND_ERR "Invalid command. Command must be one of:\n"CMNDS_LIST"\n"
#define INIT_SIZE_ERR "Vault file size must be supplied as an integer followed by a unit letter B,K,M,G\n"
#define NO_FILENAME_ERR "No filename supplied\n"

// vault io errors
#define VAULT_SIZE_ERR "Vault too small\n"
#define VAULT_OPEN_ERR "Error opening vault file: %s\n"
#define CATALOG_READ_ERR "Error reading catalog: %s\n"
#define VAULT_CREATION_ERR "Error creating vault file: %s\n"
#define CATALOG_WRITE_ERR "Error writing catalog to vault file: %s\nVault file might be corrupt\n"
#define VAULT_STRECH_ERR "Error stretching vault file to required size: %s\n"
#define FILE_OPEN_ERR "Error opening file to add to vault: %s\n"
#define VAULT_FWRITE_ERR "Error writing file to vault: %s\n"
#define VAULT_FREAD_ERR "Error reading file from vault: %s\n"
#define VAULT_SEEK_ERR "Error moving to requested vault offset: %s\n"

// vault file manipulation errors
#define MAX_FILE_ERR "Maximum number of files exceeded\n"
#define CANNOT_FIT_ERR "Could not fit file in vault\n"
#define FILE_STATS_ERR "Could not get file state: %s\n"
#define SAME_FNAME_ERR "File with same name already in vault\n"
#define MISSING_FNAME_ERR "File not in vault\n"
#define SHORT_BLOCK_ERR "Block too small for delimiters\n"
#define ADD_BLOCK_COPY_ERR "Error copying block from file to vault\n"
#define WIPE_DELIM_ERR "Failed wiping delimiters: %s\n"
#define ADD_DELIM_ERR "Failed adding delimiters\n"
#define DEFRAG_DELIM_ERR "Failed moving delimiters while defragmenting\n"
#define DATA_CORRUPTION_ERR "Failed while manipulating blocks. Catalog will be rolled back but vault data might have been corrupted.\n"
#define MOVE_BLOCK_COPY_ERR "Error moving block while defragmenting\n"
#define FETCH_CREATE_ERR "Error creating fetched file: %s\n"
#define FETCH_BLOCK_ERR "Error copying fetched file block\n"
#define FETCH_PERMS_ERR "Error setting fetched file permissions: %s\n"
#define DEL_FETCH_FILE_ERR "Error removing file after failed fetch: %s\n"


// messages
#define INIT_SUCCESS_MSG "Result: A vault created\n"
#define ADD_SUCCESS_MSG "Result: %s inserted\n"
#define FETCH_SUCCESS_MSG "Result: %s created\n"
#define RM_SUCCESS_MSG "Result: %s deleted\n"
#define DEFRAG_SUCCESS_MSG "Result: Defragmentation complete\n"
#define NUM_FILES_MSG  "Number of files:       %d\n"
#define TOTAL_SIZE_MSG "Total size:            %dB\n"
#define FRAG_RATIO_MSG "Fragmentation ratio:   %.2f\n"


#endif /* VAULT_CONSTS_H_ */
