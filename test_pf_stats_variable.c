/*
 * test_pf_stats_variable.c
 * Modified version to accept read percentage as argument
 */

#include <stdio.h>
#include <stdlib.h>
#include "pf.h"

#define FILE_NAME "test_stats.dat"
#define NUM_PAGES 50
#define NUM_REQUESTS 1000
#define BUF_SIZE 10

int main(int argc, char *argv[]) {
    int fd;
    int pagenum;
    char *pagebuf;
    int i, j;
    int read_percentage = 90; // Default
    
    if (argc > 1) {
        read_percentage = atoi(argv[1]);
        if (read_percentage < 0) read_percentage = 0;
        if (read_percentage > 100) read_percentage = 100;
    }
    
    // Initialize with LRU strategy
    PF_Init(BUF_SIZE, 0);
    
    // Create and populate test file
    PF_CreateFile(FILE_NAME);
    fd = PF_OpenFile(FILE_NAME);
    
    for (i = 0; i < NUM_PAGES; i++) {
        PF_AllocPage(fd, &pagenum, &pagebuf);
        *((int*)pagebuf) = i;
        PF_UnfixPage(fd, pagenum, 1);
    }
    
    // Run workload
    srand(42);
    for (i = 0; i < NUM_REQUESTS; i++) {
        pagenum = rand() % NUM_PAGES;
        PF_GetThisPage(fd, pagenum, &pagebuf);
        
        int is_write = (rand() % 100) >= read_percentage;
        if (is_write) {
            *((int*)pagebuf) = i;
            PF_UnfixPage(fd, pagenum, 1);
        } else {
            PF_UnfixPage(fd, pagenum, 0);
        }
    }
    
    // Print stats in parseable format
    printf("READ_WRITE_RATIO,%d/%d\n", read_percentage, 100-read_percentage);
    PF_PrintStats();
    
    PF_CloseFile(fd);
    PF_DestroyFile(FILE_NAME);
    
    return 0;
}
