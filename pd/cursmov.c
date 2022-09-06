/* cursmov.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       cursmov.c - relative cursor movement handling
 * Author:      RJM August 1987
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "font.h"
#elif MS
#else
    assert(0);
#endif


#include "datafmt.h"
#include "eval.h"


#if RISCOS
#include "riscdraw.h"
#include "ext.riscos"
#endif


/* External functions */

extern coord calcad(coord coff);
extern intl calsiz(uchar *str);
extern void cencol(colt colno);
extern void cenrow(rowt rowno);
extern void chkmov(void);
extern void chknlr(colt tcol, rowt trow);
extern BOOL chkpac(rowt rowno);
extern BOOL chkpbs(rowt rowno, intl offset);
extern BOOL chkrpb(rowt rowno);
extern void curdown(void);
extern void curosc(void);
extern void cursdown(void);
extern void cursup(void);
extern void curup(void);
extern void do_down_scroll(void);
extern void do_up_scroll(void);
extern void filhorz(colt nextcol, colt currentcolno);
extern void filvert(rowt nextrow, rowt currentrowno, BOOL call_fixpage);
extern void fixpage(rowt o_row, rowt n_row);
extern colt fstncx(void);
extern rowt fstnrx(void);
extern BOOL incolfixes(colt colno);
extern BOOL inrowfixes(rowt row);
extern BOOL inrowfixes1(rowt row);
extern BOOL isslotblank(slotp tslot);
extern void mark_row(coord rowonscr);
extern void mark_row_praps(coord rowonscr, BOOL old_row);
extern void nextcol(void);
extern void prevcol(void);
extern coord schcsc(colt col);
extern coord schrsc(rowt row);
extern uchar vertvec_entry_flags(coord roff);
extern void update_variables(void);


/* internal functions */

/*static BOOL adjpud(BOOL down, rowt rowno);*/
static intl cal_offset_in_slot(colt col, rowt row, slotp sl,
                               intl offset_ch, intl cell_offset_os);
/*static coord calpli(void);*/
static colt col_off_left(void);
static colt col_off_right(void);
static BOOL push_screen_down_one_line(void);
static void push_screen_up_one_line(void);
/*static void rebols(void);*/
/*static rowt row_off_top(void);*/


/* ----------------------------------------------------------------------- */

/* returns a pointer to horzvec */

extern SCRCOL *
horzvec(void)
{
    return(list_getptr(horzvec_mh));
}


extern SCRCOL *
horzvec_entry(coord coff)
{
    return((SCRCOL *) list_getptr(horzvec_mh) + coff);
}


/* returns the col number corresponding to a horzvec index */

extern colt
col_number(coord coff)
{
    return(horzvec_entry(coff)->colno);
}


/* returns a pointer to vertvec */

extern SCRROW *
vertvec(void)
{
    return(list_getptr(vertvec_mh));
}


extern SCRROW *
vertvec_entry(coord roff)
{
    return((SCRROW *) list_getptr(vertvec_mh) + roff);
}


extern uchar
vertvec_entry_flags(coord roff)
{
    return(vertvec_entry(roff)->flags);
}


/* returns the row number corresponding to a vertvec index */

extern rowt
row_number(coord roff)
{
    return(vertvec_entry(roff)->rowno);
}


extern void
chknlr(colt tcol, rowt trow)
{
    movement = movement | ABSOLUTE;

    newcol = (tcol >= numcol)   ? numcol - 1
                                : tcol;

    newrow = (trow >= numrow)   ? numrow - 1
                                : trow;
}


/************************************
*                                   *
* go to given row in current column *
*                                   *
************************************/

static void
TC_BC_common(rowt trow)
{
    if(!mergebuf())
        return;

    out_forcevertcentre = TRUE;

    chknlr(curcol, trow);

    if(!xf_inexpression)
        lecpos = lescrl = 0;
}


/************************************
*                                   *
*  go to the top of the column      *
*                                   *
************************************/

extern void
TopOfColumn_fn(void)
{
    TC_BC_common((rowt) 0);
}


/************************************
*                                   *
*  go to the bottom of the column   *
*                                   *
************************************/

extern void
BottomOfColumn_fn(void)
{
    TC_BC_common(numrow - 1);
}


/****************************
*                           *
*  centre the current line  *
*                           *
****************************/

extern void
CentreWindow_fn(void)
{
    if(!mergebuf())
        return;

    cenrow(currow);
}


/************************************************************
*                                                           *
* scroll screen up, forcing caret on screen if necessary    *
*                                                           *
* move cursor to top of screen, scroll it and reset cursor  *
*                                                           *
************************************************************/

extern void
ScrollUp_fn(void)
{
    SCRROW *rptr;
    rowt    trow = currow;

    if(!mergebuf())
        return;

    rptr = vertvec();

    currowoffset = (rptr->flags & PAGE) ? 1 : 0;
    currow = rptr[currowoffset].rowno;

    push_screen_up_one_line();

    currowoffset = schrsc(trow);

    if(currowoffset == NOTFOUND)
        {
        xf_drawslotcoordinates = output_buffer = TRUE;

        currowoffset = rowsonscreen-1;

        if(rptr[currowoffset].flags & PAGE)
            --currowoffset;

        mark_row_praps(currowoffset, NEW_ROW);
        }

    currow = rptr[currowoffset].rowno;
}


/************************************************************
*                                                           *
* scroll screen down, forcing caret on screen if necessary  *
*                                                           *
************************************************************/

extern void
ScrollDown_fn(void)
{
    rowt trow = currow;

    if(!mergebuf())
        return;

    /* if last row on screen don't bother */
    if(!inrowfixes(numrow-1)  &&  (schrsc(numrow-1) != NOTFOUND))
        return;

    push_screen_down_one_line();

    currowoffset = schrsc(trow);

    if(currowoffset == NOTFOUND)
        {
        xf_drawslotcoordinates = output_buffer = TRUE;
        currow = fstnrx();
        currowoffset = schrsc(currow);
        mark_row_praps(currowoffset, NEW_ROW);
        }
    else
        currow = row_number(currowoffset);
}


/************************************************************
*                                                           *
* scroll screen left, forcing caret on screen if necessary  *
*                                                           *
************************************************************/

extern void
ScrollLeft_fn(void)
{
    colt o_col = curcol;
    colt tcol;

    /* if cannot scroll left, don't bother */
    if((tcol = col_off_left()) < 0)
        {
        xf_flush = TRUE;
        return;
        }

    if(!mergebuf())
        return;

    /* fill horzvec from column to the left */
    filhorz(tcol, tcol);

    /* if now off right of screen or in partial column, set to rightmost fully visible column (if poss) */
    curcoloffset = schcsc(o_col);

    if((curcoloffset == NOTFOUND)  ||  (curcoloffset == scrbrc))
        curcoloffset = !scrbrc ? scrbrc : scrbrc-1;

    curcol = col_number(curcoloffset);

    out_screen = TRUE;
}


/************************************************************
*                                                           *
* scroll screen right, forcing caret on screen if necessary *
*                                                           *
************************************************************/

extern void
ScrollRight_fn(void)
{
    colt o_col = curcol;
    colt tcol;

    /* no more columns to bring on screen? */
    if(col_off_right() >= numcol)
        {
        xf_flush = TRUE;
        return;
        }

    if(!mergebuf())
        return;

    tcol = fstncx();

    do  {
        ++tcol;
        }
    while(!colwidth(tcol));

    /* fill horzvec from column */
    filhorz(tcol, tcol);

    /* if now off left of screen, set to leftmost non-fixed visible column */
    curcoloffset = schcsc(o_col);
    curcol       = (curcoloffset == NOTFOUND) ? tcol : o_col;
    curcoloffset = schcsc(curcol);

    out_screen = TRUE;
}


/****************************************************************************
*                                                                           *
* mark the row for redrawing.  When moving up and down within the screen    *
* most numeric slots do not need to be redrawn.  The old and new slots      *
* must be redrawn for the block cursor movement.  Text and blank slots must *
* be redrawn for overlap. drawnumbers specifies whether the numbers can be  *
* missed.                                                                   *
*                                                                           *
****************************************************************************/

#if defined(SHOW_CURRENT_ROW)

extern void
mark_row_border(coord rowonscr)
{
    /* allow multiple calls */
    if( (out_rowborout   &&  (rowonscr == rowborout ))  ||
        (out_rowborout1  &&  (rowonscr == rowborout1))  )
            return;

    rowborout1     = rowborout;
    out_rowborout1 = out_rowborout;

    rowborout      = rowonscr;
    out_rowborout  = TRUE;
}

#endif


extern void
mark_row(coord rowonscr)
{
    tracef2("[mark_row row: %d, offset: %d]\n", row_number(rowonscr), rowonscr);

    #if defined(SHOW_CURRENT_ROW)
    mark_row_border(rowonscr);
    #endif

    /* allow multiple calls */
    if( (out_rowout   &&  (rowonscr == rowout ))    ||
        (out_rowout1  &&  (rowonscr == rowout1))    )
            return;

    rowout1     = rowout;
    out_rowout1 = out_rowout;

    rowout      = rowonscr;
    out_rowout  = TRUE;
}


/**********************************************************
*                                                         *
* MRJC created this more optimal version 13/7/89          *
* I note from the speech above mark_row that the original *
* sentiments from VP have been noted - but not            *
* implemented! So this one checks for numeric slots       *
* and overlap. This was done by MRKCON in VP              *
* This routine assumes that the column has not changed    *
*                                                         *
**********************************************************/

