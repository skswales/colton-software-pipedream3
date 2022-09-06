/* handlist.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/**************************************************
*                                                 *
* moving memory list manager                      *
* MRJC                                            *
* January 1989                                    *
*                                                 *
* This highly efficient and advanced piece of     *
* code implements a generalised list manager      *
* based on a handle type heap system with movable *
* blocks of memory                                *
*                                                 *
* handleblock implemented for Windows Feb 1990    *
* off_ and link_types created April 1990          *
*                                                 *
**************************************************/

/* external flags */
#include "flags.h"

/* includes */
#include <limits.h>

#if RISCOS
#include "flex.h"
#endif

/* external header */
#if MS || WINDOWS
#include "handlist.ext"
#else
#include "ext.handlist"
#endif

/* local header file */
#include "handlist.h"

#define WINDOWS_DEBUG 0

/*
external functions
*/

extern mhandle  EXTERNAL list_allochandle(word32 size);
extern void *   EXTERNAL list_allocptr(word32 bytes);
extern itemp    EXTERNAL list_createitem(list_p lp, itemno item,
                                         intl size, intl fill);
extern void     EXTERNAL list_deallochandle(mhandle mh);
extern void     EXTERNAL list_deallocptr(void *mempnt);
extern void     EXTERNAL list_deleteitems(list_p lp, itemno item,
                                          itemno numdel);
extern void     EXTERNAL list_deregister(list_p lp);
extern void     EXTERNAL list_disposehandle(mhandlep mhp);
extern void     EXTERNAL list_disposeptr(void **mempntp);
extern intl     EXTERNAL list_ensureitem(list_p lp, itemno item);
extern intl     EXTERNAL list_entsize(list_p lp, itemno item);
extern void     EXTERNAL list_free(list_p lp);
extern word32   EXTERNAL list_freepoolspace(word32 needed);
extern intl     EXTERNAL list_garbagecollect(list_p lp);
extern memp     EXTERNAL list_getptr(mhandle hand);
extern itemp    EXTERNAL list_gotoitem(list_p lp, itemno item);
extern word32   EXTERNAL list_howmuchpoolspace(void);
extern itemp    EXTERNAL list_igotoitem(list_p lp, itemno item);
extern void     EXTERNAL list_init(list_p lp, intl maxitemsize,
                                   intl maxpoolsize);
extern itemp    EXTERNAL list_initseq(list_p lp, list_itemnop itemnop);
extern intl     EXTERNAL list_insertitems(list_p lp, itemno item,
                                          itemno numins);
extern itemp    EXTERNAL list_nextseq(list_p lp, list_itemnop itemnop);
extern word32   EXTERNAL list_packlist(list_p lp, word32 needed);
extern itemp    EXTERNAL list_prevseq(list_p lp, list_itemnop itemnop);
extern mhandle  EXTERNAL list_reallochandle(mhandle hand, word32 size);
extern void *   EXTERNAL list_reallocptr(void *mempnt, word32 bytes);
extern void     EXTERNAL list_register(list_p lp);
extern void     EXTERNAL list_unlock_handle(mhandle hand);
extern intl     EXTERNAL list_unlockpools(void);

/*
internal functions
*/

static itemp  INTERNAL addafter(list_p lp, intl size, itemno adjust);
static itemp  INTERNAL addbefore(list_p lp, intl size, itemno adjust);
static itemp  INTERNAL allocitem(list_p lp, intl size, itemno adjust);
static bytep  INTERNAL allocpool(list_p lp, intl poolix, intl size);
static itemp  INTERNAL convofftoptr(list_p lp, intl poolix, off_type off);
static void   INTERNAL deallocitem(list_p lp, itemp it);
static void   INTERNAL deallocpool(list_p lp, intl poolix);
static intl   INTERNAL expandhandleblock(void);
static itemp  INTERNAL fillafter(list_p lp, itemno itemfill, itemno adjust);
static itemp  INTERNAL fillbefore(list_p lp, itemno itemfill, itemno adjust);
static pooldp INTERNAL getpooldp(list_p lp, intl poolix);
static bytep  INTERNAL getpoolptr(pooldp pdp);
static itemp  INTERNAL reallocitem(list_p lp, itemp it, intl newsize);
static bytep  INTERNAL reallocpool(list_p lp, off_type size, intl ix);
static bytep  INTERNAL splitpool(list_p lp, itemno adjust);
static void   INTERNAL updatepoolitems(list_p lp, itemno change);

#if defined(SPARSE_VALIDATE_DEBUG) && TRACE
static void   INTERNAL _validatepool(list_p lp, BOOL recache);
static void   INTERNAL validatepools(void);
#else
#define                _validatepool(lp, recache)
#define                validatepools()
#endif
#define                validatepool(lp) _validatepool(lp, 0)

/*
static data
*/

static list_p firstblock = NULL;        /* first LISTBLOCK in chain */

/*
handle block definitions
*/

#if MS || ARTHUR || RISCOS

static void **handleblock = NULL;       /* handle block */
static intl handlesize = 0;             /* size of handle block */
static intl handlefree = 0;             /* next free handle */

#if defined SPARSE_ENDJUMPER_DEBUG
#define ENDJUMPERSIZE 8
#else
#define ENDJUMPERSIZE 0
#endif

#endif

#if WINDOWS

struct handle_block_entry
    {
    mhandle handle;
    memp pointer;
    };

typedef struct handle_block_entry FARP hand_ent_p;

static mhandle handleblock = 0;         /* handle of handle block */
static hand_ent_p handleblockp = NULL;  /* pointer to handle block */
static intl handlesize = 0;             /* size of handle block */
static intl handlefree = 0;             /* next free handle */

/*
macro to load handleblockp
*/

#define get_handleblockp() ((handleblockp)\
                                ? (handleblockp) \
                                : (handleblockp = \
                                    (hand_ent_p) GlobalLock(handleblock)))

#endif

/*******************************************
*                                          *
* allocate some memory and return a handle *
*                                          *
*******************************************/

extern mhandle EXTERNAL
list_allochandle(word32 size)
{
    #if WINDOWS
    hand_ent_p hep;
    #else
    void **nhp;
    #endif

    mhandle newhandle, i;

    tracef1("list_allochandle(%d)\n", size);

    validatepools();

    if(!size)
        return(0);

    newhandle = 0;

    while(TRUE)
        {
        if(handleblock)
            {
            if(handlefree < handlesize)
                {
                newhandle = handlefree++;
                break;
                }
            else
                {
                /* look for a spare slot */
                #ifdef SPARSE_DEBUG
                tracef0("[*** handle searching]\n");
                #endif

                #if WINDOWS

                for(i = 1, hep = get_handleblockp() + 1;
                    i < handlefree;
                    ++i, ++hep)
                    if(!hep->handle)
                        {
                        newhandle = i;
                        goto gothandle;
                        }

                #else

                for(i = 1, nhp = handleblock + 1; i < handlefree; ++i, ++nhp)
                    if(!*nhp)
                        {
                        newhandle = i;
                        goto gothandle;
                        }

                #endif
                }
            }

        if(!newhandle)
            if(expandhandleblock() < 0)
                return(0);
        }

gothandle:

    /***************************************/

    #if WINDOWS

    do
        {
        hep = get_handleblockp() + newhandle;
        hep->handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, size);
        }
    while(!hep->handle && (list_unlockpools() || list_freepoolspace(-1l)));

    return(newhandle);

    /***************************************/

    #elif MS || ARTHUR || defined(SPARSE_STD_ALLOCS)

    nhp = handleblock + newhandle;

    do  {
        *nhp = malloc((intl) size);

        if(*nhp)
            return(newhandle);

        list_unlockpools();
        }
    while(list_freepoolspace((word32) -1));

    return(0);

    /***************************************/

    #elif RISCOS

    nhp = handleblock + newhandle;

    do  {
        /*  list_unlockpools(); SKS: get upcalls from flex to
        tell handlist memory about to move */

        if(flex_alloc(nhp, (intl) size))
            {
            tracef2("[allocated handle: %d, size: %d]\n", newhandle, size);
            return(newhandle);
            }

        tracef0("[************ FLEX_ALLOC FAILED ************]\n");
        }
    while(list_freepoolspace(size) >= size);

    return(0);

    /***************************************/

    #endif
}

/********************************************
*                                           *
* allocate a block of memory, returning a   *
* pointer; the memory is locked in place    *
*                                           *
* in Windows, this function returns NEAR    *
* pointers - the block is in the Local heap *
*                                           *
********************************************/

extern void * EXTERNAL
list_allocptr(word32 bytes)
{
#if WINDOWS

    mhandle mh;
    mhandle *mhp;

    validatepools();

    if(!bytes)
        return(NULL);

    if((mh = LocalAlloc(LMEM_MOVEABLE | LMEM_NODISCARD,
                        (intl) bytes + sizeof(mhandle))) == NULL)
        return(NULL);

    if(!mh)
        return(NULL);

    mhp = (mhandle *) LocalLock(mh);

    *mhp++ = mh;
    return(mhp);

#elif MS || ARTHUR || RISCOS

    validatepools();

    return(malloc((intl) bytes));

#endif
}

/*****************************************************************************
*                                                                            *
* Create a item for a particular list and item of a given size               *
*                                                                            *
* the size must include any extra space required apart from the item         *
* overhead itself: a size of 0 means a blank item; a size of 1 leaves space  *
* for a delimiter.                                                           *
*                                                                            *
*****************************************************************************/

