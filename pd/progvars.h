/* progvars.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       progvars.h - global variables used in PipeDream 
 * Author:      RJM 8-Apr-1989
*/

#ifndef __pd__progvars_h
#define __pd__progvars_h

#if MS
#include "fastsc.ext"
#endif


/* ------------------------------ windvars.c ------------------------------- */

extern intl nDocuments;
extern intl NextUntitledNumber;

#if defined(SB_GLOBAL_REGISTER)
/* store this in v5 */
#pragma -r5
extern window_data *sb;
#pragma -r
#else
extern window_data *sb;
#endif

extern window_data *document_list;


/* ------------------------------ pdmain.c --------------------------------- */

extern int ctrlflag;

#if MS
extern BOOL autorepeat;
extern BOOL check_error;
#endif
extern char registration_number[22];


/* ------------------------------ savload.c -------------------------------- */

extern BOOL in_load;

#if !defined(Z88_OFF)
extern BOOL using_Z88;
extern BOOL Z88_on;
#else
#define using_Z88 FALSE 
#define Z88_on    FALSE 
#endif


/* --------------------------- mcdiff.c ------------------------------------ */

#if MS
extern BOOL fastdraw;
extern BOOL multiple_pages;
extern BOOL sync;
#endif

extern intl currently_inverted;
extern uchar highlights_on;

#if ARTHUR
extern uchar font_selected;
#endif


/* ----------------------------- commlin.c --------------------------------- */

#define NO_COMM ((intl) 0)          /* lukucm() is reporting error */
#define MAX_COMM_SIZE 4             /* maximum command length */

#define IS_SUBSET   -1
#define IS_ERROR    -2

extern uchar        alt_array[];
extern uchar       *buff_sofar;
extern uchar        cbuff[];                /* command line buffer */
extern intl         cbuff_offset;           /* length of string in cbuff */
extern BOOL         command_expansion;
extern uchar       *exec_ptr;
extern uchar        expanded_key_buff[];    /* for key expansions, always start with \ */
extern list_block  *first_command_redef;
extern MENU_HEAD    headline[];
extern intl         head_size;
extern BOOL         in_execfile;
extern BOOL         macro_recorder_on;
extern coord        this_heading_length;


/* ----------------------------- execs.c ----------------------------------- */

extern BOOL dont_save;

extern list_block *protected_blocks;


/* ----------------------------- pdsearch.c -------------------------------- */

#define NOCASEINFO -1
#define FIRSTUPPER 1
#define SECONDUPPER 2

#if RISCOS
/* no problems with prompt line */
#define in_search FALSE
#else
extern BOOL in_search;
#endif
extern dochandle schdochandle;
extern SLR sch_stt;             /* start of search block */
extern SLR sch_end;             /* end of search block */
extern SLR sch_pos_stt;         /* position of start of match */
extern SLR sch_pos_end;         /* position of end of match */
extern intl sch_stt_offset;     /* offset of search string in slot */
extern intl sch_end_offset;     /* offset of end of matched string */
extern BOOL dont_update_lescrl;

extern /* static */ uchar *next_replace_word;

#define WILD_FIELDS 9
extern /* static */ uchar wild_query[];
extern /* static */ intl wild_queries;

extern /* static */ intl wild_strings;

extern /* static */ list_block *wild_string;


/* ---------------------------- pdriver.c ---------------------------------- */

extern BOOL driver_loaded;  /* is there a driver loaded? */
extern BOOL hmi_as_text;
extern BOOL send_linefeeds;

extern list_block *highlight_list;

#if ARTHUR || RISCOS
#   define DEFAULT_LF   FALSE
#elif MS
#   define DEFAULT_LF   TRUE
#endif


/* ------------------------------- slector.c ------------------------------- */


/* ------------------------------- browse.c -------------------------------- */

extern char *word_to_invert;            /* invert a word in check screen display */

#if !defined(SPELL_OFF)

#if ARTHUR || RISCOS
#include "ext.spell"
#else
#include "spell.ext"
#endif

extern uchar word_to_insert[MAX_WORD];


#if defined(SPELL_BOUND)
#define spell_installed TRUE
#else
extern BOOL spell_installed;
#endif

extern list_block *first_spell;
extern list_block *first_user_dict;

