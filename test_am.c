/*
 * test_am.c
 *
 * This program tests the AM layer by building an index on
 * the student data file using three different methods and
 * comparing their performance (Time and Page I/O).
 *
 * (CORRECTED VERSION 3: Robust header skipping)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // For clock()
#include "pf.h"   // PF layer
#include "hf.h"   // HF layer
#include "am.h"   // AM layer (NEW!)

// --- File Definitions ---
#define HEAP_FILE_NAME      "student.hf"
#define INDEX_FILE_NAME     "student.hf.1" // Per the AM spec (fileName.indexNo)
#define STUDENT_DATA_FILE   "../data/student.txt"
#define STUDENT_SORTED_FILE "../data/student_sorted.txt" // From your 'sort' command
#define MAX_LINE_LENGTH     255

// --- Index Configuration ---
#define INDEX_NO 1       // We are building index #1
#define ATTR_TYPE 'i'    // 'i' for integer
#define ATTR_LENGTH 4    // 4 bytes for an integer

/*
 * Helper function to check errors from all layers
 */
void check_error(int error_code, const char *message) {
    if (error_code < 0) { // All error codes are < 0
        printf("Error: %s (code: %d)\n", message, error_code);
        exit(1);
    }
}

/*
 * Helper function to parse the roll-no (as an int) from a record.
 * This version finds the *second* field, separated by ';'
 */
int get_roll_no(char *record) {
    char *first_sep = strchr(record, ';');
    if (first_sep == NULL) {
        // This is not a data record (e.g., header, blank line)
        // We no longer print an error, just return -1
        return -1;
    }
    char *second_sep = strchr(first_sep + 1, ';');
    if (second_sep == NULL) {
        // Also not a data record
        return -1;
    }
    int len = second_sep - (first_sep + 1);
    char roll_str[32];
    if (len <= 0) return -1; // Empty field
    if (len > 31) len = 31;
    
    strncpy(roll_str, first_sep + 1, len);
    roll_str[len] = '\0';
    
    return atoi(roll_str);
}


/*
 * Main test function
 */
