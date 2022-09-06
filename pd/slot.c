/* slot.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/***********************************************************
*                                                          *
* module that travels around the PipeDream data structures *
*                                                          *
* updated for new matrix cell engine from CTYPE            *
* MRJC February 1989                                       *
*                                                          *
***********************************************************/

#include "datafmt.h"

#if ARTHUR || RISCOS
#include "ext.spell"
#elif MS
#include "spell.ext"
#endif

/*
function declarations
*/

extern mhandle alloc_handle_using_cache(word32 size);
extern void *  alloc_ptr_using_cache(word32 size, intl *resp);
extern intl  atend(colt col, rowt row);
extern rowt  atrow(colt tcol);
extern coord colwidth(colt col);
extern coord colwrapwidth(colt tcol);
extern BOOL  createcol(colt tcol);
extern intl  createhole(colt col, rowt row);
extern slotp createslot(colt tcol, rowt trow, intl size, uchar type);
extern void  delcol(colt col, colt size);
extern void  delcolandentry(colt tcol, colt size);
extern void  delcolentry(colt tcol, colt size);
extern void  delcolstart(colp tcolstart, colt size);
extern void  deregcoltab(void);
extern void  deregtempcoltab(colp tcolstart, colt size);
extern void  dstwrp(colt tcol, coord width);
extern void  garbagecollect(void);
extern BOOL  inscolentry(colt tcol);
extern intl  insertslot(dochandle han, colt tcol, rowt trow);
extern BOOL  insertslotat(colt tcol, rowt trow);
extern void  killcoltab(void);
extern void  killslot(colt tcol, rowt trow);
extern BOOL  moveslots(dochandle newdoc, colt newc, rowt newr,
                         dochandle olddoc, colt oldc, rowt oldr, rowt n);
extern slotp next_slot_in_block(BOOL direction);
extern void  pack_column(colt col);
extern void  readpcolvars(colt col, intl **widp, intl **wwidp);
extern mhandle realloc_handle_using_cache(mhandle han, word32 size);
extern void *  realloc_ptr_using_cache(void *ptr, word32 size, intl *resp);
extern void  rebnmr(void);
extern void  regcoltab(void);
extern void  regtempcoltab(colp tcolstart, colt size);
extern BOOL  restcoltab(void);
extern void  savecoltab(void);
extern intl  slotcontentssize(slotp tslot);
extern intl  slotsize(slotp tslot);
extern BOOL  swap_rows(rowt trow1, rowt trow2,
                         colt firstcol, colt lastcol, BOOL updaterefs);
extern slotp travel(colt tcol, rowt row);
extern slotp travel_here(void);

/*
internal functions
*/

typedef struct
{
    rowt  row;
    slotp sl;
    intl  size;
    uchar type;
}
swap_slot_struct;

/*static void copycont(slotp, slotp, intl);*/
static void default_col_entries(colp cp);
/*static void sortrefs(rowt trow1, rowt trow2, colt firstcol, colt lastcol);*/
static intl swap_slot(colt col, swap_slot_struct *p1, swap_slot_struct *p2, void *tempslot);

/*
macros
*/

#define indexcol(col)           (   (!colstart  ||  ((col) >= numcol))  \
                                                ? NULL                  \
                                                : (colstart + (col))    )

#define x_indexcollb(wdp, col)  (   (!(wdp)->Xcolstart  ||  ((col) >= (wdp)->Xnumcol))  \
                                                    ? NULL                              \
                                                    : &(((wdp)->Xcolstart + (col))->lb) )

#define indexcollb(col)         x_indexcollb(sb, col)


/**************************************************
*                                                 *
* allocate a block of memory and return a handle, *
* using the spell cache if needs be               *
*                                                 *
**************************************************/

extern mhandle
alloc_handle_using_cache(word32 size)
{
    #if !defined(SPELL_OFF)

    mhandle handle;
    intl res;

    if(!size)
        return(0);

    do  {
        handle = list_allochandle(size);

        if(handle)
            return(handle);

        res = spell_freemem();
        }
    while(res > 0);

    return((mhandle) res);

    #else

    return(list_allochandle(size));

    #endif
}

/***************************************************
*                                                  *
* allocate a block of memory and return a pointer, *
* using the spell cache if needs be                *
*                                                  *
***************************************************/

