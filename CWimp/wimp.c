/* Title  > c.wimp
 * Purpose: C interface to RISC OS Wimp routines
 * Version: 0.1
 *  DAHE, 31 Oct 1988: type in wimp_ploticon corrected.
 *  APT,  24 Nov 1988: colours from Wimp_ReadPalette scaled properly
 *  SKS hacked severely to compact & handle printer errors
*/

/***************************************************************************
* This source file was written by Acorn Computers Limited. It is part of   *
* the "cwimp" library for writing applications in C for RISC OS. It may be *
* used freely in the creation of programs for Archimedes. It should be     *
* used with Acorn's C Compiler Release 2 or later.                         *
*                                                                          *
* No support can be given to programmers using this code and, while we     *
* believe that it is correct, no correspondence can be entered into        *
* concerning behaviour or bugs.                                            *
*                                                                          *
* Upgrades of this code may or may not appear, and while every effort will *
* be made to keep such upgrades upwards compatible, no guarantees can be   *
* given.                                                                   *
***************************************************************************/


#include "trace.h"
#include "os.h"
#if TRACE
#include <stdlib.h>
#include "print.h"
#endif

#include "wimp.h"


/*  W I M P    S W I 's      */
/* Always done in XWimp form */

#define Initialise          (0x000400C0 | os_X)
#define CreateWindow        (0x000400C1 | os_X)
#define CreateIcon          (0x000400C2 | os_X)
#define DeleteWindow        (0x000400C3 | os_X)
#define DeleteIcon          (0x000400C4 | os_X)
#define OpenWindow          (0x000400C5 | os_X)
#define CloseWindow         (0x000400C6 | os_X)
#define Poll                (0x000400C7 | os_X)
#define RedrawWindow        (0x000400C8 | os_X)
#define UpdateWindow        (0x000400C9 | os_X)
#define GetRectangle        (0x000400CA | os_X)
#define GetWindowState      (0x000400CB | os_X)
#define GetWindowInfo       (0x000400CC | os_X)
#define SetIconState        (0x000400CD | os_X)
#define GetIconState        (0x000400CE | os_X)
#define GetPointerInfo      (0x000400CF | os_X)
#define DragBox             (0x000400D0 | os_X)
#define ForceRedraw         (0x000400D1 | os_X)
#define SetCaretPosition    (0x000400D2 | os_X)
#define GetCaretPosition    (0x000400D3 | os_X)
#define CreateMenu          (0x000400D4 | os_X)
#define DecodeMenu          (0x000400D5 | os_X)
#define WhichIcon           (0x000400D6 | os_X)
#define SetExtent           (0x000400D7 | os_X)
#define SetPointerShape     (0x000400D8 | os_X)
#define OpenTemplate        (0x000400D9 | os_X)
#define CloseTemplate       (0x000400DA | os_X)
#define LoadTemplate        (0x000400DB | os_X)
#define ProcessKey          (0x000400DC | os_X)
#define CloseDown           (0x000400DD | os_X)
#define StartTask           (0x000400DE | os_X)
#define ReportError         (0x000400DF | os_X)
#define GetWindowOutline    (0x000400E0 | os_X)
#define PollIdle            (0x000400E1 | os_X)
#define PlotIcon            (0x000400E2 | os_X)
#define SetMode             (0x000400E3 | os_X)
#define SetPalette          (0x000400E4 | os_X)
#define ReadPalette         (0x000400E5 | os_X)
#define SetColour           (0x000400E6 | os_X)
#define SendMessage         (0x000400E7 | os_X)
#define CreateSubMenu       (0x000400E8 | os_X)
#define SpriteOp            (0x000400E9 | os_X)
#define BaseOfSprites       (0x000400EA | os_X)
#define BlockCopy           (0x000400EB | os_X)
#define SlotSize            (0x000400EC | os_X)
#define ReadPixTrans        (0x000400ED | os_X)
#define ClaimFreeMemory     (0x000400EE | os_X)
#define CommandWindow       (0x000400EF | os_X)
#define TextColour          (0x000400F0 | os_X)
#define TransferBlock       (0x000400F1 | os_X)
#define ReadSysInfo         (0x000400F2 | os_X)
#define SetFontColours      (0x000400F3 | os_X)


