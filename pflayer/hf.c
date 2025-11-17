/*
 * hf.c: Implementation of the Heap File (HF) layer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For memcpy

#include "hf.h" // Our own header file

/* --- Internal Scanner Management --- */

#define HF_MAX_SCANS 20 // Max number of concurrent scans

typedef enum {
    SC_FREE,
    SC_OPEN
} HF_ScanStatus;

// This struct holds the state of an active scan
typedef struct {
    HF_ScanStatus status;
    int fileDesc;
    int currentPage;  // Page number of the page currently being scanned
    int currentSlot;  // Next slot to check on the current page
    
    // We need to keep the page buffer fixed during the scan
    char *pageBuffer; // Pointer to the buffer for currentPage
} HF_Scan;

// The global table of open scans
HF_Scan HF_ScanTable[HF_MAX_SCANS];


/*
 * Helper function to initialize the HF_ScanTable.
 * This should be called by the layer above us (AM layer)
 * or we can call it on the first HF_OpenScan.
 */
void HF_Init() {
    for (int i = 0; i < HF_MAX_SCANS; i++) {
        HF_ScanTable[i].status = SC_FREE;
    }
}


/* --- Simple Pass-Through Functions --- */

int HF_CreateFile(char *fileName) {
    // Just call the PF layer function
    int pfErr = PF_CreateFile(fileName);
    return (pfErr == PFE_OK) ? HFE_OK : HFE_PF;
}

int HF_OpenFile(char *fileName) {
    // Just call the PF layer function
    // It returns an fd >= 0 on success, or a PF error code
    return PF_OpenFile(fileName);
}

int HF_CloseFile(int fileDesc) {
    // Just call the PF layer function
    int pfErr = PF_CloseFile(fileDesc);
    return (pfErr == PFE_OK) ? HFE_OK : HFE_PF;
}


/*
 * Helper function to initialize a new page as a slotted page.
 */
void HF_InitPage(char *pageBuffer) {
    HF_PageHeader *header = (HF_PageHeader *)pageBuffer;
    header->numSlots = 0;
    // Free space starts at the end of the 4096-byte page
    header->freeSpaceOffset = PF_PAGE_SIZE; 
}


/* --- Core HF Functions --- */

int HF_InsertRec(int fileDesc, char *record, int length, RecId *recId) {
    int pageNum = -1;
    char *pageBuffer;
    HF_PageHeader *header;
    int pfErr;
    
    int neededSpace = length + sizeof(HF_Slot);

    // 1. Find a page with enough space (linear scan)
    while ((pfErr = PF_GetNextPage(fileDesc, &pageNum, &pageBuffer)) == PFE_OK) {
        header = (HF_PageHeader *)pageBuffer;
        
        // Calculate free space on this page
        int freeSpace = header->freeSpaceOffset - 
                        (sizeof(HF_PageHeader) + (header->numSlots * sizeof(HF_Slot)));
        
        if (freeSpace >= neededSpace) {
            // Found a page! Unfix it (clean) and break the loop.
            // We'll re-Get it as fixed later.
            PF_UnfixPage(fileDesc, pageNum, FALSE);
            break; 
        }

        // No space, unfix this page (clean) and check the next one
        PF_UnfixPage(fileDesc, pageNum, FALSE);
    }

    // 2. Handle loop exit
    if (pfErr == PFE_EOF) {
        // No page with space was found, so allocate a new page
        if ((pfErr = PF_AllocPage(fileDesc, &pageNum, &pageBuffer)) != PFE_OK) {
            return HFE_PF; // Could not allocate a new page
        }
        // Initialize the new page
        HF_InitPage(pageBuffer);
        header = (HF_PageHeader *)pageBuffer;

    } else if (pfErr != PFE_OK) {
        return HFE_PF; // Some other PF error
    
    } else {
        // We found a page (pageNum) in the loop.
        // We must Get it again to fix it in the buffer.
        if ((pfErr = PF_GetThisPage(fileDesc, pageNum, &pageBuffer)) != PFE_OK) {
            return HFE_PF;
        }
        header = (HF_PageHeader *)pageBuffer;
    }

    // 3. Insert the record into the page (pageBuffer)
    
    // Find the next available slot
    HF_Slot *slot = (HF_Slot *)(pageBuffer + sizeof(HF_PageHeader)) + header->numSlots;
    
    // Copy the record data, growing from the end of the page
    int recordOffset = header->freeSpaceOffset - length;
    memcpy(pageBuffer + recordOffset, record, length);

    // Update the slot
    slot->recordOffset = recordOffset;
    slot->recordLength = length;

    // Update the page header
    header->numSlots++;
    header->freeSpaceOffset = recordOffset;

    // 4. Set the output RecId
    recId->pageNum = pageNum;
    recId->slotNum = header->numSlots - 1; // It's the last slot we just added

    // 5. Unfix the page as DIRTY
    if ((pfErr = PF_UnfixPage(fileDesc, pageNum, TRUE)) != PFE_OK) {
        return HFE_PF;
    }
    
    return HFE_OK;
}


