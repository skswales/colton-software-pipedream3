/* datafmt.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Title:       datafmt.h - defines the data structures for PipeDream
 * Author:      RJM August 1987
*/

#define FREEBIE 1 /* SKS October 2021 */


/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#   include "ext.version"
#elif MS
#   include "version.ext"
#else
#   error PipeDream can only be made for ARTHUR, RISCOS or MS-DOS machines
#endif


/* get data type definitions */
#include "datatype.h"

/* get string definitions */
#include "strings.h"

#if MS
#include "install.h"
#endif

/* get declarations of variables for each window/file */
#include "windvars.h"

/* get declarations of globals */
#include "progvars.h"

/* get stubs for XXfunc for PC overlays */
#include "stubs.h"

/* get declarations of marked block functions */
#include "markers.h"

#define bad_row(trow)           (((trow) < (rowt) 0)  ||  ((trow) & BADROWBIT))
extern rowt (bad_row)(rowt trow);

#define bad_col(tcol)           (((tcol) < (colt) 0)  ||  ((tcol) & BADCOLBIT))
extern colt (bad_col)(colt tcol);

#define abs_row(trow)           ((trow) & ABSROWBIT)
extern rowt (abs_row)(rowt trow);

#define abs_col(tcol)           ((tcol) & ABSCOLBIT)
extern colt (abs_col)(colt tcol);


#define force_abs_row(trow)     ((trow) |= ABSROWBIT)
extern rowt (force_abs_row)(rowt trow);

#define force_abs_col(tcol)     ((tcol) |= ABSCOLBIT)
extern colt (force_abs_col)(colt tcol);


#define force_not_abs_row(trow) ((trow) &= (ROWNOBITS | BADROWBIT))
extern rowt (force_not_abs_row)(rowt trow);

#define force_not_abs_col(tcol) ((tcol) &= (COLNOBITS | BADCOLBIT))
extern colt (force_not_abs_col)(colt tcol);


/****************************************************************************
* character definitions
****************************************************************************/

#define TAB                     ((uchar) 0x09)
#define FORMFEED                ((uchar) 0x0C)
#define FUNNYSPACE              ((uchar) 0x0E)
#define SLRLDI                  ((uchar) 0x10)
#define FIRST_HIGHLIGHT         ((uchar) 0x18)
#define FIRST_HIGHLIGHT_TEXT    ((uchar) '1')
#define CMDLDI                  (FIRST_HIGHLIGHT + 1)
#define CTRLZ                   ((uchar) 0x1A)
#define ESCAPE                  ((uchar) 0x1B)
#define LAST_HIGHLIGHT          ((uchar) 0x1F)
#define LAST_HIGHLIGHT_TEXT     ((LAST_HIGHLIGHT - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT)
#define SPACE                   ((uchar) 0x20)
#define DUMMYHAT                FORMFEED
#define COMMA                   ((uchar) ',')
#define BACKSLASH               ((uchar) '\\')
#define FORESLASH               ((uchar) '/')
#define COLON                   ((uchar) ':')
#define SINGLE_QUOTE            ((uchar) '\'')
#define QUOTES                  ((uchar) '\"')
#define CTRLDI                  ((uchar) '|')
#define QUERY                   ((uchar) '?')
#define DOT                     ((uchar) '.')
#define DELETE                  ((uchar) 0x7F)

typedef enum
{
    HIGH_UNDERLINE      =   FIRST_HIGHLIGHT + 1 - 1,
    HIGH_BOLD           =   FIRST_HIGHLIGHT + 2 - 1,
    HIGH_ITALIC         =   FIRST_HIGHLIGHT + 4 - 1,
    HIGH_SUBSCRIPT      =   FIRST_HIGHLIGHT + 5 - 1,
    HIGH_SUPERSCRIPT    =   FIRST_HIGHLIGHT + 6 - 1
}
highlight_numbers;

typedef enum
{
    N_UNDERLINE     =   1,  /* highlight 1 */
    N_BOLD          =   2,  /* highlight 2 */
    N_ITALIC        =   8,  /* highlight 4 */
    N_SUBSCRIPT     =   16, /* highlight 5 */
    N_SUPERSCRIPT   =   32  /* highlight 6 */
}
highlight_bits;

#define ishighlight(ch)         ((ch <= LAST_HIGHLIGHT)  &&  (FIRST_HIGHLIGHT <= ch))
extern BOOL (ishighlight)(intl ch);

#define ishighlighttext(ch)     ((ch <= LAST_HIGHLIGHT_TEXT)  &&  (FIRST_HIGHLIGHT_TEXT <= ch))
extern BOOL (ishighlighttext)(intl ch);


#if ARTHUR || RISCOS
#   define inversehighlight(ch) (ishighlight(ch) && (ch != FIRST_HIGHLIGHT  ||  ch != FIRST_HIGHLIGHT+2  ||  ch != FIRST_HIGHLIGHT+1))
#elif MS
#   define inversehighlight(ch) (ishighlight(ch) && ch != FIRST_HIGHLIGHT+1)
#else
#   define inversehighlight(ch) (ishighlight(ch))
#endif


#if ARTHUR || RISCOS
#define OS_DEL          ((uchar) '\0')      /* OSCLI delimiter for system() */
#define OSPRMT          ((uchar) '*')       /* OSCLI prompt */

#elif MS
#define OS_DEL          ((uchar) '\0')      /* MESSDOS delimiter for system() */
#define OSPRMT          ((uchar) '>')       /* MESSDOS prompt */

#endif


/******************************************************************************
* colours
******************************************************************************/

#if ARTHUR || RISCOS
#if ARTHUR
#define black           0
#define white           7
#define lightblue       6
#define red             1
#endif

#define FORESTR         "7"
#define BACKSTR         "0"
#define BORDERSTR       "2"
#if ARTHUR
#define MENU_FORESTR    "0"
#define MENU_BACKSTR    "6"
#define MENU_HOTSTR     "1"
#endif
#define NEGATIVESTR     "11"
#define PROTECTSTR      "14"
#define CARETSTR        "11"
#define CURBORDERSTR    "4"

#define LARGEST_COLOUR 127

#elif MS
#define black           0
#define white           7
#define lightblue       3
#define red             4

#define BOLDMASK        0x08

#define FORESTR         "7"
#define BACKSTR         "0"
#define BORDERSTR       "3"
#define MENU_FORESTR    "0"
#define MENU_BACKSTR    "3"
#define MENU_HOTSTR     "4"
#define NEGATIVESTR     FORESTR
#define PROTECTSTR      FORESTR

#define LARGEST_COLOUR  7

#endif


/****************************************************************************
* key & function key assignments
****************************************************************************/

/* normal chars come back from rdch() in 0x00..FF
 * some chars get bits added as to whether shifting keys pressed
 * function keys come back as -ve values
*/

#if ARTHUR || RISCOS
/*
 * BBC specific function key definitions
 * F1 = 0,129; SF1 = 0,F1+16; CF1 = 0,SF1+16; CSF1 = 0,CF1+16
 * AF1 = 0,225 provided through ARTHUR INSV translation code
*/
#define          FUNC   (-129)
#define         SFUNC   ( FUNC-16)
#define         CFUNC   (SFUNC-16)
#define        CSFUNC   (CFUNC-16)
#if ARTHUR
#define       ALTFUNC   (-225)
#endif

#define         FUNC10  (-202)
#define        SFUNC10  ( FUNC10-16)
#define        CFUNC10  (SFUNC10-16)
#define       CSFUNC10  (CFUNC10-16)
#if ARTHUR
#define      ALTFUNC10  (-240)
#endif

#define            SHIFT    -1
#define             CTRL    -2
#define              ALT    -3
#define DEPRESSED_ESCAPE    0
                                /* values for inkey in depressed(n) */

#define LEFTDELETE      0x08
#define HOMEKEY         30
#define RUBOUT          0x7F


#define PRINTKEY        (  FUNC+1)          /* 'F0' */

#define        TAB_KEY  (  FUNC-9)
#define      SHIFT_TAB  ( SFUNC-9)
#define       CTRL_TAB  ( CFUNC-9)
#define CTRL_SHIFT_TAB  (CSFUNC-9)

#define ENDKEY          (  FUNC-10)

#define LEFTCURSOR      (  FUNC-11)
#define SLEFTCURSOR     ( SFUNC-11)
#define CLEFTCURSOR     ( CFUNC-11)
#define CSLEFTCURSOR    (CSFUNC-11)

#define RIGHTCURSOR     (  FUNC-12)
#define SRIGHTCURSOR    ( SFUNC-12)
#define CRIGHTCURSOR    ( CFUNC-12)
#define CSRIGHTCURSOR   (CSFUNC-12)

#define DOWNCURSOR      (  FUNC-13)
#define SDOWNCURSOR     ( SFUNC-13)
#define CDOWNCURSOR     ( CFUNC-13)
#define CSDOWNCURSOR    (CSFUNC-13)

#define UPCURSOR        (  FUNC-14)
#define SUPCURSOR       ( SFUNC-14)
#define CUPCURSOR       ( CFUNC-14)
#define CSUPCURSOR      (CSFUNC-14)

#define DELETECHAR      (  FUNC10-0)    /* F10 */
#define WRAPRIGHT       ( SFUNC10-0)
#define FORMAT          ( CFUNC10-0)

#define SWAP_POS        (  FUNC10-1)    /* F11 */
#define SAVE_POS        ( SFUNC10-1)
#define RESTORE_POS     ( CFUNC10-1)

/* F12 alone is goto star prompt */
#define CENTRE_SCREEN   ( SFUNC10-2)
/* CF12 undefined */

#define INSERTKEY       (  FUNC10-3)    /* 'F13' */


#elif MS
/* MS-DOS specific function key definitions */

#define              FUNC -59
#define             SFUNC -84
#define             CFUNC -94
#define            CSFUNC -104

#define             SHIFT   3
#define              CTRL   4
#define               ALT   8
                                        /* values for inkey in depressed(n) */

#define RUBOUT          0x08

#define        TAB_KEY  TAB
#define      SHIFT_TAB  -15
#define       CTRL_TAB  (CTRL_ADDED + 4)
#define CTRL_SHIFT_TAB  (CTRL_ADDED + SHIFT_ADDED + 4)

