/* scdraw.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       scdraw.c - screen drawing module for PipeDream
 * Author:      RJM August 1987
 * History:
 *  0.01    xx-Aug-87   RJM created
 *  0.02    31-Jan-89   SKS work for RISC OS version
 *  0.03    17-Feb-89   SKS fixed object intersects; was intersecting one cell+
 *                      conditional tracing, moved menuheadline drawing here
 *                      too. more hacking: split off bits into slotconv.c
 *  0.04    15-Mar-89   SKS split off bits into riscdraw
 *          30-Jun-89   MRJC added font bits
*/

/* standard header files */
#include "flags.h"


#if RISCOS
#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "wimpt.h"
#include "drawfdiag.h"
#include "font.h"
#elif MS
#include <graph.h>
#endif


#include "datafmt.h"

#if defined(VIEW_IO)
#include "viewio.h"
#endif


#if RISCOS
#include "ext.riscos"
#include "riscdraw.h"
#include "ext.pd"
#endif


/* exported functions */

extern void bottomline(intl xpos, intl ypos, intl xsize);
extern intl chkolp(slotp tslot, colt tcol, rowt trow);
extern void clear_editing_line(void);
extern void display_heading(intl idx);
extern void draw_one_altered_slot(colt col, rowt row);
extern void draw_row(intl roff);
extern void draw_screen(void);
#if RISCOS
extern intl dspfld(intl x, intl y, intl fwidth_ch);
#endif
extern intl font_strip_spaces(char *out_buf, char *str_in, intl *spaces);
extern intl font_truncate(char *str, intl width);
extern intl font_width(char *str);
extern BOOL is_overlapped(intl coff, intl roff);
#if defined(VIEW_IO)
extern intl justifyline(uchar *, intl, uchar, uchar *);
#else
extern intl justifyline(uchar *, intl, uchar);
#endif
extern intl limited_fwidth_of_slot(slotp tslot, colt tcol, rowt trow);
extern void my_rectangle(intl xpos, intl ypos, intl xsize, intl ysize);
extern void position_cursor(void);
extern void switch_off_highlights(void);
extern void topline(intl xpos, intl ypos, intl xsize);
extern void twzchr(char ch);


/* internal functions */

#if TRUE
static void adjust_lescrl(intl x, intl fwidth);
static intl adjust_rowout(intl rowoff);
static intl adjust_rowborout(intl rowoff);
static intl cal_dead_text(void);
static void check_output_current_slot(intl move);
static void draw_empty_bottom_of_screen(void);
static void draw_altered_slots(void);
static void draw_column_headings(void);
/*static void draw_contents_of_numslot(void);*/
#if defined(SHOW_CURRENT_ROW)
static void draw_row_border(intl roff);
#endif
static void draw_editing_line(void);
static void draw_screen_below(intl roff);
/*static void draw_slot(intl coff, intl roff, BOOL in_draw_row);*/
static void draw_slot_coordinates(void);
static void draw_slot_in_buffer(void);
static void draw_slot_in_buffer_in_place(void);
static intl end_of_block(intl xpos, rowt trow);
static intl limited_fwidth_of_slot_in_buffer(void);
static void really_draw_row_border(intl roff, intl rpos);
static intl really_draw_row_contents(intl roff, intl rpos);
static intl really_draw_slot(intl coff, intl roff, BOOL in_draw_row);
static void really_draw_slot_coordinates(char *str);
static BOOL setivs(colt, rowt);         /* set inverse, depends on slot,mark */
static intl stringout_field(char *str, intl fwidth);

#if RISCOS || 0
static void draw_spaces_with_grid(intl spaces, intl start);
#else
#define draw_spaces_with_grid(x,y) ospca(x)
#endif

#if RISCOS
static intl font_paint_justify(char *str, intl fwidth);
static void forced_draw_contents_of_numslot(riscos_redrawstr *r);
static void forced_draw_editing_line(riscos_redrawstr *r);
static void forced_draw_slot_coordinates(riscos_redrawstr *r);
static void maybe_draw_empty_bottom_of_screen(void);
static void maybe_draw_empty_right_of_screen(void);
static void maybe_draw_column_headings(void);
static void maybe_draw_contents_of_numslot(void);
static void maybe_draw_editing_line(void);
static void maybe_draw_pictures(void);
static BOOL maybe_draw_row(intl roff);
static void maybe_draw_row_border(intl roff, intl rpos);
static void maybe_draw_row_contents(intl roff, intl rpos);
static void maybe_draw_rows(void);
static void maybe_draw_slot_coordinates(void);
static void maybe_draw_unused_bit_at_bottom(void);
static void really_draw_contents_of_numslot(void);
static void really_draw_column_headings(void);
static void really_draw_editing_line(void);
/*static intl stringout_underlaid(const char *str);*/
/*static intl stringout_underlaid_field(char *str, intl fwidth);*/

#if !defined(HEADLINE_OFF)
static void maybe_draw_menu_headline(void);
static void really_draw_menu_headline(void);
#endif

#else
static void really_draw_contents_of_numslot(char *str);
#endif
#endif


#if MS || ARTHUR
#define stringout_underlaid(str)                stringout(str)
#define stringout_underlaid_field(str, fwidth)  stringout_field(str, fwidth)
#endif


#define cal_lescrl(fw) (max( ((lecpos <= lescrl)                \
                                    ? lecpos - ((fw) / 2)       \
                                    : (lecpos - (fw) > lescrl)  \
                                            ? lecpos - (fw)     \
                                            : lescrl),          \
                             0))

#if RISCOS
static intl screen_xad_os, screen_yad_os;
#endif


/* ----------------------------------------------------------------------- */

/* conditional tracing flags */

#define TRACE_DRAW      (TRACE && TRUE)
#define TRACE_MAYBE     (TRACE && TRUE)
#define TRACE_CLIP      (TRACE && FALSE)
#define TRACE_REALLY    (TRACE && TRUE)
#define TRACE_OVERLAP   (TRACE && FALSE)


#if RISCOS

/* ----------------- RISC OS specific screen drawing --------------------- */

/* now consider objects to repaint */

extern void
maybe_draw_screen(void)
{
    #if !defined(HEADLINE_OFF)
    maybe_draw_menu_headline();
    #endif
    maybe_draw_slot_coordinates();
    maybe_draw_contents_of_numslot();
    maybe_draw_editing_line();
    maybe_draw_column_headings();
    maybe_draw_empty_right_of_screen();
    maybe_draw_rows();
    maybe_draw_empty_bottom_of_screen();
    maybe_draw_pictures();
    /* erase bit at bottom after all drawn */
    maybe_draw_unused_bit_at_bottom();
}


/************************************************
*                                               *
* print a string, clearing out bg if necessary  *
* graphics cursor advanced to next text cell    *
*                                               *
************************************************/

static intl
stringout_underlaid(const char *str)
{
    intl fwidth = strlen(str);

    vtracef1(TRACE_REALLY, "stringout_underlaid(\"%s\")\n", str);

    clear_underlay(fwidth);

    return(stringout(str));
}


static intl
stringout_underlaid_field(char *str, intl fwidth)
{
    vtracef2(TRACE_REALLY, "stringout_underlaid_field(\"%s\", d)\n", str, fwidth);

    clear_underlay(fwidth);

    return(stringout_field(str, fwidth));
}


/* -------------- end of RISC OS specific drawing routines --------------- */

#endif /* RISCOS */


/****************************************************************
*                                                               *
* print a string (LJ) in a field, clearing out bg if necessary  *
* graphics cursor advanced to next text cell                    *
* NB. overlong strings get poked, so there ...                  *
*                                                               *
****************************************************************/

static intl
stringout_field(char *str, intl fwidth)
{
    intl len = strlen(str);
    intl drawn;
    char ch;

    vtracef2(TRACE_REALLY, "stringout_field(%d, \"%s\")\n", fwidth, str);

    if(len > fwidth)
        {
        ch = str[fwidth];
        str[fwidth] = '\0';
        drawn = stringout(str);
        str[fwidth] = ch;
        }
    else
        drawn = stringout(str);

    #if RISCOS
    /* move to end of field */
    if(fwidth - drawn > 0)
        riscos_movespaces(fwidth - drawn);
    #elif MS || ARTHUR
    /* pad to end of field with spaces */
    ospca(fwidth - drawn);
    #endif

    return(drawn);
}


#if MS

/**********************************************************************
*                                                                     *
* outputting individual characters to screen is rather slow so buffer *
* them up and output the lot. purge buffer on any control character   *
*                                                                     *
**********************************************************************/

static uchar strout_buff[LIN_BUFSIZ];
static intl  buff_idx = 0;

static void
purgebuffer(void)
{
    if(buff_idx == 0)
        return;

    strout_buff[buff_idx] = '\0';
    stringout(strout_buff);
    buff_idx = 0;
}

#define addtobuff(ch)  strout_buff[buff_idx++] = ch
static intl (addtobuff)(intl ch);


#elif ARTHUR || RISCOS

#define purgebuffer()
extern void (purgebuffer)(void);

#endif  /* MS */


#if !RISCOS

/********************************
*                               *
*  reset vertvec lengths to 0   *
*                               *
********************************/

static void
reinit_vertvec(void)
{
    SCRROW *rptr        = vertvec();
    SCRROW *last_rptr   = rptr + maxnrow;

    tracef0("reinit_vertvec()\n");

    while(rptr <= last_rptr)
        rptr++->length = 0;
}

#endif


/********************************************
*                                           *
* set up row information done               *
* whenever screen height set:               *
* init_screen, DSfunc, window size change   *
*                                           *
********************************************/

extern void
reinit_rows(void)
{
    /* max number of rows possible in this mode */
    maxnrow = (paghyt + 1) - BORDERLINE;
    tracef1("maxnrow        := %d\n", maxnrow);

    /* number of rows we can actually use at the moment */
    rows_available = maxnrow - borderheight;    /* depends on borbit */
    tracef1("rows_available := %d\n", rows_available);

    #if RISCOS
    /* sod this; it seems a little unidirectional to me (SKS)! */
    #elif ARTHUR
    /* can't change mode whilst running */
    #elif MS
    /* if we had more fixed rows than there are total rows, destroy fixes */
    if(n_rowfixes >= rows_available-1)
        n_rowfixes = 0;
    #endif
}


/************************************
*                                   *
* try to set a new screen height    *
* if it works, variables updated    *
*                                   *
* --out--                           *
*   FALSE -> no vertvec, sorry...   *
*                                   *
************************************/

extern BOOL
new_window_height(intl height)
{
    intl nrows;
    intl old_maximum_rows;
    mhandle mh;
    SCRROW *rptr, *last_rptr;

    tracef1("new_window_height(%d)\n", height);

    /* calculate no. of rows needed: h=25 -> 22 rows if borders off,
     * (and headline present) +1 for terminating row -> nrows=23
    */
    /* eg h=7, b=2 -> nrows=6 (7+1-2) */
    #if ARTHUR || RISCOS
    nrows = (height + 1) - BORDERLINE;
    #elif MS
    /* biggest EVER possible with this card */
    nrows = (Vcs.Vheightmax + 1) - BORDERLINE;
    #endif

    mh = vertvec_mh;

    if((nrows != maximum_rows)  &&  ((mh = realloc_handle_using_cache(mh, (word32) sizeof(SCRROW) * nrows)) > 0))
        {
        /* only (re)set variables if realloc ok */
        old_maximum_rows = maximum_rows;

        vertvec_mh = mh;

        maximum_rows = nrows;
        tracef1("maximum_rows   := %d\n", maximum_rows);

        paghyt = height - 1;
        tracef1("paghyt         := %d\n", paghyt);

        reinit_rows();          /* derives things from paghyt */

        if(nrows > old_maximum_rows)
            {
            rptr        = vertvec();
            last_rptr   = rptr + nrows;

            rptr += old_maximum_rows;

            do  {
                rptr->rowno     = 0;
                rptr->page      = 0;
                #if !RISCOS
                rptr->length    = 0;
                #endif
                rptr++->flags   = LAST;
                }
            while(rptr < last_rptr);
            }

        out_rebuildvert = TRUE;     /* shrinking or growing */
        }
    else
        tracef0("gosh - no row change: why were we called?\n");

    return((BOOL) vertvec_mh);  /* 0 -> buggered */
}


/************************************
*                                   *
* try to set a new screen width     *
* if it works, variables updated    *
*                                   *
* --out--                           *
*   FALSE -> no horzvec, sorry...   *
*                                   *
************************************/

extern BOOL
new_window_width(intl width)
{
    /* number of columns needed: +1 for terminating column */
    intl ncols = width + 1;
    intl old_maximum_cols;
    mhandle mh;
    SCRCOL *cptr, *last_cptr;

    tracef1("new_window_width(%d)\n", width);

    mh = horzvec_mh;

    if((ncols != maximum_cols)  &&  ((mh = realloc_handle_using_cache(mh, (word32) sizeof(SCRCOL) * ncols)) > 0))
        {
        /* only (re)set variables if realloc ok */
        old_maximum_cols = maximum_cols;

        horzvec_mh = mh;

        maximum_cols = ncols;
        tracef1("maximum_cols   := %d\n", maximum_cols);

        #if RISCOS
        pagwid_plus1 = width;
        #else
        pagwid_plus1 = width - 1;   /* don't print in last text col */
        #endif
        tracef1("pagwid_plus1   := %d\n", pagwid_plus1);

        pagwid = pagwid_plus1 - 1;

        maxncol = pagwid_plus1;
        tracef1("maxncol        := %d\n", maxncol);

        cols_available = maxncol - borderwidth;
        tracef1("cols_available := %d\n", cols_available);

        if(ncols > old_maximum_cols)
            {
            cptr        = horzvec();
            last_cptr   = cptr + ncols;

            cptr += old_maximum_cols;

            do  {
                cptr->colno     = 0;
                cptr++->flags   = LAST;
                }
            while(cptr < last_cptr);
            }

        /* columns need much more redrawing than rows on resize */
        out_rebuildhorz = TRUE;
        }
    else
        tracef0("gosh - no col change: why were we called?\n");

    return((BOOL) horzvec_mh);  /* NULL -> buggered */
}


/****************************
*                           *
*  initialise screen data   *
*                           *
****************************/

