/* windvars.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       windvars.h - variables pertaining to each PipeDream window
 * Author:      RJM 8-Apr-1989
*/

#ifndef __pd__windvars_h
#define __pd__windvars_h

#if !defined(MANY_DOCUMENTS)

/* dummy typedef to keep compiler happy about procedure defs */
typedef struct window_data
{
    intl foo;
}
window_data;


extern dochandle DocHandle;


/* ------------------------------ savload.c -------------------------------- */

extern intl curfil;                 /* current file number in multi-file document */
extern char *currentdirectory;
extern char *currentfilename;
#if RISCOS
extern riscos_fileinfo currentfileinfo;
#endif

extern list_block *first_file;      /* first file in multi-file document */

extern BOOLEAN glbbit;              /* global file */
extern BOOLEAN xf_altered;


/* ------------------------------- bufedit.c ------------------------------- */

extern SLR in_block;                /* current position in block */
extern SLR start_bl;
extern SLR end_bl;

extern BOOLEAN start_block;


/* --------------------------------- constr.c ------------------------------ */

extern rowt currow;                 /* current row number */
extern colt curcol;                 /* current col number */
extern colt numcol;                 /* total number of columns */
extern rowt numrow;                 /* maximum number of rows */
extern intl lecpos;                 /* cursor pos in linbuf */
extern uchar linbuf[];              /* buffer for line editing */


/* ------------------------------- slot.c ---------------------------------- */

extern colp colstart;
extern colt colsintable;


/* --------------------------------- mcdiff.c ------------------------------ */

extern coord paghyt;
extern coord pagwid;
extern coord pagwid_plus1;


/* ----------------------------- scdraw.c ---------------------------------- */

extern coord borderheight;      /* changes when status of borbit changes */
extern coord borderwidth;       /* changes when status of borbit changes */
extern coord cols_available;
extern coord curcoloffset;      /* current column in horzvec */
extern intl curpnm;
extern coord currowoffset;      /* current row in vertvec */
extern mhandle horzvec_mh;
extern coord last_thing_drawn;  /* how dirty is current line? */
extern intl lescrl;             /* offset of lecpos on screen */
extern coord maximum_cols;      /* number of cols in horzvec */
extern coord maximum_rows;      /* number of rows in vertvec */
extern coord maxncol;           /* maximum number of columns on screen */
extern coord maxnrow;           /* maximum number of rows on screen */
extern intl movement;
extern colt newcol;
extern rowt newrow;
extern coord n_rowfixes;        /* number of rows currently fixed on screen */
extern colt oldcol;
extern rowt oldrow;
extern BOOLEAN out_below;
extern BOOLEAN out_currslot;
extern BOOLEAN out_forcevertcentre;
extern BOOLEAN out_rebuildhorz;
extern BOOLEAN out_rebuildvert;
extern BOOLEAN out_rowout;
extern BOOLEAN out_rowout1;
extern BOOLEAN out_screen;
extern coord rowout;
extern coord rowout1;
extern coord rows_available; /* to draw slots in. ie screen excluding borders */
extern mhandle vertvec;
extern BOOLEAN xf_filealtered;
extern BOOLEAN xf_inexpression;
extern BOOLEAN xf_drawcolumnheadings;
extern BOOLEAN xf_draweverything;   /* serious repaint for * commands etc. */
extern BOOLEAN xf_frontmainwindow;
extern BOOLEAN xf_drawslotcoordinates;
extern BOOLEAN xf_drawsome;         /* some (altered) slots need redrawing */

extern coord error_line_dirty; /* length of string currently on error line */

#if MS
extern intl buff_idx;
extern uchar strout_buff[LIN_BUFSIZ];
#endif


#if RISCOS
/* last set of scroll offsets we opened a window at */
extern gcoord old_scx;
extern gcoord old_scy;
#endif


/* ----------------------------- cursmov.c ------------------------------- */

