/* > c.flex */

/* Title:   flex.c
 * Purpose: provide memory allocation for interactive programs requiring
 *          large chunks of store. Such programs must respond to memory
 *          full errors, and must not suffer from fragmentation.
 * Version: 0.1
*/

/* Change list:
 *
 *  MRJC    18/11/88    If setting the slotsize fails, the original value is restored
 *  MRJC    16/6/89     flex_storefree returns amount of unused flex memory
 *  MRJC    19/6/89     flex_move_anchors added to allow movement of
 *                      anchors in a certain block of memory
 *  MRJC    19/6/89     store returns amount of store available from wimp
 *  MRJC    20/6/89     flex__more checks if there is any memory before writing
 *                      wimpslot to avoid lots of silly task manager messages
 *  SKS     04/08/89    exported page size for alloc.c to check
 *  SKS     22/08/89    made flex_extend more like realloc
 *  SKS     26/09/89    made flex use faster memory copier
*/

#if defined(CHECK_ALLOCS)
#undef  TRACE
#define TRACE 1
#endif

#define BOOL  int
#define TRUE  1
#define FALSE 0

#include "include.h"

#include "werr.h"
#include "wimp.h"
#include "wimpt.h"

#if !defined(OWN_MEMCPY)
#   define  MemMove(s1, s2, n)  memmove(s1, s2, n)
#else
#   include "CModules:copymem.h"
#   define  MemMove(s1, s2, n)  copymem(s1, s2, n)
#endif

#include "flex.h"


typedef struct
{
    flex_ptr anchor;    /* *(p->anchor) should point back to this actual core (p+1) */
    int      size;      /* Exact size of logical area in bytes */
    #if defined(CHECK_ALLOCS)
    int      startguard, startguard2, startguard3, startguard4;
    #endif
    /* actual store follows (p+1) */
}
flex__rec;


#if defined(CHECK_ALLOCS)
#define FILL_BYTE    0x00
#define FILL_WORD    ((((((FILL_BYTE << 8) + FILL_BYTE) << 8) + FILL_BYTE) << 8) + FILL_BYTE)
#define endguardsize 0x10
#define zap(p, n)    memset(p, FILL_BYTE, n);
#else
#define endguardsize 0
#define zap(p, n)
#endif


/* There are two alternative implementations in this file. */

#if TRUE

/* This implementation goes above the original value of GetEnv,
 * to memory specifically requested from the Wimp (about which the
 * standard library, and malloc, know nothing). The heap is kept
 * totally compacted all the time, with pages being given back to
 * the Wimp whenever possible.
*/

int flex_pagesize;                          /* page size (exported) */


/* can get better code for loading structure members on ARM Norcroft */

static struct flex_
{
    char *          start;          /* start of flex memory */
    char *          freep;          /* free flex memory */
    char *          lim;            /* limit of flex memory */
    flex_notifyproc notifyproc;     /* notification procedure */
}
flex_;


/* From base upwards, it's divided into store blocks of
 *   the flex__rec
 *   the object
 *   align up to next word.
*/


#if TRACE

static void
flex__fail(BOOL cond, int i)
{
    if(cond)
        werr_fatal("fatal store error flex-%d", i);
}

#else
#define flex__fail(cond, i)
#endif


#define roundup(i)      ((i + 3) & ~3)

#define innards(p)      ((char *) (p + 1))

#define next_block(p)   ((flex__rec *) (innards(p) + roundup(p->size) + endguardsize))


static void 
flex__wimpslot(char **top)
{
    /* read/write the top of available memory. *top == -1 -> just read. */
    int dud = -1;
    int slot = (int) *top;
    if(slot != -1)
        slot -= 0x8000;
    tracef1("flex__wimpslot in: &%p\n", slot);
    wimpt_noerr(wimp_slotsize(&slot, &dud, &dud));
    *top = (char *) slot + 0x8000;
    tracef1("flex__wimpslot out: &%p\n", slot);
}


/****************************************************
*                                                   *
* MRJC's version of flex__more checks if            *
* the memory is available before calling wimpslot   *
*                                                   *
****************************************************/

/* Tries to get at least n more bytes, raising flex_.lim and returning TRUE if it can */

