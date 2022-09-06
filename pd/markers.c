/* markers.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       markers.c - marked block handling
 * Author:      RJM August 1987
 * History:
 *  0.01    12-May-89   SKS derived from various bits of cursmov.c etc.
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "wimpt.h"
#include "font.h"
#if TRACE
#include "akbd.h"
#include "werr.h"
#endif
#elif MS
#else
    assert(0);
#endif


#include "datafmt.h"
#include "ext.pd"


#if RISCOS
#include "riscdraw.h"
#include "ext.riscos"
#endif


/* ----------------------------------------------------------------------- */

#define TRACE_DRAG (TRACE && TRUE)
#define TRACE_MARK (TRACE && TRUE)


/* previously marked rectangle - intersect for fast updates */
static SLR old_blkstart;
static SLR old_blkend;


/* sort a pair of column/row objects into numerical order */

static void
sort_colt(colt *aa, colt *bb)
{
    colt a = *aa;
    colt b = *bb;

    if(b < a)
        {
        *bb = a;
        *aa = b;
        }
}


static void
sort_rowt(rowt *aa, rowt *bb)
{
    rowt a = *aa;
    rowt b = *bb;

    if(b < a)
        {
        *bb = a;
        *aa = b;
        }
}


/********************************************
*                                           *
*   rectangle of marked slots has changed   *
*                                           *
* --in--                                    *
*   assumes mergebuf has been done much     *
*   further up for slot correctness         *
*                                           *
*                                           *
* --out--                                   *
*   flag indicates that some slots need     *
*   redrawing rather than merely inverting  *
*                                           *
********************************************/