#define UPCURSOR        (FUNC-13)
#define DOWNCURSOR      (FUNC-21)
#define LEFTCURSOR      (FUNC-16)
#define RIGHTCURSOR     (FUNC-18)

#define SUPCURSOR       (FUNC-14)
#define SDOWNCURSOR     (FUNC-22)
#define SLEFTCURSOR     (SHIFT_ADDED + 2)
#define SRIGHTCURSOR    (SHIFT_ADDED + 3)

#define CUPCURSOR       (FUNC-12)
#define CDOWNCURSOR     (FUNC-20)
#define CLEFTCURSOR     (CFUNC-21)
#define CRIGHTCURSOR    (CFUNC-22)

#define CSUPCURSOR      (CTRL_ADDED + SHIFT_ADDED + 0)
#define CSDOWNCURSOR    (CTRL_ADDED + SHIFT_ADDED + 1)
#define CSLEFTCURSOR    (CTRL_ADDED + SHIFT_ADDED + 2)
#define CSRIGHTCURSOR   (CTRL_ADDED + SHIFT_ADDED + 3)

#define CENTRE_SCREEN   71

#define INSERTKEY       (-82)
#define DELETEKEY       (-83)

#define PRINTKEY        0

#define SAVE_POS        0
#define SWAP_POS        0
#define RESTORE_POS     0

#endif


/* Common function key assignments: FUNC-x == F(x+1) */

typedef enum
{
    HELP            = (FUNC-0),
    EXPRESSION      = (FUNC-1),
    MARKSLOT        = (FUNC-2),
    DELEOL          = (FUNC-3),
    FIRSTCOL        = (FUNC-4),
    LASTCOL         = (FUNC-5),
    INSERTROW       = (FUNC-6),
    DELETEROW       = (FUNC-7),
    INSERTCHAR      = (FUNC-8),

    MOVEBLOCK       = (SFUNC-0),
    SWAPCASE        = (SFUNC-1),
    CLEARMARKS      = (SFUNC-2),
    DELETEWORD      = (SFUNC-3),
    PREVFILE        = (SFUNC-4),
    NEXTFILE        = (SFUNC-5),
    INSROWINCOL     = (SFUNC-6),
    DELROWINCOL     = (SFUNC-7),
    WRAPLEFT        = (SFUNC-8),

    DELETEBLOCK     = (CFUNC-0),
    NEXTMATCH       = (CFUNC-1),
    INSERTCOL       = (CFUNC-2),
    INSERTPAGE      = (CFUNC-3),
    INSOVER         = (CFUNC-4),
    INSREF          = (CFUNC-5),
    SPLITLINE       = (CFUNC-6),
    JOINLINES       = (CFUNC-7),
    ADDCOL          = (CFUNC-8)

#if MS
,   DELETECHAR      = (FUNC-9)
,   WRAPRIGHT       = (SFUNC-9)
,   FORMAT          = (CFUNC-9)
#endif
}
function_key_values;


typedef enum
{
    SHIFT_ADDED = 0x100,
    CTRL_ADDED  = 0x200,
    ALT_ADDED   = 0x400
}
extra_char_bits;


/****************************************************************************
* movement manifests
****************************************************************************/

#define SCREENFUL   64
#define END         128

#define CURSOR_UP       1
#define CURSOR_DOWN     2
#define CURSOR_PREV_COL 3
#define CURSOR_NEXT_COL 4

#define CURSOR_SUP      (SCREENFUL + CURSOR_UP)
#define CURSOR_SDOWN    (SCREENFUL + CURSOR_DOWN)
#define CURSOR_FIRSTCOL (END + CURSOR_FIRST_COL)
#define CURSOR_LASTCOL  (END + CURSOR_NEXT_COL)

#define ABSOLUTE (0x80)


/****************************************************************************
* screen update flags
****************************************************************************/


/****************************************************************************
* pdmain.c
****************************************************************************/

/* exported functions */

extern BOOL act_on_c(intl c);
extern void exec_file(const char *filename);
extern intl install_pipedream(char *name);
extern int  main(int argc, char *argv[]);
extern intl read_username(char *buffer);


/* exported variables */

extern intl installed_ok;


#if MS
/****************************************************************************
* ctrlc.obj
****************************************************************************/

extern capture(int *);
extern release(void);

#endif

/****************************************************************************
* slot.c
****************************************************************************/

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
extern slotp travel_externally(dochandle han, colt tcol, rowt row);
extern slotp travel_here(void);
extern slotp travel_in_block(void);

/*
allocator parameters
*/

#if MS
#define MAXPOOLSIZE  4000
#define MAXITEMSIZE  350
#define MAXFREESPACE 4000
#elif ARTHUR || RISCOS
#define MAXPOOLSIZE  5000
#define MAXITEMSIZE  350
#define MAXFREESPACE 5000
#endif


/****************************************************************************
* mcdiff.c
****************************************************************************/

/* exported functions */

extern void ack_esc(void);
extern void bleep(void);
extern void clearkeyboardbuffer(void);
extern void clearscreen(void);      /* clear default screen, VDU 4 text */
extern void curon(void);
extern BOOL depressed(intl key);
extern BOOL depressed_ctrl(void);
extern BOOL depressed_shift(void);
extern void down_scroll(coord first_line);
extern void eraeol(void);
extern BOOL escape_disable(void);
extern BOOL escape_disable_nowinge(void);
extern void init_mc(void);
extern void invert(void);
extern BOOL keyinbuffer(void);
extern void mycls(coord leftx, coord boty, coord rightx, coord topy);
extern void reset_mc(void);
extern coord scrnheight(void);
extern coord scrnwidth(void);
extern void setcolour(intl fore, intl back);
extern coord stringout(const char *str);
extern intl translate_received_key(intl key);
extern void up_scroll(coord first_line);
extern void wrchrep(uchar ch, coord i);

#if RISCOS
extern void clearmousebuffer(void);
extern void escape_enable(void);
extern intl getcolour(intl colour);
extern BOOL keyormouseinbuffer(void);
extern BOOL mouseinbuffer(void);
extern intl rdch_riscos(void);
extern void setbgcolour(intl colour);
extern void setfgcolour(intl colour);
extern void wrch_definefunny(intl ch);
extern void wrch_funny(intl ch);
extern void wrch_undefinefunnies(void);
/* can always undefine the lot in current use */
#define wrch_undefinefunny(ch)  wrch_undefinefunnies()
#else
#define escape_enable()         ack_esc()       /* ensure none pending */
#define keyormouseinbuffer()    keyinbuffer()
#define mouseinbuffer()         FALSE
#endif

#if ARTHUR
extern void bodge_funnies_for_master_font(uchar *str);
extern intl rdch_arthur(BOOL curs);
extern void setlogcolours(void);
#endif

#if MS
extern void curoff(void);
extern void my_int86(int n, uchar ah, uchar al, uchar bh, uchar bl,
                            uchar ch, uchar cl, uchar dh, uchar dl);
extern intl rdch(BOOL cursor, BOOL doshow);
extern coord read_lineno(void);
extern void restore_screen(coord topx, coord topy, coord xlen, coord ylen);
extern void wrch(uchar x);
#endif

#if ARTHUR || RISCOS
extern intl fx_x(intl a, intl x, intl y);
extern intl fx_x2(intl a, intl x);
extern intl fx_y(intl a, intl x, intl y);
extern char *mysystem(const char *str);
extern void stampfile(const char *name, int filetype);
extern void wrch_h(char ch);
#endif

#if MS || RISCOS
extern void at(coord x, coord y);
extern void ospca(coord nspaces);
#endif

#if MS

#if 0
extern void draw_grid_vbar(void);
#else
#define draw_grid_vbar()
#endif

#endif


#if MS || ARTHUR
extern void clip_menu(coord topx, coord topy, coord xlen, coord ylen);
extern void init_screen_array(void);
extern size_t maxblock(void);
extern void save_screen(coord topx, coord topy, coord xsize, coord ysize);
#endif


/* exported variables */

/* macro definitions */

#define invoff()                if(currently_inverted) invert()

#if MS || ARTHUR
#define ospca(i)                wrchrep(SPACE, i)
extern void (ospca)(coord i);

#define setfgcolour(fg)     setcolour(fg, BACK)
extern void (setfgcolour)(coord fg);
#endif


#if RISCOS
#define curoff()                /* nothing */
extern void (curoff)(void);

#define rdch(x, y)              rdch_riscos()
extern intl (rdch)(BOOL curs, BOOL show);

#define read_lineno()           ((coord) 0)
extern coord (read_lineno)(void);

#define init_screen_array()     /* nothing */
extern void (init_screen_array)(void);

#define save_screen(a,b,c,d)    /* nothing */
extern void (save_screen)(coord topx, coord topy, coord xsize, coord ysize);
#endif


#if ARTHUR
#define at(x, y)                bbc_vduq(31, x, y)
extern void (at)(intl x, intl y);

#define curoff()                bbc_cursor(0)
extern void (curoff)(void);

#define rdch(x, y)              rdch_arthur(x)
extern intl (rdch)(BOOL curs, BOOL show);

#define read_lineno()           ((coord) bbc_vpos())
extern coord (read_lineno)(void);
#endif


#if MS
#define sb_show_if_fastdraw()  if(fastdraw) sb_show()


#define DOWN_ARROW  25
#define LEFT_ARROW  27
#define RIGHT_ARROW 26

#define VERTBAR     179
#define HORIZBAR    196
#define TOPLEFT     218
#define TOPRIGHT    191
#define BOTLEFT     192
#define BOTRIGHT    217

#define DROP_LEFT   195     /* vertical bar middle bar pointing right */
#define DROP_RIGHT  180
#define DROP_MIDDLE 193     /* horizontal bar with middle bar pointing up */

#define DOUBLE_VERTBAR  186
#define DOUBLE_HORIZBAR 205
#define DOUBLE_TOPLEFT  201
#define DOUBLE_TOPRIGHT 187
#define DOUBLE_BOTLEFT  200
#define DOUBLE_BOTRIGHT 188

