/* xferrecv.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __xferrecv_h
#define __xferrecv_h
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
int xferrecv_checkinsert(char **filename);
void xferrecv_insertfileok(void);
int xferrecv_checkprint(char **filename);
void xferrecv_printfileok(int type);
int xferrecv_checkimport(int *estsize);
 
typedef BOOL (*xferrecv_buffer_processor)(char **buffer, int *size);
int xferrecv_doimport(char *buf, int size, xferrecv_buffer_processor);
BOOL xferrecv_file_is_safe(void);
#endif