#define FILLSET 1

extern itemp EXTERNAL
list_createitem(list_p lp, itemno item, intl size, intl fill)
{
    itemno itemcur, newfill;
    itemp newitem, it;
    intl poolsave;
    off_type offsetsave;

    if(size >= lp->maxitemsize)
        return(NULL);

    validatepools();

    it = NULL;

    newitem = list_gotoitem(lp, item);

    if(!newitem)
        {
        #ifdef SPARSE_DEBUG
        tracef4("[list_createitem *: lp: %x, lp->itemc: %d, item: %d, atitem: %d]\n",
                (intl) lp, lp->itemc, (intl) item, (intl) list_atitem(lp));
        #endif

        it = list_igotoitem(lp, (itemcur = list_atitem(lp)));

        if(it && (it->flags & LIST_FILL))
            {
            /* save parameters for address recconstruction */
            poolsave = lp->pooldix;
            offsetsave = lp->itemc;

            if((itemcur + it->i.itemfill) > item)
                {
                /* check for creating hole */
                if(!size && !fill)
                    return(list_igotoitem(lp, itemcur));

                /* need to split filler block; newfill becomes
                the size of the lower filler block */
                if((newfill = (itemcur + it->i.itemfill -
                                item - FILLSET)) != 0)
                    {
                    /* adjust higher filler for new filler */
                    it->i.itemfill -= newfill;

                    if(!fillafter(lp, newfill, newfill))
                        {
                        /* replace earlier state */
                        convofftoptr(lp,
                                     poolsave,
                                     offsetsave)->i.itemfill += newfill;
                        return(NULL);
                        }

                    /* re-load higher fill pointer */
                    it = list_igotoitem(lp, itemcur);
                    }

                /* if filler is only 1 big, cause
                new item to overwrite it */
                if(it->i.itemfill == 1)
                    {
                    newitem = it;
                    it = NULL;
                    }
                /* otherwise make filler 1 smaller
                anticipating item being created */
                else
                    it->i.itemfill -= FILLSET;
                }
            else
                /* found a trailing filler - need to enlarge */
                {
                it->i.itemfill += item - (itemcur + it->i.itemfill);
                it = NULL;

                /* update numitem record */
                if(item >= lp->numitem)
                    {
                    lp->numitem = item + 1;
                    #ifdef SPARSE_DEBUG
                    tracef2("[list_createitem lp: %x, numitem: %d]\n",
                            (intl) lp, lp->numitem);
                    #endif
                    }
                }
            }
        /* adding past end of list - filler item needed */
        else if((newfill = (it ? item - (itemcur + list_leapnext(it))
                               : item - itemcur)) > 0)
            {
            if(!fillafter(lp, newfill, (itemno) 0))
                return(NULL);
            it = NULL;

            /* update numitem record */
            if(item >= lp->numitem)
                {
                lp->numitem = item + 1;
                #ifdef SPARSE_DEBUG
                tracef2("[list_createitem lp: %x, numitem: %d]\n",
                        (intl) lp, lp->numitem);
                #endif
                }
            }
        else
            it = NULL;
        }

    if(fill || !size)
        {
        fill = TRUE;
        size = FILLSIZE;
        }

    if(!newitem)
        {
        /* allocate actual item */
        if((newitem = addafter(lp, size, (itemno) FILLSET)) == NULL)
            {
            /* correct higher filler since we couldn't insert */
            if(it)
                convofftoptr(lp, poolsave, offsetsave)->i.itemfill += FILLSET;
            return(NULL);
            }

        /* update numitem record */
        if(item >= lp->numitem)
            {
            lp->numitem = item + 1;
            #ifdef SPARSE_DEBUG
            tracef2("[list_createitem lp: %x, numitem: %d]\n",
                    (intl) lp, lp->numitem);
            #endif
            }
        }
    else
        {
        /* item exists - ensure the same size */
        if((newitem = reallocitem(lp, newitem, size)) == NULL)
            return(NULL);
        }

    /* set filler flag if it is a filler or a hole */
    if(fill)
        {
        newitem->flags |= LIST_FILL;
        newitem->i.itemfill = FILLSET;
        }
    else
        newitem->flags &= ~LIST_FILL;

    return(newitem);
}

/***********************************************
*                                              *
* deallocate a block of memory, given a handle *
*                                              *
***********************************************/

extern void EXTERNAL
list_deallochandle(mhandle mh)
{
#if WINDOWS

    hand_ent_p hep;

    validatepools();

    list_unlock_handle(mh);

    hep = get_handleblockp() + mh;

    if(GlobalFree(hep->handle))
        sysexit("list_deallochandle: GlobalFree failed");

    hep->handle = 0;

#elif MS || ARTHUR || defined(SPARSE_STD_ALLOCS)

    void **hp;

    hp = handleblock + mh;

    free(*hp);
    *hp = NULL;

#elif RISCOS

    validatepools();

    /* list_unlockpools(); SKS: get upcalls from
    flex to tell handlist memory about to move */

    flex_free(handleblock + mh);

#endif
}

/************************************************************
*                                                           *
* deallocate a block of memory, given a pointer to a handle *
*                                                           *
************************************************************/

extern void EXTERNAL
list_disposehandle(mhandlep mhp)
{
    mhandle mh = *mhp;

    if(mh)
        {
        *mhp = 0;
        list_deallochandle(mh);
        }
}

/********************************************
*                                           *
* deallocate a locked block of memory,      *
* given a pointer to the memory             *
*                                           *
* in Windows, this function takes NEAR      *
* pointers - the block is in the Local heap *
*                                           *
********************************************/

extern void EXTERNAL
list_deallocptr(void *mempnt)
{
#if WINDOWS

    mhandle mh;

    validatepools();

    if(mempnt)
        {
        mh = *((mhandle *) ((char *) mempnt - sizeof(mhandle)));
        LocalUnlock(mh);
        LocalFree(mh);
        }

#elif MS || ARTHUR || RISCOS

    validatepools();

    free(mempnt);

#endif
}

/***********************************************
*                                              *
* deallocate a locked block of memory,         *
* given a pointer to the pointer to the memory *
*                                              *
* in Windows, this function takes NEAR         *
* pointers - the block is in the Local heap    *
*                                              *
***********************************************/

extern void EXTERNAL
list_disposeptr(void **mempntp)
{
    void *mempnt = *mempntp;

    if(mempnt)
        {
        *mempntp = NULL;
        list_deallocptr(mempnt);
        }
}

/*******************************
*                              *
* delete an item from the list *
*                              *
*******************************/

extern void EXTERNAL
list_deleteitems(list_p lp, itemno item, itemno numdel)
{
    itemp it;
    itemno removed;

    validatepools();

    do  {
        if((it = list_igotoitem(lp, item)) == NULL)
            return;

        if((it->flags & LIST_FILL) && (list_leapnext(it) > numdel))
            {
            it->i.itemfill -= numdel;
            updatepoolitems(lp, -numdel);
            return;
            }

        removed = list_leapnext(it);
        numdel -= removed;
        deallocitem(lp, it);
        updatepoolitems(lp, -removed);
        }
    while(numdel);
}

/****************************
*                           *
* take list block off chain *
*                           *
****************************/

extern void EXTERNAL
list_deregister(list_p lp)
{
    list_p lpt, oldlpt;

    validatepools();

    for(lpt = firstblock, oldlpt = NULL;
        lpt;
        oldlpt = lpt, lpt = lpt->nextblock)
        if(lpt == lp)
            break;

    if(!lpt)
        return;

    if(oldlpt)
        oldlpt->nextblock = lpt->nextblock;
    else
        firstblock = lpt->nextblock;

    lpt->nextblock = NULL;
    

    #ifdef SPARSE_REGISTER_DEBUG
    {
    list_p lpt;

    tracef1("[list_deregister: %x]\n", (intl) lp);

    for(lpt = firstblock; lpt; lpt = lpt->nextblock)
        tracef1("[lp: %x registered]\n", (intl) lpt);

    }
    #endif
}

/*************************************************
*                                                *
* ensure we have a item break at the specified   *
* position - insert or split filler if necessary *
*                                                *
*************************************************/

extern intl EXTERNAL
list_ensureitem(list_p lp, itemno item)
{
    if(list_gotoitem(lp, item))
        return(0);

    if(!list_createitem(lp, item, FILLSIZE, TRUE))
        return(-1);

    return(0);
}

/*****************************************
*                                        *
* return the size in bytes of an entry   *
* the number returned by be bigger than  *
* the size originally allocated since it *
* may have been rounded up for alignment *
*                                        *
*****************************************/

extern intl EXTERNAL
list_entsize(list_p lp, itemno item)
{
    itemp it;

    if((it = list_gotoitem(lp, item)) == NULL)
        return(-1);

    if(it->offsetn)
        return(it->offsetn - ITEMOVH);

    return(lp->poold->poolfree - (((bytep) it) - lp->poold->pool) - ITEMOVH);
}

/********************
*                   *
* free a whole list *
*                   *
********************/

extern void EXTERNAL
list_free(list_p lp)
{
    intl i;

    validatepools();

    for(i = lp->pooldblkfree - 1; i >= 0; --i)
        deallocpool(lp, i);
}

/**************************************************
*                                                 *
* release any free space distributed in the pools *
*                                                 *
**************************************************/