int main() {
    int hfFd;  // Heap File descriptor
    int amFd;  // Index File descriptor
    int scanFd;
    char lineBuffer[MAX_LINE_LENGTH];
    RecId recId;
    FILE *dataFile;
    int roll_no;
    clock_t start, end;
    double cpu_time_used;
    
    printf("--- AM Layer Indexing Performance Test ---\n");
    printf("WARNING: This test may take a few minutes.\n");

    // =================================================================
    printf("\n[Method 1: Indexing an Existing File]\n");
    // =================================================================

    // 1. Setup: Create the heap file
    printf("  1. Creating heap file from %s...\n", STUDENT_DATA_FILE);
    PF_Init(20, 0); // Init PF layer (20 buf, LRU)
    check_error(HF_CreateFile(HEAP_FILE_NAME), "Create heap file");
    hfFd = PF_OpenFile(HEAP_FILE_NAME); // Use PF_OpenFile
    dataFile = fopen(STUDENT_DATA_FILE, "r");
    
    while (fgets(lineBuffer, sizeof(lineBuffer), dataFile) != NULL) {
        int length = strlen(lineBuffer);
        if (lineBuffer[length - 1] == '\n') {
            lineBuffer[length - 1] = '\0';
            length--;
        }
        
        // --- NEW BUG FIX ---
        // Check if the line is valid *before* inserting
        if (get_roll_no(lineBuffer) == -1) {
            continue; // Skip header or blank line
        }
        // --- END BUG FIX ---

        check_error(HF_InsertRec(hfFd, lineBuffer, length, &recId), "Insert record");
    }
    fclose(dataFile);
    check_error(HF_CloseFile(hfFd), "Close heap file");
    printf("     ...heap file created successfully.\n");

    // 2. Test: Build the index on the existing file
    printf("  2. Building index...\n");
    PF_Init(20, 0); // Reset stats
    hfFd = PF_OpenFile(HEAP_FILE_NAME); // Use PF_OpenFile
    
    check_error(AM_CreateIndex(HEAP_FILE_NAME, INDEX_NO, ATTR_TYPE, ATTR_LENGTH), "Create index");
    amFd = PF_OpenFile(INDEX_FILE_NAME); // Use PF_OpenFile

    // Start timer
    start = clock();

    scanFd = HF_OpenScan(hfFd);
    // This loop reads back clean data, so it's already correct.
    while (HF_FindNextRec(scanFd, lineBuffer, &recId) == HFE_OK) {
        roll_no = get_roll_no(lineBuffer);
        check_error(AM_InsertEntry(amFd, ATTR_TYPE, ATTR_LENGTH, (char *)&roll_no, recId), "Insert index entry");
    }
    HF_CloseScan(scanFd);
    
    // Stop timer
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("     ...index built.\n");
    printf("  3. Results:\n");
    printf("     Time Taken: %.4f seconds\n", cpu_time_used);
    printf("     PF Layer Statistics (for Index Build only):\n");
    PF_PrintStats(); // Print the stats!

    // Cleanup for Method 1
    PF_CloseFile(amFd); // Use PF_CloseFile
    HF_CloseFile(hfFd);
    check_error(PF_DestroyFile(HEAP_FILE_NAME), "Destroy heap file");
    check_error(PF_DestroyFile(INDEX_FILE_NAME), "Destroy index file");


    // =================================================================
    printf("\n[Method 2: Incremental Build (Unsorted)]\n");
    // =================================================================

    printf("  1. Building heap file and index incrementally...\n");
    PF_Init(20, 0); // Reset stats
    
    check_error(HF_CreateFile(HEAP_FILE_NAME), "Create heap file");
    check_error(AM_CreateIndex(HEAP_FILE_NAME, INDEX_NO, ATTR_TYPE, ATTR_LENGTH), "Create index");
    
    hfFd = PF_OpenFile(HEAP_FILE_NAME); // Use PF_OpenFile
    amFd = PF_OpenFile(INDEX_FILE_NAME); // Use PF_OpenFile
    dataFile = fopen(STUDENT_DATA_FILE, "r");

    // Start timer
    start = clock();

    while (fgets(lineBuffer, sizeof(lineBuffer), dataFile) != NULL) {
        int length = strlen(lineBuffer);
        if (lineBuffer[length - 1] == '\n') {
            lineBuffer[length - 1] = '\0';
            length--;
        }
        
        // --- NEW BUG FIX ---
        // 1. Try to get the roll number first.
        roll_no = get_roll_no(lineBuffer);

        // 2. If it's -1, it's a header or blank line. Skip it.
        if (roll_no == -1) {
            continue; // Skip this line
        }
        // --- END NEW BUG FIX ---
        
        check_error(HF_InsertRec(hfFd, lineBuffer, length, &recId), "Insert record");
        check_error(AM_InsertEntry(amFd, ATTR_TYPE, ATTR_LENGTH, (char *)&roll_no, recId), "Insert index entry");
    }

    // Stop timer
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("     ...file and index built.\n");
    printf("  2. Results:\n");
    printf("     Time Taken: %.4f seconds\n", cpu_time_used);
    printf("     PF Layer Statistics (for entire process):\n");
    PF_PrintStats(); // Print the stats!

    // Cleanup for Method 2
    fclose(dataFile);
    PF_CloseFile(amFd); // Use PF_CloseFile
    HF_CloseFile(hfFd);
    check_error(PF_DestroyFile(HEAP_FILE_NAME), "Destroy heap file");
    check_error(PF_DestroyFile(INDEX_FILE_NAME), "Destroy index file");


    // =================================================================
    printf("\n[Method 3: Efficient Bulk-Load (Sorted)]\n");
    // =================================================================

    printf("  1. Building heap file and index from %s...\n", STUDENT_SORTED_FILE);
    PF_Init(20, 0); // Reset stats
    
    check_error(HF_CreateFile(HEAP_FILE_NAME), "Create heap file");
    check_error(AM_CreateIndex(HEAP_FILE_NAME, INDEX_NO, ATTR_TYPE, ATTR_LENGTH), "Create index");
    
    hfFd = PF_OpenFile(HEAP_FILE_NAME); // Use PF_OpenFile
    amFd = PF_OpenFile(INDEX_FILE_NAME); // Use PF_OpenFile
    dataFile = fopen(STUDENT_SORTED_FILE, "r");
    if (dataFile == NULL) {
        fprintf(stderr, "Error: Could not open sorted file '%s'.\n", STUDENT_SORTED_FILE);
        fprintf(stderr, "Please run the 'sort' command first.\n");
        exit(1);
    }

    // Start timer
    start = clock();
    
    while (fgets(lineBuffer, sizeof(lineBuffer), dataFile) != NULL) {
        int length = strlen(lineBuffer);
        if (lineBuffer[length - 1] == '\n') {
            lineBuffer[length - 1] = '\0';
            length--;
        }
        
        // --- NEW BUG FIX ---
        // 1. Try to get the roll number first.
        roll_no = get_roll_no(lineBuffer);

        // 2. If it's -1, it's a header or blank line. Skip it.
        if (roll_no == -1) {
            continue; // Skip this line
        }
        // --- END NEW BUG FIX ---
        
        check_error(HF_InsertRec(hfFd, lineBuffer, length, &recId), "Insert record");
        check_error(AM_InsertEntry(amFd, ATTR_TYPE, ATTR_LENGTH, (char *)&roll_no, recId), "Insert index entry");
    }

    // Stop timer
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("     ...file and index built.\n");
    printf("  2. Results:\n");
    printf("     Time Taken: %.4f seconds\n", cpu_time_used);
    printf("     PF Layer Statistics (for entire process):\n");
    PF_PrintStats(); // Print the stats!

    // Cleanup for Method 3
    fclose(dataFile);
    PF_CloseFile(amFd); // Use PF_CloseFile
    HF_CloseFile(hfFd);
    check_error(PF_DestroyFile(HEAP_FILE_NAME), "Destroy heap file");
    check_error(PF_DestroyFile(INDEX_FILE_NAME), "Destroy index file");
    
    
    printf("\n--- Test Complete ---\n");
    return 0;
}