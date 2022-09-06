/* riscdraw.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       riscdraw.c - some code for RISC OS text cell handling
 * Author:      Stuart K. Swales 15-Mar-1989
*/

/* standard header files */
#include "flags.h"


#if RISCOS
/* Module only compiled if RISCOS */

#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "wimpt.h"
#include "werr.h"
#include "font.h"
#include "print.h"
#include "colourtran.h"

#include "datafmt.h"

#include "ext.riscos"
#include "riscdraw.h"
#include "ext.pd"

extern void please_invert_textarea(intl tx0, intl ty0, intl tx1, intl ty1, intl fg, intl bg);
extern BOOL print_complain(os_error *err);


/* internal functions */

/*static void redraw_clear_area(riscos_redrawstr *r);*/
/*static void redraw_invert_area(riscos_redrawstr *r);*/

#define round_with_mask(value, mask) \
    { (value) = ((value) + (mask)) & ~(mask); }


/* ----------------------------------------------------------------------- */

#define TRACE_CLIP      (TRACE && FALSE)
#define TRACE_DRAW      (TRACE && TRUE)
#define TRACE_SETCOLOUR (TRACE && TRUE)


/*
* +-----------------------------------------------------------------------+
* |                                                                       |
* |     work area extent origin                                           |
* |       + --  --  --  --  --  --  --  --  --  -- --  --  --  --  --  -- |
* |             .                                                   ^     |
* |       | TLS          TMS                                        |     |
* |         .   +   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  |  .  |
* |       |     TCO                                                scy    |
* |                 +---+---+------------------------------+---+    |     |
* |       |     .   | B | C |     T i t l e    b a r       | T |    v     |
* |                 +---+---+------------------------------+---+ <---wy1  |
* |       |     .   |                                      | U |          |
* |                 |                                      +---+          |
* |       |     .   |                                      |   |          |
* |                 |                                      | = |          |
* |       |   L .   |                                      | # |          |
* |           M     |                                      | = |          |
* |       |   S .   |                                      |   |          |
* |                 |                                      |   |          |
* |       |     .   |                                      |   |          |
* |                 |                                      +---+          |
* |       |     .   |                                      | D |          |
* |                 +---+------------------------------+---+---+ <---wy0  |
* |       |     .   | L |      [-#-]                   | R | S |          |
* |                 +---+------------------------------+---+---+          |
* |       |<--scx-->^                                      ^              |
* |                 |                                      |              |
* |       |         |                                      |              |
* |                 wx0                                    wx1            |
* +-----------------------------------------------------------------------+
* ^
* graphics origin
*/


/*  a text cell
*   ===========
*
*        | <-------- charwidth --------> |
*        |                               |
*        |                           ->| |<- dx
*        |                             | |
*
*       +-.-|-.-|-.-|-.-|-.-|-.-|-.-|-.-+
*       |                               |
*       |               X   (9+dy)      |
*       | ----------------------------- |
*       |                               |   <- vpos for VDU 5 text plot
*       |               7   (+dy)       |
*       | ----------------------------- |
*       |                               |
*       |               6   (7+dy)      |   v
*       | ----------------------------- |   -
*       |                               |   - dy
*       |               5   (6+dy)      |   ^
*       | ----------------------------- |
*       |                               |
*       |               4   (5+dy)      |
*       | ----------------------------- |
*       |                               |
*       |               3   (4+dy)      |   v
*       | ----------------------------- |   -
*       |                               |
*       |               2   (3+dy)      |     v_dy
*       | ----------------------------- |   -
*       |                               |   ^
*       |               1   (2+dy)      |   <- system font baseline
*       | ----------------------------- |
*       |                               |
*       |               0   (1+dy)      |   <- notional cell yorg
*       + ----------------------------- +
*       |                               |
*       |               X   (0+dy)      |
*       | ----------------------------- |
*       |               X   (0)         |   <- notional cell yorg if grid
*       +-.-|-.-|-.-|-.-|-.-|-.-|-.-|-.-+
*
*        ^
*       cell xorg
*/


/* log2 bits per pixel */
intl log2bpp;


/* OS units per real pixel - needed for drawing correctly */

static intl dx;
       intl dy;
       intl dxm1;       /* dx - 1 for AND/BIC */
static intl dym1;
static intl two_dx;
static intl two_dy;
static intl cwmdx;      /* charwidth  - dx */
static intl chmdy;      /* charheight - dy */

/* a mode independent y spacing value */
#define v_dy 4

/* maximum size the contents of a window could be given in this screen mode */
static intl max_poss_height;
static intl max_poss_width;

/* limiting coordinates that the Window Manager should give us (if it were competent) */
#define     min_poss_x0     dx      /* always one pixel solid line to the left */
static intl max_poss_y1;

/* current fg/bg colours for this call of update/redraw (temp globals) */
intl current_fg = -1;
intl current_bg = -1;

static intl invert_fg;
static intl invert_bg;

static BOOL font_colours_invalid = TRUE;


/************************************************************
*                                                           *
* calculate org, textcell_org given the (x, y) position of  *
* the top left of the visible region                        *
*                                                           *
************************************************************/

static void
setorigins(intl x, intl y)
{
    int xorg = x + leftslop;        /* adjust for slop now */
    int yorg = y - topslop;

    tracef2("setting origin (%d, %d)\n", xorg, yorg);

    settextorigin(xorg, yorg);
}


extern void
killfontcolourcache(void)
{
    tracef0("killfontcolourcache()\n");

    font_colours_invalid = TRUE;
}


extern void
killcolourcache(void)
{
    tracef0("killcolourcache()\n");

    current_fg = current_bg = -1;

    font_colours_invalid = TRUE;
}


/********************************************************************
*                                                                   *
* screen redraw                                                     *
*                                                                   *
* called as many times as needed until all rectangles are repainted *
*                                                                   *
********************************************************************/

extern void
application_redraw(riscos_redrawstr *r)
{
    #if TRACE
    wimp_redrawstr *redrawstr = (wimp_redrawstr *) r;
    intl x0 = redrawstr->box.x0;
    intl y1 = redrawstr->box.y1;
    #endif

    IGNOREPARM(r);

    /* calculate 'text window' in document that needs painting */
    /* note the flip & xlate of y coordinates from graphics to text */
    cliparea.x0 = tcoord_x( graphics_window.x0);
    cliparea.x1 = tcoord_x1(graphics_window.x1);
    cliparea.y0 = tcoord_y( graphics_window.y0);
    cliparea.y1 = tcoord_y1(graphics_window.y1);

    tracef2("[app_redraw: x0 %d y1 %d]\n", x0, y1);
    tracef2(" textcell org %d, %d; ", textcell_xorg, textcell_yorg);
    tracef4(" graphics window %d, %d, %d, %d\n",
            graphics_window.x0, graphics_window.y0,
            graphics_window.x1, graphics_window.y1);
    tracef4(" invalid text area %d, %d, %d, %d\n",
            thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1);

    setcolour(FORE, BACK);

    /* ensure all of buffer displayed on redraw */
    dspfld_from = -1;

    maybe_draw_screen();

    wrch_undefinefunnies();     /* restore soft character definitions */
}


/*********************
*                    *
* swap fg/bg colours *
*                    *
*********************/

extern void
riscos_invert(void)
{
    int newbg = current_fg;
    int newfg = current_bg;

    current_fg = newfg;
    current_bg = newbg;

    vtracef1(TRACE_SETCOLOUR, "invert: wimp_setcolour(fg %d); ", newfg);
    vtracef1(TRACE_SETCOLOUR, "wimp_setcolour(bg %d)\n", newbg);

    wimpt_safe(wimp_setcolour(newfg       ));
    wimpt_safe(wimp_setcolour(newbg | 0x80));

    font_colours_invalid = TRUE;
}


/****************************************************************
*                                                               *
* Set document fg/bg colours iff different to those already set *
*                                                               *
****************************************************************/

static void
riscos_setbgcolour(int colour)
{
    if(colour != current_bg)
        {
        current_bg = colour;
        vtracef1(TRACE_SETCOLOUR, "wimp_setcolour(bg %d)\n", colour);
        wimpt_safe(wimp_setcolour(colour | 0x80));
        font_colours_invalid = TRUE;
        }
}