extern void *
alloc_ptr_using_cache(word32 size, intl *resp)
{
    #if !defined(SPELL_OFF)

    void *ptr;
    intl res = 0;

    if(resp)
        *resp = res;

    if(!size)
        return(NULL);

    do  {
        ptr = list_allocptr(size);

        if(ptr)
            return(ptr);

        if(resp)
            res = spell_freemem();

trace_on();
tracef3("[alloc_ptr_using_cache(%d, &%p) got NULL: res from spell_freemem %d\n", size, resp, res);
trace_off();
        }
    while(res > 0);

    if(resp)
        *resp = res;

    return(NULL);

    #else

    return(list_allocptr(size));

    #endif
}

/*******************************************************
*                                                      *
* return a flag indicating if past the end of a column *
*                                                      *
*******************************************************/

extern intl
atend(colt col, rowt row)
{
    return(row >= list_numitem(indexcollb(col)) ? TRUE : FALSE);
}

/******************************************************
*                                                     *
* return the current row number of a specified column *
*                                                     *
******************************************************/

extern rowt
atrow(colt tcol)
{
    return(list_atitem(indexcollb(tcol)));
}

/********************************
*                               *
* return column width of column *
*                               *
********************************/

extern coord
colwidth(colt col)
{
    colp cp;

    return(((cp = indexcol(col)) != NULL) ? cp->colwidth : 0);
}

/********************************
*                               *
* return wrap width of a column *
*                               *
********************************/

extern coord
colwrapwidth(colt tcol)
{
    colp cp;
    coord ww;

    ww = ((cp = indexcol(tcol)) == NULL) ? 0 : cp->wrapwidth;

    return(!ww ? colwidth(tcol) : ww);
}

/************************************************
*                                               *
*  copy the contents of one slot into another   *
*                                               *
************************************************/

static void
copycont(slotp nsl, slotp osl, intl size)
{
    uchar *oldtext, *newtext;

    nsl->type    = osl->type;
    nsl->flags   = osl->flags;
    nsl->justify = osl->justify;

    switch(nsl->type)
        {
        case SL_TEXT:
            oldtext = osl->content.text;
            newtext = nsl->content.text;
            break;

        case SL_PAGE:
            nsl->content.page.condval = osl->content.page.condval;
            return;

        default:
            nsl->content.number.result = osl->content.number.result;
            nsl->content.number.format = osl->content.number.format;
            oldtext = osl->content.number.text;
            newtext = nsl->content.number.text;
            break;
        }

    /* copy over formula and update it */
    memcpy(newtext, oldtext, size);
}

/**************************************************************************
*                                                                         *
* create a column                                                         *
* returns success value                                                   *
* note that colsintable is one less than the maximum numcol that will fit *
*                                                                         *
**************************************************************************/

extern BOOL
createcol(colt tcol)
{
    colp newblock, nc;
    colt y;
    colt newsize;

    /* col already exists */
    if(tcol < numcol)
        return(TRUE);

    if(tcol > LARGEST_COL_POSSIBLE)
        return(FALSE);

    tracef2("[createcol, numcol: %d, colsintable: %d]\n",
            numcol, colsintable);

    /* allocate new column table if table not big enough */
    newblock = NULL;

    if(tcol >= colsintable)
        {
        intl res;

        /* allocate new array */
        newsize  = tcol + 1;
        newblock = alloc_ptr_using_cache(sizeof(struct colentry) * (word32) newsize, &res);
        if(!newblock)
            return(reperr_null((res < 0) ? res : ERR_NOROOM));

        /* de-register old table */
        deregcoltab();

        /* copy across old table into the new one */
        if(colstart)
            memcpy(newblock,
                   colstart,
                   sizeof(struct colentry) * (size_t) numcol);

        /* must re-register copied info before freeing old colstart
         * as the free may move the objects pointed to by the list blocks
        */
        regtempcoltab(newblock, numcol);

        dispose((void **) &colstart);

        colsintable = newsize;
        colstart    = newblock;
        }

    tracef2("[createcol *, numcol: %d, colsintable: %d]\n",
            numcol, colsintable);

    /* clear out new entries */
    for(y = numcol, nc = (colstart + numcol); y < colsintable; y++, nc++)
        {
        default_col_entries(nc);
        if(y < tcol + 1)
            list_register(&nc->lb);
        }

    numcol = tcol + 1;

    tracef2("[createcol **, numcol: %d, colsintable: %d]\n",
            numcol, colsintable);

    return(TRUE);
}