extern void
mark_row_praps(coord rowonscr, BOOL old_row)
{
    colt col, i;
    rowt row;
    slotp sl, tsl;
    char *c;
    coord fwidth;

    #if defined(SHOW_CURRENT_ROW)
    mark_row_border(rowonscr);
    #endif

    /* do-while is for structure - break out to mark row */
    do  {
        if(xf_inexpression)
            break;

        col = oldcol;
        row = row_number(rowonscr);

        tracef4("[mark_row_praps col: %d, row: %d, lescrl %d, OLD_ROW %s]\n", col, row, lescrl, trace_boolstring(old_row == OLD_ROW));

        if(chkrpb(row))
            break;

        sl = travel(col, row);

        /* only text slots can be different */
        if(sl  &&  (sl->type != SL_TEXT))
            return;

        tracef0("[mark_row_praps found empty slot/text slot]\n");

        /* check for justification */
        if(sl  &&  ((sl->justify & J_BITS) != J_FREE))
            {
            tracef0("[slot is justified]\n");
            break;
            }

        /* row must be output if was scrolled/will be scrolled */
        if(old_row)
            {
            if(old_lecpos)
                {
                if(old_lescroll)
                    {
                    tracef0("[old slot had been scrolled]\n");
                    break;
                    }

                #if RISCOS
                if(grid_on  &&  (old_lecpos > colwidth(col)))
                    {
                    tracef0("[grid is on and cursor was beyond a grid bar in the field]\n");
                    break;
                    }
                #endif
                }
            }
        else
            {
            if(lecpos)
                {
                #if RISCOS
                /* if fonts are on, mark and go home */
                if(riscos_fonts)
                    {
                    tracef0("[non-zero lecpos and fonts on]\n");
                    break;
                    }

                if(grid_on  &&  (lecpos > colwidth(col)))
                    {
                    tracef0("[grid is on and cursor will be beyond a grid bar in the field]\n");
                    break;
                    }
                #endif

                fwidth = limited_fwidth_of_slot(sl, col, row);

                if(--fwidth < 0)        /* rh scroll margin */
                    fwidth = 0;

                if(lecpos > fwidth)
                    {
                    tracef0("[new slot will be scrolled]\n");
                    break;
                    }
                }
            }

        /* check for slots to the left overlapping */
        if(col  &&  !sl)
            {
            for(i = 0; i < col; i++)
                {
                tsl = travel(i, row);

                if(!tsl  ||  (tsl->type != SL_TEXT))
                    continue;

                if(!isslotblank(tsl))
                    break;
                }           

            if(i != col)
                {
                tracef0("[blank slot is overlapped by something to the left]\n");
                break;
                }
            }

        if(!sl)
            return;

        /* search for ats and highlights */
        for(c = sl->content.text; *c; c++)
            if( (*c == '@')  ||
                #if TRUE
                (*c == DELETE)  ||  (*c < SPACE))
                #else
                ((*c >= FIRST_HIGHLIGHT)  &&  (*c <= LAST_HIGHLIGHT)))
                #endif
                    {
                    tracef0("[slot has highlights]\n");
                    goto mark_and_return;
                    }

        return;
        }
    while(FALSE);

mark_and_return:

    tracef0("[mark_row_praps marking slot]\n");
    mark_row(rowonscr);
}


/****************************************************
*                                                   *
* returns the next row up off the top of the screen *
* if no more rows returns -1                        *
*                                                   *
****************************************************/

static rowt
row_off_top(void)
{
    rowt trow = fstnrx() - 1;

    while((trow >= 0)  &&  inrowfixes(trow))
        trow--;

    return(trow);
}


/************************************************************
*                                                           *
*  adjust the page offset and number for going up and down  *
*                                                           *
************************************************************/

static BOOL
adjpud(BOOL down, rowt rowno)
{
    BOOL softbreak = FALSE;
    intl tpoff;

    curpnm = pagnum;

    tracef2("[adjpud(%s, %d)]\n", trace_boolstring(down), rowno);

    if(down)
        {
        if(chkrpb(rowno))
            {
            /* hard page break: set offset in page break and check active */

            if(chkpbs(rowno, (pagoff==0) ? encpln : pagoff))
                {
                /* don't update pagnum if hard and soft together */
                if(pagoff != enclns)
                    {
                    pagnum = ++curpnm;
                    pagoff = enclns;
                    }
                }

            return(FALSE);
            }
        }
    else
        {
        /* check for conditional page break */
        if(chkrpb(rowno-1))
            {
            /* read active state and check active */
            pagoff = travel(0, rowno-1)->content.page.cpoff;

            /* if on soft break, adjust pagoff */
            if(pagoff == encpln)
                pagoff++;

            if(chkpac(rowno-1))
                if(pagoff != enclns)
                    {
                    --curpnm;
                    pagnum = curpnm;
                    }

            return(FALSE);
            }
        }

    /* check for soft break */
    tpoff = pagoff;

    if(tpoff == 0)
        plusab(curpnm, (down) ? 1 : -1);

    tpoff += (down) ? enclns : -enclns;

      if(tpoff < 0)
        tpoff = encpln;
    elif(tpoff > encpln)
        tpoff = 0;

    softbreak = ((tpoff == 0)  &&  chkfsb());
    pagoff = tpoff;

    pagnum = curpnm;

    return(softbreak);
}


/********************************************************
*                                                       *
*  move from oldrow to newrow updating page parameters  *
*                                                       *
********************************************************/

extern void
fixpage(rowt o_row, rowt n_row)
{
    BOOL down   = (n_row >= o_row);
    intl amount = (down) ? 1 : -1;
    rowt trow;

    tracef2("[fixpage(%d, %d)\n]", o_row, n_row);

    if(n_row == (rowt) 0)
        {
        pagoff = filpof;
        pagnum = filpnm;
        return;
        }

    while(n_row != o_row)
        {
        trow = o_row + amount;

        tracef1("trying trow %d\n", trow);

        /* check hard and soft breaks together */
        if(down  &&  (pagoff == 0)  &&  chkrpb(trow))
            {
            ++trow;
            ++o_row;
            }

        if(!adjpud(down, o_row))
            o_row = trow;
        }
}


/************************
*                       *
* move cursor up a line *
*                       *
************************/

extern void
curup(void)
{
    coord origoffset;

    xf_flush = TRUE;

    if(currowoffset == 0)
        {
        /* scroll another line on if at top of screen */
        push_screen_up_one_line();
        return;
        }

    /* may be a page break at top of file */
    if(currow == 0)
        return;

    origoffset = currowoffset--;

    if(vertvec_entry_flags(currowoffset) & PAGE)
        {
        /* move up over the page break */
        if(currowoffset == 0)
            {
            pagnum++;
            curpnm++;
            }

        curup();

        /* in the case of moving up over a page break
         * and scrolling the screen, the old row will
         * now be at offset 2
        */
        if(origoffset == 1)
            origoffset = 2;

        mark_row_praps(origoffset, OLD_ROW);
        }
    else
        {
        mark_row_praps(origoffset, OLD_ROW);
        currow = row_number(currowoffset);
        mark_row_praps(currowoffset, NEW_ROW);
        }
}


/*
push_screen_up_one_line scrolls the screen up
it assumes the cursor is on the top (non-page break) line
*/

extern void
push_screen_up_one_line(void)
{
    SCRROW *rptr = vertvec();
    BOOL gone_over_pb;
    rowt trow;

    gone_over_pb = FALSE;

    if(rptr->flags & FIX)
        {
        /* if last row fixed, can't do anything */
        if(rptr[rows_available].flags & FIX)
            return;

        trow = row_off_top();

        if(trow < 0)
            return;

        filvert(trow, currow, DONT_CALL_FIXPAGE);
        }
    elif(currow > 0)
        {
        adjpud(FALSE, currow);

        currow--;

        filvert(currow, currow, DONT_CALL_FIXPAGE);

        if(rptr->flags & PAGE)
            {
            if(currow == 0)
                {
                curdown();
                draw_row(0);                /* display page break */
                return;
                }

            currowoffset = 0;
            currow++;                       /* couldn't decrement it cos of pb */

            curup();
            mark_row_praps(2, OLD_ROW);     /* old row is now row 2 */
            gone_over_pb = TRUE;
            }
        }
    else
        return;

    /* if we were in the middle of drawing the whole screen, start the draw
     * again. Otherwise scroll the screen and redraw top two lines
    */
    if(out_below)
        {   /* must be in block cos of mark_to_end is macro */
        mark_to_end(0);
        }
    else
        {
        do_down_scroll();

        if(!gone_over_pb)
            {
            if(n_rowfixes+1 < rows_available)
                mark_row_praps(n_rowfixes+1, NEW_ROW);

            mark_row(n_rowfixes);
            }
        else
            draw_row(1);
        }
}


/*
 * On Acorn machines we have a choice of fast or pretty (or RISC OS, hard).
 * The faster method is using hard scroll and then redrawing the first
 * few lines. This is known as epileptic scrolling.
 * The nicer method is to scroll only those lines which are changing.
 *
 * For fast scroll do
 *   down_scroll(0);
 *   draw top four lines on screen;
 *
 * ( up_scroll(0);
 *   draw top four lines on screen; )
 *
 * On Archimedes soft scroll takes longer for larger memory screen modes.
 * Since there is no point in using modes with more than four colours,
 * the worst real cases are mode 19 (4 colours,  64r*80c,   80k)
 *                      and mode 16 (16 colours, 32r*132c, 132k)
*/

extern void
do_down_scroll(void)
{
    #if !RISCOS
    SCRROW *rptr        = vertvec();
    SCRROW *last_rptr   = rptr + n_rowfixes;

    rptr += rows_available;

    while(rptr > last_rptr)
        {
        rptr->length = (rptr-1)->length;
        rptr--;
        }

    last_rptr->length = 0;
    #endif

    down_scroll(BORDERLINE + borderheight + n_rowfixes);
}