static BOOL
new_marked_rectangle(void)
{
    #if RISCOS
    BOOL mustdraw = (log2bpp == 3); /* can't invert in 256 colour mode */
    #endif
    BOOL needs_drawsome = FALSE;
    coord coff, start_coff, end_coff;
    coord roff;
    colt  tcol;
    rowt  trow;
    slotp tslot;
    BOOL in_new_rowr, in_old_rowr;
    BOOL in_new_rect, in_old_rect;
    SCRCOL *cptr;
    SCRROW *rptr;
    coord c_width, overlap;

    vtracef4(TRACE_MARK, "new_marked_rectangle(%d, %d, %d, %d): ",
            blkstart.col, blkstart.row, blkend.col, blkend.row);
    vtracef4(TRACE_MARK, "old rectangle was (%d, %d, %d, %d)\n",
            old_blkstart.col, old_blkstart.row,
            old_blkend.col,   old_blkend.row);

    /* loop over all visible slots and 'update' slots in
     * visible range of changing rows and columns
    */
    #if MS
    if(current_document_handle() == front_doc)
    #endif
    for(roff = 0; !((rptr = vertvec_entry(roff))->flags & LAST); roff++)
        if(!(rptr->flags & PAGE))
            {
            start_coff = end_coff = -1;

            vtracef2(TRACE_MARK, "row offset %d, PICT | UNDERPICT = %d\n",
                        roff, rptr->flags & (PICT | UNDERPICT));

            trow = rptr->rowno;

            tslot = travel(col_number(0), trow);
            if(tslot  &&  (tslot->type == SL_PAGE))
                continue;

            in_new_rowr =   (blkstart.row <= trow)      &&
                            (trow <= blkend.row);

            in_old_rowr =   (old_blkstart.row <= trow)  &&
                            (trow <= old_blkend.row);

            for(coff = 0; !((cptr = horzvec_entry(coff))->flags & LAST); coff++)
                {
                vtracef1(TRACE_MARK, "column offset %d\n", coff);

                tcol = cptr->colno;

                in_new_rect =   (blkstart.col <= tcol)      &&
                                (tcol <= blkend.col)        &&
                                in_new_rowr;

                in_old_rect =   (old_blkstart.col <= tcol)  &&
                                (tcol <= old_blkend.col)    &&
                                in_old_rowr;

                if(in_new_rect != in_old_rect)
                    {
#if MS
                    mark_row(roff);
                    draw_screen();

                    if(xf_interrupted)
                        goto NO_MORE_AT_ALL;

                    goto NO_MORE_THIS_ROW;
#elif RISCOS
                    if( mustdraw  ||                                                /* forced to draw? */
                        (rptr->flags & (PICT | UNDERPICT))  ||                      /* picture on row? */
                        (   ((tslot = travel(tcol, trow)) == NULL)
                                ?   is_overlapped(coff, roff)                           /* empty slot overlapped from left? */
                                :   (   /*riscos_fonts  ||*/                                /* slot with fonts needs drawing */
                                        (   (tslot->type != SL_TEXT)                    /* text slots might need drawing: */
                                            ? FALSE
                                            :   (   ((c_width = colwidth(tcol)) < (overlap = chkolp(tslot, tcol, trow)))  ||    /* if slot overlaps to right */
                                                    (FALSE  &&  grid_on  &&  (c_width > overlap))                                           /* or if grid & shorter than colwidth */
                                                )
                                        )
                                    )
                        )
                      )
                        {
                        tracef5("disasterville: draw this whole row and breakout because mustdraw %s, PICT %s, tslot &%p -> empty overlapped %s, text | fonts %s\n",
                                trace_boolstring(mustdraw),
                                trace_boolstring(rptr->flags & (PICT | UNDERPICT)),
                                tslot,
                                trace_boolstring(tslot ? FALSE : is_overlapped(coff, roff)),
                                trace_boolstring(tslot ? ((tslot->type == SL_TEXT) || riscos_fonts) : FALSE)
                                );
                        mark_row(roff);
                        draw_screen();

                        if(xf_interrupted)
                            goto NO_MORE_AT_ALL;

                        goto NO_MORE_THIS_ROW;
                        }
                    else
                        {
                        intl fg = FORE;
                        intl bg = BACK;

                        if(tslot)
                            {
                            if(tslot->justify & PROTECTED)
                                {
                                tracef0("slot is protected\n");
                                bg = PROTECTC;
                                }

                            if( (tslot->type == SL_NUMBER)  &&
                                (tslot->content.number.result.value < 0.0))
                                {
                                tracef0("slot is negative\n");
                                fg = NEGATIVEC;
                                }
                            }

                        /* can buffer up inversions */
                        if((fg == FORE)  &&  (bg == BACK))
                            {
                            if(start_coff == -1)
                                {
                                tracef1("starting buffering up at coff %d\n", coff);
                                start_coff = coff;
                                }

                            tracef1("adding coff %d to end of buffer\n", coff);
                            end_coff = coff;
                            }
                        else
                            {
                            if(start_coff != -1)
                                {
                                tracef2("flush inverted section because colour change %d - %d\n", start_coff, end_coff);
                                please_invert_numeric_slots(start_coff, end_coff, roff, FORE, BACK);
                                start_coff = -1;
                                }

                            please_invert_numeric_slot(coff, roff, fg, bg);
                            }
                        }
#endif  /* RISCOS */
                    }
                elif(start_coff != -1)
                    {
                    tracef2("flush inverted section because rectangle edge %d - %d\n", start_coff, end_coff);
                    please_invert_numeric_slots(start_coff, end_coff, roff, FORE, BACK);
                    start_coff = -1;
                    }
                }   /* loop over cols */

            if(start_coff != -1)
                {
                tracef2("flush inverted section because row end %d - %d\n", start_coff, end_coff);
                please_invert_numeric_slots(start_coff, end_coff, roff, FORE, BACK);
                }

        NO_MORE_THIS_ROW:
            ;
            }   /* loop over rows */

NO_MORE_AT_ALL:
    ;

    return(needs_drawsome);
}


/********************************************
*                                           *
*  alter the active edge of a marked block  *
*  updating the screen to reflect this      *
*                                           *
********************************************/