/****************************************
*                                       *
* create a filler slot in the structure *
*                                       *
****************************************/

extern intl
createhole(colt col, rowt row)
{
    list_p lp;
    intl res;

    if(TRACE  &&  col == 15+9)
        {
        printf("before createcol(%d): numcol = %d\n", col, numcol);
        Debug_fn();
        trace_on();
        }

    /* make sure there is a column */
    if((col >= numcol)  &&  !createcol(col))
        return(NULL);

    lp = indexcollb(col);
    do
        {
        if(TRACE  &&  col == 15+9)
            {
            printf("before list_createitem()\n");
            Debug_fn();
            }

        if(list_createitem(lp, row, 0, FALSE))
            break;

        #ifndef SPELL_OFF

        tracef0("[createhole calling spell_freemem]\n");
        if((res = spell_freemem()) < 0)
            {
            reperr_module(ERR_SPELL, res);
            return(FALSE);
            }

        if(!res)
            return(FALSE);

        #else

        return(FALSE);

        #endif
        }
    while(TRUE);

    if(TRACE  &&  col == 15+9)
        {
        printf("after createitem()\n");
        Debug_fn();
        trace_off();
        }

    if(list_numitem(lp) > numrow)
        numrow = list_numitem(lp);

    return(TRUE);
}

/*****************************************************************************
*                                                                            *
* Create a slot for a particular column and row of a given size and type.    *
*                                                                            *
* the size must include any extra space required apart from the slot         *
* overhead itself: a size of 0 means a blank slot; a size of 1 leaves space  *
* for a delimiter.                                                           *
*                                                                            *
* the type field is used only to set the type of the resulting slot; no size *
* information is derived from the type.                                      *
*                                                                            *
*****************************************************************************/

extern slotp
createslot(colt col, rowt row, intl size, uchar type)
{
    list_itemp it;
    slotp sl;
    list_p lp;
    intl res;

    /* make sure there is a column */
    if(col >= numcol && !createcol(col))
        return(NULL);

    size += ((type == SL_TEXT) ? SL_TEXTOVH : SL_NUMBEROVH);

    sl = NULL;
    lp = indexcollb(col);
    do
        {
        if((it = list_createitem(lp, row, size, FALSE)) != NULL)
            {
            sl = (slotp) it->i.inside;

            /* set type of final slot */
            if((sl->type = type) == SL_NUMBER || type == SL_BAD_FORMULA)
                {
                sl->content.number.result.value = 0.;
                sl->content.number.format = 0;
                }

            sl->flags = 0;
            sl->justify = J_FREE;
            sl->content.text[0] = '\0';

            break;
            }

        #ifndef SPELL_OFF

        tracef0("[createslot calling spell_freemem]\n");
        if((res = spell_freemem()) < 0)
            {
            reperr_module(ERR_SPELL, res);
            break;
            }

        if(!res)
            break;

        #else

        break;

        #endif
        }
    while(TRUE);

    if(list_numitem(lp) > numrow)
        numrow = list_numitem(lp);

    return(sl);
}

/***************************************
*                                      *
* set column entries to default values *
*                                      *
***************************************/

static void
default_col_entries(colp cp)
{
    list_init(&cp->lb, MAXITEMSIZE, MAXPOOLSIZE);
    cp->colwidth  = DEFAULTWIDTH;
    cp->wrapwidth = DEFAULTWRAPWIDTH;
}

/*******************************************
*                                          *
* delete all the slots in a set of columns *
*                                          *
*******************************************/

extern void
delcol(colt tcol, colt size)
{
    colt first_col;
    list_p lp;

    size = min(size, numcol - tcol);

    if(size <= 0)
        return;

    first_col = tcol;
    tcol += size;

    while(--tcol >= first_col)
        {
        lp = indexcollb(tcol);

        if(lp)
            list_free(lp);
        }
}

/************************************************************************************
*                                                                                   *
* delete all the slots and entries in a set of column entries, closing up the table *
*                                                                                   *
************************************************************************************/

extern void
delcolandentry(colt tcol, colt size)
{
    delcol(tcol, size);
    delcolentry(tcol, size);
}

/*******************************************************
*                                                      *
* delete a set of column entries, closing up the table *
*                                                      *
*******************************************************/