/****************************
*                           *
*  move cursor down a line  *
*                           *
****************************/

extern void
curdown(void)
{
    uchar flags;
    coord origoffset;

    xf_flush = TRUE;

    if(currowoffset >= rows_available-1)
        {
        /* cursor at bottom of screen */
        if( (vertvec_entry_flags(currowoffset) & FIX)  ||
            !push_screen_down_one_line())
            return;
        }
    else
        {
        /* cursor not yet at bottom of screen */
        flags = vertvec_entry_flags(currowoffset+1);

        if((flags & PAGE))
            {
            origoffset = currowoffset;

            if(currowoffset == rows_available-2)
                origoffset--;

            currowoffset++;
            curdown();
            draw_row(origoffset);
            return;
            }

        if(!(flags & LAST))
            {
            mark_row_praps(currowoffset++,  OLD_ROW);       /* mark old row */
            mark_row_praps(currowoffset,    NEW_ROW);       /* mark new row */
            }
        }

    currow = row_number(currowoffset);
}


static BOOL
push_screen_down_one_line(void)
{
    rowt tnewrow;
    intl oldpagoff;
    BOOL gone_over_pb;

    if(currow+1 >= numrow)
        return(FALSE);

    tnewrow = fstnrx();
    oldpagoff = pagoff;
    gone_over_pb = FALSE;

    adjpud(TRUE, tnewrow);

    if((oldpagoff != 0)  ||  chkrpb(tnewrow)  ||  !chkfsb())
        {
        /* if top line isn't soft break, or there is hard break too */
        if(++tnewrow >= numrow)
            return(FALSE);
        }

    filvert(tnewrow, currow+1, DONT_CALL_FIXPAGE);

    if(vertvec_entry_flags(rows_available-1) & PAGE)
        {
        currowoffset = rows_available-1;
        curdown();
        mark_row_praps(currowoffset-2, OLD_ROW);                /* mark old row */
        gone_over_pb = TRUE;
        }

    /* if the scrolling comes in the middle of a screen draw, draw the
     * whole lot, otherwise scroll up and redraw bottom rows
    */
    if(out_below)
        {               /* must be in block cos of macro */
        mark_to_end(0);
        }
    else
        {
        do_up_scroll();

        currowoffset = rows_available-1;

        if(!gone_over_pb)
            {
            mark_row_praps(currowoffset-1, OLD_ROW);    /* mark old row */
            mark_row(currowoffset);                     /* mark new row */
            }
        else
            draw_row(rows_available-2);     /* display page break */
        }

    return(TRUE);
}


extern void
do_up_scroll(void)
{
    #if !RISCOS
    SCRROW *rptr        = vertvec();
    SCRROW *last_rptr   = rptr + rows_available - 2;

    rptr += n_rowfixes;

    while(rptr < last_rptr)
        {
        rptr->length = (rptr+1)->length;
        rptr++;
        }

    (last_rptr+1)->length = 0;
    #endif

    up_scroll(BORDERLINE + borderheight + n_rowfixes);
}


/************************************************************
*                                                           *
* return number of rows to be jumped over in screen shifts  *
* if fixed, then number of non-fixed rows                   *
* otherwise number of rows minus soft pages                 *
*                                                           *
************************************************************/

static coord
calpli(void)
{
    SCRROW *rptr = vertvec();
    SCRROW *last_rptr;
    coord res;

    if(rptr->flags & FIX)
        res = rows_available - n_rowfixes;
    else
        {
        res = 0;

        last_rptr = rptr + rows_available;

        while(rptr < last_rptr)
            if(!(rptr++->flags & PAGE))
                res++;
        }

    return((res <= 1) ? 1 : res-1);
}


/********************************
*                               *
* move cursor down a screenful  *
*                               *
********************************/

extern void
cursdown(void)
{
    coord rowstogo;
    rowt newfirstrow, oldfirst = fstnrx();
    coord oldcuroff = currowoffset;

    for(rowstogo = calpli(), newfirstrow = oldfirst;
        (newfirstrow < numrow-1)  &&  (rowstogo > 0);
        newfirstrow++)
            if(!inrowfixes(newfirstrow))
                rowstogo--;

    xf_flush = TRUE;

    if(newfirstrow == oldfirst)
        return;

    if(chkrpb(newfirstrow) && chkpac(newfirstrow))
        newfirstrow++;

    filvert(newfirstrow, newfirstrow, CALL_FIXPAGE);

    currowoffset = (oldcuroff >= rowsonscreen)
                            ? rowsonscreen-1
                            : oldcuroff;

    if(vertvec_entry_flags(currowoffset) & PAGE)
        currowoffset = (currowoffset == 0)
                                ? 1
                                : currowoffset-1;

    currow = row_number(currowoffset);
    mark_to_end(n_rowfixes);
}


/********************************
*                               *
*  move cursor up a screenful   *
*                               *
********************************/

extern void
cursup(void)
{
    coord rowstogo = calpli();
    rowt firstrow;
    coord oldcuroff = currowoffset;

    for(firstrow = fstnrx(); (firstrow >= 0)  &&  (rowstogo > 0); firstrow--)
        if(!inrowfixes(firstrow))
            rowstogo--;

    xf_flush = TRUE;

    if(firstrow == fstnrx())
        return;

    if(vertvec()->flags & PAGE)
        {
        pagnum++;
        curpnm++;
        }

    filvert(firstrow, firstrow, CALL_FIXPAGE);

    currowoffset = (firstrow < 0)
                        ? (coord) 0
                        : oldcuroff;

    if(vertvec_entry_flags(currowoffset) & PAGE)
        currowoffset = (currowoffset == 0)
                                ? 1
                                : currowoffset-1;

    currow = row_number(currowoffset);
    mark_to_end(n_rowfixes);
}


/****************************************************************************
*                                                                           *
*                           sideways movement                               *
*                                                                           *
****************************************************************************/

/****************************************
*                                       *
*  add column at right and move to it   *
*                                       *
****************************************/

extern void
AddColumn_fn(void)
{
    if(!mergebuf())
        return;

    if(createcol(numcol))
        {
        out_rebuildhorz = TRUE;
        LastColumn_fn();
        }
}


/************************
*                       *
* move to given column  *
*                       *
************************/

static void
FCO_LCO_common(colt tcol)
{
    if(!mergebuf())
        return;

    chknlr(tcol, currow);

    lecpos = lescrl = 0;
}


/************************
*                       *
* move to first column  *
*                       *
************************/

extern void
FirstColumn_fn(void)
{
    FCO_LCO_common((colt) 0);
}


/************************
*                       *
*  move to last column  *
*                       *
************************/

extern void
LastColumn_fn(void)
{
    FCO_LCO_common(numcol-1);
}


/****************************************************************************
*                                                                           *
*  find the next column to come on the left of the screen; note fixed cols  *
*                                                                           *
****************************************************************************/

static colt
col_off_left(void)
{
    colt colno = fstncx();

    do  {
        --colno;
        }
    while(  (colno >= 0)                                &&
            (incolfixes(colno)  ||  !colwidth(colno))   );

    return(colno);
}


/****************************
*                           *
*  move to previous column  *
*                           *
****************************/

extern void
prevcol(void)
{
    colt tcol;

    if(!xf_inexpression)
        lecpos = lescrl = 0;

    /* can we move to a column to the left on screen? */
    if(curcoloffset > 0)
        {
        curcol = col_number(--curcoloffset);
        xf_drawcolumnheadings = TRUE;
        mark_row(currowoffset);
        return;
        }

    if((tcol = col_off_left()) < 0)
        {
        xf_flush = TRUE;
        return;
        }

    filhorz(tcol, tcol);
    curcoloffset = 0;
    curcol = col_number(curcoloffset);

    out_screen = TRUE;
}


/************************************
*                                   *
* find the next column on the right *
*                                   *
************************************/

static colt
col_off_right(void)
{
    colt colno = col_number((scrbrc && (curcoloffset != scrbrc)) ? scrbrc-1 : scrbrc);
    
    do  {
        ++colno;
        }
    while(  (colno < numcol)                            &&
            (incolfixes(colno)  ||  !colwidth(colno))   );

    tracef1("[col_off_right returns %d]\n", colno);
    return(colno);
}


/************************
*                       *
*  move to next column  *
*                       *
************************/

extern void
nextcol(void)
{
    colt tcol;
    colt firstcol;
    SCRCOL *cptr;

    if(!xf_inexpression)
        lecpos = lescrl = 0;

    /* can we move to a column to the right on screen? */
    tracef2("[nextcol: curcoloffset = %d, scrbrc = %d]\n", curcoloffset, scrbrc);
    if(curcoloffset + 1 < scrbrc)
        {
        curcol = col_number(++curcoloffset);
        xf_drawcolumnheadings = TRUE;
        mark_row(currowoffset);
        return;
        }

    tcol = col_off_right();

    if((tcol >= numcol)  ||  (tcol == col_number(curcoloffset)))
        {
        xf_flush = TRUE;
        return;
        }

    curcol = tcol;

/* something needed here for situation where new column won't fit
 * on screen completely ?
*/

    for(firstcol = fstncx(); firstcol <= tcol; ++firstcol)
        {
        filhorz(firstcol, tcol);

        if(schcsc(tcol) != NOTFOUND)
            {
            cptr = horzvec_entry(scrbrc);

            if((cptr->flags & LAST)  ||  (cptr->colno != tcol))
                break;
            }
        }

    xf_drawcolumnheadings = out_screen = TRUE;
}