extern void
alter_marked_block(colt nc, rowt nr)
{
    BOOL update = FALSE;

    vtracef2(TRACE_DRAG, "alter_marked_block(%d, %d)\n", nc, nr);

    old_blkstart = blkstart;            /* current marked block */
    old_blkend   = blkend;

    if(blkanchor.col == blkstart.col)
        {
        vtracef0(TRACE_DRAG, "marked area at (or right of) anchor: ");
        if(nc > blkend.col)
            {
            vtracef0(TRACE_DRAG, "end mark moving even further right\n");
            blkend.col = nc;
            update = TRUE;
            }
        elif(nc < blkend.col)
            {
            if(nc > blkanchor.col)
                {
                vtracef0(TRACE_DRAG, "end mark moving left a little\n");
                blkend.col = nc;
                }
            else
                {
                vtracef0(TRACE_DRAG, "end mark moved to (or left over) anchor: flip\n");
                blkstart.col = nc;
                blkend.col   = blkanchor.col;
                }
            update = TRUE;
            }
        else
            vtracef0(TRACE_DRAG, "no col change\n");
        }
    else
        {
        vtracef0(TRACE_DRAG, "marked area left of anchor: ");
        if(nc < blkstart.col)
            {
            vtracef0(TRACE_DRAG, "start mark moving even further left\n");
            blkstart.col = nc;
            update = TRUE;
            }
        elif(nc > blkstart.col)
            {
            if(nc < blkanchor.col)
                {
                vtracef0(TRACE_DRAG, "start mark moving right a little\n");
                blkstart.col = nc;
                }
            else
                {
                vtracef0(TRACE_DRAG, "start mark moved to (or right over) anchor: flip\n");
                blkstart.col = blkanchor.col;
                blkend.col   = nc;
                }
            update = TRUE;
            }
        else
            vtracef0(TRACE_DRAG, "no col change\n");
        }


    if(blkanchor.row == blkstart.row)
        {
        vtracef0(TRACE_DRAG, "current marked area at (or below) anchor: ");
        if(nr > blkend.row)
            {
            vtracef0(TRACE_DRAG, "end mark moving even further down\n");
            blkend.row = nr;
            update = TRUE;
            }
        elif(nr < blkend.row)
            {
            if(nr > blkstart.row)
                {
                vtracef0(TRACE_DRAG, "end mark moving up a little, still below anchor\n");
                blkend.row = nr;
                }
            else
                {
                vtracef0(TRACE_DRAG, "end mark moved up to (or above) anchor: flip\n");
                blkstart.row = nr;
                blkend.row   = blkanchor.row;
                }
            update = TRUE;
            }
        else
            vtracef0(TRACE_DRAG, "no row change\n");
        }
    else
        {
        vtracef0(TRACE_DRAG, "current marked area above anchor: ");
        if(nr < blkstart.row)
            {
            vtracef0(TRACE_DRAG, "start mark moving even further up\n");
            blkstart.row = nr;
            update = TRUE;
            }
        elif(nr > blkstart.row)
            {
            if(nr < blkend.row)
                {
                vtracef0(TRACE_DRAG, "start mark moving down a little\n");
                blkstart.row = nr;
                }
            else
                {
                vtracef0(TRACE_DRAG, "start mark moved down to (or below) anchor: flip\n");
                blkstart.row = blkanchor.row;
                blkend.row   = nr;
                }
            update = TRUE;
            }
        else
            vtracef0(TRACE_DRAG, "no row change\n");
        }

    if(update)
        if(new_marked_rectangle())
            {
            /* update the screen if some slots need redrawing */
            xf_drawsome = TRUE;
            #if MS
                if(current_document_handle() == front_doc)
            #endif
                    draw_screen();
            }
}


/************************
*                       *
* clear marked block    *
*  (updates screen)     *
*                       *
************************/

extern void
clear_markers(void)
{
    vtracef0(TRACE_MARK, "clear_markers()\n");

    if(blkstart.col != NO_COL)
        {
        dochandle doc = current_document_handle();

        /* update right document's window */
        select_document_using_handle(blkdochandle);

        make_single_mark_into_block();  /* in case just one set */

        old_blkstart = blkstart;
        old_blkend   = blkend;

        /* no marks */
        blkstart.col = blkend.col = NO_COL;
        blkstart.row = blkend.row = (rowt) -1;

        if(new_marked_rectangle())
            {
            xf_drawsome = TRUE;
            #if MS
                if(current_document_handle() == front_doc)
            #endif
                    draw_screen();
            }

        select_document_using_handle(doc);
        }
    else
        vtracef0(TRACE_MARK, "no mark(s) set\n");
}


/********************************************************
*                                                       *
* if just one mark is set, make it into a marked block  *
*                                                       *
********************************************************/

