/* reperr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       reperr.c - error reporting for PipeDream
 * History:
 *  0.01    20-Feb-89   SKS derived from scdraw.c
 *  0.02    01-Mar-89   SKS added reperr_null, all reperr() return FALSE to
 *                          save space in calling functions.
 *  0.03    31-Mar-89   SKS moved rep_ferr() here
 *  0.04    04-May-89   MC  created module tables; restructure of module
 *                          error reporting to return unique numbers
 *  0.05    24-May-89   SKS created reperr_fatal()
 *  0.06    05-Jul-89   SKS made reperr use sprintf
*/

#include "datafmt.h"


#if RISCOS
#include "werr.h"

#include "ext.riscos"
#endif


/* exported functions */

extern BOOL reperr(intl errornumber, const char *text);
extern char *reperr_getstr(intl errornumber);
extern BOOL reperr_module(intl module, intl errornumber);
extern BOOL reperr_null(intl errornumber);
extern BOOL reperr_not_installed(intl errornumber);
extern BOOL rep_fserr(const char *str);


/* ----------------------------------------------------------------------- */

static char *errortab[] =
    {
    "No error",
    "Line too long",
    "Bad name",
    "Memory full",
    "File not found",
    "Bad selection",
    "Bad parameter",
    "Editing expression",
    "Cannot open file",
    "Cannot read file",
    "Cannot write to file",
    "Cannot close file",
    "Not installed for EGA/VGA",
    "Bad ^ field",
    "No list file",
    "Loop",
    "End of list",
    "Cannot buffer file ",
    "Bad option",
    "No marked block",
    "Escape",
    "Bad column",
    "Oldest position lost",
    "Z88:",
    "Bad expression",
    "Bad marker",
    "No driver loaded",
    "Driver does not support microspacing",
    "General failure",
    "Overlap",
    "Control characters ignored",
    "Cannot insert this file format",
    "Lotus:",
    "No pages",
    "Spell:",
    "Word exists",
    "Printer:",
    "Not tab file:",
    "Lines split:",
    "No room: column recalculation forced",
    "Bad string",
    "Bad slot",
    "Bad range",
    "ViewSheet:",
    "Protected slot",
    "Awaiting recalculation",
    "Bad Draw file",
    "Bad font size",
    "Bad line spacing",
    "'%s' is a directory",
    "Bad Draw file scale factor",
    "Printer in use",
    "No RISC OS printer driver",
    "Font manager:",
    "Already dumping",
    "Already merging",
    "Already doing anagrams",
    "Already doing subgrams",
    "Unable to install PipeDream",
    "Draw files must be saved before importing",
    "Memory full: unable to create dialogue box",
    "Not enough memory, or not in *Desktop world",
    "Bad print scale",
    "No marked block in this document",
    "Memory full: unable to store block to paste list",
    "Memory full: unable to recover block from paste list",
    "Cannot save a file from PipeDream into itself"
    };


/*
spelling checker error table
*/

#if !defined(SPELL_OFF)
static char *spell_errlist[] =
    {
    "Too many dictionaries open",
    "File already exists",
    "Filing system error",
    "Can't open",
    "Bad dictionary",
    "Out of memory",
    "Bad word",
    "Dictionary read only",
    "Can't close",
    "Not implemented",
    "Word not found",
    "Escape",
    "Can't update dictionary",
    "Can't read dictionary",
    "Can't enlarge dictionary",
    };
#else
#   define spell_errlist BAD_POINTER
#endif


/*
evaluator errors
*/

static char *eval_errlist[] =
    {
    "Irr",
    "Too many files",
    "Bad logarithm",
    "Bad date",
    "Lookup",
    "Mixed types",
    "-ve root",
    "Propagated",
    "Divide by zero",
    "Bad index",
    "Stack overflow",
    "FP error",
    "Circular reference",
    "File not loaded",
    "More than 1 file loaded",
    };

/*
Z88 error strings
*/

#if !defined(Z88_OFF)
static char *Z88_errlist[] =
    {
    "EOF",
    "Bad link with Z88",
    "Z88 not initialised",
    "Not enough memory to access Z88",
    "Too many Z88 directory entries",
    "ARGS",
    "OPEN",
    "No room",
    };