static void
riscos_setfgcolour(int colour)
{
    if(colour != current_fg)
        {
        current_fg = colour;
        vtracef1(TRACE_SETCOLOUR, "wimp_setcolour(fg %d)\n", colour);
        wimpt_safe(wimp_setcolour(colour));
        font_colours_invalid = TRUE;
        }
}

extern void
riscos_setcolour(int colour, BOOL isbackcolour)
{
    if(isbackcolour)
        riscos_setbgcolour(colour);
    else
        riscos_setfgcolour(colour);
}


extern void
riscos_setcolours(int bg, int fg)
{
    riscos_setbgcolour(bg);
    riscos_setfgcolour(fg);
}


/****************************************
*                                       *
* recache variables after a mode change *
*                                       *
****************************************/

extern void
cachemodevariables(void)
{
    dochandle doc = current_document_handle();
    window_data *wdp = NULL;
    intl xeig = bbc_vduvar(bbc_XEigFactor);
    intl yeig = bbc_vduvar(bbc_YEigFactor);
    intl x = (bbc_vduvar(bbc_XWindLimit) + 1) << xeig;
    intl y = (bbc_vduvar(bbc_YWindLimit) + 1) << yeig;

    screen_x_os = x;
    screen_y_os = y;

    log2bpp = bbc_vduvar(bbc_Log2BPP);

    /* OS units per pixel */
    dx      = (1 << xeig);
    dy      = (1 << yeig);
    dxm1    = dx - 1;
    dym1    = dy - 1;
    two_dx  = 2 * dx;
    two_dy  = 2 * dy;
    cwmdx   = charwidth  - dx;
    chmdy   = charheight - dy;
    tracef6("dx = %d, dy = %d, cwmdx = %d, chmdy = %d, xsize = %d, ysize = %d\n",
                dx, dy, cwmdx, chmdy, x, y);

    max_poss_width  = x - leftline_width - vscroll_width;
    max_poss_height = y - title_height - hscroll_height;
    tracef2("max poss height = %d, max poss width = %d\n",
                    max_poss_height, max_poss_width);

    max_poss_y1     = y - title_height;


    font_readscalefactor(&x_scale, &y_scale);
    tracef2("font x_scale = %d, font y_scale = %d\n", x_scale, y_scale);


    /* loop over documents setting new height */

    while((wdp = next_document(wdp)) != NULL)
        {
        select_document(wdp);
        new_grid_state();
        (void) new_window_height(windowheight());
#if 0
/* fix does not work due to our clever draw_screen() */
        /* may need to set caret up again on mode change (256 colour modes) */
        if(main_window == caret_window)
            xf_acquirecaret = xf_interrupted = TRUE;
#endif
        }

    select_document_using_handle(doc);
}


/* provide these for RISC_OSLib Draw code */
extern int wimpt_dx(void); extern int wimpt_dx(void) { return(dx); }
extern int wimpt_dy(void); extern int wimpt_dy(void) { return(dy); }


/********************************************
*                                           *
* recache variables after a palette change  *
*                                           *
********************************************/

static wimp_paletteword current_palette[16];

extern void
cachepalettevariables(void)
{
    intl i;
    wimp_palettestr p;

    colourtran_invalidate_cache();

    wimpt_safe(wimp_readpalette(&p));

    for(i = 0; i < 16; ++i)
        {
        current_palette[i] = p.c[i];
        tracef2("palette entry %d = &%8.8X\n", i, p.c[i]);
        }
}


extern intl
rgb_for_wimpcolour(intl wimpcolour)
{
    return(current_palette[wimpcolour].word & 0xFFFFFF00);
}


extern void
my_force_redraw(riscos_window w)
{
    wimp_redrawstr r;

    if(w != window_NULL)
        {
        r.w      = (wimp_w) w;
        r.box.x0 = -0x7FFF;
        r.box.y0 = -0x7FFF;
        r.box.x1 = +0x7FFF;
        r.box.y1 = +0x7FFF;

        wimpt_safe(wimp_force_redraw(&r));
        }
    else
        tracef0("NULL window handle\n");
}


/* merely set the vspace/vrubout variables */
/* does NOT force redraw or set new window height */

extern void
new_grid_state(void)
{
    charvspace = charheight + (grid_on ? 2*v_dy + dy : 0);

    /* grid is dy thick with one v_dy line above it and below it (at top of char),
     * system font is charheight high, but is plotted from dy below
    */
    vdu5textoffset = (charheight - dy) + (grid_on ? dy + v_dy : 0);

    /* if no grid, position font baseline v_dy above system font baseline */
    fontbaselineoffset = v_dy + (grid_on ? dy + v_dy : v_dy);

    charvrubout_pos = charvspace - (vdu5textoffset + dy);

    /* don't rubout grid line! */
    charvrubout_neg = vdu5textoffset - (grid_on ? dy : 0);

    tracef5("charvspace = %d, vdu5textoffset = %d, fontbaselineoffset = %d, charvrubout_pos = %d, charvrubout_neg = %d\n",
            charvspace, vdu5textoffset, fontbaselineoffset, charvrubout_pos, charvrubout_neg);
}


/************************************************
*                                               *
*  how many whole text cells fit in the window  *
*                                               *
************************************************/

extern coord
windowheight(void)
{
    wimp_wstate s;
    intl os_height;
    intl height = 3; /* hmm */
    intl min_height = calrad(3);        /* else rebols & friends explode */
    BOOL err;

    err = (main_window == window_NULL);

    if(!err)
        err = (BOOL) wimpt_safe(wimp_get_wind_state(main__window, &s));

    if(!err)
        os_height = (s.o.box.y1 - topslop) - s.o.box.y0;
    else
        os_height = height * charvspace;

    height = os_height / charvspace;

    unused_bit_at_bottom = (height * charvspace != os_height);

    height = max(height, min_height);

    tracef3("windowheight is %d os %d text, ubb = %s\n",
            os_height, height, trace_boolstring(unused_bit_at_bottom));
    return(height);
}


/* else PipeDream may explode - try it one day */
#define MAX_WIDTH 255

extern coord
windowwidth(void)
{
    wimp_wstate s;
    intl width;
    BOOL err;

    err = (main_window == window_NULL);

    if(!err)
        err = (BOOL) wimpt_safe(wimp_get_wind_state(main__window, &s));

    if(!err)
        width = (s.o.box.x1 - (s.o.box.x0 + leftslop)) / charwidth;
    else
        width = BORDERWIDTH + 4;

    #ifndef NOLIMIT
    width = min(width, MAX_WIDTH);
    #endif

    tracef1("windowwidth is %d\n", width);
    return(width);
}


/********************************************
*                                           *
* position graphics cursor for text output  *
*                                           *
********************************************/

extern void
at(intl tx, intl ty)
{
    vtracef2(TRACE_DRAW, "at(%d, %d)\n", tx, ty);

    wimpt_safe(bbc_move(gcoord_x(tx), gcoord_y_textout(ty)));
}


extern void
at_fonts(intl x, intl ty)
{
    if(riscos_fonts)
        {
        vtracef3(TRACE_DRAW, "at_fonts(%d (%d), %d)\n", x, textcell_xorg + x, ty);

        wimpt_safe(bbc_move(textcell_xorg + x, gcoord_y_textout(ty)));
        }
    else
        at(x, ty);
}


/****************************************
*                                       *
*  clear out specified text area to bg  *
*  graphics cursor spastic at end       *
*                                       *
****************************************/

extern void
clear_textarea(intl tx0, intl ty0, intl tx1, intl ty1, BOOL zap_grid)
{
    intl x0, y0, x1, y1;

    tracef4("clear_textarea(%d, %d, %d, %d)\n", tx0, ty0, tx1, ty1);

    if((tx0 != tx1)  &&  ((ty0 != ty1) || zap_grid))
        {
        x0 = gcoord_x(tx0);
        y0 = gcoord_y(ty0);
        x1 = gcoord_x(tx1) - dx;
        y1 = gcoord_y(ty1) - (zap_grid ? 0 : dy);

        /* limit coordinates for RISC OS */
        x0 = max(x0, SHRT_MIN);
        y0 = max(y0, SHRT_MIN);
        x1 = min(x1, SHRT_MAX);
        y1 = min(y1, SHRT_MAX);

        wimpt_safe(bbc_move(x0, y0));
        wimpt_safe(bbc_plot(bbc_RectangleFill + bbc_DrawAbsBack, x1, y1));
        }
}


