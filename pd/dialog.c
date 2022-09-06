/* dialog.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       dialog.c - dialog box handling
 * Author:      RJM August 1987
 * History:
 *  0.01    23-Jan-89   SKS split off from lists.c
 *  0.02    08-Mar-89   SKS started hacks for RISC OS dboxes
*/

/* standard header files */
#include "flags.h"

#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"

#elif MS
#include <graph.h>
#else
    assert(0);
#endif


#include "datafmt.h"


#if RISCOS
#include "ext.riscos"
#include "ext.pd"
#include "riscdialog.h"
#endif


/* exported functions */

#if !RISCOS
extern BOOL allowed_in_dialog(intl c);
#endif
extern BOOL dialog_box(intl boxnumber);
extern void dialog_finalise(void);
extern BOOL dialog_initialise(void);
extern void dialog_initialise_once(void);
extern void getoption(uchar *optr);
extern BOOL init_dialog_box(intl boxnumber);
extern void recover_options_from_list(void);
extern void save_options_to_list(void);
extern void update_all_dialog_from_windvars(void);
extern void update_all_windvars_from_dialog(void);
extern void update_dialog_from_fontinfo(void);
extern void update_dialog_from_windvars(intl boxnumber);
extern void update_fontinfo_from_dialog(void);
extern void update_windvars_from_dialog(intl boxnumber);
#if RISCOS
extern void dialog_box_end(void);
extern BOOL dialog_box_ended(void);
#endif


/* internal functions */

#if TRUE
/*static void extract_parameters(DIALOG *dptr, intl items);*/
/*static DIALOG *find_option(uchar ch1, uchar ch2, intl *dbox_num);*/
static BOOL init___dialog_box(intl boxnumber, BOOL windvars, BOOL allocatestrings);
/*static BOOL read_parm(uchar *array);*/
/*static void save_opt_to_list(DIALOG *start, intl n);*/

#if MS || ARTHUR
static void dsp_dialog_field(coord xpos, coord ypos, DIALOG *dptr);
static BOOL edit_with_c(intl c, DIALOG *dptr, intl boxnumber,
                        coord fieldx, coord fieldy, intl *idx);
#endif
#endif


/* ----------------------------------------------------------------------- */

#define YESNOSTR          "YN"
#define NOYESSTR          "NY"

#define ZEROSTR           "0"
#define ONESTR            "1"
#define TWOSTR            "2"


#if !defined(SPELL_OFF) && (MS || ARTHUR)
static const char dictionarynamestr[]   = "Dictionary name";
static const char userdictionaryqstr[]  = "User dictionary?";
#endif


/****************************************************************************
*                                                                           *
*                               dialog boxes                                *
*                                                                           *
****************************************************************************/

#define TEXTWIDTH 20

#define NO_OPT  0
#define NO_LIST NULL


#if MS || ARTHUR
static BOOL on_composite_one = TRUE;
static BOOL num_in_linbuf = FALSE;  /* numbers can be edited in linbuf, or changed with cursor keys */

static intl retval = 0;

/* character to display the next character in DIALOG_HOT */
/* NB. This is hardwired into the dialog entries, so take care!!! */

#define DIALOG_HIGHLIGHT_TOKEN '\177'


#define NO_HELP 255

static const char *
dialog_help_table[] =
{
#define HELP_FILENAME   0
    "Filename",

#define HELP_SLOT_REF   1
    "Slot reference",

#define HELP_FL_ROWS    2
    "First and last rows",

#define HELP_ARROWS     3
    /* left and right arrow chars are system dependent */
#if RISCOS
    "Use \x88 and \x89",
#elif ARTHUR
    "Use \x83 and \x82",
#elif MS
    "Use \x1B and \x1A",
#endif

#define HELP_PRESS_KEY  4
    PRESSANYKEY_STR,

#define HELP_FL_COLS    5
    "First and last columns",

#define HELP_LCR        6
    "/left/centre/right/",

#define HELP_COLUMN     7
    "Column",

#define HELP_FL_SLOTS   8
    "First and last slots",

#define HELP_CONDITION  9
    "Condition eg row > 10",
};

#endif  /* MS || ARTHUR */


static const char *
crlf_list[] =
{
/* note that the positions of these strings is given by
 *  SAV_LSEP_CR
 *  SAV_LSEP_LF
 *  SAV_LSEP_CRLF
 *  SAV_LSEP_LFCR
*/
#if RISCOS
    "LF",
    "CR",
    "CR,LF",
    "LF,CR",
#elif ARTHUR
    "CR",
    "LF",
    "CR,LF",
    "LF,CR",
#elif MS
    "CR,LF",
    "CR",
    "LF",
    "LF,CR",
#endif
    NULL
};


static const char *
thousands_list[] =
{
    thousands_none_STR,
    thousands_comma_STR,
    thousands_dot_STR,
    thousands_space_STR,
    NULL
};


static const char *
port_list[] =
    {
#if ARTHUR || RISCOS
#if RISCOS
    printertype_RISC_OS_STR,
#endif
    printertype_Parallel_STR,
    printertype_Serial_STR,
    printertype_Network_STR,
    printertype_User_STR,
#elif MS
    "LPT1",
    "LPT2",
    "LPT3",
    "COM1",
    "COM2",
#endif

    NULL
    };


static const char *
baud_list[] =
    {
#if ARTHUR || RISCOS
    "9600",
    "75",
    "150",
    "300",
    "1200",
    "2400",
    "4800",
    "9600",
    "19200",
#elif MS
    "9600",
    "4800",
    "2400",
    "1200",
    "600",
    "300",
    "150",
    "110",
#endif

    NULL
    };


static const char *
func_list[] =
    {
    F1_STR,
    F2_STR,
    F3_STR,
    F4_STR,
    F5_STR,
    F6_STR,
    F7_STR,
    F8_STR,
    F9_STR,
    F10_STR,
#if ARTHUR || RISCOS
    F11_STR,
    F12_STR,
#endif

    Shift_F1_STR,
    Shift_F2_STR,
    Shift_F3_STR,
    Shift_F4_STR,
    Shift_F5_STR,
    Shift_F6_STR,
    Shift_F7_STR,
    Shift_F8_STR,
    Shift_F9_STR,
    Shift_F10_STR,
#if ARTHUR || RISCOS
    Shift_F11_STR,
    Shift_F12_STR,
#endif

    Ctrl_F1_STR,
    Ctrl_F2_STR,
    Ctrl_F3_STR,
    Ctrl_F4_STR,
    Ctrl_F5_STR,
    Ctrl_F6_STR,
    Ctrl_F7_STR,
    Ctrl_F8_STR,
    Ctrl_F9_STR,
    Ctrl_F10_STR,
#if ARTHUR || RISCOS
    Ctrl_F11_STR,
    Ctrl_F12_STR,

    Ctrl_Shift_F1_STR,
    Ctrl_Shift_F2_STR,
    Ctrl_Shift_F3_STR,
    Ctrl_Shift_F4_STR,
    Ctrl_Shift_F5_STR,
    Ctrl_Shift_F6_STR,
    Ctrl_Shift_F7_STR,
    Ctrl_Shift_F8_STR,
    Ctrl_Shift_F9_STR,
    Ctrl_Shift_F10_STR,
    Ctrl_Shift_F11_STR,
    Ctrl_Shift_F12_STR,
#endif

#if MS || ARTHUR
    "Alt F1",
    "Alt F2",
    "Alt F3",
    "Alt F4",
    "Alt F5",
    "Alt F6",
    "Alt F7",
    "Alt F8",
    "Alt F9",
    "Alt F10",
#if ARTHUR
    "Alt F11",
    "Alt F12",
#endif
#endif

    NULL,
    };


#if RISCOS
/* No file by default */
#define LOAD_INIT_STR NULLSTR
#else
/* Wild selection by default */
#define LOAD_INIT_STR WILD_STR
#endif


/* load/save dialog text/options */

#if defined(DTP_EXPORT)
#   define  TYPES_DTP           "D"
#   define  FORMAT_DTP          ",\177Acorn DTP"

#   define  TYPES_1WP           "D"
#   define  FORMAT_1WP          ",\1771st Word Plus"
#else
#   define  TYPES_DTP           ""
#   define  FORMAT_DTP          ""

#   define  TYPES_1WP           ""
#   define  FORMAT_1WP          ""
#endif

#if defined(VIEW_IO)
#   define  TYPES_VIEW          "V"
#   define  FORMAT_VIEW         ",\177VIEW"
#else
#   define  TYPES_VIEW          ""
#   define  FORMAT_VIEW         ""
#endif

#if !defined(LOTUS_OFF)
#   define  TYPES_LOTUS         "L"
#   define  FORMAT_LOTUS        ",\177Lotus"
#else
#   define  TYPES_LOTUS         ""
#   define  FORMAT_LOTUS        ""
#endif

#if !defined(VIEWSHEET_OFF)
#   define  TYPES_VIEWSHEET     "S"
#   define  FORMAT_VIEWSHEET    ",\177ViewSheet"
#else
#   define  TYPES_VIEWSHEET     ""
#   define  FORMAT_VIEWSHEET    ""
#endif

#define LOAD_TYPES_STR  "AP" TYPES_VIEW TYPES_LOTUS "CT" TYPES_VIEWSHEET TYPES_1WP
#define SAVE_TYPES_STR   "P" TYPES_VIEW TYPES_LOTUS "CT" TYPES_DTP

#if MS || ARTHUR
static const char LOAD_FORMAT_STR[] =
    "Format: \177Auto,\177Pd"   FORMAT_VIEW FORMAT_LOTUS    \
    ",\177Comma,\177Tab"    FORMAT_VIEWSHEET    FORMAT_1WP;

static const char SAVE_FORMAT_STR[] =
    "Format: "       "\177Pd"   FORMAT_VIEW FORMAT_LOTUS    \
    ",\177Comma,\177Tab"    FORMAT_DTP;
#endif


/* dialog boxes - arrays of dialog box entries */

/* get the offset of variable var in the windvars structure */
#define WV(var)     ((int) &((window_data *) NULL)->var)

/* var field can be one of three things:
 * NULL             variable only stored in dialog box
 * WV(variable)     variable stored in a windvars variable
*/

#if RISCOS
#   define dentry(s, h, f, o1, o2, opt, ol, var)    {       f, o1, o2, opt, ol, NULL, var }
#elif MS || ARTHUR
#   define dentry(s, h, f, o1, o2, opt, ol, var)    { s, h, f, o1, o2, opt, ol, NULL, var }
#endif


