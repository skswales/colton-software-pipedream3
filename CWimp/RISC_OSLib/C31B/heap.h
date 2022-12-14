/* heap.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __heap_h
#define __heap_h
#ifndef __os_h
#include "os.h"
#endif
void heap_init(BOOL heap_shrink);
void *heap_alloc(unsigned int size);
void heap_free(void *heapptr);
#endif
