/*
 * am_internal.h
 *
 * This file contains the INTERNAL definitions for the AM layer.
 * It is only included by .c files inside the amlayer.
 */

#ifndef AM_INTERNAL_H
#define AM_INTERNAL_H

/* --- Page Header Structs (from your old am.h) --- */

typedef struct am_leafheader
{
    char pageType;
    int nextLeafPage;
    short recIdPtr;
    short keyPtr;
    short freeListPtr;
    short numinfreeList;
    short attrLength;
    short numKeys;
    short maxKeys;
}   AM_LEAFHEADER; /* Header for a leaf page */

typedef struct am_intheader 
{
    char pageType;
    short numKeys;
    short maxKeys;
    short attrLength;
}   AM_INTHEADER ; /* Header for an internal node */


/* --- Constants and Macros (from your old am.h) --- */

/* Note: AM_Errno is declared in am.h */
# define AM_Check if (errVal != PFE_OK) {AM_Errno = AME_PF; return(AME_PF) ;}
# define AM_si sizeof(int)
# define AM_ss sizeof(short)
# define AM_sl sizeof(AM_LEAFHEADER)
# define AM_sint sizeof(AM_INTHEADER)
# define AM_sc sizeof(char)
# define AM_sf sizeof(float)

# define AM_NULL 0 /* Null pointer for lists in a page */
# define AM_MAX_FNAME_LENGTH 80
# define AM_NULL_PAGE -1 
# define FREE 0 
# define FIRST 1 
# define BUSY 2
# define LAST 3
# define OVER 4
# define ALL 0
# define EQUAL 1
# define LESS_THAN 2
# define GREATER_THAN 3
# define LESS_THAN_EQUAL 4
# define GREATER_THAN_EQUAL 5
# define NOT_EQUAL 6
# define AM_MAXATTRLENGTH 256

#endif // AM_INTERNAL_H