extern BOOL
init_screen(void)
{
    BOOL ok;

    tracef0("init_screen()\n");

    ok =    new_window_height(paghyt)   &&
            new_window_width(pagwid);

    if(TRACE  &&  !ok)
        tracef0("***** init_screen() failed *****\n");

    return(ok);
}


/****************************
*                           *
*  initialise screen data   *
*                           *
****************************/

extern BOOL
screen_initialise(void)
{
    BOOL ok;
    intl ph, pw;

    tracef0("screen_initialise()\n");

    #if RISCOS
    new_grid_state();       /* set charvspace/vrubout - doesn't redraw */

    ph = windowheight();
    pw = windowwidth();
    #elif MS || ARTHUR
    ph = scrnheight()+1;
    pw = scrnwidth()+1;
    #endif

    /* even on RISC OS, may have changed from burned-in grid state */
    ok =    new_window_height(ph)   &&
            new_window_width(pw);

    if(!ok)
        {
        tracef0("***** screen_initialise() failed *****\n");
        screen_finalise();
        }

    return(ok);
}


/************************
*                       *
* finalise screen data  *
*                       *
************************/

extern void
screen_finalise(void)
{
    list_disposehandle(&horzvec_mh);

    list_disposehandle(&vertvec_mh);
}


/**********************************
*                                 *
* draw screen appropriately after *
* something has happened          *
*                                 *
**********************************/

extern void
draw_screen(void)
{
    BOOL old_movement;

    vtracef0(TRACE_DRAW, "\n*** draw_screen()\n");

    oldcol = curcol;
    oldrow = currow;

    xf_interrupted = FALSE; /* always reset, gets set again if needed */

    if(xf_draweverything)
        {
        new_screen();   /* calls draw_screen() again itself */
        return;
        }

    #if MS || ARTHUR
    /* contrary to RJM's assertion, I believe things DO expect colours */
    setcolour(FORE, BACK);
    #endif

    if(row_number(0) >= numrow)
        cenrow(numrow-1);

    if(out_rebuildvert)
        filvert(fstnrx(), currow, CALL_FIXPAGE);    /* first non-fixed row */

    if(out_rebuildhorz)
        filhorz(fstncx(), curcol);

    old_movement = movement;

    if((int) movement)
        {
        xf_drawslotcoordinates = TRUE;

        if(!dont_update_lescrl)
            lescrl = 0;
        else
            dont_update_lescrl = FALSE;

        if(movement & ABSOLUTE)
            chkmov();
        else
            {
            switch((int) movement)
                {
                case CURSOR_UP:
                    curup();
                    break;

                case CURSOR_DOWN:
                    curdown();
                    break;

                case CURSOR_PREV_COL:
                    prevcol();
                    break;

                case CURSOR_NEXT_COL:
                    nextcol();
                    break;

                case CURSOR_SUP:
                    cursup();
                    break;

                case CURSOR_SDOWN:
                    cursdown();
                    break;

                default:
                    break;
                }
            }

        movement = 0;
        }

    curosc();

    #if RISCOS
    /* re-load screen vector if we change rows with draw files about */
    if(draw_file_refs  &&  (pict_currow != currow))
        {
        tracef0("[draw_screen calling filvert: pict_currow != currow]\n");
        filvert(fstnrx(), currow, DONT_CALL_FIXPAGE);
        }

    if(pict_on_screen != pict_was_on_screen)
        {
        tracef0("*** forcing out_screen because pict_on_screen changed\n");
        pict_was_on_screen = pict_on_screen;
        out_screen = TRUE;
        }
    #endif

    check_output_current_slot(old_movement);

    if(xf_drawslotcoordinates  ||  out_screen)
        draw_slot_coordinates();

    if(xf_flush)
        {
        xf_flush = FALSE;
        clearkeyboardbuffer();
        }

    #if RISCOS
    (void) draw_altered_state();        /* sets extent, scroll offsets */

    /* send window to the front after adjusting scroll offsets! */
    if(xf_frontmainwindow)
        riscos_frontmainwindow(FALSE);
    #endif

    #if !defined(HEADLINE_OFF)
    if(xf_drawmenuheadline)
        display_heading(-1);
    #endif

    if(xf_drawcolumnheadings)
        draw_column_headings();

    if(out_screen)
        draw_screen_below(0);
    elif(out_below)
        draw_screen_below(rowtoend);

    if(out_rowout)
        draw_row(adjust_rowout(rowout));

    if(out_rowout1)
        draw_row(adjust_rowout(rowout1));

    #if defined(SHOW_CURRENT_ROW)
    if(out_rowborout)
        draw_row_border(adjust_rowborout(rowborout));

    if(out_rowborout1)
        draw_row_border(adjust_rowborout(rowborout1));
    #endif

    if(xf_drawsome)
        draw_altered_slots();

    draw_slot_in_buffer();      /* if there is one */

    position_cursor();

    out_currslot = FALSE;

    sb_show_if_fastdraw();
}


#define BIG_TX ((3 << 13) / charwidth)
#define BIG_TY ((3 << 13) / charheight)

extern void
new_screen(void)
{
    vtracef0(TRACE_DRAW, "new_screen()\n");

    #if !RISCOS
    reinit_vertvec();
    #endif

    /* clear out entire window */
    #if RISCOS
    please_clear_textarea(-BIG_TX, BIG_TY, BIG_TX, BIG_TY);
    #elif MS || ARTHUR
    clearscreen();                      /* NB. sets xf_draweverything */
    #endif

    xf_draweverything = FALSE;          /* unset this redraw flag */

    #if !defined(HEADLINE_OFF)
    xf_drawmenuheadline =
    #endif
    xf_drawslotcoordinates = xf_drawcolumnheadings =
    out_screen = out_rebuildhorz = out_rebuildvert = out_currslot = TRUE;

    draw_screen();
}


#if RISCOS

/****************************************************************************
*                                                                           *
*                           display draw files                              *
*                                                                           *
****************************************************************************/

static void
really_draw_picture(drawfep dfep, drawfrp dfrp, intl x0, intl y0, intl x1, intl y1)
{
    BOOL flag;
    draw_error err;
    draw_redrawstr r;

    vtracef0(TRACE_REALLY, "really_draw_picture()\n");

    if(!set_graphics_window_from_textarea(x0, y0, x1, y1, TRUE /*SKS 09.10.2021*/))
        return;

    /* drawing may shuffle core by use of flex - which now upcalls */
    /* list_unlockpools(); */

    setbgcolour(BACK);

    #if !defined(DONT_CLEAR_DRAWFILES)
    clear_thistextarea();
    #endif

    /* origin of diagram positioned at r.box.x0,r.box.y1 (abs) */
    r.box.x0 = gcoord_x(x0);
    r.box.y1 = gcoord_y(y1) - dfrp->ysize_os - dy;
    r.scx    = 0;
    r.scy    = 0;
    r.g      = *((draw_box *) &graphics_window);

    tracef6("draw_render_diag(&%p, %d; box %d, %d, %d, %d; ",
        dfep->diag.data, dfep->diag.length,
        r.box.x0, r.box.y0, r.box.x1, r.box.y1);
    tracef5("gw %d, %d, %d, %d; scale %g\n",
        r.g.x0, r.g.y0, r.g.x1, r.g.y1, dfrp->xfactor);
    flag = draw_render_diag((draw_diag *) &dfep->diag, &r, dfrp->xfactor, &err);

    if(!flag)
        {
        if(err.type == DrawOSError)
            tracef2("Draw: os error &%8.8X, \"%s\"\n",
                            err.err.os.errnum, err.err.os.errmess);
        else
            tracef2("Draw: draw error &%8.8X, \"%s\"\n",
                            err.err.draw.code, err.err.draw.location);

        /* update structure before dealloc moves it */
        dfep->error = ERR_BADDRAWFILE;
        list_disposehandle(&dfep->memoryh);
        }

    restore_graphics_window();

    /* Draw rendering destroys current graphics & font colour settings */
    killcolourcache();
    setcolour(FORE, BACK);
}


static intl
find_coff(colt tcol)
{
    SCRCOL *cptr = horzvec();
    intl coff;

    if(tcol < cptr->colno)
        return(-1);

    coff = 0;

    while(!(cptr->flags & LAST))
        {
        if(tcol == cptr->colno)
            return(coff);

        coff++;
        cptr++;
        }

    return(-2);
}


static void
maybe_draw_pictures(void)
{
    intl coff, roff;
    colt tcol;
    rowt trow;
    drawfep dfep;
    drawfrp dfrp;
    intl x0, x1, y0, y1;
    SCRCOL *cptr;
    SCRROW *rptr;

    vtracef0(TRACE_MAYBE, "maybe_draw_pictures(): ");

    if(pict_on_screen)
        {
        rptr = vertvec();

        for(roff = 0; roff < rowsonscreen; roff++, rptr++)
            {
            if(rptr->flags & PICT)
                {
                trow = rptr->rowno;

                cptr = horzvec();

                for(coff = 0; !(cptr->flags & LAST); coff++, cptr++)
                    {
                    tcol = cptr->colno;

                    if(draw_find_file(current_document_handle(), tcol, trow, &dfep, &dfrp))
                        {
                        tracef2("found picture at col %d, row %d\n", tcol, trow);

                        x0 = calcad(find_coff(dfrp->col));
                        x1 = x0 + tsize_x(dfrp->xsize_os);
                        y1 = calrad(roff) - 1;      /* picture hangs from top */
                        y0 = y1 + tsize_y(dfrp->ysize_os);
                        y0 = min(y0, paghyt);

                        tracef6("xsize = %d, ysize = %d; (%d %d %d %d)\n",
                                tsize_x(dfrp->xsize_os), tsize_y(dfrp->ysize_os), x0, y0, x1, y1);

                        if(textobjectintersects(x0, y0, x1, y1))
                            {
                            really_draw_picture(dfep, dfrp, x0, y0, x1, y1);

                            /* reload as drawing may shuffle core */
                            cptr = horzvec_entry(coff);
                            rptr = vertvec_entry(roff);
                            }
                        }
                    }
                }
            }
        }
    else
        tracef0("pict_on_screen = FALSE\n");
}

#endif  /* RISCOS */


#if !defined(HEADLINE_OFF)

/****************************************************************************
*                                                                           *
*               display menu headings along the top line                    *
*                                                                           *
****************************************************************************/

extern void
display_heading(intl idx)
{
    xf_drawmenuheadline = FALSE;        /* unset my redraw flag */

    vtracef0(TRACE_DRAW, "\n*** draw_menu_headline()\n");


#if RISCOS

    please_redraw_textarea( HEADLINE_X0, HEADLINE_Y0,
                            HEADLINE_X1, HEADLINE_Y1);
}


static void
maybe_draw_menu_headline(void)
{
    vtracef0(TRACE_MAYBE, "maybe_draw_menu_headline()\n");

    /* required to redraw left & top slop too */

    if(textobjectintersects(HEADLINE_X0-1,  HEADLINE_Y0,
                            HEADLINE_X1,    HEADLINE_Y1-1))
        really_draw_menu_headline();
}


static void
really_draw_menu_headline(void)
{
    vtracef0(TRACE_REALLY, "really_draw_menuheadline()\n");

#endif /* RISCOS */


    {
    intl apnamelength = strlen(applicationname);
    intl i;
    intl xpos = HEADLINE_X0;
    char *ptr;

    setcolour(MENU_FORE, MENU_BACK);

    #if RISCOS
    clear_thistextarea();
    #endif

    at(xpos, HEADLINE_Y0);

    /* display each menu heading */

    for(i = 0; i < head_size; i++)
        if(headline[i].installed)
            {
            char *thisname  = (char *) headline[i].name;
            BOOL thisone = FALSE;

            if(i == idx)
                {
                thisone = TRUE;
                this_heading_length = strlen((const char *) thisname);
                if(*thisname != *alt_array)
                    {
                    alt_array[0] = *thisname;
                    alt_array[1] = '\0';
                    }
                }

            setcolour(MENU_HOT, MENU_BACK);
            if(thisone)
                at(xpos+1, HEADLINE_Y0);
            else
                wrch(SPACE);
            wrch(*thisname);

            setcolour(MENU_FORE, MENU_BACK);
            xpos += 3 + stringout(thisname + 1);
            if(thisone)
                at(xpos, HEADLINE_Y0);
            else
                wrch(SPACE);
            }


    #if defined(SPELL_OFF)
    /* put up a dot for each saved slot position, no room if SPELL */

    for(i = 0; i < saved_index; i++, xpos++)
        wrch('.');
    #endif


    /* display the subcommand currently being entered */

    ospca(ALTARRAY_X0 - xpos);
    xpos = ALTARRAY_X0;
    setcolour(MENU_HOT, MENU_BACK);
    xpos += stringout(alt_array);
    ospca(ALTARRAY_X0 + 5 - xpos);
    xpos = ALTARRAY_X0 + 5;


    /* display the current filename */

    if(idx == -2)
        ptr = "Pause..";
    elif(!str_isblank(ptr = fndfcl()))
        {
        intl spacefree;
        intl len = strlen(ptr);
        char array[80];

        spacefree = pagwid - appnamelength - xpos;
        if(len > spacefree)
            {
            strcpy(array+2, ptr+len-spacefree+2);
            ptr = array;
            ptr[0] = ptr[1] = '.';
            }
        }
    else
        ptr = "F1=Help";

    setcolour(MENU_FORE, MENU_BACK);
    xpos += stringout(ptr);


    /* display our name (right justified) */

    ospca(pagwid_plus1 - appnamelength - xpos);
    at(pagwid_plus1 - appnamelength, HEADLINE_Y0);
    wrch(SPACE);
    stringout((uchar *) applicationname);

    
    /* restore colours */
    setcolour(FORE, BACK);

    sb_show_if_fastdraw();
    }
}

#endif  /* HEADLINE_OFF */


/****************************************************************************
*                                                                           *
*               display the current slot coordinates                        *
*                                                                           *
****************************************************************************/

