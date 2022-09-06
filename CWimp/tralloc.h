/* tralloc.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       tralloc.h - allocation tracing
 * Author:      Stuart K. Swales 09-May-1989
*/

#ifndef __wimplib__tralloc_h
#define __wimplib__tralloc_h

/* exported functions */

extern void  *tralloc_calloc(size_t num, size_t size);
extern void   tralloc_dispose(void **v);
extern void   tralloc_free(void *a);
extern void   tralloc_init(void);
extern void  *tralloc_malloc(size_t size);
extern void  *tralloc_realloc(void *a, size_t size);
extern size_t tralloc_size(void *size);

#if defined(WATCH_ALLOCS)
/* Redirect allocation functions to traceable entries */
#   undef   calloc
#   define  calloc  tralloc_calloc
#   undef   dispose
#   define  dispose tralloc_dispose
#   undef   free
#   define  free    tralloc_free
#   undef   malloc
#   define  malloc  tralloc_malloc
#   undef   realloc
#   define  realloc tralloc_realloc
#endif

#endif  /* __wimplib__tralloc_h */

/* end of tralloc.h */
