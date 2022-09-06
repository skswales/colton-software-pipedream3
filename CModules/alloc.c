/* alloc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Title:       alloc.c - allocation in an extensible flex block
 * Author:      Stuart K. Swales 23-Aug-1989
*/

#if defined(WATCH_ALLOCS)
#   undef   WATCH_ALLOCS
#endif


#if !defined(EXPORT_FIXED_ALLOCS)
#   define   EXPORT_FIXED_ALLOCS
#endif


#if defined(CHECK_ALLOCS)
#   undef   TRACE
#   define  TRACE 1
#endif


/* standard header files */

#include "flags.h"


/* external header file */

#if ARTHUR || RISCOS
#include "ext.alloc"
#include "ext.handlist"
#elif MS
#include "alloc.ext"
#include "handlist.ext"
#endif

/* exported functions */

extern void  *alloc_calloc(size_t num, size_t size);
extern void   alloc_dispose(void **v);
extern void   alloc_free(void *a);
extern int    alloc_init(void);
extern void  *alloc_malloc(size_t size);
extern void  *alloc_realloc(void *a, size_t size);
extern size_t alloc_size(void *a);
extern void   alloc_tidy_up(void);

#if defined(EXPORT_FIXED_ALLOCS)
extern void   fixed_dispose(void **v);
extern void   fixed_free(void *a);
extern void  *fixed_malloc(size_t size);
extern void  *fixed_realloc(void *a, size_t size);
#endif


#if RISCOS
#include "swinumbers.h"

#include "os.h"
#include "werr.h"
#include "flex.h"
#endif


/* Interface to RISC OS Heap Manager */ 

#define XOS_Heap (XOS_MASK | OS_Heap)


typedef enum
{
    HeapReason_Init,
    HeapReason_Desc,
    HeapReason_Get,
    HeapReason_Free,
    HeapReason_ExtendBlock,
    HeapReason_ExtendHeap,
    HeapReason_ReadBlockSize
}
HeapReasonCodes;


typedef struct
{
    intl magic;     /* ID word */
    intl free;      /* offset to first block on free list ***from this location*** */
    intl hwm;       /* offset to first free location */
    intl size;      /* size of heap, including header */
                    /* rest of heap follows here ... */
}
riscos_heap;


#if defined(CHECK_ALLOCS)
#define FILL_BYTE      0x00
#define FILL_WORD      ((((((FILL_BYTE << 8) + FILL_BYTE) << 8) + FILL_BYTE) << 8) + FILL_BYTE)
#define startguardsize 0x10
#define endguardsize   0x10
#define zap(p, n)      memset(p, FILL_BYTE, n);
#else
#define startguardsize 0
#define endguardsize   0
#define zap(p, n)
#endif


typedef struct
{
    intl size;      /* rounded size of used block */
                    /* data follows here ... */
}
riscos_usedblock;

/* amount of core allocated to this object */
#define blocksize(core) ((((riscos_usedblock *) core) - 1)->size - sizeof(riscos_usedblock))


typedef struct
{
    intl free;      /* offset to next block on free list ***from this location*** */
    intl size;      /* size of free block */
                    /* free space follows here ... */
}
riscos_freeblock;


/* RISC OS only maintains size field on used blocks */ 
#define HEAPMGR_OVERHEAD    sizeof(riscos_usedblock)

/* Round to integral number of RISC OS heap manager granules
 * This size is given by the need to fit a freeblock header into
 * any space that might be freed or fragmented on allocation.
*/
#define round_heapmgr(n)    ((n + (sizeof(riscos_freeblock)-1)) & ~(sizeof(riscos_freeblock)-1))


/* Initial size of block to claim for heap */
#define HEAP_INITSIZE       0x2000

/* How much to grow heap by when small allocation needs to grow it */
#if defined(SHAKE_MEMORY_VIOLENTLY)
#define HEAP_INCREMENT      0x0040
#define flex_pagesize       0x0080
#else
#define HEAP_INCREMENT      0x1000
#endif


/* ----------------------------------------------------------------------- */

static riscos_heap *heap_address = NULL;


#if TRACE
static os_error *
alloc__error(os_error *e)
{
    if(e)
        werr("alloc error: %s", e->errmess);

    return(e);
}


static void
alloc__ensure(void)
{
    if(!heap_address)
        werr_fatal("Heap not initialised");
}

#else
#define alloc__error(e) (e)
#define alloc__ensure()
#endif


#if !defined(OLD_ALLOCS)