static void
draw_slot_coordinates(void)
{
    char array[LIN_BUFSIZ];

    xf_drawslotcoordinates = FALSE;         /* unset my redraw flag */

    vtracef0(TRACE_DRAW, "\n*** draw_slot_coordinates()\n");

    writeref(array, 0, curcol, currow);     /* always current doc */

#if RISCOS

    array[slotcoordinates_size-1] = '\0';

    if(strcmp(slotcoordinates, array))
        {
        /* redraw iff changed */

        strcpy(slotcoordinates, array);

        thisarea.x0 = SLOTCOORDS_X0;
        thisarea.y0 = SLOTCOORDS_Y0;
        thisarea.x1 = SLOTCOORDS_X1;
        thisarea.y1 = SLOTCOORDS_Y1;

        please_update_thistextarea(forced_draw_slot_coordinates);
        }
    else
        vtracef0(TRACE_DRAW, "slot coordinates not changed\n");

#else

    really_draw_slot_coordinates(array);

#endif
}


#if RISCOS

static void
forced_draw_slot_coordinates(riscos_redrawstr *r)
{
    IGNOREPARM(r);

    vtracef0(TRACE_REALLY, "forced_draw_slot_coordinates()\n");

    setbgcolour(BACK);

    really_draw_slot_coordinates(slotcoordinates);
}


static void
maybe_draw_slot_coordinates(void)
{
    vtracef0(TRACE_MAYBE, "maybe_draw_slot_coordinates()\n");

    /* required to redraw left slop too */

    if(textobjectintersects(SLOTCOORDS_X0-1, SLOTCOORDS_Y0,
                            SLOTCOORDS_X1,   SLOTCOORDS_Y1))
        {
        really_draw_slot_coordinates(slotcoordinates);
        setfgcolour(FORE);
        }
}

#endif  /* RISCOS */


static void
really_draw_slot_coordinates(char *str)
{
    vtracef1(TRACE_REALLY, "really_draw_slot_coordinates(\"%s\")\n", str);

    #if RISCOS
    /* required to redraw left slop too */
    clear_thistextarea();
    #endif

    at(SLOTCOORDS_X0, SLOTCOORDS_Y0);

    setfgcolour(CURBORDERC);

    #if RISCOS
    stringout_field(str, SLOTCOORDS_X1 - SLOTCOORDS_X0);

    #elif MS
    stringout_underlaid_field(str, SLOTCOORDS_X1 - SLOTCOORDS_X0);

    setfgcolour(FORE);
    #endif
}


/****************************************************************************
*                                                                           *
*           display contents of current numeric slot on contents line       *
*                                                                           *
****************************************************************************/

static void
draw_contents_of_numslot(void)
{
    char  array[LIN_BUFSIZ];
    slotp tslot;
    intl length, fwidth;

    #if RISCOS
    /* We can cope with this as we use a dbox for search */
    #elif MS || ARTHUR
    if(in_search)
        return;
    #endif

    tslot = travel_here();

    if(!tslot  ||  (tslot->type == SL_TEXT)  ||  (tslot->type == SL_PAGE))
        *array = '\0';
    else
        prccon(array, tslot);   /* decompile tslot into array */

    length = strlen(array);

#if RISCOS
    numericslotcontents_length = fwidth = length;

    if(strcmp(numericslotcontents, array))
        {
        /* redraw iff changed */
        strcpy(numericslotcontents, array);

        thisarea.x0 = NUMERICSLOT_X0;
        thisarea.y0 = NUMERICSLOT_Y0;
        thisarea.x1 = NUMERICSLOT_X1;
        thisarea.y1 = NUMERICSLOT_Y1;

        please_update_thistextarea(forced_draw_contents_of_numslot);
        }
    else
        vtracef0(TRACE_DRAW, "contents of numeric slot not changed\n");

#elif MS || ARTHUR

    really_draw_contents_of_numslot(array);

#endif
}


#if RISCOS

static void
forced_draw_contents_of_numslot(riscos_redrawstr *r)
{
    IGNOREPARM(r);

    vtracef0(TRACE_REALLY, "forced_draw_contents_of_numslot()\n");

    setcolour(FORE, BACK);

    really_draw_contents_of_numslot();
}


static void
maybe_draw_contents_of_numslot(void)
{
    vtracef0(TRACE_MAYBE, "maybe_draw_contents_of_numslot()\n");

    if(textobjectintersects(NUMERICSLOT_X0, NUMERICSLOT_Y0,
                            NUMERICSLOT_X1, NUMERICSLOT_Y1))
        really_draw_contents_of_numslot();
}

#endif  /* RISCOS */


#if RISCOS
static void
really_draw_contents_of_numslot(void)
#else
static void
really_draw_contents_of_numslot(char *numericslotcontents)
#endif
{
    vtracef0(TRACE_REALLY, "really_draw_contents_of_numslot()\n");

    #if RISCOS
    clear_thistextarea();
    #endif

    at(NUMERICSLOT_X0, NUMERICSLOT_Y0);

    stringout_field(numericslotcontents, NUMERICSLOT_X1 - NUMERICSLOT_X0);
}


/****************************************************************************
*                                                                           *
* display slot_in_buffer if there is one. Either in place or in expression  *
*                                                                           *
****************************************************************************/

static void
draw_slot_in_buffer(void)
{
    intl fwidth_ch;
    #if RISCOS
    intl fwidth_mp, swidth_mp;
    char paint_str[PAINT_STRSIZ];
    #endif

    if(slot_in_buffer)
        {
        if(!output_buffer)
            {
            tracef2("[draw_slot_in_buffer: lescrl %d, old_lescrl %d]\n", lescrl, old_lescroll);

            /* if scroll position is different, we must output */
            if(lescrl != old_lescroll)
                output_buffer = TRUE;
            else
                {
                /* if scroll position will be different, we must output */
                fwidth_ch = xf_inexpression
                                    ? pagwid_plus1
                                    : limited_fwidth_of_slot_in_buffer();

                if(--fwidth_ch < 0)     /* rh scroll margin */
                    fwidth_ch = 0;

                tracef2("[draw_slot_in_buffer: fwidth_ch %d, lecpos %d]\n", fwidth_ch, lecpos);

                #if RISCOS
                if(riscos_fonts  &&  !xf_inexpression  &&  fwidth_ch)
                    {
                    /* fonty cal_lescrl a little more complex */

                    tracef0("[draw_slot_in_buffer: fonty_cal_lescrl(fwidth)]\n");

                    /* is the caret off the left of the field? (scroll margin one char) */
                    if(lecpos <= lescrl)
                        {
                        if(lescrl)
                            /* off left - will need to centre caret in field */
                            output_buffer = TRUE;
                        }
                    else
                        {
                        /* is the caret off the right of the field? (scroll margin one char) */
                        fwidth_mp = ch_to_mp(fwidth_ch);

                        expand_current_slot_in_fonts(paint_str, TRUE, NULL);
                        swidth_mp = font_width(paint_str);

                        tracef2("[draw_slot_in_buffer: fwidth_mp %d, swidth_mp %d]\n", fwidth_mp, swidth_mp);

                        if(swidth_mp > fwidth_mp)
                            /* off right - will need to right justify */
                            output_buffer = TRUE;
                        }
                    }
                else
                #endif
                    {
                    tracef1("[draw_slot_in_buffer: cal_lescrl(fwidth) %d]\n", cal_lescrl(fwidth_ch));

                    if(cal_lescrl(fwidth_ch) != lescrl)
                        output_buffer = TRUE;
                    }
                }
            }
    
        tracef1("[draw_slot_in_buffer: output_buffer = %s]\n", trace_boolstring(output_buffer));

        if(output_buffer)
            {
            if(xf_inexpression)
                draw_editing_line();
            else
                draw_slot_in_buffer_in_place();

            old_lescroll = lescrl;
            old_lecpos = lecpos;    /* for movement checks */
            output_buffer = FALSE;
            }
        }
}


/************************************
*                                   *
* slot being edited on editing line *
*                                   *
************************************/

extern void
clear_editing_line(void)
{
    vtracef0(TRACE_DRAW, "\n*** clear_editing_line()\n");

    #if RISCOS
    /* required to redraw left slop too */

    thisarea.x0 = -1;
    thisarea.y0 = EDTLIN_Y0;
    thisarea.x1 = EDTLIN_X1;
    thisarea.y1 = EDTLIN_Y1;

    please_update_textarea(forced_draw_editing_line,
                           -1, EDTLIN_Y0, EDTLIN_X1, EDTLIN_Y1);
    #elif MS || ARTHUR
    setcolour(FORE, BACK);
    at(EDTLIN_X0, EDTLIN_Y0);
    eraeol();
    #endif
}


static void
draw_editing_line(void)
{
    coord x0 = EDTLIN_X0;
    intl dead_text;

    vtracef0(TRACE_DRAW, "\n*** draw_editing_line()\n");

    adjust_lescrl(0, RHS_X);

    dead_text = cal_dead_text();

    x0 += dead_text;

    #if RISCOS

    thisarea.x0 = x0;
    thisarea.y0 = EDTLIN_Y0;
    thisarea.x1 = EDTLIN_X1;
    thisarea.y1 = EDTLIN_Y1;

    please_update_thistextarea(forced_draw_editing_line);

    #elif MS || ARTHUR

    dspfld(x0, EDTLIN_Y0, EDTLIN_X1 - x0, 0, dead_text);

    #endif
}


#if RISCOS

/* draw contents of editing_line; maybe editing slot, maybe empty */

static void
forced_draw_editing_line(riscos_redrawstr *r)
{
    IGNOREPARM(r);

    vtracef0(TRACE_REALLY, "forced_draw_editing_line()\n");

    setcolour(FORE, BACK);

    really_draw_editing_line();
}


static void
maybe_draw_editing_line(void)
{
    vtracef0(TRACE_MAYBE, "maybe_draw_editing_line()\n");

    /* required to redraw left slop too */

    if(textobjectintersects(-1, EDTLIN_Y0, EDTLIN_X1, EDTLIN_Y1))
        really_draw_editing_line();
}


static void
really_draw_editing_line(void)
{
    intl fwidth;

    vtracef0(TRACE_REALLY, "really_draw_editing_line()\n");

    if(xf_inexpression)
        {
        /* required to redraw left slop too */
        clear_textarea(-1, EDTLIN_Y0, EDTLIN_X0, EDTLIN_Y1, FALSE);

        /* dspfld does its own background clearing as appropriate */
        fwidth = EDTLIN_X1 - EDTLIN_X0;
        fwidth -= dspfld(EDTLIN_X0, EDTLIN_Y0, EDTLIN_X1 - EDTLIN_X0);

        if(fwidth > 0)
            ospca(fwidth);
        }
    else
        clear_thistextarea();

    if(!borbit  &&  grid_on)
        {
        /* draw grid bar just below editing line */
        at(borderwidth, EDTLIN_Y0);
        draw_grid_hbar(hbar_length);
        }
}

#endif  /* RISCOS */


/********************************
*                               *
* slot is being edited in place *
*                               *
********************************/

static void
draw_slot_in_buffer_in_place(void)
{
    intl x0 = calcad(curcoloffset);
    intl x1;
    intl y0 = calrad(currowoffset);
    intl y1 = y0 - 1;
    intl fwidth = colwidth(curcol);
    intl overlap = chkolp(travel_here(), curcol, currow);
    intl dead_text;

    vtracef0(TRACE_DRAW, "\n*** draw_slot_in_buffer_in_place()\n");

    adjust_lescrl(x0, overlap);

    fwidth = max(fwidth, overlap);

    x1 = x0 + fwidth;

    dead_text = cal_dead_text();

    x0 += dead_text;

    #if RISCOS

    riscos_removecaret();

    please_redraw_textarea(x0, y0, x1, y1);

    #if 1
    if(main_window == caret_window)
        /* restore caret in a wee while if we need it */
        xf_acquirecaret = TRUE;
    #else
    riscos_restorecaret();
    #endif

    #elif MS || ARTHUR

    (void) setivs(curcol, currow);

    dspfld(x0, y0, fwidth, 0, dead_text);

    #endif  /* RISCOS */
}


/****************************************************************************
*                                                                           *
*                       display the column headings                         *
*                                                                           *
****************************************************************************/

static void
draw_column_headings(void)
{
    xf_drawcolumnheadings = FALSE;      /* unset my redraw flag */

    if(!borbit)
        return;

    vtracef0(TRACE_DRAW, "\n*** draw_column_headings()\n");


#if RISCOS

    please_redraw_textarea( COLUMNHEAD_X0,  COLUMNHEAD_Y0,
                            COLUMNHEAD_X1,  COLUMNHEAD_Y1);
}


static void
maybe_draw_column_headings(void)
{
    vtracef0(TRACE_MAYBE, "maybe_draw_column_headings()\n");

    /* required to redraw left slop too */

    if(!borbit)
        {
        if(textobjectintersects(COLUMNHEAD_X0-1,    COLUMNHEAD_Y0,
                                COLUMNHEAD_X0,      COLUMNHEAD_Y1))
            clear_thistextarea();

        return;
        }

    if(textobjectintersects(COLUMNHEAD_X0-1,    COLUMNHEAD_Y0,
                            COLUMNHEAD_X1,      COLUMNHEAD_Y1))
        really_draw_column_headings();
}