extern void
make_single_mark_into_block(void)
{
    if((blkstart.col != NO_COL)  &&  (blkend.col == NO_COL))
        {
        vtracef0(TRACE_MARK, "making single mark into marked block\n");
        blkend = blkstart;
        }
    else
        vtracef0(TRACE_MARK, "no mark set/full marked block set\n");
}


/********************************
*                               *
* set a single slot as marked   *
* adjusts screen to show change *
*                               *
********************************/

static void
set_single_mark(colt tcol, rowt trow)
{
    clear_markers();

    /* setting new mark, anchor here to this document */
    blkdochandle  = current_document_handle();

    blkanchor.col = tcol;
    blkanchor.row = trow;

    /* set new block of one slot, end mark fudged */
    blkstart.col = tcol;
    blkstart.row = trow;
    blkend.col   = tcol;
    blkend.row   = trow;

    /* fudge non-existent old block position */
    old_blkstart.row = old_blkend.row = (rowt) -1;
    old_blkstart.col = old_blkend.col = (colt) -1;

    if(new_marked_rectangle())
        {
        /* update the screen if some slots need redrawing */
        xf_drawsome = TRUE;
        #if MS
            if(current_document_handle() == front_doc)
        #endif
                draw_screen();
        }

    /* set correct end mark */
    blkend.col = NO_COL;
}


/********************************************
*                                           *
* adjust an existing/set a new marked block *
* adjusts screen to show change             *
*                                           *
********************************************/

extern void
set_marked_block(colt scol, rowt srow, colt ecol, rowt erow, BOOL new)
{
    vtracef5(TRACE_MARK, "set_marked_block(%d, %d, %d, %d, new=%s)\n",
                            scol, srow, ecol, erow, new);

    /* always keep markers ordered */
    sort_colt(&scol, &ecol);
    sort_rowt(&srow, &erow);

    if(new)
        {
        clear_markers();

        /* fudge non-existent old block position */
        old_blkstart.row = old_blkend.row = (rowt) -1;
        old_blkstart.col = old_blkend.col = (colt) -1;

        /* setting new marked block, anchor here to this document */
        blkdochandle  = current_document_handle();

        blkanchor.col = scol;
        blkanchor.row = srow;
        }
    else
        {
        make_single_mark_into_block();

        /* note old block position */
        old_blkstart = blkstart;
        old_blkend   = blkend;
        }

    /* set new block */
    blkstart.col = scol;
    blkstart.row = srow;
    blkend.col   = ecol;
    blkend.row   = erow;

    if(new_marked_rectangle())
        {
        /* update the screen if some slots need redrawing */
        xf_drawsome = TRUE;
        #if MS
            if(current_document_handle() == front_doc)
        #endif
                draw_screen();
        }
}


/****************************************
*                                       *
* set a marker:                         *
*                                       *
* if two marks already set or if one    *
* mark is set in another document       *
* then clear marker(s) first.           *
*                                       *
****************************************/

extern void
set_marker(colt tcol, rowt trow)
{
    colt ecol, scol;
    rowt erow, srow;
    BOOL new;

    vtracef2(TRACE_MARK, "set_marker(%d, %d)\n", tcol, trow);

    /* setting first mark if no marks, one set elsewhere or two anywhere */
    new = (blkstart.col == NO_COL)  ||  /* none? */
          (blkend.col   != NO_COL)  ||  /* two? */
          (blkdochandle != current_document_handle());  /* one else? */

    if(new)
        /* setting first mark, all fresh please */
        set_single_mark(tcol, trow);
    else
        {
        /* setting second mark, leave anchor as is */
        scol = blkstart.col;
        srow = blkstart.row;
        ecol = tcol;
        erow = trow;

        set_marked_block(scol, srow, ecol, erow, FALSE);
        }
}


/********************************************************
*                                                       *
* is the slot marked?                                   *
* marked blocks lie between blkstart.col, blkend.col,   *
*                           blkstart.row, blkend.row    *
*                                                       *
********************************************************/

extern BOOL
inblock(colt tcol, rowt trow)
{
    if(blkdochandle != current_document_handle())
        return(FALSE);              /* marked block not in this document */

    if(blkend.col != NO_COL)        /* block of slots */
        return( (blkstart.col <= tcol)  &&  (blkstart.row <= trow)  &&
                (tcol <= blkend.col)    &&  (trow <= blkend.row)    );

    if(blkstart.col != NO_COL)      /* one slot */
        return((tcol == blkstart.col)  &&  (trow == blkstart.row));

    return(FALSE);                  /* no block */
}


