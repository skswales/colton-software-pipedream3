/* tralloc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       tralloc.c - allocation tracing
 * Author:      Stuart K. Swales 09-May-1989
*/

#define HALT_ON_FAIL TRUE


#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "trace.h"

#if HALT_ON_FAIL
#include "werr.h"
#endif

#if defined(WATCH_ALLOCS)
#   undef   WATCH_ALLOCS
#endif

#include "tralloc.h"

#include "misc.h"


/* exported functions */

extern void  *tralloc_calloc(size_t num, size_t size);
extern void   tralloc_dispose(void **v);
extern void   tralloc_free(void *a);
extern void   tralloc_init(void);
extern void  *tralloc_malloc(size_t size);
extern void  *tralloc_realloc(void *a, size_t size);
extern size_t tralloc_size(void *a);


/* internal functions */

#if defined(OLD_ALLOCS)
static int bytes_start(void);
#endif

#define round(a) ((a + (4-1)) & ~(4-1))


/* ----------------------------------------------------------------------- */

#if defined(KEEP_RECORDS)
static int total_alloc = 0;
#endif


#if !defined(OLD_ALLOCS)

extern int alloc_totalfree(void);

static int
total_free(void)
{
    return(alloc_totalfree());
}


extern int alloc_largestfree(void);

static int
largest_free(void)
{
    return(alloc_largestfree());
}


#else

static int total__free = 0;

static int
total_free(void)
{
    return(total__free);
}


static int
largest_free(void)
{
    return(-1);
}


/**************************************
*                                     *
* estimate how much core is left free *
*                                     *
**************************************/

#define BYTES_BLOCK struct _bytes_block

BYTES_BLOCK
    {
    BYTES_BLOCK *next;
    BYTES_BLOCK *prev;
    };

#define BIGGEST_TRY (8192*256)

static int
bytes_start(void)
{
    int count = 0;
    unsigned int this_try = (unsigned) BIGGEST_TRY;
    BYTES_BLOCK *first = NULL;
    BYTES_BLOCK *thisone = NULL;

    while(this_try >= sizeof(BYTES_BLOCK))
        {
        BYTES_BLOCK *new_one = NULL;

        while(!new_one  &&  (this_try >= sizeof(BYTES_BLOCK)))
            if((new_one = malloc(this_try)) == NULL)
                this_try /= 2;

        if(new_one)
            {
            if(!first)
                {
                first = thisone = new_one;
                first->prev = NULL;
                count = this_try;
                }
            else
                {
                thisone->next = new_one;
                new_one->prev = thisone;
                thisone = new_one;
                count += this_try;
                }
            }
        }

    while(thisone)
        {
        BYTES_BLOCK *oldone = thisone;

        thisone = thisone->prev;
        free(oldone);
        }

    return(count);
}

#endif


/************************************************************************/

#if TRACE
extern void alloc_traversefree(void);
#endif

extern void *
tralloc_calloc(size_t num, size_t size)
{
#if defined(KEEP_RECORDS)
    int tsize = size * num;
#endif
    void *a;

    tracef2("calloc(%d, %d) ", num, size);

#if defined(KEEP_RECORDS)
    a = calloc(1, tsize + sizeof(int));

    if(a)
        {
        *((int *) a) = round(tsize + sizeof(int));
        total_alloc += *((int *) a);
        a = ((int *) a) + 1;
        }
#else
    a = calloc(num, size);
#endif

    tracef3("yields &%p [free: %d %d]",
            a, total_free(), largest_free());
#if TRACE
    alloc_traversefree();
#endif
    tracef0("\n");

    if(HALT_ON_FAIL  &&  !a)
        werr("calloc(%d, %d) yields NULL", num, size);

    return(a);
}


extern void
tralloc_dispose(void **v)
{
    void *a;

    tracef2("dispose(&%p -> &%p) ", v, *v);

    if(v)
        {
        a = *v;

        if(a)
            {
            *v = NULL;
#if defined(KEEP_RECORDS)
            a = ((int *) a) - 1;
            total_alloc -= *((int *) a);
#endif

            free(a);
            tracef2("[free: %d %d]",
                    total_free(), largest_free());
#if TRACE
            alloc_traversefree();
#endif
            }
        }

    tracef0("\n");
}


extern void
tralloc_free(void *a)
{
    tracef2("free(&%p (size %d))", a, tralloc_size(a));

    if(a)
        {
#if defined(KEEP_RECORDS)
        a = ((int *) a) - 1;
        total_alloc -= *((int *) a);
#endif

        free(a);
        tracef2("[free: %d %d]",
                total_free(), largest_free());
#if TRACE
        alloc_traversefree();
#endif
        }

    tracef0("\n");
}


#if defined(USE_BOUND_LIBRARY)  &&  defined(OLD_ALLOCS)
extern void __heap_checking_on_all_allocates(BOOL);
extern void __heap_checking_on_all_deallocates(BOOL);
#endif

extern void
tralloc_init(void)
{
    #if defined(USE_BOUND_LIBRARY)  &&  defined(OLD_ALLOCS)
    __heap_checking_on_all_allocates(TRUE);
    __heap_checking_on_all_deallocates(TRUE);
    #endif

    #if defined(OLD_ALLOCS)
    total__free = bytes_start();
    #endif

    tracef1("tralloc_init() [bytes free: %d]\n", total_free());
}


extern void *
tralloc_malloc(size_t size)
{
    void *a;

    tracef1("malloc(%d) ", size);

#if defined(KEEP_RECORDS)
    if(a)
        {
        *((int *) a) = round(size + sizeof(int));
        total_alloc += *((int *) a);
        a = ((int *) a) + 1;
        }

    a = malloc(size + sizeof(int));
#else
    a = malloc(size);
#endif

    tracef3("yields &%p [free: %d %d]",
            a, total_free(), largest_free());
#if TRACE
    alloc_traversefree();
#endif
    tracef0("\n");

    if(HALT_ON_FAIL  &&  !a)
        werr("malloc(%d) yields NULL", size);

    return(a);
}


extern void *
tralloc_realloc(void *a, size_t size)
{
    tracef2("realloc(&%p, %d) ", a, size);

#if defined(KEEP_RECORDS)
    {
    int old_size;

    if(a)
        {
        a = ((int *) a) - 1;
        old_size = *((int *) a);
        }
    else
        old_size = 0;

    a = realloc(a, size + sizeof(int));

    if(a)
        {
        *((int *) a) = round(size + sizeof(int));
        total_alloc += *((int *) a);
        total_alloc -= old_size;
        a = ((int *) a) + 1;
        }
    }
#else
    a = realloc(a, size);
#endif

    tracef3("yields &%p [free: %d %d]",
            a, total_free(), largest_free());
#if TRACE
    alloc_traversefree();
#endif
    tracef0("\n");

    if(HALT_ON_FAIL  &&  !a  &&  size)
        werr("realloc(&%p, %d) yields NULL", a, size);

    return(a);
}


extern size_t
tralloc_size(void *a)
{
    #if defined(KEEP_RECORDS)
    return(* (((int *) a) - 1));
    #else
    #if !defined(OLD_ALLOCS)
    return(alloc_size(a));
    #else
    return(sizeof(int));
    #endif
    #endif
}

/* end of tralloc.c */