extern /* static */ intl most_recent;   /* no. of most recently used used dictionary */

extern intl master_dictionary;
#endif


/* ------------------------------ help.c ----------------------------------- */


/* ------------------------------ viewio.c --------------------------------- */


/* ------------------------------- dialog.c -------------------------------- */

#if RISCOS
/* no special actions in dialog */
#define in_dialog_box FALSE
#else
extern BOOL in_dialog_box;
#endif
extern uchar *keyidx;           /* next character in soft key definition */


/* ---------------------------- bufedit.c -------------------------------- */


/* ---------------------------- reperr.c --------------------------------- */

extern BOOL been_error;         /* reperr sets this for others to notice */


/* ---------------------------- scdraw.c --------------------------------- */

extern BOOL allow_output;       /* sometimes errors are suppressed */
extern BOOL microspacing;

extern uchar smispc;            /* standard microspacing */
extern BOOL xf_flush;
extern BOOL xf_iovbit;
extern BOOL xf_leftright;


/* --------------------------- cursmov.c --------------------------------- */

extern SAVPOS saved_pos[];
extern intl saved_index;


/* --------------------------- numbers.c --------------------------------- */

extern BOOL activity_indicator;
extern list_block *draw_file_list;
extern list_block *draw_file_refs;


/* --------------------------- doprint.c --------------------------------- */

extern BOOL prnbit;         /* printer on */
extern BOOL sqobit;         /* sequential (non-screen) output */
extern BOOL micbit;         /* microspacing on */
extern BOOL spobit;         /* spool */

extern uchar off_at_cr;     /* 8 bits for turning highlight off at CR */
extern uchar h_waiting;     /* 8 bits determining which highlights are waiting */
extern uchar h_query;       /* 8 bits specifying whether the highlight is a ? field */

extern list_block *first_macro;     /* list of macro parameters */


/* --------------------------- lists.c ----------------------------------- */

extern list_block *def_first_option; /* default options read from pd.ini */
extern list_block *deleted_words;   /* list of deleted strings */
extern list_block *first_key;       /* first key expansion in chain */


/* --------------------------- slot.c ------------------------------------ */

extern colp def_colstart;           /* default column structure from pd.ini */
extern colt def_numcol;


/* ----------------------------- riscos.c -------------------------------- */

#if RISCOS
extern riscos_window caret_window;
#endif


extern dochandle blkdochandle;
extern SLR blkanchor;               /* block anchor */
extern SLR blkstart;                /* block start */
extern SLR blkend;                  /* block end */


/* ----------------------------------------------------------------------- */

#if MS || ARTHUR
extern intl calshe_break;

extern dochandle front_doc;
#endif

#if RISCOS
extern dochandle browsing_doc;
extern BOOL dumpdict_wants_nulls;
extern BOOL mergedict_wants_nulls;
extern dochandle pausing_doc;
extern dochandle anagram_doc;

extern MENU_HEAD *printer_font_mhptr;
extern intl printer_font_offset;
extern BOOL printer_font_attached;

/* on screen only! - printer conversion fixed */
#define ch_to_os(c) ((c) * charwidth)
#define os_to_mp(o) ((o) * x_scale)
#define ch_to_mp(c) os_to_mp(ch_to_os(c))
extern intl x_scale;
extern intl y_scale;

extern intl screen_x_os;
extern intl screen_y_os;

extern coord_box graphics_window;
extern coord_box cliparea;
extern coord_box thisarea;
extern BOOL paint_is_update;

extern intl riscos_font_xad;    /* mp coordinates of left hand baseline point */
extern intl riscos_font_yad;
extern BOOL riscos_printing;
extern BOOL draw_to_screen;

extern list_block *graphics_link_list;
#else
#define riscos_printing FALSE
#define draw_to_screen TRUE
#endif

extern BOOL currently_protected;
extern BOOL out_alteredstate;
extern BOOL whole_file_saved_ok;
extern BOOL exec_filled_dialog;

extern intl insert_reference_stt_offset;
extern intl insert_reference_end_offset;
extern SLR  insert_reference_slr;

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

#if TRACE || TRUE
extern BOOL trace_enabled;
#endif

#endif  /* __pd__progvars_h */

/* end of progvars.h */