/********************************************
*                                           *
* save, restore and swap position of cursor *
*                                           *
********************************************/

/************************************
*                                   *
* push current slot onto (FA) stack *
*                                   *
************************************/

extern void
SavePosition_fn(void)
{
#if !defined(SMALLPD)

    if(saved_index == SAVE_DEPTH)
        {
#if 1
        /* lose first stacked position */
        memmove(&saved_pos[0], &saved_pos[1], sizeof(SAVPOS) * (SAVE_DEPTH - 2));
#else
        intl i;

/*  RJM commented out this message on 8.7.89 cos it's more tedious than helpful
        reperr_null(ERR_OLDEST_LOST);
*/

        for(i = 0; i < SAVE_DEPTH - 2; i++)
            saved_pos[i] = saved_pos[i+1];
#endif

        saved_index--;
        }

    saved_pos[saved_index  ].file    = curfil;  /* no harm */
    saved_pos[saved_index  ].ref.col = curcol;
    saved_pos[saved_index  ].ref.row = currow;
    saved_pos[saved_index++].ref.doc = (docno) current_document_handle();

    #if !defined(HEADLINE_OFF)  &&  defined(SPELL_OFF)
    /* menu headline has dots to represent stacked positions */
    xf_drawmenuheadline = TRUE;
    #endif
#endif
}


/************************************
*                                   *
*  pop current slot from (FA) stack *
*                                   *
************************************/

extern void
RestorePosition_fn(void)
{
#if !defined(SMALLPD)
    dochandle odoc = current_document_handle();
    dochandle doc;
    intl index;

    if(!mergebuf())
        return;

    index = --saved_index;

    if(index < 0)
        {
        saved_index = 0;
        bleep();
        return;
        }

    /* errors in POP are tough */

    doc = (dochandle) saved_pos[index].ref.doc;

    tracef1("POP to docno %d\n", doc);

    /* has document been deleted since position saved? */
    if(doc == DOCHANDLE_NONE)
        {
        bleep();
        return;
        }

    select_document_using_handle(doc);

    if(!mergebuf())
        return;

    if(glbbit)
        {
        intl file = saved_pos[index].file;

        /* go back to the file */
        while(!been_error  &&  (file < curfil))
            PrevFile_fn();

        /* go back to the file */
        while(!been_error  &&  (file > curfil))
            NextFile_fn();
        }

    if(is_current_document())
        {
        tracef2("restoring position to col %d row %d\n", 
                saved_pos[index].ref.col, saved_pos[index].ref.row);

        chknlr(saved_pos[index].ref.col, saved_pos[index].ref.row);
        lecpos = lescrl = 0;

        if(doc != odoc)
            {
            #if RISCOS
            xf_frontmainwindow = TRUE;
            #else
            xf_draweverything = TRUE;
            #endif
            }

        #if RISCOS
        xf_acquirecaret = TRUE;
        #endif
        }
    else
        tracef0("multi-file failure destroyed current document\n");
#endif
}


extern void
SwapPosition_fn(void)
{
#if !defined(SMALLPD)
    intl oldindex = saved_index;
    SAVPOS oldpos;

    oldpos.file    = curfil;
    oldpos.ref.col = curcol;
    oldpos.ref.row = currow;
    oldpos.ref.doc = current_document_handle();

    RestorePosition_fn();

    if(oldindex > 0)
        saved_pos[saved_index] = oldpos;

    saved_index++;
#endif
}


/************
*           *
* goto slot *
*           *
************/

extern void
GotoSlot_fn(void)
{
#if !defined(SMALLPD)
    char tstr[EXT_REF_NAMLEN + 1], *extstr;
    intl count, baddoc;
    colt tcol;
    rowt trow;
    dochandle han = DOCHANDLE_NONE;

    while(dialog_box(D_GOTO))
        {
        extstr = (uchar *) d_goto[0].textfield;

        while(*extstr++ == SPACE)
            ;
        buff_sofar = --extstr;

        baddoc = 0;

        if(*extstr++ == '[')
            {
            /* read in name to temporary buffer */
            for(count = 0;
                *extstr  &&  (*extstr != ']')  &&  (count < EXT_REF_NAMLEN);
                ++count, ++extstr)
                    tstr[count] = *extstr;

            if(count  &&  (*extstr == ']'))
                {
                tstr[count++] = '\0';
                if((han = find_document_using_leafname(tstr))
                                                        == DOCHANDLE_NONE)
                    baddoc = 1;
                buff_sofar = extstr;
                }
            }

        if(!baddoc)
            {
            tcol = getcol();            /* assumes buff_sofar set */
            trow = getsbd();
            }

        if(bad_reference(tcol, trow)  ||  baddoc)
            {
            reperr_null(ERR_BAD_SLOT);  /* and let him try again... */
            continue;
            }

        if(!mergebuf())
            {
            dialog_box_end();
            break;
            }

        if(han != DOCHANDLE_NONE)
            {
            select_document_using_handle(han);
            #if RISCOS
            xf_frontmainwindow = TRUE;
            #endif
            }

        chknlr(tcol, trow-1);
        lecpos = lescrl = 0;

        #if RISCOS
        xf_acquirecaret = TRUE;
        #endif

        if(dialog_box_ended())
            break;
        }

#endif
}


#if RISCOS

/********************************************************************
*                                                                   *
* offset in vertvec that gives vertical position on screen,         *
* mapping off top/bottom to top/bottom row of data visible on sheet *
*                                                                   *
********************************************************************/

extern coord
calroff_click(coord ty)
{
    coord roff = calroff(ty);

    if(roff < 0)
        roff = 0;
    elif(roff > rowsonscreen-1)
        roff = rowsonscreen-1;

    tracef2("calroff_click(%d) returns %d\n", ty, roff);
    return(roff);
}

#endif /* RISCOS */


/*****************************************************
*                                                    *
* horizontal position on screen of offset in horzvec *
*                                                    *
*****************************************************/

extern coord
calcad(coord coff)
{
    coord sofar = borderwidth;
    SCRCOL *cptr  = horzvec();
    SCRCOL *there = cptr + coff;

    while(cptr < there)
        sofar += colwidth(cptr++->colno);

    return(sofar);
}


/****************************************************************
*                                                               *
*  offset in horzvec that gives horizontal position on screen   *
*                                                               *
****************************************************************/

extern coord
calcoff(coord xpos)
{
    SCRCOL *cptr;
    coord coff;
    coord sofar;

    if(xpos < 0)
        return(OFF_LEFT);

    if(xpos < borderwidth)
        return(-1);

    /* loop till we've passed the xpos or falled off horzvec */

    cptr  = horzvec();
    coff  = -1;
    sofar = borderwidth;

    while(sofar <= xpos)
        {
        if(cptr->flags & LAST)
            {
            coff = OFF_RIGHT;
            break;
            }

        coff++;

        sofar += colwidth(cptr++->colno);
        }

    tracef2("calcoff(%d) returns %d\n", xpos, coff);
    return(coff);
}


#if RISCOS

/********************************************************************
*                                                                   *
* offset in horzvec that gives horizontal position on screen,       *
* mapping off left/right to left/right col of data visible on sheet *
*                                                                   *
********************************************************************/

extern coord
calcoff_click(coord xpos)
{
    SCRCOL *cptr;
    coord coff;
    coord sofar;

    if(xpos < borderwidth)
        return(0);                      /* OFF_LEFT, -1 -> first column */

    /* loop till we've passed the xpos or falled off horzvec */

    cptr  = horzvec();
    coff  = -1;
    sofar = borderwidth;

    while(sofar <= xpos)
        {
        if(cptr->flags & LAST)
            break;                      /* OFF_RIGHT -> last kosher column */

        coff++;

        sofar += colwidth(cptr++->colno);
        }

    tracef2("calcoff_click(%d) returns %d\n", xpos, coff);
    return(coff);
}

#endif /* RISCOS */


/*
 * takes real column number, not offset in table
 * is colno a fixed column on screen?
*/

extern BOOL
incolfixes(colt colno)
{
    coord coff;

    if(!(horzvec()->flags & FIX))
        return(FALSE);

    coff = schcsc(colno);

    return((coff != NOTFOUND)  &&  (horzvec_entry(coff)->flags & FIX));
}


/*
 * search for row in vertvec
 * return offset of row in table, or NOTFOUND
*/

extern coord
schrsc(rowt row)
{
    SCRROW *i_rptr  = vertvec();
    SCRROW *rptr    = i_rptr;

    tracef1("schrsc(%d)\n", row);

    while(!(rptr->flags & LAST))
        {
        tracef1("comparing with row %d\n ", rptr->rowno);

        if((rptr->rowno == row)  &&  !(rptr->flags & PAGE))
            return((coord) (rptr - i_rptr));

        rptr++;
        }

    tracef0("row not found\n");
    return(NOTFOUND);
}


/*
 * search for column in horzvec
 * return offset of column in table, or NOTFOUND
*/

extern coord
schcsc(colt col)
{
    SCRCOL *i_cptr  = horzvec();
    SCRCOL *cptr    = i_cptr;

    while(!(cptr->flags & LAST))
        {
        if(cptr->colno == col)
            return((coord) (cptr - i_cptr));

        cptr++;
        }

    return(NOTFOUND);
}


/****************************
*                           *
*  toggle column fix state  *
*                           *
****************************/

