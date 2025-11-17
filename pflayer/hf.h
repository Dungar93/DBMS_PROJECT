/*
 * hf.h: Header file for the Heap File (HF) layer.
 */

#ifndef HF_H
#define HF_H

#include "pf.h" // We are building on top of the PF layer

/* --- Data Structures --- */

/**
 * RecId: Record Identifier
 * This struct is used to uniquely identify a record in a heap file.
 * It consists of the page number and the slot number within that page.
 */
typedef struct {
    int pageNum;
    int slotNum;
} RecId;


/**
 * HF_PageHeader: Slotted Page Header
 * This structure is stored at the very beginning of every heap file page.
 */
typedef struct {
    int numSlots;           // Number of slots currently used on this page
    int freeSpaceOffset;    // Byte offset (from end of page) where free space begins
    int nextPage;           // Page number of the next page with free space (NOT USED in this simple version)
} HF_PageHeader;


/**
 * HF_Slot: Slot Directory Entry
 * An array of these slots (the "slot directory") grows from the
 * top of the page, right after the HF_PageHeader.
 */
typedef struct {
    int recordOffset;  // Byte offset (from end of page) where the record data starts
    int recordLength;  // Length of the record in bytes
} HF_Slot;


/* --- Error Codes --- */

// Define error codes for the HF layer, starting from a base offset
#define HFE_OK          0
#define HFE_PF         -20  // Error from PF layer
#define HFE_EOF        -21  // End of file reached
#define HFE_PAGEFULL   -22  // No space on page
#define HFE_INVALIDREC -23  // Invalid record
#define HFE_SCANOPEN   -24  // Scan is already open
#define HFE_SCANCLOSED -25  // Scan is closed or invalid
#define HFE_SCANEOF    -26  // End of scan

/* --- Function Prototypes (The HF API) --- */

/**
 * Creates a new heap file named 'fileName'.
 * Calls PF_CreateFile().
 * Returns HFE_OK on success, or a PF error code.
 */
int HF_CreateFile(char *fileName);


/**
 * Opens an existing heap file named 'fileName'.
 * Calls PF_OpenFile().
 * Returns a file descriptor (fd) >= 0 on success, or a PF error code.
 */
int HF_OpenFile(char *fileName);


/**
 * Closes a heap file identified by 'fileDesc'.
 * Calls PF_CloseFile().
 * Returns HFE_OK on success, or a PF error code.
 */
int HF_CloseFile(int fileDesc);


/**
 * Inserts a record into the heap file.
 *
 * @param fileDesc  File descriptor for the open heap file.
 * @param record    Pointer to the record data to be inserted.
 * @param length    Length of the record data in bytes.
 * @param recId     (Output) Pointer to a RecId struct where the new
 * record's ID will be stored.
 * @return HFE_OK on success, or an error code.
 */
int HF_InsertRec(int fileDesc, char *record, int length, RecId *recId);


/**
 * Deletes a record, identified by 'recId', from the heap file.
 * Deletes by setting the slot's recordLength to -1 (tombstone).
 *
 * @param fileDesc  File descriptor for the open heap file.
 * @param recId     The ID of the record to be deleted.
 * @return HFE_OK on success, or an error code.
 */
int HF_DeleteRec(int fileDesc, RecId recId);


/**
 * Initializes a scan of all records in the heap file.
 *
 * @param fileDesc  File descriptor for the open heap file.
 * @return A scan descriptor (scanDesc) >= 0 on success, or an error code.
 */
int HF_OpenScan(int fileDesc);


/**
 * Retrieves the next valid record from an open scan.
 *
 * @param scanDesc  The scan descriptor from HF_OpenScan.
 * @param record    (Output) A buffer where the record data will be copied.
 * @param recId     (Output) The RecId of the record being returned.
 * @return HFE_OK on success, HFE_SCANEOF if no more records, or an error code.
 */
int HF_FindNextRec(int scanDesc, char *record, RecId *recId);


/**
 * Closes a scan.
 *
 * @param scanDesc  The scan descriptor from HF_OpenScan.
 * @return HFE_OK on success, or an error code.
 */
int HF_CloseScan(int scanDesc);


#endif // HF_H