extern word32 EXTERNAL
list_freepoolspace(word32 needed)
{
    list_p lp;
    word32 space_found;

    tracef0("[*** doing list_freepoolspace]\n");

    validatepools();

    if(needed < 0)
        needed = INT_MAX;

    for(lp = firstblock, space_found = 0;
        lp && (space_found < needed);
        lp = lp->nextblock)
            space_found += list_packlist(lp, needed);

    tracef2("[list_freepoolspace needed: %d, found: %d]\n",
            needed, space_found);

    return(space_found);
}

/***************************************
*                                      *
* garbage collect for a list, removing *
* adjacent filler blocks               *
*                                      *
***************************************/

extern intl EXTERNAL
list_garbagecollect(list_p lp)
{
    itemno item, nextitem;
    itemp it;
    itemno fill;
    intl res;

    validatepools();

    for(item = 0, res = 0; item < list_numitem(lp); ++item)
        {
        it = list_igotoitem(lp, item);

        if(it && (it->flags & LIST_FILL) && (item == list_atitem(lp)))
            {
            nextitem = item + list_leapnext(it);
            it = list_igotoitem(lp, nextitem);

            if(it &&
               (it->flags & LIST_FILL) &&
               (nextitem == list_atitem(lp)))
                {
                fill = it->i.itemfill;
                deallocitem(lp, it);
                updatepoolitems(lp, -fill);
                it = list_igotoitem(lp, item);
                it->i.itemfill += fill;
                updatepoolitems(lp, fill);
                res += 1;

                #if WINDOWS_DEBUG
                opendebug();
                writetodebug("[Filler recovered]\r\n");
                closedebug();
                #endif

                #ifdef SPARSE_DEBUG
                tracef0("[Filler recovered]\n");
                #endif
                }
            }
        }

    validatepools();

    tracef1("[%d fillers recovered]\n", res);
    return(res);
}

/*****************************************
*                                        *
* given handle, return pointer to object *
*                                        *
*****************************************/

extern memp EXTERNAL
list_getptr(mhandle hand)
{
    /* don't put validation here! */

    #if WINDOWS

    hand_ent_p hep;

    if(!handleblockp)
        if((handleblockp = (hand_ent_p) GlobalLock(handleblock)) == NULL)
            sysexit("list_getptr: GlobalLock failed 1");

    if(!((hep = handleblockp + hand)->pointer))
        {
        if(!hep->handle)
            sysexit("list_getptr: NULL handle referenced");
        else if((hep->pointer = GlobalLock(hep->handle)) == NULL)
            sysexit("list_getptr: GlobalLock failed 2");
        }

    return(hep->pointer);

    #elif MS || ARTHUR || RISCOS

    return(handleblock ? *(handleblock + hand) : NULL);

    #endif
}

/*********************************************************************
*                                                                    *
* travel to a particular item                                        *
*                                                                    *
* due to the structure used, where a item may conceptually exist,    *
* but an explicit entry in the list chain does not exist, travel()   *
* will return a NULL pointer for a item that exists but has no entry *
* of its own in the structure                                        *
*                                                                    *
* the function list_atitem() must then be called to return the       *
* actual item that was achieved                                      *
*                                                                    *
*********************************************************************/

extern itemp EXTERNAL
list_gotoitem(list_p lp, itemno item)
{
    itemno i, t;
    itemp it;
    bytep pp;
    pooldp pdp;

    validatepools();

    if(!lp)
        return(NULL);

    #if TRACE
    /* check list block registration */
    {
    list_p lpt;

    for(lpt = firstblock; lpt; lpt = lpt->nextblock)
        if(lpt == lp)
            break;

    if(!lpt)
        {
        tracef1("[lp: %x not registered]\n", (intl) lp);

        for(lpt = firstblock; lpt; lpt = lpt->nextblock)
            tracef1("[lp: %x found]\n", (intl) lpt);
        }
    }
    #endif

    /* load pool descriptor pointer */
    if((pdp = lp->poold) == NULL)
        if((pdp = getpooldp(lp, lp->pooldix)) == NULL)
            return(NULL);

    /* get to correct item */
    if((pp = pdp->pool) == NULL)
        if((pp = getpoolptr(pdp)) == NULL)
            return(NULL);

    it = (itemp) (pp + lp->itemc);

    if((i = lp->item) == item)
        {
        #ifdef SPARSE_DEBUG
        if(!lp->itemc && (item != pdp->poolitem))
            tracef1("*** mismatch: item: %d\n", (intl) item);
        #endif

        return(!(it->flags & LIST_FILL) ? it : NULL);
        }

    /* skip backwards to pool if necessary */
    if(item < pdp->poolitem)
        {
        do
            {
            --lp->pooldix;
            --lp->poold;
            --pdp;
            }
        while(item < pdp->poolitem);

        i = pdp->poolitem;
        pp = getpoolptr(pdp);
        it = (itemp) pp;
        }

    if(item > i)
        {
        do
            {
            /* go down list */
            while(it->offsetn)
                {
                if((i + (t = list_leapnext(it))) > item)
                    goto there;
                it = (itemp) (((bytep) it) + it->offsetn);
                if((i += t) == item)
                    goto there;
                }

            if((lp->pooldix + 1 < lp->pooldblkfree) &&
               (item >= (pdp + 1)->poolitem))
                {
                do
                    {
                    ++lp->pooldix;
                    ++lp->poold;
                    ++pdp;
                    }
                while((lp->pooldix + 1 < lp->pooldblkfree) &&
                      (item >= (pdp + 1)->poolitem));

                i = pdp->poolitem;
                pp = getpoolptr(pdp);
                it = (itemp) pp;
                }
            else
                break;
            }
        while(TRUE);
        }
    else if(item < i)
        /* go up the list */
        {
        do
            it = (itemp) (((bytep) it) - it->offsetp);
        while((i -= list_leapnext(it)) > item);
        }

there:

    lp->item = i;
    lp->itemc = ((bytep) it) - pp;
    return((i == item) && !(it->flags & LIST_FILL) ? it : NULL);
}

/************************************************************
*                                                           *
* calculate how much free space is distributed in the pools *
*                                                           *
************************************************************/

extern word32 EXTERNAL
list_howmuchpoolspace(void)
{
    list_p lp;
    word32 res;

    validatepools();

    for(lp = firstblock, res = 0; lp; lp = lp->nextblock)
        {
        pooldp pdp;
        intl i;

        for(i = 0, pdp = getpooldp(lp, 0);
            pdp && i < lp->pooldblkfree;
            ++i, ++pdp)
            if(pdp->poolh)
                res += pdp->poolsize - pdp->poolfree;
        }

    tracef1("[howmuchpoolspace: %ld bytes]\n", res);

    return(res);
}

/*************************************************************************
*                                                                        *
* internal gotoitem                                                      *
*                                                                        *
* igotoitem only returns a null pointer if there is no item at all in    *
* the structure; it may not get to the item specified, in which case     *
* it returns the nearest item BEFORE the one asked for. this may be      *
* a filler item, of course. this routine is for use by the internal      *
* structure management only; generally list_gotoitem() is the one to use *
*                                                                        *
*************************************************************************/

extern itemp EXTERNAL
list_igotoitem(list_p lp, itemno item)
{
    itemp it;

    validatepools();

    return((it = list_gotoitem(lp, item)) != NULL
                 ? it
                 : lp ? convofftoptr(lp, lp->pooldix, lp->itemc)
                      : NULL);
}

/**************************
*                         *
* initialise a list block *
*                         *
**************************/

extern void EXTERNAL
list_init(list_p lp, intl maxitemsize, intl maxpoolsize)
{
    lp->itemc = lp->pooldblkh = lp->pooldix =
    lp->pooldblksize = lp->pooldblkfree = 0;
    lp->item = lp->numitem = 0;
    lp->poold = lp->pooldblkp = NULL;
    lp->nextblock = NULL;

    maxitemsize = min(MAX_POOL, maxitemsize);
    maxpoolsize = min(MAX_POOL, maxpoolsize);

    lp->maxitemsize = ((maxitemsize / sizeof(align)) + 1) *
                        sizeof(align) + ITEMOVH;
    lp->maxpoolsize = maxpoolsize;
    lp->poolsizeinc = max(maxpoolsize / 4, lp->maxitemsize);
}

/************************
*                       *
* initialise a sequence *
*                       *
************************/

extern itemp EXTERNAL
list_initseq(list_p lp, list_itemnop itemnop)
{
    itemp it;

    validatepools();

    #ifdef SPARSE_SEQ_DEBUG
    tracef3("[list_initseq(&%p, &%p (current %d))]\n", lp, itemnop, *itemnop);
    #endif

    if((it = list_igotoitem(lp, *itemnop)) == NULL)
        return(NULL);

    if(!(it->flags & LIST_FILL))
        return(it);

    return(list_nextseq(lp, itemnop));
}

/***************************
*                          *
* insert items into a list *
*                          *
***************************/

extern intl EXTERNAL
list_insertitems(list_p lp, itemno item, itemno numins)
{
    itemp it, psl;

    /* if there's no item there, do nothing */
    if(((it = list_igotoitem(lp, item)) == NULL) || (item >= list_numitem(lp)))
        return(0);

    /* if we have a filler item either side,
     * add to the filler item to insert
    */
    if((it->flags & LIST_FILL))
        {
        it->i.itemfill += numins;
        updatepoolitems(lp, numins);
        return(0);
        }

    psl = (item == 0) ? NULL : list_igotoitem(lp, item - 1);
    if(psl && ((psl->flags & LIST_FILL)))
        {
        psl->i.itemfill += numins;
        updatepoolitems(lp, numins);
        return(0);
        }

    /* create filler at insert position */
    list_igotoitem(lp, item);
    if(!fillbefore(lp, numins, (itemno) 0))
        return(-1);
    updatepoolitems(lp, numins);

    return(0);
}