extern void
delcolentry(colt tcol, colt size)
{
    size = min(size, numcol - tcol);

    if(size <= 0)
        return;

    /* remove references to table */
    deregcoltab();

    /* close up table */
    memmove(colstart + tcol,
            colstart + tcol + size,
            (size_t) (numcol - (tcol + size)) * sizeof(struct colentry));

    minusab(numcol, size);

    /* reregister table */
    regcoltab();
}

/*************************************
*                                    *
* delete all columns and free memory *
* associated with a column array     *
*                                    *
*************************************/

extern void
delcolstart(colp tcolstart, colt size)
{
    if((NULL != tcolstart) && (0 != size)) /* FIX 06.10.2021 as PD4 */
    {
        colp cp = tcolstart + size;

        while(--cp >= tcolstart)
            list_free(&cp->lb);

        deregtempcoltab(tcolstart, size);

        free(tcolstart);
    }
}

/****************************************************
*                                                   *
*  deregister all the columns in the column table   *
*                                                   *
****************************************************/

extern void
deregcoltab(void)
{
    deregtempcoltab(colstart, numcol);
}

/***************************************************
*                                                  *
* deregister all columns in temporary column table *
*                                                  *
***************************************************/

extern void
deregtempcoltab(colp tcolstart, colt size)
{
    if((NULL != tcolstart) && (0 != size)) /* FIX 06.10.2021 as PD4 */
    {
        colp cp = tcolstart + size;

        while(--cp >= tcolstart)
            list_deregister(&cp->lb);
    }
}

/****************************************************************************
*                                                                           *
*  distribute a given wrap width among columns to right until all used up   *
*                                                                           *
****************************************************************************/

extern void
dstwrp(colt tcol, coord width)
{
    colp colinfo;

    while((tcol < numcol)  &&  (width >= 0))
        {
        colinfo = indexcol(tcol);
        colinfo->wrapwidth = width;
        width -= colinfo->colwidth;
        ++tcol;
        }

    xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
    filealtered(TRUE);
}


/***************************************
*                                      *
* garbage collect in the sparse matrix *
*                                      *
***************************************/

extern void
garbagecollect(void)
{
    colt tcol = numcol;

    while(--tcol >= 0)
        while(list_garbagecollect(indexcollb(tcol)))
            ;
}

/*******************************
*                              *
* insert column entry in table *
* returns success value        *
*                              *
*******************************/

extern BOOL
inscolentry(colt tcol)
{
    if(!createcol(numcol))
        return(FALSE);

    deregcoltab();

    /* note that createcol increments numcol */
    memmove(colstart + tcol + 1,
            colstart + tcol,
            (size_t) (numcol - (tcol + 1)) * sizeof(struct colentry));

    default_col_entries(colstart + tcol);

    regcoltab();

    return(TRUE);
}

/**************************************************************************
*                                                                         *
* insert a new blank slot at this position                                *
* return TRUE if slot inserted or no need to because after end of column  *
* return FALSE if no room                                                 *
*                                                                         *
**************************************************************************/

extern intl
insertslot(dochandle han, colt col, rowt row)
{
    list_p lp;
    intl res;
    window_data *wdp;

    tracef3("[insertslot: %d, %d, %d]\n", han, col, row);

    wdp = find_document_using_handle(han);

    lp = x_indexcollb(wdp, col);

    do  {
        if(!list_insertitems(lp, row, (rowt) 1))
            break;

        #ifndef SPELL_OFF

        tracef0("[insertslot calling spell_freemem]\n");
        if((res = spell_freemem()) < 0)
            {
            reperr_module(ERR_SPELL, res);
            return(res);
            }

        if(!res)
            return(ERR_NOROOM);

        #else

        return(ERR_NOROOM);

        #endif
        }
    while(TRUE);

    if(list_numitem(lp) > numrow)
        numrow = list_numitem(lp);

    filealtered(TRUE);

    return(TRUE);
}

extern BOOL
insertslotat(colt col, rowt row)
{
    intl res = insertslot(current_document_handle(), col, row);

    if(res < 0)
        return(reperr_null(res));

    return(TRUE);
}

/********************
*                   *
* kill column table *
*                   *
********************/

extern void
killcoltab(void)
{
    deregcoltab();

    dispose((void **) &colstart);

    numcol = colsintable = 0;
    colstart = NULL;
}

/****************
*               *
* delete a slot *
*               *
****************/

