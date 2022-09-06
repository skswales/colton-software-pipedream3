/* magnify.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __magnify_h
#define __magnify_h
 
void magnify_select (int *mul, int *div, int maxmul, int maxdiv,
 void (*proc)(void *), void *phandle) ;
#endif