static void
really_draw_column_headings(void)
{
    vtracef0(TRACE_REALLY, "really_draw_column_headings()\n");

#endif  /* RISCOS */


    {
    SCRCOL *cptr;
    intl coff;
    intl fieldleft = COLUMNHEAD_X1 - borderwidth;
    colt colno;
    intl cwid, length, arrowpos, len;
    char colnoarray[20];
    uchar outch;

    #if RISCOS
    clear_thistextarea();
    #endif

    #if RISCOS
    at(COLUMNHEAD_X0 + borderwidth, COLUMNHEAD_Y0); /* lh gap cleared */
    #elif MS || ARTHUR
    at(COLUMNHEAD_X0, COLUMNHEAD_Y0);
    ospca(borderwidth);                             /* clear out lh gap */
    #endif

    coff = 0;

    for(; !((cptr = horzvec_entry(coff))->flags & LAST)  &&  (fieldleft > 0); ++coff)
        {
        colno = cptr->colno;
        cwid = colwidth(colno);

        if(colno == curcol)
            {
            setfgcolour(CURBORDERC);
            #if ARTHUR || RISCOS
            outch = COLUMN_DOTS;
            wrch_definefunny(outch);    /* as we're going to wrchrep it */
            #elif MS
            outch = BORDERFIXCH;
            #endif
            }
        else
            {
            setfgcolour(BORDERC);
            outch = incolfixes(colno) ? BORDERFIXCH : BORDERCH;
            }

        length = writecol(colnoarray, colno);

        if(length <= cwid)
            {
            len = cwid - length;

            if(len < fieldleft)
                {
                wrchrep(outch, len);

                #if ARTHUR || RISCOS
                if(outch == COLUMN_DOTS)
                    wrch_undefinefunny(outch);
                #endif

                fieldleft -= len;

                if(fieldleft < length)
                    {
                    colnoarray[fieldleft] = '\0';
                    fieldleft = 0;
                    }
                else
                    fieldleft -= length;

                stringout(colnoarray);
                }
            else
                {
                wrchrep(outch, fieldleft);
                fieldleft = 0;
                }
            }
        else
            {
            if(fieldleft < cwid)
                {
                colnoarray[fieldleft] = '\0';
                fieldleft = 0;
                }
            else
                fieldleft -= cwid;

            stringout(colnoarray + length - cwid);
            }
        }

    #if MS || ARTHUR
    ospca(fieldleft);                       /* pad to rhs of screen */
    #endif


    /* restore fg colour & draw the down arrow */
    setfgcolour(FORE);

    arrowpos = calcad(curcoloffset) + colwrapwidth(curcol) - 2;
    if(arrowpos <= RHS_X)
        {
        at(arrowpos, COLUMNHEAD_Y0);
        wrch_definefunny(DOWN_ARROW);
        #if RISCOS
        riscos_printchar(DOWN_ARROW);   /* ALWAYS overprinting */
        #elif MS || ARTHUR
        wrch_funny(DOWN_ARROW);
        #endif
        wrch_undefinefunny(DOWN_ARROW);
        }

    #if RISCOS
    if(grid_on)
        {
        /* draw grid bar just below column headings */
        at(borderwidth, COLUMNHEAD_Y0);
        draw_grid_hbar(hbar_length);
        }
    #endif
    }
}


/****************************************************************************
*                                                                           *
*           display rows (numbers and contents) across screen               *
*                                                                           *
****************************************************************************/

#if RISCOS
#define os_if_fonts(nchars) (riscos_fonts ? ch_to_os(nchars) : (nchars))
#else
#define os_if_fonts(nchars) (nchars)

#define at_fonts(x, y)      at(x, y)
#define ospca_fonts(n)      ospca(n)
#endif

/***********************************
*                                  *
* get screen redrawn from row roff *
*                                  *
***********************************/

#if RISCOS
static intl allow_interrupt_below = INT_MAX;
#endif

static void
draw_screen_below(intl roff)
{
    #if !RISCOS
    intl j;
    #endif

    out_screen = out_below = FALSE; /* reset my redraw flags */

    vtracef1(TRACE_DRAW, "\n*** draw_screen_below(%d)\n", roff);

    #if RISCOS

    if(roff < rowsonscreen)
        {
        allow_interrupt_below = roff;
        please_redraw_textarea(0,       calrad(rowsonscreen - 1),
                               RHS_X,   calrad(roff) - 1);
        allow_interrupt_below = INT_MAX;
        }

    if(xf_interrupted)
        return;

    #elif MS

    for(j = roff; j < rowsonscreen; j++)
        {
        draw_row(j);

        if(keyinbuffer()  ||  (autorepeat  &&  depressed_shift()))
            {
            vtracef1(TRACE_DRAW, "draw_screen_below interrupted: j = %d\n", j);
            rowtoend = j + 1;
            xf_interrupted = out_below = TRUE;
            return;
            }
        }

    #endif  /* RISCOS */

    draw_empty_bottom_of_screen();

    if(roff == 0)
        xf_drawsome = FALSE;

    rowtoend = rows_available;
}


#if RISCOS

/* loop down all 'visible' rows, stopping after the bottom partial row
 * (highest roff) in the clipped area has been drawn.
*/

static void
maybe_draw_rows(void)
{
    intl roff;
    BOOL morebelow;

    for(roff = 0; roff < rowsonscreen; roff++)
        {
        morebelow = maybe_draw_row(roff);

        if(!morebelow)
            break;

        if( (roff >= allow_interrupt_below) &&
            (roff < rowsonscreen)           &&
            keyormouseinbuffer()            )
            {
            vtracef1(TRACE_MAYBE, "mabye_draw_rows interrupted: roff = %d\n", roff);
            rowtoend = roff + 1;
            xf_interrupted = out_below = TRUE;
            return;
            }
        }
}


/* returns FALSE if this line is below the bottom of the cliparea and
 * so no more lines should be considered for redraw.
*/

static BOOL
maybe_draw_row(intl roff)
{
    intl rpos = calrad(roff);

    vtracef2(TRACE_MAYBE, "maybe_draw_row(%d), rpos = %d --- ", roff, rpos);

    /* NB. cliparea.y0 >= rpos > cliparea.y1 is condition we want to draw */

    if(cliparea.y0 < rpos)
        {
        vtracef0(TRACE_MAYBE, "row below clipped area\n");
        return(FALSE);
        }

    if(cliparea.y1 < rpos)
        {
        vtracef0(TRACE_MAYBE, "row in clipped area\n");

        /* limit matched textarea to this line */
        thisarea.y0 = rpos;
        thisarea.y1 = rpos-1;

        maybe_draw_row_border(roff, rpos);
        maybe_draw_row_contents(roff, rpos);
        }
    else
        vtracef0(TRACE_MAYBE, "row above clipped area\n");

    return(TRUE);
}


/* only called if some part of row intersects (y) */

static void
maybe_draw_row_border(intl roff, intl rpos)
{
    /* required to redraw left slop too */

    if(!borbit)
        {
        if(textxintersects(-1, 0))
            {
            clear_thistextarea();

            /* draw first vbar - needs to include hbar */
            at(-1, rpos);
            draw_grid_vbar(TRUE);
            }

        return;
        }

    if(textxintersects(-1, borderwidth))
        really_draw_row_border(roff, rpos);
}


/* the new length that this row should be given (only for update) */
static intl new_row_length;

static void
maybe_draw_row_contents(intl roff, intl rpos)
{
    if(textxintersects(borderwidth, borderwidth + hbar_length))
        {
        new_row_length = really_draw_row_contents(roff, rpos);
        /* only matters if update, not redraw */

        if(grid_on)
            {
            /* draw last vbar */
            at(borderwidth + hbar_length, rpos);
            draw_grid_vbar(FALSE);

            at(borderwidth, rpos);
            draw_grid_hbar(hbar_length);
            }
        }
}

#endif  /* RISCOS */


extern void
draw_row(intl roff)
{
    intl rpos;

    vtracef1(TRACE_DRAW, "\n*** draw_row(%d) --- ", roff);

    if(vertvec_entry(roff)->flags & LAST)
        {
        vtracef0(TRACE_REALLY, "last row\n");
        return;
        }

    if(roff > rowsonscreen)
        {
        vtracef2(TRACE_DRAW, "roff (%d) > rowsonscreen (%d)\n", roff, rowsonscreen);
        return;
        }

    if( out_rowout   &&  (rowout  == roff))
        out_rowout  = FALSE;

    if( out_rowout1  &&  (rowout1 == roff))
        out_rowout1 = FALSE;

    #if defined(SHOW_CURRENT_ROW)
    if(out_rowborout   &&  (rowborout  == roff))
        out_rowborout  = FALSE;

    if(out_rowborout1  &&  (rowborout1 == roff))
        out_rowborout1 = FALSE;
    #endif

    rpos = calrad(roff);


#if RISCOS
    vtracef1(TRACE_DRAW, "rpos = %d\n", rpos);

    /* must get WHOLE line updated as new line may be longer than old */
    please_redraw_textarea(0, rpos, borderwidth + hbar_length, rpos-1);

    /* faint possibility that data may have moved */
    #if FALSE
    /* and now the line has been redrawn over ALL clip rectangles that
     * the Window Manager concocts, we can set its length
    */
    vertvec_entry(roff)->length = new_row_length;
    vtracef1(TRACE_DRAW, "rptr->length := %d\n", vertvec_entry(roff)->length);
    #endif

#elif MS || ARTHUR

    /* much easier, eh? */
    vertvec_entry(roff)->length = really_draw_row_contents(roff, rpos);
#endif
}


#if defined(SHOW_CURRENT_ROW)

static void
draw_row_border(intl roff)
{
    #if RISCOS
    intl rpos;

    vtracef1(TRACE_DRAW, "\n*** draw_row_border(%d) --- ", roff);

    if(vertvec_entry(roff)->flags & LAST)
        {
        vtracef0(TRACE_REALLY, "last row\n");
        return;
        }

    if(roff > rowsonscreen)
        {
        vtracef2(TRACE_DRAW, "roff (%d) > rowsonscreen (%d)\n", roff, rowsonscreen);
        return;
        }

    if( out_rowborout   &&  (rowborout  == roff))
        out_rowborout  = FALSE;

    if( out_rowborout1  &&  (rowborout1 == roff))
        out_rowborout1 = FALSE;

    if(!borbit)
        return;

    rpos = calrad(roff);

    vtracef1(TRACE_DRAW, "rpos = %d\n", rpos);

    please_redraw_textarea(0, rpos, borderwidth, rpos-1);
    #else
    draw_row(roff);
    #endif
}

#endif


/* only called if borbit on RISC OS */

static void
really_draw_row_border(intl roff, intl rpos)
{
    SCRROW *rptr;

    vtracef1(TRACE_REALLY, "really_draw_row_border(%d): ", roff);

    #if RISCOS
    clear_thistextarea();
    #else
    if(!borbit)
        return;
    #endif

    at(0, rpos);

    rptr = vertvec_entry(roff);

    /* is the row a soft page break? if RISCOS, draw just that bit of it
     * that is in the row border for faster clipping,
     * otherwise drawn whole by really_draw_row_contents()
    */
    if(rptr->flags & PAGE)
        {
        setfgcolour(BORDERC);
        #if RISCOS
        vtracef0(TRACE_REALLY, "border has soft page break\n");
        wrchrep('^', borderwidth);
        #else
        /* soft page breaks fully drawn elsewhere */
        return;
        #endif
        }
    else
        {
        /* draw row number in border */
        rowt trow = rptr->rowno;
        char array[16], *ptr;
        #if defined(SHORT_BORDER)
        char *start = array + 3;
        #else
        char *start = array + 2;
        #endif
        char fillch = 0;

        sprintf(array, "%7ld ", (long) trow + 1);

        if(rptr->flags & FIX)
            fillch = BORDERFIXCH;

        #if defined(SHOW_CURRENT_ROW)
        if(trow == currow)
            {
            fillch = COLUMN_DOTS;
            wrch_definefunny(fillch);
            setfgcolour(CURBORDERC);
            }
        else
        #endif
            setfgcolour(BORDERC);

        if(fillch)
            {
            ptr = start;
            while(*ptr++ == SPACE)
                ptr[-1] = fillch;
            }

        stringout(start);

        #if defined(SHOW_CURRENT_ROW)
        if(fillch == COLUMN_DOTS)
            wrch_undefinefunny(fillch);
        #endif

        /* draw first vbar - needs to include hbar */
        if(grid_on)
            draw_grid_vbar(TRUE);
        }

    setfgcolour(FORE);
}


static intl
really_draw_row_contents(intl roff, intl rpos)
{
    SCRROW *rptr = vertvec_entry(roff);
    intl newlen, len;
    intl coff;
    char tbuf[32];

    vtracef1(TRACE_REALLY, "really_draw_row_contents(%d): ", roff);

    #if !RISCOS
    really_draw_row_border(roff, rpos);
    #endif

    /* is the row a soft (automatic) page break? */

    if(rptr->flags & PAGE)
        {
        /* if RISCOS then left-hand part drawn separately */
        len = RISCOS ? min(hbar_length, RHS_X - borderwidth) : RHS_X;
        vtracef0(TRACE_REALLY, "row has soft page break\n");
        #if RISCOS
        clear_thistextarea();
        at(borderwidth, rpos);
        #endif
        setfgcolour(BORDERC);
        wrchrep('^', len);
        setfgcolour(FORE);
        newlen = os_if_fonts(RISCOS ? len : RHS_X - borderwidth);
        }
    elif(chkrpb(rptr->rowno))
        {
        setfgcolour(BORDERC);

        if(chkfsb()  &&  chkpac(rptr->rowno))
            {
            /* conditional page break occurred */
            len = min(hbar_length, RHS_X - borderwidth);
            vtracef0(TRACE_REALLY, "row has good hard page break\n");
            #if RISCOS
            clear_thistextarea();
            at(borderwidth, rpos);
            #endif
            wrchrep('~', len);
            newlen = os_if_fonts(len);
            }
        else
            {
            /* conditional page break not breaking */
            len = sprintf(tbuf, "~ %d", travel(0, rptr->rowno)->content.page.condval);
            vtracef1(TRACE_REALLY, "row has failed hard page break %s\n", tbuf);
            #if RISCOS
            at(borderwidth, rpos);
            #endif
            stringout_underlaid(tbuf);
            newlen = os_if_fonts(len);
            vtracef1(TRACE_REALLY, "length of line so far: %d\n", newlen);
            }

        setfgcolour(FORE);
        }
    else
        {
        vtracef0(TRACE_REALLY, "normal row: drawing slots\n");

        #if RISCOS
        at(borderwidth, rpos);
        #endif

        newlen = 0;     /* maybe nothing drawn! */

        for(coff = 0; !(horzvec_entry(coff)->flags & LAST); coff++)
            {
            newlen = really_draw_slot(coff, roff, TRUE);
            invoff();
            }

        newlen -= os_if_fonts(borderwidth);
        }

    /* clear crud off eol */
    #if RISCOS
    ospca_fonts(os_if_fonts(hbar_length) - newlen);
    #else
    ospca(rptr->length - newlen);
    #endif

    return(newlen);
}


#if RISCOS