#define WIMP_TASK_WORD ('T' + ('A' << 8) + ('S' << 16) + ('K' << 24))


#ifndef NULL
#define NULL 0
#endif


#ifdef OldWimp
os_error *
wimp_initialise(int *v /*out*/)
{
    os_regset r;
    os_error *e;
    r.r[0] = 120;
    e = os_swix(Initialise, &r);
    if(!e)
        *v =  r.r[0];
    return(e);
}
#endif


os_error *
wimp_taskinit(const char *name, wimp_t *t)
{
    os_regset r;
    os_error *e;
    r.r[0] = 200;
    r.r[1] = WIMP_TASK_WORD;
    r.r[2] = (int) name;
    e = os_swix(Initialise, &r);
    if(!e  &&  t)
        *t = r.r[1];
    return(e);
}


static os_error *
wimp__swi(int swinumber, void *blk)
{
    os_regset r;
    r.r[0] = 0;
    r.r[1] = (int) blk;
    return(os_swix(swinumber, &r));
}


os_error *
wimp_create_wind(wimp_wind *w, wimp_w *result /*out*/)
{
    os_regset r;
    os_error *e;
    r.r[1] = (int) w;
    e = os_swix(CreateWindow, &r);
    if(!e)
        *result = r.r[0]; 
    return(e);
}


os_error *
wimp_create_icon(wimp_icreate *i, wimp_i *result /*out*/)
{
    os_regset r;
    os_error *e;
    r.r[1] = (int)i;
    e = os_swix(CreateIcon, &r);
    if(!e)
        *result = r.r[0]; 
    return(e);
}


os_error *
wimp_delete_wind(wimp_w w)
{
    return(wimp__swi(DeleteWindow, &w));
}


os_error *
wimp_delete_icon(wimp_w w, wimp_i i)
{
    int j[2];
    j[0] = (int) w;
    j[1] = (int) i;
    return(wimp__swi(DeleteIcon, j));
}


os_error *
wimp_open_wind(wimp_openstr *o)
{
    return(wimp__swi(OpenWindow, o));
}


os_error *
wimp_close_wind(wimp_w w)
{
    return(wimp__swi(CloseWindow, &w));
}


os_error *
wimp_redraw_wind(wimp_redrawstr *wr, int *result /*out*/)
{
    os_regset r;
    os_error *e;
    r.r[1] = (int) wr;
    e = os_swix(RedrawWindow, &r);
    if(!e)
        *result = r.r[0];
    return(e);
}


os_error *
wimp_update_wind(wimp_redrawstr *wr, int *result /*out*/)
{
    os_regset r;
    os_error *e;
    r.r[1] = (int) wr;
    e = os_swix(UpdateWindow, &r);
    if(!e)
      *result = r.r[0];
    return(e);
}


os_error *
wimp_get_rectangle(wimp_redrawstr *wr, int *result /*out*/)
{
    os_regset r;
    os_error *e;
    r.r[1] = (int) wr;
    e = os_swix(GetRectangle, &r);
    if(!e)
        *result = r.r[0];
    return(e);
}


os_error *
wimp_get_wind_state(wimp_w w, wimp_wstate *result /*out*/)
{
    result->o.w = w;
    return(wimp__swi(GetWindowState, result));
}


os_error *
wimp_get_wind_info(wimp_winfo *result /*out*/)
{
    return(wimp__swi(GetWindowInfo, result));
}


typedef struct
{
    wimp_w          wind_h;
    wimp_i          icon_h;
    wimp_iconflags  flags_v;
    wimp_iconflags  flags_m;
}
wimp__handles_and_flags;


os_error *
wimp_set_icon_state(wimp_w w, wimp_i i, wimp_iconflags value, wimp_iconflags mask)
{
    wimp__handles_and_flags b;
    b.wind_h  = w;
    b.icon_h  = i;
    b.flags_v = value;
    b.flags_m = mask;
    return(wimp__swi(SetIconState, &b));
}


typedef struct
{
    wimp_w      wind_h;
    wimp_i      icon_h;
    wimp_icon   icon_s;
}
wimp__icon_and_handles;