static BOOL
flex__more(int n)
{
    if(flex_extrastore() < n)
        {
        tracef1("[flex__more returns FALSE, extrastore is: &%X]\n", flex_extrastore());
        return(FALSE);
        }

    tracef2("[flex__more calling wimpslot flex_.lim: &%p, required flex_.lim: &%p]\n", flex_.lim, flex_.lim + n);

    flex_.lim = flex_.lim + n;

    flex__wimpslot(&flex_.lim);

    tracef1("[flex__more: flex_.lim is now: &%p]\n", flex_.lim);
    return(TRUE);
}



static BOOL
flex__ensure(int n)
{
    tracef3("[flex__ensure(%d) flex_.lim: &%p, flex_.freep: &%p]\n", n, flex_.lim, flex_.freep);
    n -= flex_.lim - flex_.freep;
    return((n <= 0)  ||  flex__more(n));
}


/* MRJC doctored 20/6/89 to avoid calling wimpslot when not necessary */

static void
flex__give(void)
{
    /* Gives away memory, lowering flex_.lim, if possible. */

    tracef2("[flex__give() flex_.lim: &%p, flex_.freep: &%p]\n", flex_.lim, flex_.freep);

    if((((int) flex_.lim - 1) / flex_pagesize)  !=  (((int) flex_.freep - 1) / flex_pagesize))
        {
        flex_.lim = flex_.freep;
        flex__wimpslot(&flex_.lim);
        tracef1("[flex__give called wimpslot, new flex_.lim: &%p]\n", flex_.lim);
        }
}


extern BOOL
flex_alloc(flex_ptr anchor, int n)
{
    flex__rec *p;
    int required = sizeof(flex__rec) + roundup(n) + endguardsize;

    tracef2("[flex_alloc(&%p, %d)]\n", anchor, n);

    if((n < 0)  ||  !flex__ensure(required))
        {
        *anchor = NULL;
        trace_on();
        tracef1("[flex_alloc(%d) yields NULL]\n", n);
        trace_pause();
        trace_off();
        return(FALSE);
        }

    /* allocate at end of memory */
    p = (flex__rec *) flex_.freep;

    flex_.freep = flex_.freep + required;

    zap(p, required);

    p->anchor = anchor;
    p->size   = n;              /* store requested amount, not allocated amount */

    *anchor   = innards(p);     /* point to punter's part of allocated object */

    tracef1("[flex_alloc yields &%p]\n", *anchor);
    return(TRUE);
}


#if TRACE

/* show all flex pointers for debugging purposes */
extern void
flex_display(void)
{
    flex__rec *p = (flex__rec *) flex_.start;

    tracef3("[flex__display(): flex_.start &%p flex_.lim &%p flex_.freep &%p]\n", flex_.start, flex_.lim, flex_.freep);

    while(p < (flex__rec *) (flex_.freep - endguardsize))
        {
        tracef4("flex block &%p->&%p->&%p, size %d", p, p->anchor, *(p->anchor), p->size);

        if(*(p->anchor) != innards(p))
            tracef0(" <<< bad block!");

        tracef0("\n");

        p = next_block(p);
        }
}


extern void
flex_limits(void **start, void **end, int *free, int *size)
{
    if(start)
        *start = flex_.start;

    if(end)
        *end   = flex_.freep - endguardsize;

    if(free)
        *free  = flex_.lim - flex_.freep;

    if(size)
        *size  = flex_.lim - flex_.start;
}

#endif


/*********************************************************************
*                                                                    *
* --in--                                                             *
* oldstart is address of block of anchors                            *
* block_size is number of anchors in block                           *
* move_amount is amount by which block has moved                     *
*                                                                    *
* --out--                                                            *
* all flex pointers to anchors in the block adjusted by move_amount  *
*                                                                    *
* MRJC 19/6/89                                                       *
*                                                                    *
*********************************************************************/