static void
draw_slot(intl coff, intl roff, BOOL in_draw_row)
{
    intl cpos = calcad(coff);
    intl rpos = calrad(roff);
    colt tcol = col_number(coff);
    rowt trow = row_number(roff);
    slotp tslot = travel(tcol, trow);
    intl fwidth = colwidth(tcol);
    intl overlap;

    IGNOREPARM(in_draw_row);    /* NEVER in_draw_row in RISC OS */

    vtracef2(TRACE_DRAW, "\n*** draw_slot(%d, %d)\n", coff, roff);

    if(tslot  &&  (tslot->type == SL_TEXT))
        {
        overlap = chkolp(tslot, tcol, trow);
        fwidth = max(fwidth, overlap);
        }

    please_redraw_textarea(cpos, rpos, cpos + fwidth, rpos-1);
}

#else

static void
draw_slot(intl coff, intl roff, BOOL in_draw_row)
{
    (void) really_draw_slot(coff, roff, in_draw_row);
}

#endif  /* RISCOS */


/****************************
*                           *
* returns new screen column *
*                           *
****************************/

static intl
really_draw_slot(intl coff, intl roff, BOOL in_draw_row)
{
    intl cpos = calcad(coff);
    intl rpos = calrad(roff);
    colt tcol = col_number(coff);
    SCRROW *rptr = vertvec_entry(roff);
    rowt trow = rptr->rowno;
    slotp thisslot = travel(tcol, trow);
    BOOL slotblank = isslotblank(thisslot);
    intl os_cpos = os_if_fonts(cpos);
    intl c_width = colwidth(tcol);
    intl fwidth, overlap;
    intl widthofslot, spaces_at_end, spaces_to_draw;
    intl end_of_inverse, to_do_inverse;
    BOOL slot_in_block;

    static intl last_offset;        /* last draw_slot finished here */

    vtracef4(TRACE_REALLY, "really_draw_slot(%d, %d, %s): slotblank = %s\n",
                coff, roff, trace_boolstring(in_draw_row),
                trace_boolstring(slotblank));

    /* if at beginning of line reset last offset to bos */

    if((coff == 0)  ||  !in_draw_row)
        {
        last_offset = os_cpos;
        vtracef1(TRACE_REALLY, "last_offset := %d\n", last_offset);
        }

    /* special case for blank, partially overlapped slots.
     * just need to space to the end of the column
    */
    if(slotblank  &&  (last_offset > os_cpos))
        {
        /* c_width is column width truncated to eos */
        if( c_width > RHS_X - cpos)
            c_width = RHS_X - cpos;

        spaces_to_draw = os_cpos + os_if_fonts(c_width) - last_offset;

        vtracef1(TRACE_REALLY, "blank, partially overlapped slot: spaces_to_draw := %d\n", spaces_to_draw);

        if(spaces_to_draw > 0)          /* last slot partially overlaps */
            {
            at_fonts(last_offset, rpos);
            (void) setivs(tcol, trow);
        /*  setprotectedstatus(NULL); */
            ospca_fonts(spaces_to_draw);
            last_offset += spaces_to_draw;
            if(grid_on)
                draw_grid_vbar(FALSE);
            }

        return(last_offset);
        }


    /* possible overlap state */
    overlap = chkolp(thisslot, tcol, trow);

    /* if column width > overlap draw some spaces at end */
    spaces_at_end = c_width - overlap;

    if(slotblank  ||  (thisslot->type == SL_TEXT)  ||  (spaces_at_end > 0))
        {
        /* draw blanks & text to overlap position, spaces (>=0) beyond */
        fwidth = overlap;
        spaces_at_end = max(spaces_at_end, 0);
        }
    else
        {
        fwidth = c_width;
        spaces_at_end = 0;
        }

#if defined(CLIP_TO_WINDOW)
    /* now fwidth is maximum of overlap and colwidth, truncate to screen:
     * this can lead to fields being reformatted when window size changes
     * and so is something to be avoided
    */
    if( fwidth > RHS_X - cpos)
        fwidth = RHS_X - cpos;
#endif


    /* we are drawing to last_offset, which is the value returned */

    last_offset = os_cpos + os_if_fonts(fwidth + spaces_at_end);


    /* stored page number invalid if silly cases */
    curpnm = (!encpln || n_rowfixes) ? 0 : rptr->page;


    /* switch inverse on if in block (or on number slot?) */

    slot_in_block = setivs(tcol, trow);
    setprotectedstatus(thisslot);

    #if RISCOS
    /* set window covering slot to prevent bits of fonts sticking out:
     * if no window set then nothing to plot - hoorah!
     * this may even help system font plotting
    */
    if(!set_graphics_window_from_textarea(cpos, rpos, cpos + fwidth, rpos - 1, riscos_fonts))
        {
        at(cpos, rpos);
        widthofslot = 0;
        }
    else
    #endif

    {
    /* this is current slot to be drawn by dspfld
     * don't bother drawing current slot if not numeric
     * we need the !xf_inexpression for moving inverse cursor around
    */
    if( (tcol == curcol)  &&  (trow == currow)  &&  !xf_inexpression  &&
        (!thisslot  ||
         ((thisslot->type == SL_TEXT)  &&  !(thisslot->justify & PROTECTED)) ||
         (thisslot->type == SL_PAGE)))
        {
        widthofslot = dspfld(cpos, rpos, fwidth);
#if RISCOS
        if(riscos_fonts)
            widthofslot = roundtoceil(widthofslot, x_scale);
#endif
        }
    elif(slotblank)
        {
        at(cpos, rpos);
        widthofslot = 0;
        }
    else
        {
        if( thisslot->flags & SL_ALTERED)
            thisslot->flags = SL_ALTERED ^ thisslot->flags;

        if( (thisslot->type == SL_NUMBER)  &&
            (thisslot->content.number.result.value < 0.0))
            {
            if(currently_inverted)
                #if RISCOS
                setbgcolour(NEGATIVEC);
                #else
                if(currently_protected)
                    setcolour(PROTECTC, NEGATIVEC);
                else
                    setcolour(BACK, NEGATIVEC);
                #endif
            else
                #if RISCOS
                setfgcolour(NEGATIVEC);
                #else
                if(currently_protected)
                    setcolour(NEGATIVEC, PROTECTC);
                else
                    setcolour(NEGATIVEC, BACK);
                #endif
            }

        #if RISCOS
        if(riscos_fonts)
            {
            /* position font output */
            screen_xad_os = gcoord_x(cpos);
            screen_yad_os = gcoord_y(rpos);

            riscos_font_xad = x_scale * screen_xad_os;
            riscos_font_yad = y_scale * (screen_yad_os + fontbaselineoffset);

            ensurefontcolours();

            widthofslot = roundtoceil(outslt(thisslot, trow, fwidth), x_scale);
            }
        else
        #endif
            {
            at(cpos, rpos);

            widthofslot = os_if_fonts(outslt(thisslot, trow, fwidth));
            }

        tracef1("widthofslot = %d (OS)\n", widthofslot);
        }
    }

    /* now get fwidth & cpos into os units */

    fwidth = os_if_fonts(fwidth + spaces_at_end) - widthofslot;

    os_cpos += widthofslot;

    #if RISCOS
    /* font painting probably doesn't move output cursor as we want
     * also reset graphics window to paint end of line
    */
    if(riscos_fonts)
        {
        restore_graphics_window();

        at_fonts(os_cpos, rpos);
        }
    #endif

    /* still have to output fwidth spaces, some of them inverse.
     * note that maybe both in block and in expression, therefore being
     * in block doesn't imply we are drawing in inverse
     *
     * need to draw spaces to end of block,
     * toggle inverse and draw spaces to fwidth
    */
    end_of_inverse = end_of_block(os_cpos, trow);
    to_do_inverse  = end_of_inverse - os_cpos;

    tracef3("end_of_inverse %d, os_cpos %d -> to_do_inverse %d\n",
            end_of_inverse, os_cpos, to_do_inverse);

    if(to_do_inverse > 0)
        {
        to_do_inverse = min(to_do_inverse, fwidth);
        draw_spaces_with_grid(to_do_inverse, os_cpos);
        os_cpos += to_do_inverse;
        fwidth -= to_do_inverse;
        }

    if(slot_in_block)
        invert();

    draw_spaces_with_grid(fwidth, os_cpos);

    switch_off_highlights();

    return(last_offset);            /* new screen x address */
}


#if RISCOS

/*************************************************************************
*                                                                        *
* draw spaces on screen from position start, poking in grid if necessary *
*                                                                        *
*************************************************************************/

static void
draw_spaces_with_grid(intl spaces_left, intl start)
{
    colt  tcol;
    intl coff;
    intl c_width, end;
    BOOL last;

    vtracef2(TRACE_REALLY, "draw_spaces_with_grid(%d, %d)\n",
                spaces_left, start);

    if(grid_on)
        {
        /* where we will draw the last grid vbar */
        end = start + spaces_left;

        /* which column are we starting to draw spaces in? */
        coff = calcoff(riscos_fonts ? start / charwidth : start);

        if(coff >= 0)
            {
            tcol = col_number(coff);
            c_width = os_if_fonts(colwidth(tcol)) - (start - os_if_fonts(calcad(coff)));
            tracef2("c_width(%d) = %d\n", tcol, c_width);
            last = FALSE;
            }
        else
            last = TRUE;

        while(spaces_left >= 0)
            {
            tracef1("spaces_left = %d\n", spaces_left);

            if(last)
                {
                tracef0("last column hit - space to end\n");
                ospca_fonts(spaces_left);
                /* don't draw grid vbar as we are given spastic (but correct)
                 * widths for right margin positions beyond last column end
                */
                break;
                }

            if( c_width > spaces_left)
                c_width = spaces_left;

            ospca_fonts(c_width);
            spaces_left -= c_width;

            draw_grid_vbar(FALSE);

            coff++;

            last = (horzvec_entry(coff)->flags & LAST);

            if(!last  &&  (spaces_left > 0))
                {
                tcol = col_number(coff);
                c_width = os_if_fonts(colwidth(tcol));

                tracef2("c_width(%d) = %d\n", tcol, c_width);
                }
            }
        }
    else
        ospca_fonts(spaces_left);
}

#endif


/************************************************
*                                               *
* if mark_row has marked a soft page break      *
* inadvertenly, draw page row and next row down *
*                                               *
************************************************/

static intl
adjust_rowout(intl rowoff)
{
    uchar flags = vertvec_entry(rowoff)->flags;

    if((flags & PAGE)  &&  !(flags & LAST))
        draw_row(rowoff + 1);

    return(rowoff);
}


static intl
adjust_rowborout(intl rowoff)
{
    #if defined(SHOW_CURRENT_ROW)
    uchar flags = vertvec_entry(rowoff)->flags;

    if((flags & PAGE)  &&  !(flags & LAST))
        draw_row_border(rowoff + 1);
    #endif

    return(rowoff);
}


static void
check_output_current_slot(intl move)
{
    if(out_currslot)
        {
        if(atend(curcol, currow))
            mark_to_end(currowoffset);
        else
            mark_slot(travel_here());
        }

    if(!slot_in_buffer  ||  move)
        draw_contents_of_numslot();

    if( out_screen  ||
        out_below   ||
        (out_rowout  &&  (rowout  == currowoffset)) ||
        (out_rowout1 &&  (rowout1 == currowoffset)) )
        {
        output_buffer = TRUE;
        dspfld_from = -1;
        }

    filbuf();
}


/*****************************************************
*                                                    *
* set inverse state                                  *
* Varies if current numeric slot and in marked block *
*                                                    *
*****************************************************/

static BOOL
setivs(colt tcol, rowt trow)
{
    BOOL invert_because_editing = (xf_inexpression  &&  (tcol == curcol)  &&  (trow == currow));
    BOOL invert_because_inblock = inblock(tcol, trow);

    if(invert_because_inblock ^ invert_because_editing)
        invert();

    return(invert_because_inblock);
}


/*********************************************
*                                            *
* if xf_drawsome, some slots need redrawing. *
* Scan through horzvec and vertvec           *
* drawing the ones that have changed         *
*                                            *
*********************************************/

static void
draw_altered_slots(void)
{
    SCRCOL *i_cptr = horzvec();
    SCRROW *i_rptr = vertvec();
    SCRCOL *cptr;
    SCRROW *rptr;
    intl    coff;
    intl    roff;
    slotp   tslot;

    vtracef0(TRACE_DRAW, "\n*** draw_altered_slots()\n");

    rptr = i_rptr;

    while(!(rptr->flags & LAST))
        {
        if(!(rptr->flags & PAGE))
            {
            cptr = i_cptr;

            while(!(cptr->flags & LAST))
                {
                tslot = travel(cptr->colno, rptr->rowno);

                if(tslot)
                    {
                    if(tslot->type == SL_PAGE)
                        break;

                    if(tslot->flags & SL_ALTERED)
                        {
                        coff = (intl) (cptr - i_cptr);
                        roff = (intl) (rptr - i_rptr);

                        draw_slot(coff, roff, FALSE);

                        i_cptr  = horzvec();
                        i_rptr  = vertvec();
                        cptr    = i_cptr + coff;
                        rptr    = i_rptr + roff;

                        invoff();
                        }

                    #if RISCOS
                    if(keyormouseinbuffer())
                    #else
                    if(keyormouseinbuffer()  ||  (autorepeat  &&  depressed_shift()))
                    #endif
                        {
                        vtracef0(TRACE_DRAW, "draw_altered_slots interrupted\n");
                        xf_interrupted = TRUE;
                        return;
                        }
                    }

                cptr++;
                }
            }

        rptr++;
        }

    xf_drawsome = FALSE;        /* only reset when all are done */
}


/********************************************
*                                           *
* draw one slot, given the slot reference   *
* called just from evaluator for            *
* interruptability                          *
*                                           *
********************************************/