/* on PCs bold can be done by setting bit four of the character attribute */
#define BOLD 8
#endif


#if ARTHUR || RISCOS
#define sb_show_if_fastdraw()   /* nothing */
extern void (sb_show_if_fastdraw)(void);

#define wrch(x)                 bbc_vdu((int) x)
extern void (wrch)(intl x);


/* deal with refinition of characters */

#if RISCOS
#define font_selected   129
#define FIRST_FUNNY     (font_selected)
#elif ARTHUR
#define MASTER_FONT     161
#define OTHER_FONT      129
#define FIRST_FUNNY     OTHER_FONT
#endif

/* these must always be printed using wrch_definefunny(ch),wrchrep(ch,n)
 * or wrch_funny(ch)
*/

#define COLUMN_DOTS     (font_selected)
#define DOWN_ARROW      (font_selected + 1)

#if ARTHUR
#define UP_ARROW        (font_selected + 2)
#define LEFT_ARROW      (font_selected + 3)
#define RIGHT_ARROW     (font_selected + 4)

#define VERTBAR         (font_selected + 4)
#define HORIZBAR        (font_selected + 5)
#define TOPLEFT         (font_selected + 6)
#define TOPRIGHT        (font_selected + 7)
#define BOTLEFT         (font_selected + 8)
#define BOTRIGHT        (font_selected + 9)

#define DROP_LEFT       (font_selected + 10)
#define DROP_MIDDLE     (font_selected + 11)
#define DROP_RIGHT      (font_selected + 12)

#define LAST_FUNNY      (FIRST_FUNNY + 13)
#endif

#endif


#if MS || RISCOS
#define bodge_funnies_for_master_font(a)    /* nothing */
extern void (bodge_funnies_for_master_font)(uchar *a);

#define setlogcolours()         /* nothing */
extern void (setlogcolours)(void);
#endif


#if MS || ARTHUR
#define wrch_funny(ch)           wrch(ch)
extern void (wrch_funny)(intl ch);

#define wrch_definefunny(ch)    /* nothing */
extern void (wrch_definefunny)(intl ch);

#define wrch_undefinefunnies()  /* nothing */
extern void (wrch_undefinefunnies)(void);
#endif


/****************************************************************************
* savload.c
****************************************************************************/

/* exported functions */

extern void block_updref(colt startcol, colt coffset);
extern BOOL excloa(const char *filename);
extern BOOL excsav(const char *name, char filetype, BOOL initfile);
extern BOOL excsav_core(const char *name, char filetype, BOOL initfile);
extern intl find_file_type(const char *name);
extern char *fndfcl(void);
extern BOOL iniglb(void);
extern BOOL loadfile(char *name, BOOL new_window, BOOL dont_load_as_list);
extern intl nxfloa(void);
extern BOOL plain_slot(slotp tslot, colt tcol, rowt trow,
                       char filetype, uchar *buffer);
extern BOOL save_existing(void);
extern void save_opt_to_file(FILE *output, DIALOG *start, intl n);
extern void set_width_and_wrap(colt tcol, coord width, coord wrapwidth);

#if RISCOS
extern intl rft_from_option(intl option);
extern void restore_current_window_pos(void);
extern void save_current_window_pos(void);
#else
#define restore_current_window_pos()
#define save_current_window_pos()

#define xferrecv_file_is_safe() TRUE
#define xfersend_file_is_safe() TRUE
#endif


/* Limit imposed by wimp xfer block */
#define MAX_FILENAME 212

#define inserting (d_load[1].option == 'Y')

#define saving_part_file()  ((d_save[SAV_COLRANGE].option == 'Y')   ||  \
                             (d_save[SAV_ROWCOND].option == 'Y')    ||  \
                             (d_save[SAV_BLOCK].option == 'Y')      )


/* flags to determine whether loadfile_recurse creates a new window */
typedef enum
{
    EXISTING_WINDOW = FALSE,
    NEW_WINDOW      = TRUE
}
loadfile_recurse_create_flags;


typedef enum
{
    SAV_NAME,
    SAV_COLRANGE,
    SAV_ROWCOND,
    SAV_BLOCK,
    SAV_LINESEP,        /* position of CRLF status in dialog box */
    SAV_xxxFORMAT       /* position of save format in dialog box - use windvar! d_save_FORMAT */
}
d_save_offsets;


typedef enum
{
#if ARTHUR || RISCOS
    SAV_LSEP_LF,
    SAV_LSEP_CR,
    SAV_LSEP_CRLF,
    SAV_LSEP_LFCR
#elif MS
    SAV_LSEP_CRLF,
    SAV_LSEP_CR,
    SAV_LSEP_LF,
    SAV_LSEP_LFCR
#endif
}
d_save_linesep_offsets;


/* File types */

#define AUTO_CHAR   'A'
#define CSV_CHAR    'C'
#define DTP_CHAR    'D'
#define LOTUS_CHAR  'L'
#define PD_CHAR     'P'
#define SHEET_CHAR  'S'
#define TAB_CHAR    'T'
#define VIEW_CHAR   'V'


/****************************************************************************
* pdriver.c
****************************************************************************/

/* exported functions */

extern void load_driver(void);  /* load a printer driver: called on startup and by PDfunc */


/****************************************************************************
* slector.c
****************************************************************************/

/* exported functions */

extern char *   add_prefix_to_name(char *buffer /*out*/, const char *name, BOOL allow_cwd);
extern BOOL     add_path(char *filename /*inout*/, const char *src, BOOL allow_cwd);
extern BOOL     checkoverwrite(const char *filename);
extern BOOL     filereadable(const char *filename);
extern BOOL     get_cwd(char *buffer /*out*/);
extern void     get_prefix(char *buffer /*out*/, BOOL allow_cwd);
extern BOOL     isrooted(const char *name);
extern FILE *   myfopen(const char *name, const char *atts);
extern int      myfclose(FILE *file);
extern void     mysetvbuf(FILE *file, void *buffer, size_t bufsize);
#if RISCOS
extern uword32  filelength(FILE *file);
#endif


#if defined(Z88_OFF)  &&  !RISCOS
#define         myfgetc(input)          fgetc(input)
extern int     (myfgetc)(FILE *input);
#else
extern int      myfgetc(FILE *input);
#endif


#if MS
#define myfputc(ch, output) fputc(ch, output)
extern int     (myfputc)(int ch, FILE *output);
#elif ARTHUR || RISCOS
extern int      myfputc(int ch, FILE *output);
#endif

#if !defined(SAVE_OFF)
extern BOOL     away_byte(intl ch, FILE *output);
extern BOOL     away_eol(FILE *output);
extern BOOL     away_string(const char *str, FILE *output);
#endif

#if MS || ARTHUR
extern BOOL getfilename(uchar *array);
extern BOOL isdirectory(uchar *pathname, uchar *rest);
#endif

#if !defined(Z88_OFF) || defined(Z88FS)
extern intl looks_like_z88(const char *name);
#else
#define looks_like_z88(name) 0
extern intl (looks_like_z88)(uchar *name);
#endif

#if RISCOS
/* filename pointer; myfopen returns FILE* */
#define FILE_IN_RAM_BUFFER ((char *) -1)
#define CFILE_IN_RAM_BUFFER ((const char *) -1)
#endif


/****************************************************************************
* browse.c
****************************************************************************/

#if !defined(SPELL_OFF)

#if defined(SPELL_BOUND)
#define check_spell_installed() TRUE
#else
extern BOOL check_spell_installed(void);
#endif

extern void check_word(void);   /* check current word on line */
extern void close_user_dictionaries(void);
extern void del_spellings(void);
extern intl dict_number(const char *name, BOOL create);


#define A_LETTER struct a_letter

A_LETTER
    {
    uchar letter;
    BOOLEAN used;
    };


/* Number of items in a browse box */
#define BROWSE_DEPTH 12

#endif  /* SPELL_OFF */


#if RISCOS
extern void anagram_null(void);
extern void browse_null(void);
extern void dumpdict_null(void);
extern void mergedict_null(void);
#endif


/****************************************************************************
* bufedit.c
****************************************************************************/

extern BOOL all_widths_zero(colt tcol1, colt tcol2);
extern void chkwrp(void);           /* check for and do wrap */
extern void chrina(uchar ch, BOOL allow_check);
extern void delete_bit_of_line(intl stt_offset, intl length, BOOL save);
extern BOOL fnxtwr(void);
extern void inschr(uchar ch);
extern BOOL insert_string(const char *str, BOOL allow_check);


/****************************************************************************
* option page construct offsets
****************************************************************************/

typedef enum
{
    O_DE,       /* title string */
    O_TN,       /* text/numbers */
    O_IW,       /* insert on wrap */
    O_BO,       /* borders */
    O_JU,       /* justify */
    O_WR,       /* wrap */
    O_DP,       /* decimal places */
    O_MB,       /* minus/brackets */
    O_TH,       /* thousands separator */
    O_IR,       /* insert on return */
    O_DF,       /* date format */
    O_LP,       /* leading characters */
    O_TP        /* trailing characters */
    #if defined(GRID_ON)
,   O_GR        /* grid on/off */
    #endif
}
d_options_offsets;


typedef enum
{
    TH_BLANK,
    TH_COMMA,
    TH_DOT,
    TH_SPACE
}
d_options_thousands_offsets;


typedef enum
{
    O_PL,   /* page length */
    O_LS,   /* line spacing */
    O_PS,   /* start page string */
    O_TM,   /* top margin */
    O_HM,   /* header margin */
    O_FM,   /* footer margin */
    O_BM,   /* bottom margin */
    O_LM,   /* left margin */
    O_HE,   /* header string */
    O_FO    /* footer string */
}
d_poptions_offsets;


typedef enum
{
    OR_AM,      /* auto/manual recalc */
    OR_RC,      /* recalc mode */
    OR_RI,      /* iterations? */
    OR_RN,      /* maximum number of iterations */
    OR_RB       /* maximum change between iterations */
}
d_recalc_offsets;

#define iteration()         (d_recalc_RI == 'Y')
#define iterations_number() (str_isblank(d_recalc_RN) ? -1L : atol(d_recalc_RN))
extern double iterations_change(void);
extern void getoption(uchar *);     /* read option from input stream */
extern BOOL str_isblank(const char *);  /* is string only spaces? */


