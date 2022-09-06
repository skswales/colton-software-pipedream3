/* progvars.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       progvars.c - global variables used in PipeDream 
 * Author:      RJM 8-Apr-1989
*/

/* header file */
#include "datafmt.h"

#if RISCOS
#include "ext.riscos"
#endif


#define DONT_CARE 0


/* ----------------------------- windvars.c ------------------------------ */

intl nDocuments         = 0;
intl NextUntitledNumber = 1;

#if defined(SB_GLOBAL_REGISTER)
/* use global integer to store this much-accessed global */
#else
/* pointer to current document window statics */
window_data *sb = NO_DOCUMENT;
#endif

window_data *document_list = NULL;  /* list of all the documents */


/* ----------------------------- pdmain.c -------------------------------- */

int ctrlflag = 0;

#if MS
BOOL autorepeat = FALSE;
BOOL check_error = FALSE;
#endif
char registration_number[22];


/* ----------------------------- savload.c ------------------------------- */

BOOL in_load = FALSE;

#if !defined(Z88_OFF)
BOOL using_Z88 = FALSE;
BOOL Z88_on = FALSE;
#endif


/* ----------------------------- mcdiff.c -------------------------------- */

#if MS
BOOL fastdraw;      /* these three are all read from install string */
BOOL multiple_pages;
BOOL sync;
#endif

intl currently_inverted;
uchar highlights_on = FALSE;


/* ----------------------------- commlin.c ------------------------------- */

uchar alt_array[MAX_COMM_SIZE+4];   /* only ever one Alt sequence going */
uchar *buff_sofar = NULL;
uchar cbuff[CMD_BUFSIZ];            /* command line buffer */
intl cbuff_offset = 0;              /* length of string in cbuff */
BOOL command_expansion = FALSE;
uchar *exec_ptr = NULL;
uchar expanded_key_buff[8]  = { CMDLDI, '\0' }; /* for key expansions, always start with \ */
list_block *first_command_redef = NULL;
BOOL in_execfile = FALSE;
BOOL macro_recorder_on = FALSE;


/* ----------------------------- execs.c --------------------------------- */

BOOL dont_save = FALSE;

list_block *protected_blocks = NULL;


/* ----------------------------- pdsearch.c ------------------------------ */

#if RISCOS
/* no problems with prompt line */
#else
BOOL in_search = FALSE;
#endif

dochandle schdochandle = DOCHANDLE_NONE;
SLR sch_stt;            /* start of search block */
SLR sch_end;            /* end of search block */
SLR sch_pos_stt;        /* position of start of match */
SLR sch_pos_end;        /* position of end of match */
intl sch_stt_offset;        /* offset of search string in slot */
intl sch_end_offset;        /* offset of end of matched string */
BOOL dont_update_lescrl = FALSE;

/* static */ uchar *next_replace_word = NULL;

/* static */ uchar wild_query[WILD_FIELDS];
/* static */ intl wild_queries = 0;

/* static */ intl wild_strings = 0;

/* static */ list_block *wild_string = NULL;


/* ---------------------------- browse.c ----------------------------------- */

char *word_to_invert = NULL;        /* invert a given word in check screen display */

#if !defined(SPELL_OFF)

#if !defined(SPELL_BOUND)
BOOL spell_installed = FALSE;
#endif

/* static */ uchar word_to_insert[MAX_WORD];

list_block *first_spell = NULL;
list_block *first_user_dict = NULL;

/* static */ intl most_recent = -1; /* no. of most recently used used dictionary */

intl master_dictionary = -1;

#endif


/* ---------------------------- dialog.c --------------------------------- */

#if RISCOS
/* no special actions for when in dialog */
#else
BOOL in_dialog_box = FALSE;
#endif

uchar *keyidx = NULL;           /* next character in soft key definition */


/* ---------------------------- equip.c ---------------------------------- */

#if MS

/* local header file */
#include "fastsc.h"

struct Vstate Vcs;      /* current video state */
struct Vstate Vos;      /* old video state */

#endif


/* ---------------------------- scdraw.c ----------------------------------- */

BOOL xf_leftright   = FALSE;
BOOL xf_iovbit      = TRUE;
BOOL xf_flush       = FALSE;
BOOL allow_output   = TRUE;     /* sometimes errors are suppressed */
BOOL microspacing   = FALSE;
uchar smispc = 1;               /* standard microspacing */


/* ---------------------------- cursmov.c ----------------------------------- */

