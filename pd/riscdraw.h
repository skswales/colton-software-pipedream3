/* riscdraw.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       riscdraw.h - exported objects from riscdraw.c
 * Requires:
 *  "wimplib:"
 *  "ext.riscos"
 * Author:      Stuart K. Swales 15-Mar-1989
*/

#if RISCOS
/* Only export objects if RISCOS */

#ifndef __pd__riscdraw_h
#define __pd__riscdraw_h

/* exported functions */

extern void at(intl tx, intl ty);
extern void at_fonts(intl x, intl ty);
extern void application_open_request(wimp_eventstr *e);
extern void application_redraw(riscos_redrawstr *r);
extern void application_scroll_request(wimp_eventstr *e);
extern void cachemodevariables(void);
extern void cachepalettevariables(void);
extern void clear_textarea(intl tx0, intl ty0, intl tx1, intl ty1, BOOL zap_grid);
extern void clear_thistextarea(void);
extern void clear_underlay(intl len);
extern BOOL draw_altered_state(void);
extern void draw_grid_hbar(intl len);
extern void draw_grid_vbar(BOOL include_hbar);
extern void ensurefontcolours(void);
extern intl gcoord_x(intl tx);
extern intl gcoord_y(intl ty);
extern intl gcoord_y_fontout(intl ty);
extern intl gcoord_y_textout(intl ty);
extern void killfontcolourcache(void);
extern void killcolourcache(void);
extern void my_force_redraw(riscos_window w);
extern void ospca(intl nspaces);
extern void ospca_fonts(intl nspaces);
extern void new_grid_state(void);
extern void please_clear_textarea(intl tx0, intl ty0, intl tx1, intl ty1);
extern void please_invert_numeric_slot(intl coff, intl roff, intl fg, intl bg);
extern void please_invert_numeric_slots(intl start_coff, intl end_coff, intl roff, intl fg, intl bg);
extern void please_redraw_entire_window(void);
extern void please_redraw_textarea(intl tx0, intl ty0, intl tx1, intl ty1);
extern void please_redraw_textline(intl tx0, intl ty0, intl tx1);
extern void please_update_textarea(riscos_redrawproc proc, intl tx0, intl ty0, intl tx1, intl ty1);
extern void please_update_thistextarea(riscos_redrawproc proc);
extern void print_setcolours(intl fore, intl back);
extern void print_setfontcolours(void);
extern void restore_graphics_window(void);
extern intl rgb_for_wimpcolour(intl wimpcolour);
extern void riscos_movespaces(intl nspaces);
extern void riscos_movespaces_fonts(intl nspaces);
extern void riscos_printspaces(intl nspaces);
extern void riscos_printspaces_fonts(intl nspaces);
extern intl roundtoceil(intl a, intl b);
extern intl roundtofloor(intl a, intl b);
extern void scroll_textarea(intl tx0, intl ty0, intl tx1, intl ty1, intl nlines);
extern void setcaretpos(intl x, intl y);
extern BOOL set_graphics_window_from_textarea(intl tx0, intl ty0, intl tx1, intl ty1, BOOL set_gw);
extern void settextorigin(intl x, intl y);
extern intl tcoord_x(intl x);
extern intl tcoord_y(intl y);
extern intl tcoord_x_remainder(intl x);
extern intl tcoord_x1(intl x);
extern intl tcoord_y1(intl y);
extern intl tsize_x(intl x);
extern intl tsize_y(intl y);
extern BOOL textobjectintersects(intl tx0, intl ty0, intl tx1, intl ty1);
extern BOOL textxintersects(intl tx0, intl tx1);
extern intl texttooffset_x(intl tx);
extern intl texttooffset_y(intl ty);
extern intl windowheight(void);
extern intl windowwidth(void);
extern void riscprint_resume(void);
extern void riscprint_set_printer_data(void);
extern BOOL riscprint_start(void);
extern void riscprint_suspend(void);
extern void riscprint_end(BOOL ok);
extern BOOL riscprint_page(intl copies, BOOL landscape, intl scale,
                           intl sequence, riscos_printproc pageproc);


/* in slotconv.c! */
extern intl font_charwid(font ch_font, intl ch);
extern os_error *font_complain(os_error *err);


/* exported variables */

extern intl dxm1;
extern intl dy;
extern intl current_bg;
extern intl current_fg;
extern intl log2bpp;


/* PipeDream specific character printing stuff */

#define leftslop    two_dx      /* needed for visible lhs caret */
#define topslop     0 /* (charheight/4) */


#define CARET_REALCOLOUR    (1 << 27)
#define CARET_COLOURED      (1 << 26)
#define CARET_INVISIBLE     (1 << 25)
#define CARET_SYSTEMFONT    (1 << 24)
#define CARET_COLOURSHIFT   16              /* bits 16..23 */
#define CARET_BODGE_Y       (charheight/4)  /* extra bits on top & bottom of caret */

/* The Window Manager default */
#define CARET_DEFAULT_COLOUR    11

#endif  /* __pd__riscdraw_h */

#endif  /* RISCOS */

/* end of riscdraw.h */