extern void
FixColumns_fn(void)
{
    uchar flags;
    SCRCOL *cptr, *last_cptr;

    if(!mergebuf())
        return;

    cptr = horzvec();

    if(cptr->flags & FIX)
        {
        last_cptr = cptr + n_colfixes;
        n_colfixes = 0;
        }
    else
        {
        n_colfixes = curcoloffset + 1;
        last_cptr = cptr + n_colfixes;
        }

    while((cptr < last_cptr)  &&  !((flags = cptr->flags) & LAST))
        cptr++->flags = flags ^ FIX;

    out_rebuildhorz = xf_drawcolumnheadings = out_screen = TRUE;
    filealtered(TRUE);
}


/*
 * fill column table horzvec; forces column headings to be redrawn
 *
 * scrbrc: if all columns fit, scrbrc is offset of end marker ie 1+last col
 * if last column doesn't fit, scrbrc is offset of truncated column
*/

extern void
filhorz(colt nextcol, colt currentcolno)
{
    intl scrwidth = (intl) cols_available;
    colt trycol;
    colt o_curcol = curcol;
    SCRCOL *scol = horzvec();
    colt first_col_number = scol->colno;
    colt fixed_colno = first_col_number;
    coord o_colsonscreen  = colsonscreen;
    colt o_ncx = fstncx();
    coord maxwrap = 0;
    coord this_colstart = 0;
    coord this_wrap;
    uchar fixed;
    intl tlength;
    coord vecoffset = 0;

    tracef2("[filhorz: nextcol %d curcolno %d]\n", nextcol, currentcolno);

    out_rebuildhorz = FALSE;

    colsonscreen = 0;

    while(vecoffset < numcol)
        {
        fixed = (vecoffset < n_colfixes) ? FIX : 0;

        scol = horzvec_entry(vecoffset);

        tracef2("[filhorz: vecoffset %d, fixed %s]\n", vecoffset, trace_boolstring(fixed));

        if(!fixed)
            do  {
                trycol = nextcol++;

                if(trycol >= numcol)
                    {
                    scrbrc = vecoffset;
                    scol->flags = LAST;
                    goto H_FILLED;
                    }
                }
            while(incolfixes(trycol)  ||  !colwidth(trycol));
        else
            trycol = fixed_colno++;

        scol->colno = trycol;
        scol->flags = fixed;

        if(trycol == currentcolno)
            curcoloffset = vecoffset;

        tlength = colwidth(trycol);

        if(tlength)
            {
            /* right margin position of this column */
            this_wrap = this_colstart + colwrapwidth(trycol);
            maxwrap = max(maxwrap, this_wrap);

            /* righthand edge of this column */
            this_colstart += tlength;

            scrwidth -= tlength;

            ++colsonscreen;

            if(scrwidth <= 0)
                break;

            ++vecoffset;
            }
        }

    /* three situations:
     * 1) ran out of columns: srcbrc = vecoffset (LAST)
     * 2) filled exactly:     srcbrc = vecoffset + 1 (LAST)
     * 3) ran out of space:   srcbrc = vecoffset (last col)
    */
    scrbrc = vecoffset + (scrwidth == 0);
    (scol+1)->flags = LAST;

H_FILLED:

    curcoloffset = min(curcoloffset, colsonscreen-1);
    curcol = col_number(curcoloffset);

    if(curcol != o_curcol)
        lecpos = lescrl = 0;

    /* how long to draw a horizontal bar - rightmost right margin or column edge */
    hbar_length = max(this_colstart, maxwrap);

    tracef2("[filhorz: scrbrc := %d, numcol %d]\n", scrbrc, numcol);
    tracef3("[filhorz: colsonscreen %d != o_colsonscreen %d    %s]\n",
            colsonscreen, o_colsonscreen, trace_boolstring(colsonscreen != o_colsonscreen));
    tracef3("[filhorz: col_number(0) %d != first_col_number %d %s]\n",
            col_number(0), first_col_number, trace_boolstring(col_number(0) != first_col_number));
    tracef3("[filhorz: o_ncx %d != fstncx() %d                 %s]\n",
            o_ncx, fstncx(), trace_boolstring(o_ncx != fstncx()));

    /* if different number of columns, need to redraw */
    if( (colsonscreen  != o_colsonscreen)       ||
        (col_number(0) != first_col_number)     ||
        (n_colfixes  &&  (fstncx() != o_ncx))   )
            xf_drawcolumnheadings = out_screen = TRUE;
}


extern void
FixRows_fn(void)
{
    SCRROW *rptr;
    uchar flags;

    if(!mergebuf())
        return;

    rptr = vertvec();

    if(rptr->flags & FIX)
        {
        n_rowfixes = 0;

        while(!((flags = rptr->flags) & LAST))
            rptr++->flags = flags & ~FIX;       /* clear fix bit */

        /* reevaluate page numbers */
        update_variables();
        }
    else
        {
        rowt thisrow = rptr->rowno;

        while((thisrow <= currow)  &&  !((flags = rptr->flags) & LAST))
            {
            rptr->rowno   = thisrow++;
            rptr++->flags = flags | FIX;
            }

        n_rowfixes = (coord) (currow - row_number(0) + 1);
        }

    out_rebuildvert = out_screen = TRUE;
    filealtered(TRUE);
}


/****************************
*                           *
*  centre column on screen  *
*                           *
****************************/

extern void
cencol(colt colno)
{
    intl stepback;
    intl firstcol;
    coord fixwidth = 0;
    SCRCOL *cptr = horzvec();

    tracef1("[cencol(%d)]\n", colno);

    colno = min(colno, numcol-1);

    while(!(cptr->flags & LAST)  &&  (cptr->flags & FIX))
        fixwidth += colwidth(cptr++->colno);

    stepback = (cols_available - fixwidth) / 2;
    stepback = max(stepback, 0);

    for(firstcol = (intl) colno; (stepback > 0) && (firstcol >= 0); --firstcol)
        {
        while((firstcol > 0)  &&  (incolfixes((colt) firstcol) || !colwidth(firstcol)))
            --firstcol;

        stepback -= colwidth((colt) firstcol);
        }

    firstcol += 1 + (stepback < 0);
    firstcol = min(firstcol, colno);

    filhorz((colt) firstcol, colno);
}


/***********************
*                      *
* centre row on screen *
*                      *
***********************/

extern void
cenrow(rowt rowno)
{
    intl stepback;
    rowt firstrow;

    rowno = min(rowno, numrow - 1);

    stepback = (rows_available - n_rowfixes - 1) / 2;

    for(firstrow = rowno; (stepback > 0)  &&  (firstrow >= 0); --firstrow, --stepback)
        while(inrowfixes(firstrow)  &&  (firstrow > 0))
            --firstrow;

    firstrow = max(firstrow, 0);

    /* ensure a page-break at tos doesn't confuse things */
/*  vertvec->flags &= ~PAGE; */
    out_screen = TRUE;

    filvert(firstrow, rowno, CALL_FIXPAGE);
}


/********************************
*                               *
*  fill vertical screen vector  *
*                               *
********************************/

extern void
filvert(rowt nextrow, rowt currentrowno, BOOL call_fixpage)
{
    SCRROW *rptr;
    coord vecoffset;
    coord lines_on_screen = rows_available;
    intl temp_pagoff;
    uchar saveflags;
    rowt first_row_number = row_number(0);
    slotp tslot;
    BOOL on_break;
    #if RISCOS
    intl pictrows;

    pict_currow = currow;
    pict_on_currow = 0;
    #endif

    out_forcevertcentre = out_rebuildvert = FALSE;

    tracef3("************** filvert(%d, %d, %s)\n",
            nextrow, currentrowno, trace_boolstring(call_fixpage));

    if(nextrow < 0)
        nextrow = 0;

    /* update the page numbering if allowed to and no fixes */
    if(call_fixpage  &&  !n_rowfixes  &&  chkfsb())
        fixpage(fstnrx(), nextrow);

    curpnm = pagnum;
    temp_pagoff = pagoff;

    for(vecoffset = 0, rowsonscreen = 0
        #if RISCOS
        , pictrows = 0, pict_on_screen = 0
        #endif
        ;
        lines_on_screen > 0;
        nextrow++, rowsonscreen++, lines_on_screen--, vecoffset++)
        {
        on_break = FALSE;

        rptr = vertvec_entry(vecoffset);

        saveflags = rptr->flags & (~FIX & ~LAST & ~PAGE & ~PICT & ~UNDERPICT);

        tracef2("filvert - rptr &%p, %d\n", rptr, vecoffset);

        if(vecoffset < n_rowfixes)
            {
            rptr->rowno = first_row_number + vecoffset;
            saveflags |= FIX;
            }

        if(nextrow >= numrow)
            {
            rptr->flags = LAST;
            rptr->rowno = 0;
            tracef1("[filvert pict_on_screen is: %d]\n", pict_on_screen);
            return;
            }

        /* if not on a fixed row */
        if(!(saveflags & FIX))
            {
            while(inrowfixes(nextrow))
                nextrow++;

            if(nextrow >= numrow)
                {
                rptr->flags = LAST;
                rptr->rowno = 0;
                tracef1("[filvert pict_on_screen is: %d]\n", pict_on_screen);
                return;
                }

            /* do pages if rows not fixed */
            if(chkfsb())
                {
                on_break = TRUE;
                /* on break */

                /* if hard break - do chkrpb(nextrow) explicitly to save time */
                tslot = travel(0, nextrow);
                if(tslot  &&  (tslot->type == SL_PAGE))
                    {
                    if(chkpbs(nextrow, temp_pagoff))
                        temp_pagoff = 0;
                    elif(temp_pagoff == 0)
                        saveflags |= PAGE;
                    else
                        {
                        temp_pagoff -= enclns;
                        on_break = FALSE;
                        }
                    }
                elif(temp_pagoff == 0)
                    saveflags |= PAGE;
                else
                    on_break = FALSE;
                }

            #if RISCOS
            if(!on_break && !(saveflags & PAGE))
                {
                drawfep dfp;
                drawfrp dfrp;

                /* check for a picture */
                if(draw_file_refs &&
                   draw_find_file(current_document_handle(),
                                  -1, nextrow, &dfp, &dfrp))
                    {
                    if(nextrow == currentrowno)
                        pict_on_currow = TRUE;
                    else
                        {
                        tracef2("[filvert found draw file row: %d, %d rows]\n",
                                nextrow, tsize_y(dfrp->ysize_os));
                        saveflags |= PICT;
                        pictrows = tsize_y(dfrp->ysize_os);
                        }
                    }
                }
            #endif

            rptr->rowno = nextrow;
            rptr->page  = curpnm;

            /* chkpba() */
            if(encpln > 0)
                {
                /* add line spacing */
                temp_pagoff += enclns;
                if(temp_pagoff > encpln)
                    temp_pagoff = 0;
                }

            if(on_break)
                {
                curpnm++;
                #if RISCOS
                pictrows = 0;
                #endif
                }

            if(saveflags & PAGE)
                {
                nextrow--;
                #if RISCOS
                pictrows = 0;
                #endif
                }
            }

        #if RISCOS
        if(pictrows)
            {
            pictrows--;
            saveflags |= UNDERPICT;
            }

        if(saveflags & PICT)
            plusab(pict_on_screen, ((intl) nextrow + 1) * (rowsonscreen + 1));
        #endif

        rptr->flags = saveflags;

        if(saveflags & FIX)
            nextrow--;

        /* mark current row position in vector */
        if((rptr->rowno == currentrowno)  &&  !(saveflags & PAGE))
            currowoffset = rowsonscreen;
        }

    vertvec_entry(rowsonscreen)->flags = LAST;

    tracef1("[filvert pict_on_screen is: %d]\n", pict_on_screen);
}