/************************************************
*                                               *
* set up block ie. make in_block top left slot  *
* if no block set to current slot               *
*                                               *
************************************************/

extern void
init_marked_block(void)
{
    init_block(&blkstart, &blkend);
}


extern void
init_doc_as_block(void)
{
    SLR bs;
    SLR be;

    bs.col = (colt) 0;
    bs.row = (rowt) 0;
    be.col = numcol - 1;
    be.row = numrow - 1;

    init_block(&bs, &be);
}


extern void
init_block(const SLR *bs, const SLR *be)
{
    start_bl = *bs;
    end_bl   = *be;

    if( start_bl.col & BADCOLBIT)
        start_bl.col = NO_COL;

    /* if no mark(s) set in this document, use current slot */

    if(start_bl.col != NO_COL)
        {
        in_block = start_bl;
        if( end_bl.col & BADCOLBIT)
            end_bl.col = NO_COL;
        if( end_bl.col == NO_COL)
            end_bl.row = start_bl.row;  /* single slot (marked) */
        }
    else
        {
        /* single slot (current) */
        start_bl.row = currow;
        in_block.col = curcol;
        in_block.row = currow;
        end_bl       = start_bl;    /* NO_COL, currow */
        }

    start_block = TRUE;
}


/*******************
*                  *
* could be macroed *
*                  *
*******************/

extern void
force_next_col(void)
{
    in_block.row = end_bl.row;
}


/*********************************************************************
*                                                                    *
* this must be called after init_block and before reading first slot *
*                                                                    *
*********************************************************************/

extern BOOL
next_in_block(BOOL direction)
{
    /* always return first slot - perhaps current slot */
    if(start_block)
        {
        start_block = FALSE;
        return(TRUE);
        }

    /* for zero or one markers, only slot returned above */
    if(end_bl.col == NO_COL)
        return(FALSE);

    /* next column */
    if(direction == DOWN_COLUMNS)
        {
        if(++in_block.row > end_bl.row)
            {
            /* off bottom so reset */
            in_block.row = start_bl.row;

            if(++in_block.col > end_bl.col)
                return(FALSE);                  /* off side too */
            }
        }
    else
        {
        if(++in_block.col > end_bl.col)
            {
            /* off rhs so reset */
            in_block.col = start_bl.col;

            if(++in_block.row > end_bl.row)
                return(FALSE);                  /* off bottom too */
            }
        }

    return(TRUE);
}


/****************************************
*                                       *
*  calculate percentage through block   *
*                                       *
****************************************/

extern intl
percent_in_block(BOOL direction)
{
    colt ncol = end_bl.col - start_bl.col + 1;
    rowt nrow = end_bl.row - start_bl.row + 1;
    colt tcol = in_block.col - start_bl.col;
    rowt trow = in_block.row - start_bl.row;

    tracef5("[percent_in_block: DOWN_COLUMNS %s ncol %d nrow %d tcol %d trow %d]\n",
            trace_boolstring(direction == DOWN_COLUMNS), ncol, nrow, tcol, trow);

    return( (intl) (
            (direction == DOWN_COLUMNS)
                    ? (100 * (word32) tcol + (100 * (word32) trow) / (word32) nrow) / (word32) ncol
                    : (100 * (word32) trow + (100 * (word32) tcol) / (word32) ncol) / (word32) nrow
            ));
}


/************************************************************************
*                                                                       *
* mark block                                                            *
* if 0 or 2 markers already set, set first marker to current slot       *
* if 1 marker set, set second. Afterwards first marker is top left      *
* and second marker is bottom right even if markers not specified at    *
* these slots or in this order.                                         *
*                                                                       *
************************************************************************/

/****************
*               *
* clear markers *
*               *
****************/

extern void
ClearMarkedBlock_fn(void)
{
    /* ensure slot present for marking correctness */
    if(!mergebuf_nocheck())
        return;

    clear_markers();
}


extern void
MarkSheet_fn(void)
{
    /* ensure slot present for marking correctness */
    if(!mergebuf_nocheck())
        return;

    set_marked_block(0, 0, numcol-1, numrow-1, TRUE);
}