/****************************
*                           *
* get next item in sequence *
*                           *
****************************/

extern itemp EXTERNAL
list_nextseq(list_p lp, list_itemnop itemnop)
{
    itemp it;
    bytep pp;
    pooldp pdp;

    validatepools();

    #ifdef SPARSE_SEQ_DEBUG
    tracef4("[list_nextseq(&%p, &%p (current %d)): numitem = %d]\n", lp, itemnop, *itemnop, list_numitem(lp));
    #endif

    /* get current slot pointer */
    if( (lp->item != *itemnop)      ||
        ((pdp = lp->poold) == NULL) ||
        ((pp = pdp->pool) == NULL)  )
        {
        #if defined(SPARSE_DEBUG) || defined(SPARSE_SEQ_DEBUG)
        if(!pdp || !pp)
            tracef0("[list_nextseq: reloading pdp/pp]\n");
        else
            tracef2("[list_nextseq: lp->item %d != *itemnop %d]\n", lp->item, *itemnop);
        #endif

        if((it = list_igotoitem(lp, *itemnop)) == NULL)
            return(NULL);
        }
    else
        it = (itemp) (pp + lp->itemc);

    #if defined(SPARSE_DEBUG) || defined(SPARSE_SEQ_DEBUG)
    tracef1("[list_nextseq: it = &%p\n", it);
    #endif

    /* skip over fillers to next real row */
    do  {
        /* work out next row boundary and move to it */
        *itemnop = lp->item + list_leapnext(it);
        #if defined(SPARSE_DEBUG) || defined(SPARSE_SEQ_DEBUG)
        tracef1("[list_nextseq: *itemnop := %d\n", *itemnop);
        #endif
        if(*itemnop >= list_numitem(lp))
            return(NULL);
        it = list_igotoitem(lp, *itemnop);
        }
    while(it->flags & LIST_FILL);

    #if defined(SPARSE_DEBUG) || defined(SPARSE_SEQ_DEBUG)
    tracef1("[list_nextseq: returning it = &%p\n", it);
    #endif

    return(it);
}

/************************************************************
*                                                           *
* free pool space in a given list upto the amount requested *
*                                                           *
************************************************************/

extern word32 EXTERNAL
list_packlist(list_p lp, word32 needed)
{
    pooldp pdp;
    intl i;
    word32 space_found;

    validatepools();

    if(needed < 0)
        needed = INT_MAX;

    tracef2("[list_packlist(&%p, %d)]\n", lp, needed);

    for(i = 0, space_found = 0, pdp = getpooldp(lp, 0);
        pdp && i < lp->pooldblkfree && space_found < needed;
        ++i, ++pdp)
        {
        tracef4("[list_packlist: consider pdp &%p, poolh %d, size %d, free %d]\n",
                pdp, pdp->poolh, pdp->poolsize, pdp->poolfree);

        if(pdp->poolh  &&  (pdp->poolsize > pdp->poolfree))
            {
            space_found += (word32) pdp->poolsize - pdp->poolfree;
            reallocpool(lp, pdp->poolfree, i);
            /* reload pointer after dealloc */
            pdp = getpooldp(lp, i);
            }
        }

    /* pack descriptor if needed */
    if(space_found < needed  &&  lp->pooldblksize > lp->pooldblkfree)
        {
        mhandle newpdh;

        /* unlock descriptor block for realloc */
        if(lp->pooldblkp)
            lp->pooldblkp = lp->poold = NULL;

        newpdh = list_reallochandle(lp->pooldblkh,
                                    sizeof(struct POOLDESC) *
                                    (word32) lp->pooldblkfree);
        space_found += ((word32) lp->pooldblksize - lp->pooldblkfree) *
                       sizeof(struct POOLDESC);
        lp->pooldblksize = lp->pooldblkfree;
        lp->pooldblkh = newpdh;
        }

    tracef2("[list_packlist: found %d, in lp &%p]\n", space_found, lp);

    return(space_found);
}

/****************************
*                           *
* get next item in sequence *
*                           *
****************************/

extern itemp EXTERNAL
list_prevseq(list_p lp, list_itemnop itemnop)
{
    itemp it;
    itemno item;

    validatepools();

    if((item = *itemnop) != 0)
        {
        do
            {
            it = list_igotoitem(lp, item - 1);
            item = list_atitem(lp);
            }
        while((it->flags & LIST_FILL) && item);

        *itemnop = item;

        if(it->flags & LIST_FILL)
            return(NULL);

        return(it);
        }
    else
        return(NULL);
}

/****************************************
*                                       *
* reallocate some memory given a handle *
*                                       *
****************************************/

extern mhandle EXTERNAL
list_reallochandle(mhandle hand, word32 size)
{
#if WINDOWS

    HANDLE gh;
    hand_ent_p hep;

    validatepools();

    if(!hand)
        return(list_allochandle(size));

    if(!size)
        {
        list_deallochandle(hand);
        return(0);
        }

    list_unlock_handle(hand);

    do  {
        hep = get_handleblockp() + hand;
        gh = GlobalReAlloc(hep->handle, size, GMEM_MOVEABLE | GMEM_NODISCARD);
        }
    while(!gh && (list_unlockpools() || list_freepoolspace(-1l)));

    if(!gh)
        return(0);

    hep = get_handleblockp() + hand;
    hep->handle = gh;

    return(hand);

#elif MS || ARTHUR || defined(SPARSE_STD_ALLOCS)

    void **hp, **newp;

    validatepools();

    if(!hand)
        return(list_allochandle(size));

    if(!size)
        {
        list_deallochandle(hand);
        return(0);
        }

    hp = handleblock + hand;

    do
        {
        if((newp = realloc(*hp, (intl) size)) != NULL)
            {
            *hp = newp;
            return(hand);
            }
        list_unlockpools();
        }
    while(list_freepoolspace((word32) -1));

    return(0);

#elif RISCOS

    word32 extra;

    validatepools();

    if(!hand)
        return(list_allochandle(size));

    if(!size)
        {
        list_deallochandle(hand);
        return(0);
        }

    tracef2("[list_reallochandle(%d, %d)]\n", hand, size);

    do  {
    /*  list_unlockpools(); SKS: get upcalls from flex to tell handlist memory about to move */

        if(flex_extend((handleblock + hand), (intl) size))
            {
            tracef2("[list_reallochandle: %d, size: %d]\n", hand, size);
            return(hand);
            }

        tracef0("[************ FLEX_EXTEND FAILED ************]\n");
        extra = size - flex_size(handleblock + hand);
        }
    while(list_freepoolspace(extra) >= extra);

    tracef2("[list_reallochandle: %d, size: %d]\n", 0, 0);
    return(0);

#endif
}

/**************************************************
*                                                 *
* change the size of an existing block of memory, *
* given a pointer to the memory                   *
*                                                 *
* in Windows, this function takes and returns     *
* NEAR pointers - the block is in the Local heap  *
*                                                 *
**************************************************/

extern void * EXTERNAL
list_reallocptr(void *mempnt, word32 bytes)
{
#if WINDOWS

    mhandle mh, oldmh, *mhp;

    validatepools();

    if(!mempnt)
        return(list_allocptr(bytes));

    if(!bytes)
        {
        list_deallocptr(mempnt);
        return(NULL);
        }

    oldmh = *((mhandle *) ((char *) mempnt - sizeof(mhandle)));
    LocalUnlock(oldmh);
    mh = LocalReAlloc(oldmh,
                      (intl) bytes + sizeof(mhandle),
                      LMEM_MOVEABLE | LMEM_NODISCARD);

    if(!mh)
        {
        LocalLock(oldmh);
        return(NULL);
        }

    mhp = (mhandle *) LocalLock(mh);
    *mhp++ = mh;
    return(mhp);

#elif MS || ARTHUR || RISCOS

    validatepools();

    return(realloc(mempnt, (intl) bytes));

#endif
}

/**************************
*                         *
* add list block to chain *
*                         *
**************************/

extern void EXTERNAL
list_register(list_p lp)
{
    validatepools();
    validatepool(lp);

    lp->nextblock = firstblock;
    firstblock = lp;

    #ifdef SPARSE_REGISTER_DEBUG
    {
    list_p lpt;

    tracef1("[list_register: %x]\n", (intl) lp);

    for(lpt = firstblock; lpt; lpt = lpt->nextblock)
        tracef1("[lp: %x registered]\n", (intl) lpt);
    
    }
    #endif
}

/************************
*                       *
* unlock a given handle *
*                       *
************************/

extern void EXTERNAL
list_unlock_handle(mhandle hand)
{
    #if WINDOWS

    hand_ent_p hep;

    validatepools();

    hep = get_handleblockp() + hand;

    if(hep->pointer)
        {
        GlobalUnlock(hep->handle);
        hep->pointer = NULL;
        }

    #else

    IGNOREPARM(hand);

    validatepools();

    return;

    #endif
}

/***********************************************
*                                              *
* unlock any pools which are locked so that    *
* the allocator can do any dreaded shuffling   *
*                                              *
* --out--                                      *
* flag indicates whether any pools were locked *
*                                              *
***********************************************/

