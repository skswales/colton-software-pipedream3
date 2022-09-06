/* datatype.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       datatype.h - PipeDream datatypes
 * Author:      RJM 8-Apr-1989
*/

#ifndef DATATYPE_H_SET
#define DATATYPE_H_SET

#ifndef BOOL
#define BOOL intl
#endif


#if ARTHUR || RISCOS
typedef intl dochandle;
typedef uchar BOOLEAN;          /* packed flag */
#elif MS
typedef intl dochandle;
typedef uchar BOOLEAN;
#endif


/* handle of a graphics DDE link */
typedef intl ghandle;


/* ----------------------------------------------------------------------- */

typedef time_t DATE;

/* time */

#if ARTHUR || RISCOS
#define YEAR_OFFSET 80
#elif MS
#define YEAR_OFFSET 36
#endif


/* ----------------------------------------------------------------------- */

typedef int coord;      /* coordinate type for screen, MUST BE SIGNED */
#if RISCOS
typedef int gcoord;     /* graphics coordinate type */
#endif


/********************************************************************
*                                                                   *
* Column datatypes                                                  *
*                                                                   *
*   Columns are intl, 2 bytes on PC, 16k columns, 4 bytes on ARM    *
*   Rows 4 bytes. On PC rows could be limited to 2 bytes?           *
*   This would restrict us to 16k rows.                             *
*   Probably enough for your typical portfolio.                     *
*                                                                   *
********************************************************************/

#if ARTHUR || RISCOS
/*
 * 29 bit quantities, 30th bit for absolute, 31th for bad
 * -ve used for falling out of loops
*/

typedef int colt;               /* column numbers are single bytes */

#define NO_COL                  ((colt) ((1L << 30)-1))
#define LARGEST_COL_POSSIBLE    ((colt) ((1L << 29)-1))
#define COLNOBITS               LARGEST_COL_POSSIBLE
#define ABSCOLBIT               ((colt) (1L << 29))
#define BADCOLBIT               ((colt) (1L << 30))
#define ABSEVALCOLBIT           ((colt) (1L << 31))

#elif MS
/*
 * 13 bit quantities, 14th bit for absolute, 15th for bad
 * -ve used for falling out of loops
*/

typedef int colt;

#define NO_COL                  ((colt) ((1L << 14)-1))
#define LARGEST_COL_POSSIBLE    ((colt) ((1L << 13)-1))
#define COLNOBITS               LARGEST_COL_POSSIBLE
#define ABSCOLBIT               ((colt) (1L << 13))
#define BADCOLBIT               ((colt) (1L << 14))
#define ABSEVALCOLBIT           ((colt) (1L << 15))

#endif

#ifdef SHORT_ROWT
typedef int rowt;           /* row numbers are most efficient */

#define NO_ROW                  ((rowt) (NO_COL))
#define LARGEST_ROW_POSSIBLE    ((rowt) (LARGEST_COL_POSSIBLE))
#define ROWNOBITS               LARGEST_ROW_POSSIBLE
#define ABSROWBIT               ((rowt) (ABSCOLBIT))
#define BADROWBIT               ((rowt) (BADCOLBIT))
#define ABSEVALROWBIT           ((rowt) (ABSEVALCOLBIT))
#else

typedef long int rowt;          /* row numbers are longs */

#define NO_ROW                  ((rowt) ((1L << 30) -1)
#define LARGEST_ROW_POSSIBLE    ((rowt) ((1L << 29) -1))
#define ROWNOBITS               LARGEST_ROW_POSSIBLE
#define ABSROWBIT               ((rowt) (1L << 29))
#define BADROWBIT               ((rowt) (1L << 30))
#define ABSEVALROWBIT           ((rowt) (1L << 31))
#endif


/****************************************************************************
* other constants
****************************************************************************/

#define CMD_BUFSIZ  ((intl) 255)        /* size of buffers */
#define ERR_BUFSIZ  ((intl) 255)
#define LIN_BUFSIZ  ((intl) 255)
#define MAXFLD      (LIN_BUFSIZ-1)

/* On RISC OS the worst case is MAXFLD ctrlchars being expanded to
 * four characters each plus an overhead for an initial font change and NUL
*/
#define PAINT_STRSIZ    (LIN_BUFSIZ * 4)

/* Output buffer for expression compilation: worst case is MAXFLD full of
 * small SLR, op such as '+A1' (3b) -> id,SLR,op (11b) plus last OPR_END
*/
#define COMPILE_BUFSIZ  ((LIN_BUFSIZ * 11) / 3)


/* ----------------------------------------------------------------------- */

/* structures for expression analyser */

/*
 * strings are stored as a pointer and a length, NO delimiter
*/

#define STRING struct string_
STRING
    {
    uchar *ptr;
    intl length;
    };