extern void
MarkSlot_fn(void)
{
    /* ensure slot present for marking correctness */
    if(!mergebuf_nocheck())
        return;

    set_marker(curcol, currow);
}


#if RISCOS

/* where the mouse got clicked */
static gcoord mx;
static gcoord my;


/********************
*                   *
*  start a drag off *
*                   *
********************/

static void
start_drag(intl dragt)
{
    wimp_dragstr dragstr;

    dragtype    = dragt;
    draghandle  = current_document_handle();

    dragstr.window      = main__window;
    dragstr.type        = wimp_USER_HIDDEN;
#if 0
    /* Window Manager ignores inner box on hidden drags */
    dragstr.box.x0      = mx;
    dragstr.box.y0      = my;
    dragstr.box.x1      = mx+30;
    dragstr.box.y1      = my+30;
#endif
    dragstr.parent.x0   = 0;
    dragstr.parent.y0   = 0;
    dragstr.parent.x1   = screen_x_os;
    dragstr.parent.y1   = screen_y_os;

    wimpt_complain(wimp_drag_box(&dragstr));
}


/****************************************************
*                                                   *
*  someone has clicked & dragged in our main window *
*                                                   *
****************************************************/

static BOOL  drag_not_started_to_mark;
static coord dragstart_tx;
static coord dragstart_ty;
static RANGE dragstart;

static void
prepare_for_drag_mark(coord tx, coord ty, colt scol, rowt srow, colt ecol, rowt erow)
{
    dragstart_tx            = tx;
    dragstart_ty            = ty;
    dragstart.first.col     = scol;
    dragstart.first.row     = srow;
    dragstart.second.col    = ecol;
    dragstart.second.row    = erow;
    drag_not_started_to_mark = TRUE;
}


/* column to constrain selection made by dragging to */
static colt dragcol = -1;

/* Shift-drag means try to continue selection else start selection */

static void
application_startdrag(coord tx, coord ty, BOOL selectclicked)
{
    dochandle doc = current_document_handle();
    BOOL blkindoc = (blkstart.col != NO_COL)  &&  (doc == blkdochandle);
    BOOL shiftpressed = depressed_shift();
    coord coff = calcoff(tx);   /* not _click */
    coord roff = calroff(ty);   /* not _click */
    colt  tcol;
    rowt  trow;
    SCRROW *rptr;

    /* stop us wandering off bottom of sheet */
    roff = min(roff, rowsonscreen - 1);

    tracef3("it's a drag start: at roff %d, coff %d, select = %s: ",
                roff, coff, trace_boolstring(selectclicked));

    dragcol = -1;                       /* no constraint on drag yet */
    drag_not_started_to_mark = FALSE;   /* drags mark immediately */

    if(roff >= 0)
        {
        rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
            tracef0("in soft page break - drag ignored\n");
        else
            {
            trow = rptr->rowno;

            if((coff >= 0)  ||  (coff == OFF_RIGHT))
                {
                window_data *wdp = find_document_using_window(caret_window);
                if(wdp  &&  wdp->Xxf_inexpression)
                    {
                    tracef0("dragging to insert reference to a range - first coordinate already entered\n");
                    start_drag(INSERTING_REFERENCE);
                    }
                else
                    {
                    /* mark normal block */
                    coff = get_column(tx, trow, 0, selectclicked);
                    tcol = col_number(coff);

                    if(blkindoc  &&  shiftpressed)
                        {
                        tracef0("continue mark\n");
                        make_single_mark_into_block();
                        }
                    else
                        {
                        tracef2("col #%d, row #%d - start mark\n", tcol, trow);
                        prepare_for_drag_mark(tx, ty, tcol, trow, tcol, trow);
                        }

                    start_drag(MARK_BLOCK);

                    if(selectclicked)
                        dragcol = tcol; /* constrain drag selection to tcol */
                    }
                }
            elif(IN_ROW_BORDER(coff))
                {
                /* mark all columns over given rows */
                if(blkindoc  &&  shiftpressed)
                    {
                    tracef0("in row border - continuing all columns mark\n");
                    make_single_mark_into_block();
                    }
                else
                    {
                    tracef0("in row border - starting all columns mark\n");
                    prepare_for_drag_mark(tx, ty, 0, trow, numcol-1, trow);
                    }

                start_drag(MARK_ALL_COLUMNS);
                }
            else
                tracef0("off left - ignored\n");
            }
        }
    elif(IN_COLUMN_HEADING(roff))
        {
        if(coff >= 0)
            {
            /* mark all rows over given columns */
            if(blkindoc  &&  shiftpressed)
                {
                tracef0("in column headings - continuing all rows mark\n");
                make_single_mark_into_block();
                }
            else
                {
                tracef0("in column headings - starting all rows mark\n");
                tcol = col_number(coff);
                prepare_for_drag_mark(tx, ty, tcol, 0, tcol, numrow-1);
                }

            start_drag(MARK_ALL_ROWS);
            }
    /*  elif(IN_ROW_BORDER(coff))   */
        else
            tracef0("off left/right of column headings - ignored\n");
        }
    else
        tracef0("above column headings - ignored\n");
}