extern coord rowsonscreen;
extern coord rowtoend;
extern coord scrbrc; /* offset of PARTIAL column in horzvec, or LAST if all fit */


/* ------------------------------ numbers.c -------------------------------- */

extern BOOLEAN buffer_altered;
extern colt edtslr_col;
extern rowt edtslr_row;
extern BOOLEAN global_recalc;
extern BOOLEAN output_buffer;
extern BOOLEAN recalc_bit;
extern uchar refs_in_this_slot;
extern uchar refs_in_this_sheet;
extern BOOLEAN slot_in_buffer;


/* ----------------------------- doprint.c --------------------------------- */

extern BOOLEAN wrpbit;      /* wrap on */
extern BOOLEAN jusbit;      /* justify on */
extern BOOLEAN txnbit;      /* text/numbers */
extern BOOLEAN rcobit;      /* recalculate mode */
extern BOOLEAN minbit;      /* minus/brackets */
extern BOOLEAN borbit;      /* drawing borders are on the screen */
extern BOOLEAN iowbit;      /* insert rows/columns on wrap */

extern intl real_pagelength;
extern intl real_pagcnt;


/* ----------------------------- dialog.c ---------------------------------- */

extern intl filpnm;             /* page number at top of file */
extern intl filpof;             /* page offset at top of file */
extern intl enclns;             /* line spacing */
extern intl encpln;             /* page length - margins */
extern intl pagnum;             /* page number at top of screen */
extern intl pagoff;             /* page offset at top of screen */


/* ----------------------------- riscos.c -------------------------------- */

#if RISCOS
extern riscos_window main_window;
#endif


#else   /* MANY_DOCUMENTS */

/* ----- all the window data collected together into one single object ----- */