extern intl EXTERNAL
list_unlockpools(void)
{
    list_p lp;
    intl res;

    validatepools();

    tracef0("[Unlock pools]\n");

    for(lp = firstblock, res = 0; lp; lp = lp->nextblock)
        {
        /* unlock all pools and pool descriptors */
        pooldp pdp;
        intl i;
        BOOL desc_was_locked;

        #ifdef SPARSE_DEBUG
        tracef1("[Unlock lp: %x]\n", (intl) lp);
        #endif

        /* SKS */
        desc_was_locked = (lp->pooldblkp != NULL);
        pdp = getpooldp(lp, 0);
        if(pdp)
            { 
            #ifdef SPARSE_DEBUG
            tracef1("[Unlock pooldblkp: %x]\n", (intl) lp->pooldblkp);
            #endif

            for(i = 0; i < lp->pooldblkfree; ++i, ++pdp)
                {
                #ifdef SPARSE_DEBUG
                if(pdp->pool)
                    tracef2("[Discarding pool ptr for: %d, pool %p]\n",
                            pdp - lp->pooldblkp, pdp->pool);
                #endif

                if(pdp->pool)
                    {
                    #if WINDOWS
                    list_unlock_handle(pdp->poolh);
                    #endif
                    pdp->pool = NULL;
                    res = 1;
                    }
                }

            #if WINDOWS
            list_unlock_handle(lp->pooldblkh);
            #endif

            /* only say descriptor block unlocked if it was locked to start with */
            if(desc_was_locked)
                res = 1;
            }

        lp->pooldblkp = lp->poold = NULL;
        }

    #if WINDOWS
    /* unlock any handles in the handle block */

    if(handleblock)
        {
        intl i;
        hand_ent_p hep;

        for(i = 0, hep = get_handleblockp(); i < handlefree; ++i, ++hep)
            if(hep->pointer)
                {
                GlobalUnlock(hep->handle);
                hep->pointer = NULL;
                }

        /* unlock handle block itself */
        GlobalUnlock(handleblock);
        handleblockp = NULL;
        }

    #endif

    return(res);
}

/*********************************************
*                                            *
* add in item to list after current position *
*                                            *
*********************************************/

static itemp INTERNAL
addafter(list_p lp, intl size, itemno adjust)
{
    itemp it;
    off_type olditemc;
    itemno olditem;

    olditemc = -1;
    if(lp->poold && lp->poold->pool)
        {
        olditemc = lp->itemc;
        olditem = lp->item;

        it = (itemp) (((bytep) lp->poold->pool) + lp->itemc);
        if(it->offsetn)
            lp->itemc += it->offsetn;
        else
            lp->itemc = lp->poold->poolfree;
        lp->item += list_leapnext(it);
        }

    it = addbefore(lp, size, adjust);

    if(!it  &&  (olditemc != -1))
        {
        lp->itemc = olditemc;
        lp->item = olditem;
        }

    return(it);
}

/**********************************************
*                                             *
* add in item to list before current position *
*                                             *
**********************************************/

static itemp INTERNAL
addbefore(list_p lp, intl size, itemno adjust)
{
    itemp it;

    it = allocitem(lp, size, adjust);

    if(it)
        it->flags = 0;

    return(it);
}

/*****************************
*                            *
* allocate memory for a item *
*                            *
*****************************/

static itemp INTERNAL
allocitem(list_p lp, intl size, itemno adjust)
{
    pooldp pdp;
    off_type newsl;
    itemp it, nextsl, prevsl;

    if((pdp = lp->poold) == NULL)
        pdp = getpooldp(lp, lp->pooldix);

    /* create a pool if we have none */
    if(!lp->poold)
        {
        if(!allocpool(lp, lp->pooldix, lp->poolsizeinc))
            return(NULL);
        else
            if((pdp = lp->poold) == NULL)
                pdp = getpooldp(lp, lp->pooldix);
        }

    /* round up size */
    size += ITEMOVH;
    if(size & (sizeof(align) - 1))
        size += sizeof(align);
    size &= (0 - sizeof(align));

    /* is there room in the pool ? */
    if(pdp->poolfree + size >= pdp->poolsize)
        {
        #ifdef SPARSE_DEBUG
        tracef0("[allocitem - no room in pool]\n");
        #endif

        /* check if pool must be split */
        if(pdp->poolsize + size >= lp->maxpoolsize)
            { 
            #ifdef SPARSE_DEBUG
            tracef0("[allocitem - splitting pool]\n");
            #endif

            if(!splitpool(lp, adjust))
                return(NULL);
            }
        elif(!reallocpool(lp,
                          pdp->poolsize + lp->poolsizeinc,
                          lp->pooldix))
            return(NULL);

        /* reload pool descriptor pointer */
        pdp = getpooldp(lp, lp->pooldix);
        }

    #ifdef SPARSE_DEBUG
    tracef2("[allocitem: pdp: %x, pdp->pool: %x]\n",
            (intl) pdp, (intl) pdp->pool);
    #endif

    /* insert space into pool */
    newsl = lp->itemc;
    it = (itemp) (pdp->pool + newsl);
    nextsl = (itemp) (pdp->pool + lp->itemc + size);

    /* is there a item after this one ? */
    if(pdp->poolfree - newsl)
        {
        memmove(nextsl, it, pdp->poolfree - newsl);
        prevsl = nextsl->offsetp ? (itemp) (((bytep) it) - nextsl->offsetp)
                                 : NULL;
        nextsl->offsetp = it->offsetn = size;
        }
    else
        {
        it->offsetn = 0;
        if(newsl)
            {
            /* find previous item, painfully, by working down */
            prevsl = (itemp) pdp->pool;
            while(prevsl->offsetn)
                prevsl = (itemp) (((bytep) prevsl) + prevsl->offsetn);
            }
        else
            prevsl = NULL;
        }

    /* is there a item before this one ? */
    if(prevsl)
        it->offsetp = prevsl->offsetn = ((bytep) it) - ((bytep) prevsl);
    else
        it->offsetp = 0;

    /* update free space in pool */
    pdp->poolfree += size;

    return(it);
}

/********************************************
*                                           *
* allocate a memory pool, leaving it locked *
*                                           *
********************************************/

static bytep INTERNAL
allocpool(list_p lp, intl poolix, intl size)
{
    pooldp pdp, pdpi;
    intl i;
    mhandle poolh;

    /* check that pool descriptor block is large enough */
    if(lp->pooldblkfree + 1 >= lp->pooldblksize)
        {
        if(!lp->pooldblkh)
            {
            lp->pooldblkh = list_allochandle(sizeof(struct POOLDESC) *
                                             (word32) POOLDBLKSIZEINC);
            if(!lp->pooldblkh)
                return(NULL);

            lp->pooldblksize = POOLDBLKSIZEINC;

            #ifdef SPARSE_DEBUG
            tracef1("[allocpool allocated dblkh: %d]\n", lp->pooldblkh);
            #endif
            }
        else
            {
            mhandle newpdh;

            /* unlock descriptor block for realloc */
            if(lp->pooldblkp)
                {
                #if WINDOWS
                list_unlock_handle(lp->pooldblkh);
                #endif

                lp->pooldblkp = lp->poold = NULL;
                }

            newpdh = list_reallochandle(lp->pooldblkh,
                                        sizeof(struct POOLDESC) *
                                        ((word32) lp->pooldblksize + POOLDBLKSIZEINC));
            if(!newpdh)
                return(NULL);

            #ifdef SPARSE_DEBUG
            tracef1("[allocpool reallocated dblkh: %d]\n", lp->pooldblkh);
            #endif

            lp->pooldblksize += POOLDBLKSIZEINC;
            lp->pooldblkh = newpdh;
            }
        }

    /* ensure that shrinker never frees the entry we need;
     * create dummy entry so that packer does not try to discard
    */
    pdp = getpooldp(lp, lp->pooldblkfree++);
    pdp->poolh = 0;
    pdp->poolsize = 0;
    pdp->poolitem = 0;
    pdp->poolfree = 0;
    pdp->pool = NULL;
    
    /* allocate a pool */
    poolh = list_allochandle((word32) size);
    if(!poolh)
        {
        --lp->pooldblkfree;
        return(NULL);
        }

    /* load address of descriptor block;
     * also loads lp->pooldblkp
    */
    pdp = getpooldp(lp, poolix);

    /* insert new pool into pool descriptor block */
    for(i = lp->pooldblkfree - 1, pdpi = &lp->pooldblkp[i];
        i > poolix; --i, --pdpi)
            *pdpi = *(pdpi - 1);

    if((lp->pooldix >= poolix) &&
       (lp->pooldix != lp->pooldblkfree - 1))
        {
        ++lp->pooldix;
        if(lp->poold)
            ++lp->poold;
        }

    pdp->poolh = poolh;
    pdp->poolsize = size;
    pdp->poolitem = 0;
    pdp->poolfree = 0;
    pdp->pool = NULL;

    return(getpoolptr(pdp));
}

/***************************************
*                                      *
* extract item pointer from list block *
*                                      *
***************************************/

static itemp INTERNAL
convofftoptr(list_p lp, intl poolix, off_type off)
{
    pooldp pdp;
    bytep pp;

    if((pdp = getpooldp(lp, poolix)) == NULL)
        return(NULL);

    if((pp = pdp->pool) == NULL)
        if((pp = getpoolptr(pdp)) == NULL)
            return(NULL);

    return((itemp) (pp + off));
}

/*********************
*                    *
* deallocate an item *
*                    *
*********************/