os_error *
wimp_get_icon_info(wimp_w w, wimp_i i, wimp_icon *result /*out*/)
{
    os_regset r;
    os_error *e;
    wimp__icon_and_handles b;
    b.wind_h = w;
    b.icon_h = i;
    r.r[1] = (int) &b;
    e = os_swix(GetIconState, &r);
    if(!e)
        *result = b.icon_s;
    return(e);
}


typedef struct
{
    wimp_mousestr m;
    int dud;            /* Wimp renowned for crapping on things, especially this one */
}
wimp__mstr;


os_error *
wimp_get_point_info(wimp_mousestr *result /*out*/)
{
    os_regset r;
    os_error *e;
    wimp__mstr m;
    r.r[1] = (int) &m;
    e = os_swix(GetPointerInfo, &r);
    if(!e)
        *result = m.m;
    return(e);
}


os_error *
wimp_drag_box(wimp_dragstr *d)
{
    return(wimp__swi(DragBox, d));
}


os_error *wimp_force_redraw(wimp_redrawstr *r)
{
    return(os_swix(ForceRedraw, (os_regset *) r));
}


os_error *
wimp_set_caret_pos(wimp_caretstr *c)
{
    return(os_swix(SetCaretPosition, (os_regset *) c));
}


os_error *
wimp_get_caret_pos(wimp_caretstr *c)
{
    return(wimp__swi(GetCaretPosition, c));
}


os_error *
wimp_create_menu(wimp_menustr *m, int x, int y)
{
    return(os_swi4(CreateMenu, 0, (int) m, x, y));
}


os_error *
wimp_decode_menu(wimp_menustr *m, void *p1, void *p2)
{
    return(os_swi4(DecodeMenu, 0, (int) m, (int) p1, (int) p2));
}


os_error *
wimp_which_icon(wimp_which_block *w, wimp_i *results)
{
    return(os_swi4(WhichIcon, w->window, (int) results, w->bit_mask, w->bit_set));
}


os_error *
wimp_set_extent(wimp_redrawstr *wr)
{
    return(os_swi2(SetExtent, wr->w, (int) wr + 4));
}


os_error *
wimp_set_point_shape(wimp_pshapestr *p)
{
    return(os_swix(SetPointerShape, (os_regset *) p));
}


os_error *
wimp_open_template(const char *name)
{
    return(wimp__swi(OpenTemplate, (char *) name));
}


os_error *
wimp_close_template(void)
{
    return(os_swix(CloseTemplate, NULL));
}


os_error *
wimp_load_template(wimp_template *t)
{
    return(os_swix(LoadTemplate, (os_regset *) t));
}


os_error *
wimp_processkey(int chcode)
{
    return(os_swi1(ProcessKey, chcode));
}


os_error *
wimp_closedown(void)
{
    return(os_swix(CloseDown, NULL));
}


os_error *
wimp_taskclose(wimp_t t)
{
    return(os_swi2(CloseDown, t, WIMP_TASK_WORD));
}


os_error *
wimp_starttask(const char *clicmd)
{
    return(os_swi1(StartTask, (int) clicmd));
}


os_error *
wimp_getwindowoutline(wimp_redrawstr *wr)
{
    return(wimp__swi(GetWindowOutline, wr));
}


os_error *
wimp_ploticon(const wimp_icon *i)
{
    return(wimp__swi(PlotIcon, (void *) i));
}


os_error *
wimp_setmode(int mode)
{
    return(os_swi1(SetMode, mode));
}


os_error *
wimp_readpalette(wimp_palettestr *p)
{
    os_regset r;
    os_error *e;
    int i, t;
    r.r[1] = (int) p;
    e = os_swix(ReadPalette, &r);
    if(!e)
        for(i = 0; i < 20; i++)
            {
            t = ((int *) p)[i] & 0xF0F0F0FF;
            ((int *) p)[i] = t | ((t>>4) & 0xF0F0F00);
            /* copy all the colour nibbles down, to scale by 17/16 */
            }
    return(e);
}


os_error *
wimp_setpalette(wimp_palettestr *p)
{
    os_regset r;
    r.r[1] = (int) p;
    return(os_swix(SetPalette, &r));
}


os_error *
wimp_setcolour(int colour)
{
    os_regset r;
    r.r[0] = colour;
    return(os_swix(SetColour, &r));
}


os_error *
wimp_textcolour(int colour)
{
    os_regset r;
    r.r[0] = colour;
    return(os_swix(TextColour, &r));
}