/* document number */
typedef uchar docno;

typedef struct
{
    colt col;
    rowt row;
    docno doc;
}
SLR;
#define SLRSIZE (1 + sizeof(docno) + sizeof(colt) + sizeof(rowt))


typedef struct
{
    SLR first;
    SLR second;
}
RANGE;


typedef struct
{
    colt stt;
    colt end;
}
COLRANGE;


/*
 * each term is stored as a SYMB_TYPE struct.
 * .type specifies what the object is
 * .value.symb is an operator, or a nerror number
 * .value.str is a string. The string is stored as pointer and length
 * .value.slot is a slot reference
 * .value.date is a date
 * .value.num  is a numeric constant coerced to double
*/

#define SYMB_TYPE struct symb_type
SYMB_TYPE
    {
    uchar type;

    union
        {
        intl symb;
        STRING str;
        SLR slot;
        RANGE range;
        time_t date;
        double num;
        long intg;
        } value;
    };


/* ----------------------------- lists.c --------------------------------- */

#if ARTHUR || RISCOS
#include "ext.lists"
#elif MS
#include "lists.ext"
#endif



/**************************************************************************
* slot.c
**************************************************************************/

typedef struct slot *slotp;                 /* slot pointer type */

/*
column entry for use in sparse matrix
*/

struct colentry
{
    list_block lb;
    intl wrapwidth;
    intl colwidth;
};

typedef struct colentry *colp;


/* object on deleted words list describing a block */

typedef struct
{
    colp    del_colstart;
    colt    del_col_size;
    rowt    del_row_size;
}
saved_block_descriptor;


/* ------------------------------ commlin.c ------------------------------ */

#define short_menus() (d_menu[0].option)

#define NOMENU      0
#define SHORT       1
#define LONG        2
#define TICKABLE    4
#define TICK_STATUS 8
#define GREYABLE    16
#define GREY_STATUS 32
#define HAS_DIALOG  64

#if defined(DEMO)
#define GREYDEMO    (GREYABLE | GREY_STATUS)
#else
#define GREYDEMO    0
#endif


#define MENU struct _menu
MENU
    {
    const char *title;
    const char *command;
    intl key;
    uchar func_key; /* 1..16 == F1 to F10,cursors; 17..32 == SHIFT; 33..48 == CTRL */
    uchar flags;
    ints funcnum;   /* better packing */
    void (*cmdfunc)(void);
    };


#define MENU_HEAD struct _menu_head
MENU_HEAD
    {
    const char *name;
    MENU *tail;
    char installed;
    char items;
    char titlelen;
    char commandlen;
    #if MS || ARTHUR
    char keylen;
    BOOLEAN beenhere;
    #endif
    #if RISCOS
    void *m; /* abuse: RISC OS submenu^ kept here */
    #endif
    };


/* ----------------------------- scdraw.c -------------------------------- */

/* horizontal and vertical screen tables */

#define SCRROW struct _scrrow
SCRROW
    {
    rowt rowno;
    intl page;
    #if !RISCOS
    coord length;
    #endif
    uchar flags;
    };


#define SCRCOL struct _scrcol
SCRCOL
    {
    colt colno;
    uchar flags;
    };


/* ------------------------------ slector.c ------------------------------ */

#if ARTHUR || RISCOS
typedef struct find_t
    {
    uchar *ptr;
    uchar name[LIN_BUFSIZ];
    } find_t;
#elif MS
/* An analogous type is defined by DOS */
#endif


/* ------------------------------ help.c --------------------------------- */

#define SCREENITEM struct _screenitem
SCREENITEM
    {
    coord offset;
    char *text;
    };
#define SCREENHEADER struct _screenheader

SCREENHEADER
    {
    const SCREENITEM *screen;
    char *heading;
    };


/* ------------------------------ dialog.c ------------------------------- */

#if RISCOS
/* Saves acres of superfluous code masking */
typedef intl    optiontype;
#else
typedef uchar   optiontype;
#endif

#define DIALOG struct _dialog_entry
DIALOG
    {
#if MS || ARTHUR
    const char *tag;    /* screen description of field */
    uchar helptype;     /* extra info for this line */
#endif
    uchar type;         /* type of field, text, number, special */
    uchar ch1;          /* first character of save option string */
    uchar ch2;          /* second character of save option string */
    optiontype option;      /* single character option eg Y, sometimes int index */
    const char *optionlist; /* range of possible values for option, first is default */
    char *textfield;    /* user specified name of something */
#if defined(MANY_DOCUMENTS)
    int offset;         /* of corresponding variable in windvars */
#endif
    };

/* dialog box entry checking: n is maximum used in d_D[n].whatever expr. */
#define assert_dialog(n, D) assert((n+1) == dialog_head[D].items)


#if RISCOS
typedef void (*dialog_proc)(DIALOG *dptr);
#endif