static void INTERNAL
deallocitem(list_p lp, itemp it)
{
    off_type size, itoff;
    bytep pp;
    itemp nextsl, prevsl;
    pooldp pdp;
    itemno trailfill;

    if((pdp = lp->poold) == NULL)
        pdp = getpooldp(lp, lp->pooldix);

    if((pp = pdp->pool) == NULL)
        pp = getpoolptr(pdp);

    /* work out size */
    itoff = ((bytep) it) - pp;
    if(it->offsetn)
        size = it->offsetn;
    else
        {
        if(it->offsetp)
            size = pdp->poolfree - itoff;
        else
            {
            deallocpool(lp, lp->pooldix);
            return;
            }
        }

    /* sort out item links */
    if(it->offsetn)
        {
        nextsl = (itemp) (((bytep) it) + it->offsetn);

        /* check for special case of trailing filler slot */
        if(!it->offsetp &&
           !nextsl->offsetn &&
           (nextsl->flags & LIST_FILL) &&
           ((lp->pooldix + 1) == lp->pooldblkfree))
            {
            trailfill = list_leapnext(nextsl);
            deallocpool(lp, lp->pooldix);
            lp->numitem -= trailfill;

            #ifdef SPARSE_DEBUG
            tracef2("[deallocitem lp: %x, numitem: %d]\n",
                    (intl) lp, lp->numitem);
            #endif

            return;
            }

        nextsl->offsetp = it->offsetp;
        }
    else
        {
        /* step back to previous item so we don't hang in space */
        prevsl = (itemp) (((bytep) it) - it->offsetp);
        lp->itemc -= it->offsetp;
        lp->item -= list_leapnext(prevsl);
        prevsl->offsetn = 0;
        }

    memmove(it, ((bytep) it) + size, pdp->poolfree - size - itoff);
    pdp->poolfree -= size;
}

/**********************************
*                                 *
* deallocate pool and remove from *
* pool descriptor block           *
*                                 *
**********************************/

static void INTERNAL
deallocpool(list_p lp, intl poolix)
{
    pooldp pdp;
    intl i;

    pdp = getpooldp(lp, poolix);

    /* the pool is dead, kill the pool */
    list_deallochandle(pdp->poolh);

    /* remove descriptor */
    for(i = poolix; i < lp->pooldblkfree - 1; ++i, ++pdp)
        *pdp = *(pdp + 1);

    if(!--lp->pooldblkfree)
        {
        /* deallocate pool descriptor block */
        list_deallochandle(lp->pooldblkh);

        lp->pooldblkh = lp->pooldblksize = lp->pooldblkfree = lp->pooldix =
        lp->itemc = 0;
        lp->item = lp->numitem = 0;
        lp->pooldblkp = lp->poold = NULL;
        }
    elif(lp->pooldix >= lp->pooldblkfree)
        {
        /* make sure not pointing past the end */
        --lp->pooldix;
        --lp->poold;
        lp->item = getpooldp(lp, lp->pooldix)->poolitem;
        lp->itemc = 0;
        }

    return;
}

/**********************
*                     *
* expand handle block *
*                     *
**********************/

static intl INTERNAL
expandhandleblock(void)
{
#if MS || ARTHUR || RISCOS

    #if !defined(SPARSE_FLEX_HANDLEBLOCK)
    void **newblock;
    #endif
    intl i, blockinc;
    void **hp;
    size_t newsize;

    blockinc = HANDLEBLOCKINC;

    /* on RISCOS, calculate initial handleblock size */
    #if RISCOS && !defined(SPARSE_STD_ALLOCS) && 0 /* this is insane for new systems! */
    if(!handleblock)
        blockinc = max(flex_extrastore() / 2000, INITMAXHANDLEBLOCK);
    #endif

    newsize = (handlesize + blockinc + ENDJUMPERSIZE) * sizeof(memp);

    #if defined(SPARSE_FLEX_HANDLEBLOCK) /* on RISC OS */
    /* This relies on the handleblock being the second flex object allocated, the first being the alloc heap.
     * All subsequent flex blocks will then correctly reanchor to handleblock in the cases of (i) the alloc heap
     * growing/shrinking, moving handleblock..end of flex (as reanchor done before any movement) and (ii) handleblock
     * itself growing, moving all subsequent flex blocks which will correctly reanchor.
    */
    if(! ((!handleblock ? flex_alloc : flex_extend) ((flex_ptr) &handleblock, newsize)))
        return(-1);
    #else
    /* NB. MSC5.1 realloc prone to catastrophic failure when it fails to extend the block */

    newblock = realloc(handleblock, newsize);

    if(!newblock)
        return(-1);

    #if RISCOS && !defined(SPARSE_STD_ALLOCS)
    /* on RISC OS, flex must be told that handle block has moved (buggy - SKS 14.3.90) */
    if(handleblock)
        flex_move_anchors(handleblock, handlesize, newblock - handleblock);
    #endif

    handleblock = newblock;
    #endif

    handlesize += blockinc;

    /* clear out all the new handles */
    for(i = handlefree, hp = handleblock + handlefree;
        i < handlesize;
        ++i, ++hp)
        *hp = NULL;

    /* make sure we don't use the first handle */
    if(!handlefree)
        handlefree = 1;

    tracef2("[expand_handleblock room for %d handles, size is: %d]\n",
            handlesize, sizeof(memp) * handlesize);

    return(0);

#elif WINDOWS

    word32 size;
    mhandle gh;
    hand_ent_p hep;
    intl i;

    size = (handlesize + HANDLEBLOCKINC) * sizeof(struct handle_block_entry);

    if(handleblockp)
        {
        GlobalUnlock(handleblock);
        handleblock = 0;
        }

    if(!handleblock)
        gh = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, size);
    else
        gh = GlobalReAlloc(handleblock, size, GMEM_MOVEABLE | GMEM_NODISCARD);

    if(!gh)
        return(-1);

    handleblock = gh;
    handlesize += HANDLEBLOCKINC;

    if((handleblockp = (hand_ent_p) GlobalLock(handleblock)) == NULL)
        sysexit("expandhandleblock: GlobalLock failed");

    /* clear out all the new handle entries */
    for(i = handlefree, hep = handleblockp + handlefree;
        i < handlesize;
        ++i, ++hep)
        {
        hep->handle = 0;
        hep->pointer = NULL;
        }

    /* make sure we don't use the first handle */
    if(!handlefree)
        handlefree = 1;

    return(0);

#else

    return(0);

#endif
}

/****************************************************
*                                                   *
* create filler item after current position in list *
*                                                   *
****************************************************/

static itemp INTERNAL
fillafter(list_p lp, itemno itemfill, itemno adjust)
{
    itemp it;

    it = addafter(lp, FILLSIZE, adjust);
    if(!it)
        return(NULL);

    it->flags |= LIST_FILL;
    it->i.itemfill = itemfill;
    return(it);
}

/***************************************************
*                                                  *
* create filler item in list before specified item *
*                                                  *
***************************************************/

static itemp INTERNAL
fillbefore(list_p lp, itemno itemfill, itemno adjust)
{
    itemp it;

    it = addbefore(lp, FILLSIZE, adjust);
    if(!it)
        return(NULL);

    it->flags |= LIST_FILL;
    it->i.itemfill = itemfill;
    return(it);
}

/**********************************
*                                 *
* get pointer to descriptor block *
*                                 *
**********************************/

static pooldp INTERNAL
getpooldp(list_p lp, intl poolix)
{
    pooldp pdp;

    if(!lp->pooldblkp)
        {
        if(!lp->pooldblkh)
            return(NULL);
        else
            {
            lp->pooldblkp = list_getptr(lp->pooldblkh);

            #ifdef SPARSE_DEBUG
            tracef1("[loaded new pooldblkp: %x]\n", (intl) lp->pooldblkp);
            #endif
            }
        }

    pdp = (poolix >= lp->pooldblkfree) ? NULL : lp->pooldblkp + poolix;

    /* set temporary pointer save */
    if(poolix == lp->pooldix)
        lp->poold = pdp;

    return(pdp);
}

/*****************************************
*                                        *
* return the address of a pool of memory *
*                                        *
*****************************************/

static bytep INTERNAL
getpoolptr(pooldp pdp)
{
    if(!pdp->pool)
        {
        if(!pdp->poolh)
            return(NULL);

        pdp->pool = (bytep) list_getptr(pdp->poolh);

        #ifdef SPARSE_DEBUG
        tracef1("[loading poolptr %p]\n", pdp->pool);
        #endif
        }

    return(pdp->pool);
}

/*******************************
*                              *
* reallocate memory for a item *
*                              *
*******************************/