static int
initialise_heap(size_t initsize)
{
    os_regset r;
    os_error *e;
    size_t nbytes = round_heapmgr(max(HEAP_INITSIZE, initsize + HEAPMGR_OVERHEAD));

    if(flex_alloc((flex_ptr) &heap_address, nbytes))
        {
        tracef2("got heap at &%p, size %d\n", heap_address, nbytes);

        r.r[0] = HeapReason_Init;
        r.r[1] = (int) heap_address;
        /* no r2 */
        r.r[3] = nbytes;
        e = alloc__error(os_swix(XOS_Heap, &r));
        return(!e);
        }

    return(FALSE);
}


/********************************************************
*                                                       *
*  ensure that a block of given size can be allocated,  *
*  the heap being extended as necessary                 *
*                                                       *
********************************************************/

static intl
needtoallocate(size_t size)
{
    intl current_hwm    = heap_address->hwm;
    intl current_size   = heap_address->size;
    intl spare          = current_size - current_hwm;
    intl need           = round_heapmgr(size + HEAPMGR_OVERHEAD);
    intl delta          = need - spare;

    tracef4("needtoallocate(%d): current_hwm = &%p, current_size = &%p, spare = %d\n",
            size, current_hwm, current_size, spare);

    if(delta <= 0)
        {
        tracef0("*** no heap extension required\n");
        trace_pause();
        return(TRUE);
        }

    delta = max(delta, HEAP_INCREMENT);

    tracef2("extending heap &%p by %d\n", heap_address, delta);

#if TRUE
    /* test out new version */
    do  {
        if(flex_extend((flex_ptr) &heap_address, current_size + delta))
            {
            heap_address->size = flex_size((flex_ptr) &heap_address);

            tracef2("heap &%p now size %d\n", heap_address, heap_address->size);

            return(TRUE);
            }

        tracef0("*** heap extension failed - free pool space\n");
        trace_pause();
        list_unlockpools();
        }
    while(list_freepoolspace(delta) >= delta);

    tracef0("*** heap extension failed - return FALSE\n");
    trace_pause();
    return(FALSE);
#else
    if(!flex_extend((flex_ptr) &heap_address, current_size + delta))
        {
        tracef0("*** heap extension failed\n");
        trace_pause();
        return(FALSE);
        }

    heap_address->size = flex_size((flex_ptr) &heap_address);

    tracef2("heap &%p now size %d\n", heap_address, heap_address->size);

    return(TRUE);
#endif
}


/********************************************
*                                           *
* release the free store at the top of the  *
* heap and flex area back to the free pool  *
*                                           *
********************************************/ 

static void
freeextrastore(void)
{
    intl current_hwm    = heap_address->hwm;
    intl current_size   = heap_address->size;
    intl spare          = current_size - current_hwm;

    if(spare + flex_storefree() >= flex_pagesize)
        {
        tracef0("contracting heap to free some space\n");

        if(!flex_extend((flex_ptr) &heap_address, current_hwm))
            {
            tracef0("*** heap contraction failed\n");
            trace_pause();
            return;
            }

        heap_address->size = flex_size((flex_ptr) &heap_address);

        tracef2("heap &%p now contracted to size %d\n", heap_address, heap_address->size);
        }
}

#endif


extern void *
alloc_calloc(size_t num, size_t size)
{
#if defined(OLD_ALLOCS)
    return(calloc(num, size));
#else
    size_t nbytes = (num * size + (sizeof(word32) - 1)) & ~(sizeof(word32) - 1);
    word32 *a = alloc_malloc(nbytes);
    word32 *b;

    if(a)
        {
        b = a + nbytes / sizeof(word32);

        do { *--b = 0; } while(b != a);
        }

    return(a);
#endif
}


extern void
alloc_dispose(void **v)
{
    void *a;

    if(v)
        {
        a = *v;

        if(a)
            {
            *v = NULL;
            alloc_free(a);
            }
        }
}


extern void
alloc_free(void *a)
{
#if defined(OLD_ALLOCS)
    free(a);
#else
    char *core;
    os_regset r;
    os_error *e;

    if(a)
        {
        alloc__ensure();

        core = (char *) a - startguardsize;

        #if defined(CHECK_ALLOCS)
        {
        word32 *end   = (word32 *) (core + startguardsize);
        word32 *ptr   = end;
        word32 *start = (word32 *) core;
        while(ptr > start)
            if(*--ptr != FILL_WORD)
                werr_fatal("alloc_free: object &%p: fault at startguard offset %d: &%8.8X != FILL_WORD &8.8X",
                            a, (start - ptr) * sizeof(word32), *ptr, FILL_WORD);
        }
        {
        char *end   = core + blocksize(core);
        char *ptr   = end - endguardsize;
        char *start = a;
        while(ptr < end)
            if(*ptr++ != FILL_BYTE)
                werr_fatal("alloc_free: object &%p: size %d fault at endguard offset %d: &%2.2X != FILL_BYTE &%2.2X",
                            a, blocksize(core) - (startguardsize + endguardsize), (--ptr - start) * sizeof(char), *ptr, FILL_BYTE);
        }
        #endif

        r.r[0] = HeapReason_Free;
        r.r[1] = (int) heap_address;
        r.r[2] = (int) core;
        e = alloc__error(os_swix(XOS_Heap, &r));

        if(!e)
            freeextrastore();
        }
#endif
}


