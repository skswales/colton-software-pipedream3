/* dboxtcol.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __dboxtcol_h
#define __dboxtcol_h
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef int dboxtcol_colour ; 
typedef void (*dboxtcol_colourhandler)(dboxtcol_colour col, void *handle);
#define dboxtcol_Transparent (-1)
BOOL dboxtcol(dboxtcol_colour *colour , BOOL allow_transparent,
 char *name, dboxtcol_colourhandler proc, void *handle) ;
#endif