extern void
draw_one_altered_slot(colt col, rowt row)
{
    SCRCOL *i_cptr  = horzvec();
    SCRROW *i_rptr  = vertvec();
    SCRCOL *cptr    = i_cptr;
    SCRROW *rptr    = i_rptr;
    slotp   tslot;

    vtracef0(TRACE_DRAW, "\n*** draw_one_altered_slot()\n");

    #if RISCOS
    /* we cope with slots being updated behind a dialog box and other windows */
    #elif MS || ARTHUR
    if(in_dialog_box  ||  (current_document_handle() != front_doc))
        return;
    #endif

    /* cheap tests to save loops if slot not in window - fixing complicates */
    if( !(rptr->flags & FIX)  &&
        ((rptr->rowno > row)  ||  (rptr[rowsonscreen-1].rowno < row))
        )
        return;

    if( !(cptr->flags & FIX)  &&
        (cptr->colno > col)
        )
        return;

    /* no need to reload pointers as only one slot is being drawn */
    while(!(rptr->flags & LAST))
        {
        if((rptr->rowno == row)  &&  !(rptr->flags & PAGE))
            while(!(cptr->flags & LAST))
                {
                if(cptr->colno == col)
                    {
                    tslot = travel(cptr->colno, rptr->rowno);

                    if(tslot)
                        {
                        #if MS || ARTHUR
                        curoff();
                        #endif

                        draw_slot((intl)(cptr - i_cptr),
                                  (intl)(rptr - i_rptr), FALSE);

                        sb_show_if_fastdraw();

                        invoff();
                        }

                    return;
                    }

                cptr++;
                }

        rptr++;
        }
}


/************************************************
*                                               *
*  clear to bottom of screen from end of sheet  *
*                                               *
************************************************/

static void
draw_empty_bottom_of_screen(void)
{
    intl start = calrad(rowsonscreen);

    vtracef0(TRACE_DRAW, "\n*** draw_empty_bottom_of_screen()\n");

    if(start > paghyt)
        {
        vtracef2(TRACE_DRAW, "--- start (%d) > paghyt (%d) \n", start, paghyt);
        return;
        }

#if RISCOS

    /* may be overlapped by a Draw file - redraw not clear */
    please_redraw_textarea(0, paghyt, RHS_X, start-1);
}


static void
maybe_draw_empty_bottom_of_screen(void)
{
    intl start = calrad(rowsonscreen);

    vtracef0(TRACE_MAYBE, "maybe_draw_empty_bottom_of_screen() --- ");

    if(start > paghyt)
        {
        vtracef2(TRACE_MAYBE, "start (%d) > paghyt (%d) \n", start, paghyt);
        }
    else
        {
        vtracef0(TRACE_MAYBE, "some bottom on screen\n");

        /* required to redraw left slop too */

        if(textobjectintersects(-1, paghyt, RHS_X, start-1))
            {
            vtracef0(TRACE_REALLY, "really_draw_empty_bottom_of_screen()\n");

            clear_thistextarea();
            }
        }

#elif MS || ARTHUR

    {
    SCRROW *rptr        = vertvec();
    SCRROW *last_rptr   = rptr + maxnrow;

    rptr += rowsonscreen;

    while(rptr < last_rptr)
        rptr++->length = 0;

    mycls(0, paghyt, RHS_X, start);
    }

#endif  /* RISCOS */
}


#if RISCOS

static void
maybe_draw_empty_right_of_screen(void)
{
    BOOL grid_below = !borbit  &&  grid_on;
    intl grid_adjust = grid_below ? 1 : 0;
    intl x0 = borderwidth + hbar_length;
    intl y0 = calrad(rowsonscreen) - 1;
    intl x1 = RHS_X;
    intl y1 = calrad(0) - 1 - grid_adjust;

    /* required to redraw one more raster if grid below editing line */

    if(textobjectintersects(x0, y0, x1, y1))
        {
        vtracef0(TRACE_REALLY, "really_draw_empty_right_of_screen()\n");

        clear_textarea(thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1 + grid_adjust,
                       grid_below);
        }
}


static void
maybe_draw_unused_bit_at_bottom(void)
{
    if(unused_bit_at_bottom)
        {
        vtracef0(TRACE_MAYBE, "maybe_draw_unused_bit_at_bottom()\n");

        /* required to redraw left slop too */

        if(textobjectintersects(-1, paghyt+1, RHS_X, paghyt))
            {
            vtracef0(TRACE_REALLY, "really_draw_unused_bit_at_bottom()\n");
            
            /* may have been overlapped by a Draw file */
            clear_thistextarea();
            }
        }
}

#endif  /* RISCOS */


/********************************************************************************
*                                                                               *
* expand the slot and call the appropriate justification routine to print it    *
*                                                                               *
* fwidth maximum width of screen available                                      *
* returns number of characters drawn or number of millipoints                   *
*                                                                               *
********************************************************************************/

extern intl
outslt(slotp tslot, rowt trow, intl fwidth)
{
    #if RISCOS
    /* need lots more space cos of fonty bits in RISCOS */
    uchar array[PAINT_STRSIZ];
    #else
    uchar array[LIN_BUFSIZ];
    #endif
    uchar justify;
    intl res;

    tracef3("[outslt: slot &%p, row %d, fwidth %d]\n", tslot, trow, fwidth);

    justify = expand_slot(tslot, trow, array, fwidth, TRUE, TRUE, TRUE);

    switch(justify)
        {
        case J_LCR:
            return(lcrjust(array, fwidth, FALSE));


        case J_LEFTRIGHT:
        case J_RIGHTLEFT:
            #if RISCOS
            if(riscos_fonts)
                return(font_paint_justify(array, fwidth));
            #endif

            #if defined(VIEW_IO)
            res = justifyline(array, fwidth, justify, NULL);
            #else
            res = justifyline(array, fwidth, justify);
            #endif
            microspacing = FALSE;
            return(res);


        default:
            return(onejst(array, fwidth, justify));
        }
}


/********************************
*                               *
* set up a font rubout box      *
* given swidth and current pos  *
*                               *
* NB. Only for screen use!      *
*                               *
********************************/

#if RISCOS

static void
font_setruboutbox(intl swidth_mp)
{
    intl swidth_os = roundtoceil(swidth_mp, x_scale);

    tracef1("[font_ruboutbox: swidth_os = %d]\n", swidth_os);

    wimpt_safe(bbc_move(screen_xad_os, screen_yad_os + (grid_on ? dy : 0)));
    wimpt_safe(bbc_plot(bbc_MoveCursorRel, swidth_os, +charvrubout_pos +charvrubout_neg));
}

#endif


/********************************
*                               *
*  output one justified string  *
*                               *
********************************/

extern intl
onejst(uchar *str, intl fwidth_ch, uchar type)
{
    /* leave one char space to the right in some cases */
    const intl fwidth_adjust_ch = ((type == J_CENTRE)  ||  (type == J_RIGHT)) ? 1 : 0;
    intl spaces, swidth_ch;
    uchar *ptr, ch;
    #if RISCOS
    intl fwidth_mp, swidth_mp, x_mp, xx_mp;
    char paint_buf[PAINT_STRSIZ], *paint_str;
    intl paint_op = font_ABS;
    #endif

    fwidth_ch -= fwidth_adjust_ch;

    #if RISCOS
    if(riscos_fonts)
        {
        if(type != J_FREE)
            {
            font_strip_spaces(paint_buf, str, NULL);
            paint_str = paint_buf;
            }
        else
            paint_str = str;

        swidth_mp = font_width(paint_str);

        fwidth_mp = ch_to_mp(fwidth_ch);

        if(swidth_mp > fwidth_mp)
            {
            /* fill as much field as possible if too big */
            x_mp  = 0;
            xx_mp = font_truncate(paint_str, fwidth_mp + ch_to_mp(fwidth_adjust_ch));
            }
        else
            {
            switch(type)
                {
                default:
                #if TRACE
                    tracef0("*** error in OneJst ***\n");

                case J_FREE:
                case J_LEFT:
                #endif
                    x_mp  = 0;
                    xx_mp = swidth_mp;
                    break;

                case J_RIGHT:
                    x_mp  = fwidth_mp - swidth_mp;
                    xx_mp = fwidth_mp;
                    break;

                case J_CENTRE:
                    x_mp  = (fwidth_mp - swidth_mp) / 2;
#if 1
                    xx_mp = fwidth_mp;  /* make centred fields fill field (for grid) */
#else
                    xx_mp = swidth_mp + x_mp;
#endif
                    break;
                }
            }

        if(draw_to_screen)
            {
            font_setruboutbox(xx_mp);

            paint_op |= font_RUBOUT;
            }

        tracef2("[onejst font_paint x: %d, y: %d]\n",
                riscos_font_xad + x_mp, riscos_font_yad);

        #if TRACE
        trace_system("memory %p + 30", paint_str);
        #endif

        font_complain(font_paint(paint_str, paint_op,
                                 riscos_font_xad + x_mp, riscos_font_yad));

        return(xx_mp);
        }
    #endif

    /* ignore leading and trailing spaces */

    if(type != J_FREE)
        {
        --str;
        do { ch = *++str; } while(ch == SPACE);
        }

    /* find end of string */
    ptr = str;
    while(*ptr)
        ptr += font_skip(ptr);

    /* strip trailing spaces */
    do { ch = *--ptr; } while(ch == SPACE);

    /* reinstate funny space put on by sprintnumber */
    if(ch == FUNNYSPACE)
        *ptr = SPACE;

    *++ptr = '\0';

    swidth_ch = calsiz(str);

    spaces = 0;

    if(swidth_ch > fwidth_ch)
        /* fill as much field as possible if too big */
        fwidth_ch += fwidth_adjust_ch;
    else
        switch(type)
            {
            default:
            #if TRACE
                tracef0("*** error in OneJst ***\n");

            case J_FREE:
            case J_LEFT:
            #endif
                break;

            case J_RIGHT:
                spaces = fwidth_ch - swidth_ch;
                break;

            case J_CENTRE:
                spaces = (fwidth_ch - swidth_ch) >> 1;
                break;
            }

    if(spaces > 0)
        ospca(spaces);

    return(spaces + strout(str, fwidth_ch, TRUE));
}


/********************************************************
*                                                       *
* get rid of leading and trailing spaces in a string    *
* puts NULL at end and returns first non-space char     *
*                                                       *
********************************************************/

static uchar *
lose_spaces(uchar *str)
{
    uchar *ptr, ch;

    /* lose leading spaces */
    --str;
    do { ch = *++str; } while(ch == SPACE);

    /* set ptr to end of string */
    ptr = str;
    while(*ptr)
        ptr += font_skip(ptr);

    /* move back over spaces */
    ++ptr;
    while((--ptr > str)  &&  (*(ptr-1) == SPACE))
        ;

    /* lose trailing spaces */
    *ptr = '\0';

    return(str);
}


/**************************************************
*                                                 *
* display three strings at str to width of fwidth *
*                                                 *
**************************************************/

extern intl
lcrjust(uchar *str, intl fwidth_ch, BOOL reversed)
{
    uchar *str2, *str3, *tstr;
    intl str1len, str2len, str3len;
    intl spaces1, spaces2;
    intl sofar;
    #if RISCOS
    char paint1_buf[PAINT_STRSIZ], paint2_buf[PAINT_STRSIZ], paint3_buf[PAINT_STRSIZ];
    intl swidth1_mp, swidth2_mp, swidth3_mp;
    intl fwidth_mp, x_mp, xx_mp;
    #endif

    vtracef3(TRACE_REALLY, "lcrjust(\"%s\", fwidth_ch = %d, reversed = \"%s\")\n",
                str, fwidth_ch, trace_boolstring(reversed));

    /* wee three strings from outslt are: baring nulls wee travel() sofar */
    str2 = str;
    while(*str2)
        str2 += font_skip(str2);
    ++str2;

    str3 = str2;
    while(*str3)
        str3 += font_skip(str3);
    ++str3;

    if(reversed)
        {
        tstr = str3;
        str3 = str;
        str  = tstr;
        }

    /* leave one char space to the right */
    --fwidth_ch;

    #if RISCOS
    if(riscos_fonts)
        {
        x_mp  = 0;
        xx_mp = 0;

        font_strip_spaces(paint1_buf, str,  NULL);
        font_strip_spaces(paint2_buf, str2, NULL);
        font_strip_spaces(paint3_buf, str3, NULL);

        swidth1_mp = font_width(paint1_buf);
        swidth2_mp = font_width(paint2_buf);
        swidth3_mp = font_width(paint3_buf);

        fwidth_mp = ch_to_mp(fwidth_ch);

        if(draw_to_screen)
            clear_underlay(fwidth_ch);

        if( swidth1_mp > fwidth_mp)
            swidth1_mp = font_truncate(paint1_buf, fwidth_mp);

        if(swidth1_mp)
            {
            font_complain(font_paint(paint1_buf, font_ABS,
                                     riscos_font_xad, riscos_font_yad));
            xx_mp = swidth1_mp;
            }


        x_mp = (fwidth_mp - swidth2_mp) / 2;

        if(swidth2_mp  &&  (x_mp >= xx_mp))
            {
            font_complain(font_paint(paint2_buf, font_ABS,
                                     riscos_font_xad + x_mp, riscos_font_yad));
            xx_mp = x_mp + swidth2_mp;
            }


        x_mp = fwidth_mp - swidth3_mp;

        if(swidth3_mp  &&  (x_mp >= xx_mp))
            {
            font_complain(font_paint(paint3_buf, font_ABS,
                                     riscos_font_xad + x_mp, riscos_font_yad));
            xx_mp = x_mp + swidth3_mp;
            }

        return(xx_mp);
        }
    #endif

    /* get rid of leading and trailing spaces */
    str  = lose_spaces(str);
    str2 = lose_spaces(str2);
    str3 = lose_spaces(str3);

    str1len = calsiz(str);
    str2len = calsiz(str2);
    str3len = calsiz(str3);

    spaces1 = ((fwidth_ch - str2len) >> 1) - str1len;

    spaces2 = fwidth_ch - str1len - spaces1 - str2len - str3len;

    if(spaces2 < 0)
        spaces1 += spaces2;


    sofar = strout(str, fwidth_ch, TRUE);

    switch_off_highlights();

    if(spaces1 > 0)
        {
        sofar += spaces1;
        ospca(spaces1);
        }


    sofar += strout(str2, fwidth_ch - sofar, TRUE);

    switch_off_highlights();

    if(spaces2 > 0)
        {
        sofar += spaces2;
        ospca(spaces2);
        }


    sofar += strout(str3, fwidth_ch - sofar, TRUE);

    switch_off_highlights();

    return(sofar);
}


/*******************************
*                              *
* paint justified fonty string *
*                              *
*******************************/

#if RISCOS

