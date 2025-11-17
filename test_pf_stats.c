/*
 * test_pf_stats.c
 *
 * This program tests the configurable buffer manager and statistics collection.
 * It runs a specific workload and prints the resulting I/O statistics.
 */

#include <stdio.h>
#include <stdlib.h> // For exit()
#include "pf.h"     // Your PF layer header

// --- Configuration ---
#define TEST_FILE_NAME  "pf_test_workload.db"
#define BUFFER_SIZE     20  // The number of pages in the buffer pool
#define NUM_PAGES       50  // The number of pages in our test file
#define NUM_REQUESTS    1000 // Total number of page requests to simulate

// --- Strategies (must match what you used in buf.c) ---
#define LRU 0
#define MRU 1
// ---

/*
 * Helper function to check PF errors and exit if something bad happens.
 */
void check_error(int error_code, const char *message) {
    if (error_code != PFE_OK) {
        printf("Error: %s\n", message);
        PF_PrintError("PF Error");
        exit(1);
    }
}

/*
 * Main test function
 */
int main(int argc, char **argv) {
    int fd;
    int pagenum;
    char *pagebuf;
    int error;
    int i;

    // --- Select Strategy ---
    int strategy = LRU;
    printf("Testing with Strategy: %s\n", (strategy == LRU) ? "LRU" : "MRU");
    printf("Buffer Size: %d pages\n", BUFFER_SIZE);
    printf("File Size: %d pages\n", NUM_PAGES);
    printf("Total Requests: %d\n", NUM_REQUESTS);
    printf("---------------------------\n");


    // 1. Initialize PF Layer
    // We pass our new parameters here!
    PF_Init(BUFFER_SIZE, strategy);

    // 2. Create and Open a Test File
    check_error(PF_CreateFile(TEST_FILE_NAME), "Creating file");
    fd = PF_OpenFile(TEST_FILE_NAME);
    if (fd < 0) {
        check_error(fd, "Opening file");
    }

    // 3. Allocate and fill the file with NUM_PAGES
    // We make them all dirty to force writes.
    printf("Writing %d initial pages to file...\n", NUM_PAGES);
    for (i = 0; i < NUM_PAGES; i++) {
        check_error(PF_AllocPage(fd, &pagenum, &pagebuf), "Allocating page");
        // 'pagebuf' is just a pointer, we can write some data to it.
        // Let's write the page number into the page.
        sprintf(pagebuf, "This is page %d", pagenum);
        
        // Unfix the page, marking it as dirty
        check_error(PF_UnfixPage(fd, pagenum, TRUE), "Unfixing dirty page");
    }
    printf("File initialization complete.\n");
/* --- ADD THIS LINE --- */
    check_error(PF_CloseFile(fd), "Closing file after init");
    // 4. Run the Workload
    // This simulates the 90% read / 10% write workload
    printf("Running %d-request workload (90%% Read / 10%% Write)...\n", NUM_REQUESTS);
    
    // We reset stats *after* file creation to only measure the workload
    PF_Init(BUFFER_SIZE, strategy); // This resets all counters
    
    // We must re-open the file since PF_Init() cleared the file table
    fd = PF_OpenFile(TEST_FILE_NAME); 
    if (fd < 0) {
        check_error(fd, "Re-opening file");
    }

    srand(0); // Use a fixed seed for reproducible tests
    for (i = 0; i < NUM_REQUESTS; i++) {
        // Pick a random page to access
        int page_to_access = rand() % NUM_PAGES;
        
        // Get the page
        error = PF_GetThisPage(fd, page_to_access, &pagebuf);
        check_error(error, "Getting page for workload");

        // Decide if this is a read or a write (90% read)
        if (i % 10 == 0) {
            // --- 10% Write ---
            // Modify the page buffer
            sprintf(pagebuf, "This page was modified at request %d", i);
            // Unfix as dirty
            check_error(PF_UnfixPage(fd, page_to_access, TRUE), "Unfixing dirty");
        } else {
            // --- 90% Read ---
            // We just read the page, do nothing, and unfix it.
            check_error(PF_UnfixPage(fd, page_to_access, FALSE), "Unfixing clean");
        }
    }
    printf("Workload complete.\n");


    // 5. Close the file (this flushes all dirty pages)
    check_error(PF_CloseFile(fd), "Closing file");
    printf("File closed.\n");

    // 6. Print the Statistics!
    // This calls your new PF_PrintStats() function.
    PF_PrintStats();

    // 7. Clean up
    check_error(PF_DestroyFile(TEST_FILE_NAME), "Destroying file");

    return 0;
}