SAVPOS saved_pos[SAVE_DEPTH];
intl saved_index = 0;


/* ---------------------------- numbers.c ----------------------------------- */

BOOL activity_indicator = FALSE;
list_block *draw_file_list = NULL;
list_block *draw_file_refs = NULL;


/* ---------------------------- doprint.c ----------------------------------- */

BOOL prnbit = FALSE;        /* printer on */
BOOL sqobit = FALSE;        /* sequential (non-screen) output */
BOOL micbit = FALSE;        /* microspacing on */
BOOL spobit = FALSE;        /* spool */

uchar off_at_cr = 0xFF;     /* 8 bits for turning highlight off at CR */
uchar h_waiting = 0;        /* 8 bits determining which highlights are waiting */
uchar h_query   = 0;        /* 8 bits specifying whether the highlight is a ? field */

list_block *first_macro = NULL;     /* list of macro parameters */


/* ---------------------------- pdriver.c ----------------------------------- */

BOOL driver_loaded = FALSE;         /* no driver loaded initially */
BOOL hmi_as_text = FALSE;
BOOL send_linefeeds = DEFAULT_LF;

list_block *highlight_list = NULL;


/* ---------------------------- reperr.c ------------------------------------ */

BOOL been_error = FALSE;        /* reperr sets this for others to notice */


/* ---------------------------- lists.c ------------------------------------- */

list_block *def_first_option = NULL;    /* default options read from pd.ini */
list_block *deleted_words    = NULL;    /* list of deleted words */
list_block *first_key        = NULL;    /* list of key definitions */


/* ---------------------------- slot.c ------------------------------------- */

colp def_colstart = NULL;           /* default column structure from pd.ini */
colt def_numcol = 0;


/* ---------------------------- riscos.c ----------------------------------- */

#if RISCOS
riscos_window caret_window = window_NULL;
#endif


/* ------------------------------------------------------------------------- */

dochandle blkdochandle = DONT_CARE;
SLR blkanchor   = { NO_COL, 0 };    /* block anchor */
SLR blkstart    = { NO_COL, 0 };    /* block start */
SLR blkend      = { NO_COL, 0 };    /* block end */


#if MS || ARTHUR
intl calshe_break = 0;
 
dochandle front_doc = DOCHANDLE_NONE;

#endif

#if RISCOS
dochandle browsing_doc  = DOCHANDLE_NONE;
BOOL dumpdict_wants_nulls   = FALSE;
BOOL mergedict_wants_nulls  = FALSE;
dochandle pausing_doc   = DOCHANDLE_NONE;
dochandle anagram_doc   = DOCHANDLE_NONE;

MENU_HEAD *printer_font_mhptr;
intl printer_font_offset;
BOOL printer_font_attached = FALSE;
intl x_scale;                           /* OS units per millipoint */
intl y_scale;                           /* OS units per millipoint */
intl screen_x_os;                       /* width of screen in OS units */
intl screen_y_os;                       /* height of screen in OS units */


/* boxes are inclusive, inclusive, exclusive, exclusive */

/* current absolute graphics window coordinates set by Window manager
 *
 * NB. this is in OS units
*/
coord_box graphics_window;


/* current relative graphics window coordinates set by Window manager
 *
 * NB. this is in text cells (y0 >= y1)
*/
coord_box cliparea;     /* left, bottom, off right, off top */


/* currently intersected object coordinates to save recalculations
 * between the maybe_ and really_ phases of drawing an object.
 *
 * Important: can't be used between the draw_ and maybe_ phases as only
 * the Window manager knows what is going on in the world nowadays.
*/
coord_box thisarea;


/* whether screen paint is an update caused by us (background not cleared)
 * or a redraw caused by Window Manager (background already cleared to BACK)
 * still needed for RISC OS printing
*/
BOOL paint_is_update;


intl riscos_font_xad;
intl riscos_font_yad;
BOOL riscos_printing = FALSE;
BOOL draw_to_screen = TRUE;

list_block *graphics_link_list = NULL;
#endif

BOOL currently_protected;
BOOL out_alteredstate = FALSE;          /* doesn't need to be in windvars */
BOOL whole_file_saved_ok;

BOOL exec_filled_dialog = FALSE;        /* was dialog box filled in by exec file? */

intl insert_reference_stt_offset;
intl insert_reference_end_offset;
SLR  insert_reference_slr;

#if TRACE || TRUE
BOOL trace_enabled = FALSE;
#endif

/* end of progvars.c */