extern int
alloc_init(void)
{
    tracef0("alloc_init()\n");

    #if RISCOS
    if(!flex_init())
        return(FALSE);
    #endif

#if defined(OLD_ALLOCS)
    return(TRUE);
#else
    return(initialise_heap(0));
#endif
}


extern void *
alloc_malloc(size_t size)
{
#if defined(OLD_ALLOCS)
    return(malloc(size));
#else
    char *core;
    os_regset r;
    os_error *e;

    if(!size)
        return(NULL);

    alloc__ensure();

    size += startguardsize + endguardsize;

    r.r[0] = HeapReason_Get;
    r.r[1] = (int) heap_address;
    /* no r2 */
    r.r[3] = size;
    e = os_swix(XOS_Heap, &r);

    if(e)
        {
        if(!needtoallocate(size))
            return(NULL);

        r.r[0] = HeapReason_Get;
        r.r[1] = (int) heap_address;
        /* no r2 */
        r.r[3] = size;
        e = alloc__error(os_swix(XOS_Heap, &r));

        if(TRACE  &&  (e  ||  !r.r[2]))
            werr("alloc_malloc(%d) failed unexpectedly after heap extension\n", size);

        if(e)
            return(NULL);
        }

    core = (char *) r.r[2];

    zap(core, blocksize(core));

    return(core + startguardsize);
#endif
}


extern void *
alloc_realloc(void *a, size_t size)
{
#if defined(OLD_ALLOCS)
    return(realloc(a, size));
#else
    char *core;
    os_regset r;
    os_error *e;
    size_t new_size, current_size;
    intl delta;

    /* shrinking to zero size? */

    if(!size)
        {
        if(a)
            alloc_free(a);

        return(NULL);
        }


    /* first-time allocation? */

    if(!a)
        return(alloc_malloc(size));


    alloc__ensure();

    core = (char *) a - startguardsize;
    size += startguardsize + endguardsize;

    #if defined(CHECK_ALLOCS)
    {
    word32 *end   = (word32 *) (core + startguardsize);
    word32 *ptr   = end;
    word32 *start = (word32 *) core;
    while(ptr > start)
        if(*--ptr != FILL_WORD)
            werr_fatal("alloc_realloc: object &%p: fault at startguard offset %d: &%8.8X != FILL_WORD &8.8X",
                        a, (start - ptr) * sizeof(word32), *ptr, FILL_WORD);
    }
    {
    char *end   = core + blocksize(core);
    char *ptr   = end - endguardsize;
    char *start = a;
    while(ptr < end)
        if(*ptr++ != FILL_BYTE)
            werr_fatal("alloc_realloc: object &%p: size %d fault at offset %d: &%2.2X != FILL_BYTE &%2.2X",
                        a, blocksize(core) - (startguardsize + endguardsize), (--ptr - start) * sizeof(char), *ptr, FILL_BYTE);
    }
    #endif

    new_size     = round_heapmgr(size + HEAPMGR_OVERHEAD);
    current_size = blocksize(core) + HEAPMGR_OVERHEAD;
    delta        = new_size - current_size;


    /* block not changing allocated size? */

    if(!delta)
        {
        zap(core + (blocksize(core) - endguardsize), endguardsize);

        return(a);
        }


    /* block shrinking? */

    if(delta < 0)
        {
        r.r[0] = HeapReason_ExtendBlock;
        r.r[1] = (int) heap_address;
        r.r[2] = (int) core;
        r.r[3] = delta;
        e = alloc__error(os_swix(XOS_Heap, &r));

        if(e)
            return(NULL);

        freeextrastore();

        core = (char *) r.r[2];

        zap(core + (blocksize(core) - endguardsize), endguardsize);

        return(core + startguardsize);
        }


    /* block is growing */

    r.r[0] = HeapReason_ExtendBlock;
    r.r[1] = (int) heap_address;
    r.r[2] = (int) core;
    r.r[3] = delta;
    e = os_swix(XOS_Heap, &r);

    if(e)
        {
        if(!needtoallocate(new_size))
            return(NULL);

        r.r[0] = HeapReason_ExtendBlock;
        r.r[1] = (int) heap_address;
        r.r[2] = (int) core;
        r.r[3] = delta;
        e = alloc__error(os_swix(XOS_Heap, &r));

        if(TRACE  &&  (e  ||  !r.r[2]))
            werr("alloc_realloc(&%p, %d) failed unexpectedly after heap extension\n", a, size);

        if(e)
            return(NULL);
        }

    freeextrastore();

    core = (char *) r.r[2];

    zap(core + (blocksize(core) - endguardsize), endguardsize);

    return(core + startguardsize);
#endif
}


