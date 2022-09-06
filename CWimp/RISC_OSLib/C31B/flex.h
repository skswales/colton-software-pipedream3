/* flex.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __flex_h
#define __flex_h
typedef void **flex_ptr;
int flex_alloc(flex_ptr anchor, int n);
void flex_free(flex_ptr anchor);
int flex_size(flex_ptr);
int flex_extend(flex_ptr, int newsize);
int flex_midextend(flex_ptr, int at, int by);
void flex_init(void);
#endif