typedef struct window_data
{
    /* This MUST be at offset 0 for Tutu nasty link mechanism */
    struct window_data *link;

    dochandle DocHandle;        /* normal key to look up with */


/* ---------------------------- savload.c -------------------------------- */

    #define currentfilename             (sb->Xcurrentfilename)
    char *Xcurrentfilename;

    #define currentdirectory            (sb->Xcurrentdirectory)
    char *Xcurrentdirectory;

#if RISCOS
    #define currentfileinfo             (sb->Xcurrentfileinfo)
    riscos_fileinfo Xcurrentfileinfo;
#endif

    #define first_file                  (sb->Xfirst_file)
    list_block *Xfirst_file;

    #define curfil                      (sb->Xcurfil)
    intl Xcurfil;

    #define glbbit                      (sb->Xglbbit)
    BOOLEAN Xglbbit;

    #define xf_filealtered              (sb->Xxf_filealtered)
    BOOLEAN Xxf_filealtered;


/* ------------------------- markers.c ----------------------------------- */

    #define start_block                 (sb->Xstart_block)
    BOOLEAN Xstart_block;

    #define in_block                    (sb->Xin_block)
    SLR Xin_block;

    #define start_bl                    (sb->Xstart_bl)
    SLR Xstart_bl;

    #define end_bl                      (sb->Xend_bl)
    SLR Xend_bl;


/* -------------------------- constr.c ----------------------------------- */

    #define linbuf                      (sb->Xlinbuf)
    uchar Xlinbuf[LIN_BUFSIZ];

    #define lecpos                      (sb->Xlecpos)
    intl Xlecpos;

    #define curcol                      (sb->Xcurcol)
    colt Xcurcol;

    #define currow                      (sb->Xcurrow)
    rowt Xcurrow;

    #define numcol                      (sb->Xnumcol)
    colt Xnumcol;

    #define numrow                      (sb->Xnumrow)
    rowt Xnumrow;


/* ---------------------------- slot.c ----------------------------------- */

    #define colstart                    (sb->Xcolstart)
    colp Xcolstart;

    #define colsintable                 (sb->Xcolsintable)
    colt Xcolsintable;


/* --------------------------- mcdiff.c ---------------------------------- */

    #define paghyt                      (sb->Xpaghyt)
    coord Xpaghyt;

    #define pagwid                      (sb->Xpagwid)
    coord Xpagwid;

    #define pagwid_plus1                (sb->Xpagwid_plus1)
    coord Xpagwid_plus1;


/* ---------------------------- commlin.c -------------------------------- */


/* ---------------------------- scdraw.c --------------------------------- */

    #define horzvec_mh                  (sb->Xhorzvec_mh)
    mhandle Xhorzvec_mh;

    #define vertvec_mh                  (sb->Xvertvec_mh)
    mhandle Xvertvec_mh;


    #define maxncol                     (sb->Xmaxncol)
    coord Xmaxncol;

    #define maxnrow                     (sb->Xmaxnrow)
    coord Xmaxnrow;


    #define out_screen                  (sb->Xout_screen)
    BOOLEAN Xout_screen;

    #define out_rebuildhorz             (sb->Xout_rebuildhorz)
    BOOLEAN Xout_rebuildhorz;

    #define out_rebuildvert             (sb->Xout_rebuildvert)
    BOOLEAN Xout_rebuildvert;

    #define out_forcevertcentre         (sb->Xout_forcevertcentre)
    BOOLEAN Xout_forcevertcentre;

    #define out_below                   (sb->Xout_below)
    BOOLEAN Xout_below;

    #define out_rowout                  (sb->Xout_rowout)
    BOOLEAN Xout_rowout;

    #define out_rowout1                 (sb->Xout_rowout1)
    BOOLEAN Xout_rowout1;

    #define out_currslot                (sb->Xout_currslot)
    BOOLEAN Xout_currslot;


    #define rowout                      (sb->Xrowout)
    coord Xrowout;

    #define rowout1                     (sb->Xrowout1)
    coord Xrowout1;


    #define xf_inexpression             (sb->Xxf_inexpression)
    BOOLEAN Xxf_inexpression;


    #define xf_draweverything           (sb->Xxf_draweverything)
    BOOLEAN Xxf_draweverything;

    #define xf_frontmainwindow          (sb->Xxf_frontmainwindow)
    BOOLEAN Xxf_frontmainwindow;

    #define xf_drawcolumnheadings       (sb->Xxf_drawcolumnheadings)
    BOOLEAN Xxf_drawcolumnheadings;

    #define xf_drawsome                 (sb->Xxf_drawsome)
    BOOLEAN Xxf_drawsome;

    #define xf_drawslotcoordinates      (sb->Xxf_drawslotcoordinates)
    BOOLEAN Xxf_drawslotcoordinates;


    #define maximum_cols                (sb->Xmaximum_cols)
    coord Xmaximum_cols;

    #define maximum_rows                (sb->Xmaximum_rows)
    coord Xmaximum_rows;

    #define cols_available              (sb->Xcols_available)
    coord Xcols_available;

    #define rows_available              (sb->Xrows_available)
    coord Xrows_available;

    #define n_rowfixes                  (sb->Xn_rowfixes)
    coord Xn_rowfixes;

    #define curcoloffset                (sb->Xcurcoloffset)
    coord Xcurcoloffset;

    #define currowoffset                (sb->Xcurrowoffset)
    coord Xcurrowoffset;

    #define oldcol                      (sb->Xoldcol)
    colt Xoldcol;

    #define newcol                      (sb->Xnewcol)
    colt Xnewcol;

    #define oldrow                      (sb->Xoldrow)
    rowt Xoldrow;

    #define newrow                      (sb->Xnewrow)
    rowt Xnewrow;


    #define movement                    (sb->Xmovement)
    intl Xmovement;

    #define lescrl                      (sb->Xlescrl)
    intl Xlescrl;


    #define last_thing_drawn            (sb->Xlast_thing_drawn)
    coord Xlast_thing_drawn;


    #define borderheight                (sb->Xborderheight)
    coord Xborderheight;

    #define borderwidth                 (sb->Xborderwidth)
    coord Xborderwidth;


    #define curpnm                      (sb->Xcurpnm)
    intl Xcurpnm;


    #define error_line_dirty            (sb->Xerror_line_dirty)
    coord Xerror_line_dirty;


    #if RISCOS
    #define curr_scx                    (sb->Xcurr_scx)
    gcoord Xcurr_scx;

    #define curr_scy                    (sb->Xcurr_scy)
    gcoord Xcurr_scy;
    #endif


/* ----------------------------- cursmov.c ------------------------------- */

    #define rowsonscreen                (sb->Xrowsonscreen)
    coord Xrowsonscreen;

    #define scrbrc                      (sb->Xscrbrc)
    coord Xscrbrc;

    #define rowtoend                    (sb->Xrowtoend)
    coord Xrowtoend;


/* ----------------------------- numbers.c ------------------------------- */

    #define refs_in_this_slot           (sb->Xrefs_in_this_slot)
    uchar Xrefs_in_this_slot;

    #define refs_in_this_sheet          (sb->Xrefs_in_this_sheet)
    uchar Xrefs_in_this_sheet;


    #define edtslr_col                  (sb->Xedtslr_col)
    colt Xedtslr_col;

    #define edtslr_row                  (sb->Xedtslr_row)
    rowt Xedtslr_row;


    #define slot_in_buffer              (sb->Xslot_in_buffer)
    BOOLEAN Xslot_in_buffer;

    #define output_buffer               (sb->Xoutput_buffer)
    BOOLEAN Xoutput_buffer;

    #define global_recalc               (sb->Xglobal_recalc)
    BOOLEAN Xglobal_recalc;

    #define recalc_bit                  (sb->Xrecalc_bit)
    BOOLEAN Xrecalc_bit;

    #define buffer_altered              (sb->Xbuffer_altered)
    BOOLEAN Xbuffer_altered;


/* ---------------------------- doprint.c -------------------------------- */

    #define wrpbit                      (sb->Xwrpbit)
    BOOLEAN Xwrpbit;

    #define jusbit                      (sb->Xjusbit)
    BOOLEAN Xjusbit;

    #define txnbit                      (sb->Xtxnbit)
    BOOLEAN Xtxnbit;

    BOOLEAN Xspare___;

    #define rcobit                      (sb->Xrcobit)
    BOOLEAN Xrcobit;

    #define minbit                      (sb->Xminbit)
    BOOLEAN Xminbit;

    #define borbit                      (sb->Xborbit)
    BOOLEAN Xborbit;

    #define iowbit                      (sb->Xiowbit)
    BOOLEAN Xiowbit;


    #define real_pagelength             (sb->Xreal_pagelength)
    intl Xreal_pagelength;

    #define real_pagcnt                 (sb->Xreal_pagcnt)
    intl Xreal_pagcnt;


/* ----------------------------- dialog.c -------------------------------- */

    #define encpln                      (sb->Xencpln)
    intl Xencpln;

    #define enclns                      (sb->Xenclns)
    intl Xenclns;

    #define pagoff                      (sb->Xpagoff)
    intl Xpagoff;

    #define pagnum                      (sb->Xpagnum)
    intl Xpagnum;

    #define filpof                      (sb->Xfilpof)
    intl Xfilpof;

    #define filpnm                      (sb->Xfilpnm)
    intl Xfilpnm;


/* --------------------------- riscos.c ---------------------------------- */

    #if RISCOS
    #define main_window                 (sb->Xmain_window)
    #define main__window                ((wimp_w) (sb->Xmain_window))
    riscos_window Xmain_window;
    #endif


/* ----------------------------------------------------------------------- */

#if RISCOS
    #define riscos_font_error           (sb->Xriscos_font_error)
    intl Xriscos_font_error;

    #define old_xf_fileloaded           (sb->Xold_xf_fileloaded)
    BOOL Xold_xf_fileloaded;

    #define lastcursorpos_x             (sb->Xlastcursorpos_x)
    coord Xlastcursorpos_x;

    #define lastcursorpos_y             (sb->Xlastcursorpos_y)
    coord Xlastcursorpos_y;

    #define xf_acquirecaret             (sb->Xxf_acquirecaret)
    BOOL Xxf_acquirecaret;
#endif


/* ----------------------------------------------------------------------- */

    #define file_is_help                (sb->Xfile_is_help)
    BOOL Xfile_is_help;


    #if RISCOS
    #define numericslotcontents         (sb->Xnumericslotcontents)
    char Xnumericslotcontents[LIN_BUFSIZ+1];

    #define main_dbox                   (sb->Xmain_dbox)
    #define main__dbox                  ((dbox) (sb->Xmain_dbox))
    void *Xmain_dbox;                   /* should be dbox but ... */

    #define grid_on                     (sb->Xgrid_on)
    BOOL Xgrid_on;

    #define charvspace                  (sb->Xcharvspace)
    intl Xcharvspace;
    #endif

    #define xf_interrupted              (sb->Xxf_interrupted)
    BOOLEAN Xxf_interrupted;

    #if RISCOS
    #define unused_bit_at_bottom        (sb->Xunused_bit_at_bottom)
    BOOLEAN Xunused_bit_at_bottom;
    #endif


/* ----------------------------- dialog.c -------------------------------- */

    #define d_save_FORMAT               (sb->Xd_save_FORMAT)
    uchar Xd_save_FORMAT;


    #define d_poptions_PL               (sb->Xd_poptions_PL) 
    uchar Xd_poptions_PL;

    #define d_poptions_LS               (sb->Xd_poptions_LS) 
    uchar Xd_poptions_LS;

    #define d_poptions_PS               (sb->Xd_poptions_PS) 
    uchar *Xd_poptions_PS;

    #define d_poptions_TM               (sb->Xd_poptions_TM) 
    uchar Xd_poptions_TM;

    #define d_poptions_HM               (sb->Xd_poptions_HM) 
    uchar Xd_poptions_HM;

    #define d_poptions_FM               (sb->Xd_poptions_FM) 
    uchar Xd_poptions_FM;

    #define d_poptions_BM               (sb->Xd_poptions_BM) 
    uchar Xd_poptions_BM;

    #define d_poptions_LM               (sb->Xd_poptions_LM) 
    uchar Xd_poptions_LM;

    #define d_poptions_HE               (sb->Xd_poptions_HE) 
    uchar *Xd_poptions_HE;

    #define d_poptions_FO               (sb->Xd_poptions_FO) 
    uchar *Xd_poptions_FO;


    #define d_options_DE                (sb->Xd_options_DE) 
    uchar *Xd_options_DE;

    #define d_options_TN                (sb->Xd_options_TN) 
    uchar Xd_options_TN;

    #define d_options_IW                (sb->Xd_options_IW) 
    uchar Xd_options_IW;

    #define d_options_BO                (sb->Xd_options_BO) 
    uchar Xd_options_BO;

    #define d_options_JU                (sb->Xd_options_JU) 
    uchar Xd_options_JU;

    #define d_options_WR                (sb->Xd_options_WR) 
    uchar Xd_options_WR;

    #define d_options_DP                (sb->Xd_options_DP) 
    uchar Xd_options_DP;

    #define d_options_MB                (sb->Xd_options_MB) 
    uchar Xd_options_MB;

    #define d_options_TH                (sb->Xd_options_TH) 
    uchar Xd_options_TH;

    #define d_options_IR                (sb->Xd_options_IR)
    uchar Xd_options_IR;

    #define d_options_DF                (sb->Xd_options_DF)
    uchar Xd_options_DF;

    #define d_options_LP                (sb->Xd_options_LP)
    uchar *Xd_options_LP;

    #define d_options_TP                (sb->Xd_options_TP)
    uchar *Xd_options_TP;

    #define d_options_GR                (sb->Xd_options_GR)
    uchar Xd_options_GR;


    #define d_recalc_AM                 (sb->Xd_recalc_AM) 
    uchar Xd_recalc_AM;

    #define d_recalc_RC                 (sb->Xd_recalc_RC) 
    uchar Xd_recalc_RC;

    #define d_recalc_RI                 (sb->Xd_recalc_RI) 
    uchar Xd_recalc_RI;

    #define d_recalc_RN                 (sb->Xd_recalc_RN) 
    uchar *Xd_recalc_RN;

    #define d_recalc_RB                 (sb->Xd_recalc_RB) 
    uchar *Xd_recalc_RB;


    #if RISCOS
    #define charvrubout_pos             (sb->Xcharvrubout_pos)
    intl Xcharvrubout_pos;

    #define textcell_xorg               (sb->Xtextcell_xorg)
    intl Xtextcell_xorg;

    #define textcell_yorg               (sb->Xtextcell_yorg)
    intl Xtextcell_yorg;
    #endif


    #define recalc_forced               (sb->Xrecalc_forced)
    intl Xrecalc_forced;


    #if RISCOS
    #define old_numcol                  (sb->Xold_numcol)
    colt Xold_numcol;

    #define old_numrow                  (sb->Xold_numrow)
    rowt Xold_numrow;

    #define old_rowsonscreen            (sb->Xold_rowsonscreen)
    coord Xold_rowsonscreen;
    #endif

    #define xf_fileloaded               (sb->Xxf_fileloaded)
    BOOL Xxf_fileloaded;

    #define rebuild_sheet_links         (sb->Xrebuild_sheet_links)
    intl Xrebuild_sheet_links;

    #define pict_on_screen              (sb->Xpict_on_screen)
    intl Xpict_on_screen;

    #define pict_was_on_screen          (sb->Xpict_was_on_screen)
    intl Xpict_was_on_screen;

    #define initial_filetype            (sb->Xinitial_filetype)
    char Xinitial_filetype;

    #if RISCOS
    #define numericslotcontents_length  (sb->Xnumericslotcontents_length)
    uchar Xnumericslotcontents_length;  /* very small number */

    #define slotcoordinates_size        16
    #define slotcoordinates             (sb->Xslotcoordinates)
    char Xslotcoordinates[slotcoordinates_size];

    #define riscos_fonts                (sb->Xriscos_fonts)
    BOOL Xriscos_fonts;

    #define fake_window                 (sb->Xfake_window)
    #define fake__window                ((wimp_w) (sb->Xfake_window))
    riscos_window Xfake_window;

    #define fake_dbox                   (sb->Xfake_dbox)
    #define fake__dbox                  ((dbox) (sb->Xfake_dbox))
    void *Xfake_dbox;                   /* should be dbox but ... */

    #define global_font                 (sb->Xglobal_font)
    char *Xglobal_font;

    #define global_font_x               (sb->Xglobal_font_x)
    intl Xglobal_font_x;

    #define global_font_y               (sb->Xglobal_font_y)
    intl Xglobal_font_y;

    #define global_font_leading         (sb->Xglobal_font_leading)
    intl Xglobal_font_leading;

    #define hbar_length                 (sb->Xhbar_length)
    intl Xhbar_length;

    #define pict_currow                 (sb->Xpict_currow)
    rowt Xpict_currow;

    #define pict_on_currow              (sb->Xpict_on_currow)
    rowt Xpict_on_currow;
    #endif

    #define old_lescroll                (sb->Xold_lescroll)
    intl Xold_lescroll;

    #define dspfld_from                 (sb->Xdspfld_from)
    intl Xdspfld_from;

    #define quitseen                    (sb->Xquitseen)
    BOOL Xquitseen;

    #if RISCOS
    #define curr_xext                   (sb->Xcurr_xext)
    intl Xcurr_xext;

    #define curr_yext                   (sb->Xcurr_yext)
    intl Xcurr_yext;

    #define delta_scx                   (sb->Xdelta_scx)
    intl Xdelta_scx;

    #define delta_scy                   (sb->Xdelta_scy)
    intl Xdelta_scy;
    #endif

    #define colsonscreen                (sb->Xcolsonscreen)
    coord Xcolsonscreen;

    #define n_colfixes                  (sb->Xn_colfixes)
    coord Xn_colfixes;

    #if RISCOS
    #define charvrubout_neg             (sb->Xcharvrubout_neg)
    intl Xcharvrubout_neg;

    #define vdu5textoffset              (sb->Xvdu5textoffset)
    intl Xvdu5textoffset;

    #define fontbaselineoffset          (sb->Xfontbaselineoffset)
    intl Xfontbaselineoffset;
    #endif

    #define old_lecpos                  (sb->Xold_lecpos)
    intl Xold_lecpos;

    #if defined(SHOW_CURRENT_ROW)
    #define out_rowborout               (sb->Xout_rowborout)
    BOOLEAN Xout_rowborout;

    #define out_rowborout1              (sb->Xout_rowborout1)
    BOOLEAN Xout_rowborout1;

    #define rowborout                   (sb->Xrowborout)
    coord Xrowborout;

    #define rowborout1                  (sb->Xrowborout1)
    coord Xrowborout1;
    #endif

    intl the_last_thing;
}
window_data;