extern size_t
alloc_size(void *a)
{
#if defined(OLD_ALLOCS)
    abort();
#else
    char *core;
    size_t size;

    if(!a)
        return(0);

    core = (char *) a - startguardsize;
    size = blocksize(core) - (startguardsize + endguardsize);

    #if defined(CHECK_ALLOCS)
    {
    word32 *end   = (word32 *) (core + startguardsize);
    word32 *ptr   = end;
    word32 *start = (word32 *) core;
    while(ptr > start)
        if(*--ptr != FILL_WORD)
            werr_fatal("alloc_size: object &%p: fault at startguard offset %d: &%8.8X != FILL_WORD &8.8X",
                        a, (ptr - end) * sizeof(word32), *ptr, FILL_WORD);
    }
    {
    char *end   = core + blocksize(core);
    char *ptr   = end - endguardsize;
    char *start = a;
    while(ptr < end)
        if(*ptr++ != FILL_BYTE)
            werr_fatal("alloc_size: object &%p: size %d fault at offset %d: &%2.2X != FILL_BYTE &%2.2X",
                        a, blocksize(core) - (startguardsize + endguardsize), (--ptr - start) * sizeof(char), *ptr, FILL_BYTE);
    }
    #endif

    return(size);
#endif
}


/****************************
*                           *
*  release surplus memory   *
*                           *
****************************/

extern void
alloc_tidy_up(void)
{
#if !defined(OLD_ALLOCS)
    freeextrastore();
#endif
}


#if RISCOS && TRACE

extern void
alloc_limits(void **start, void **end, int *size, int *free)
{
    alloc__ensure();

    if(start)
        *start = heap_address + 1;

    if(end)
        *end   = (char *) heap_address + heap_address->hwm;

    if(size)
        *size  = heap_address->size;

    if(free)
        *free  = alloc_totalfree();
}


/* a function used by tralloc.c when WATCH_ALLOCS defined there */

extern intl
alloc_largestfree(void)
{
    os_regset r;
    os_error *e;

    alloc__ensure();

    r.r[0] = HeapReason_Desc;
    r.r[1] = (int) heap_address;
    e = alloc__error(os_swix(XOS_Heap, &r));

    return(e ? 0 : ((r.r[2] > 0) ? r.r[2] : 0));
}


extern intl
alloc_totalfree(void)
{
    os_regset r;
    os_error *e;

    alloc__ensure();

    r.r[0] = HeapReason_Desc;
    r.r[1] = (int) heap_address;
    e = alloc__error(os_swix(XOS_Heap, &r));

    return(e ? 0 : ((r.r[3] > 0) ? r.r[3] : 0));
}


extern void
alloc_traversefree(void)
{
    char *freep         = (char *) &heap_address->free;
    intl offset         = heap_address->free;
    intl current_hwm    = heap_address->hwm;
    intl current_size   = heap_address->size;
    intl spare          = current_size - current_hwm;
    intl size;

    tracef2(" current size &%p; spare block %d; free list:", current_size, spare);

    alloc__ensure();

    if(offset < 0)
        werr_fatal("alloc heap has corrupt free link at &%p", freep);

    while(offset)
        {
        freep   += offset;

        offset  = ((riscos_freeblock *) freep)->free;
        size    = ((riscos_freeblock *) freep)->size;

        tracef2(" &%p,%d", freep, size);

        if(offset < 0)
            werr_fatal("alloc heap has corrupt free list offset %d at &%p", offset, freep);
        if((size <= 0)  ||  (offset  &&  (size >= offset)))
            werr_fatal("alloc heap has corrupt free list size %d at &%p", size, freep);
        }
}

#endif  /* TRACE */


#if defined(EXPORT_FIXED_ALLOCS)

/********************************************************
*                                                       *
* these allocation routines do not cause flex memory    *
* to be moved as they use the standard C library code   *
*                                                       *
********************************************************/

#undef free
#undef malloc
#undef realloc

extern void
fixed_dispose(void **v)
{
    void *a;

    if(v)
        {
        a = *v;

        if(a)
            {
            *v = NULL;
            free(a);
            }
        }
}


extern void
fixed_free(void *a)
{
    free(a);
}


extern void *
fixed_malloc(size_t size)
{
    return(malloc(size));
}


extern void *
fixed_realloc(void *a, size_t size)
{
    return(realloc(a, size));
}

#endif  /* EXPORT_FIXED_ALLOCS */

/* end of alloc.c */