extern void
flex_move_anchors(void **oldstart, int block_size, int move_amount)
{
    flex__rec *p = (flex__rec *) flex_.start;

    tracef3("[flex_move_anchors(&%p, %d, by = %d)]\n", oldstart, block_size, move_amount);

    while(p < (flex__rec *) (flex_.freep - endguardsize))
        {
        /* check if anchor points to moved block */
        if( (p->anchor >= oldstart)  &&
            (p->anchor <  oldstart + block_size))
                p->anchor = p->anchor + move_amount;

        #if defined(CHECK_ALLOCS)
        {
        int *end = (int *) innards(p);
        int *ptr = end;
        int *start = &p->startguard;
        while(ptr > start)
            if(*--ptr != FILL_WORD)
                werr_fatal("flex_move_anchors: object &%p: size %d fault at startguard offset %d: &%8.8X != FILL_WORD &8.8X",
                            start, p->size, (ptr - end) * sizeof(int), *ptr, FILL_WORD);
        }
        {
        char *start = innards(p);                               /* where object returned to punter starts */
        char *ptr   = start + p->size;                          /* where object returned to punter ends */
        char *end   = start + roundup(p->size) + endguardsize;  /* where object really ends */
        while(ptr < end)
            if(*ptr++ != FILL_BYTE)
                werr_fatal("flex_move_anchors: object &%p: size %d fault at endguard offset %d: &%2.2X != FILL_BYTE &2.2X",
                            start, p->size, (--ptr - start) * sizeof(char), *ptr, FILL_BYTE);
        }
        #endif

        /* check we are again in registration */
        if(TRACE)
            if(*(p->anchor) != innards(p))
                werr_fatal("flex_move_anchors: p->anchor &%p *(p->anchor) &%p != object &%p (oldstart &%p size %d motion %d)",
                            p->anchor, *(p->anchor), innards(p), oldstart, block_size, move_amount);

        /* move to next flex block */
        p = next_block(p);
        }
}


static void
flex__reanchor(flex__rec *startp, int by)
{
    flex__rec *p = startp;

    /* Move all the anchors from p upwards. This is in anticipation
     * of that block of the heap being shifted.
    */
    tracef3("[flex_reanchor(&%p, by = %d): flex_.freep = &%p]\n", p, by, flex_.freep);

    while(p < (flex__rec *) (flex_.freep - endguardsize))
        {
        if(TRACE  &&  by)
            tracef3("reanchoring object &%p (&%p) to &%p  ", innards(p), p->anchor, innards(p) + by);

        #if defined(CHECK_ALLOCS)
        {
        int *end = (int *) innards(p);
        int *ptr = end;
        int *start = &p->startguard;
        while(ptr > start)
            if(*--ptr != FILL_WORD)
                werr_fatal("flex__reanchor: object &%p: size %d fault at startguard offset %d: &%8.8X != FILL_WORD &8.8X",
                            start, p->size, (ptr - end) * sizeof(int), *ptr, FILL_WORD);
        }
        {
        char *start = innards(p);                               /* where object returned to punter starts */
        char *ptr   = start + p->size;                          /* where object returned to punter ends */
        char *end   = start + roundup(p->size) + endguardsize;  /* where object really ends */
        while(ptr < end)
            if(*ptr++ != FILL_BYTE)
                werr_fatal("flex__reanchor: object &%p: size %d fault at endguard offset %d: &%2.2X != FILL_BYTE &2.2X",
                            start, p->size, (--ptr - start) * sizeof(char), *ptr, FILL_BYTE);
        }
        #endif

        /* check current registration */
        if(TRACE)
            if(*(p->anchor) != innards(p))
                werr_fatal("flex__reanchor: p->anchor &%p *(p->anchor) &%p != object &%p (motion %d)",
                            p->anchor, *(p->anchor), innards(p), by);

        /* point anchor to where block will be moved */
        *(p->anchor) = innards(p) + by;

        /* does anchor needs moving (is it above the reanchor start and in the flex area?) */
        if(((char *) p->anchor >= (char *) startp)  &&  ((char *) p->anchor < flex_.freep))
            {
            tracef3("moving anchor for object &%p from &%p to &%p  ", innards(p), p->anchor, (char *) p->anchor + by);
            p->anchor = (flex_ptr) ((char *) p->anchor + by);
            }

        p = next_block(p);
        }

    tracef0("\n");
}