/****************************************************************************
* dialog.c : dialog boxes
****************************************************************************/

#define F_TEXT      ((uchar) 0)
#define F_NUMBER    ((uchar) 1)
#define F_SPECIAL   ((uchar) 2)
#define F_ERRORTYPE ((uchar) 3)
#define F_COMPOSITE ((uchar) 4)
#define F_COLOUR    ((uchar) 5)
#define F_CHAR      ((uchar) 6)
#define F_ARRAY     ((uchar) 7)
#define F_LIST      ((uchar) 8)


/* These constants MUST be kept in line with the dialog header
 * table as they are indexes into it !!!
 * RJM thinks this conditional compilation is going too far
 * can get rid of it if we force dialog_header to have all entries
*/

typedef enum
{
    D_ERROR,
    D_LOAD,
    D_SAVE_EXISTING,
    D_OVERWRITE,
    D_SAVE,
    D_SAVEINIT,
    D_PRINT,
    D_MSPACE,
    D_DRIVER,
    D_POPTIONS,
    D_SORT,
    D_REPLICATE,
    D_SEARCH,
    D_OPTIONS,
    D_PARAMETER,
    D_WIDTH,
    D_MARGIN,
    D_NAME,
    D_EXECFILE,
    D_GOTO,
    D_DECIMAL,
    D_REMHIGH,
    D_INSHIGH,
    D_DEFKEY,
    D_DEF_FKEY,
    D_INSPAGE,
    D_CREATE,
    D_COLOURS,
    D_INSCHAR,
    D_ABOUT,
    D_STATUS,
    D_COUNT,
    D_PAUSE,
    D_SAVE_DELETED,
    #if !defined(SPELL_OFF)
    D_AUTO,
    D_CHECKON,
    D_CHECK,
    D_CHECK_MESS,
    D_USER_CREATE,
    D_USER_OPEN,
    D_USER_CLOSE,
    D_USER_DELETE,
    D_USER_INSERT,
    D_USER_BROWSE,
    D_USER_DUMP,
    D_USER_MERGE,
    D_USER_ANAG,
    D_USER_SUBGRAM,
    D_USER_LOCK,
    D_USER_UNLOCK,
    D_USER_PACK,
    #endif
    #if ARTHUR
    D_FSERR,
    #endif
    D_FIXES,
    D_DELETED,
    D_RECALC,
    D_PROTECT,
    D_MACRO_FILE,
    #if RISCOS
    D_FONTS,
    #endif
    D_MENU,
    D_DEF_CMD,
    D_THE_LAST_ONE
}
dialog_header_offsets;


#if RISCOS
typedef enum
{
    D_FONTS_G,
    D_FONTS_X,
    D_FONTS_Y,
    D_FONTS_S
}
d_fonts_offsets;
#endif


extern DHEADER dialog_head[];

#if MS || ARTHUR
extern DIALOG d_error[];
#endif
extern DIALOG d_load[];
extern DIALOG d_save_existing[];
extern DIALOG d_overwrite[];
extern DIALOG d_save[];
extern DIALOG d_saveinit[];
extern DIALOG d_print[];
extern DIALOG d_mspace[];
extern DIALOG d_driver[];
extern DIALOG d_poptions[];
extern DIALOG d_sort[];
extern DIALOG d_replicate[];
extern DIALOG d_search[];
extern DIALOG d_options[];
extern DIALOG d_parameter[];
extern DIALOG d_width[];
extern DIALOG d_name[];
extern DIALOG d_execfile[];
extern DIALOG d_goto[];
extern DIALOG d_decimal[];
extern DIALOG d_inshigh[];
extern DIALOG d_defkey[];
extern DIALOG d_def_fkey[];
extern DIALOG d_inspage[];
extern DIALOG d_create[];
extern DIALOG d_colours[];
extern DIALOG d_inschar[];
extern DIALOG d_about[];
#if MS || ARTHUR
extern DIALOG d_status[];
#endif
extern DIALOG d_count[];
extern DIALOG d_auto[];
extern DIALOG d_checkon[];
extern DIALOG d_check[];
extern DIALOG d_check_mess[];
extern DIALOG d_user_create[];
extern DIALOG d_user_open[];
extern DIALOG d_user_close[];
extern DIALOG d_user_delete[];
extern DIALOG d_user_insert[];
extern DIALOG d_user_browse[];
extern DIALOG d_user_dump[];
extern DIALOG d_user_merge[];
extern DIALOG d_user_anag[];
extern DIALOG d_user_lock[];
extern DIALOG d_user_unlock[];
extern DIALOG d_user_pack[];
extern DIALOG d_pause[];
extern DIALOG d_save_deleted[];
#if ARTHUR
extern DIALOG d_fserr[];
#endif
extern DIALOG d_fixes[];

#define D_PASTE_SIZE    10
extern DIALOG d_deleted[];

extern DIALOG d_recalc[];
extern DIALOG d_protect[];
extern DIALOG d_recalc[];
extern DIALOG d_macro_file[];
#if RISCOS
extern DIALOG d_fonts[];
#endif
extern DIALOG d_menu[];
extern DIALOG d_def_cmd[];

extern BOOL allowed_in_dialog(intl c);
extern BOOL dialog_box(intl boxnumber);
extern void dialog_finalise(void);
extern BOOL dialog_initialise(void);
extern void dialog_initialise_once(void);
extern BOOL init_dialog_box(intl boxno);
extern void save_options_to_list(void);
extern void recover_options_from_list(void);
extern void update_all_dialog_from_windvars(void);
extern void update_all_windvars_from_dialog(void);
extern void update_dialog_from_fontinfo(void);
extern void update_dialog_from_windvars(intl boxnumber);
extern void update_fontinfo_from_dialog(void);
extern void update_windvars_from_dialog(intl boxnumber);

#if RISCOS
extern void dialog_box_end(void);
extern BOOL dialog_box_ended(void);
#elif MS || ARTHUR
#define dialog_box_end()    ;
#define dialog_box_ended()  TRUE

extern void (dialog_box_end)(void);
extern BOOL (dialog_box_ended)(void);
#endif


/****************************************************************************
* constr.c
****************************************************************************/

#define DEFAULTWIDTH        12
#define DEFAULTWRAPWIDTH    0

extern BOOL store_bad_formula(uchar *ptr, colt tcol, rowt trow, intl error);

extern void   my_init_ref(uchar);
extern uchar *my_next_ref(uchar *, uchar);

extern BOOL check_not_blank_sheet(void);
extern void constr_finalise(void);
extern BOOL constr_initialise(void);
extern void constr_initialise_once(void);

extern void clslnf(void);       /* close linking files */
extern void delfch(void);       /* free file chain */
extern void newfil(void);       /* reset variables */
extern void newbul(void);       /* initialize variables for new */
extern void newbuw(void);       /* initialize for new sub-file */
extern void reset_filpnm(void);
extern BOOL dftcol(void);       /* setup default columns */
extern void delfil(void);       /* delete contents of file (cols and rows) */

extern void str_clr(char **strvar);
extern BOOL str_set(char **strvar, const uchar *str);
#define str_setc(strvar, str) str_set(strvar, (const uchar *) str)
extern BOOL (str_setc)(char **strvar, const char *str);
extern void str_swap(char **a, char **b);


/****************************************************************************
*                                                                           *
*       positions of things on screen. note that Y DECREASES up screen      *
*                                                                           *
****************************************************************************/

#define LHS_X           ((coord) 0)
#if defined(CLIP_TO_WINDOW)
#define RHS_X           pagwid_plus1
#else
#define RHS_X           (pagwid_plus1 + 1)  /* allow semi-printed chars off right */
#endif
#define TOP_Y           ((coord) 0)


#if !defined(HEADLINE_OFF)
/* menu header line */

#define HEADLINE_X0     LHS_X
#define HEADLINE_X1     RHS_X
#define HEADLINE_Y0     TOP_Y
#define HEADLINE_Y1     (HEADLINE_Y0 - 1)


/* position of the part command entered */

#define ALTARRAY_X0     (HEADLINE_X0 + 50)
#endif


/* coordinates of current slot */

#define SLOTCOORDS_X0   LHS_X
#define SLOTCOORDS_X1   (SLOTCOORDS_X0 + 10)
#ifndef HEADLINE_OFF
#define SLOTCOORDS_Y0   (HEADLINE_Y0 + 1)
#else
#define SLOTCOORDS_Y0   TOP_Y
#endif
#define SLOTCOORDS_Y1   (SLOTCOORDS_Y0 - 1)


/* numeric contents of current slot */

#define NUMERICSLOT_X0  SLOTCOORDS_X1
#define NUMERICSLOT_X1  RHS_X
#define NUMERICSLOT_Y0  SLOTCOORDS_Y0
#define NUMERICSLOT_Y1  SLOTCOORDS_Y1


#define CMDXAD          SLOTCOORDS_X1   /* after maximum slot coordinates */
#define CMDYAD          SLOTCOORDS_Y0   /* and on the same line */

#define PMLLIN          SLOTCOORDS_Y0   /* prompts & errors on same line */
#define ERRLIN          SLOTCOORDS_Y0


/* editing expression line */

#define EDTLIN_X0       LHS_X
#define EDTLIN_X1       RHS_X
#define EDTLIN_Y0       (SLOTCOORDS_Y0 + 1)
#define EDTLIN_Y1       (EDTLIN_Y0 - 1)

#define EDTLIN          EDTLIN_Y0


/* column headings (if present) */

#define COLUMNHEAD_X0   LHS_X
#define COLUMNHEAD_X1   RHS_X
#define COLUMNHEAD_Y0   (EDTLIN + 1)
#define COLUMNHEAD_Y1   (COLUMNHEAD_Y0 - 1)


/* first available screen line:
 * rows start at y = BORDERLINE + borderheight (0 or 1)
*/
#define BORDERLINE  COLUMNHEAD_Y0

/* column headings are one char high */
#define BORDERHEIGHT    ((coord) 1)


