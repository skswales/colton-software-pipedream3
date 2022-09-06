/* > h.xfersend */

/* Title:   xfersend.h
 * Purpose: general purpose export of data by dragging icon
 * Requires:
 *  dbox.h
 *  BOOL
*/

/***************************************************************************
* This source file was written by Acorn Computers Limited. It is part of   *
* the "cwimp" library for writing applications in C for RISC OS. It may be *
* used freely in the creation of programs for Archimedes. It should be     *
* used with Acorn's C Compiler Release 2 or later.                         *
*                                                                          *
* No support can be given to programmers using this code and, while we     *
* believe that it is correct, no correspondence can be entered into        *
* concerning behaviour or bugs.                                            *
*                                                                          *
* Upgrades of this code may or may not appear, and while every effort will *
* be made to keep such upgrades upwards compatible, no guarantees can be   *
* given.                                                                   *
***************************************************************************/


typedef BOOL (*xfersend_saveproc)(const char *filename, void *handle);
/* Should return TRUE if save was successful,
 * else should werr to report the error and return FALSE.
*/


typedef BOOL (*xfersend_sendproc)(void *handle, int *maxbuf);
/* Send the file in chunks no bigger than *maxbuf, by calling
 * xfersend_sendbuf (see below). Calls to xfersend_sendbuf could change the
 * value of *maxbuf, but this will always be > 0.
*/


typedef int (*xfersend_printproc)(const char *filename, void *handle);
/* The file has been dropped on a printer: either print the file directly, or
 * save it into the given filename, from where it will be printed by the
 * printer application. Return the filetype of the saved file, or one of
 * the following reason codes:
*/

#define xfersend_printPrinted   -1      /* file dealt with internally */
#define xfersend_printFailed    -2      /* had an error along the way */

/* The print saveproc should report any errors it encounters itself.
 * If saving to a file, it should convert the data into a type that can be
 * printed by the printer application (i.e. text).
*/


typedef void (*xfersend_clickproc)(dbox d, dbox_field f, int *filetypep, void *handle);
/* An unknown field has been pressed in the passed dbox:
 * you may change the filetype too.
*/


extern BOOL xfersend(
                int filetype, const char *name, int estsize,
                xfersend_saveproc, void *savehandle,
                xfersend_sendproc, xfersend_printproc,
                dbox d,
                xfersend_clickproc, void *clickhandle);
/* Offer the specified data for export with the given filetype, suggesting
 * the given filename. Try to estimate the size of the file, not essential but
 * may help performance of pipes. Returns TRUE if the data was indeed exported
 * successfully. 0 may be passed instead of the sendproc, in which case the
 * in-core memory transfer cannot occur; 0 may be passed instead of the
 * printproc, which implies printing uses the same format as saveing.
 * A dbox may be passed, and should have the following icons:
*/

#define xfersend_FOK    0       /* (action) 'ok' button */
#define xfersend_FName  1       /* (writeable) filename */
#define xfersend_FIcon  2       /* (click/drag) icon to drag */


extern BOOL xfersend_sendbuf(char *buffer, int size);
/* Called by an xfersend_sendproc to empty the buffer, with n being the
 * number of characters placed in the buffer (>= 0). Returns TRUE if
 * successful. If it returns FALSE (e.g. protocol broken) the
 * xfersend_sendproc must also (immediately) return FALSE.
*/

extern BOOL xfersend_file_is_safe(void);
/* Returns TRUE if file recipient will not modify it; changing the
 * window title of the file can be done conditionally on this result.
 * This can be called within your xfersend_saveproc,sendproc, or printproc,
 * or immediately after the main xfersend.
*/

extern void xfersend_read_leafname_during_send(char *name /*out*/, int length);
/* read the leafname currently set during a ram transfer */


/* end of xfersend.h */