extern void
flex_free(flex_ptr anchor)
{
    flex__rec *p = (flex__rec *) *anchor;
    flex__rec *next;
    int nbytes_above;
    int blksize;

    tracef3("[flex_free(&%p -> &%p (size %d))]\n", anchor, p, flex_size(anchor));

    if(!p--)
        return;

    #if defined(CHECK_ALLOCS)
    {
    int *end = (int *) innards(p);
    int *ptr = end;
    int *start = &p->startguard;
    while(ptr > start)
        if(*--ptr != FILL_WORD)
            werr_fatal("flex_free: object &%p: size %d fault at startguard offset %d: &%8.8X != FILL_WORD &8.8X",
                        start, p->size, (ptr - end) * sizeof(int), *ptr, FILL_WORD);
    }
    {
    char *start = innards(p);                               /* where object returned to punter starts */
    char *ptr   = start + p->size;                          /* where object returned to punter ends */
    char *end   = start + roundup(p->size) + endguardsize;  /* where object really ends */
    while(ptr < end)
        if(*ptr++ != FILL_BYTE)
            werr_fatal("flex_free: object &%p: size %d fault at endguard offset %d: &%2.2X != FILL_BYTE &2.2X",
                        start, p->size, (--ptr - start) * sizeof(char), *ptr, FILL_BYTE);
    }
    #endif

    flex__fail(p->anchor != anchor, 0);

    next = next_block(p);
    nbytes_above = flex_.freep - (char *) next;
    blksize = sizeof(flex__rec) + roundup(p->size) + endguardsize;

    if(nbytes_above)
        {
        /* give client an idea that the world is about to move */
        if( flex_.notifyproc)
            flex_.notifyproc();

        flex__reanchor(next, -blksize);

        MemMove(p, next, nbytes_above);
        }

    flex_.freep = flex_.freep - blksize;

    if(TRACE  &&  nbytes_above)
        /* a quick check after all that */
        flex__reanchor(p, 0);

    *anchor = NULL;

    flex__give();
}


extern BOOL
flex_extend(flex_ptr anchor, int newsize)
{
    flex__rec *p = (flex__rec *) *anchor;

    tracef3("[flex_extend(&%p -> &%p, %d)]\n", anchor, p, newsize);

    if(!p--)
        return(flex_alloc(anchor, newsize));

    if(!newsize)
        {
        flex_free(anchor);
        return(FALSE);
        }

    return(flex_midextend(anchor, p->size, newsize - p->size));
}