extern void
killslot(colt col, rowt row)
{
    list_p lp;

    lp = indexcollb(col);

    if(row >= list_numitem(lp))
        return;

    list_deleteitems(lp, row, (rowt) 1);
}

/*********************************************
*                                            *
* move the n slots at oldc,oldr to newc,newr *
*                                            *
*********************************************/

extern intl
moveslots(dochandle newdoc, colt newc, rowt newr,
          dochandle olddoc, colt oldc, rowt oldr,
          rowt n)
{
    colt ncol = newc;
    rowt nrow = newr;
    colt ocol = oldc;
    rowt orow = oldr;
    rowt i;
    slotp osl, nsl;
    uchar type;
    intl size;
    dochandle curhan;
    intl res = 1;
    window_data *wdp;

    tracef5("[moveslots newc: %d, newr: %d, oldc: %d, oldr: %d, n: %d]\n",
            newc, newr, oldc, oldr, n);

    curhan = current_document_handle();

    select_document_using_handle(newdoc);

    /* loop for each row */
    for(i = 0; i < n; i++, newr++)
        {
        tracef1("[moveslots newr: %d]\n", newr);

        osl = travel_externally(olddoc, oldc, oldr);
        if(osl)
            {
            size = slotcontentssize(osl);
            type = osl->type;
            }

        if(newr < list_numitem(indexcollb(newc)))
            {
            res = insertslot(newdoc, newc, newr);

            if(res < 0)
                break;

            if(list_numitem(indexcollb(newc)) > numrow)
                numrow = list_numitem(indexcollb(newc));

            if((newdoc == olddoc)  &&  (oldc == newc)  &&  (oldr >= newr))
                ++oldr;
            }

        if(osl)
            {
            nsl = createslot(newc, newr, size, type);

            if(!nsl)
                {
                res = ERR_NOROOM;
                break;
                }

            /* reload old pointer */
            osl = travel_externally(olddoc, oldc, oldr);
            mark_slot_as_moved(nsl);
            copycont(nsl, osl, size);
            }
        else
            {
            if(!createhole(newc, newr))
                {
                res = ERR_NOROOM;
                break;
                }
            }

        wdp = find_document_using_handle(olddoc);

        if(oldr < list_numitem(x_indexcollb(wdp, oldc)))
            {
            list_deleteitems(x_indexcollb(wdp, oldc), oldr, (rowt) 1);

            if((newdoc == olddoc) && (oldc == newc) && (newr >= oldr))
                --newr;
            }
        }

    graph_send_xblock(ocol, orow, ocol, orow + n, olddoc);
    graph_send_xblock(ncol, nrow, ncol, nrow + n, newdoc);

    select_document_using_handle(curhan);

    return(res);
}

/*****************************************
*                                        *
* speedy next_in_block for sparse matrix *
* returns next slot address              *
*                                        *
*****************************************/

extern slotp
next_slot_in_block(BOOL direction)
{
    static list_itemp it;
    slotp sl;

    if(direction == DOWN_COLUMNS)
        {
        /* always return first slot - perhaps current slot */
        if(start_block)
            {
            start_block = FALSE;

            it = list_initseq(indexcollb(in_block.col), &in_block.row);

            if(it  &&
                /* RJM 27.5.89 notices that this doesn't necessarily get
                    single or no marker case right because end_bl.row could
                    be anything (esp.0). So explicit test here for one or no
                    markers.
                */
                ((end_bl.col == NO_COL)  ||  (in_block.row <= end_bl.row))
               )
                return((slotp) it->i.inside);
            }

        /* for zero or one markers, only slot returned above */
        if(end_bl.col == NO_COL)
            return(NULL);

        if(it)
            {
            it = list_nextseq(indexcollb(in_block.col), &in_block.row);

            tracef3("list_nextseq returned &%p and row %d: end_bl.row = %d\n", it, in_block.row, end_bl.row);

            if(it  &&  (in_block.row <= end_bl.row))
                return((slotp) it->i.inside);
            }

        /* start a new column */
        do  {
            if(++in_block.col > end_bl.col)
                return(NULL);

            in_block.row = start_bl.row;

            it = list_initseq(indexcollb(in_block.col), &in_block.row);

            if(it  &&  (in_block.row <= end_bl.row))
                return((slotp) it->i.inside);
            }
        while(TRUE);
        }
    else
        {
        if(start_block)
            {
            start_block = FALSE;
            sl = travel_in_block();
            if(sl)
                return(sl);
            }

        if(end_bl.col == NO_COL)
            return(NULL);

        do  {
            if(++in_block.col > end_bl.col)
                {
                /* off rhs so reset */
                in_block.col = start_bl.col;
                if(++in_block.row > end_bl.row)
                    return(NULL);
                }

            sl = travel_in_block();

            if(sl)
                return(sl);
            }
        while(TRUE);
        }
}

