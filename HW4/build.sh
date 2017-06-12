#!/bin/bash

gcc -o pcc_client pcc_client.c
gcc -o pcc_server pcc_server.c -lpthread
