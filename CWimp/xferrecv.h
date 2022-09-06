/* Title: -> h.xferrecv
 * Purpose: general purpose import of data by dragging icon
 * Requires:
 *  BOOL
 */

/****************************************************************************
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

int xferrecv_checkinsert(char **filename);
/* Is the current event (uses wimpt) an invitation to insert a file? If so
   return the file type and write a pointer to the filename into *filename. If
   not return -1. */

void xferrecv_insertfileok(void);
/* An insert has been completed successfully. This sends an acknowledge back
   to the original application. */

int xferrecv_checkprint(char **filename);
/* Is the current event (uses wimpt) an invitation to print a file? If so
   return the file type and write a pointer to the filename into *filename. If
   not return -1. The application can print the file directly, or convert it
   into <Printer$Temp> for subsequent dumping by the printer application */

void xferrecv_printfileok(int type);
/* A print has been completed successfully. This sends an acknowledge back
   to the printer application; if printing was done to <Printer$Temp>, then
   indicate the file type of the resulting file. */

int xferrecv_checkimport(int *estsize);
/* Is the current event (uses wimpt) an invitation to receive a data send
   operation? If so return the file type, if not return -1. Estimated size is
   also returned. */

typedef BOOL (*xferrecv_buffer_processor)(char **buffer, int *size);
/* Called to process a filled buffer; must empty the current buffer, or
   allocate more space and update the buffer pointer and size, or return FALSE.
   *buffer, *size on entry are the current buffer indicators. */

int xferrecv_doimport(char *buf, int size, xferrecv_buffer_processor);
/* Receives data into the buffer; calls the buffer processor if the buffer
   given becomes full. Returns no of bytes in final transfer if the transaction
   completed sucessfully; -1 otherwise. In particular, if the import actually
   happens via a scrap file then the buffer processor will not be called and
   -1 will be returned - the next message you receive will be an insert
   request, xferrecv will handle deleting the scrap file etc. automatically. */

BOOL xferrecv_file_is_safe(void) ;
/* Returns TRUE if the file was received from a safe source; i.e. the
   filename will validly refer to the file for the foreseeable future */

/* end */