/************************************
*                                   *
*  clear out thisarea (text) to bg  *
*  graphics cursor spastic at end   *
*                                   *
************************************/

extern void
clear_thistextarea(void)
{
    clear_textarea(thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1, FALSE);
}


/****************************************************
*                                                   *
* clear the underlay for a subsequent string print  *
* graphics cursor restored to current text cell     *
*                                                   *
****************************************************/

extern void
clear_underlay(intl len)
{
    vtracef1(TRACE_DRAW, "clear_underlay(%d)\n", len);

    if(len > 0)
        {
        intl x = len * charwidth - dx;
        intl ypos = charvrubout_pos;
        intl yneg = charvrubout_neg;

        if(ypos)
            wimpt_safe(bbc_plot(bbc_MoveCursorRel,               0, +ypos));

        wimpt_safe(bbc_plot(bbc_RectangleFill + bbc_DrawRelBack, x, -ypos -yneg));
        wimpt_safe(bbc_plot(bbc_MoveCursorRel,                  -x,       +yneg));
        }
}


/************************************************
*                                               *
* conversions from text cell coordinates to     *
* absolute graphics coordinates for output      *
*                                               *
* returns bottom left corner of text cell       *
*                                               *
* NB. text output requires a further correction *
*  of +charfudgey as VDU 5 plotting is spastic  *
*                                               *
************************************************/

extern intl
gcoord_x(intl x)
{
    return(textcell_xorg + x * charwidth);
}


#define gc_y(y) (textcell_yorg - (y+1) * charvspace)

extern intl
gcoord_y(intl y)
{
    return(gc_y(y));
}


extern intl
gcoord_y_fontout(intl y)
{
    return(gc_y(y) + fontbaselineoffset);
}


extern intl
gcoord_y_textout(intl y)
{
    return(gc_y(y) + vdu5textoffset);
}


/********************
*                   *
*  print n spaces   *
*                   *
********************/

extern void
ospca(intl nspaces)
{
    if(nspaces > 0)
        {
        #if !defined(PRINT_OFF)
        if(sqobit)
            {
            if(riscos_printing)
                riscos_movespaces(nspaces);
            else
                wrchrep(SPACE, nspaces);
            return;
            }
        #endif

        riscos_printspaces(nspaces);
        }
}


/* needn't worry about printing */

extern void
ospca_fonts(intl nspaces)
{
    (riscos_fonts ? riscos_printspaces_fonts : ospca) (nspaces);
}


/********************************************************
*                                                       *
* request that an area of text cells be cleared to bg   *
*                                                       *
********************************************************/

static void
redraw_clear_area(riscos_redrawstr *r)
{
    tracef4("redraw_cleararea: graphics window %d, %d, %d, %d\n",
            graphics_window.x0, graphics_window.y0,
            graphics_window.x1, graphics_window.y1);

    IGNOREPARM(r);

    setbgcolour(BACK);
    wimpt_safe(bbc_clg());
}


