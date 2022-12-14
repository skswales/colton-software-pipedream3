/* xfersend.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __xfersend_h
#define __xfersend_h
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
#ifndef __wimp_h
#include "wimp.h"
#endif
 
typedef BOOL (*xfersend_saveproc)(char *filename, void *handle);
 
typedef BOOL (*xfersend_sendproc)(void *handle, int *maxbuf);
typedef int (*xfersend_printproc)(char *filename, void *handle);
#define xfersend_printPrinted -1 
#define xfersend_printFailed -2 
 
BOOL xfersend(int filetype, char *name, int estsize,
 xfersend_saveproc, xfersend_sendproc, xfersend_printproc,
 wimp_eventstr *e, void *handle);
 
BOOL xfersend_pipe(int filetype, char *name, int estsize,
 xfersend_saveproc, xfersend_sendproc, xfersend_printproc,
 void *handle, wimp_t task);
BOOL xfersend_sendbuf(char *buffer, int size);
BOOL xfersend_file_is_safe(void) ;
void xfersend_set_fileissafe(BOOL value);
void xfersend_close_on_xfer(BOOL do_we_close, wimp_w w);
void xfersend_clear_unknowns(void);
#endif