#endif  /* MANY_DOCUMENTS */


/* exported procedures */

extern BOOL         create_new_document(void);
extern BOOL         create_new_untitled_document(void);
extern void         destroy_current_document(void);
extern intl         documents_modified(void);
extern dochandle    find_document_using_leafname(const char *name);
extern char        *find_leafname_using_handle(dochandle doc);
extern void         front_document_using_handle(dochandle doc);
extern void         init_window_data_once(void);
extern char        *leafname(const char *filename);
extern dochandle    next_document_handle(dochandle cdoc);
extern BOOL         mergebuf_all(void);
extern BOOL         set_untitled_document(void);
#if !MS
extern intl         stricmp(const char *a, const char *b);
extern intl         strnicmp(const char *a, const char *b, size_t n);
extern char        *stristr(const char *a, const char *b);
#endif
extern char        *strncatind(char *a, const char *b, size_t *np /*inout*/);


#if defined(TRACE_DOC)
extern window_data *current_document(void);
extern dochandle    current_document_handle(void);
extern window_data *find_document_using_handle(dochandle doc);
extern BOOL         is_current_document(void);
extern window_data *next_document(window_data *cp);
extern void         select_document(window_data *new_sb);
extern void         select_document_using_handle(dochandle newdoc);
extern void         trace_sb(void);
#else
#   define          current_document()                  (sb)