static intl
font_paint_justify(char *str_in, intl fwidth_ch)
{
    /* leave one char space to the right */
    const intl fwidth_adjust_ch = 1;
    intl fwidth_mp, swidth_mp, lead_space_mp;
    char paint_buf[PAINT_STRSIZ];
    intl paint_op = font_ABS;

    fwidth_ch -= fwidth_adjust_ch;

    /* account for spaces */
    lead_space_mp = font_strip_spaces(paint_buf, str_in, NULL);

    fwidth_mp = ch_to_mp(fwidth_ch);

    swidth_mp = font_width(paint_buf);

    tracef4("[font_justify width: %d, fwidth_mp: %d, fwidth_ch: %d, lead_space_mp: %d]\n",
            swidth_mp, fwidth_mp, fwidth_ch, lead_space_mp);

    if(swidth_mp + lead_space_mp > fwidth_mp)
        swidth_mp = font_truncate(paint_buf, fwidth_mp + ch_to_mp(fwidth_adjust_ch) - lead_space_mp);
    else
        {
        swidth_mp = fwidth_mp;

        paint_op |= font_JUSTIFY;
        }

    if(draw_to_screen)
        {
        /* rubout box must be set up before justification point */
        font_setruboutbox(swidth_mp);

        paint_op |= font_RUBOUT;
        }

    if(paint_op & font_JUSTIFY)
        /* provide right-hand justification point */
        wimpt_safe(bbc_move(roundtofloor(riscos_font_xad + fwidth_mp, x_scale),
                            riscos_font_yad / y_scale));

    font_complain(font_paint(paint_buf, paint_op,
                             riscos_font_xad + lead_space_mp,
                             riscos_font_yad));

    return(swidth_mp);
}

#endif


/***************************************
*                                      *
* display the line justified to fwidth *
* if VIEW_IO and out_array set,        *
* expand the the line with soft spaces *
* into out_array (VIEW saving)         *
*                                      *
***************************************/

extern intl
#if defined(VIEW_IO)
justifyline(uchar *str, intl fwidth, uchar type, uchar *out_array)
#else
justifyline(uchar *str, intl fwidth, uchar type)
#endif
{
    uchar *from;
    intl leadingspaces = 0, trailingspaces = 0, words, gaps, spacestoadd;
    intl linelength, firstgap, secondgap, gapbreak, fudge_for_highlights;
    intl c_width, count;
    intl added_sofar = 0, mcount = 0;

    /* send out the leading spaces */
    while(*str++ == SPACE)
        leadingspaces++;

    from = --str;   /* point to first non-space */

    fwidth -= leadingspaces;

    #if defined(VIEW_IO)
    /* if outputting to VIEW file, send the spaces */
    if(out_array)
        while(leadingspaces > 0)
            {
            *out_array++ = SPACE;
            --leadingspaces;
            }
    #endif  /* VIEW_IO */

    ospca(leadingspaces);

    if(!*from)
        return((intl) leadingspaces);   /* line was just full of spaces */

    /* count words, gaps, trailing spaces */
    fudge_for_highlights = 0;
    words = 0;
    while(*from)
        {
        words++;

        while(*from  &&  (*from != SPACE))
            {
            if(ishighlight(*from))
                fudge_for_highlights++;
                /* if it's a highlight but displayed as inverse char
                 * it uses an extra character space on screen
                */
            from++;
            }

        trailingspaces = 0;
        while(*from++ == SPACE)
            trailingspaces++;
        --from;
        }

    /* get rid of trailing spaces */
    *(from - trailingspaces) = '\0';

    gaps = words-1;
    if(gaps < 1)
        #if defined(VIEW_IO)
        {
        if(out_array)
            {
            strncpy(out_array, str, fwidth);
            return(leadingspaces + strlen(out_array));
            }
        else
            return(leadingspaces + strout(str, fwidth, TRUE));
        }
        #else
        return(leadingspaces + strout(str, fwidth, TRUE));
        #endif

    linelength = (from-str) - fudge_for_highlights - trailingspaces;

    setssp();                       /* set standard spacing */

    /* c_width is width of character, one on screen, if microspacing pitch + offset */
    microspacing = (prnbit  &&  (micbit  ||  riscos_printing));
    c_width =   !microspacing
                    ? 1
                    :
                #if RISCOS
                riscos_printing
                    ? charwidth
                    :
                #endif
                      (intl) smispc;

    /* spacestoadd is in micro space units */
    if((spacestoadd = (fwidth-linelength-1) * c_width) <= 0)
        #if defined(VIEW_IO)
        {
        if(out_array)
            {
            strncpy(out_array, str, fwidth);
            return(leadingspaces + strlen(out_array));
            }
        else
            return(leadingspaces + strout(str, fwidth, TRUE));
        }
        #else
        return(leadingspaces + strout(str, fwidth, TRUE));
        #endif

    /* firstgap in microspace units */
    firstgap = spacestoadd / gaps;

    /* gapbreak is number of words in for change in gap */
    gapbreak = spacestoadd - (gaps * firstgap);

    if(type == J_LEFTRIGHT)
        {
        secondgap = firstgap;
        firstgap++;
        }
    else
        {
        secondgap = firstgap+1;
        gapbreak = gaps - gapbreak;
        }

    /* send out each word, followed by gap */
    from = str;
    count = 0;
    while(((count + added_sofar) <
        #if defined(VIEW_IO)
         (fwidth + (out_array ? fudge_for_highlights : 0)))
        #else
         (fwidth))
        #endif
            &&  *from)
        {
        BOOL nullfound = FALSE;
        intl oldcount;
        intl spacesout;
        uchar *lastword;
        intl addon;
        intl widthleft;

        /* find end of word */
        for(lastword = from; *from != SPACE; from++)
            if(*from == '\0')
                {
                nullfound = TRUE;
                break;
                }

        /* *from must be space here */
        *from = '\0';

        /* send out word */
        #if defined(VIEW_IO)
        if(out_array)
            {
            intl tcount = strlen(strncpy(out_array, lastword,
                                 fwidth-count-added_sofar+fudge_for_highlights));
            out_array += tcount;
            count += tcount;
            }
        else
            count += strout(lastword, fwidth-count - added_sofar, FALSE);
        #else
        count += strout(lastword, fwidth-count - added_sofar, FALSE);
        #endif

        if(nullfound)
            break;

        oldcount = count;
        /* reset *from to space */

        for(spacesout = 0, *from = SPACE;
            (added_sofar + count < fwidth)  &&  (*from == SPACE);
            count++, spacesout += c_width, from++)
            ;

        if(*from == '\0')
            {
            count = oldcount;
            break;
            }

        /* add gap */
        addon = (gapbreak-- > 0) ? firstgap : secondgap;

        widthleft = c_width * (fwidth-count-added_sofar);
        if(addon > widthleft)
            addon = widthleft;

        /* need to output real spaces in gap plus micro increment */
        spacesout += addon;

        /* retain count of micro spaces sent */
        mcount += addon;
        added_sofar = mcount / c_width;

        /* send space out */

        #if !defined(PRINT_OFF)
        if(microspacing)
            mspace((uchar) spacesout);
        else
        #endif

        #if defined(VIEW_IO)
        if(out_array)
            {
            /* spacesout-addon hard, addon soft */
            while(spacesout-- > addon)
                *out_array++ = SPACE;                   

            while(addon-- > 0)
                *out_array++ = TEMP_SOFT_SPACE;
            }
        else
        #endif

        if(spacesout > 0)
            {
            if(sqobit  &&  !riscos_printing)
                do { sndchr(SPACE); } while(--spacesout > 0);
            else
                {
                if(highlights_on)
                    {
                    if(draw_to_screen)
                        clear_underlay(spacesout);

                    do { sndchr(SPACE); } while(--spacesout > 0);
                    }
                else
                    ospca(spacesout);
                }
            }
        }

    purgebuffer();

    return(leadingspaces + count + added_sofar);
}


/****************************************************
*                                                   *
* return the width of a fonty string in millipoints *
*                                                   *
****************************************************/

#if RISCOS

extern intl
font_width(char *str)
{
    font_string fs;

    fs.s = str;
    fs.x = fs.y = INT_MAX;
    fs.split = -1;
    fs.term = INT_MAX;

    tracef1("[font_width str in: \"%s\", ", trace_string(str));
    font_complain(font_strwidth(&fs));
    tracef1("width: %d]\n", fs.x);
    return(fs.x);
}

#endif


/*******************************************
*                                          *
* truncate a fonty string to a given width *
*                                          *
*******************************************/

#if RISCOS

extern intl
font_truncate(char *str, intl width)
{
    font_string fs;

    fs.s = str;
    fs.x = width;
    fs.y = INT_MAX;
    fs.split = -1;
    fs.term = INT_MAX;

    font_complain(font_strwidth(&fs));
    tracef3("[font_truncate before width: %d, term: %d, str: \"%s\"]\n",
            width, fs.term, str);
    str[fs.term] = '\0';
    tracef2("[font_truncate after width: %d, str: \"%s\"]\n", fs.x, str);
    return(fs.x);
}

#endif


/****************************************************
*                                                   *
* account for and strip leading and trailing spaces *
* from a fonty string, taking note of leading font/ *
* highlight changes too                             *
*                                                   *
****************************************************/

#if RISCOS

extern intl
font_strip_spaces(char *out_buf, char *str_in, intl *spaces)
{
    intl lead_spaces, lead_space_mp, font_change;
    char *i, *o, *str, ch;

    /* find end of string and find leading space */
    lead_spaces = font_change = 0;
    str = NULL;
    i = str_in;
    o = out_buf;
    while(*i)
        {
        intl nchar;

        font_change = is_font_change(i);

        if(!str)
            {
            if(*i == SPACE)
                {
                ++lead_spaces;
                ++i;
                continue;
                }
            else if(!font_change)
                str = i;
            }

        nchar = font_skip(i);
        memcpy(o, i, nchar);

        i += nchar;
        o += nchar;
        }

    /* stuff trailing spaces */
    if(o != out_buf)
        {
        while(*--o == SPACE && !font_change);
        *(o + 1) = '\0';
        }
    else
        *o = '\0';

    if(str)
        {
        ch = *str;
        *str = '\0';
        lead_space_mp = font_width(str_in);
        *str = ch;
        }
    else
        lead_space_mp = 0;

    if(spaces)
        *spaces = lead_spaces;

    return(lead_space_mp);
}

#endif


/***************************************************
*                                                  *
* initialise flags and justification(microspacing) *
* maybe skip leading spaces                        *
*                                                  *
* for(each char in string && !end of field)        *
*   deal with highlights, gaps, microspacing       *
*   sndchr(ch)                                     *
* clean up after justification and highlights      *
* return number of chars output                    *
*                                                  *
* with_atts false displays in white on black       *
*                                                  *
***************************************************/

extern intl
strout(uchar *str, intl fwidth, BOOL purge)
{
    intl count = 0;
    #if RISCOS
    intl len;
    #endif

    #if RISCOS
    if(draw_to_screen  &&  (fwidth > 0))
        {
        len = strlen(str);
        clear_underlay(min(len, fwidth));
        }
    #endif

    while(*str  &&  (count < fwidth))
        {
        if( *str == FUNNYSPACE)
            *str = SPACE;

        /* sndchr returns width of thing (highlight?) output */
        if(sndchr(*str++))
            count++;
        }

    if(purge)
        purgebuffer();

    return(count);
}


/****************************************************************
*                                                               *
* send char to screen or printer dealing with highlights etc.   *
* doesn't clear it's own background - caller is responsible     *
* return TRUE if print-head moved on                            *
*                                                               *
****************************************************************/

extern BOOL
sndchr(uchar ch)
{
    intl eorval;

    #if !defined(PRINT_OFF)
    /* if RISC OS printing, need to paint char as usual */
    if(sqobit  &&  !riscos_printing)
        return(prnout(ch));
    #endif

    if((ch >= SPACE)  &&  (ch != DELETE))
        {
        #if ARTHUR || RISCOS
        if(highlights_on)
            wrch_h(ch);
        else
            wrch(ch);
        #elif MS
        addtobuff(ch);
        #endif

        return(TRUE);
        }

    purgebuffer();

    switch(ch)
        {
        #if ARTHUR || RISCOS
        case HIGH_UNDERLINE:
            eorval = N_UNDERLINE;
            break;

        case HIGH_ITALIC:
            eorval = N_ITALIC;
            break;

        case HIGH_SUBSCRIPT:
            eorval = N_SUBSCRIPT;
            break;

        case HIGH_SUPERSCRIPT:
            eorval = N_SUPERSCRIPT;
            break;
        #endif

        case HIGH_BOLD:
            #if MS
            if(currently_inverted)
                d_colours[BACK].option ^= BOLDMASK;
            else
                d_colours[FORE].option ^= BOLDMASK;

            setcolour(FORE, BACK);
            #endif
            eorval = N_BOLD;
            break;

        default:
            twzchr(ch);
            return(TRUE);
        }

    highlights_on ^= eorval;
    return(FALSE);
}


/*
find the last column position on the screen in an inverse block given
an x position in inverse and the column
*/

static intl
end_of_block(intl xpos, rowt trow)
{
#if defined(CLIP_TO_WINDOW)
    intl os_pagwid_plus1 = os_if_fonts(RHS_X);
#endif
    #if RISCOS
    intl coff = calcoff(riscos_fonts ? (xpos / charwidth) : xpos);
    #else
    intl coff = calcoff(xpos);
    #endif
    colt tcol;
    SCRCOL *cptr = horzvec_entry(coff);

    /* get screen address of beginning of column underneath x */
    xpos = os_if_fonts(calcad(coff));

    /* while the column is marked add its width on to x */
    for(;;)
        {
        if(cptr->flags & LAST)
            return(xpos);

        tcol = cptr->colno;

        if(!inblock(tcol, trow))
            return(xpos);

        xpos += os_if_fonts(colwidth(tcol));

#if defined(CLIP_TO_WINDOW)
        if(xpos >= os_pagwid_plus1)
            return(os_pagwid_plus1);
#endif

        cptr++;
        }
}


/****************************************
*                                       *
* correct lescrl to that value suitable *
* for outputting the current buffer     *
* in the given field at a position      *
*                                       *
****************************************/