#if defined(SHORT_BORDER)
/* row numbers displayed as four digits + space wide */
#define BORDERWIDTH     ((coord) 5)
#else
/* row numbers displayed as five digits + space wide */
#define BORDERWIDTH     ((coord) 6)
#endif


/****************************************************************************
* commlin.c
****************************************************************************/

/* exported functions */

extern void check_state_changed(void);
extern intl command_edit(intl ch);
extern void display_heading(intl idx);
extern BOOL do_command(intl key, BOOL spool);
extern void do_execfile(const char *name);
extern void draw_bar(coord xpos, coord ypos, coord length);
extern intl fiddle_with_menus(intl c, BOOL alt_detected);
extern word32 getsbd(void);
extern void headline_initialise_once(void);
extern intl inpchr(BOOL curs);
extern void insert_state_changed(void);
extern void menu_state_changed(void);
extern void output_char_to_macro_file(uchar ch);
extern void out_comm_start_to_macro_file(MENU *mptr);
extern void out_comm_parms_to_macro_file(DIALOG *dptr, intl size, BOOL ok);
extern void out_comm_end_to_macro_file(MENU *mptr);
extern BOOL schkvl(intl c);

#define ALLOW_MENUS     1
#define NO_MENUS        0

extern intl command_edit(intl);


#define NO_VAL ((coord) -1)     /* assumed negative in calls to getsbd */

extern word32 getsbd(void);     /* return single byte decimal from buff_sofar, updating it */


#define NO_OPTS         ((uchar) 0)
#define BAD_OPTS        ((uchar) 0x80)


/* menus */
extern void draw_bar(coord, coord, coord);


/****************************************************************************
* execs.c
****************************************************************************/

typedef enum
{
    N_Quit = 1,
    N_Escape,
    N_NewWindow,
    N_NextWindow,
    N_CloseWindow,
    N_TidyUp,

    /* movements */
    N_CursorUp,
    N_CursorDown,
    N_CursorLeft,
    N_CursorRight,
    N_PrevWord,
    N_NextWord,
    N_StartOfSlot,
    N_EndOfSlot,
    N_ScrollUp,
    N_ScrollDown,
    N_ScrollLeft,
    N_ScrollRight,
    N_CentreWindow,
    N_PageUp,
    N_PageDown,
    N_PrevColumn,
    N_NextColumn,
    N_TopOfColumn,
    N_BottomOfColumn,
    N_FirstColumn,
    N_LastColumn,
    N_SavePosition,
    N_RestorePosition,
    N_SwapPosition,
    N_GotoSlot,

    /* editing operations */
    N_Return,
    N_InsertSpace,
    N_InsertCharacter,
    N_SwapCase,
    N_DeleteCharacterLeft,
    N_DeleteCharacterRight,
    N_DeleteWord,
    N_DeleteToEndOfSlot,
    N_Paste,
    N_JoinLines,
    N_SplitLine,
    N_InsertRow,
    N_DeleteRow,
    N_InsertRowInColumn,
    N_DeleteRowInColumn,
    N_AddColumn,
    N_InsertColumn,
    N_DeleteColumn,
    N_FormatParagraph,
    N_InsertPageBreak,
    N_EditExpression,
    N_InsertReference,

    /* layout manipulation */
    N_FixColumns,
    N_FixRows,
    N_ColumnWidth,
    N_RightMargin,
    N_MoveMarginLeft,
    N_MoveMarginRight,
    N_LeftAlign,
    N_CentreAlign,
    N_RightAlign,
    N_LCRAlign,
    N_FreeAlign,
    N_LeadingCharacters,
    N_TrailingCharacters,
    N_DecimalPlaces,
    N_SignBrackets,
    N_SignMinus,
    N_DefaultFormat,

    N_Search,
    N_NextMatch,
    N_PrevMatch,

    /* block operations */
    N_ClearMarkedBlock,
    N_ClearProtectedBlock,
    N_CopyBlock,
    N_CopyBlockToPasteList,
    N_DeleteBlock,
    N_ExchangeNumbersText,
    N_HighlightBlock,
    N_MarkSlot,
    N_MoveBlock,
    N_RemoveHighlights,
    N_Replicate,
    N_ReplicateDown,
    N_ReplicateRight,
    N_SetProtectedBlock,
    N_Snapshot,
    N_SortBlock,

    N_LoadFile,
    N_NameFile,
    N_SaveFile,
    N_SaveInitFile,
    N_CreateLinkingFile,
    N_FirstFile,
    N_NextFile,
    N_PrevFile,
    N_LastFile,

    N_Print,
    N_PageLayout,
    N_PrinterConfig,
    N_MicrospacePitch,
    N_SetMacroParm,
#if !RISCOS
    N_PrintStatus,
#endif
    N_AlternateFont,
    N_Bold,
    N_ExtendedSequence,
    N_Italic,
    N_Subscript,
    N_Superscript,
    N_Underline,
    N_UserDefinedHigh,

    N_Options,
    N_InsertOvertype,
    N_Colours,
    N_Recalculate,
    N_RecalcOptions,
    N_About,
    N_WordCount,
    N_RecordMacroFile,
    N_DoMacroFile,
    N_PasteListDepth,
    N_MenuSize,
    N_DefineKey,
    N_DefineFunctionKey,
    N_DefineCommand,
    N_Help,
    N_Pause,
    N_OSCommand,
#if MS
    N_DeepScreen,
#endif

    N_AutoSpellCheck,
    N_BrowseDictionary,
    N_DumpDictionary,
    N_MergeFileWithDict,
    N_Anagrams,
    N_Subgrams,
    N_CheckDocument,
    N_InsertWordInDict,
    N_DeleteWordFromDict,
    N_LockDictionary,
    N_UnlockDictionary,
    N_CreateUserDict,
    N_OpenUserDict,
    N_CloseUserDict,
    N_PackUserDict,
    N_DisplayOpenDicts
#if RISCOS
,   N_PRINTERFONT
,   N_INSERTFONT
,   N_PRINTERLINESPACE
,   N_SaveFileAsIs
,   N_MarkSheet
,   N_Debug
#endif
}
function_numbers;


extern void Quit_fn(void);
extern void Escape_fn(void);
extern void NewWindow_fn(void);
extern void NextWindow_fn(void);
extern void CloseWindow_fn(void);
extern void TidyUp_fn(void);

/* movements */
extern void CursorUp_fn(void);
extern void CursorDown_fn(void);
extern void CursorLeft_fn(void);
extern void CursorRight_fn(void);
extern void PrevWord_fn(void);
extern void NextWord_fn(void);
extern void StartOfSlot_fn(void);
extern void EndOfSlot_fn(void);
extern void ScrollUp_fn(void);
extern void ScrollDown_fn(void);
extern void ScrollLeft_fn(void);
extern void ScrollRight_fn(void);
extern void CentreWindow_fn(void);
extern void PageUp_fn(void);
extern void PageDown_fn(void);
extern void PrevColumn_fn(void);
extern void NextColumn_fn(void);
extern void TopOfColumn_fn(void);
extern void BottomOfColumn_fn(void);
extern void FirstColumn_fn(void);
extern void LastColumn_fn(void);
extern void SavePosition_fn(void);
extern void RestorePosition_fn(void);
extern void SwapPosition_fn(void);
extern void GotoSlot_fn(void);

/* editing operations */
extern void Return_fn(void);
extern void InsertSpace_fn(void);
extern void InsertCharacter_fn(void);
extern void SwapCase_fn(void);
extern void DeleteCharacterLeft_fn(void);
extern void DeleteCharacterRight_fn(void);
extern void DeleteWord_fn(void);
extern void DeleteToEndOfSlot_fn(void);
extern void Paste_fn(void);
extern void JoinLines_fn(void);
extern void SplitLine_fn(void);
extern void InsertRow_fn(void);
extern void DeleteRow_fn(void);
extern void InsertRowInColumn_fn(void);
extern void DeleteRowInColumn_fn(void);
extern void AddColumn_fn(void);
extern void InsertColumn_fn(void);
extern void DeleteColumn_fn(void);
extern void FormatParagraph_fn(void);
extern void InsertPageBreak_fn(void);
extern void EditExpression_fn(void);
extern void InsertReference_fn(void);

/* layout manipulation */
extern void FixColumns_fn(void);
extern void FixRows_fn(void);
extern void ColumnWidth_fn(void);
extern void RightMargin_fn(void);
extern void MoveMarginLeft_fn(void);
extern void MoveMarginRight_fn(void);
extern void LeftAlign_fn(void);
extern void CentreAlign_fn(void);
extern void RightAlign_fn(void);
extern void LCRAlign_fn(void);
extern void FreeAlign_fn(void);
extern void LeadingCharacters_fn(void);
extern void TrailingCharacters_fn(void);
extern void DecimalPlaces_fn(void);
extern void SignBrackets_fn(void);
extern void SignMinus_fn(void);
extern void DefaultFormat_fn(void);

extern void Search_fn(void);
extern void NextMatch_fn(void);
extern void PrevMatch_fn(void);

/* block operations */
extern void ClearMarkedBlock_fn(void);
extern void ClearProtectedBlock_fn(void);
extern void CopyBlock_fn(void);
extern void CopyBlockToPasteList_fn(void);
extern void DeleteBlock_fn(void);
extern void ExchangeNumbersText_fn(void);
extern void HighlightBlock_fn(void);
extern void MarkSheet_fn(void);
extern void MarkSlot_fn(void);
extern void MoveBlock_fn(void);
extern void RemoveHighlights_fn(void);
extern void Replicate_fn(void);
extern void ReplicateDown_fn(void);
extern void ReplicateRight_fn(void);
extern void SetProtectedBlock_fn(void);
extern void Snapshot_fn(void);
extern void SortBlock_fn(void);

extern void LoadFile_fn(void);
extern void NameFile_fn(void);
extern void SaveFile_fn(void);
extern void SaveFileAsIs_fn(void);
extern void SaveInitFile_fn(void);
extern void CreateLinkingFile_fn(void);
extern void FirstFile_fn(void);
extern void NextFile_fn(void);
extern void PrevFile_fn(void);
extern void LastFile_fn(void);