static void
application_doubleclick(coord tx, coord ty, BOOL selectclicked)
{
    coord coff = calcoff(tx);   /* not _click */
    coord roff = calroff(ty);   /* not _click */
    colt  tcol;
    rowt  trow;
    SCRROW *rptr;

    /* stop us wandering off bottom of sheet */
    roff = min(roff, rowsonscreen - 1);

    tracef3("it's a double-click: at roff %d, coff %d, select = %s: ",
                roff, coff, trace_boolstring(selectclicked));

    if(roff >= 0)
        {
        rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
            tracef0("in soft page break - double click ignored\n");
        else
            {
            trow = rptr->rowno;

            if((coff >= 0)  ||  (coff == OFF_RIGHT))
                {
                tracef0("mark slot\n");
                coff = get_column(tx, trow, 0, selectclicked);
                tcol = col_number(coff);
                set_marker(tcol, trow);
                }
            elif(IN_ROW_BORDER(coff))
                {
                tracef0("in row border - mark row\n");
                set_marked_block(0, trow, numcol-1, trow, TRUE);
                }
            else
                tracef0("off left - ignored\n");
            }
        }
    elif(IN_COLUMN_HEADING(roff))
        {
        if(coff >= 0)
            {
            tracef0("in column headings - mark column\n");
            tcol = col_number(coff);
            set_marked_block(tcol, 0, tcol, numrow-1, TRUE);
            }
        elif(IN_ROW_BORDER(coff))
            {
            if(selectclicked)
                {
                tracef0("top left corner of borders - mark entire sheet\n");
                application_process_command(N_MarkSheet);
                }
            else
                {
                tracef0("top left corner of borders - clear markers\n");
                application_process_command(N_ClearMarkedBlock);
                }
            }
        else
            tracef0("off left/right of column headings - ignored\n");
        }
    else
        tracef0("above column headings - ignored\n");
}


/************************************************
*                                               *
*  a button event has occurred in this window   *
*                                               *
************************************************/

extern void
application_button_click(gcoord x, gcoord y, intl buttonstate)
{
    static BOOL initially_selectclicked = FALSE;

    /* text cell coordinates */
    coord tx = tcoord_x(x);
    coord ty = tcoord_y(y);
    intl xcelloffset = tcoord_x_remainder(x);   /* x offset in cell (OS units) */
    BOOL selectclicked;

    tracef6("app_button_click: g(%d, %d) t(%d, %d) xco %d bstate %X\n",
                x, y, tx, ty, xcelloffset, buttonstate);

    /* ensure we can find slot for positioning, overlap tests etc. must allow spellcheck as we may move */
    if(!mergebuf())
        return;
    filbuf();
    /* but guaranteed that we can simply slot_in_buffer = FALSE for movement */

    mx = x;
    my = y;

    /* have to cope with Pink 'Plonker' Duck Man's ideas on double clicks (fixed in new RISC OS+):
     * He says that left then right (or vice versa) is a double click!
    */
    if(buttonstate & (wimp_BLEFT | wimp_BRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BLEFT) != 0);

        /* must be same button that started us off */
        if(initially_selectclicked != selectclicked)
            buttonstate = buttonstate << 8; /* force a single click */
        }

    /* convert ctrl-single click into double click for marking start without caret movement */
    if(buttonstate & (wimp_BCLICKLEFT | wimp_BCLICKRIGHT))
        if(depressed_ctrl())
            buttonstate = buttonstate >> 8; /* force a double click */

      if(buttonstate & (wimp_BLEFT      | wimp_BRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BLEFT) != 0);

        application_doubleclick(tx, ty, selectclicked);
        }
    elif(buttonstate & (wimp_BDRAGLEFT  | wimp_BDRAGRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BDRAGLEFT) != 0);

        application_startdrag(tx, ty, selectclicked);
        }
    elif(buttonstate & (wimp_BCLICKLEFT | wimp_BCLICKRIGHT))
        {
        selectclicked = ((buttonstate & wimp_BCLICKLEFT) != 0);

        initially_selectclicked = selectclicked;
        application_click(tx, ty, xcelloffset, selectclicked);
        }
}