#if !RISCOS
DIALOG d_error[] =
    {
    dentry(NULL,                                    HELP_PRESS_KEY, F_ERRORTYPE,    '\0','\0',  NO_OPT, NO_LIST,            NULL    )
    };
#endif


DIALOG d_load[] =
    {
    dentry(NAME_OF_FILE_STR,                        HELP_FILENAME,  F_TEXT,         '\0','\0',  NO_OPT, LOAD_INIT_STR,      NULL    ),
    dentry("Insert at slot?",                       HELP_SLOT_REF,  F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Load only range of rows",               HELP_FL_ROWS,   F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry(LOAD_FORMAT_STR,                         HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, LOAD_TYPES_STR,     NULL    )
    };


DIALOG d_save_existing[] =
    {
    dentry("Save text?",                            HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,           NULL    ),
    dentry(NAME_OF_FILE_STR,                        HELP_FILENAME,  F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,            NULL    )
    };


DIALOG d_overwrite[] =
    {
    dentry("Overwrite existing file?",              HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,           NULL    )
    };


DIALOG d_saveinit[] =
    {
    dentry(NAME_OF_FILE_STR,                        HELP_FILENAME,  F_TEXT,         '\0','\0',  NO_OPT, PD_INITFILE_STR,    NULL    )
    };


DIALOG d_save[] =
    {
    dentry(NAME_OF_FILE_STR,                        HELP_FILENAME,  F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,            NULL    ),
    dentry("Save only range of columns",            HELP_FL_COLS,   F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Save selection of rows",                HELP_CONDITION, F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Save only marked block",                HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Line separator",                        HELP_ARROWS,    F_ARRAY,        'C','R',    NO_OPT, (char *) crlf_list, NULL    ),
    dentry(SAVE_FORMAT_STR,                         HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, SAVE_TYPES_STR,     WV(Xd_save_FORMAT)  )
    };


DIALOG d_print[] =
    {
    dentry("To \177Printer, \177Screen or \177File",HELP_FILENAME,  F_COMPOSITE,    '\0','\0',  NO_OPT, "PSF",              NULL    ),
    dentry("Using parameter file",                  HELP_FILENAME,  F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Omit blank fields",                     HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Print only range of columns",           HELP_FL_COLS,   F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Print selection of rows",               HELP_CONDITION, F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Print only marked block",               HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Two sided",                             HELP_ARROWS,    F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Number of copies",                      HELP_ARROWS,    F_NUMBER,       '\0','\0',  NO_OPT, ONESTR,             NULL    ),
    dentry("Wait between pages",                    HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,           NULL    )
#if RISCOS
,   dentry("\177Portrait or \177Landscape",         HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, "PL",               NULL    )
,   dentry("Print scale",                           HELP_ARROWS,    F_NUMBER,       '\0','\0',  NO_OPT, "100",              NULL    )
#endif
    };


DIALOG d_mspace[] =
    {
    dentry("Microspace on?",                        HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,           NULL    ),
    dentry("Microspace pitch",                      HELP_ARROWS,    F_NUMBER,       '\0','\0',  NO_OPT, "12",               NULL    ),
    };


DIALOG d_driver[] =
    {
    dentry("Printer driver",                        HELP_FILENAME,  F_TEXT,         'P','D',    NO_OPT, NO_LIST,            NULL    ),
    dentry("Type",                                  HELP_ARROWS,    F_ARRAY,        'P','T',    NO_OPT, (char *) port_list, NULL    ),
#if ARTHUR || RISCOS
    dentry("Printer server number",                 HELP_FILENAME,  F_TEXT,         'P','N',    NO_OPT, "0.235",            NULL    ),
#endif
    dentry("Serial baud rate",                      HELP_ARROWS,    F_ARRAY,        'P','B',    NO_OPT, (char *) baud_list, NULL    ),
    dentry("Data bits",                             HELP_ARROWS,    F_SPECIAL,      'P','W',    NO_OPT, "84567",            NULL    ),
    dentry("Parity: \177Even,\177Odd,\177None",     HELP_ARROWS,    F_SPECIAL,      'P','P',    NO_OPT, "NEO",              NULL    ),
    dentry("Stop bits",                             HELP_ARROWS,    F_SPECIAL,      'P','O',    NO_OPT, "12",               NULL    )
    };


DIALOG d_poptions[] =
    {
    dentry("Page length",                           HELP_ARROWS,    F_NUMBER,           'P','L',    NO_OPT, "66",       WV(Xd_poptions_PL)  ),
    dentry("Line spacing",                          HELP_ARROWS,    F_NUMBER,           'L','S',    NO_OPT, ONESTR,     WV(Xd_poptions_LS)  ),
    dentry("Start page",                            NO_HELP,        F_TEXT,             'P','S',    NO_OPT, NO_LIST,    WV(Xd_poptions_PS)  ),
    dentry("Top margin",                            HELP_ARROWS,    F_NUMBER,           'T','M',    NO_OPT, ZEROSTR,    WV(Xd_poptions_TM)  ),
    dentry("Header margin",                         HELP_ARROWS,    F_NUMBER,           'H','M',    NO_OPT, TWOSTR,     WV(Xd_poptions_HM)  ),
    dentry("Footer margin",                         HELP_ARROWS,    F_NUMBER,           'F','M',    NO_OPT, TWOSTR,     WV(Xd_poptions_FM)  ),
    dentry("Bottom margin ",                        HELP_ARROWS,    F_NUMBER,           'B','M',    NO_OPT, "8",        WV(Xd_poptions_BM)  ),
    dentry("Left margin",                           HELP_ARROWS,    F_NUMBER,           'L','M',    NO_OPT, ZEROSTR,    WV(Xd_poptions_LM)  ),
    dentry("Header",                                HELP_LCR,       F_TEXT,             'H','E',    NO_OPT, NO_LIST,    WV(Xd_poptions_HE)  ),
    dentry("Footer",                                HELP_LCR,       F_TEXT,             'F','O',    NO_OPT, NO_LIST,    WV(Xd_poptions_FO)  )
    };


DIALOG d_sort[] =
    {
#if TRUE
    dentry("Sort on column",                        HELP_COLUMN,    F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("Ascending/descending",                  HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Sort on column",                        HELP_COLUMN,    F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("Ascending/descending",                  HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Sort on column",                        HELP_COLUMN,    F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("Ascending/descending",                  HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Sort on column",                        HELP_COLUMN,    F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("Ascending/descending",                  HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Sort on column",                        HELP_COLUMN,    F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("Ascending/descending",                  HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Update references",                     HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Multi-row records",                     HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    )
#else
    dentry("Sort on column",                        HELP_COLUMN,    F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("In ascending order",                    HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Update references",                     HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Multi-row records",                     HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    )
#endif
    };


DIALOG d_replicate[] =
    {
    dentry("Range to copy from",                    HELP_FL_SLOTS,  F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("Range to copy to",                      HELP_FL_SLOTS,  F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    )
    };


/* order of d_search has to correspond with manifests in pdsearch.c */

DIALOG d_search[] =
    {
    dentry("String to search for",          NO_HELP,        F_TEXT,         '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("Replace with",                  NO_HELP,        F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,   NULL    ),
    dentry("Ask for confirmation",          HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Match upper and lower case",    HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, YESNOSTR,   NULL    ),
    dentry("Search expression slots",       HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,   NULL    ),
    dentry("Search only marked block",      HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,   NULL    ),
    dentry("Search only range of columns",  HELP_FL_COLS,   F_COMPOSITE,    '\0','\0',  NO_OPT, NOYESSTR,   NULL    ),
    dentry("Search all files in list",      HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,   NULL    ),
    dentry("Search from current file",      HELP_ARROWS,    F_SPECIAL,      '\0','\0',  NO_OPT, NOYESSTR,   NULL    )
    };


DIALOG d_options[] =
    {
    dentry("Title",                         NO_HELP,        F_TEXT,     'D','E',    NO_OPT, NO_LIST,        WV(Xd_options_DE)   ),
    dentry("Text/numbers",                  HELP_ARROWS,    F_SPECIAL,  'T','N',    NO_OPT, "TN",           WV(Xd_options_TN)   ),
    dentry("Insert on wrap",                HELP_ARROWS,    F_SPECIAL,  'I','W',    NO_OPT, "RC",           WV(Xd_options_IW)   ),
    dentry("Borders",                       HELP_ARROWS,    F_SPECIAL,  'B','O',    NO_OPT, YESNOSTR,       WV(Xd_options_BO)   ),
    dentry("Justify",                       HELP_ARROWS,    F_SPECIAL,  'J','U',    NO_OPT, NOYESSTR,       WV(Xd_options_JU)   ),
    dentry("Wrap",                          HELP_ARROWS,    F_SPECIAL,  'W','R',    NO_OPT, YESNOSTR,       WV(Xd_options_WR)   ),
    dentry("Decimal places",                HELP_ARROWS,    F_SPECIAL,  'D','P',    NO_OPT, DecimalPlaces_Parm_STR, WV(Xd_options_DP)   ),
    dentry("Minus/brackets",                HELP_ARROWS,    F_SPECIAL,  'M','B',    NO_OPT, "MB",           WV(Xd_options_MB)   ),
    dentry("Thousands separator",           HELP_ARROWS,    F_ARRAY,    'T','H',    NO_OPT, (char *) thousands_list,    WV(Xd_options_TH)   ),
    dentry("Insert on return",              HELP_ARROWS,    F_SPECIAL,  'I','R',    NO_OPT, NOYESSTR,       WV(Xd_options_IR)   ),
    dentry("Date format",                   HELP_ARROWS,    F_SPECIAL,  'D','F',    NO_OPT, "EAT",          WV(Xd_options_DF)   ),
    dentry("Leading characters",            NO_HELP,        F_TEXT,     'L','P',    NO_OPT, POUND_STR,      WV(Xd_options_LP)   ),
    dentry("Trailing characters",           NO_HELP,        F_TEXT,     'T','P',    NO_OPT, "%",            WV(Xd_options_TP)   )
#ifdef GRID_ON
,   dentry("Grid",                          HELP_ARROWS,    F_SPECIAL,  'G','R',    NO_OPT, NOYESSTR,       WV(Xd_options_GR)   )
#endif
    };


DIALOG d_parameter[] =
    {
    dentry("Parameter number",              HELP_ARROWS,    F_LIST,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Field",                         HELP_ARROWS,    F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_width[] =
    {
    dentry(NULL,                            HELP_ARROWS,    F_NUMBER,   '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Specify column range",          HELP_FL_COLS,   F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_name[] =
    {
    dentry(NAME_OF_FILE_STR,                HELP_FILENAME,  F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_execfile[] =
    {
    dentry(NAME_OF_FILE_STR,                HELP_FILENAME,  F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_goto[] =
    {
    dentry("Go to slot",                    HELP_SLOT_REF,  F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_decimal[] =
    {
    dentry("Number of decimal places",      HELP_ARROWS,    F_SPECIAL,  '\0','\0',  NO_OPT, DecimalPlaces_Parm_STR, NULL    )
    };


DIALOG d_inshigh[] =
    {
    dentry("Highlight number",              HELP_ARROWS,    F_SPECIAL,  '\0','\0',  NO_OPT, "12345678",     NULL    )
    };


DIALOG d_defkey[] =
    {
    dentry("Key to define",                 HELP_ARROWS,    F_LIST,     '\0','\0',  128,    NO_LIST,        NULL    ),
    dentry("Definition",                    NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_def_fkey[] =
    {
    dentry("Function key to define",        HELP_ARROWS,    F_ARRAY,    '\0','\0',  NO_OPT, (char *) func_list, NULL    ),
    dentry("Definition",                    NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_inspage[] =
    {
    dentry("Number of unbroken lines",      HELP_ARROWS,    F_NUMBER,   '\0','\0',  NO_OPT, ZEROSTR,        NULL    )
    };


DIALOG d_create[] =
    {
    dentry("Number of file",                HELP_ARROWS,    F_NUMBER,   '\0','\0',  NO_OPT, ONESTR,         NULL    ),
    dentry("Number of columns",             HELP_ARROWS,    F_NUMBER,   '\0','\0',  NO_OPT, ONESTR,         NULL    ),
    dentry("Number of rows",                HELP_ARROWS,    F_NUMBER,   '\0','\0',  NO_OPT, ONESTR,         NULL    )
    };


DIALOG d_colours[] =
    {
    dentry("Text",                          HELP_ARROWS,    F_COLOUR,   'C','F',    NO_OPT, FORESTR,        NULL    ),
    dentry("Background",                    HELP_ARROWS,    F_COLOUR,   'C','B',    NO_OPT, BACKSTR,        NULL    ),
    dentry("Border",                        HELP_ARROWS,    F_COLOUR,   'C','S',    NO_OPT, BORDERSTR,      NULL    )
#if MS || ARTHUR
,   dentry("Menu text",                     HELP_ARROWS,    F_COLOUR,   'C','M',    NO_OPT, MENU_FORESTR,   NULL    )
,   dentry("Menu background",               HELP_ARROWS,    F_COLOUR,   'C','N',    NO_OPT, MENU_BACKSTR,   NULL    )
,   dentry("Menu cursor",                   HELP_ARROWS,    F_COLOUR,   'C','H',    NO_OPT, MENU_HOTSTR,    NULL    )
#endif
,   dentry("Negative numbers",              HELP_ARROWS,    F_COLOUR,   'C','L',    NO_OPT, NEGATIVESTR,    NULL    )
,   dentry("Protected slots",               HELP_ARROWS,    F_COLOUR,   'C','P',    NO_OPT, PROTECTSTR,     NULL    )
#if RISCOS
,   dentry("Caret colour",                  HELP_ARROWS,    F_COLOUR,   'C','C',    NO_OPT, CARETSTR,       NULL    )
#endif
,   dentry("Current border",                HELP_ARROWS,    F_COLOUR,   'C','T',    NO_OPT, CURBORDERSTR,   NULL    )
    };


#if RISCOS
#define DEF_INS_CHAR 0xC0
#else
#define DEF_INS_CHAR 0x80
#endif

DIALOG d_inschar[] =
    {
    dentry("Character",                     HELP_ARROWS,    F_CHAR,     '\0','\0',  DEF_INS_CHAR,   NO_LIST,        NULL    )
    };


DIALOG d_about[] =
    {
#if defined(SMALLPD)
    dentry("Small internal PD",             HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
#else
    dentry(nameandversionstring,            HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
#endif
    dentry("Copyright Colton Software 1989",HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Registration number:",          HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


#if !RISCOS

DIALOG d_status[] =
    {
    dentry("Page",                              HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Type action",                       HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Printer driver",                    HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    )
#if defined(BYTES_FREE)
,   dentry("Bytes free",                        HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    )
#endif
#if defined(HEAP_INFO)
,   dentry("Cache blocks",                      HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Largest block",                     HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Total cache size",                  HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Free blocks in heap",               HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Largest free block",                HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Used blocks in heap",               HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Largest used block",                HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    )
#endif
    };

#endif


DIALOG d_count[] =
    {
    dentry(NULLSTR,                             HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


#if !defined(SPELL_OFF)

DIALOG d_auto[] =
    {
    dentry("Automatic check?",                  HELP_ARROWS,    F_SPECIAL,  'S','A',    NO_OPT, NOYESSTR,       NULL    )
    };


DIALOG d_checkon[] =
    {
    dentry("Check marked block",                HELP_ARROWS,    F_SPECIAL,  '\0','\0',  NO_OPT, NOYESSTR,       NULL    ),
    dentry("Check all files in list",           HELP_ARROWS,    F_SPECIAL,  '\0','\0',  NO_OPT, NOYESSTR,       NULL    )
    };


DIALOG d_check[] =
    {
    dentry("Change word",                       NO_HELP,        F_COMPOSITE,'\0','\0',  NO_OPT, NOYESSTR,       NULL    ),
    dentry("Browse",                            HELP_ARROWS,    F_SPECIAL,  '\0','\0',  NO_OPT, NOYESSTR,       NULL    ),
    dentry("Add to user dictionary",            NO_HELP,        F_COMPOSITE,'\0','\0',  NO_OPT, NOYESSTR,       NULL    )
    };


DIALOG d_check_mess[] =
    {
    dentry(NULLSTR,                             HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry(NULLSTR,                             HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_user_create[] =
    {
    dentry(dictionarynamestr,                   NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, USERDICT_STR,   NULL    )
    };


DIALOG d_user_open[] =
    {
    dentry(dictionarynamestr,                   NO_HELP,        F_TEXT,     'S','O',    NO_OPT, USERDICT_STR,   NULL    )
    };


DIALOG d_user_close[] =
    {
    dentry(dictionarynamestr,                   NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, USERDICT_STR,   NULL    )
    };


DIALOG d_user_delete[] =
    {
    dentry("Word to delete",                    NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry(dictionarynamestr,                   NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_user_insert[] =
    {
    dentry("Word to insert",                    NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry(dictionarynamestr,                   NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_user_browse[] =
    {
    dentry("Word",                              NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry(userdictionaryqstr,                  NO_HELP,        F_COMPOSITE,'\0','\0',  NO_OPT, NOYESSTR,       NULL    )
    };


DIALOG d_user_dump[] =
    {
    dentry("Word template",                     NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("Filename",                          NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry(userdictionaryqstr,                  NO_HELP,        F_COMPOSITE,'\0','\0',  NO_OPT, NOYESSTR,       NULL    )
    };


DIALOG d_user_merge[] =
    {
    dentry("Filename",                          NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry(userdictionaryqstr,                  NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_user_anag[] =
    {
    dentry("Word",                              NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry(userdictionaryqstr,                  NO_HELP,        F_COMPOSITE,'\0','\0',  NO_OPT, NOYESSTR,       NULL    )
    };


DIALOG d_user_lock[] =
    {
    dentry(userdictionaryqstr,                  NO_HELP,        F_COMPOSITE,'\0','\0',  NO_OPT, NOYESSTR,       NULL    )
    };


DIALOG d_user_unlock[] =
    {
    dentry(userdictionaryqstr,                  NO_HELP,        F_COMPOSITE,'\0','\0',  NO_OPT, NOYESSTR,       NULL    )
    };


DIALOG d_user_pack[] =
    {
    dentry("Old user dictionary",               NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    ),
    dentry("New user dictionary",               NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };

#endif  /* SPELL_OFF */


DIALOG d_pause[] =
    {
    dentry("Seconds to pause",                  HELP_ARROWS,    F_NUMBER,   '\0','\0',  NO_OPT, NO_LIST,        NULL    )
    };


DIALOG d_save_deleted[] =
    {
    dentry("Cannot store block - continue deletion?",   HELP_ARROWS,    F_COMPOSITE,'\0','\0',  NO_OPT, NOYESSTR,   NULL    )
    };


/* This is a dummy dialog box, used only for loading and saving fix info.
 * Doesn't need graphic representation.
*/

DIALOG d_fixes[] =
    {
    dentry(NULL,                                    0,              F_TEXT,     'F','R',    NO_OPT, NO_LIST,    NULL    ),
    dentry(NULL,                                    0,              F_TEXT,     'F','C',    NO_OPT, NO_LIST,    NULL    )
    };


DIALOG d_deleted[] =
    {
    dentry("Number of deletions to be remembered",  HELP_ARROWS,    F_NUMBER,   'D','N',    D_PASTE_SIZE,   NO_LIST,    NULL    )
    };


DIALOG d_recalc[] =
    {
    dentry("Calc: auto/manual",                     HELP_ARROWS,    F_SPECIAL,  'A','M',    NO_OPT, "AM",       WV(Xd_recalc_AM)    ),
    dentry("Calc: columns/rows/natural",            HELP_ARROWS,    F_SPECIAL,  'R','C',    NO_OPT, "NCR",      WV(Xd_recalc_RC)    ),
    dentry("Iteration",                             HELP_ARROWS,    F_SPECIAL,  'R','I',    NO_OPT, NOYESSTR,   WV(Xd_recalc_RI)    ),
    dentry("Maximum number of iterations",          NO_HELP,        F_TEXT,     'R','N',    NO_OPT, "100",      WV(Xd_recalc_RN)    ),
    dentry("Maximum change between iterations",     NO_HELP,        F_TEXT,     'R','B',    NO_OPT, "0.001",    WV(Xd_recalc_RB)    )
    };


/* This is a dummy dialog box, used only for loading and saving protect info.
 * Doesn't need graphic representation.
*/

DIALOG d_protect[] =
    {
    dentry(NULLSTR,                             0,              F_TEXT,     'P','Z',    NO_OPT, NO_LIST,    NULL    )
    };


DIALOG d_macro_file[] =
    {
    dentry(NAME_OF_FILE_STR,                    HELP_FILENAME,  F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,    NULL    )
    };


#if RISCOS
DIALOG d_fonts[] =
    {
    dentry(NULLSTR,                             NO_HELP,        F_TEXT,     'F','G',    NO_OPT, NO_LIST,    NULL    ),
    dentry(NULLSTR,                             NO_HELP,        F_TEXT,     'F','X',    NO_OPT, NO_LIST,    NULL    ),
    dentry(NULLSTR,                             NO_HELP,        F_TEXT,     'F','Y',    NO_OPT, NO_LIST,    NULL    ),
    dentry(NULLSTR,                             NO_HELP,        F_TEXT,     'F','S',    NO_OPT, NO_LIST,    NULL    )
    };
#endif


DIALOG d_menu[] =
    {
    dentry(NULLSTR,                             NO_HELP,        F_NUMBER,   'M','S',    NO_OPT, ONESTR,     NULL    )
    };


DIALOG d_def_cmd[] =
    {
    dentry("Command to define",                 NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,    NULL    ),
    dentry("Command to execute",                NO_HELP,        F_TEXT,     '\0','\0',  NO_OPT, NO_LIST,    NULL    )
    };


#if ARTHUR
DIALOG d_fserr[] =
    {
    dentry(NULL,                                HELP_PRESS_KEY, F_ERRORTYPE,'\0','\0',  NO_OPT, NO_LIST,    NULL    )
    };
#endif


#if RISCOS
#   define dhead(text, dial, width, proc, name, flags)  \
    { dial, sizeof(dial) / sizeof(DIALOG), flags, proc, name    }
#elif MS || ARTHUR
#   define dhead(text, dial, width, proc, name, flags)  \
    { dial, sizeof(dial) / sizeof(DIALOG), text, width          }
#endif

#define dhead_none  dhead((void *) 1, d_pause, 0, NULL, BAD_POINTER, 0)

#define EXEC_CAN_FILL   0x0001

DHEADER dialog_head[] =
    {
#if RISCOS
    dhead_none,
#else
    dhead(  "Error",                        d_error,        25, NULL,               BAD_POINTER,    0               ),  /* no dialog box on RISCOS */
#endif
    dhead(  "Load",                         d_load,         39, dproc_loadfile,     "loadfile",     EXEC_CAN_FILL   ),
    dhead(  "Text modified",                d_save_existing,23, dproc_save_existing,NULL,           0               ),  /* uses dboxquery on RISCOS */
    dhead(  "Overwrite file",               d_overwrite,    28, dproc_overwrite,    NULL,           0               ),  /* uses dboxquery on RISCOS */
    dhead(  "Save",                         d_save,         39, dproc_savefile,     "savefile",     EXEC_CAN_FILL   ),
    dhead(  "Save initialisation file",     d_saveinit,     34, dproc_onetext,      "onefile",      EXEC_CAN_FILL   ),
    dhead(  "Print",                        d_print,        34, dproc_print,        "print",        EXEC_CAN_FILL   ),
    dhead(  "Microspace",                   d_mspace,       32, dproc_microspace,   "microspace",   EXEC_CAN_FILL   ),
    dhead(  "Printer configuration",        d_driver,       28, dproc_printconfig,  "printconfig",  EXEC_CAN_FILL   ),
    dhead(  "Page layout",                  d_poptions,     20, dproc_pagelayout,   "pagelayout",   EXEC_CAN_FILL   ),
    dhead(  "Sort",                         d_sort,         27, dproc_sort,         "sort",         EXEC_CAN_FILL   ),
    dhead(  "Replicate",                    d_replicate,    21, dproc_twotext,      "replicate",    EXEC_CAN_FILL   ),
    dhead(  "Search",                       d_search,       32, dproc_search,       "search",       EXEC_CAN_FILL   ),
    dhead(  "Options",                      d_options,      30, dproc_options,      "options",      EXEC_CAN_FILL   ),
    dhead(  "Parameter",                    d_parameter,    20, dproc_numtext,      "setparm",      EXEC_CAN_FILL   ),
    dhead(  "Column width",                 d_width,        28, dproc_numtext,      "columnwidth",  EXEC_CAN_FILL   ),
    dhead(  "Right margin",                 d_width,        32, dproc_numtext,      "rightmargin",  EXEC_CAN_FILL   ),
    dhead(  "Name of file",                 d_name,         18, dproc_onetext,      "namefile",     EXEC_CAN_FILL   ),
    dhead(  "Exec file",                    d_execfile,     18, dproc_onetext,      "onefile",      EXEC_CAN_FILL   ),
    dhead(  "Go to slot",                   d_goto,         14, dproc_onetext,      "gotoslot",     EXEC_CAN_FILL   ),
    dhead(  "Decimal places",               d_decimal,      28, dproc_decimal,      "decimal",      EXEC_CAN_FILL   ),
    dhead(  "Remove highlights",            d_inshigh,      24, dproc_onespecial,   "insremhigh",   EXEC_CAN_FILL   ),
    dhead(  "Insert highlights",            d_inshigh,      24, dproc_onespecial,   "insremhigh",   EXEC_CAN_FILL   ),
    dhead(  "Define key",                   d_defkey,       24, dproc_defkey,       "defkey",       EXEC_CAN_FILL   ),
    dhead(  "Define function key",          d_def_fkey,     28, dproc_deffnkey,     "deffnkey",     EXEC_CAN_FILL   ),
    dhead(  "Insert page break",            d_inspage,      28, dproc_onenumeric,   "pagebreak",    EXEC_CAN_FILL   ),
    dhead(  "Create linking file",          d_create,       25, dproc_createlinking,"createlink",   EXEC_CAN_FILL   ),
    dhead(  "Screen colours",               d_colours,      23, dproc_colours,      "colours",      EXEC_CAN_FILL   ),
    dhead(  "Insert",                       d_inschar,      17, dproc_insertchar,   "insertchar",   EXEC_CAN_FILL   ),
    dhead(  applicationname,                d_about,        25, dproc_aboutfile,    "aboutfile",    EXEC_CAN_FILL   ),
#if RISCOS
    dhead_none,
#else
    #if defined(HEAP_INFO)
        dhead(  "Status",                   d_status,       22, NULL,               NULL,           EXEC_CAN_FILL   ),
    #else
        dhead(  "Status",                   d_status,       18, NULL,               NULL,           EXEC_CAN_FILL   ),
    #endif
#endif
    dhead(  "Count",                        d_count,        25, dproc_onetext,      "count",        EXEC_CAN_FILL   ),
    dhead(  "Pause",                        d_pause,        25, dproc_onenumeric,   "pause",        EXEC_CAN_FILL   )
#if defined(SAVE_DELETED_BLOCK)
,   dhead(  "Saving block",                 d_save_deleted, 45, dproc_save_deleted, NULL,           0               )   /* uses dboxquery on RISCOS */
#endif

#if !defined(SPELL_OFF)
,   dhead(  "Automatic Check",              d_auto,         25, NULL,               BAD_POINTER,    EXEC_CAN_FILL   ),  /* not produced on RISCOS */
    dhead(  "Check document",               d_checkon,      29, dproc_checkdoc,     "checkdoc",     EXEC_CAN_FILL   ),
    dhead(  "Checking",                     d_check,        29, dproc_checking,     "checking",     EXEC_CAN_FILL   ),
    dhead(  "Checked",                      d_check_mess,   29, dproc_checked,      "checked",      EXEC_CAN_FILL   ),
    dhead(  "Create user dictionary",       d_user_create,  29, dproc_onetext,      "cropcldict",   EXEC_CAN_FILL   ),
    dhead(  "Open user dictionary",         d_user_open,    29, dproc_onetext,      "cropcldict",   EXEC_CAN_FILL   ),
    dhead(  "Close user dictionary",        d_user_close,   29, dproc_onetext,      "cropcldict",   EXEC_CAN_FILL   ),
    dhead(  "Delete from user dictionary",  d_user_delete,  31, dproc_twotext,      "insdelword",   EXEC_CAN_FILL   ),
    dhead(  "Insert in user dictionary",    d_user_insert,  29, dproc_twotext,      "insdelword",   EXEC_CAN_FILL   ),
    dhead(  "Browse",                       d_user_browse,  29, dproc_browse,       "browse",       EXEC_CAN_FILL   ),
    dhead(  "Dump",                         d_user_dump,    29, dproc_dumpdict,     "dumpdict",     EXEC_CAN_FILL   ),
    dhead(  "Merge",                        d_user_merge,   29, dproc_twotext,      "mergedict",    EXEC_CAN_FILL   ),
    dhead(  "Anagram",                      d_user_anag,    29, dproc_anasubgram,   "anasubgram",   EXEC_CAN_FILL   ),
    dhead(  "Subgram",                      d_user_anag,    29, dproc_anasubgram,   "anasubgram",   EXEC_CAN_FILL   ),
    dhead(  "Lock",                         d_user_lock,    29, dproc_onecomponoff, "unlockdict",   EXEC_CAN_FILL   ),
    dhead(  "Unlock",                       d_user_unlock,  29, dproc_onecomponoff, "unlockdict",   EXEC_CAN_FILL   ),
    dhead(  "Pack",                         d_user_pack,    29, dproc_twotext,      "packuser",     EXEC_CAN_FILL   )
#endif

#if ARTHUR
,   dhead(  "Filing system",                d_fserr,        32, NULL,               NULL,           EXEC_CAN_FILL   )
#endif  

    /* dummy dialog box for saving options */
,   dhead(  "",                             d_fixes,        0,  NULL,               BAD_POINTER,    0               )   /* no dialog box */
,   dhead(  "Paste",                        d_deleted,      42, dproc_onenumeric,   "pastedepth",   EXEC_CAN_FILL   )
,   dhead(  "Recalculation options",        d_recalc,       42, dproc_recalc,       "recalcopts",   EXEC_CAN_FILL   )

    /* dummy dialog box for saving protection */
,   dhead(  "",                             d_protect,      0,  NULL,               BAD_POINTER,    0               )   /* no dialog box */
,   dhead(  "Record macro file",            d_macro_file,   23, dproc_onetext,      "onefile",      EXEC_CAN_FILL   )
#if RISCOS
,   dhead(  "",                             d_fonts,        0,  NULL,               BAD_POINTER,    0               )   /* no dialog box */
#endif
,   dhead(  "",                             d_menu,         0,  NULL,               BAD_POINTER,    0               )   /* no dialog box */
,   dhead(  "Define command",               d_def_cmd,      28, dproc_twotext,      "defcmd",       EXEC_CAN_FILL   )
    };
#define NDIALOG_BOXES (sizeof(dialog_head) / sizeof(DHEADER))


/************************************************************
*                                                           *
*  find an option and return a pointer to the dialog entry  *
*                                                           *
************************************************************/

static DIALOG *
find_option(uchar ch1, uchar ch2, intl *dbox_num)
{
    DHEADER *dhptr      = dialog_head;
    DHEADER *last_dhptr = dhptr + NDIALOG_BOXES;

    --dhptr;

    /* for each dialog box */
    do  {
        DIALOG *dptr        = dhptr->dialog_box;
        DIALOG *last_dptr   = dptr + dhptr->items;

        /* for each entry in each dialog box */
        do  {
            if((dptr->ch1 == ch1)  &&  (dptr->ch2 == ch2))
                {
                *dbox_num = dhptr - dialog_head;
                return(dptr);
                }
            }
        while(++dptr < last_dptr);
        }
    while(++dhptr < last_dhptr);

    *dbox_num = 0;
    return(NULL);
}


/****************************************************************
*                                                               *
* get option reads an option from the input and adds it to list *
* looks for %OP%mnstring CR where mn=option mnemonic            *
*                                                               *
****************************************************************/

extern void
getoption(uchar *optr)
{
    DIALOG *dptr;
    uchar array[LIN_BUFSIZ];
    uchar *to = array;
    uchar *from;
    intl dbox_num;

    if(memcmp(optr, "%OP%", (unsigned) 4))
        return;

    tracef1("getoption(%s)\n", optr);

    optr += 4;

    dptr = find_option(optr[0], optr[1], &dbox_num);
    if(!dptr)
        {
        tracef0("no such option found\n");
        return;
        }

    update_dialog_from_windvars(dbox_num);

    from = optr + 2;

    switch(dptr->type)
        {
        case F_SPECIAL:
            dptr->option = *from;
            break;

        case F_ARRAY:
        case F_COLOUR:
        case F_NUMBER:
        case F_LIST:
            dptr->option = (uchar) atoi((char *) from);
            break;

        case F_TEXT:
            #if !defined(SPELL_OFF)
            /* user dictionaries have to be treated specially,
             * 'cos there may be more than one
            */
            if(dptr == d_user_open)
                dict_number((char *) from, TRUE);
            #endif

            if(dptr == d_protect)
                add_to_protect_list(from);

            /* copy string and encode highlights  */

            while(*from)
                if( (*from == '%')
                    &&  (from[1] == 'H')  &&  (from[3] == '%')
                    &&  ishighlighttext(from[2]))
                    {
                    *to++ = from[2] - FIRST_HIGHLIGHT_TEXT + FIRST_HIGHLIGHT;
                    from += 4;
                    }
                else
                    *to++ = *from++;

            *to = '\0';
            (void) str_set(&dptr->textfield, array);
            break;

        default:
            break;
        }

    update_windvars_from_dialog(dbox_num);
}


/****************************************************************
*                                                               *
* save the option pointed at by dptr to the default option list *
*                                                               *
****************************************************************/

static void
save_opt_to_list(DIALOG *start, intl n)
{
    DIALOG *dptr        = start;
    DIALOG *last_dptr   = dptr + n;
    word32 key;
    char array[20];
    char *ptr;
    intl res;

    for(; dptr < last_dptr; dptr++)
        {
        key = (((word32) dptr->ch1) << 8) + (word32) dptr->ch2;
        ptr = array;

        switch(dptr->type)
            {
            case F_TEXT:
                if(str_isblank(dptr->textfield))
                    continue;
                ptr = dptr->textfield;
                break;

            case F_ARRAY:
                if(!dptr->option)
                    continue;
                sprintf(ptr, "%d", (int) dptr->option);
                break;

            case F_LIST:
            case F_NUMBER:
            case F_COLOUR:
                if((int) dptr->option == atoi(dptr->optionlist))
                    continue;
                sprintf(ptr, "%d", (int) dptr->option);
                break;

            case F_SPECIAL:
                if(dptr->option == *dptr->optionlist)
                    continue;
                ptr[0] = dptr->option;
                ptr[1] = '\0';
                break;

            default:
                break;
            }

        res = add_to_list(&def_first_option, key, (uchar *) ptr, &res);

        if(res <= 0)
            {
            reperr_null(res ? res : ERR_NOROOM);
            break;
            }
        }
}


/* write an intl into a str_set string */

static BOOL
write_int_to_string(char **var, intl number)
{
    char array[32];
    sprintf(array, "%d", number);
    return(str_setc(var, array));
}

#if RISCOS

extern void
update_dialog_from_fontinfo(void)
{
    init_dialog_box(D_FONTS);

    if(global_font)
        {
        /* even saves defaults onto list */
        (void) str_set(            &d_fonts[D_FONTS_G].textfield, global_font);
        (void) write_int_to_string(&d_fonts[D_FONTS_X].textfield, global_font_x);
        (void) write_int_to_string(&d_fonts[D_FONTS_Y].textfield, global_font_y);
        (void) write_int_to_string(&d_fonts[D_FONTS_S].textfield, global_font_leading);
        }
}


extern BOOL ensure_global_font_valid(void); /* from slotconv */

extern void
update_fontinfo_from_dialog(void)
{
    str_clr(&global_font);

    if(d_fonts[D_FONTS_G].textfield)
        {
        str_swap(&global_font, &d_fonts[D_FONTS_G].textfield);
        /* try the new font out */
        riscos_fonts = TRUE;
        riscos_font_error = FALSE;
        ensure_global_font_valid();
        }
    else
        riscos_fonts = FALSE;

    if(d_fonts[D_FONTS_X].textfield)
        {
        sscanf(d_fonts[D_FONTS_X].textfield, "%d", &global_font_x); 
        str_clr(&d_fonts[D_FONTS_X].textfield);
        }

    if(d_fonts[D_FONTS_Y].textfield)
        {
        sscanf(d_fonts[D_FONTS_Y].textfield, "%d", &global_font_y); 
        str_clr(&d_fonts[D_FONTS_Y].textfield);
        }

    if(d_fonts[D_FONTS_S].textfield)
        {
        sscanf(d_fonts[D_FONTS_S].textfield, "%d", &global_font_leading); 
        str_clr(&d_fonts[D_FONTS_S].textfield);
        }
}

#endif

/************************************************************
*                                                           *
* save the current options on to the default list           *
* done after loading pd.ini                                 *
* these will be recovered on an implicit or explicit new    *
*                                                           *
************************************************************/

extern void
save_options_to_list(void)
{
    update_all_dialog_from_windvars();

    update_dialog_from_fontinfo();

    save_opt_to_list(d_options, dialog_head[D_OPTIONS].items);
    save_opt_to_list(d_poptions, dialog_head[D_POPTIONS].items);
    save_opt_to_list(d_recalc, dialog_head[D_RECALC].items);
    save_opt_to_list(d_save + SAV_LINESEP, 1);

#if RISCOS
    save_opt_to_list(d_fonts, dialog_head[D_FONTS].items);
#endif

/* no update_fontinfo_from_dialog(); needed */

    update_all_windvars_from_dialog();
}


/****************************************************************
*                                                               *
* on a new recover the options from the pd.ini file which were  *
* put on the default option list                                *
* assumes that all variables are currently in dialog boxes      *   
*                                                               *
****************************************************************/

extern void
recover_options_from_list(void)
{
    LIST *lptr;
    DIALOG *dptr;
    word32 key;
    uchar ch1, ch2;
    intl dbox_num;

    

    for(lptr = first_in_list(&def_first_option);
        lptr;
        lptr = next_in_list(&def_first_option))
        {
        key = lptr->key;

        ch2 = (uchar) (key & 0xFF);
        key >>= 8;
        ch1 = (uchar) (key & 0xFF);

        dptr = find_option(ch1, ch2, &dbox_num);
        if(!dptr)
            continue;

        switch(dptr->type)
            {
            case F_SPECIAL:
                dptr->option = *(lptr->value);
                break;

            case F_ARRAY:
            case F_COLOUR:
            case F_NUMBER:
            case F_LIST:
                dptr->option = (uchar) atoi((char *) lptr->value);
                break;

            case F_TEXT:
                (void) str_set(&dptr->textfield, lptr->value);  /* keep trying */
                break;

            default:
                break;
            }
        }

    /* update any variables that are not accessed thru dialog boxes */

    setlogcolours();

    update_fontinfo_from_dialog();
}


extern void
update_dialog_from_windvars(intl boxnumber)
{
    DHEADER *dhptr    = dialog_head + boxnumber;
    DIALOG *dptr      = dhptr->dialog_box;
    DIALOG *last_dptr = dptr + dhptr->items;
    intl wvoffset;

    tracef2("update_dialog_from_windvars(%d) dialog box &%p\n", boxnumber, dhptr);

    do  {
        wvoffset = dptr->offset;

        if(wvoffset)
            {
            void *ptr = (void *) (((char *) sb) + wvoffset);

            tracef2("offset/ptr to windvars variable = &%x, &%p, ",
                    wvoffset, ptr);

            switch(dptr->type)
                {
                case F_ERRORTYPE:
                case F_TEXT:
                    /* copy the windvars variable to dialog[n].textfield */
                    /* still need primary copy for screen updates */
                    (void) str_set(&dptr->textfield, * (char **) ptr);
                    tracef2("dialog[%d].textfield is now \"%s\"\n",
                            dptr - dhptr->dialog_box,
                            trace_string(dptr->textfield));
                    break;

                case F_NUMBER:
                case F_ARRAY:
                default:
                    /* copy the windvars variable to dialog[n].option */
                    dptr->option = * (uchar *) ptr;
                    tracef3("dialog[%d].option is now %d, '%c'\n",
                            dptr - dhptr->dialog_box,
                            dptr->option, dptr->option);
                    break;
                }
            }
        }
    while(++dptr < last_dptr);
}


extern void
update_windvars_from_dialog(intl boxnumber)
{
    DHEADER *dhptr    = dialog_head + boxnumber;
    DIALOG *dptr      = dhptr->dialog_box;
    DIALOG *last_dptr = dptr + dhptr->items;
    intl wvoffset;

    tracef2("update_windvars_from_dialog(%d) dialog box &%p\n", boxnumber, dhptr);

    do  {
        wvoffset = dptr->offset;

        if(wvoffset)
            {
            void *ptr = (void *) (((char *) sb) + wvoffset);

            tracef2("offset/ptr to windvars variable = &%x, &%p, ",
                        wvoffset, ptr);

            switch(dptr->type)
                {
                case F_ERRORTYPE:
                case F_TEXT:
                    /* move the dialog[n].textfield to windvars variable */
                    str_clr((char **) ptr);
                    * (char **) ptr = dptr->textfield;
                    dptr->textfield = NULL;
                    tracef2("windvar for [%d].textfield is now \"%s\"\n",
                                dptr - dhptr->dialog_box,
                                trace_string(* (char **) ptr));
                    break;

                case F_NUMBER:
                case F_ARRAY:
                default:
                    /* copy the dialog[n].option to windvars variable */
                    * (uchar *) ptr = dptr->option;
                    tracef3("windvar for [%d].option is now %d, '%c'\n",
                                dptr - dhptr->dialog_box,
                                * (uchar *)  ptr, * (uchar *) ptr);
                    break;
                }
            }
        }
    while(++dptr < last_dptr);
}


extern void
update_all_dialog_from_windvars(void)
{
    intl boxnumber;

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        update_dialog_from_windvars(boxnumber);     /* move wvars -> dialog */
}


extern void
update_all_windvars_from_dialog(void)
{
    intl boxnumber;

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        update_windvars_from_dialog(boxnumber);     /* move dialog -> wvars */
}


/****************************************************
*                                                   *
* destroy all windvars fields in all dialog boxes   *
* done on destruction of each new document          *
*                                                   *
****************************************************/

extern void
dialog_finalise(void)
{
    intl boxnumber;

    tracef0("dialog_finalise()\n");

    update_all_dialog_from_windvars();              /* move wvars -> dialog */

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        /* deallocate just windvars bits */
        init___dialog_box(boxnumber, TRUE, FALSE);
}


/********************************************************
*                                                       *
* initialise all windvars fields in all dialog boxes    *
* done on creation of each new document                 *
*                                                       *
********************************************************/

extern BOOL
dialog_initialise(void)
{
    intl boxnumber;
    BOOL res = TRUE;

    tracef0("dialog_initialise()\n");

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        /* allocate just windvars bits */
        init___dialog_box(boxnumber, (boxnumber != D_FONTS), TRUE);

        /* NB. fonts are windvars but not used via dialog vars
         * so destroy all data stored in the dialog before it gets 'restored'
        *  to the new document!
        */

    recover_options_from_list();                    /* make default dialogs */

    update_all_windvars_from_dialog();              /* move dialog -> wvars */

    return(res);
}


/************************************************************
*                                                           *
* initialise all except windvars fields in all dialog boxes *
* done only once to set up statics template                 *
*                                                           *
************************************************************/

extern void
dialog_initialise_once(void)
{
    intl boxnumber;

    tracef0("dialog_initialise_once()\n");

    /* can't do as preprocessor check */
    assert((D_THE_LAST_ONE) != (NDIALOG_BOXES+1));

    for(boxnumber = 0; boxnumber < NDIALOG_BOXES; boxnumber++)
        (void) init___dialog_box(boxnumber, FALSE, TRUE);   /* allocating non-wv strings */
}


/****************************
*                           *
* clear all fields in box i *
*                           *
****************************/

extern BOOL
init_dialog_box(intl boxnumber)
{
    BOOL res;

    res =        init___dialog_box(boxnumber,   TRUE,   TRUE);  /* do all windvars fields */
    res = res && init___dialog_box(boxnumber,   FALSE,  TRUE);  /* do all other fields */

    return(res);
}


static BOOL
init___dialog_box(intl boxnumber, BOOL windvars, BOOL allocatestrings)
{
    DHEADER *dhptr      = dialog_head + boxnumber;
    DIALOG *dptr        = dhptr->dialog_box;
    DIALOG *last_dptr   = dptr + dhptr->items;
    BOOL res            = TRUE;

    if((boxnumber == D_ABOUT)  &&  !windvars)
        /* This is fudged manually */
        {
        #if ARTHUR
        char array[256];
        *array = (RISCOS) ? 'R' : 'A';
        strcpy(array + 1, registration_number);
        return(str_setc(&d_about[2].textfield, array));
        #elif MS
        d_about[2].textfield = registration_number;
        return(res);
        #endif
        }

    do  {
        intl wvoffset = dptr->offset;

        if((wvoffset != 0) == windvars) /* init one set or the other */
            {
            /* for F_TEXT, F_COMPOSITE */
            str_clr(&dptr->textfield);

            if(dptr->optionlist != NO_LIST)
                {
                switch(dptr->type)
                    {
                    case F_TEXT:
                        if(allocatestrings)
                            res = res && str_setc(&dptr->textfield, dptr->optionlist);
                        break;

                    case F_ARRAY:
                        dptr->option = 0;
                        break;

                    case F_NUMBER:
                    case F_COLOUR:
                        dptr->option = (uchar) atoi(dptr->optionlist);
                        break;

                    case F_LIST:
                        /* optionlist gives list_block * */
                        break;                          

                /*  case F_COMPOSITE:   */
                /*  case F_SPECIAL:     */
                /*  case F_CHAR:        */
                    default:
                        dptr->option = *dptr->optionlist;
                        break;
                    }
                }
            }
        }
    while(++dptr < last_dptr);

    return(res);
}


#if MS || ARTHUR

/************************************************
*                                               *
* display the field in the dialog box at x,y    *
*                                               *
* field types are:                              *
*                                               *
*       "text field"                            *
*       [Y]             is a special            *
*       [N] "composite field"                   *
*       [7]  [  ]       is a colour             *
*       [12]            is a number             *
*       [48] [A]        is a char               *
*                                               *
************************************************/

static void
dsp_dialog_field(coord xpos, coord ypos, DIALOG *dptr)
{
    uchar bounds = QUOTES;
    uchar maxlen = 0;
    char array[10];
    intl sofar;

    /* decide delimiters for field, [] for specials, " for text */

    at(xpos-1, ypos);

    switch(dptr->type)
        {
        case F_CHAR:
        case F_COLOUR:
        case F_NUMBER:
        case F_LIST:
                if(num_in_linbuf)
                    {
                    maxlen = TEXTWIDTH;
                    bounds = ']';
                    break;
                    }

                sofar = sprintf(array, "[%d]", (int) dptr->option);
                stringout((uchar *) array);
                setcolour(MENU_FORE, MENU_BACK);
                ospca(6 - sofar);

                switch(dptr->type)
                    {
                    case F_NUMBER:
                            break;

                    case F_LIST:
                            if(dptr != d_defkey)
                                break;

                    case F_CHAR:
                            wrch('[');
                            #if MS
                                wrch(dptr->option);
                            #elif ARTHUR
                                if(dptr->option >= SPACE)
                                    wrch(dptr->option);
                                else
                                    wrch(SPACE);
                            #endif
                            setcolour(MENU_FORE, MENU_BACK);
                            wrch(']');
                            break;

                    case F_COLOUR:
                            wrch('[');
                            setcolour((uchar) (dptr-d_colours), (uchar) (dptr-d_colours));
                            ospca(5);
                            setcolour(MENU_FORE, MENU_BACK);
                            wrch(']');
                            setlogcolours();
                            break;

                    default:
                            break;

                    }

                at(xpos, ypos);
                return;

        case F_SPECIAL:
                wrch('[');
                wrch(dptr->option);
                wrch(']');
                at(xpos, ypos);
                return;

        case F_ARRAY:
                wrch('[');
                sofar = stringout(*((char **) dptr->optionlist + dptr->option));
                wrch(']');
                setcolour(MENU_FORE, MENU_BACK);
                ospca(TEXTWIDTH-sofar);
                return;

        case F_ERRORTYPE:
                bounds = SPACE;
                if(!dptr->textfield)
                    return;

                /* deliberate fall through */

        case F_TEXT:
                maxlen = TEXTWIDTH;
                break;

        case F_COMPOSITE:
                wrch('[');
                wrch(dptr->option);
                wrch(']');
                xpos += 5;
                at(xpos-1, ypos);
                maxlen = TEXTWIDTH-5;
                break;

        default:
                return;
        }

    if( num_in_linbuf  ||  (dptr->type == F_ERRORTYPE)      ||
        ((dptr->type == F_COMPOSITE)  &&  on_composite_one) )
        setcolour(MENU_FORE, MENU_BACK);

    if(num_in_linbuf)
        wrch('[');
    elif(dptr->type == F_ERRORTYPE)
        maxlen++;
    else
        wrch(bounds);

    dspfld_from = -1;
    dspfld(xpos, ypos, maxlen, bounds);

    if((dptr->type == F_COMPOSITE)  &&  on_composite_one)
        at(xpos - 5, ypos);
}


/************************************
*                                   *
*  display text for a dialog entry  *
*                                   *
************************************/

static void
dsp_dialog_text(DHEADER *header, DIALOG *dptr, coord xpos, coord ypos, coord xlen)
{
    coord sofar = 1;
    char *ptr = (char *) dptr->tag;

    at(xpos, ypos);
    wrch(SPACE);

    if(ptr)
        {
        char ch;

        while((ch = *ptr++) != '\0')
            if(ch == DIALOG_HIGHLIGHT_TOKEN)
                {
                setcolour(MENU_HOT, MENU_BACK);
                wrch(*ptr++);
                setcolour(MENU_FORE, MENU_BACK);
                sofar++;
                }
            else
                {
                wrch(ch);
                sofar++;
                }
        }

    ospca(header->tag_len - sofar);

    sofar = max(sofar, header->tag_len);

    ospca(xlen - sofar);
}


/****************************************************
*                                                   *
* determine if a command is allowed in a dialog box *
*                                                   *
****************************************************/

extern BOOL
allowed_in_dialog(intl c)
{
    switch(c)
        {
        case N_SplitLine:
            /* this only to allow split line not to beep for those people
             * who insert on carriage return
            */
        case N_CursorLeft:
        case N_CursorRight:
        case N_StartOfSlot:
        case N_EndOfSlot:
        case N_Underline:
        case N_Bold:
        case N_ExtendedSequence:
        case N_Italic:
        case N_Subscript:
        case N_Superscript:
        case N_AlternateFont:
        case N_UserDefinedHigh:
        case N_InsertReference:
        case N_InsertSpace:
        case N_InsertCharacter:
        case N_DeleteCharacterRight:
        case N_DeleteWord:
        case N_DeleteToEndOfSlot:
        case N_ClearMarkedBlock:
        case N_SwapCase:
        case N_Paste:
            return(TRUE);

        default:
            return(FALSE);
        }
}


/*
*/

static BOOL
edit_with_c(intl c, DIALOG *dptr, intl boxnumber,
            coord fieldx, coord fieldy, intl *idx)
{
    intl len;

    retval = 0;

    if(c < 0)
        {
        int key = -c;

        switch(key)
            {
            case N_Escape:
                #if MS || ARTHUR
                ack_esc();
                #endif
                retval = -1;
                return(FALSE);


            case N_CursorLeft:
                /* if it's a special field rotate it */

                if( (dptr->type == F_SPECIAL)  ||
                    ((dptr->type == F_COMPOSITE)  &&  on_composite_one))
                    {
                    char *offset = strchr(dptr->optionlist, dptr->option);

                    if(--offset < dptr->optionlist)
                        offset = (char *) dptr->optionlist + strlen(dptr->optionlist) - 1;

                    dptr->option = *offset;

                    if( (boxnumber == D_POPTIONS)  ||
                        (boxnumber == D_OPTIONS))
                        filealtered(TRUE);

                    break;
                    }

                /* if it's a colour or a number rotate it */

                switch(dptr->type)
                    {
                    case F_ARRAY:
                        if(dptr->option > 0)
                            dptr->option--;
                        else
                            while(*((char **) dptr->optionlist + (dptr->option + 1)) != NULL)
                                dptr->option++;

                        return(TRUE);

                    case F_NUMBER:
                    case F_LIST:
                    case F_COLOUR:
                    case F_CHAR:
                        if(num_in_linbuf)
                        break;

                        dptr->option--;

                        if( (boxnumber == D_POPTIONS) ||
                            (boxnumber == D_OPTIONS))
                            filealtered(TRUE);

                        return(TRUE);

                    default:
                        break;
                    }


                /* if it's at the start of a composite text field fall
                 * through to get to the special field, otherwise go left
                */

                if((dptr->type != F_COMPOSITE)  ||  (lecpos != 0))
                    {
                    CursorLeft_fn();
                    break;
                    }

                /* deliberate fall through */


            case N_CursorUp:
            case N_CursorDown:
            case N_Return:
                lecpos = 0;

                setcolour(MENU_FORE, MENU_BACK);
                dsp_dialog_field(fieldx, fieldy, dptr);

                on_composite_one = TRUE;
                slot_in_buffer = FALSE;

                if(buffer_altered)
                    {
                    buffer_altered = FALSE;

                    /* page layout or global option ? */
                    if(dptr->ch1)
                        filealtered(TRUE);

                    switch(dptr->type)
                        {
                        intl num;

                        case F_TEXT:
                        case F_COMPOSITE:
                            if(!str_set(&dptr->textfield, linbuf))
                                return(FALSE);
                            break;

                        case F_CHAR:
                        case F_COLOUR:
                        case F_NUMBER:
                        case F_LIST:
                            num = atoi((char *) linbuf);
                            dptr->option = (uchar) num;
                            if((intl) dptr->option != num)
                                {
                                retval = ERR_BAD_PARM;
                                return(FALSE);
                                }

                            setcolour(MENU_FORE, MENU_BACK);
                            dsp_dialog_field(fieldx, fieldy, dptr);
                            break;

                        default:
                            break;
                        }
                    }

                if(key == N_Return)
                    return(FALSE);

                if(key == N_CursorDown)
                    (*idx)++;
                elif(key == N_CursorUp)
                    (*idx)--;

                clearkeyboardbuffer();
                break;


            case N_CursorRight:
                /* if on composite special field, move to the text
                 * field, rotating the special if needs be
                */
                if((dptr->type == F_COMPOSITE)  &&  on_composite_one)
                    {
                    char *ptr = (char *) dptr->optionlist;

                    while(*++ptr)
                        ;

                    dptr->option = *--ptr;
                    on_composite_one = FALSE;

                    break;
                    }

                /* if it's a colour rotate it */

                switch(dptr->type)
                    {
                    case F_ARRAY:
                        if(*((char **) dptr->optionlist + (++(dptr->option))) == NULL)
                            dptr->option = 0;
                        return(TRUE);

                    case F_COLOUR:
                    case F_CHAR:
                    case F_NUMBER:
                    case F_LIST:
                        if(num_in_linbuf)
                            break;

                        dptr->option++;
                        clearkeyboardbuffer();

                        if( (boxnumber == D_POPTIONS) ||
                            (boxnumber == D_OPTIONS))
                            filealtered(TRUE);

                        return(TRUE);

                    default:
                        break;
                    }

                if(dptr->type == F_SPECIAL)
                    {
                    char *offset = strchr(dptr->optionlist, dptr->option);

                    if(!*++offset)
                        offset = (char *) dptr->optionlist;

                    dptr->option = *offset;

                    if( (boxnumber == D_POPTIONS)  ||
                        (boxnumber == D_OPTIONS))
                        filealtered(TRUE);

                    clearkeyboardbuffer();
                    break;
                    }

                do_command(key, FALSE);
                break;


            default:
                if(((dptr->type == F_COMPOSITE)  &&  on_composite_one)
                    &&  !allowed_in_dialog(key))
                    {
                    bleep();
                    clearkeyboardbuffer();
                    }
                else
                    do_command(key, FALSE);

                break;
            }

        len = strlen((char *) linbuf);

        if(buffer_altered  &&  lecpos > len)    /* is this the funny??? */
            lecpos = len;

        if(key == N_Escape)
            return(FALSE);
        }
    elif(c > 0)
        {
        BOOL temp_iovbit = xf_iovbit;
        BOOL already_taken = FALSE;

        switch(dptr->type)
            {
            case F_SPECIAL:
            case F_COMPOSITE:
                    already_taken = TRUE;
                    if(on_composite_one)
                        {
                        c = toupper(c);
                        if(strchr(dptr->optionlist, (int) c))
                            {
                            if(dptr->ch1 != '\0')
                                filealtered(TRUE);

                            dptr->option = (uchar) c;
                            }
                        else
                            {
                            bleep();
                            clearkeyboardbuffer();
                            }
                            break;
                        }

                    /* deliberate fall-through */

            case F_COLOUR:
            case F_CHAR:
            case F_NUMBER:
            case F_LIST:
                    if(!already_taken) /* cos these fell thru */
                        {
                        if(isdigit(c))
                            {
                            if(num_in_linbuf)
                                ;   /* fall through to text field */
                            else
                                {
                                buffer_altered = num_in_linbuf = TRUE;
                                *linbuf = (uchar) c;
                                linbuf[(lecpos = 1)] = '\0';
                                break;
                                }
                            }
                        else
                            {
                            bleep();
                            break;
                            }
                        }

                    /* deliberate fall through */


            case F_TEXT:
                    {
                    /* leading and trailing places have field lengths
                     * restricted to 4 characters.  They happen to be
                     * only dialog entries with ch2 = 'P'
                    */
                    intl maxfieldlength = (dptr->ch2 == 'P') ? 4 : MAXFLD;

                    if( (lecpos >= maxfieldlength)  ||
                        (xf_iovbit  &&  (strlen((char *) linbuf) >= maxfieldlength)))
                        {
                        bleep();
/* been_error = TRUE; */
                        clearkeyboardbuffer();
                        }
                    else
                        chrina((uchar) c);
                    break;
                    }

            default:
                    bleep();
                    break;
            }

        xf_iovbit = temp_iovbit;
        }

    return(TRUE);
}

#endif  /* MS || ARTHUR */


/****************************
*                           *
*  read a single parameter  *
*                           *
****************************/

static BOOL
read_parm(uchar *array)
{
    for(;;)
        {
        switch(*exec_ptr)
            {
            case '\0':
                return(FALSE);

            case CR:
            case TAB:
                *array = '\0';
                return(TRUE);

            default:
                *array++ = *exec_ptr++;
                break;
            }
        }
}


/************************************************************************
*                                                                       *
* when execing a file read parameters from \ commands into dialog box   *
* eg \GS |I a1|m                                                        *
* exec_ptr has the first parameter, leave exec_ptr pointing at |M       *
*                                                                       *
************************************************************************/

static void
extract_parameters(DIALOG *dptr, intl items)
{
    intl count = 0;
    char array[LIN_BUFSIZ+1];
    uchar *ptr;
    intl c;

    tracef0("[extract parameters in]\n");

    /* exec_ptr points at TAB to start */
    for(; *exec_ptr != CR; count++, dptr++)
        {
        if(count >= items)
            {
            strcpy(array, (char *) exec_ptr);
            goto ERRORPOINT;
            }

        /* read the parameter */
        exec_ptr++;

        if(!read_parm(array))
            goto ERRORPOINT;

        tracef1("[read_parm read: %s]\n", trace_string(array));

        switch(dptr->type)
            {
            case F_COLOUR:
            case F_CHAR:
            case F_NUMBER:
                dptr->option = (uchar) atoi((char *) array);
                break;


            case F_COMPOSITE:
            case F_SPECIAL:
                ptr = array;
                while(*ptr++ == SPACE)
                    ;

                c = *--ptr;
                if(!c  ||  !strchr(dptr->optionlist, c))
                    goto ERRORPOINT;

                dptr->option = (uchar) c;
                if(dptr->type == F_SPECIAL)
                    break;

                /* this is composite */
                if(*exec_ptr == CR)
                    return;

                /* so get the textfield */
                exec_ptr++;
                read_parm(array);

                /* deliberate fall thru */


            case F_TEXT:
                if(!str_set(&dptr->textfield, array))
                    return;
                break;


            case F_LIST:
                dptr->option = (uchar) atoi((char *) array);
                break;


            case F_ARRAY:
                #if 1
                {
                BOOL found = FALSE;
                char **list = (char **) dptr->optionlist;

                tracef1("[F_ARRAY searching for: %s]\n", trace_string(array));

                /* MRJC fix added here 16.8.89 to stop
                 * this loop zooming off end of array
                */
                for(c = 0; *(list + c); c++)
                    {
                    tracef1("[F_ARRAY comparing with: %s]\n", trace_string(*(list + c)));
                    if(!stricmp(array, *(list + c)))
                        {
                        dptr->option = c;
                        found = TRUE;
                        break;
                        }
                    }

                if(!found)
                    {
                    tracef0("[F_ARRAY key not found]\n");
                    goto ERRORPOINT;                        
                    }
                }

                #else

                if(dptr != d_def_fkey)
                    goto ERRORPOINT;

                /* function key definition - get string */
                ptr = array;
                while(*ptr++ == SPACE)
                    ;
                ptr--;

                /* of form F1, SF1, CF1, ZF1 or AF1 */

                c = 0;
                switch(toupper(*ptr))
                    {
#if MS
#   define KEYS_DEFINABLE   10
#elif ARTHUR || RISCOS
#   define KEYS_DEFINABLE   12
#endif

                    #if MS || ARTHUR
                    /* Alt Fn */
                    case 'A':
                        c += KEYS_DEFINABLE;
                    #endif

                    #if ARTHUR || RISCOS
                    /* Ctrl-Shift Fn */
                    case 'Z':
                        c += KEYS_DEFINABLE;
                    #endif

                    case 'C':
                        c += KEYS_DEFINABLE;

                    case 'S':
                        c += KEYS_DEFINABLE;
                        ptr++;
                        if(toupper(*ptr) != 'F')
                            goto ERRORPOINT;

                        /* drop through */

                    case 'F':
                        {
                        intl funcnum = atoi((char *) ++ptr);

                        if((funcnum < 1)  ||  (funcnum > KEYS_DEFINABLE))
                            goto ERRORPOINT;

                        dptr->option = (uchar) (funcnum + c - 1);
                        }
                        break;

                    default:
                        goto ERRORPOINT;
                    }
                #endif
                break;

            default:
                break;
            }
        }

    tracef0("[extract parameters out]\n");

    return;


ERRORPOINT:
    reperr(ERR_BAD_PARM, array);
}


/************************************************
*                                               *
* if in an exec file, fillin fields from file   *
* else draw a dialog box and let the user       *
* user fiddle around with the fields            *
* always ensures current buffer to memory and   *
* exits with the buffer refilled but unmodified *
*                                               *
* --out--                                       *
*   TRUE if dialog box filled in ok             *
*                                               *
************************************************/

extern BOOL
dialog_box(intl boxnumber)
{
    DHEADER *dhptr  = dialog_head + boxnumber;
    DIALOG *dptr    = dhptr->dialog_box;
    BOOL res;

    tracef1("dialog_box(%d)\n", boxnumber);

    #if RISCOS
    /* RISCOS doesn't use linbuf to process dialog: mergebuf just to set filealtered */
    if(!mergebuf_nocheck())
        return(FALSE);
    filbuf();
    #else

    retval = 0; /* needed for when reperr called with this routine */

    /* can't abuse linbuf if in expression */
    if(xf_inexpression  &&  (boxnumber != D_ERROR))
        return(reperr_null(ERR_EDITINGEXP));

    /* linbuf used in dialog box handling */
    if(mergebuf_nocheck())
        return(FALSE);

    #endif  /* RISCOS */


    update_dialog_from_windvars(boxnumber);

    exec_filled_dialog = FALSE;

    /* if execing a file don't display a dialog box,
     * just read the parameters until a CR is found
     * unless the dialog box demands to be
     * displayed and have user interaction.
    */
    if((dhptr->flags & EXEC_CAN_FILL) &&  (in_execfile  ||  command_expansion))
        {
        if(exec_ptr)
            {
            /* exec_ptr points at TAB parameter...
             * read the parameters and leave exec_ptr on |M
            */
            extract_parameters(dptr, dhptr->items);
            exec_filled_dialog = TRUE;
            }

        res = !been_error;
        }
    else

#if RISCOS

    {
    res = (dhptr->dproc)
                ? riscdialog_execute(dhptr->dproc, dhptr->dname, dptr, boxnumber)
                : reperr_not_installed(ERR_GENFAIL);

    if((dhptr->flags & EXEC_CAN_FILL)  &&  macro_recorder_on)
        out_comm_parms_to_macro_file(dhptr->dialog_box, dhptr->items, res);
    }

    update_windvars_from_dialog(boxnumber);

    return(res);    

#elif MS || ARTHUR

    {
    coord xpos, ypos, xsize, ysize, xpos_plus1;
    intl templecpos = lecpos;
    intl idx = 0;
    intl previous_field = -1;
    coord fieldx, fieldy;
    BOOL tsqobit;

    tsqobit = sqobit;       /* don't send to printer */
    sqobit  = FALSE;

    in_dialog_box = TRUE;

    bodge_funnies_for_master_font((uchar *) POUND_STR);
    bodge_funnies_for_master_font((uchar *) dialog_help_table[HELP_ARROWS]);

    wrch_definefunny(LEFT_ARROW);
    wrch_definefunny(RIGHT_ARROW);

    init_screen_array();

    xsize      = 4 + dhptr->tag_len + TEXTWIDTH;
    ysize      = 3 + dhptr->items;
    xpos       = (pagwid - xsize) / 2;
    ypos       = (paghyt - ysize) / 2 + 1;
    xpos_plus1 = xpos + 1;
    fieldx     = xpos_plus1 + dhptr->tag_len;

    save_screen(xpos, ypos, xsize, ysize);

    setcolour(MENU_HOT, MENU_BACK);
    my_rectangle(xpos, ypos, xsize, ysize);

    /* put header in top left of rectangle line */

    if(!str_isblank(dhptr->name))
        {
        at(xpos_plus1, ypos);
        wrch_funny(DROP_RIGHT);

        setcolour(MENU_FORE, MENU_BACK);
        stringout(dhptr->name);
        setcolour(MENU_HOT, MENU_BACK);

        wrch_funny(DROP_LEFT);
        }

    /* blank out first line */

    at(xpos_plus1, ypos+1);
    ospca(xsize-2);

    /* restore any screen that needs it */

    #if MS
        if(!fastdraw)
            clip_menu(xpos, ypos, xsize, ysize);
    #endif

    num_in_linbuf = FALSE;

    /* display all the items */

    setcolour(MENU_FORE, MENU_BACK);
    for(idx = 0; idx < dhptr->items; dptr++, idx++)
        {
        if( ((dptr->type == F_SPECIAL)  ||  (dptr->type == F_COMPOSITE))
                &&  (dptr->option == NO_OPT))
            dptr->option = *dptr->optionlist;

        fieldy = ypos + 2 + idx;

        dsp_dialog_text(dhptr, dptr, xpos_plus1, fieldy, xsize-2);

        if(dptr->type != F_SPECIAL)
            {
            strcpy((char *) linbuf, dptr->textfield
                                        ? dptr->textfield
                                        : NULLSTR);
            lecpos = 0;
            }

        dsp_dialog_field(fieldx, fieldy, dptr);
        }

    /* do the editing */

    on_composite_one = rscbit = TRUE;

    idx = 0;

    for(;;)
        {
        intl c;
        coord len = 0;

        if(idx < 0)
            idx = dhptr->items - 1;
        elif(idx >= dhptr->items)
            idx = 0;

        dptr = dhptr->dialog_box + idx;
        fieldy = ypos + 2 + idx;

        /* load linbuf for new field */
        if(!slot_in_buffer  &&  dptr->type != F_SPECIAL)
            {
            strcpy((char *) linbuf, dptr->textfield
                                        ? dptr->textfield
                                        : NULLSTR);
            lecpos = 0;
            slot_in_buffer = TRUE;
            }

        /* display the help line in the top of the box, perhaps */

        if(idx != previous_field)
            {
            num_in_linbuf = FALSE;
            previous_field = idx;

            at(fieldx-2, ypos);

            if(dptr->helptype != NO_HELP)
                {
                setcolour(MENU_FORE, MENU_BACK);
                wrch(SPACE);
                len = 2 + stringout((uchar *) dialog_help_table[dptr->helptype]);
                wrch(SPACE);
                }

            setcolour(MENU_HOT, MENU_BACK);
            wrch_definefunny(HORIZBAR);
            wrchrep(HORIZBAR, TEXTWIDTH + 4 - len);
            }


        /* display the field */

        if( ((dptr->type == F_COMPOSITE) && !on_composite_one) ||
            (dptr->type == F_TEXT) )
            setcolour(MENU_FORE, MENU_BACK);
        else
            setcolour(MENU_BACK, MENU_FORE);

        dsp_dialog_field(fieldx, fieldy, dptr);

        sb_show_if_fastdraw();


        /* if it's an error just get keypress and return */

        if(dptr->type == F_ERRORTYPE)
            {
            slot_in_buffer = buffer_altered = FALSE;
            if(dptr == d_error)
                bleep();

            ack_esc();
            clearkeyboardbuffer();
            rdch(FALSE, TRUE);
            break;
            }

        /* only flash cursor in text field, or text part of composite */

        c = inpchr(num_in_linbuf  ||  (dptr->type == F_TEXT)  ||
                    (!on_composite_one  &&  (dptr->type == F_COMPOSITE)));

        if(!edit_with_c(c, dptr, boxnumber, fieldx, fieldy, &idx) || been_error)
            break;
        }

    /* clean up */

    in_dialog_box = FALSE;  /* must be before clip_menu to get colours right */

    clip_menu(0, 0, 0, 0);  /* restore screen */

    sb_show_if_fastdraw();

    lecpos = templecpos;
    slot_in_buffer = buffer_altered = rscbit = FALSE;
    xf_drawmenuheadline = TRUE;
    sqobit = tsqobit;

    if(macro_recorder_on)
        out_comm_parms_to_macro_file(dhptr->dialog_box, dhptr->items,
                                    !retval && !been_error);
    }

    update_windvars_from_dialog(boxnumber);

    filbuf();

    if(retval > 0)
        return(reperr_null(retval));
        /* NB. retval gets zeroed by reperr */

    return(!retval  &&  !been_error);

#endif  /* RISCOS */
}


#if RISCOS

/************************************************
*                                               *
* explicitly kill off any persisting dialog box *
*                                               *
************************************************/

extern void
dialog_box_end(void)
{
    tracef0("dialog_box_end() --- explicit termination\n");
    riscdialog_dispose();
}


extern BOOL
dialog_box_ended(void)
{
    BOOL res = exec_filled_dialog  ||  riscdialog_ended();
    tracef1("dialog_box_ended() returns %s\n", trace_boolstring(res));
    if(!res)
        draw_screen();
    return(res);
}

#endif  /* RISCOS */

/* end of dialog.c */
