/*
 * test_hf.c
 *
 * This program tests the HF layer by loading a text file (student.txt)
 * into a heap file and then calculating storage utilization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pf.h" // Must include for PF_PAGE_SIZE
#include "hf.h" // Your new HF layer header

#define STUDENT_DATA_FILE "../data/student.txt" // <-- !!! CHECK THIS FILENAME !!!
#define HEAP_FILE_NAME    "student.hf"
#define MAX_LINE_LENGTH   255 // Max length of one line in student.txt

/*
 * Helper function to check PF/HF errors
 */
void check_error(int error_code, const char *message) {
    if (error_code != HFE_OK && error_code != PFE_OK) {
        printf("Error: %s (code: %d)\n", message, error_code);
        // You might want to add an HF_PrintError function later
        exit(1);
    }
}

int main() {
    FILE *dataFile;
    char lineBuffer[MAX_LINE_LENGTH];
    int hfFd; // Heap File descriptor
    RecId recId;

    // Statistics counters
    long totalBytesInserted = 0;
    int  numRecordsInserted = 0;
    int  totalPagesUsed = 0;

    printf("--- HF Layer Test Utility ---\n");

    // 1. Initialize the PF layer (with your settings from Obj 1)
    // We'll use 20 buffers and LRU for this test.
    PF_Init(20, 0); // 20 buffers, LRU strategy

    // 2. Open the student.txt data file
    dataFile = fopen(STUDENT_DATA_FILE, "r");
    if (dataFile == NULL) {
        fprintf(stderr, "Error: Could not open data file '%s'.\n", STUDENT_DATA_FILE);
        fprintf(stderr, "Please check the path and file name.\n");
        exit(1);
    }

    // 3. Create and Open the new Heap File
    check_error(HF_CreateFile(HEAP_FILE_NAME), "Creating heap file");
    hfFd = HF_OpenFile(HEAP_FILE_NAME);
    if (hfFd < 0) {
        check_error(hfFd, "Opening heap file");
    }

    printf("Loading data from '%s' into '%s'...\n", STUDENT_DATA_FILE, HEAP_FILE_NAME);

    // 4. Read data file line by line and insert into heap file
    while (fgets(lineBuffer, sizeof(lineBuffer), dataFile) != NULL) {
        // Remove the newline character from fgets
        int length = strlen(lineBuffer);
        if (lineBuffer[length - 1] == '\n') {
            lineBuffer[length - 1] = '\0';
            length--;
        }

        // Insert the record
        check_error(HF_InsertRec(hfFd, lineBuffer, length, &recId), "Inserting record");
        
        // Update our statistics
        totalBytesInserted += length;
        numRecordsInserted++;
        
        // Keep track of the highest page number
        if (recId.pageNum + 1 > totalPagesUsed) {
            totalPagesUsed = recId.pageNum + 1;
        }
    }

    printf("Data loading complete.\n");
    printf("---------------------------\n");
    printf("Total Records Inserted: %d\n", numRecordsInserted);
    printf("Total Bytes Inserted:   %ld\n", totalBytesInserted);
    printf("Total Pages Used:       %d\n", totalPagesUsed);
    printf("---------------------------\n");

    // 5. Close files
    fclose(dataFile);
    check_error(HF_CloseFile(hfFd), "Closing heap file");

    // 6. Calculate and Print Utilization Statistics
    
    // Slotted Page Utilization
    long totalSlottedSpace = (long)totalPagesUsed * PF_PAGE_SIZE;
    double slottedUtil = (double)totalBytesInserted / totalSlottedSpace;

    // Static Utilization (for max record lengths 50, 100, 200)
    long staticSpace50 = (long)numRecordsInserted * 50;
    long staticSpace100 = (long)numRecordsInserted * 100;
    long staticSpace200 = (long)numRecordsInserted * 200;

    double staticUtil50 = (double)totalBytesInserted / staticSpace50;
    double staticUtil100 = (double)totalBytesInserted / staticSpace100;
    double staticUtil200 = (double)totalBytesInserted / staticSpace200;

   printf("\n==================== STORAGE UTILIZATION TABLE ====================\n");
printf("%-30s | %-20s | %-15s\n", "Metric", "Calculation", "Utilization (%)");
printf("--------------------------------------------------------------------------\n");

printf("%-30s | %ld / (%d * %d)       | %-15.2f\n",
       "Slotted Page Utilization",
       totalBytesInserted, totalPagesUsed, PF_PAGE_SIZE,
       slottedUtil * 100.0);

printf("%-30s | %ld / (%d * 50)       | %-15.2f\n",
       "Static Util (50 bytes)",
       totalBytesInserted, numRecordsInserted,
       staticUtil50 * 100.0);

printf("%-30s | %ld / (%d * 100)      | %-15.2f\n",
       "Static Util (100 bytes)",
       totalBytesInserted, numRecordsInserted,
       staticUtil100 * 100.0);

printf("%-30s | %ld / (%d * 200)      | %-15.2f\n",
       "Static Util (200 bytes)",
       totalBytesInserted, numRecordsInserted,
       staticUtil200 * 100.0);

printf("==========================================================================\n");


    // 7. Clean up the created heap file
    PF_DestroyFile(HEAP_FILE_NAME);
    
    return 0;
}