/********************************************
*                                           *
*  on null events, alter the marked block   *
*                                           *
********************************************/

static BOOL
now_marking(coord tx, coord ty)
{
    if(!drag_not_started_to_mark)
        return(TRUE);

    (void) mergebuf_nocheck();
    filbuf();

    if((dragstart_tx != tx)  ||  (dragstart_ty != ty))
        {
        drag_not_started_to_mark = FALSE;
        set_marked_block(dragstart.first.col,   dragstart.first.row,
                         dragstart.second.col,  dragstart.second.row, TRUE);
        return(TRUE);
        }

    return(FALSE);
}


extern void
application_drag(gcoord x, gcoord y, BOOL ended)
{
    coord tx    = tcoord_x(x);
    coord ty    = tcoord_y(y);
    coord coff  = calcoff_click(tx);    /* map OFF_LEFT/RIGHT to lh/rh col */
    coord roff  = calroff_click(ty);    /* map off top/bot to top/bot row */
    colt  tcol;
    rowt  trow;
    SCRROW *rptr;
    SLR here;  

    /* stop us wandering off bottom (or top) of sheet */
      if(roff > rowsonscreen - 1)
         roff = rowsonscreen - 1;
    elif(roff < 0)
         roff = 0;

    vtracef1(TRACE_DRAG, "application_drag: type = %d\n", dragtype);

    rptr = vertvec_entry(roff);

    switch(dragtype)
        {
        case INSERTING_REFERENCE:
            if(ended  &&  !(rptr->flags & PAGE))
                {
                window_data *wdp = find_document_using_window(caret_window);

                if(wdp  &&  wdp->Xxf_inexpression)
                    {
                    here.col = col_number(coff);
                    here.row = rptr->rowno;

                    /* ensure range ordered */
                    sort_colt(&insert_reference_slr.col, &here.col);
                    sort_rowt(&insert_reference_slr.row, &here.row);

                    lecpos = insert_reference_stt_offset;
                    delete_bit_of_line(lecpos, insert_reference_end_offset - lecpos, FALSE);
                    insert_reference_to(wdp->DocHandle, insert_reference_slr.doc, insert_reference_slr.col, insert_reference_slr.row, FALSE);
                    insert_reference_to(wdp->DocHandle, wdp->DocHandle, here.col, here.row, TRUE);
                    }
                }
            break;


        case MARK_ALL_COLUMNS:
            /* marking fixed set of columns in dynamic selection of rows */
            if(!(rptr->flags & PAGE)  &&  now_marking(tx, ty))
                {
                trow = rptr->rowno;
                alter_marked_block(ACTIVE_COL, trow);
                }
            break;


        case MARK_ALL_ROWS:
            /* marking fixed set of rows in dynamic selection of columns */
            if(now_marking(tx, ty))
                {
                tcol = (dragcol < 0) ? col_number(coff) : dragcol;
                alter_marked_block(tcol, ACTIVE_ROW);
                }
            break;


        case MARK_BLOCK:
            /* marking arbitrary (or constrained) block */
            if(!(rptr->flags & PAGE)  &&  now_marking(tx, ty))
                {
                tcol = (dragcol < 0) ? col_number(coff) : dragcol;
                trow = rptr->rowno;
                alter_marked_block(tcol, trow);
                }
            break;
        }
}


#endif  /* RISCOS */

/* end of markers.c */