#else
#   define  Z88_errlist BAD_POINTER
#endif


#if !defined(LOTUS_OFF)
static char *pd123_errlist[] =
    {
    "Error reading or writing file",
    "Out of memory",
    "Expression not completely converted",
    "Bad input file",
    "File has too many rows or columns",
    };
#else
#   define pd123_errlist BAD_POINTER
#endif


#if !defined(VIEWSHEET_OFF)
static char *sheet_errlist[] = 
    {
    "Can't read file",
    "Out of memory",
    };
#else
#   define sheet_errlist BAD_POINTER
#endif


/*
table of module error tables
*/

static char **module_table[] =
    {
    spell_errlist,
    eval_errlist,
    Z88_errlist,
    pd123_errlist,
    sheet_errlist,
    };


/************************************************************
*                                                           *
*  reperr() - report error, doesn't yet do system messages  *
*                                                           *
*  --out--                                                  *
*   error reported, been_error set TRUE                     *
*                                                           *
************************************************************/

extern BOOL
reperr(intl errornumber, const char *text)
{
    char array[LIN_BUFSIZ+1];
    char *errorp;

    if(!text)
        text = NULLSTR;

    #if MS || ARTHUR
    ack_esc();
    #endif

    keyidx = NULL;
    cbuff_offset = 0;

    if(activity_indicator)
        actind_end();

    if(allow_output  &&  !been_error)
        {
        errorp = reperr_getstr(errornumber);

        #if RISCOS
        if(!strstr(errorp, "%s"))
            {
            strcpy(array, errorp);
            strcat(array, " %s");
            errorp = array;
            }

        werr(errorp, text);
        #elif MS || ARTHUR
        clearkeyboardbuffer();

        if(!strstr(errorp, "%s"))
            {
            strcpy(array, errorp);
            strcat(array, " ");
            strcat(array, text);
            }
        else
            sprintf(array, errorp, text);

        d_error[0].tag = array;
        dialog_box(D_ERROR);

        clearkeyboardbuffer();
        #endif
        }

    been_error = TRUE;

    return(FALSE);
}


/*********************************
*                                *
* return string for error number *
*                                *
*********************************/

extern char *
reperr_getstr(intl errornumber)
{
    /* select error from module table */
    if( errornumber > 0)
        errornumber = 0;

    if(errornumber <= MAIN_ERR_END)
        {
        char **error_table;

        error_table = module_table[(-errornumber / MODULE_INCREMENT) - 1];
        return(error_table[-errornumber % MODULE_INCREMENT]);
        }
    else
        return(errortab[-errornumber]);
}


/***********************
*                      *
* module error handler *
*                      *
***********************/

extern BOOL
reperr_module(intl module, intl errornumber)
{
    tracef2("reperr_module(%d, %d)\n", module, errornumber);

    return(reperr(module, reperr_getstr(errornumber)));
}


/****************************
*                           *
*  reperr with no 2nd parm  *
*                           *
****************************/

extern BOOL
reperr_null(intl errornumber)
{
    return(reperr(errornumber, NULL));
}


/************************
*                       *
* 'Not installed' error *
*                       *
************************/

extern BOOL
reperr_not_installed(intl errornumber)
{
    return(reperr(errornumber, "not installed"));
}


#if ARTHUR || RISCOS

extern BOOL
rep_fserr(const char *str)
{
    if(allow_output  &&  !been_error)
        {
        #if RISCOS
        werr("%s", str);
        #else
        char array[LIN_BUFSIZ+1];

        clearkeyboardbuffer();

        strncpy(array, str, sizeof(array));

        d_fserr[0].tag = array;
        dialog_box(D_FSERR);

        clearkeyboardbuffer();
        #endif
        }

    been_error = TRUE;

    return(FALSE);
}

#endif


#if MS

extern void
reperr_fatal(const char *format, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, format);
    vsprintf(buffer, format, args);
    #if !defined(VA_END_SUPERFLUOUS)
    va_end(args);
    #endif

    reperr(ERR_GENFAIL, buffer);

    exit(EXIT_FAILURE);
}

#endif  /* MS */

/* end of reperr.c */