#   define          is_current_document()               \
                        (sb != NO_DOCUMENT)

#   define          current_document_handle()           \
                        (is_current_document() ? sb->DocHandle : DOCHANDLE_NONE)

#   define          find_document_using_handle(doc)     \
                        document_array[doc]

#   define          next_document(cp)                   \
                        ((cp == NULL) ? document_list : ((window_data *) cp)->link)

#   define          select_document(new_sb)         (sb = new_sb)

#   define          select_document_using_handle(doc)   \
                        (sb = document_array[doc])

#   define          trace_sb()
#endif

#if RISCOS
extern window_data *find_document_using_window(riscos_window window);
#endif


/* special values of window_data *types */

#if ARTHUR || RISCOS
#   define  NO_DOCUMENT ((window_data *) 0x70000000)
#elif MS
#   define  NO_DOCUMENT ((window_data *) 0x00000042)
#endif


/* special values of dochandle types */

#define DOCHANDLE_NONE      ((dochandle) 0x00)
#define DOCHANDLE_MAX       ((dochandle) 0x40)  /* 1..63 valid */
#define DOCHANDLE_SEVERAL   ((dochandle) 0xFF)


/* exported variables */

extern window_data *document_array[DOCHANDLE_MAX];

#endif  /* __pd__windvars_h */

/* end of windvars.h */