extern BOOL
flex_midextend(flex_ptr anchor, int at, int by)
{
    flex__rec *p = ((flex__rec *) *anchor) - 1;
    flex__rec *next;
    int growth, shrinkage;

    tracef4("[flex_midextend(&%p -> &%p, at = %d, by = %d)]\n", anchor, p, at, by);

    #if defined(CHECK_ALLOCS)
    {
    int *end = (int *) innards(p);
    int *ptr = end;
    int *start = &p->startguard;
    while(ptr > start)
        if(*--ptr != FILL_WORD)
            werr_fatal("flex_midextend: object &%p: size %d fault at startguard offset %d: &%8.8X != FILL_WORD &8.8X",
                        start, p->size, (ptr - end) * sizeof(int), *ptr, FILL_WORD);
    }
    {
    char *start = innards(p);                               /* where object returned to punter starts */
    char *ptr   = start + p->size;                          /* where object returned to punter ends */
    char *end   = start + roundup(p->size) + endguardsize;  /* where object really ends */
    while(ptr < end)
        if(*ptr++ != FILL_BYTE)
            werr_fatal("flex_midextend: object &%p: size %d fault at endguard offset %d: &%2.2X != FILL_BYTE &2.2X",
                        start, p->size, (--ptr - start) * sizeof(char), *ptr, FILL_BYTE);
    }
    #endif

    flex__fail(p->anchor != anchor, 1);
    flex__fail(at > p->size, 2);

    if(by > 0)
        {
        /* Amount by which the block will actually grow. */
        growth = roundup(p->size + by) - roundup(p->size);

        if(growth)
            /* block extension not needed for very small extensions */
            {
            if(!flex__ensure(growth))
                return(FALSE);

            /* give client an idea that the world is about to move */
            if( flex_.notifyproc)
                flex_.notifyproc();

            next = next_block(p);

            /* The move has to happen in two parts because the moving
             * of objects above is word-aligned, while the extension within
             * the object may not be.
            */

            /* move subsequent blocks up to new position */
            flex__reanchor(next, growth);

            MemMove(((char *) next) + roundup(growth),
                    next,
                    flex_.freep - (char *) next);


            /* move end of this object upwards */
            MemMove(innards(p) + at + by,
                    innards(p) + at,
                    p->size - at);

            flex_.freep = flex_.freep + growth;
            }

        p->size = p->size + by;

        zap(innards(p) + p->size, (char *) next_block(p) - (innards(p) + p->size));

        #if TRACE
        /* a quick check after all that */
        flex__reanchor(next_block(p), 0);
        #endif
        }
    elif(by)
        {
        shrinkage = roundup(p->size) - roundup(p->size + by);
        /* a positive value */

        flex__fail(-by > at, 3);

        if(shrinkage)
            {
            /* give client an idea that the world is about to move */
            if( flex_.notifyproc)
                flex_.notifyproc();

            next = next_block(p);

            /* move end of this block downwards */
            MemMove(innards(p) + at + by,
                    innards(p) + at,
                    p->size - at);
            }

        p->size = p->size + by;

        zap(innards(p) + p->size, (char *) next_block(p) - (innards(p) + p->size));

        if(shrinkage)
            {
            /* move subsequent blocks down to new position */
            flex__reanchor(next, - shrinkage);

            MemMove(((char *) next) - shrinkage,
                    next,
                    flex_.freep - (char *) next);

            flex_.freep = flex_.freep - shrinkage;

            flex__give();

            #if TRACE
            /* a quick check after all that */
            flex__reanchor(next_block(p), 0);
            #endif
            }
        }

    return(TRUE);
}


/******************************************************************
*                                                                 *
* Determine how much free space there is in the rest of the world *
*                                                                 *
******************************************************************/

extern int
flex_extrastore(void)
{
    int dud = -1;
    int freeslot;

    wimpt_safe(wimp_slotsize(&dud, &dud, &freeslot));

    return(freeslot);
}


/* MRJC modified 19/6/89 to always define flex_.start */

#define XOS_UpdateMEMC 0x2001A

extern int
flex_init(void)
{
    void *a;
    int temp, dud;
    static const int page_sizes[] = { 4*1024, 8*1024, 16*1024, 32*1024 };
    (void) os_swi2r(XOS_UpdateMEMC, 0, 0, &temp, &dud);
    flex_pagesize = page_sizes[(temp & 0xC) >> 2];
    tracef1("flex_init(): flex_pagesize = %d\n", flex_pagesize);
#if defined(FLEX_FAKE_PAGESIZE) /* May need to fake 32KB for debug - PipeDream 3 never saw a 4KB page system I think */
    flex_pagesize = 32*1024;
#endif

    flex_.lim = (char *) -1;
    flex__wimpslot(&flex_.lim);
    flex_.start = flex_.freep = flex_.lim;
    tracef1("flex_.lim = &%p\n", flex_.lim);

    /* Check that we're in the Wimp environment. */
    if(flex_alloc(&a, 1))
        {
        flex_free(&a);
        return(TRUE);
        }

    return(FALSE);
}


extern int
flex_size(flex_ptr anchor)
{
    flex__rec *p = (flex__rec *) *anchor;

    if(!p--)
        return(0);

    #if defined(CHECK_ALLOCS)
    {
    int *end = (int *) innards(p);
    int *ptr = end;
    int *start = &p->startguard;
    while(ptr > start)
        if(*--ptr != FILL_WORD)
            werr_fatal("flex_size: object &%p: size %d fault at startguard offset %d: &%8.8X != FILL_WORD &8.8X",
                        start, p->size, (ptr - end) * sizeof(int), *ptr, FILL_WORD);
    }
    {
    char *start = innards(p);                               /* where object returned to punter starts */
    char *ptr   = start + p->size;                          /* where object returned to punter ends */
    char *end   = start + roundup(p->size) + endguardsize;  /* where object really ends */
    while(ptr < end)
        if(*ptr++ != FILL_BYTE)
            werr_fatal("flex_size: object &%p: size %d fault at endguard offset %d: &%2.2X != FILL_BYTE &2.2X",
                        start, p->size, --ptr - start, *ptr, FILL_BYTE);
    }
    #endif

    flex__fail(p->anchor != anchor, 4);

    return(p->size);
}