/************************************
*                                   *
* is this row fixed on the screen ? *
* real row number, not offset       *
*                                   *
************************************/

extern BOOL
inrowfixes(rowt row)
{
    return(n_rowfixes  &&  inrowfixes1(row));
}


extern BOOL
inrowfixes1(rowt row)
{
    coord roff = schrsc(row);

    return((roff != NOTFOUND)  &&  (vertvec_entry_flags(roff) & FIX));
}


/***************************************
*                                      *
* check row for conditional page break *
* real row number, not offset          *
*                                      *
***************************************/

extern BOOL
chkrpb(rowt rowno)
{
    slotp tslot = travel(0, rowno);

    return(tslot  &&  (tslot->type == SL_PAGE));
}


/*
 * set hard page offset and return whether active
*/

extern BOOL
chkpbs(rowt rowno, intl offset)
{
    travel(0, rowno)->content.page.cpoff = offset;

    return(chkpac(rowno));
}


/*
 * check if hard page break is expanded
 *
 * hard page break stores offset of line in current page
*/

extern BOOL
chkpac(rowt rowno)
{
    slotp tslot = travel(0, rowno);
    intl condval = tslot->content.page.condval;
    intl rowonpage = tslot->content.page.cpoff;

    return((condval == 0)  ||  (condval >= (encpln-rowonpage) / enclns));
}


/****************************************
*                                       *
* get first non fixed column on screen  *
* if all fixed, return first column     *
* returns actual column number          *
*                                       *
***************************************/

extern colt
fstncx(void)
{
    SCRCOL *i_cptr  = horzvec();
    SCRCOL *cptr    = i_cptr - 1;

    while(!((++cptr)->flags & LAST))
        if(!(cptr->flags & FIX))
            return(cptr->colno);

    return(i_cptr->colno);
}


/************************************
*                                   *
* get first non fixed row on screen *
* if all fixed, return first row    *
* returns actual row number         *
*                                   *
************************************/

extern rowt
fstnrx(void)
{
    SCRROW *i_rptr  = vertvec();
    SCRROW *rptr    = i_rptr - 1;

    while(!((++rptr)->flags & LAST))
        if(!(rptr->flags & (FIX | PAGE)))
            return(rptr->rowno);

    return(i_rptr->rowno);
}


/************************************************************************
*                                                                       *
* update all the variables that depend upon the page layout options box *
* and the global options box                                            *
*                                                                       *
************************************************************************/

extern void
update_variables(void)
{
    intl idx;
    intl linespace;
    rowt oldtop;
    uchar old_date = d_options_DF;

    iowbit = (uchar) (d_options_IW == 'R'); /* insert on wrap */
    txnbit = (uchar) (d_options_TN == 'T'); /* text/numbers */
    borbit = (uchar) (d_options_BO == 'Y'); /* borders on/off */

    /*
     * note that all the following variables depend on borbit
     * this should only happen in the global options menu
    */
    borderwidth  = (borbit) ? BORDERWIDTH  : 0;
    borderheight = (borbit) ? BORDERHEIGHT : 0;

    if((rows_available = paghyt + 1 - BORDERLINE - borderheight) < 0)
        rows_available = 0;
    if((cols_available = pagwid_plus1 - borderwidth) < 0)
        cols_available = 0;

    jusbit = (uchar) (d_options_JU == 'Y'); /* justified text */
    wrpbit = (uchar) (d_options_WR == 'Y'); /* wrapped text */
    rcobit = (uchar) (d_recalc_AM == 'A');  /* recalculate mode */
    minbit = (uchar) (d_options_MB == 'M'); /* minus/brackets */

    if(old_date != d_options_DF)
        global_recalc = recalc_bit = TRUE;

    linespace = (intl) d_poptions_LS;
    if(linespace < 1)
        linespace = 1;

    idx =     (intl) d_poptions_PL
            - (intl) d_poptions_TM
            - (intl) d_poptions_HM
            - (intl) d_poptions_FM
            - (intl) d_poptions_BM;

    if(!str_isblank(d_poptions_HE))
        idx--;

    if(!str_isblank(d_poptions_FO))
        idx--;

    /* don't let encpln go to dangerous values */

    encpln = (idx <= linespace) ? 0 : idx;
    enclns = linespace;

    real_pagelength = encpln;

    /* set encpln to next multiple of enclns */

    while((encpln % enclns) != 0)
        encpln++;

    if(!n_rowfixes  &&  vertvec_mh)
        {
        oldtop = fstnrx();
        filvert((rowt) 0, (rowt) 0, CALL_FIXPAGE);
        reset_filpnm();
        filvert(oldtop, currow, CALL_FIXPAGE);
        }

    #if !RISCOS
    if(vertvec_mh)
        {
        SCRROW *rptr        = vertvec();
        SCRROW *last_rptr   = rptr + rows_available;

        while(rptr <= last_rptr)
            rptr++->length = pagwid_plus1;
        }
    #endif

    #if !defined(HEADLINE_OFF)
    xf_drawmenuheadline =
    #endif
        out_screen = out_rebuildhorz = out_rebuildvert = out_currslot = TRUE;

    #if RISCOS
    grid_on = (d_options_GR == 'Y');
    new_grid_state();
    /* may need to move if grid state different */
    if(main_window == caret_window)
        xf_acquirecaret = TRUE;
    (void) new_window_height(windowheight());
    my_force_redraw(main_window);
    #endif
}


/***********************************************************
*                                                          *
* get size of string accounting for highlights etc         *
* ie. length of string when drawn on screen by draw_slot() *
*                                                          *
***********************************************************/

extern intl
calsiz(uchar *str)
{
    intl size;

    for(size = 0; *str /*!= '\0'*/; str++)
        if(!ishighlight(*str))
            size++;

    return(size);
}


/*
*/

static void
rebols(void)
{
    newcol = oldcol;
    newrow = oldrow;
    chkmov();
    curosc();
}


/*
 * all calls to this should be followed by a curosc()
*/

extern void
chkmov(void)
{
    colt  tcol;
    coord coff;
    coord roff, troff;

    tracef4("[chkmov(): newcol %d newrow %d oldcol %d oldrow %d]\n", newcol, newrow, oldcol, oldrow);

    if(!colwidth(newcol))
        {
        tracef1("[chkmov: colwidth(%d) == 0 --- silly!]\n", newcol);

        /* find somewhere to go then! */
        tcol = newcol;

        while((tcol >= 0)  &&  !colwidth(tcol))
            --tcol;

        if(tcol == -1)
            {
            tracef0("[chkmov: fell off left; try marching right]\n");
            tcol = newcol;
            while((tcol < numcol)  &&  !colwidth(tcol))
                ++tcol;
            }

        tracef2("[chkmov: colwidth(%d) == 0; setting newcol %d]\n", newcol, (tcol != numcol) ? tcol : curcol);
        newcol = (tcol != numcol) ? tcol : curcol;
        }

    curcol = newcol;

    if(oldcol != newcol)
        xf_drawcolumnheadings = TRUE;

    coff = schcsc(newcol);

    /* when coff == scrbrc, only part of the column is on the screen */
    if((coff == NOTFOUND)  ||  (coff == scrbrc))
        {
        out_screen = TRUE;
        cencol(newcol);
        }
    else
        curcoloffset = coff;


    currow = newrow;

    if( (inrowfixes(newrow)  &&  out_forcevertcentre)   ||
        ((roff = schrsc(newrow)) == NOTFOUND)           )
        {
        cenrow(newrow);
        out_screen = TRUE;
        }
    else
        {
        troff = schrsc(oldrow);

        if(troff != NOTFOUND)
            {
            if(newcol == oldcol)
                mark_row_praps(troff, NEW_ROW);
            else
                mark_row(troff);
            }

        currowoffset = roff;

        if((newcol == oldcol)  &&  !word_to_invert)
            mark_row_praps(currowoffset, NEW_ROW);
        else
            mark_row(currowoffset);
        }
}