/******************************************
*                                         *
* pack a column, releasing any free space *
*                                         *
******************************************/

extern void
pack_column(colt col)
{
    list_packlist(indexcollb(col), -1);
}

/************************************
*                                   *
* read pointers to column variables *
*                                   *
************************************/

extern void
readpcolvars(colt col, intl **widp, intl **wwidp)
{
    colp cp;

    cp = indexcol(col);
    *widp = &cp->colwidth;
    *wwidp = &cp->wrapwidth;
}

/****************************************************
*                                                   *
* reallocate a block of memory and return a handle, *
* using the spell cache if needs be                 *
*                                                   * 
****************************************************/

extern mhandle
realloc_handle_using_cache(mhandle han, word32 size)
{
    #if !defined(SPELL_OFF)

    mhandle handle;
    intl res;

    if(!han)
        return(alloc_handle_using_cache(size));

    if(!size)
        {
        list_deallochandle(han);
        return(0);
        }

    do  {
        handle = list_reallochandle(han, size);

        if(handle)
            return(handle);

        res = spell_freemem();
        }
    while(res > 0);

    return((mhandle) res);

    #else

    return(list_reallochandle(han, size));

    #endif
}

/*****************************************************
*                                                    *
* reallocate a block of memory and return a pointer, *
* using the spell cache if needs be                  *
*                                                    * 
*****************************************************/

extern void *
realloc_ptr_using_cache(void *ptr, word32 size, intl *resp)
{
    #if !defined(SPELL_OFF)

    intl res = 0;

    if(!ptr)
        return(alloc_ptr_using_cache(size, resp));

    if(resp)
        *resp = res;

    if(!size)
        {
        list_deallocptr(ptr);
        return(NULL);
        }

    do  {
        ptr = list_reallocptr(ptr, size);

        if(ptr)
            return(ptr);

        if(resp)
            res = spell_freemem();
        }
    while(res > 0);

    if(resp)
        *resp = res;

    return(NULL);

    #else

    return(list_reallocptr(ptr, size));

    #endif
}

/****************************************************
*                                                   *
* rebuild numrow because of insertions or deletions *
*                                                   *
****************************************************/

extern void
rebnmr(void)
{
    colt col;
    rowt maxrow;

    for(col = 0, maxrow = 0; col < numcol; col++)
        maxrow = max(maxrow, list_numitem(indexcollb(col)));

    numrow = maxrow ? maxrow : 1;
}

/************************************************
*                                               *
* register all the columns in the column table  *
*                                               *
************************************************/

extern void
regcoltab(void)
{
    regtempcoltab(colstart, numcol);
}

/*****************************************************
*                                                    *
* register all the columns in temporary column table *
*                                                    *
*****************************************************/

extern void
regtempcoltab(colp tcolstart, colt size)
{
    colt i;
    colp cp;

    for(i = 0, cp = tcolstart; i < size; ++i, ++cp)
        list_register(&cp->lb);
}

/***********************
*                      *
* restore column table *
*                      *
***********************/

extern BOOL
restcoltab(void)
{
    colp d_colstart = def_colstart;
    colt d_numcol = def_numcol;

    if(!d_colstart)
        return(TRUE);

    if(!createcol(d_numcol - 1))
        return(FALSE);

    deregcoltab();

    memcpy(colstart, d_colstart, sizeof(struct colentry) * (size_t) d_numcol);

    regcoltab();

    return(TRUE);
}

/********************
*                   *
* save column table *
*                   *
********************/

extern void
savecoltab(void)
{
    deregcoltab();

    def_numcol = numcol;
    def_colstart = colstart;

    colstart = NULL;
}

/****************************************************************************
*                                                                           *
* return size of contents of slot in bytes, including necessary termination *
*                                                                           *
****************************************************************************/

extern intl
slotcontentssize(slotp tslot)
{
    intl type = tslot->type;
    intl size = slotsize(tslot);
    return(size - ((type == SL_TEXT) ? SL_TEXTOVH : SL_NUMBEROVH));
}