extern void
please_clear_textarea(intl tx0, intl ty0, intl tx1, intl ty1)
{
    tracef4("please_clear_textarea(%d, %d, %d, %d)\n", tx0, ty0, tx1, ty1);

    if(TRACE  &&  trace_enabled  &&  ((tx0 > tx1)  ||  (ty0 < ty1)))
        werr_fatal("please_clear_textarea(%d, %d, %d, %d) is stupid",
                    tx0, ty0, tx1, ty1);

    riscos_updatearea(  redraw_clear_area, main_window,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}


/****************************************************
*                                                   *
*  request that an area of text cells be inverted   *
*                                                   *
****************************************************/

static void
redraw_invert_area(riscos_redrawstr *r)
{
    intl invertEORcolour = getcolour(invert_fg) ^ getcolour(invert_bg);

    tracef5("redraw_invert_area: graphics window (%d, %d, %d, %d), EOR %d\n",
            graphics_window.x0, graphics_window.y0,
            graphics_window.x1, graphics_window.y1, invertEORcolour);

    IGNOREPARM(r);

    wimpt_safe(bbc_gcol(3, 0x80 | invertEORcolour));
    wimpt_safe(bbc_clg());
}


extern void
please_invert_textarea(intl tx0, intl ty0, intl tx1, intl ty1, intl fg, intl bg)
{
    tracef6("please_invert_textarea(%d, %d, %d, %d) fg = %d, bg = %d\n",
                tx0, ty0, tx1, ty1, fg, bg);

    if(TRACE  &&  trace_enabled  &&  ((tx0 > tx1)  ||  (ty0 < ty1)))
        werr_fatal("please_invert_textarea(%d, %d, %d, %d) is stupid",
                    tx0, ty0, tx1, ty1);

    invert_fg = fg;
    invert_bg = bg;

    riscos_updatearea(  redraw_invert_area, main_window,
                        texttooffset_x(tx0),
                        /* invert grid hspace, not hbar */
                        texttooffset_y(ty0) + ((grid_on) ? dy : 0),
                        /* don't invert grid vbar */
                        texttooffset_x(tx1) - ((grid_on) ? dx : 0),
                        texttooffset_y(ty1));
}


extern void
please_invert_numeric_slot(intl coff, intl roff, intl fg, intl bg)
{
    intl tx0 = calcad(coff);
    intl tx1 = tx0 + colwidth(col_number(coff));
    intl ty0 = calrad(roff);
    intl ty1 = ty0 - 1;

    please_invert_textarea(tx0, ty0, tx1, ty1, fg, bg);
}


/* invert a set of slots, taking care with the grid */

extern void
please_invert_numeric_slots(intl start_coff, intl end_coff, intl roff, intl fg, intl bg)
{
    intl tx0, tx1, ty0, ty1;

    if(grid_on)
        while(start_coff <= end_coff)
            please_invert_numeric_slot(start_coff++, roff, fg, bg);
    else
        {
        tx0 = calcad(start_coff);
        tx1 = tx0;
        ty0 = calrad(roff);
        ty1 = ty0 - 1;

        while(start_coff <= end_coff)
            tx1 += colwidth(col_number(start_coff++));

        please_invert_textarea(tx0, ty0, tx1, ty1, fg, bg);
        }
}


/************************************************
*                                               *
* request that an area of text cells be redrawn *
*                                               *
************************************************/

extern void
please_redraw_textarea(intl tx0, intl ty0, intl tx1, intl ty1)
{
    tracef4("please_redraw_textarea(%d, %d, %d, %d)\n", tx0, ty0, tx1, ty1);

    if(TRACE  &&  trace_enabled  &&  ((tx0 > tx1)  ||  (ty0 < ty1)))
        werr_fatal("please_redraw_textarea(%d, %d, %d, %d) is stupid",
                    tx0, ty0, tx1, ty1);

    riscos_updatearea(  application_redraw, main_window,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}


extern void
please_redraw_textline(intl tx0, intl ty0, intl tx1)
{
    please_redraw_textarea(tx0, ty0, tx1, ty0 - 1);
}


extern void
please_redraw_entire_window(void)
{
    /* must draw left & top slops too */
    please_redraw_textarea(-1, paghyt + 2, RHS_X, -1);
}


/************************************************************************
*                                                                       *
* request that an area of text cells be redrawn by a given procedure    *
*                                                                       *
************************************************************************/

extern void
please_update_textarea(riscos_redrawproc proc,
                        intl tx0, intl ty0, intl tx1, intl ty1)
{
    tracef4("please_update_textarea(%d, %d, %d, %d)\n", tx0, ty0, tx1, ty1);

    if(TRACE  &&  trace_enabled  &&  ((tx0 > tx1)  ||  (ty0 < ty1)))
        werr_fatal("please_update_textarea(%d, %d, %d, %d) is stupid",
                    tx0, ty0, tx1, ty1);

    riscos_updatearea(  proc, main_window,
                        texttooffset_x(tx0),
                        /* include grid hspace & hbar */
                        texttooffset_y(ty0),
                        texttooffset_x(tx1),
                        texttooffset_y(ty1));
}


extern void
please_update_thistextarea(riscos_redrawproc proc)
{
    please_update_textarea(proc, thisarea.x0, thisarea.y0, thisarea.x1, thisarea.y1);
}


/********************************************************
*                                                       *
* set a graphics window to cover the intersection       *
* of the passed graphics window and the desired window  *
*                                                       *
* returns TRUE if some part of window visible           *
*                                                       *
********************************************************/

extern BOOL
set_graphics_window_from_textarea(intl tx0, intl ty0, intl tx1, intl ty1, BOOL set_gw)
{
    intl x0 = gcoord_x(tx0);
    intl y0 = gcoord_y(ty0);
    intl x1 = gcoord_x(tx1);
    intl y1 = gcoord_y(ty1);

    tracef4("set_gw_from_textarea(%d, %d, %d, %d)\n",
            tx0, ty0, tx1, ty1);
    tracef4("textarea  (%d, %d, %d, %d) (OS)\n",
            x0, y0, x1, y1);
    tracef4("window gw (%d, %d, %d, %d) (OS)\n",
            graphics_window.x0, graphics_window.y0, graphics_window.x1, graphics_window.y1);

    x0 = max(x0, graphics_window.x0);
    y0 = max(y0, graphics_window.y0);
    x1 = min(x1, graphics_window.x1);
    y1 = min(y1, graphics_window.y1);

    tracef4("intersection (%d, %d, %d, %d) (OS)\n", x0, y0, x1, y1);

    if((x0 >= x1)  ||  (y0 >= y1))
        {
        tracef0("zero size window requested - OS incapable\n");
        return(FALSE);
        }

    if(set_gw)
        {
        /* when setting graphics window, all points are inclusive */
        x1 -= dx;
        y1 -= dy;

        /* limit coordinates for RISC OS */
        x0 = max(x0, SHRT_MIN);
        y0 = max(y0, SHRT_MIN);
        x1 = min(x1, SHRT_MAX);
        y1 = min(y1, SHRT_MAX);

        tracef4("setting gw (%d, %d, %d, %d) (OS)\n", x0, y0, x1, y1);

        wimpt_safe(bbc_gwindow(x0, y0, x1, y1));
        }

    return(TRUE);
}


extern void
restore_graphics_window(void)
{
    wimpt_safe(bbc_gwindow(graphics_window.x0,      graphics_window.y0,
                           graphics_window.x1 - dx, graphics_window.y1 - dy));
}


/********************************************************
*                                                       *
*  round to ± infinity at multiples of a given number   *
*                                                       *
********************************************************/

extern intl
roundtoceil(intl a, intl b)
{
    return( ((a <= 0) ? a : (a + (b - 1))) / b );
}


extern intl
roundtofloor(intl a, intl b)
{
    return( ((a >= 0) ? a : (a - (b - 1))) / b );
}


/* I hate this stupid language ... */

static intl
muldiv(intl a, intl b, intl c)
{
    tracef3("muldiv(%d, %d, %d)\n", a, b, c);

    return((intl) (((double) a * (double) b) / (double) c));
}


/****************************************
*                                       *
* scroll the given area down by n lines *
* as if it were a real text window      *
*                                       *
****************************************/

extern void
scroll_textarea(intl tx0, intl ty0, intl tx1, intl ty1, intl nlines)
{
    wimp_box box;
    intl y, uy0, uy1, ht;

    vtracef5(TRACE, "scroll_textarea((%d, %d, %d, %d), %d)\n",
                            tx0, ty0, tx1, ty1, nlines);

    if(TRACE  &&  trace_enabled  &&  ((tx0 > tx1)  ||  (ty0 < ty1)))
        werr_fatal("scroll_textarea((%d, %d, %d, %d), %d) is stupid",
                        tx0, ty0, tx1, ty1, nlines);

    if(nlines != 0)
        {
        box.x0 = texttooffset_x(tx0);
        box.x1 = texttooffset_x(tx1+1);
        ht     = nlines * charvspace;

        if(ht > 0)
            {
            /* scrolling area down, clear top line(s) */
            box.y0 = texttooffset_y(ty0) + ht;
            box.y1 = texttooffset_y(ty1-1);
            y      = box.y0 - ht;
            uy0    = box.y1 - ht;
            uy1    = box.y1;
            }
        else
            {
            /* scrolling area up, clear bottom line(s) */
            box.y0 = texttooffset_y(ty0);
            box.y1 = texttooffset_y(ty1-1) + ht;
            y      = box.y0 - ht;
            uy0    = box.y0;
            uy1    = y;
            }

        riscos_removecaret();

        tracef6("wimp_blockcopy((%d, %d, %d, %d), %d, %d)\n",
                            box.x0, box.y0, box.x1, box.y1, box.x0, y);

        wimpt_safe(wimp_blockcopy(main__window, &box, box.x0, y));

        /* get our clear lines routine called */
        riscos_updatearea(redraw_clear_area, main_window,
                          box.x0, uy0, box.x1, uy1);

        riscos_restorecaret();
        }
}


/********************************************
*                                           *
* set absolute position of text cell origin *
*                                           *
********************************************/

extern void
settextorigin(int x, int y)
{
    tracef2("settextorigin(%d, %d)\n", x, y);

    textcell_xorg = x;
    textcell_yorg = y;
}


/****************************************************
*                                                   *
* conversions from absolute graphics coordinates    *
* to text cell coordinates for input                *
* (and their friends for calculating cliparea)      *
*                                                   *
* returns bottom left corner of text cell           *
*                                                   *
****************************************************/

extern intl
tsize_x(intl x)
{
    return(roundtoceil(x, charwidth));
}


extern intl
tsize_y(intl y)
{
    return(roundtoceil(y, charvspace));
}


extern intl
tcoord_x(intl x)
{
    return(roundtofloor(x - textcell_xorg, charwidth));
}


extern intl
tcoord_y(intl y)
{
    return(roundtoceil(textcell_yorg - y, charvspace) - 1);
}


extern intl
tcoord_x_remainder(intl x)
{
    intl tx = tcoord_x(x);

    return((x - textcell_xorg) - tx * charwidth);
}


extern intl
tcoord_x1(intl x)
{
    return(roundtoceil(x - textcell_xorg, charwidth));
}


extern intl
tcoord_y1(intl y)
{
    return(roundtofloor(textcell_yorg - y, charvspace) - 1);
}


/************************************************************
*                                                           *
* work out whether an text object intersects the cliparea   *
* if it does, stick the given coordinates in thisarea       *
*                                                           *
************************************************************/

extern BOOL
textobjectintersects(intl x0, intl y0, intl x1, intl y1)
{
    BOOL intersects = !((cliparea.x0 >=          x1)    ||
                        (         y1 >= cliparea.y0)    ||
                        (         x0 >= cliparea.x1)    ||
                        (cliparea.y1 >=          y0)    );

    vtracef4(TRACE_CLIP, "textobjectintersects: %d %d %d %d (if any true, fails)\n",
             x0, y0, x1, y1);
    vtracef1(TRACE_CLIP, "x0 >= cliparea.x1 %s, ", trace_boolstring(x0 >= cliparea.x1));
    vtracef1(TRACE_CLIP, "x1 <= cliparea.x0 %s, ", trace_boolstring(x1 <= cliparea.x0));
    vtracef1(TRACE_CLIP, "y0 <= cliparea.y1 %s, ", trace_boolstring(y0 <= cliparea.y1));
    vtracef1(TRACE_CLIP, "y1 >= cliparea.y0 %s\n", trace_boolstring(y1 >= cliparea.y0));

    if(intersects)
        {
        thisarea.x0 = x0;
        thisarea.y0 = y0;
        thisarea.x1 = x1;
        thisarea.y1 = y1;
        }

    return(intersects);
}


/************************************************************
*                                                           *
* work out whether an text x pair intersects the cliparea   *
*                                                           *
************************************************************/

extern BOOL
textxintersects(intl x0, intl x1)
{
    BOOL intersects = !((cliparea.x0 >=          x1)    ||
                        (         x0 >= cliparea.x1)    );

    vtracef2(TRACE_CLIP, "textxintersects: %d %d %d %d (if any true, fails)\n",
             x0, x1);
    vtracef1(TRACE_CLIP, "x0 >= cliparea.x1 %s, ", trace_boolstring(x0 >= cliparea.x1));
    vtracef1(TRACE_CLIP, "x1 <= cliparea.x0 %s, ", trace_boolstring(x1 <= cliparea.x0));

    if(intersects)
        {
        thisarea.x0 = x0;
        thisarea.x1 = x1;
        }

    return(intersects);
}


/************************************************
*                                               *
* return bottom left corner of text cell        *
* relative to the real work area extent origin  *
* useful for setting caret position etc.        *
*                                               *
************************************************/

extern intl
texttooffset_x(intl x)
{
    return((/*curr_scx*/ + leftslop) + x * charwidth);
}


extern intl
texttooffset_y(intl y)
{
    return((/*curr_scy*/ - topslop) - (y+1) * charvspace);
}


/********************************************************
*                                                       *
* move graphics cursor along by n spaces                *
* assumes we are currently text cell printing aligned   *
*                                                       *
********************************************************/

extern void
riscos_movespaces(intl nspaces)
{
    vtracef1(TRACE_DRAW, "riscos_movespaces(%d)\n", nspaces);

    if(nspaces != 0)        /* -ve allowed */
        wimpt_safe(bbc_plot(bbc_MoveCursorRel, nspaces * charwidth, 0));
}


extern void
riscos_movespaces_fonts(intl nspaces)
{
    vtracef1(TRACE_DRAW, "riscos_movespaces_fonts(%d)\n", nspaces);

    if(nspaces != 0)        /* -ve allowed */
        wimpt_safe(bbc_plot(bbc_MoveCursorRel,
                            riscos_fonts ? nspaces : nspaces * charwidth, 0));
}


/********************************************************
*                                                       *
* clear out n spaces to background                      *
* assumes we are currently text cell printing aligned   *
*                                                       *
* --out--                                               *
*   graphics cursor advanced to next text cell          *
*                                                       *
********************************************************/

extern void
riscos_printspaces(intl nspaces)
{
    vtracef1(TRACE_DRAW, "riscos_printspaces(%d)\n", nspaces);

    if(nspaces != 0)        /* -ve allowed */
        {
        intl ldx = dx;
        intl x = nspaces * charwidth - ldx;
        intl ypos = charvrubout_pos;
        intl yneg = charvrubout_neg;

        if(ypos)
            wimpt_safe(bbc_plot(bbc_MoveCursorRel,                0, +ypos));

        wimpt_safe(bbc_plot(bbc_RectangleFill + bbc_DrawRelBack,  x, -ypos  -yneg));
        wimpt_safe(bbc_plot(bbc_MoveCursorRel,                  ldx,        +yneg));
        }
}


extern void
riscos_printspaces_fonts(intl nspaces)
{
    vtracef1(TRACE_DRAW, "riscos_printspaces_fonts(%d)\n", nspaces);

    if(nspaces != 0)        /* -ve allowed */
        {
        intl ldx = dx;
        intl x = (riscos_fonts ? nspaces : nspaces * charwidth) - ldx;
        intl ypos = charvrubout_pos;
        intl yneg = charvrubout_neg;

        if(ypos)
            wimpt_safe(bbc_plot(bbc_MoveCursorRel,                0, +ypos));

        wimpt_safe(bbc_plot(bbc_RectangleFill + bbc_DrawRelBack,  x, -ypos  -yneg));
        wimpt_safe(bbc_plot(bbc_MoveCursorRel,                  ldx,        +yneg));
        }
}


/********************************************************
*                                                       *
* print character after explicitly filling background   *
* assumes we are currently text cell printing aligned   *
*                                                       *
* --out--                                               *
*   graphics cursor advanced to next text cell          *
*                                                       *
********************************************************/

extern void
riscos_printchar(intl ch)
{
    intl x = cwmdx;                         /* avoids reload over procs */
    intl ypos = charvrubout_pos;
    intl yneg = charvrubout_neg;

    vtracef1(TRACE_DRAW, "riscos_printchar(%d)\n", ch);

    if(ypos)
        wimpt_safe(bbc_plot(bbc_MoveCursorRel,               0, +ypos));

    wimpt_safe(bbc_plot(bbc_RectangleFill + bbc_DrawRelBack, x, -ypos -yneg));
    wimpt_safe(bbc_plot(bbc_MoveCursorRel,                  -x,       +yneg));

    wimpt_safe(bbc_vdu(ch));
}


/* ----------------------------------------------------------------------- */

/********************************************
*                                           *
* scroll requests come through when the     *
* user clicks on the scroll icons or in the *
* the scroll bar (not in the sausage).      *
*                                           *
********************************************/

extern void
application_scroll_request(wimp_eventstr *e)
{
    intl xdir = e->data.scroll.x;   /* -1 for left, +1 for right */
    intl ydir = e->data.scroll.y;   /* -1 for down, +1 for up    */
                                    /*     ±2 for page scroll    */

    tracef2("app_scroll_request: xdir %d ydir %d\n", xdir, ydir);

      if(ydir ==  2)
        application_process_command(N_PageUp);
    elif(ydir ==  1)
        application_process_command(N_ScrollUp);
    elif(ydir == -1)
        application_process_command(N_ScrollDown);
    elif(ydir == -2)
        application_process_command(N_PageDown);

      if(xdir ==  2)
        application_process_command(N_LastColumn);
    elif(xdir ==  1)
        application_process_command(N_ScrollRight);
    elif(xdir == -1)
        application_process_command(N_ScrollLeft);
    elif(xdir == -2)
        application_process_command(N_FirstColumn);
}


/************************************************
*                                               *
* compute x scroll offset given current extent  *
*                                               *
************************************************/

static intl
cols_for_extent(void)
{
    intl res = (intl) numcol;
    colt tcol;

    for(tcol = 0; tcol < numcol; ++tcol)
        if(!colwidth(tcol))
            --res;

    return(res ? res : 1);
}


static intl
compute_scx(void)
{
    colt ffc    = fstncx();
    colt nfixes = n_colfixes;
    intl scx;

    if(nfixes)
        if(ffc >= col_number(0))
            ffc -= nfixes;

    scx = muldiv(curr_xext, (intl) ffc, cols_for_extent());

    /* scroll offsets must be rounded to pixels so as not to confuse Neil */
    round_with_mask(scx, dxm1);

    tracef1("computed scx %d (OS)\n", scx);
    return(scx);
}


/************************************************
*                                               *
* compute y scroll offset given current extent  *
*                                               *
************************************************/

static intl
rows_for_extent(void)
{
    return((intl) numrow + 1);
}


static intl
compute_scy(void)
{
    rowt ffr    = fstnrx();
    rowt nfixes = n_rowfixes;
    intl scy;

    if(nfixes)
        if(ffr >= row_number(0))
            ffr -= nfixes;

    scy = muldiv(curr_yext, (intl) ffr, rows_for_extent()); /* -ve, +ve, +ve */

    /* scroll offsets must be rounded to pixels so as not to confuse Neil */
    round_with_mask(scy, dym1);

    tracef1("computed scy %d (OS)\n", scy);
    return(scy);
}


static os_error *
openpane(wimp_box *boxp, wimp_w behind)
{
    wimp_openstr o;

    tracef6("opening pane %d; x0 %d, x1 %d;  y0 %d, y1 %d; behind %d\n",
            main__window, boxp->x0, boxp->x1, boxp->y0, boxp->y1, behind);

    o.w     = main__window;
    o.box   = *boxp;        /* pane completely covers fake window */
    o.scx   = 0;
    o.scy   = 0;
    o.behind = behind;

    return(wimpt_complain(wimp_open_wind(&o)));
}


/********************************************************************
*                                                                   *
* we are being asked to open our main window: if certain things are *
* changed we must do all sorts of reshuffling                       *
*                                                                   *
********************************************************************/

extern void
application_open_request(wimp_eventstr *e)
{
    wimp_openstr *op            = &e->data.o;
    intl    scx                 = op->scx;
    intl    scy                 = op->scy;
    wimp_w  behind              = op->behind;
    intl    x0, x1, y0, y1;
    intl    osheight, oswidth;
    BOOL    old_unused_bit      = unused_bit_at_bottom;
    /* note knowledge of height <-> pagvars conversion */
    intl    old_window_height   = paghyt + 1;
    intl    old_window_width    = pagwid_plus1;
    intl    window_height;
    intl    window_width;

    tracef2("\n\n*** app_open_request: w %d, behind %d\n", op->w, behind);
    tracef6("                : x0 %d, x1 %d;  y0 %d, y1 %d; scx %d, scy %d\n",
            op->box.x0, op->box.x1, op->box.y0, op->box.y1, scx, scy);

    /* have to tweak requested coordinates as we have a wierd extent */
    if( op->box.x0 < min_poss_x0)
        op->box.x0 = min_poss_x0;

    if( op->box.x1 > op->box.x0 + max_poss_width)
        op->box.x1 = op->box.x0 + max_poss_width;

    if( op->box.y1 > max_poss_y1)
        op->box.y1 = max_poss_y1;

    if( op->box.y0 < op->box.y1 - max_poss_height)
        op->box.y0 = op->box.y1 - max_poss_height;

    if(behind != -2)
        {
        /* on mode change, Neil tells us to open windows behind themselves! */
        if(behind == fake__window)
            behind = main__window;

        /* always open pane first */
        openpane(&op->box, behind);

        /* always open fake window behind pane window */
        op->behind = main__window;
        }

    wimpt_complain(wimp_open_wind(op));

    /* reopen pane with corrected coords */
    wimpt_safe(wimp_get_wind_state(fake__window, (wimp_wstate *) op));

    /* if fake window was sent to the back, open pane behind the window that
     * the fake window ended up behind
    */
    if(behind == -2)
        behind = op->behind;

    openpane(&op->box, behind);

    x0 = op->box.x0;                /* absolute gcoords */
    x1 = op->box.x1;
    y0 = op->box.y0;
    y1 = op->box.y1;

    osheight = y1 - y0;
    oswidth  = x1 - x0;

    tracef6("opened window at: x0 %d, x1 %d;  y0 %d, y1 %d; width %d (OS) height %d (OS)\n",
            x0, x1, y0, y1, oswidth, osheight);

    /* note absolute coordinates of window (NOT work area extent) origin */
    setorigins(x0, y1);

    window_height = windowheight();
    window_width  = windowwidth();

    /* suss resize after opening as then we know how big we really are */
    if(old_window_height != window_height)
        (void) new_window_height(window_height);

    if(old_window_width != window_width)
        (void) new_window_width(window_width);


    /* is someone trying to scroll through the document
     * by dragging the scroll bars?
    */
    if(scx != curr_scx)
        {
        /* work out which column to put at left */
        colt leftcol, delta, newcurcol;
        colt o_leftcol  = fstncx();
        colt nfixes     = n_colfixes;

        tracef3("horizontal scroll bar dragged: scx %d, curr_scx %d, curr_xext %d\n",
                scx, curr_scx, curr_xext);

        leftcol = (colt) muldiv(scx, cols_for_extent(), curr_xext);

        if(nfixes)
            if(leftcol >= col_number(0))
                leftcol += nfixes;

        if( leftcol > numcol - 1)
            leftcol = numcol - 1;

        tracef1("put col %d at left of scrolling area\n", leftcol);

        if(leftcol != o_leftcol)
            {
            /* window motion may reposition caret */
            (void) mergebuf();

            if(curcoloffset < nfixes)
                /* caret is in fixed section - do not move */
                delta = curcol - leftcol;
            else
                {
                /* keep caret at same/similar offset to right of fixed section */
                delta = curcol - o_leftcol;

                if( delta > numcol - 1 - leftcol)
                    delta = numcol - 1 - leftcol;
                }

            newcurcol = leftcol + delta;

            filhorz(leftcol, newcurcol);

            if(curcol != newcurcol)
                chknlr(newcurcol, currow);

            out_screen = TRUE;
            }

        /* note the scroll offset we opened at and the difference
         * between what we did open at and the scroll offset that
         * we would set given a free hand in the matter.
        */
        curr_scx    = scx;
        delta_scx   = scx - compute_scx();
        tracef1("delta_scx := %d\n", delta_scx);
        }


    if(scy != curr_scy)
        {
        /* work out which row to put at top */
        rowt toprow, delta, newcurrow;
        rowt o_toprow   = fstnrx();
        rowt nfixes     = n_rowfixes;

        tracef3("vertical scroll bar dragged: scy %d, curr_scy %d, curr_yext %d\n",
                scy, curr_scy, curr_yext);

        toprow = (rowt) muldiv(rows_for_extent(), scy, curr_yext);  /* +ve, -ve, -ve */

        if(nfixes)
            if(toprow >= row_number(0))
                toprow += nfixes;

        tracef3("row %d is my first guess, numrow %d, rows_available %d\n", toprow, numrow, rows_available);

        /* eg. toprow = 42, numrow = 72, rows_available = 30 triggers fudge of 6 */

        if(!nfixes  &&  (toprow >= (numrow - 1) - ((rowt) rows_available - 1))  &&  encpln)
            {
            tracef0("put last row at bottom of scrolling area\n");

            toprow += (rowt) rows_available / ((rowt) encpln + 1);
            }

        tracef1("put row %d at top of scrolling area\n", toprow);

        if( toprow > numrow - 1)
            toprow = numrow - 1;

        if(toprow != o_toprow)
            {
            /* window motion may reposition caret */
            (void) mergebuf();

            if(currowoffset < nfixes)
                /* caret is in fixed section - do not move */
                delta = currow - toprow;
            else
                {
                /* keep caret at same/similar offset below fixed section */
                delta = currow - o_toprow;

                if( delta > numrow - 1 - toprow)
                    delta = numrow - 1 - toprow;
                }

            newcurrow = toprow + delta;

            filvert(toprow, newcurrow, TRUE);

            if(newcurrow != currow)
                chknlr(curcol, newcurrow);

            out_below = TRUE;
            rowtoend = schrsc(fstnrx());
            }

        /* note the scroll offset we opened at and the difference
         * between what we did open at and the scroll offset that
         * we would set given a free hand in the matter.
        */
        curr_scy    = scy;
        delta_scy   = scy - compute_scy();
        tracef1("delta_scy := %d\n", delta_scy);
        }


    if(window_height != old_window_height)
        {
        BOOL smash = FALSE;
        intl smash_y0;

        /* window motion may reposition caret */
        (void) mergebuf();

        if(window_height > old_window_height)
            {
            /* Window Manager assumes when making window bigger that
             * we had been clipping to the window itself and not
             * just some subwindow like we do, so add the old unused
             * rectangle to its list that it'll give us next.
            */
            if(old_unused_bit)
                {
                smash    = TRUE;
                smash_y0 = old_window_height;
                }
            }
        elif(unused_bit_at_bottom)
            {
            /* When making window smaller it assumes that
             * we will clip to the window itself and not
             * just some subwindow like we do, so add the new unused
             * rectangle to its list that it'll give us next.
            */
            smash    = TRUE;
            smash_y0 = window_height;
            }

        if(smash)
            {
            wimp_redrawstr r;
            r.w      = main__window;
            r.box.x0 = texttooffset_x(-1);              /* lhs slop too */
            r.box.x1 = texttooffset_x(window_width+1);  /* new!, possible bit at right */
            r.box.y0 = texttooffset_y(smash_y0);
            r.box.y1 = r.box.y0 + charvspace;
            tracef5("calling wimp_force_redraw(%d; %d, %d, %d, %d)\n",
                        r.w, r.box.x0, r.box.y0, r.box.x1, r.box.y1);
            wimpt_safe(wimp_force_redraw(&r));
            }
        }


    if(window_width != old_window_width)
        {
        /* window motion may reposition caret */
        (void) mergebuf();

        if(window_width > old_window_width)
            {
            wimp_redrawstr r;
            r.w      = main__window;
            r.box.x0 = texttooffset_x(old_window_width);
            r.box.x1 = texttooffset_x(window_width+1);  /* new!, possible bit at right */
            r.box.y0 = texttooffset_y(-1);              /* top slop too */
            r.box.y1 = texttooffset_y(window_height);
            tracef5("calling wimp_force_redraw(%d; %d, %d, %d, %d)\n",
                        r.w, r.box.x0, r.box.y0, r.box.x1, r.box.y1);
            wimpt_safe(wimp_force_redraw(&r));
            }
        }


    /* if opened at front, claim the caret */
    if((behind == -1)  &&  (main_window != caret_window))
        xf_acquirecaret = TRUE;

    draw_screen();          /* which does draw_altered_state() */
}


/* suss what RISC OS specific stuff has changed since last draw_screen
 * and take appropriate action
*/

extern BOOL
draw_altered_state(void)
{
    wimp_redrawstr r;
    wimp_wstate wstate;
    intl oswidth, osheight;
    intl xext, yext, scx, scy;

    /* read current fake window size etc. */
    wimpt_safe(wimp_get_wind_state(fake__window, &wstate));
    tracef5("\n\n\n*** draw_altered_state(): get_wind_state returns %d, %d, %d, %d;",
                wstate.o.w, wstate.o.box.x0, wstate.o.box.y0,
                wstate.o.box.x1, wstate.o.box.y1);
    tracef2(" scx %d, scy %d\n", wstate.o.scx, wstate.o.scy);

    oswidth  = wstate.o.box.x1 - wstate.o.box.x0;
    osheight = wstate.o.box.y1 - wstate.o.box.y0;
    tracef2(" width %d (OS), height %d (OS)\n", oswidth, osheight);


    /* recompute extents */
    xext = muldiv(oswidth,  (intl) numcol, colsonscreen ? colsonscreen : 1);

    if( xext < max_poss_width)
        xext = max_poss_width;


    yext = muldiv(osheight, (intl) numrow, rowsonscreen);

    if( yext < max_poss_height)
        yext = max_poss_height;

    yext = -yext;

    /* extent must be rounded to pixels so as not to confuse Neil */
    round_with_mask(xext, dxm1);
    round_with_mask(yext, dym1);
    tracef2("computed xext %d (OS), yext %d (OS)\n", xext, yext);

    if((xext != curr_xext)  ||  (yext != curr_yext))
        {
        tracef2("different extents: old xext %d, yext %d\n", curr_xext, curr_yext);

        /* note new extent */
        curr_xext = xext;
        curr_yext = yext;

        /* set new extent of fake window */
        r.w      = fake__window;
        r.box.x0 = 0;
        r.box.x1 = xext;
        r.box.y0 = yext;
        r.box.y1 = 0;

        tracef5("calling wimp_set_extent(%d; %d, %d, %d, %d)\n",
                    r.w, r.box.x0, r.box.y0, r.box.x1, r.box.y1);
        wimpt_safe(wimp_set_extent(&r));
        }


    /* now think what to do with the scroll offsets */
    scx = compute_scx();
    scy = compute_scy();

    if(((scx + delta_scx) != curr_scx)  ||  ((scy + delta_scy) != curr_scy))
        {
        /* note new scroll offsets and zero the deltas */
        curr_scx    = scx;
        curr_scy    = scy;
        delta_scx   = 0;
        delta_scy   = 0;

        /* reopen fake window at new scroll offsets */
        wstate.o.scx = scx;
        wstate.o.scy = scy;
        tracef5("calling wimp_open_wind(%d; %d, %d, %d, %d;",
                wstate.o.w, wstate.o.box.x0, wstate.o.box.y0,
                wstate.o.box.x1, wstate.o.box.y1);
        tracef2(" %d, %d)\n", wstate.o.scx, wstate.o.scy);
        wimpt_safe(wimp_open_wind(&wstate.o));
        }

    return(FALSE);
}


extern void
setcaretpos(intl x, intl y)
{
    tracef2("setcaretpos(%d, %d)\n", x, y);

    riscos_setcaretpos( main_window,
                        (riscos_fonts  &&  !xf_inexpression)
                                ? x + leftslop
                                : texttooffset_x(x),
                        texttooffset_y(y));
}


/****************************************
*                                       *
* set caret position in a window        *
* NB. this takes offsets from xorg/yorg *
*                                       *
****************************************/

extern void
riscos_setcaretpos(riscos_window w, int x, int y)
{
    intl caretbits;
    wimp_caretstr caret;

    y -= CARET_BODGE_Y;

    caretbits   = (charvspace + 2 * CARET_BODGE_Y)                                  |
                   ((riscos_fonts  &&  !xf_inexpression) ? 0 : CARET_SYSTEMFONT)    |
                   CARET_COLOURED | CARET_REALCOLOUR                                |
                   ((current_palette[getcolour(CARETC)].bytes.gcol ^
                     current_palette[getcolour(BACK)].bytes.gcol) << CARET_COLOURSHIFT);

    caret.w      = (wimp_w) w;
    caret.i      = -1;          /* never in any icon */
    caret.x      = x;
    caret.y      = y;
    caret.height = caretbits;
    caret.index  = 0;

    tracef4("riscos_setcaretpos(%d, %d, %d): %8.8X\n", w, x, y, caret.height);

    wimpt_safe(wimp_set_caret_pos(&caret));
}


/*********************************************
*                                            *
* remove caret from display in a window      *
* but keeping the input focus in that window *
*                                            *
*********************************************/

/* stored caret position: relative to work area origin */
static intl caret_rel_x;
static intl caret_rel_y;

extern void
riscos_removecaret(void)
{
    wimp_caretstr caret;

    tracef0("riscos_removecaret()\n");

    if(main_window == caret_window)
        {
        wimpt_safe(wimp_get_caret_pos(&caret));

        caret_rel_x = caret.x /*- curr_scx*/;   /* +ve */
        caret_rel_y = caret.y /*- curr_scy*/;   /* -ve */

        tracef4("hiding caret from offset %d %d at relative pos %d, %d\n",
                caret.x, caret.y, caret_rel_x, caret_rel_y);

        /* way off top */
        caret.y = 100;

        wimpt_safe(wimp_set_caret_pos(&caret));
        }
}


/******************************
*                             *
* restore caret after removal *
*                             *
******************************/

extern void
riscos_restorecaret(void)
{
    wimp_caretstr caret;

    tracef0("riscos_restorecaret()\n");

    if(main_window == caret_window)
        {
        wimpt_safe(wimp_get_caret_pos(&caret));

        caret.x = /*curr_scx*/ + caret_rel_x;
        caret.y = /*curr_scy*/ + caret_rel_y;

        tracef4("restoring caret from relative pos %d %d to offset %d, %d\n",
                caret_rel_x, caret_rel_y, caret.x, caret.y);

        wimpt_safe(wimp_set_caret_pos(&caret));
        }
}


/********************************
*                               *
*  --in--   'at' position       *
*                               *
*  --out--  position corrupted  *
*                               *
********************************/

extern void
draw_grid_hbar(intl len)
{
    intl fg = current_fg;

    setfgcolour(BORDERC);

    wimpt_safe(bbc_plot(bbc_MoveCursorRel,              -dx,             -vdu5textoffset));
    wimpt_safe(bbc_plot(bbc_SolidBoth + bbc_DrawRelFore, len * charwidth, 0));

    riscos_setcolour(fg, FALSE);
}


/********************************
*                               *
*  --in--   'at' position       *
*                               *
*  --out--  position preserved  *
*                               *
********************************/

extern void
draw_grid_vbar(BOOL include_hbar)
{
    intl fg = current_fg;
    intl ldx = dx;
    intl ypos = charvrubout_pos;
    intl yneg = charvrubout_neg + (include_hbar ? dy : 0);

    setfgcolour(BORDERC);

    wimpt_safe(bbc_plot(bbc_MoveCursorRel,              -ldx, +ypos));
    wimpt_safe(bbc_plot(bbc_SolidBoth + bbc_DrawRelFore,   0, -ypos -yneg));
    wimpt_safe(bbc_plot(bbc_MoveCursorRel,              +ldx,       +yneg));

    riscos_setcolour(fg, FALSE);
}


extern void
filealtered(BOOL newstate)
{
    if(xf_filealtered != newstate)
        {
        xf_filealtered = newstate;

        riscos_settitlebar(currentfilename);
        }
}


/****************************************************************************
*                                                                           *
*                           RISC OS printing                                *
*                                                                           *
****************************************************************************/

static print_positionstr print_where = {0, 0};

#define XOS_Find (13 + (1<<17))

extern void
riscprint_set_printer_data(void)
{
    print_pagesizestr psize;

    tracef0("riscprint_set_printer_data()\n");

    if(!print_pagesize(&psize))
        {
        print_where.dx = psize.bbox.x0;
        print_where.dy = psize.bbox.y0;
        tracef2("new print position %d %d\n", print_where.dx, print_where.dy);
        }
}


static int  job;
static int  oldjob;
static print_pagesizestr psize;
static intl usable_x;
static intl usable_y;
static BOOL has_colour;

extern BOOL
riscprint_start(void)
{
    print_infostr pinfo;
    BOOL bum;

    tracef0("\n\n*** riscprint_start()\n");

    job = 0;
    oldjob = -1;

    if(print_info(&pinfo))
        return(reperr_null(ERR_NORISCOSPRINTER));

    has_colour = (pinfo.features & print_colour);

    if(print_complain(print_pagesize(&psize)))
        return(FALSE);

    usable_x = roundtofloor(psize.bbox.x1 - psize.bbox.x0, (72000/180));
    usable_y = roundtofloor(psize.bbox.y1 - psize.bbox.y0, (72000/180));

    tracef2("page size: x = %g in., y = %g in.\n",
            psize.xsize / 72000., psize.ysize / 72000.);
    tracef4("page outline %d %d %d %d (mp)\n",
            psize.bbox.x0, psize.bbox.y0, psize.bbox.x1, psize.bbox.y1);
    tracef2("usable x = %d (mp), %d (os)\n",
            psize.bbox.x1 - psize.bbox.x0, usable_x);
    tracef2("usable y = %d (mp), %d (os)\n",
            psize.bbox.y1 - psize.bbox.y0, usable_y);

    /* open file to printer to get job handle */
    bum = (BOOL) os_swi2r(13+(1<<17), 0x8C, (int) "printer:", &job, NULL);
    tracef1("got print handle %d\n", job);

    if(bum  ||  (job == 0))
        return(reperr_null(ERR_PRINTERINUSE));

    if(!bum)
        bum = print_complain(print_selectjob(job, NULL, &oldjob));

    tracef1("old print handle %d\n", oldjob);

    return(!bum);
}


/********************************************************************
*                                                                   *
*  ensure font colours are set to reflect those set via setcolour   *
*                                                                   *
********************************************************************/

extern void
ensurefontcolours(void)
{
    if(riscos_fonts  &&  font_colours_invalid)
        {
        tracef2("wimp_setfontcolours(%d, %d)\n", current_fg, current_bg);
        (void) font_complain(wimp_setfontcolours(current_fg, current_bg));
        font_colours_invalid = FALSE;
        }
}


#define stat_bg     0xFFFFFF00  /* full white */
#define stat_fg     0x00000000  /* full black */
#define stat_neg    0xFF000000  /* full red */

extern void
print_setcolours(intl fore, intl back)
{
    if(has_colour)
        setcolour(fore, back);
    else
        riscos_setcolours(0 /*bg*/, 7 /*fg*/);
}


extern void
print_setfontcolours(void)
{
    font fh;
    wimp_paletteword bg, fg;
    int offset;

    if(riscos_fonts  &&  font_colours_invalid)
        {
        fh = 0;
        bg.word = stat_bg;
        fg.word = (has_colour  &&  (current_fg == NEGATIVEC)) ? stat_neg : stat_fg;
        offset = 14;
        tracef2("colourtran_setfontcolours(%8.8X, %8.8X)\n", fg.word, bg.word);
        (void) print_complain(colourtran_setfontcolours(&fh, &bg, &fg, &offset));
        font_colours_invalid = FALSE;
        }
}


extern BOOL
riscprint_page(intl copies, BOOL landscape, intl scale,
               intl sequence, riscos_printproc pageproc)
{
    print_box ssize;
    print_box pbox;
    print_transmatstr transform;
    char buffer[32];
    char *pageptr;
    int  ID;
    BOOL more, bum;
    intl xform = (0x10000 * scale) / 100;   /* fixed binary point number 16.16 */

    tracef2("riscprint_page(%d copies, landscape = %s)\n",
                copies, trace_boolstring(landscape));

    if(!scale)
        return(reperr_null(ERR_BADPRINTSCALE));

    if(landscape)
        {
        /* landscape output: -90 deg rotation */
        transform.xx =  0;
        transform.xy =  xform;
        transform.yx = -xform;
        transform.yy =  0;
        }
    else
        {
        /* portrait output: no rotation */
        transform.xx =  xform;
        transform.xy =  0;
        transform.yx =  0;
        transform.yy =  xform;
        }

    /* our object size - optimise here sometime for margins */
    ssize.x0 = 0;
    ssize.x1 = ((landscape ? usable_y : usable_x) * 100) / scale;
    ssize.y0 = 0;
    ssize.y1 = ((landscape ? usable_x : usable_y) * 100) / scale;

    if(landscape)
        print_where.dx += usable_x /*ssize.y1*/ *(72000/180);
    tracef4("calling giverectangle with (0, 0, %d, %d) (os), print_where %d %d(real mp)\n", ssize.x1, ssize.y1, print_where.dx, print_where.dy);

    bum = print_complain(print_giverectangle(1, &ssize, &transform,
                                                &print_where,
                                                -1 /* background RGB */));
    tracef1("print_giverectangle returned %d\n", bum);

    if(landscape)
        print_where.dx -= usable_x /*ssize.y1*/ *(72000/180);

    if(!bum)
        {
        tracef0("print_drawpage()\n");

        killcolourcache();

        if(curpnm)
            {
            sprintf(buffer, "p%d", curpnm);
            pageptr = buffer;
            }
        else
            pageptr = NULL;

        bum = print_complain(print_drawpage(copies,
                                            &pbox,
                                            sequence,   /* sequence number */
                                            pageptr,    /* textual page number */
                                            &more, &ID));

        while(!bum  &&  more)
            {
            tracef4("print loop ... gw %d %d %d %d\n",
                    graphics_window.x0, graphics_window.y0,
                    graphics_window.x1, graphics_window.y1);

            #if TRACE && FALSE
            if(trace_enabled)
                {
                int x, y;

                setcolour(FORE, BACK);

                bum = print_complain(bbc_circle(0, 0, 25));

                /* draw an inches grid */
                for(x = 0; x < usable_x + 180; x += 180)
                    {
                if(!bum)
                        bum = print_complain(bbc_move(x, -180));
                    if(!bum)
                        bum = printt_complain(bbc_plot(bbc_SolidBoth + bbc_DrawRelFore, 0,
                                                        usable_y + 2*180));
                    if(bum)
                        break;
                    }

                for(y = 0; y < usable_y + 180; y += 180)
                    {
                    if(!bum)
                        bum = print_complain(bbc_move(-180, y));
                    if(!bum)
                        bum = print_complain(bbc_plot(bbc_SolidBoth + bbc_DrawRelFore,
                                                        usable_x + 2*180, 0));
                    if(bum)
                        break;
                    }
                }
            #endif

            /* print page */

            riscos_font_yad = ssize.y1 * (72000/180) - global_font_leading;
            tracef1("\n\n*** initial riscos_font_yad := %d (mp)\n",
                        riscos_font_yad);

            killcolourcache();

            if(!bum)
                pageproc();

            if(been_error)
                bum = TRUE;

            if(!bum)
                bum = print_complain(print_getrectangle(&pbox, &more, &ID));
            /* will output showpage etc. when no more rectangles for page */
            }
        }

    killcolourcache();

    return(!bum);
}


extern void
riscprint_end(BOOL ok)
{
    tracef1("riscprint_end(): %s print\n", !ok ? "aborting" : "ending");

    if(ok)
        ok = !print_complain(print_endjob(job));

    if(!ok)
        (void) print_complain(print_abortjob(job));

    if(oldjob != -1)
        {
        tracef1("reselecting job %d\n", oldjob);
        (void) print_complain(print_selectjob(oldjob, NULL, &oldjob));
        }

    tracef0("closing printer stream\n");
    (void) print_complain(os_swi2(XOS_Find, 0, job));
}


extern void
riscprint_resume(void)
{
    int dummy_job;

    tracef0("riscprint_resume()\n");

    (void) print_complain(print_selectjob(job, NULL, &dummy_job));
}


extern void
riscprint_suspend(void)
{
    int dummy_job;

    tracef0("riscprint_suspend()\n");

    (void) print_complain(print_selectjob(0, NULL, &dummy_job));
}

#endif /* RISCOS */

/* end of riscdraw.c */