static void
adjust_lescrl(intl x, intl fwidth_ch)
{
    #if RISCOS
    char paint_str[PAINT_STRSIZ];
    intl fwidth_mp, swidth_mp;
    #endif

    fwidth_ch = min(fwidth_ch, pagwid_plus1 - x);

    if(--fwidth_ch < 0)     /* rh scroll margin */
        fwidth_ch = 0;

#if RISCOS
    if(riscos_fonts  &&  !xf_inexpression  &&  fwidth_ch)
        {
        /* adjust for fancy font printing */

        fwidth_mp = ch_to_mp(fwidth_ch);

        do  {
            /* fonty cal_lescrl a little more complex */

            /* is the caret off the left of the field? (scroll margin one char) */
            if(lecpos <= lescrl)
                {
                /* off left - try to centre caret in field */
                if(lescrl)
                    lescrl = max(lecpos - 2, 0);        /* 3rd attempt! */

                break;
                }
            else
                {
                /* is the caret off the right of the field? (scroll margin one char) */
                expand_current_slot_in_fonts(paint_str, TRUE, NULL);
                swidth_mp = font_width(paint_str);

                tracef2("[adjust_lescrl: fwidth_mp %d, swidth_mp %d]\n", fwidth_mp, swidth_mp);

                if(swidth_mp > fwidth_mp)
                    /* off right - try to right justify */
                    lescrl++;
                else
                    break;
                }
            }
        while(TRUE);
        }
    else
#endif
        {
        /* adjust for standard font printing */

        lescrl = cal_lescrl(fwidth_ch);
        }
}


/************************************************
*                                               *
* optimise re-painting only from dspfld_from    *
* to stop the flickering and speed it up        *
*                                               *
************************************************/

static intl
cal_dead_text(void)
{
    intl linelen = strlen(linbuf);
    intl offset = min(lescrl, linelen);
    char *ptr = (char *) linbuf + offset;
    intl dead_text;

    dspfld_from = min(dspfld_from, linelen);

    dead_text = dspfld_from - offset;

    /* always reset */
    dspfld_from = -1;

    if((lescrl == old_lescroll)  &&  (dead_text > 0))
        {
        ptr += dead_text;
        while((*--ptr == SPACE)  &&  dead_text)
            --dead_text;
        ++ptr;

        tracef1("[cal_dead_text: %d dead text]\n", dead_text);
        }
    else
        dead_text = 0;

    if(riscos_fonts  &&  !xf_inexpression)
        /* too hard man */
        dead_text = 0;

    return(dead_text);
}


/****************************************
*                                       *
* display the field in linbuf to x, y   *
* fwidth is maximum field width allowed *
*                                       *
* it must clear its own background      *
*                                       *
****************************************/

extern intl
dspfld(intl x, intl y, intl fwidth_ch)
{
    intl dspfld_written;
    intl trailing_spaces, chars_printed, width_left;
    intl linelen, offset;
    char *ptr, *start;
    #if RISCOS
    char paint_str[PAINT_STRSIZ];
    intl fwidth_mp, swidth_mp;
    intl this_font;
    #endif

    tracef3("[dspfld: x %d, y %d, fwidth_ch %d]\n", x, y, fwidth_ch);

#if RISCOS
    if(riscos_fonts  &&  !xf_inexpression)
        {
        /* fancy font printing */

        screen_xad_os = gcoord_x(x);
        screen_yad_os = gcoord_y(y);

        riscos_font_xad = x_scale * screen_xad_os;
        riscos_font_yad = y_scale * (screen_yad_os + fontbaselineoffset);

        ensurefontcolours();

        fwidth_mp = ch_to_mp(fwidth_ch);

        expand_current_slot_in_fonts(paint_str, FALSE, &this_font);
        swidth_mp = font_width(paint_str);
        tracef2("[dspfld: fwidth_mp = %d, swidth_mp = %d]\n", fwidth_mp, swidth_mp);

        if( swidth_mp > fwidth_mp)
            swidth_mp = font_truncate(paint_str, fwidth_mp);

        font_setruboutbox(swidth_mp);

        font_complain(font_paint(paint_str, font_ABS + font_RUBOUT,
                                 riscos_font_xad, riscos_font_yad));

        if(word_to_invert)
            killfontcolourcache();

        dspfld_written = swidth_mp;
        }
    else
#endif
    {
    /* standard font printing */

    linelen = strlen(linbuf);
    offset = min(lescrl, linelen);
    ptr = (char *) linbuf + offset;
    trailing_spaces = 0;
    chars_printed = 0;

    fwidth_ch = min(fwidth_ch, RHS_X - x);

    width_left = fwidth_ch;

    at(x, y);

    tracef4("[dspfld: x = %d, fwidth_ch = %d, width_left = %d, lescrl = %d]\n",
            x, fwidth_ch, width_left, lescrl);

    while(width_left > 0)
        {
        start = ptr;

        /* gather up ordinary characters for block print */
        while((*ptr++ > SPACE)  &&  (width_left > 0))
            --width_left;
            
        /* and print them */
        if(--ptr != start)
            {
            /* any trailing spaces left over from last time? */
            if(trailing_spaces)
                {
                chars_printed += trailing_spaces;
                ospca(trailing_spaces);
                trailing_spaces = 0;
                }

            /* output string */
            chars_printed += ptr - start;
            stringout_underlaid_field(start, ptr - start);
            }

        /* gather up trailing spaces for eventual block print */
        while((*ptr++ == SPACE)  &&  (width_left > 0))
            {
            ++trailing_spaces;
            --width_left;
            }

        /* at end of field */
        if(!*--ptr  ||  (width_left <= 0))
            {
            /* may be some spaces to output if
             * lecpos is off the end of the buffer
            */
            trailing_spaces = (lecpos - lescrl) - chars_printed;
            trailing_spaces = min(trailing_spaces, width_left);
            if(trailing_spaces > 0)
                {
                chars_printed += trailing_spaces;
                ospca(trailing_spaces);
                }
            dspfld_written = chars_printed;
            break;
            }
        elif(*ptr < SPACE)
            {
            if(trailing_spaces > 0)
                {
                chars_printed += trailing_spaces;
                ospca(trailing_spaces);
                trailing_spaces = 0;
                }

            /* output space or control char */
            --width_left;
            ++chars_printed;
            twzchr(*ptr++);
            }
        }

    #if !defined(SPELL_OFF)
    /* do we need to invert a particular word? */
    if(word_to_invert)
        {
        at(x + lecpos - lescrl, y);

        invert();
        stringout_underlaid(word_to_invert);
        invert();

        at(x + chars_printed, y);
        }
    #endif
    }

    return(dspfld_written);
}


/********************************************
*                                           *
*  position the cursor/caret on the screen  *
*                                           *
********************************************/

extern void
position_cursor(void)
{
    intl x, y;
#if RISCOS
    char paint_str[PAINT_STRSIZ];
    intl swidth_mp;
    BOOL force;

    tracef1("position_cursor(): force = %s; ", trace_boolstring(xf_acquirecaret));
#endif

    if(cbuff_offset)
        return;

    if(xf_inexpression)
        {
        x = EDTLIN_X0;
        y = EDTLIN_Y0;
        }
    else
        {
        x = calcad(curcoloffset);
        y = calrad(currowoffset);
        }

    #if RISCOS
    force = xf_acquirecaret;

    /* reset acquisition flag */
    xf_acquirecaret = FALSE;

    if(riscos_fonts  &&  !xf_inexpression)
        {
        /* fancy font positioning */

        if(lecpos > lescrl)
            {
            expand_current_slot_in_fonts(paint_str, TRUE, NULL);
            swidth_mp = font_width(paint_str);
            }
        else
            swidth_mp = 0;

        x = ch_to_os(x) + roundtofloor(swidth_mp, x_scale);     /* into OS */
        }
    else
    #endif
        {
        /* standard font positioning */

        x += (lecpos - lescrl < 0)
                    ? (intl) 0
                    : (intl) (lecpos - lescrl);

        x = min(x, pagwid_plus1);
        }

    #if RISCOS
    if(force  ||  (x != lastcursorpos_x)  ||  (y != lastcursorpos_y))
        {
        tracef2("caret moving to %d, %d\n", x, y);
        lastcursorpos_x = x;
        lastcursorpos_y = y;
        setcaretpos(x, y);
        }
    else
        tracef0("not moving caret\n");

    #elif MS || ARTHUR

    /* must always reposition text output cursor after drawing things */
    at(x, y);

    #endif
}


/************************
*                       *
* switch off highlights *
*                       *
************************/

extern void
switch_off_highlights(void)
{
    #if !defined(PRINT_OFF)
    if(prnbit)
        {
        prnout(EOS);
        return;
        }
    #endif

    #if MS
    d_colours[FORE].option &= ~BOLDMASK;
    d_colours[BACK].option &= ~BOLDMASK;
    #endif

    setcolour(FORE, BACK);

    highlights_on = 0;
}


/************************************************************************
*                                                                       *
* display highlights (inverse 1-8) and control characters (inverse @-?) *
*                                                                       *
************************************************************************/

extern void
twzchr(char ch)
{
    ch = (ch == DELETE)
                ?   '?'
                :
         ishighlight(ch)
                ?   ch + (FIRST_HIGHLIGHT_TEXT - FIRST_HIGHLIGHT)
                :   ch + ('A' - 1);

    #if MS || ARTHUR
    invert();
    wrch(ch);
    invert();
    #elif RISCOS
    riscos_invert();
    riscos_printchar(ch);
    riscos_invert();
    #endif
}


/************************************************
*                                               *
*  how big a field the given slot must plot in  *
*                                               *
************************************************/

extern intl
limited_fwidth_of_slot(slotp tslot, colt tcol, rowt trow)
{
    intl fwidth = chkolp(tslot, tcol, trow);    /* text slot contents never drawn beyond overlap */
    intl limit  = pagwid_plus1 - calcad(curcoloffset);

    assert(tcol == curcol);

    return(min(fwidth, limit));
}


static intl
limited_fwidth_of_slot_in_buffer(void)
{
    return(limited_fwidth_of_slot(travel_here(), curcol, currow));
}


/****************************************************************
*                                                               *
* get overlap width for slot taking account of column width     *
*                                                               *
* RJM, realises on 18.5.89 that when printing,                  *
* not all the columns are in horzvec. Hence the sqobits.        *
*                                                               *
****************************************************************/

extern intl
chkolp(slotp tslot, colt tcol, rowt trow)
{
    BOOL printing   = sqobit;
    intl totalwidth = colwidth(tcol);
    intl wrapwidth  = colwrapwidth(tcol);
    colt trycol;
    SCRCOL *cptr;

    vtracef5(TRACE_OVERLAP, "chkolp(&%p, %d, %d): totalwidth = %d, wrapwidth = %d\n",
            tslot, tcol, trow, totalwidth, wrapwidth);

    if(totalwidth >= wrapwidth)
        return(wrapwidth);

    if((curcol == tcol)  &&  (currow == trow)  &&  xf_inexpression)
        return(totalwidth);

    /* check to see if its a weirdo masquerading as a blank */
    if(tslot  &&  (tslot->type != SL_TEXT)  &&  isslotblank(tslot))
        return(totalwidth);

    if(printing)
        trycol = tcol;
    else
        {
        /* cptr := horzvec entry (+1) for trycol */
        cptr = horzvec();

        do  {
            if(cptr->flags & LAST)
                /* no more cols so return wrapwidth */
                return(wrapwidth);
            }
        while(cptr++->colno != tcol);
        }

    while(totalwidth < wrapwidth)
        {
        if(printing)
            /* when printing, next column is always trycol+1 */
            trycol++;
        else
            {
            /* get next column in horzvec */
            if(cptr->flags & LAST)
                return(wrapwidth);

            trycol = cptr++->colno;
            }

        if(trycol >= numcol)
            return(wrapwidth);

        /* can we overlap it? */
        #if TRACE_OVERLAP
        tracef2("travelling to %d, %d\n", trycol, trow);
        #endif
        tslot = travel(trycol, trow);
        if(tslot  &&  !isslotblank(tslot))
            break;

        if(!printing)
            /* don't overlap to current slot */
            if( ((trycol == curcol)  &&  (trow == currow))          ||
                /* don't allow overlap from non-marked to marked */
                (inblock(trycol, trow)  &&  !inblock(tcol, trow))
                )
                break;

        totalwidth += colwidth(trycol);
        }

    return(min(totalwidth, wrapwidth));
}


/***************************************************************
*                                                              *
* is a slot possibly overlapped on the screen                  *
*                                                              *
* tests whether there is a text slot on the screen to the left *
* with a big enough wrap width to overlap this slot            *
*                                                              *
***************************************************************/

extern BOOL
is_overlapped(intl coff, intl roff)
{
    rowt trow = row_number(roff);
    intl gap = 0;

    while(--coff >= 0)
        {
        colt  tcol  = col_number(coff);
        slotp tslot = travel(tcol, trow);

        gap += colwidth(tcol);

        if(tslot)
            {
            if(tslot->type != SL_TEXT)
                return(FALSE);

            return(colwrapwidth(tcol) >= gap);
            }
        }

    return(FALSE);
}


#if MS || ARTHUR

/********************************
*                               *
* draw bottom line of rectangle *
*                               *
********************************/

extern void
bottomline(intl xpos, intl ypos, intl xsize)
{
    at(xpos, ypos);
    wrch_funny(BOTLEFT);

    wrch_definefunny(HORIZBAR);
    wrchrep(HORIZBAR, xsize-2);

    wrch_funny(BOTRIGHT);
}


/****************************************
*                                       *
* draw a rectangle with a border in it  *
*                                       *
****************************************/

extern void
my_rectangle(intl xpos, intl ypos, intl xsize, intl ysize)
{
    intl i;

    topline(xpos, ypos, xsize);

    for(i = ypos + 1; i < ypos + ysize - 1; i++)
        {
        at(xpos, i);
        wrch_funny(VERTBAR);

        at(xpos + xsize - 1, i);
        wrch(VERTBAR);          /* ^^^ already defined */
        }

    bottomline(xpos, i, xsize);
}


/********************************
*                               *
*  draw top line of rectangle   *
*                               *
********************************/

extern void
topline(intl xpos, intl ypos, intl xsize)
{
    at(xpos, ypos);
    wrch_funny(TOPLEFT);

    wrch_definefunny(HORIZBAR);
    wrchrep(HORIZBAR, xsize-2);

    wrch_funny(TOPRIGHT);
}

#endif  /* MS || ARTHUR */

/* end of scdraw.c */