void *
wimp_baseofsprites(void)
{
    os_regset r;
    (void) os_swix(BaseOfSprites, &r);
    return((void *) r.r[1]);
}


os_error *
wimp_blockcopy(wimp_w w, const wimp_box *source, int x, int y)
{
    os_regset r;
    r.r[0] = w;
    r.r[1] = source->x0;
    r.r[2] = source->y0;
    r.r[3] = source->x1;
    r.r[4] = source->y1;
    r.r[5] = x;
    r.r[6] = y;
    return(os_swix(BlockCopy, &r));
}


os_error *
wimp_reporterror(os_error *e, wimp_errflags flags, const char *name)
{
#if TRACE
    int out;
    os_error *err;
    (void) print_reset();
    if(flags == 0)
        flags = wimp_EOK;   /* bastard Neil I hate him */
    err = os_swi3r(ReportError, (int) e, flags | wimp_ECANCEL, (int) name, NULL, &out, NULL);
    if((flags == wimp_EOK)  &&  (out & wimp_ECANCEL))
        wimpt_abort(e);
    return(err);
#else
    return(os_swi3(ReportError, (int) e, flags, (int) name));
#endif
}


os_error *
wimp_sendmessage(wimp_etype code, wimp_msgstr *m, wimp_t dest)
{
    os_error *e;

    tracef3("sending event %s message %s to task &%p: ", trace_wimp_eventcode(code),
            (code >= wimp_ESEND) ? trace_wimp_xmessage(m, TRUE) : trace_wimp_xevent(code, m),
            dest);

    if(code >= wimp_ESEND)
        {
        /* round up to word size for clients */
        if(m->hdr.size & 3)
            m->hdr.size = (m->hdr.size + 3) & ~3;
        }

    e = os_swi3(SendMessage, code, (int) m, dest);

    #if TRACE
    if(!e)
        tracef2("got my_ref %d, my task id &%p\n", m->hdr.my_ref, m->hdr.task);
    #endif

    return(e);
}


os_error *
wimp_sendwmessage(wimp_etype code, wimp_msgstr* m, wimp_w w, wimp_i i)
{
    os_error *e;

    tracef4("sending event %s message %s to window %d icon %d: ", trace_wimp_eventcode(code),
            (code >= wimp_ESEND) ? trace_wimp_xmessage(m, TRUE) : trace_wimp_xevent(code, m),
            w, i);

    if(code >= wimp_ESEND)
        {
        /* round up to word size for clients */
        if(m->hdr.size & 3)
            m->hdr.size = (m->hdr.size + 3) & ~3;
        }

    e = os_swi4(SendMessage, code, (int) m, w, i);

    #if TRACE
    if(!e)
        tracef2("got my_ref %d, my task id &%p\n", m->hdr.my_ref, m->hdr.task);
    #endif

    return(e);
}


os_error *
wimp_create_submenu(const wimp_menustr *sub, int x, int y)
{
    return(os_swi4(CreateSubMenu, 0, (int) sub, x, y));
}


os_error *
wimp_slotsize(int *currentslot /*inout*/,
              int *nextslot /*inout*/,
              int *freepool /*out*/)
{
    return(os_swi3r(SlotSize, *currentslot, *nextslot, 0,
                              currentslot, nextslot, freepool));
}


os_error *
wimp_transferblock(
    wimp_t sourcetask,
    const char *sourcebuf,
    wimp_t desttask,
    char *destbuf,
    int buflen)
{
    os_regset r;
    r.r[0] = sourcetask;
    r.r[1] = (int) sourcebuf;
    r.r[2] = desttask;
    r.r[3] = (int) destbuf;
    r.r[4] = buflen;
    r.r[5] = 0;
    return(os_swix(TransferBlock, &r));
}


os_error *
wimp_spriteop(int reason_code, const char *name)
{
    os_regset r;
    r.r[0] = reason_code;
    r.r[2] = (int) name;
    return(os_swix(SpriteOp, &r));
}


os_error *
wimp_spriteop_full(os_regset *r)
{
    return(os_swix(SpriteOp, r));
}


os_error *
wimp_setfontcolours(int foreground, int background)
{
    return(os_swi3(SetFontColours, 0, background, foreground));
}


/* end of wimp.c */