extern void
curosc(void)
{
    SCRCOL *cptr;
    SCRROW *rptr;

    if(curcoloffset >= scrbrc)
        {
        /* if nothing on screen */

        cptr = horzvec();

        if(!scrbrc  &&  (cptr->flags & LAST))
            {
            rebols();
            return;
            }

        curcoloffset = (cptr[scrbrc].flags & LAST)
                            ? (scrbrc-1)
                            : scrbrc;

        curcol = cptr[curcoloffset].colno;
        }


    rptr = vertvec();

    if(rptr->flags & LAST)
        {
        rebols();
        return;
        }

    /* row not off end, should also check page break */

    currowoffset = min(currowoffset, rowsonscreen-1);

    if(rptr[currowoffset].flags & PAGE)
        {
        rebols();
        return;
        }

    currow = rptr[currowoffset].rowno; /* must be outside previous block */
}


/************************
*                       *
*  check slot is blank  *
*                       *
************************/

extern BOOL
isslotblank(slotp tslot)
{
    uchar *str;
    uchar ch;

    if(!tslot)
        return(TRUE);

    switch(tslot->type)
        {
        case SL_TEXT:
            str = tslot->content.text;
            break;

        case SL_INTSTR:
            str = tslot->content.number.text +
                  tslot->content.number.result.str_offset;
            break;

        case SL_PAGE:
        default:
            return(FALSE);
        }

    /* check if only characters in slot are spaces
     * Is done explicitly for speed
    */
    do { ch = *str++; } while(ch == SPACE);

    return(ch == '\0');
}


#if RISCOS

/********************************************************************
*                                                                   *
*  decide what column a click at the given text cell would go into  *
*                                                                   *
********************************************************************/

/* offset of given text cell in column that we decided upon */
intl newoffset;

extern coord
get_column(coord tx, rowt trow, intl xcelloffset, BOOL selectclicked)
{
    coord coff = calcoff_click(tx); /* actual grid address with l/r map */
    coord trycoff;
    colt  tcol;
    slotp tslot;
    intl tryoffset;

    /* ensure we can find caret in current slot! */
    (void) mergebuf_nocheck();
    filbuf();

    newoffset = 0;

    if(selectclicked)
        {
        trycoff = coff;

        tracef0("SELECT: find the leftmost column\n");

        /* find any preceding non-blank slot,
         * or stop at first column on screen.
        */
        do  {
            tryoffset = tx - calcad(trycoff);
            tcol  = col_number(trycoff);
            tslot = travel(tcol, trow);
            }
        while(!tslot  &&  (--trycoff >= 0));

        /* Manic jump-left always clicking */
        if(!tslot)
            {
            tracef0("no non-blank slots: got slot at coff 0\n");
            return(0);
            }
        elif(tslot->type == SL_TEXT)
            {
            newoffset = cal_offset_in_slot(tcol, trow, tslot,
                                           tryoffset, xcelloffset);
            tracef2("preceding text slot at coff %d, newoffset %d\n", trycoff, newoffset);
            return(trycoff);
            }
        else
            {
            tracef0("preceding non-text slot: first non-blank one\n");
            return((trycoff == coff) ? trycoff : trycoff + 1);
            }
        }

    tracef0("ADJUST: found the underlying column\n");

    tcol  = col_number(coff);
    tslot = travel(tcol, trow);

    if(tslot  &&  (tslot->type == SL_TEXT))
        newoffset = cal_offset_in_slot(tcol, trow, tslot,
                                       tx - calcad(coff), xcelloffset);

    return(coff);
}


/***************************************************
*                                                  *
* given a slot and a character + cell offset, work *
* out the new cursor offset position in the slot   *
*                                                  *
* MRJC 19/7/89                                     *
*                                                  *
***************************************************/

static intl
cal_offset_in_slot(colt col, rowt row, slotp sl,
                   intl offset_ch, intl cell_offset_os)
{
    BOOL in_linbuf  = (col == curcol)  &&  (row == currow);
    intl justify    = in_linbuf ? 0 : (sl->justify & J_BITS);
    intl fwidth_ch  = chkolp(sl, col, row);
    intl swidth_ch, count, gaps, whole, odd, non_odd_gaps, lead;
    char *c;
    #if RISCOS
    intl fwidth_adjust_ch;
    char tbuf[PAINT_STRSIZ];
    intl success;
    font_string fs;
    intl fwidth_mp, swidth_mp;
    intl this_font;
    char wid_buf[PAINT_STRSIZ];
    intl lead_spaces, lead_space_mp;
    intl spaces;
    #endif

    tracef4("[cal_offset_in_slot: (%d, %d): offset_ch = %d, cell_offset = %d (OS)]\n",
            col, row, offset_ch, cell_offset_os);

    if( offset_ch < 0)
        offset_ch = cell_offset_os = 0;

    #if RISCOS
    if(riscos_fonts)
        {
        success = FALSE;

        fs.s = tbuf;
        fs.x = os_to_mp(ch_to_os(offset_ch) + cell_offset_os);
        fs.y = 0;

        tracef4("[offset_ch: %d, offset_os: %d, fs.x: %d, fs.y: %d]\n",
                offset_ch, cell_offset_os, fs.x, fs.y);

        if(in_linbuf)
            /* get current fonty string */
            expand_current_slot_in_fonts(tbuf, FALSE, &this_font);
        else
            /* get a fonty string */
            font_expand_for_break(tbuf, sl->content.text);

        swidth_mp = font_width(tbuf);

        /* never look for characters that won't be plotted */
        switch(justify)
            {
        /*  case J_CENTRE:      */
        /*  case J_RIGHT:       */
        /*  case J_LCR:         */
        /*  case J_LEFTRIGHT:   */
        /*  case J_RIGHTLEFT:   */
            default:
                fwidth_adjust_ch = 1;
                break;

            case J_FREE:
            case J_LEFT:
                fwidth_adjust_ch = 0;
                break;
            }

        fwidth_ch -= fwidth_adjust_ch;
        fwidth_mp = ch_to_mp(fwidth_ch);

        tracef1("[cal_offset_in_slot: fwidth_ch = %d]\n", fwidth_ch);
        tracef2("[cal_offset_in_slot: fwidth_mp = %d, swidth_mp = %d]\n", fwidth_mp, swidth_mp);

        if( swidth_mp > fwidth_mp)
            {
            swidth_mp = font_truncate(tbuf, fwidth_mp + ch_to_mp(fwidth_adjust_ch));

            justify = 0;
            }

        /* what a muddle are the font calls; they NEVER do
         * what you want, resulting in messes like this...
        */
        if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
            {
            /* strip the goddamn leading spaces */
            lead_space_mp = font_strip_spaces(wid_buf, tbuf, &lead_spaces);

            tracef3("[lead_spaces: %d, lead_space_mp: %d, fs.x: %d]\n",
                    lead_spaces, lead_space_mp, fs.x);

            /* see if we clicked past the leading spaces */
            if(fs.x > lead_space_mp)
                {
                #if TRACE
                trace_system("memory %p + 30", tbuf);
                #endif

                fs.s = wid_buf;
                fs.x -= lead_space_mp;
                fwidth_mp -= lead_space_mp;
                swidth_mp = font_width(wid_buf);

                /* do you believe that you have to do this ? */
                c = wid_buf;
                spaces = 0;
                while(*c)
                    if(*c == SPACE)
                        {
                        ++c;
                        ++spaces;
                        continue;
                        }
                    else
                        c += font_skip(c);
                /* I didn't */

                tracef4("[fwidth_mp: %d, swidth_mp: %d, lead_space_mp: %d, spaces: %d]\n",
                        fwidth_mp, swidth_mp, lead_space_mp, spaces);

                if(spaces  &&  (swidth_mp > 0)  &&  (fwidth_mp >= swidth_mp))
                    {
                    font_complain(
                        font_findcaretj(&fs,
                                        (fwidth_mp - swidth_mp) / spaces,
                                        0)
                                 );
                    fs.term += lead_spaces;
                    success = TRUE;
                    /* well, everything's relative */

                    tracef5("[offset_ch: %d, offset_os: %d, fs.x: %d, fs.y: %d, fs.term: %d]\n",
                            offset_ch, cell_offset_os, fs.x, fs.y, fs.term);
                    }
                }
            }

        /* at least it's fairly simple in the unjustified case */
        if(!success)
            {
            font_complain(font_findcaret(&fs));

            tracef5("[offset_ch: %d, offset_os: %d, fs.x: %d, fs.y: %d, fs.term: %d]\n",
                    offset_ch, cell_offset_os, fs.x, fs.y, fs.term);
            }

        /* skip fonty stuff to get real position */
        c = tbuf;
        offset_ch = in_linbuf ? lescrl : 0;
        while(*c  &&  (fs.term > 0))
            if(is_font_change(c))
                {
                /* miss initial font change */
                if(c != tbuf)
                    ++offset_ch;
                fs.term -= font_skip(c);
                c += font_skip(c);
                }
            else
                {
                if(in_linbuf)
                    {
                    char ch = linbuf[offset_ch];

                    tracef4("[cal_offset_in_slot: linbuf[%d] = %c %d, fs.term = %d]\n", offset_ch, ch, ch, fs.term);

                    /* if clicked in the middle of a highlight/expanded ctrlchar, maybe put caret to left */
                    if(ishighlight(ch))
                        {
                        if(fs.term < 2)
                            break;

                        fs.term -= 3 - 1;
                        }
                    elif((ch < SPACE)   ||
                         ((ch >= 127)  &&  (ch < 160)  &&  !font_charwid(this_font, ch)))
                        {
                        if(fs.term < 3)
                            break;

                        fs.term -= 4 - 1;
                        }
                    }
                --fs.term;
                ++offset_ch;
                ++c;
                }
        }
    else
    #endif
        {
        if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
            {
            swidth_ch = 0;

            c = sl->content.text;

            while(*c++ == SPACE)
                ++swidth_ch;
            --c;

            for(gaps = 0; *c; ++c)
                {
                /* highlights have zero width */
                if(ishighlight(*c))
                    continue;

                /* count gaps */
                if(*c == SPACE)
                    {
                    do { ++swidth_ch; } while(*++c == SPACE);

                    if(*c--)
                        ++gaps;

                    continue;
                    }

                /* stop on @A1@ field */
                if(*c == SLRLDI)
                    {
                    justify = 0;
                    break;
                    }

                ++swidth_ch;
                }

            if((swidth_ch >= fwidth_ch)  ||  !gaps)
                justify = 0;

            if(justify)
                {
                whole = (fwidth_ch - swidth_ch) / gaps;
                odd = fwidth_ch - (whole * gaps) - swidth_ch;
                if(justify == J_LEFTRIGHT)
                    non_odd_gaps = 0;
                else
                    non_odd_gaps = gaps - odd;
                }

            tracef6("[cal_off gaps: %d, fwidth: %d, swidth: %d, whole: %d, odd: %d, non_oddg: %d]\n",
                    gaps, fwidth_ch, swidth_ch, whole, odd, non_odd_gaps);
            }
        else
            justify = 0;

        /* account for highlights */
        if(!in_linbuf)
            for(c = sl->content.text, count = offset_ch, gaps = 0, lead = 1;
                *c && count > 0;
                ++c, --count)
                switch(*c)
                    {
                    /* give up if we get an SLR - too hard */
                    case SLRLDI:
                        count = 0;
                        break;

                    case HIGH_UNDERLINE:
                    case HIGH_BOLD:
                    case HIGH_ITALIC:
                    case HIGH_SUBSCRIPT:
                    case HIGH_SUPERSCRIPT:
                        ++offset_ch;
                        ++count;
                        lead = 0;
                        break;

                    case SPACE:
                        {
                        intl to_remove, odd_used;

                        if(lead || !justify)
                            break;
                        ++gaps;
                        odd_used = (odd && (gaps > non_odd_gaps)) ? 1 : 0;
                        to_remove = whole + odd_used;
                        offset_ch -= to_remove;
                        count -= to_remove;
                        odd -= odd_used;
                        tracef3("[gaps: %d, odd_used: %d, to_remove: %d]\n",
                                gaps, odd_used, to_remove);
                        }

                    default:
                        lead = 0;
                        break;
                    }

        /* move on one if click in RHS of character */
        if(cell_offset_os > (charwidth*3/4))
            ++offset_ch;
        }

    tracef0("\n");

    /* check that offset is inside string */
    if(in_linbuf)
        return(min(offset_ch, strlen(linbuf)));

    c = sl->content.text;
    while(*c)
        if(*c++ == SLRLDI)
            c += SLRSIZE - 1;

    return(min(offset_ch, c - sl->content.text));
}