int HF_DeleteRec(int fileDesc, RecId recId) {
    char *pageBuffer;
    int pfErr;

    // 1. Get the page
    if ((pfErr = PF_GetThisPage(fileDesc, recId.pageNum, &pageBuffer)) != PFE_OK) {
        return HFE_PF;
    }

    HF_PageHeader *header = (HF_PageHeader *)pageBuffer;

    // 2. Check for invalid slot number
    if (recId.slotNum < 0 || recId.slotNum >= header->numSlots) {
        PF_UnfixPage(fileDesc, recId.pageNum, FALSE); // Unfix clean
        return HFE_INVALIDREC;
    }

    // 3. Find the slot
    HF_Slot *slot = (HF_Slot *)(pageBuffer + sizeof(HF_PageHeader)) + recId.slotNum;

    // 4. "Delete" the record by setting its length to -1 (tombstone)
    // We don't compact the page to save time.
    slot->recordLength = -1; 

    // 5. Unfix the page as DIRTY
    if ((pfErr = PF_UnfixPage(fileDesc, recId.pageNum, TRUE)) != PFE_OK) {
        return HFE_PF;
    }

    return HFE_OK;
}


/* --- Scanner Functions --- */

int HF_OpenScan(int fileDesc) {
    static int scan_init_done = 0;
    if (!scan_init_done) {
        HF_Init();
        scan_init_done = 1;
    }

    // 1. Find a free slot in the scan table
    for (int i = 0; i < HF_MAX_SCANS; i++) {
        if (HF_ScanTable[i].status == SC_FREE) {
            // 2. Initialize the scan state
            HF_ScanTable[i].status = SC_OPEN;
            HF_ScanTable[i].fileDesc = fileDesc;
            HF_ScanTable[i].currentPage = -1; // Start before the first page
            HF_ScanTable[i].currentSlot = -1;
            HF_ScanTable[i].pageBuffer = NULL;
            
            // 3. Return the index as the scan descriptor
            return i;
        }
    }

    // No free scan slots
    return HFE_SCANOPEN;
}


int HF_FindNextRec(int scanDesc, char *record, RecId *recId) {
    // 1. Check for valid scan descriptor
    if (scanDesc < 0 || scanDesc >= HF_MAX_SCANS || 
        HF_ScanTable[scanDesc].status != SC_OPEN) {
        return HFE_SCANCLOSED;
    }

    HF_Scan *scan = &HF_ScanTable[scanDesc];
    HF_PageHeader *header;
    HF_Slot *slot;
    int pfErr;

    while (1) { // Loop until we find a record or hit EOF
        
        // 2. Check if we need to get a new page
        if (scan->currentSlot == -1) {
            
            // 2a. Unfix the *previous* page, if there was one
            if (scan->pageBuffer != NULL) {
                PF_UnfixPage(scan->fileDesc, scan->currentPage, FALSE);
                scan->pageBuffer = NULL;
            }

            // 2b. Get the *next* page in the file
            pfErr = PF_GetNextPage(scan->fileDesc, &(scan->currentPage), &(scan->pageBuffer));
            
            if (pfErr == PFE_EOF) {
                return HFE_SCANEOF; // End of file, no more records
            } else if (pfErr != PFE_OK) {
                return HFE_PF; // Some other PF error
            }

            // 2c. We have a new page, so reset the slot counter
            scan->currentSlot = 0;
        }

        // 3. Check the slots on the current page
        header = (HF_PageHeader *)scan->pageBuffer;
        
        if (scan->currentSlot < header->numSlots) {
            // 3a. Point to the next slot to check
            slot = (HF_Slot *)(scan->pageBuffer + sizeof(HF_PageHeader)) + scan->currentSlot;
            
            // 3b. Increment slot counter for the *next* iteration
            scan->currentSlot++; 

            // 3c. Check if this slot is valid (not deleted)
            if (slot->recordLength != -1) {
                // Found a valid record!
                
                // Copy its data to the output buffer
                memcpy(record, scan->pageBuffer + slot->recordOffset, slot->recordLength);
                
                // Set the output RecId
                recId->pageNum = scan->currentPage;
                recId->slotNum = scan->currentSlot - 1; // (since we already incremented)
                
                // Return success
                return HFE_OK; 
            }
            // If slot was deleted (length == -1), loop continues to check next slot

        } else {
            // 4. We've checked all slots on this page.
            // Set currentSlot to -1 to signal that we need the next page
            scan->currentSlot = -1;
            // Loop continues to the next page
        }
    }
}


int HF_CloseScan(int scanDesc) {
    // 1. Check for valid scan descriptor
    if (scanDesc < 0 || scanDesc >= HF_MAX_SCANS || 
        HF_ScanTable[scanDesc].status != SC_OPEN) {
        return HFE_SCANCLOSED;
    }

    HF_Scan *scan = &HF_ScanTable[scanDesc];

    // 2. Unfix the last page that was being scanned (if any)
    if (scan->pageBuffer != NULL) {
        PF_UnfixPage(scan->fileDesc, scan->currentPage, FALSE);
    }

    // 3. Mark the scan slot as free
    scan->status = SC_FREE;

    return HFE_OK;
}