static itemp INTERNAL
reallocitem(list_p lp, itemp it, intl newsize)
{
    bytep pp;
    pooldp pdp;
    off_type extraspace, itemend, oldsize;

    /* check for a delete */
    if(!newsize)
        {
        deallocitem(lp, it);
        return(NULL);
        }

    if((pdp = lp->poold) == NULL)
        pdp = getpooldp(lp, lp->pooldix);

    /* create a pool if we have none */
    if(!pdp)
        return(allocitem(lp, newsize, (itemno) 0));

    lp->itemc = ((bytep) it) - pdp->pool;

    /* round up size */
    newsize += ITEMOVH;
    if(newsize & (sizeof(align) - 1))
        newsize += sizeof(align);
    newsize &= (0 - sizeof(align));

    /* work out old size */
    if(it->offsetn)
        oldsize = it->offsetn;
    else
        oldsize = pdp->poolfree - lp->itemc;

    extraspace = it ? newsize - oldsize : newsize;

    /* is there room in the pool ? */
    if(pdp->poolfree + extraspace >= pdp->poolsize)
        {
        /* check if pool must be split */
        if(pdp->poolsize + extraspace >= lp->maxpoolsize)
            {
            if(!splitpool(lp, (itemno) 0))
                return(NULL);
            }
        else if(!reallocpool(lp,
                             pdp->poolsize + lp->poolsizeinc,
                             lp->pooldix))
            return(NULL);

        /* re-load descriptor pointer */
        if((pdp = lp->poold) == NULL)
            pdp = getpooldp(lp, lp->pooldix);
        }

    /* adjust pool for space needed */
    if((pp = pdp->pool) == NULL)
        pp = getpoolptr(pdp);

    itemend = lp->itemc + oldsize;
    memmove(pp + itemend + extraspace, pp + itemend, pdp->poolfree - itemend);
    pdp->poolfree += extraspace;

    /* store new item size in item, & update link in next item */
    it = (itemp) (pp + lp->itemc);
    if(it->offsetn)
        {
        it->offsetn = oldsize + extraspace;
        ((itemp) (((bytep) it) + it->offsetn))->offsetp =
                            oldsize + extraspace;
        }

    return(it);
}

/******************************
*                             *
* reallocate a pool of memory *
*                             *
******************************/

static bytep INTERNAL
reallocpool(list_p lp, off_type size, intl ix)
{
    mhandle mh;
    pooldp pdp;

    pdp = getpooldp(lp, ix);

    #ifdef SPARSE_DEBUG
    tracef2("[reallocpool pdp: %x, pdp->pool: %x]\n",
            (intl) pdp, (intl) pdp->pool);
    #endif

    if(pdp->pool)
        {
        #if WINDOWS
        list_unlock_handle(pdp->poolh);
        #endif

        pdp->pool = NULL;
        }

    #ifdef SPARSE_DEBUG
    tracef2("[reallocpool * pdp->pool: %d, pooldblkh: %d]\n",
            pdp->pool, lp->pooldblkh);
    #endif

    mh = list_reallochandle(pdp->poolh, (word32) size);
    if(!mh)
        return(NULL);

    pdp = getpooldp(lp, ix);
    pdp->poolh = mh;
    pdp->poolsize = size;

    #ifdef SPARSE_DEBUG
    tracef2("[reallocpool *** poolh: %d, pdp->pool: %x]\n",
            pdp->poolh, (intl) pdp->pool);
    tracef2("[reallocpool *** pdp: %x, pool: %x]\n",
            (intl) pdp, (intl) getpoolptr(pdp));
    #endif

    return(getpoolptr(pdp));
}

/**********************
*                     *
* split a memory pool *
*                     *
**********************/

static bytep INTERNAL
splitpool(list_p lp, itemno adjust)
{
    bytep newpp;
    intl justadd;
    off_type newsize;
    pooldp pdp, newpdp;

    #ifdef SPARSE_DEBUG
    tracef0("*** splitting pool\n");
    #endif

    if((pdp = lp->poold) == NULL)
        pdp = getpooldp(lp, lp->pooldix);
    
    #ifdef SPARSE_DEBUG
    tracef1("[Existing pdp: %x]\n", (intl) pdp);
    #endif

    /* if adding to the end of the pool, don't split,
    just add a new pool onto the end */
    justadd = lp->itemc >= pdp->poolfree;
    if(justadd)
        {
        /* free space in old pool for efficiency */
        reallocpool(lp, pdp->poolfree, lp->pooldix);
        newsize = lp->poolsizeinc;
        }
    else
        newsize = pdp->poolsize / 2 + lp->maxitemsize;

    /* allocate new pool */
    if((newpp = allocpool(lp, lp->pooldix + 1, newsize)) == NULL)
        return(NULL);
    
    #ifdef SPARSE_DEBUG
    tracef1("[Newpp: %x]\n", (intl) newpp);
    #endif

    /* re-load pointer */
    pdp = getpooldp(lp, lp->pooldix);

    #ifdef SPARSE_DEBUG
    tracef1("[Pdp after newpp alloced: %x]\n", (intl) pdp);
    #endif

    /* load pointer for new pool */
    newpdp = getpooldp(lp, lp->pooldix + 1);

    #ifdef SPARSE_DEBUG
    tracef1("[Newpdp: %x]\n", (intl) newpdp);
    #endif

    /* are we adding the item to the end of the pool? */
    if(!justadd)
        {
        bytep pp;
        itemp it, psl;
        itemno itemc;
        off_type offset, split, bytes;

        #ifdef SPARSE_DEBUG
        tracef2("[Existing pdp->pool: %x, poolix: %d]\n",
                (intl) pdp->pool, lp->pooldix);
        #endif

        /* get address of current pool */
        if((pp = pdp->pool) == NULL)
            pp = getpoolptr(pdp);

        #ifdef SPARSE_DEBUG
        tracef1("[Existing pp: %x]\n", (intl) pp);
        #endif

        /* find a place to split */
        itemc = pdp->poolitem;
        it = (itemp) pp;
        offset = 0;
        split = pdp->poolsize / 2;

        while(offset < split)
            {
            itemc += list_leapnext(it);
            offset += it->offsetn;
            it = (itemp) (((bytep) it) + it->offsetn);
            if(adjust && itemc > lp->item)
                {
                itemc += adjust;
                #ifdef SPARSE_DEBUG
                tracef2("[***** splitpool adding adjust of: %d, new itemc: %d *****]\n",
                        adjust, itemc);
                #endif
                adjust = 0;
                }
            }

        #ifdef SPARSE_DEBUG
        tracef2("[Offset: %d, Split: %d]\n", offset, split);
        #endif

        /* copy second half into new pool */
        psl = (itemp) (((bytep) it) - it->offsetp);
        psl->offsetn = it->offsetp = 0;
        bytes = pdp->poolfree - (((bytep) it) - pp);

        #ifdef SPARSE_DEBUG
        tracef3("[Newpp: %x, it: %x, bytes: %d]\n", (intl) newpp, (intl) it, bytes);
        #endif

        memmove(newpp, it, bytes);

        /* update descriptor blocks */
        pdp->poolfree = ((bytep) it) - pp;

        newpdp->poolitem = itemc;
        newpdp->poolfree = bytes;

        /* if item in next block, update item offset and pool descriptor */
        if(lp->itemc >= pdp->poolfree)
            {
            lp->itemc -= pdp->poolfree;
            ++lp->pooldix;
            lp->poold = NULL;
            }
        }
    else
        {
        lp->itemc = 0;
        ++lp->pooldix;
        lp->poold = NULL;
        newpdp->poolitem = lp->item;
        }

    /* re-load pointers */
    pdp = getpooldp(lp, lp->pooldix);
    return(getpoolptr(pdp));
}

/********************************************
*                                           *
* update the item counts in pools below the *
* current pool after an insert or a delete  *
*                                           *
********************************************/

static void INTERNAL
updatepoolitems(list_p lp, itemno change)
{
    pooldp pdp;
    intl i;

    if((pdp = lp->poold) == NULL)
        if((pdp = getpooldp(lp, lp->pooldix)) == NULL)
            return;

    for(i = lp->pooldix; i < lp->pooldblkfree; ++i, ++pdp)
        if(pdp->poolitem > lp->item)
            pdp->poolitem += change;

    if(lp->numitem)
        {
        lp->numitem += change;

        #ifdef SPARSE_DEBUG
        tracef2("[updatepoolitems lp: %x, numitem: %d]\n",
                (intl) lp, lp->numitem);
        #endif
        }
}

#if defined(SPARSE_VALIDATE_DEBUG) && TRACE

/*****************************************
*                                        *
* attempt to validate the pool structure *
*                                        *
*****************************************/

static void INTERNAL
valfatal(int rc, memp p1, memp p2)
{
    char *ptr;

    switch(rc)
        {
        case 1:
            ptr = "lp &%p < al_start &%p";
            break;

        case 2:
            ptr = "lp &%p >= al_end &%p";
            break;

        case 3:
            ptr = "lp->pooldblkp &%p < fl_start &%p";
            break;

        case 4:
            ptr = "lp->pooldblkp &%p >= fl_end &%p";
            break;

        case 5:
            ptr = "!lp->pooldblkh but lp->pooldblkp &%p";
            break;

        case 6:
            ptr = "lp->pooldblkh %d not in handleblock (size %d)";
            break;

        case 7:
            ptr = "lp->pooldblkp &%p not same as handleblock[i] &%p";
            break;

        case 8:
            ptr = "handleblock[i] &%p < fl_start &%p";
            break;

        case 9:
            ptr = "handleblock[i] &%p >= fl_end &%p";
            break;

        case 10:
            ptr = "!pdp->poolh but pdp->pool &%p";
            break;

        case 11:
            ptr = "pdp->poolh %d not in handleblock (size %d)";
            break;

        case 12:
            ptr = "pdp->pool &%p not same as handleblock[i] &%p";
            break;

        case 13:
            ptr = "pdp->pool &%p < fl_start &%p";
            break;

        case 14:
            ptr = "pdp->pool &%p >= fl_end &%p";
            break;

        default:
            ptr = "??? &%p &%p rc = %d";
            break;
        }

    #if defined(SPECIAL_CASE)
    printf(ptr, p1, p2, rc);
    #elif(TRACE)
    trace_on();
    tracef(ptr, p1, p2, rc);
    trace_pause();
    trace_off();
    #endif
}