/****************************************************************************
*                                                                           *
*  insert a reference to a (possibly) external slot in the given document   *
*                                                                           *
****************************************************************************/

extern void
insert_reference_to(dochandle insdoc, dochandle tdoc, colt tcol, rowt trow, BOOL allow_draw)
{
    char array[LIN_BUFSIZ+1];
    docno d = (insdoc == tdoc) ? 0 : ensure_cur_docno();
    dochandle doc = current_document_handle();

    tracef4("inserting reference to doc %d, col #%d, row #%d in doc %d\n", tdoc, tcol, trow, insdoc);

    /* remember where we put it for case of drag */
    insert_reference_stt_offset = lecpos;

    insert_reference_slr.col = tcol;
    insert_reference_slr.row = trow;
    insert_reference_slr.doc = tdoc;    /* abuse of field to store wrong type */

    /* expand column then row into array */
    writeref(array, d, tcol, trow);

    select_document_using_handle(insdoc);

    insert_string(array, FALSE);

    insert_reference_end_offset = lecpos;

    if(allow_draw)
        draw_screen();              /* in expr. editing doc */

    select_document_using_handle(doc);
}


extern void
application_click(coord tx, coord ty, intl xcelloffset, BOOL selectclicked)
{
    dochandle doc   = current_document_handle();
    BOOL blkindoc   = (blkstart.col != NO_COL)  &&  (doc == blkdochandle);
    BOOL shiftpressed = depressed_shift();
    coord coff      = calcoff(tx);  /* not _click */
    coord roff      = calroff(ty);  /* not _click */
    colt  tcol;
    rowt  trow;
    SCRROW *rptr;
    window_data *wdp;
    intl len;
    BOOL acquire    = FALSE;
    BOOL motion     = FALSE;

    /* stop us wandering off bottom of sheet */
    roff = min(roff, rowsonscreen - 1);

    tracef0("it's a click: ");

    if(roff >= 0)
        {
        rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
            {
            tracef0("in soft page break - click ignored\n");
            acquire = TRUE;
            }
        else
            {
            trow = rptr->rowno;

            if( (coff >= 0)  ||  (coff == OFF_RIGHT)  ||
                ((coff == OFF_LEFT)  &&  !shiftpressed))
                {
                wdp = find_document_using_window(caret_window);

                if(wdp  &&  wdp->Xxf_inexpression)
                    {
                    coff = calcoff_click(tx);
                    tcol = col_number(coff);

                    tracef0("editing expression, use ADJUST paradigm: ");

                    insert_reference_to(wdp->DocHandle, doc, tcol, trow, TRUE);
                    }
                else
                    {
                    if(chkrpb(trow)  &&  chkfsb()  &&  chkpac(trow))
                        {
                        tracef0("in hard page break - go to the start\n");
                        tcol = 0;
                        newoffset = 0;
                        }
                    else
                        {
                        coff = get_column(tx, trow, xcelloffset, selectclicked);
                        tcol = col_number(coff);
                        }

                    tracef2("not editing expression: in sheet at row #%d, col #%d\n", trow, tcol);

                    if(shiftpressed)
                        {
                        /* either alter current block or set new block:
                         * mergebuf has been done by caller to ensure slot marking correct
                        */
                        if(blkindoc)
                            {
                            tracef0("shift pressed - adjust marked block\n");
                            make_single_mark_into_block();
                            alter_marked_block(tcol, trow);
                            }
                        else
                            {
                            tracef0("shift pressed - set marked block\n");
                            set_marked_block(curcol, currow, tcol, trow, TRUE);
                            }
                        }
                    else
                        {
                        if((tcol != curcol)  ||  (trow != currow))
                            {
                            /* position caret in new slot; mergebuf has been done by caller */
                            slot_in_buffer = FALSE;
                            chknlr(tcol, trow);
                            lescrl = 0;
                            }

                        lecpos = newoffset;

                        acquire = motion = TRUE;
                        }
                    }
                }
            elif(IN_ROW_BORDER(coff))
                {
                if(shiftpressed)
                    {
                    /* either alter current block or set new block:
                     * mergebuf has been done by caller to ensure slot marking correct
                    */
                    if(blkindoc)
                        {
                        tracef0("alter number of marked rows\n");
                        make_single_mark_into_block();
                        alter_marked_block(ACTIVE_COL, trow);
                        }
                    else
                        {
                        tracef0("mark all columns from caret to given row\n");
                        set_marked_block(0, currow, numcol-1, trow, TRUE);
                        }
                    }
                else
                    {
                    tracef0("shift not pressed - ignored\n");
                    acquire = TRUE;
                    }
                }
            else
                {
                tracef0("off left/right - ignored\n");
                acquire = TRUE;
                }
            }
        }
    elif(IN_COLUMN_HEADING(roff))
        {
        if(coff >= 0)
            {
            tcol = col_number(coff);

            if(shiftpressed)
                {
                /* either alter current block or set new block:
                 * mergebuf has been done by caller to ensure slot marking correct
                */
                if(blkindoc)
                    {
                    tracef0("alter number of marked columns\n");
                    make_single_mark_into_block();
                    alter_marked_block(tcol, ACTIVE_ROW);
                    }
                else
                    {
                    tracef0("mark all rows from caret to given column\n");
                    set_marked_block(curcol, 0, tcol, numrow-1, TRUE);
                    }
                }
            else
                {
                tracef0("shift not pressed - ignored\n");
                acquire = TRUE;
                }
            }
        else
            {
            tracef0("off left/right of column headings - ignored\n");
            acquire = TRUE;
            }
        }
    elif(ty == EDTLIN)
        {
        if((tx >= 0)  &&  (tx <= RHS_X))
            {
            tracef0("in valid part of edit line\n");

            if(xf_inexpression)
                {
                len = strlen(linbuf);

                if(tx >= len)
                    /* no right drift */
                    tx = len;
                else
                    /* if clicked in rh of cell, move caret to right */
                    if(xcelloffset >= (charwidth*3)/4)
                        tx++;

                lecpos = tx;
                lescrl = 0;

                acquire = motion = TRUE;
                }
            }
        else
            {
            tracef0("not in valid part of edit line - mostly ignored\n");
            acquire = TRUE;
            }
        }
    else
        {
        tracef0("above sheet data - mostly ignored\n");
        acquire = TRUE;
        }

    if(acquire  &&   (caret_window != main_window))
        xf_acquirecaret = TRUE;

    if(xf_acquirecaret  ||  motion)
        draw_screen();
}


#endif  /* RISCOS */

/* end of cursmov.c */