/*******************************************************
*                                                      *
* return size of slot in bytes, including any overhead *
*                                                      *
*******************************************************/

extern intl
slotsize(slotp tslot)
{
    uchar *p;
    uchar *ptr;

    switch((int) tslot->type)
        {
        case SL_TEXT:
            {
            ptr = p = tslot->content.text;

            while(*ptr)
                if(*ptr++ == SLRLDI)
                    ptr += SLRSIZE - 1;

            return(SL_TEXTOVH + 1 + (ptr - p));
            }

        case SL_ERROR:
        case SL_NUMBER:
        case SL_STRVAL:
        case SL_INTSTR:
        case SL_DATE:
            return(SL_NUMBEROVH +
                   exp_len(tslot->content.number.text));

        case SL_BAD_FORMULA:
            return(SL_NUMBEROVH +
                   1 + strlen((char *) tslot->content.number.text));

        default:
            return(SL_NUMBEROVH);
        }
}

/****************************************************
*                                                   *
* update references when rows are exchanged in sort *
* updated for external references 30.5.89 MRJC      *
*                                                   *
****************************************************/

static void
sortrefs(rowt trow1, rowt trow2, colt firstcol, colt lastcol)
{
    dochandle curhan;
    docno curdoc, doc;
    intl index, num_dep_docs;
    uchar *rptr;
    rowt diff1 = trow1 - trow2;
    rowt diff2 = trow2 - trow1;
    slotp tslot;
    rowt rref;
    colt cref;
    docno dref;

    #if RISCOS
    draw_swaprefs(trow1, trow2, firstcol, lastcol);
    #endif

    curhan = current_document_handle();
    if((num_dep_docs = init_dependent_docs(&curdoc, &index)) == 0)
        {
        tracef0("[sortrefs num_dep_docs = 0: no action]\n");
        return;
        }

    tree_swaprefs(trow1, trow2, firstcol, lastcol);

    /* loop for each dependent sheet */
    do  {
        do  {
            if(num_dep_docs)
                {
                doc = next_dependent_doc(&curdoc, &index);
                --num_dep_docs;
                if(check_docvalid(doc) >= 0)
                    {
                    switch_document(doc);
                    break;
                    }
                }
            doc = curdoc;
            }
        while(num_dep_docs);

        recalc_bit = TRUE;

        /* for every slot in spreadsheet */
        init_doc_as_block();

        while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
            {
            tracef3("[sortrefs: tslot := &%p (%d, %d)]\n", tslot, in_block.col, in_block.row);

            if(tslot->flags & SL_REFS)
                {
                rptr = (tslot->type == SL_TEXT)
                                ? tslot->content.text
                                : tslot->content.number.text;

                my_init_ref(tslot->type);

                while((rptr = my_next_ref(rptr, tslot->type)) != NULL)
                    {
                    /* for each slot reference */
                    dref = (docno) talps(rptr, sizeof(docno));
                    rptr += sizeof(docno);

                    /* check if reference in column range */
                    cref = (colt) talps(rptr, sizeof(colt));

                    if( (!dref && (doc != curdoc))      ||
                        ( dref && (dref != curdoc))     ||
                        ( cref &  COLNOBITS) < firstcol ||
                        ( cref &  COLNOBITS) > lastcol
                       )
                        {
                        rptr += sizeof(colt) + sizeof(rowt);
                        continue;
                        }

                    rref = (rowt) talps((rptr += sizeof(colt)), sizeof(rowt));

                    /* change from first row to second row,
                     * keeping special bits
                    */
                    if((rref & ROWNOBITS) == trow1)
                        {
                        splat(rptr, (word32) (rref+diff2), sizeof(rowt));
                        filealtered(TRUE);
                        }
                    /* change from second row to first row,
                     * keeping special bits
                    */
                    elif((rref & ROWNOBITS) == trow2)
                        {
                        splat(rptr, (word32) (rref+diff1), sizeof(rowt));
                        filealtered(TRUE);
                        }

                    rptr += (int) sizeof(rowt);
                    }
                }
            }
        }
    while(num_dep_docs);

    select_document_using_handle(curhan);
}

/*******************************************************
*                                                      *
* swap the rows, can assume slots exist in all columns *
* trow1 is guaranteed to be higher up than trow2       *
*                                                      *
*******************************************************/