extern void Print_fn(void);
extern void PageLayout_fn(void);
extern void PrinterConfig_fn(void);
extern void MicrospacePitch_fn(void);
extern void SetMacroParm_fn(void);
#if !RISCOS
extern void PrintStatus_fn(void);
#endif
extern void AlternateFont_fn(void);
extern void Bold_fn(void);
extern void ExtendedSequence_fn(void);
extern void Italic_fn(void);
extern void Subscript_fn(void);
extern void Superscript_fn(void);
extern void Underline_fn(void);
extern void UserDefinedHigh_fn(void);

extern void Options_fn(void);
extern void InsertOvertype_fn(void);
extern void Colours_fn(void);
extern void Recalculate_fn(void);
extern void RecalcOptions_fn(void);
extern void About_fn(void);
extern void WordCount_fn(void);
extern void RecordMacroFile_fn(void);
extern void DoMacroFile_fn(void);
extern void PasteListDepth_fn(void);
extern void MenuSize_fn(void);
extern void DefineKey_fn(void);
extern void DefineFunctionKey_fn(void);
extern void DefineCommand_fn(void);
extern void Help_fn(void);
extern void Pause_fn(void);
extern void OSCommand_fn(void);
#if MS
extern void DeepScreen_fn(void);
#endif
extern void AutoSpellCheck_fn(void);
extern void BrowseDictionary_fn(void);
extern void DumpDictionary_fn(void);
extern void MergeFileWithDict_fn(void);
extern void Anagrams_fn(void);
extern void Subgrams_fn(void);
extern void CheckDocument_fn(void);
extern void InsertWordInDict_fn(void);
extern void DeleteWordFromDict_fn(void);
extern void LockDictionary_fn(void);
extern void UnlockDictionary_fn(void);
extern void CreateUserDict_fn(void);
extern void OpenUserDict_fn(void);
extern void CloseUserDict_fn(void);
extern void DisplayOpenDicts_fn(void);
extern void PackUserDict_fn(void);

extern void Debug_fn(void);

extern void small_DeleteCharacter_fn(void);     /* without reformatting */


extern void updref(colt, rowt, colt, rowt, colt, rowt);
extern intl do_delete_block(BOOL do_save);
extern intl symbcmp(SYMB_TYPE *, SYMB_TYPE *);
extern intl stringcmp(const uchar *ptr1, const uchar *ptr2);


#define U_ONE 1
#define U_ALL 2
extern void updroi(uchar flags);
extern void delrwb(uchar flags);

extern void rebnmr(void);
extern void prccml(uchar *);


extern BOOL bad_reference(colt tcol, rowt trow);

extern void add_to_protect_list(uchar *ptr);
extern void clear_protect_list(void);
extern BOOL protected_slot(colt tcol, rowt trow);
extern BOOL protected_slot_in_block(colt firstcol, rowt firstrow, colt lastcol, rowt lastrow);
extern BOOL protected_slot_in_range(const SLR *bs, const SLR *be);
extern void setprotectedstatus(slotp tslot);
extern void save_protected_bits(FILE *output);
extern BOOL test_protected_slot(colt tcol, rowt trow);

#define is_protected_slot(tslot)    (tslot->justify & PROTECTED)


/****************************************************************************
* scdraw.c
****************************************************************************/

/* exported functions */

extern void bottomline(coord xpos, coord ypos, coord xsize);
extern void clear_editing_line(void);
extern void display_heading(intl idx);
extern void draw_one_altered_slot(colt col, rowt row);
extern void draw_screen(void);
extern intl font_strip_spaces(char *out_buf, char *str_in, intl *spaces);
extern intl font_width(char *str);
extern BOOL is_overlapped(coord coff, coord roff);
#ifdef VIEW_IO
extern coord justifyline(uchar *, coord, uchar, uchar *);
#else
extern coord justifyline(uchar *, coord, uchar);
#endif
extern intl limited_fwidth_of_slot(slotp tslot, colt tcol, rowt trow);
extern void my_rectangle(coord xpos, coord ypos, coord xsize, coord ysize);
extern coord onejst(uchar *str, coord fwidth, uchar type);
extern coord outslt(slotp tslot, rowt row, coord fwidth);
extern void position_cursor(void);
extern void switch_off_highlights(void);
extern void topline(coord xpos, coord ypos, coord xsize);
extern void twzchr(char ch);
extern coord lcrjust(uchar *str, coord fwidth, BOOL reversed);

#define FIX       ((uchar) (0x01))
#define LAST      ((uchar) (0x02))
#define PAGE      ((uchar) (0x04))
#define PICT      ((uchar) (0x08))
#define UNDERPICT ((uchar) (0x10))

#if RISCOS
extern intl font_truncate(char *str, intl width);
#endif


/****************************************************************************
* doprint.c
****************************************************************************/

#define EOS     '\0'    /* prnout called with this to turn off highlights */
#define P_ON    ((word32) 257)  /* printer on */
#define P_OFF   ((word32) 258)  /* printer off */
#define E_P     ((word32) 259)  /* end page */
#define HMI_P   ((word32) 260)  /* HMI prefix */
#define HMI_S   ((word32) 261)  /* HMI suffix */
#define HMI_O   ((word32) 262)  /* HMI offset */

extern void  fixpage(rowt, rowt);
extern void  force_calshe(void);
extern intl  getfield(FILE *input, uchar *array, BOOL ignore_lead_spaces);
extern coord header_length(colt first, colt last);
extern void  mspace(intl n);
extern void  out_h_string(uchar *, intl);
extern void  print_document(void);
extern BOOL  prnout(intl);
extern void  rawprint(intl ch);
extern void  reset_print_options(void);
extern void  set_pitch(intl n);
extern void  setssp(void);
extern BOOL  sndchr(uchar);

#if MS
extern void myhandler(void);
#endif


/****************************************************************************
* cursmov.c
****************************************************************************/

extern coord calcad(coord coff);            /* horzvec offset -> x pos */
extern coord calcoff(coord tx);             /* x pos -> horzvec offset */
extern coord calcoff_click(coord tx);       /* ditto, but round into l/r cols */
extern coord calcoff_overlap(coord xpos, rowt trow);

#define calrad(roff)    (roff + BORDERLINE + borderheight)
extern coord (calrad)(coord roff);          /* vertvec offset -> y pos */
#define calroff(ty)     (ty   - BORDERLINE - borderheight)
extern coord (calroff)(coord ty);           /* y pos -> vertvec offset */
extern coord calroff_click(coord ty);       /* ditto, but round into t/b rows */

extern coord chkolp(slotp tslot, colt tcol, rowt trow); /* find overlap width */
extern BOOL  chkrpb(rowt trow);             /* check row for page break */
extern BOOL  chkpac(rowt trow);             /* page break active? */
extern BOOL  chkpbs(rowt trow, intl offset);
/* check page breaks enabled */
#define chkfsb() (encpln  &&  (!n_rowfixes  ||  prnbit))

extern void mark_row_border(coord rowonscr);

extern void filhorz(colt, colt);
extern void filvert(rowt, rowt, BOOL);

extern void chknlr(colt tcol, rowt trow);   /* for absolute movement */
#define rstpag()                            /* reset page counts */

extern SCRCOL *horzvec(void);               /* current address of horzvec (moveable) */
extern SCRCOL *horzvec_entry(coord coff);
extern colt col_number(coord coff);

extern SCRROW *vertvec(void);               /* current address of vertvec (moveable) */
extern SCRROW *vertvec_entry(coord roff);
extern rowt row_number(coord roff);

#if RISCOS
extern intl  newoffset;
extern coord get_column(coord tx, rowt trow, intl xcelloffset, BOOL selectclicked);
extern void  application_click(coord tx, coord ty, intl xcelloffset, BOOL selectclicked);
extern void  get_slr_for_point(gcoord x, gcoord y, BOOL selectclicked, char *buffer /*out*/);
extern void  insert_reference_to(dochandle insdoc, dochandle tdoc, colt tcol, rowt trow, BOOL allow_draw);
#endif

/* column offsets returned from calcoff */
#define OFF_LEFT (-1)
#define OFF_RIGHT (-2)


#define NOTFOUND        ((coord) -1)
#define BORDERCH        DOT
#define BORDERFIXCH     ((uchar) '_')


#define CALL_FIXPAGE 1
#define DONT_CALL_FIXPAGE 0

extern intl calsiz(uchar *);
extern void cencol(colt tcol);
extern void cenrow(rowt trow);
extern void chkmov(void);
extern void curosc(void);               /* check cursor on screen */
extern void cursdown(void);
extern void cursup(void);
extern void curup(void);
extern void curdown(void);
extern colt fstncx(void);               /* first non-fixed column on screen */
extern rowt fstnrx(void);               /* first non-fixed row on screen */
extern BOOL incolfixes(colt tcol);
extern BOOL inrowfixes(rowt trow);
extern BOOL inrowfixes1(rowt trow);
extern BOOL isslotblank(slotp tslot);
extern void mark_row(coord);
extern void mark_row_praps(coord rowonscr, BOOL old_row);
#define OLD_ROW TRUE
#define NEW_ROW FALSE
extern void mark_slot(slotp);
extern void mark_to_end(coord);
extern void nextcol(void);
extern void prevcol(void);
extern coord schcsc(colt);              /* search for col on screen */
extern coord schrsc(rowt);              /* search for row on screen */


/****************************************************************************
* numbers.c
****************************************************************************/

extern BOOL actind(intl message, intl number);
extern void actind_end(void);
extern intl compile_text_slot(char *array /*out*/, const char *from, uchar *slot_refsp /*out*/);
extern intl cplent(uchar *oprstb /*out*/, intl hash_to_dollar);
extern void draw_close_file(dochandle han);
extern intl draw_find_file(dochandle han, colt col, rowt row,
                           drawfep *drawfile, drawfrp *drawref);
extern void draw_recache_file(const char *drawfilename);
extern void draw_removeblock(dochandle han, colt scol, rowt srow, colt ecol, rowt erow);
extern void draw_removeslot(colt col, rowt row);
extern LIST *draw_search_cache(char *name, intl namlen, word32 *maxkey);
extern intl draw_str_insertslot(colt col, rowt row);
extern void draw_swaprefs(rowt row1, rowt row2, colt scol, colt ecol);
extern void draw_tidy_up(void);
extern intl draw_tree_str_insertslot(colt col, rowt row, intl sort);
extern void draw_tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow);
extern void draw_tree_removeslot(colt col, rowt row);
extern void draw_updref(colt mrksco, rowt mrksro,
                        colt mrkeco, rowt mrkero,
                        colt coldiff, rowt rowdiff);
