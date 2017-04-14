#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define USAGE_ERR "USAGE: data_filter <size> <input_file> <output_file>\n"
#define SIZE_ERR "Size must be an integer followed by a letter B,K,M,G\n"
#define ALLOC_ERR "Allocation error\n"
#define IFILE_ERR "Error opening input file: %s\n"
#define OFILE_ERR "Error opening output file: %s\n"
#define FREAD_ERR "Error reading from file: %s\n"
#define FWRITE_ERR "Error writing to file: %s\n"
#define OUTPUT_MSG "%.0f characters requested, %.0f characters read, %.0f are printable\n"

#define L_BUFF 4096
#define S_BUFF 512
#define L_FILE 524188

// parse size string
double get_size(char* size_str) {
    if (strlen(size_str) < 2)
        return 0;
    
    double psize = atoi(size_str);

    // validate digits 
    int i;
    for (i=0; i<strlen(size_str)-1; i++)
        if (!isdigit(size_str[i]))
            return 0;
    
    // get units
    switch (size_str[strlen(size_str)-1]) {
        case 'G':
            psize *= 1024;
        case 'M':
            psize *= 1024;
        case 'K':
            psize *= 1024;
        case 'B':
            break;
        default:
            return 0;
    }

    return psize;
}


int main(int argc, char** argv) {
    if (argc < 4) {
        printf(USAGE_ERR);
        return -1;
    }
    int res = -1;

    // parse output size
    double request_size = get_size(argv[1]);
    if (request_size <= 0) {
        printf(SIZE_ERR);
        return -1;
    }
    double processed_size = 0, printable_size = 0;
    
    // allocate buffer
    ssize_t buffer_size = ((request_size > L_FILE) ? L_BUFF : S_BUFF);
    char* input_buffer = (char*) malloc(sizeof(char)*buffer_size);
    char* output_buffer = (char*) malloc(sizeof(char)*buffer_size);
    if (!input_buffer || !output_buffer) {
        free(input_buffer);
        free(output_buffer);
        printf(ALLOC_ERR);
        return -1;
    }
    
    // open input and output files
    int input_fd=-1, output_fd=-1;
    input_fd = open(argv[2],O_RDONLY);
    if (input_fd < 0) {
        printf(IFILE_ERR, strerror(errno));
        goto FINISH;
    }
    output_fd = open(argv[3],O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (!output_fd) {
        printf(OFILE_ERR, strerror(errno));
        goto FINISH;
    }
    
    // start processing
    ssize_t read_size, j=0, i;
    while (processed_size < request_size) {
        read_size = read(input_fd, input_buffer, buffer_size);
        // error reading from file
        if (read_size < 0) {
            printf(FREAD_ERR,strerror(errno));
            goto FINISH;
        }
        // end of file -> go to beginnig for next read
        if (read_size != buffer_size)
            lseek(input_fd, 0, SEEK_SET);

        // don't process more bytes than necessary
        if (read_size > request_size - processed_size)
            read_size = request_size - processed_size;

        for (i=0; i < read_size; i++) {
            // check if printable
            if (input_buffer[i] <= 126 && input_buffer[i] >= 32) {
                output_buffer[j++] = input_buffer[i];
                printable_size++;
            }
            processed_size++;
            
            // flush buffer into file when full (or done processing)
            if (j >= buffer_size || request_size == processed_size) {
                if (write(output_fd,output_buffer, j) != j) {
                    printf(FWRITE_ERR, strerror(errno));
                    goto FINISH;
                }
                j = 0;
            }
        }
    }
    
    res = 0;

    // print output and exit
    FINISH:
    printf(OUTPUT_MSG,request_size, processed_size, printable_size);
   
    free(input_buffer);
    free(output_buffer);
    if (input_fd >= 0) close(input_fd);
    if (output_fd >= 0) close(output_fd);

    return res;
}