extern BOOL
swap_rows(rowt trow1, rowt trow2,
          colt firstcol, colt lastcol,
          BOOL updaterefs)
{
    colt col;
    char tempslot[LIN_BUFSIZ + 1 + SL_NUMBEROVH];
    swap_slot_struct s1, s2;

    tracef2("[swap rows: %d, %d]\n", (intl) trow1, (intl) trow2);

    s1.row = trow1;
    s2.row = trow2;

    for(col = firstcol; col <= lastcol; ++col)
        {
        tracef1("[swap_rows: col %d]\n", col);

        s1.sl = travel(col, s1.row);
        if(s1.sl)
            {
            s1.size = slotsize(s1.sl);
            s1.type = s1.sl->type;
            mark_slot_as_moved(s1.sl);
            tracef2("[swap_rows moved: %d, %d]\n", col, s1.row);
            }
        else
            s1.size = 0;

        s2.sl = travel(col, s2.row);
        if(s2.sl)
            {
            s2.size = slotsize(s2.sl);
            s2.type = s2.sl->type;
            mark_slot_as_moved(s2.sl);
            tracef2("[swap_rows moved: %d, %d]\n", col, s2.row);
            }
        else
            s2.size = 0;

        /* swapo the slots */
        if(s1.size > s2.size)
            {
            tracef2("[swap_rows: size1 %d > size2 %d]\n", s1.size, s2.size);
            if(!swap_slot(col, &s1, &s2, tempslot))
                return(FALSE);
            }
        elif(s1.size < s2.size)
            {
            tracef2("[swap_rows: size1 %d < size2 %d]\n", s1.size, s2.size);
            if(!swap_slot(col, &s2, &s1, tempslot))
                return(FALSE);
            }
        /* same size, non-zero? */
        elif(s1.size)
            {
            tracef1("[swap_rows: same size %d]\n", s1.size);
            memcpy(tempslot, s2.sl,    s2.size);
            memcpy(s2.sl,    s1.sl,    s1.size);
            memcpy(s1.sl,    tempslot, s2.size);
            }

        if(s1.size || s2.size)
            {
            /* no point transmitting two identical null slots, eh? */
            graph_send_slot(col, s1.row);
            graph_send_slot(col, s2.row);
            }
        }

    tracef0("[swap_rows: update references perhaps]\n");
    if(updaterefs)
        sortrefs(s1.row, s2.row, firstcol, lastcol);

    tracef0("[swap_rows out]\n");

    return(TRUE);
}


/***************************************************
*                                                  *
* helper routine for swap rows - to swap two slots *
*                                                  *
***************************************************/

static intl
swap_slot(colt col, swap_slot_struct *p1, swap_slot_struct *p2, void *tempslot)
{
    if(p2->size)
        memcpy(tempslot, p2->sl, p2->size);

    if((p2->sl = createslot(col, p2->row, p1->size, p1->type)) == NULL)
        return(FALSE);

    p1->sl = travel(col, p1->row);

    memcpy(p2->sl, p1->sl, p1->size);

    if(!p2->size)
        return(createhole(col, p1->row));

    if((p1->sl = createslot(col, p1->row, p2->size, p2->type)) == NULL)
        return(FALSE);

    memcpy(p1->sl, tempslot, p2->size);

    return(TRUE);
}


/*************************************************************************
*                                                                        *
* travel to a particular row, column                                     *
*                                                                        *
* returns pointer to slot if it exists.                                  *
* Returns NULL if column doesn't exist                                   *
* Returns NULL if column does exist, but row doesn't.  In this case, new *
* position stored in column vector.                                      *
*                                                                        *
* the function atrow() must then be called to return the actual          *
* row that was achieved                                                  *
*                                                                        *
*************************************************************************/

extern slotp
travel_here(void)
{
    return(travel(curcol, currow));
}


extern slotp
travel_in_block(void)
{
    return(travel(in_block.col, in_block.row));
}


extern slotp
travel(colt col, rowt row)
{
    list_itemp it = list_gotoitem(indexcollb(col), row);

    return(it ? (slotp) it->i.inside : NULL);
}


extern slotp
travel_externally(dochandle han, colt col, rowt row)
{
    window_data *wdp = find_document_using_handle(han);
    list_itemp it = list_gotoitem(x_indexcollb(wdp, col), row);

    return(it ? (slotp) it->i.inside : NULL);
}

/* end of slot.c */