extern void endeex(void);
extern void endeex_nodraw(void);
extern void filbuf(void);
extern colt getcol(void);
extern BOOL graph_active_present(dochandle han);
extern ghandle graph_add_entry(ghandle ghan, dochandle han, colt col, rowt row,
                            intl xsize, intl ysize, const char *leaf, const char *tag,
                            intl task);
extern void graph_close_file(dochandle han);
extern void graph_draw_tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow);
extern void graph_draw_tree_removeslot(colt col, rowt row);
extern void graph_removeblock(dochandle han, colt scol, rowt srow, colt ecol, rowt erow);
extern void graph_removeslot(colt col, rowt row);
extern void graph_remove_entry(ghandle ghan);
extern graphlinkp graph_search_list(ghandle ghan);
extern void graph_send_block(colt scol, rowt srow, colt ecol, rowt erow);
extern void graph_send_xblock(colt scol, rowt srow, colt ecol, rowt erow, dochandle han);
extern void graph_send_slot(colt col, rowt row);
extern void graph_send_split_blocks(colt scol, rowt srow, colt ecol, rowt erow);
extern void graph_updref(colt mrksco, rowt mrksro,
                         colt mrkeco, rowt mrkero,
                         colt coldiff, rowt rowdiff);
extern intl is_draw_file_slot(dochandle han, colt col, rowt row);
extern void mark_slot(slotp tslot);
extern void merexp(void);
extern BOOL mergebuf(void);
extern BOOL mergebuf_nocheck(void);
extern BOOL merst1(colt tcol, rowt trow);
extern void prccon(uchar *target, slotp ptr);
extern intl readfxy(intl id, char **pfrom, char **pto, char **name,
                    double *xp, double *yp);
extern void seteex(void);
extern void seteex_nodraw(void);
extern void splat(uchar *to, word32 num, intl size);
extern word32 talps(uchar *from, intl size);
extern intl writecol(uchar *array, colt col);
extern intl writeref(uchar *ptr, docno doc, colt tcol, rowt trow);


/* actind arguments */
#define TIMER_DELAY     20
#define NO_ACTIVITY     ((intl) -1)

#define DEACTIVATE      ((intl) -1)
#define ACT_LOAD        ((intl) 0)
#define ACT_SAV         ((intl) 1)
#define ACT_SORT        ((intl) 2)
#define ACT_CALCULAT    ((intl) 3)
#define ACT_COPY        ((intl) 4)
#define ACT_PRINT       ((intl) 5)
#if !defined(SPELL_OFF)
#define ACT_CHECK       ((intl) 6)
#endif
#define ACT_BASHING     ((intl) 7)


/****************************************************************************
* eval.c, expcomp.c, semantic.c
****************************************************************************/

/*
eval.c
*/

/*********************************************
*                                           *
* macro to return a current document number, *
* and ensure that it is valid               *
*                                           *
*********************************************/

#define ensure_cur_docno() (num_doc[current_document_handle()]\
                              ? num_doc[current_document_handle()]\
                              : set_docno(current_document_handle()))

#define recalcorder() (d_recalc_RC)
#define RECALC_COLS     ('C')
#define RECALC_ROWS     ('R')
#define RECALC_NATURAL  ('N')

#define CALC_ACT 0
#define CALC_DRAW 1
#define CALC_NONE 2
#define CALC_NOINTORDRAW 3

#define CALC_RESTART    0
#define CALC_CONTINUE   1

typedef enum
{
    CALC_ABORTED    = -1,
    CALC_COMPLETED,
    CALC_KEY,
    CALC_TIMEOUT
}
recalc_return_codes;

#define mark_slot_as_moved(sl) (orab((sl)->flags, SL_MOVED))

extern uchar *nam_doc[];
extern docno num_doc[];
extern docno han_doc[];
extern docno *lnk_doc[];
extern uchar num_lnk[];

extern intl calshe(intl draw, intl state);
extern intl check_docvalid(docno doc);
extern void eval_slot(SYMB_TYPE *, intl);
extern void evasto(slotp tslot, colt tcol, rowt trow);
extern void exp_close_file(dochandle han);
extern void exp_eval(uchar *);
extern void exp_rename_file(void);
extern void inc_rows(uchar *expression);
extern intl init_dependent_docs(docno *doc, intl *index);
extern BOOL init_stack(void);
extern intl init_supporting_files(docno *doc, intl *index);
extern docno maybe_cur_docno(dochandle curhan);
extern docno next_dependent_doc(docno *doc, intl *index);
extern docno next_supporting_file(docno *doc, intl *index,
                                  dochandle *han, char **name);
extern intl read_docname(char *str, docno *doc);
extern intl selrow(uchar *, rowt, BOOL);
extern docno set_docno(dochandle han);
extern void switch_document(docno newdoc);
extern intl tree_build(void);
extern void tree_delete(void);
extern intl tree_exp_insertslot(colt, rowt, intl);
extern void tree_moveblock(colt scol, rowt srow, colt ecol, rowt erow);
extern void tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow);
extern void tree_removeslot(colt col, rowt row);
extern intl tree_str_insertslot(colt col, rowt row, intl sort);
extern void tree_swaprefs(rowt row1, rowt row2, colt scol, colt ecol);
extern void tree_switchoff(void);
extern void tree_updref(colt mrksco, rowt mrksro, colt mrkeco, rowt mrkero,
                 colt coldiff, rowt rowdiff);
extern intl write_docname(char *str, docno doc);

/*
expcomp.c
*/

extern intl exp_compile(uchar *, char *, BOOL *, intl);
extern intl exp_decompile(char *, uchar *);
extern uchar *exp_findslr(uchar *);
extern void exp_initslr(void);
extern intl exp_len(uchar *);


/***************************************************************************
* reperr.c
***************************************************************************/

/* exported functions */

extern BOOL reperr(intl errornumber, const char *text);
extern char *reperr_getstr(intl errornumber);
extern BOOL reperr_module(intl module, intl errornumber);
extern BOOL reperr_null(intl errornumber);
extern BOOL reperr_not_installed(intl errornumber);
extern BOOL rep_fserr(const char *str);
#if RISCOS
#   define  reperr_fatal    werr_fatal
#elif MS
extern void reperr_fatal(const char *format, ...);
#endif


/* error definition */

#define MAIN_ERR_START 0

#define ERR_NOERROR         MAIN_ERR_START - 0
#define ERR_LINETOOLONG     MAIN_ERR_START - 1
#define ERR_BAD_NAME        MAIN_ERR_START - 2
#define ERR_NOROOM          MAIN_ERR_START - 3    /* not enough memory */
#define ERR_NOTFOUND        MAIN_ERR_START - 4    /* file not found */
#define ERR_BAD_SELECTION   MAIN_ERR_START - 5
#define ERR_BAD_PARM        MAIN_ERR_START - 6
#define ERR_EDITINGEXP      MAIN_ERR_START - 7
#define ERR_CANNOTOPEN      MAIN_ERR_START - 8
#define ERR_CANNOTREAD      MAIN_ERR_START - 9
#define ERR_CANNOTWRITE     MAIN_ERR_START - 10
#define ERR_CANNOTCLOSE     MAIN_ERR_START - 11
#define ERR_EGA             MAIN_ERR_START - 12
#define ERR_BAD_HAT         MAIN_ERR_START - 13
#define ERR_NOLISTFILE      MAIN_ERR_START - 14
#define ERR_LOOP            MAIN_ERR_START - 15
#define ERR_ENDOFLIST       MAIN_ERR_START - 16
#define ERR_CANNOTBUFFER    MAIN_ERR_START - 17
#define ERR_BAD_OPTION      MAIN_ERR_START - 18
#define ERR_NOBLOCK         MAIN_ERR_START - 19
#define ERR_ESCAPE          MAIN_ERR_START - 20
#define ERR_BAD_COL         MAIN_ERR_START - 21
#define ERR_OLDEST_LOST     MAIN_ERR_START - 22
#define ERR_Z88             MAIN_ERR_START - 23
#define ERR_BAD_EXPRESSION  MAIN_ERR_START - 24
#define ERR_BAD_MARKER      MAIN_ERR_START - 25
#define ERR_NO_DRIVER       MAIN_ERR_START - 26
#define ERR_NO_MICRO        MAIN_ERR_START - 27
#define ERR_GENFAIL         MAIN_ERR_START - 28
#define ERR_OVERLAP         MAIN_ERR_START - 29
#define ERR_CTRL_CHARS      MAIN_ERR_START - 30
#define ERR_CANNOTINSERT    MAIN_ERR_START - 31
#define ERR_LOTUS           MAIN_ERR_START - 32
#define ERR_NOPAGES         MAIN_ERR_START - 33
#define ERR_SPELL           MAIN_ERR_START - 34
#define ERR_WORDEXISTS      MAIN_ERR_START - 35
#define ERR_PRINTER         MAIN_ERR_START - 36
#define ERR_NOTTABFILE      MAIN_ERR_START - 37
#define ERR_LINES_SPLIT     MAIN_ERR_START - 38
#define ERR_NOTREE          MAIN_ERR_START - 39
#define ERR_BAD_STRING      MAIN_ERR_START - 40
#define ERR_BAD_SLOT        MAIN_ERR_START - 41
#define ERR_BAD_RANGE       MAIN_ERR_START - 42
#define ERR_SHEET           MAIN_ERR_START - 43
#define ERR_PROTECTED       MAIN_ERR_START - 44
#define ERR_AWAITRECALC     MAIN_ERR_START - 45
#define ERR_BADDRAWFILE     MAIN_ERR_START - 46
#define ERR_BADFONTSIZE     MAIN_ERR_START - 47
#define ERR_BADLINESPACE    MAIN_ERR_START - 48
#define ERR_ISADIR          MAIN_ERR_START - 49
#define ERR_BADDRAWSCALE    MAIN_ERR_START - 50
#define ERR_PRINTERINUSE    MAIN_ERR_START - 51
#define ERR_NORISCOSPRINTER MAIN_ERR_START - 52
#define ERR_FONTY           MAIN_ERR_START - 53
#define ERR_ALREADYDUMPING  MAIN_ERR_START - 54
#define ERR_ALREADYMERGING  MAIN_ERR_START - 55
#define ERR_ALREADYANAGRAMS MAIN_ERR_START - 56
#define ERR_ALREADYSUBGRAMS MAIN_ERR_START - 57
#define ERR_CANTINSTALL     MAIN_ERR_START - 58
#define ERR_BADSCRAPLOAD    MAIN_ERR_START - 59
#define ERR_NOROOMFORDBOX   MAIN_ERR_START - 60
#define ERR_NOTINDESKTOP    MAIN_ERR_START - 61
#define ERR_BADPRINTSCALE   MAIN_ERR_START - 62
#define ERR_NOBLOCKINDOC    MAIN_ERR_START - 63
#define ERR_CANTSAVEPASTEBLOCK  MAIN_ERR_START - 64
#define ERR_CANTLOADPASTEBLOCK  MAIN_ERR_START - 65
#define ERR_CANTSAVETOITSELF    MAIN_ERR_START - 66