static BOOL validate_enabled = FALSE;

static memp fl_start = (memp) (    32*1024);
static memp fl_end   = (memp) (4*1024*1024);
static memp al_start = (memp) (    32*1024);
static memp al_end   = (memp) (4*1024*1024);

static void
cachebits(void)
{
#if defined(SPECIAL_CASE)
    /* release version may not have either of these */
#else
    alloc_limits(&al_start, &al_end, NULL, NULL);
    flex_limits (&fl_start, &fl_end, NULL, NULL);
#endif
}


#if defined(SPECIAL_CASE)

static void
_validatepool(list_p lp, BOOL recache)
{
    /* validate all pools and pool descriptors */
    memp ptr;
    pooldp pdp;
    intl i;
    BOOL desc_was_locked, pool_was_locked;

    if(!validate_enabled)
        return;

    printf("[Validating lp &%p", lp);

    if(recache && FALSE)
        cachebits();

    if((bytep) lp < (bytep) al_start)
        /* bad lp */
        valfatal(1, lp, al_start);

    if((bytep) lp >= (bytep) al_end)
        /* bad lp */
        valfatal(2, lp, al_end);

    desc_was_locked = (lp->pooldblkp != NULL);

    printf(": descriptor handle %d ptr &%p]\n", lp->pooldblkh, lp->pooldblkp);

    if(lp->pooldblkh)
        {
        if((unsigned long) lp->pooldblkh >= (unsigned long) handlefree)
            /* not in table */
            valfatal(6, (memp) lp->pooldblkh, (memp) handlefree);

        if(lp->pooldblkp)
            if(lp->pooldblkp != (ptr = list_getptr(lp->pooldblkh)))
                /* cached ptr different to that in table */
                valfatal(7, lp->pooldblkp, ptr);
        }
    else
        {
        if(lp->pooldblkp)
            /* no handle but crap ptr */
            valfatal(5, lp->pooldblkp, NULL);
        }

    if(lp->pooldblkp)
        {
        if((bytep) lp->pooldblkp < (bytep) fl_start)
            valfatal(3, lp->pooldblkp, fl_start);

        if((bytep) lp->pooldblkp >= (bytep) fl_end)
            valfatal(4, lp->pooldblkp, fl_end);
        }

    pdp = lp->pooldblkp = list_getptr(lp->pooldblkh);

    if(pdp)
        { 
        printf("[Validating pools in descriptor &%p: free %d]\n", lp->pooldblkp, lp->pooldblkfree);

        for(i = 0; i < lp->pooldblkfree; ++i, ++pdp)
            {
            pool_was_locked = (pdp->pool != NULL);

            printf("[Validating pool (&%p[%d] = &%p): handle %d ptr &%p]\n", lp->pooldblkp, i, pdp, pdp->poolh, pdp->pool);

            if(pdp->poolh)
                {
                if((unsigned long) pdp->poolh >= (unsigned long) handlefree)
                    /* not in table */
                    valfatal(10, (void *) pdp->poolh, (void *) handlefree);

                if(pdp->pool)
                    if(pdp->pool != (ptr = list_getptr(pdp->poolh)))
                        /* cached ptr different to that in table */
                        valfatal(11, pdp->pool, ptr);
                }
            else
                {
                if(pdp->pool)
                    /* no handle but crap ptr */
                    valfatal(12, pdp->pool, NULL);
                }

            if(pdp->pool)
                {
                if((bytep) pdp->pool < (bytep) fl_start)
                    valfatal(13, pdp->pool, fl_start);

                if((bytep) pdp->pool >= (bytep) fl_end)
                    valfatal(14, pdp->pool, fl_end);
                }

            /* would be really tedious to check contents! */

            if(!pool_was_locked)
                {
                #if WINDOWS
                list_unlock_handle(pdp->poolh);
                #endif

                pdp->pool = NULL;
                }
            }

        if(!desc_was_locked)
            {
            #if WINDOWS
            list_unlock_handle(lp->pooldblkh);
            #endif

            lp->pooldblkp = NULL;
            }
        }
}

static void INTERNAL
validatepools(void)
{
    intl i;
    memp ptr;
    list_p lp;

    if(!validate_enabled)
        return;

    printf("\n\n[Validate handleblock[1..%d]: ", handlefree);
    if(FALSE)
        cachebits();

    for(i = 1; i < handlefree; ++i)
        {
        ptr = handleblock[i];
        printf("[%d] = &%p  ", i, ptr);
        if(ptr)
            {
            if((bytep) ptr < (bytep) fl_start)
                valfatal(8, ptr, fl_start);

            if((bytep) ptr >= (bytep) fl_end)
                valfatal(9, ptr, fl_end);

            /* get flex to check for spillage */
            (void) flex_size((flex_ptr) &handleblock[i]);
            }
        }

    printf("\n[Validate registered pools]\n");

    for(lp = firstblock; lp; lp = lp->nextblock)
        _validatepool(lp, 0);
}

#else /* normal trace */

static void
_validatepool(list_p lp, BOOL recache)
{
    /* validate all pools and pool descriptors */
    memp ptr;
    pooldp pdp;
    intl i;
    BOOL desc_was_locked, pool_was_locked;

    if(!validate_enabled)
        return;

    tracef1("[Validating lp &%p", lp);

    if(recache)
        cachebits();

    if((bytep) lp < (bytep) al_start)
        /* bad lp */
        valfatal(1, lp, al_start);

    if((bytep) lp >= (bytep) al_end)
        /* bad lp */
        valfatal(2, lp, al_end);

    desc_was_locked = (lp->pooldblkp != NULL);

    tracef2(": descriptor handle %d ptr &%p]\n", lp->pooldblkh, lp->pooldblkp);

    if(lp->pooldblkh)
        {
        if((unsigned long) lp->pooldblkh >= (unsigned long) handlefree)
            /* not in table */
            valfatal(6, (memp) lp->pooldblkh, (memp) handlefree);

        if(lp->pooldblkp)
            if(lp->pooldblkp != (ptr = list_getptr(lp->pooldblkh)))
                /* cached ptr different to that in table */
                valfatal(7, lp->pooldblkp, ptr);
        }
    else
        {
        if(lp->pooldblkp)
            /* no handle but crap ptr */
            valfatal(5, lp->pooldblkp, NULL);
        }

    if(lp->pooldblkp)
        {
        if((bytep) lp->pooldblkp < (bytep) fl_start)
            valfatal(3, lp->pooldblkp, fl_start);

        if((bytep) lp->pooldblkp >= (bytep) fl_end)
            valfatal(4, lp->pooldblkp, fl_end);
        }

    pdp = lp->pooldblkp = list_getptr(lp->pooldblkh);

    if(pdp)
        { 
        tracef2("[Validating pools in descriptor &%p: free %d]\n", lp->pooldblkp, lp->pooldblkfree);

        for(i = 0; i < lp->pooldblkfree; ++i, ++pdp)
            {
            pool_was_locked = (pdp->pool != NULL);

            tracef5("[Validating pool (&%p[%d] = &%p): handle %d ptr &%p]\n", lp->pooldblkp, i, pdp, pdp->poolh, pdp->pool);

            if(pdp->poolh)
                {
                if((unsigned long) pdp->poolh >= (unsigned long) handlefree)
                    /* not in table */
                    valfatal(10, (void *) pdp->poolh, (void *) handlefree);

                if(pdp->pool)
                    if(pdp->pool != (ptr = list_getptr(pdp->poolh)))
                        /* cached ptr different to that in table */
                        valfatal(11, pdp->pool, ptr);
                }
            else
                {
                if(pdp->pool)
                    /* no handle but crap ptr */
                    valfatal(12, pdp->pool, NULL);
                }

            if(pdp->pool)
                {
                if((bytep) pdp->pool < (bytep) fl_start)
                    valfatal(13, pdp->pool, fl_start);

                if((bytep) pdp->pool >= (bytep) fl_end)
                    valfatal(14, pdp->pool, fl_end);
                }

            /* would be really tedious to check contents! */

            if(!pool_was_locked)
                {
                #if WINDOWS
                list_unlock_handle(pdp->poolh);
                #endif

                pdp->pool = NULL;
                }
            }

        if(!desc_was_locked)
            {
            #if WINDOWS
            list_unlock_handle(lp->pooldblkh);
            #endif

            lp->pooldblkp = NULL;
            }
        }
}

static void INTERNAL
validatepools(void)
{
    intl i;
    memp ptr;
    list_p lp;

    if(!validate_enabled)
        return;

    tracef1("[Validate handleblock[0..%d]: ", handlefree);
    cachebits();

    for(i = 0; i < handlefree; ++i)
        {
        ptr = handleblock[i];
        tracef2("[%d] = &%p  ", i, ptr);
        if(ptr)
            {
            if((bytep) ptr < (bytep) fl_start)
                valfatal(8, ptr, fl_start);

            if((bytep) ptr >= (bytep) fl_end)
                valfatal(9, ptr, fl_end);

            /* get flex to check for spillage */
            (void) flex_size((flex_ptr) &handleblock[i]);
            }
        }

    tracef0("\n[Validate registered pools]\n");

    for(lp = firstblock; lp; lp = lp->nextblock)
        _validatepool(lp, 0);
}

#endif /* SPECIAL_CASE */

extern void
list_toggle_validate(void)
{
    validate_enabled = !validate_enabled;
}

#endif /* SPARSE_VALIDATE_DEBUG */


/* end of handlist.c */