#define DHEADER struct _dheader
DHEADER
    {
    DIALOG *dialog_box;
    intl items;
    #if RISCOS
    intl flags;
    dialog_proc dproc;
    const char *dname;
    #else
    const char *name;
    coord tag_len;
    #endif
    };


/* ------------------------------ mcdiff.c ------------------------------- */

#define SCRLIN struct _scrlin
SCRLIN
    {
    uchar ch[80];
    uchar att[80];
    };


/* --------------------------- cursmov.c --------------------------------- */

#define SAVE_DEPTH 5
#define SAVPOS struct _savpos
SAVPOS
    {
    intl file;
    SLR ref;
    };


/* --------------------------- pdriver.c -------------------------------- */

#define DRIVER struct _driver
DRIVER
    {
    BOOL (*out)(intl ch);
    BOOL (*on)(void);
    BOOL (*off)(BOOL ok);
    };


/* --------------------------- numbers.c -------------------------------- */

/*
structure of an entry in the draw file list
*/

struct draw_file_entry
    {
    char *name;
    intl error;
    intl flag;
    mhandle memoryh;
    struct
        {
        char *data;
        int length;
        } diag;
    BOOL processed;             /* flag needed for recache when two files have same leafname */
    };

typedef struct draw_file_entry *drawfep;


/*
structure of reference to a draw file
*/

struct draw_file_ref
    {
    word32 draw_file_key;
    dochandle han;
    colt col;
    rowt row;
    double xfactor;
    double yfactor;
    intl xsize_os;
    intl ysize_os;
    };

typedef struct draw_file_ref *drawfrp;


/*
 * structure of an entry in the graphics link list
 * ghandle of entry used as key
*/

struct graphics_link_entry
{
    dochandle han;          /* where the block is */
    colt col;
    rowt row;

    intl task;              /* task id of client */
    BOOL update;            /* does client want updates? */
    BOOL datasent;          /* data sent without end marker */

    ghandle ghan;
    intl xsize;
    intl ysize;
    char text[1];           /* leafname & tag, 0-terminated */
};

typedef struct graphics_link_entry *graphlinkp;


/* --------------------------- windvars.c -------------------------------- */


/* --------------------------- riscos.c ---------------------------------- */

#if RISCOS

/* Abstract objects for export */
typedef intl   riscos_window;
typedef struct riscos__eventstr  *riscos_eventstr;
typedef struct riscos__redrawstr *riscos_redrawstr;

typedef struct
{
    unsigned int exec; /* order important! */
    unsigned int load;
    unsigned int length;
}
riscos_fileinfo;

typedef void (*riscos_redrawproc)(riscos_redrawstr *redrawstr);

typedef void (*riscos_printproc)(void);


typedef struct
{
    intl x0, y0, x1, y1;
}
coord_box;

#endif  /* RISCOS */


/* ----------------------------------------------------------------------- */

#if ARTHUR || RISCOS
#define BAD_POINTER ((void *) 0x90000000)
#elif MS
#define BAD_POINTER ((void *) 0x900000)
#endif


/* move to datafmt sometime */
typedef enum
{
#if ARTHUR || RISCOS
#if RISCOS
    driver_riscos,
#endif
    driver_parallel,
    driver_serial,
    driver_network,
    driver_user
#else
    driver_lpt1,
    driver_lpt2,
    driver_lpt3,
    driver_com1,
    driver_com2
#endif
}
driver_types;


/* set of colours used by PipeDream */

typedef enum
{
    FORE,
    BACK,
    BORDERC,

    #if MS
    MENU_FORE,
    MENU_BACK,
    MENU_HOT,
    #endif

    NEGATIVEC,
    PROTECTC,

    #if RISCOS
    CARETC,
    #endif

    CURBORDERC,

    N_COLOURS
}
d_colour_offsets;


/* the following are entries in the print dialog box */

typedef enum
{
    P_PSF,
    P_PARMFILE,
    P_OMITBLANK,
    P_COLS,
    P_ROWS,
    P_BLOCK,
    P_TWOSIDE,
    P_COPIES,
    P_WAIT,
    #if RISCOS
    P_ORIENT,
    P_SCALE,
    #endif
    P_THE_LAST_ONE
}
d_print_offsets;


/* entries in the sort block dialog box */

#define SORT_FIELD_DEPTH        5           /* number of col,ascend pairs */
#define SORT_FIELD_COLUMN       0
#define SORT_FIELD_ASCENDING    1
#define SORT_UPDATE_REFS        (SORT_FIELD_COLUMN + (SORT_FIELD_DEPTH * 2))
#define SORT_MULTI_ROW          (SORT_UPDATE_REFS + 1)

#endif  /* DATATYPE_H_SET */

/* end of datatype.h */