/* module errors start after this */
#define MAIN_ERR_END        -1000
#define MODULE_INCREMENT    1000

#define SPELL_ERR_START     MAIN_ERR_END
#define EVAL_ERR_START      MAIN_ERR_END - 1 * MODULE_INCREMENT
#define Z88_ERR_START       MAIN_ERR_END - 2 * MODULE_INCREMENT
#define PD123_ERR_START     MAIN_ERR_END - 3 * MODULE_INCREMENT
#define SHEET_ERR_START     MAIN_ERR_END - 4 * MODULE_INCREMENT
/* insert new module errors in here */
#define NEXT_ERR_START      MAIN_ERR_END - 5 * MODULE_INCREMENT


/****************************************************************************
* pdsearch.c
****************************************************************************/

typedef enum
{
    SCH_TARGET,
    SCH_REPLACE,
    SCH_CONFIRM,
    SCH_CASE,
    SCH_EXPRESSIONS,
    SCH_BLOCK,
    SCH_COLRANGE,
    SCH_ALLFILES,
    SCH_FROMCURRENT
}
d_search_offsets;


/* following variables used in check as well as search */

extern BOOL do_replace(uchar *replace_str);


/****************************************************************************
definition of slot structure
****************************************************************************/

struct slot
    {
    uchar type;                 /* type of slot */
    uchar flags;                /* error, altered & contains slot refs */
    uchar justify;              /* justify bits and protected bit */

    union
        {
                                        /* text slots */
        uchar text[1];
                                        /* number slots */
        struct
            {
            union
                {
                DATE resdate;
                double value;
                intl errno;
                intl str_offset;
                SLR str_slot;
                } result;

            uchar format;
            uchar text[1];
            } number;

        struct
            {
            intl cpoff;     /* current page offset */
            intl condval;   /* value for conditional page eject, 0=unconditional */
            } page;

        } content;
    };

#if ARTHUR || RISCOS

#define SL_NUMBEROVH    ((intl) sizeof(struct slot) - 1)
#define SL_SLOTOVH      ((intl) 4)
#define SL_TEXTOVH      SL_SLOTOVH

#elif MS

#define SL_NUMBEROVH    ((intl) sizeof(struct slot) - 1)
#define SL_SLOTOVH      ((intl) 3)
#define SL_TEXTOVH      SL_SLOTOVH

#endif


/****************************************************************************
slot types
****************************************************************************/

#define SL_DATE     ((uchar) 0)
#define SL_STRVAL   ((uchar) 1)
#define SL_TEXT     ((uchar) 2)
#define SL_INTSTR   ((uchar) 3)
#define SL_NUMBER   ((uchar) 4)
#define SL_ERROR    ((uchar) 5)
#define SL_PAGE     ((uchar) 6)

/* the following appear in expressions and are parsed by expression analyser */

#define SL_SLOTR    ((uchar) 7)
#define SL_FUNC     ((uchar) 8)
#define SL_OP       ((uchar) 9)
#define SL_BRA      ((uchar) 10)
#define SL_KET      ((uchar) 11)
#define SL_COMMA    ((uchar) 12)
#define SL_END      ((uchar) 13)
#define SL_RANGE    ((uchar) 14)
#define SL_BLANK    ((uchar) 15)
#define SL_LIST     ((uchar) 16)
#define SL_INTL     ((uchar) 17)

#define SL_BAD_FORMULA  ((uchar) 18)


/****************************************************************************
slot flags - mutually exclusive
****************************************************************************/

#define SL_REFS     ((uchar) 1)
#define SL_ALTERED  ((uchar) 2)
#define SL_RECALCED ((uchar) 4)
#define SL_CIRC ((uchar) 8)
#define SL_MUSTRECALC ((uchar) 16)
#define SL_MOVED ((uchar) 32)
#define SL_VISITED ((uchar) 64)
#define SL_TOBEDEL ((uchar) 128)
#define SL_ITERATE ((uchar) 128)


/****************************************************************************
format flags
****************************************************************************/

#define F_BRAC      ((uchar) 0x80)      /* 0 = -, 1=() */
#define F_DCP       ((uchar) 0x40)      /* 1 = use DCPSID as format, otherwise option dcp */
#define F_LDS       ((uchar) 0x20)      /* 1 = lead characters */
#define F_TRS       ((uchar) 0x10)      /* 1 = trail characters */
#define F_DCPSID    ((uchar) 0x0F)      /* 0x0F=free, 0-9 */


/****************************************************************************
justification flags
****************************************************************************/

#define J_FREE      ((uchar) 0) /* these must be same as C_FREE -> C_LCR */
#define J_LEFT      ((uchar) 1)
#define J_CENTRE    ((uchar) 2)
#define J_RIGHT     ((uchar) 3)
#define J_LEFTRIGHT ((uchar) 4)
#define J_RIGHTLEFT ((uchar) 5)
#define J_LCR       ((uchar) 6)

#define J_BITS      ((uchar) 127)
#define CLR_J_BITS  ((uchar) 128)
#define PROTECTED   ((uchar) 128)


/* things that need to find a home */

extern void switch_off_highlights(void);
extern void new_screen(void);
extern BOOL init_screen(void);
extern BOOL new_window_height(coord height);
extern BOOL new_window_width(coord width);
extern BOOL screen_initialise(void);
extern void screen_finalise(void);
extern void reinit_rows(void);      /* update row information after mode change */
#if MS || ARTHUR
extern intl dspfld(coord x, coord y, coord fwidth, uchar delimiter);
#endif
extern coord strout(uchar *, coord, BOOL);
extern void draw_row(coord);

extern BOOL save_words(uchar *ptr);


extern void defkey(void);           /* define a soft key */
extern void adddef(void);           /* add definition to list */
extern void dsp_border_col(coord x, coord y, uchar *string);
extern void update_variables(void); /* write option page info internally */
extern void change_border_variables(void);

#if RISCOS
extern void filealtered(BOOL newstate);
#else
#define filealtered(newstate) (xf_filealtered = newstate)
#endif


#if RISCOS
extern intl inpchr_riscos(intl c);
#endif

extern BOOL same_name_warning(const char *name, const char *mess);
extern BOOL dependent_files_warning(void);
extern BOOL dependent_links_warning(void);
extern void close_window(void);

extern void dispose(void **aa);


/****************************************************************************
* slotconv.c
****************************************************************************/

/* exported functions */

extern void expand_current_slot_in_fonts(char *to /*out*/, BOOL partial, intl *this_fontp /*out*/);
extern void expand_lcr(uchar *from,
                       rowt row, uchar *array /*out*/, coord fwidth,
                       BOOL expand_refs, intl font_switch, BOOL expand_ctrl, BOOL compile_lcr);
extern uchar expand_slot(slotp tslot,
                         rowt row, uchar *array /*out*/, coord fwidth,
                         BOOL expand_refs, intl font_switch, BOOL expand_ctrl);
extern void font_close_file(dochandle doc);
extern void font_expand_for_break(char *to, char *from);
extern slotp find_string(SLR *slotref, BOOL *loop_found);
extern intl font_skip(char *at);
extern intl font_stack(intl to_stack);
extern void font_tidy_up(void);
extern intl font_unstack(intl stack_state);
extern uchar get_dec_field_from_opt(void);
extern intl is_font_change(char *ch);


#if RISCOS
/* NorCroft is still particularly poor at a->m += b (and a->m++ too)
 * due to high-level optimisation getting &a->m
 *
 *      ADD     ptr, address_of_a, #m
 *      LDR     rn, [ptr]
 *      ADD     rn, rn, b
 *      STR     rn, [ptr]
 *  not
 *      LDR     rn, [address_of_a, #m]
 *      ADD     rn, rn, b
 *      STR     rn, [address_of_a, #m]
*/
#define inc(a)          ((a) = (a) + 1)
#define dec(a)          ((a) = (a) - 1)
#define plusab(a, b)    ((a) = (a) + (b))
#define minusab(a, b)   ((a) = (a) - (b))
#define timesab(a, b)   ((a) = (a) * (b))
#define andab(a, b)     ((a) = (a) & (b))
#define orab(a, b)      ((a) = (a) | (b))
#define bicab(a, b)     ((a) = (a) & ~(b))
#else
#define inc(a)          ((a)++)
#define dec(a)          ((a)--)
#define plusab(a, b)    ((a) += (b))
#define minusab(a, b)   ((a) -= (b))
#define timesab(a, b)   ((a) *= (b))
#define andab(a, b)     ((a) &= (b))
#define orab(a, b)      ((a) |= (b))
#define bicab(a, b)     ((a) &= ~(b))
#endif


/* return codes for exit(rc) */

#if !defined(EXIT_SUCCESS)
#   define   EXIT_SUCCESS   0
#endif

#if !defined(EXIT_FAILURE)
#   define   EXIT_FAILURE   1
#endif

/* end of datafmt.h */