extern void
flex_set_notify(flex_notifyproc proc)
{
    flex_.notifyproc = proc;
}


/* how much store do we have unused at the end of the flex area? */

extern int
flex_storefree(void)
{
    return(flex_.lim - flex_.freep);
}


#else

/* This is a temporary implementation, it simply goes to malloc.
 * Extension is done by copying, with the inevitable fragmentation resulting,
 * as you would expect. It is portable C, so would be useful when porting
 * to a different system.
*/

#define GUARDSPACE 10000
/* We always insist on this much being left before returning space from
flex. This guards against malloc falling over. */

static void flex__fail(int i)
{
  werr_fatal("fatal store error fl-1-%i.", i);
}


int flex_alloc(flex_ptr anchor, int n)
{
  char *guard = malloc(GUARDSPACE);
  flex__rec *p;
  BOOL result;

  tracef2("flex_alloc %i %i.\n", (int) anchor, n);

  if (guard == 0) guard = malloc(GUARDSPACE);
  if (guard == 0) {
    *anchor = 0;
    return 0;
  };
  p = malloc(n + sizeof(flex__rec));
  if (p == 0) p = malloc(n + sizeof(flex__rec));
  if (p==0) {
    result = FALSE;
  } else {
    p->anchor = anchor;
    p->size = n;
    *anchor = innards(p);
    result = TRUE;
  };
  free(guard);
  if (result == 0) *anchor = 0;
  return result;
}


void flex_free(flex_ptr anchor)
{
  flex__rec *p = ((flex__rec *) *anchor) - 1;
  if (p->anchor != anchor) {
    flex__fail(0);
  }
  free(p);
  *anchor = 0;
}


int flex_size(flex_ptr anchor)
{
  flex__rec *p = ((flex__rec *) *anchor) - 1;
  if (p->anchor != anchor) {
    flex__fail(4);
  }
  return(p->size);
}


int flex_extend(flex_ptr anchor, int newsize)
{
  flex__rec *p = ((flex__rec *) *anchor) - 1;
  return(flex_midextend(anchor, p->size, newsize - p->size));
}


BOOL flex_midextend(flex_ptr anchor, int at, int by)
{
  char *guard = malloc(GUARDSPACE);
  flex__rec *p;
  BOOL result = TRUE;

  if (guard == 0) guard = malloc(GUARDSPACE);
  if (guard == 0) return FALSE;
  p = ((flex__rec *) *anchor) - 1;
  if (p->anchor != anchor) {
    flex__fail(1);
  }
  if (at > p->size) {
    flex__fail(2);
  }
  if (by < 0 && (-by) > at) {
    flex__fail(3);
  }
  if (by == 0) {
    /* do nothing */
  } else {
    flex__rec *p1 = malloc(p->size + by + sizeof(flex__rec));
    if (p1 == 0) p1 = malloc(p->size + by + sizeof(flex__rec));
    if (p1 == 0) {
      result = FALSE;
    } else {
      (void) memcpy(
        /* to */ innards(p1),
        /* from */ innards(p),
        /* nbytes */ min(at, at + by));
      (void) memcpy(
        /* to */ at + by + innards(p1),
        /* from */ at + innards(p1),
        /* nbytes */ p->size  - at);
      p1->anchor = anchor;
      p1->size = p->size + by;
      *anchor = innards(p1);
    }
  };
  free(guard);
  return result;
}


int flex_storefree(void)
{
  /* totally imaginary, at the moment. */
  return(0);
}


void flex_init(void)
{
  char *guard = malloc(GUARDSPACE);
  char foo[10000];
  if (guard == 0)
    werr_fatal("Not enough space.");
  foo[123] = 0;
  free(guard);
}

#endif


/* end of flex.c */
