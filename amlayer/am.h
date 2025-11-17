/*
 * am.h: Header file for the AM layer.
 * (This is a corrected and completed version)
 */

#ifndef AM_H
#define AM_H

/* --- Error Codes (from your original file) --- */
#define AME_OK 0
#define AME_INVALIDATTRLENGTH -1
#define AME_NOTFOUND -2
#define AME_PF -3
#define AME_INTERROR -4
#define AME_INVALID_SCANDESC -5
#define AME_INVALID_OP_TO_SCAN -6
#define AME_EOF -7
#define AME_SCAN_TAB_FULL -8
#define AME_INVALIDATTRTYPE -9
#define AME_FD -10
#define AME_INVALIDVALUE -11

/* --- Constants (from your original file) --- */
#define AM_NOT_FOUND 0 /* Key is not in tree */
#define AM_FOUND 1     /* Key is in tree */
#define MAXSCANS 20

/*
 * NOTE: Your RecId is defined in hf.h, which am.h does not include.
 * test_am.c is fine because it includes *both* hf.h and am.h.
 * For the AM layer itself, it just treats RecId as an 'int'.
 */
#ifndef HF_H
typedef int RecId;
#endif

/* --- AM Layer Function Prototypes (THE MISSING PART) --- */

extern int AM_CreateIndex(char *fileName, int indexNo, char attrType, int attrLength);

extern int AM_DestroyIndex(char *fileName, int indexNo);





extern int AM_InsertEntry(int fd, char attrType, int attrLength, char *value, RecId recId);

extern int AM_DeleteEntry(int fd, char attrType, int attrLength, char *value, RecId recId);

extern int AM_OpenIndexScan(int fd, char attrType, int attrLength, int op, char *value);

extern int AM_FindNextEntry(int scanDesc);

extern int AM_CloseIndexScan(int scanDesc);

/* * These were in your file as globals. 
 * They should be defined in amglobals.c and declared here.
 */
extern int AM_RootPageNum;
extern int AM_LeftPageNum;
extern int AM_Errno;

#endif // AM_H