/* windvars.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       windvars.c - variables pertaining to each PipeDream window
 * Author:      RJM 8-Apr-1989
*/

#if defined(TRACE_DOC)
#define TRACE 1
#endif


/* standard header files */
#include "flags.h"


/* header file */
#include "datafmt.h"


#if RISCOS
#include "werr.h"
#include "misc.h"

#include "ext.riscos"
#endif


#define DONT_CARE 0


#if !defined(MANY_DOCUMENTS)

dochandle DocHandle = 1;

/* ------------------------- savload.c ----------------------------------- */

intl curfil = DONT_CARE;    /* current file number in multi-file document */

char *currentfilename  = NULL;
char *currentdirectory = NULL;
#if RISCOS
riscos_fileinfo currentfileinfo;
#endif

list_block *first_file = NULL;  /* first file in multi-file document */

BOOLEAN glbbit = FALSE;     /* global file */
BOOLEAN xf_filealtered = FALSE;


/* ------------------------- bufedit.c ----------------------------------- */

BOOLEAN start_block = FALSE;

SLR in_block;                   /* current position in block */
SLR start_bl;
SLR end_bl;


/* -------------------------- constr.c ----------------------------------- */

uchar linbuf[LIN_BUFSIZ];       /* buffer for line editing */
intl lecpos = 0;                /* cursor pos in linbuf */

rowt currow = (rowt) 0;         /* current row number */
colt curcol = (colt) 0;         /* current col number */

colt numcol = 0;                /* total number of columns */
rowt numrow = 0;                /* maximum number of rows */


/* ---------------------------- slot.c ----------------------------------- */

colp colstart = NULL;
colt colsintable = 0;


/* --------------------------- mcdiff.c ---------------------------------- */

coord paghyt = 0;           /* window height: number of lines on screen-1,  ie 24 for mode 3 */
coord pagwid = 0;           /* window width:  number of char spaces-1.      ie 78 for mode 3 */
coord pagwid_plus1 = 1;     /* window width:  number of char spaces.        ie 79 for mode 3 */


/* ---------------------------- commlin.c -------------------------------- */


/* ---------------------------- scdraw.c --------------------------------- */

mhandle vertvec_mh = 0;
mhandle horzvec_mh = 0;

coord maxncol = 0;              /* maximum number of columns on screen */
coord maxnrow = 0;              /* maximum number of rows on screen */

BOOLEAN out_screen = FALSE;
BOOLEAN out_rebuildhorz = FALSE;
BOOLEAN out_rebuildvert = FALSE;
BOOLEAN out_forcevertcentre = FALSE;
BOOLEAN out_rowout = FALSE;
BOOLEAN out_rowout1 = FALSE;
BOOLEAN out_currslot = FALSE;
BOOLEAN out_below = FALSE;

coord rowout = 0;
coord rowout1 = 0;

BOOLEAN xf_inexpression         = FALSE;

BOOLEAN xf_draweverything       = FALSE;    /* for * commands etc. */
BOOLEAN xf_frontmainwindow      = FALSE;
BOOLEAN xf_drawcolumnheadings   = TRUE;
BOOLEAN xf_drawsome             = FALSE;    /* some slots need redrawing */
BOOLEAN xf_drawslotcoordinates  = FALSE;

coord maximum_cols = 0;
coord maximum_rows = 0;

coord rows_available = 0;       /* to draw slots in. ie screen excluding borders */
coord cols_available = 0;
coord n_rowfixes = 0;           /* number of rows currently fixed on screen */
intl movement = 0;
intl lescrl = 0;                /* offset of lecpos on screen */
coord curcoloffset = 0;         /* current column in horzvec */
coord currowoffset = 0;         /* current row in vertvec */
colt oldcol = 0;
colt newcol = 0;
rowt oldrow = 0;
rowt newrow = 0;
coord last_thing_drawn = 0;     /* how dirty is current line? */

/* these change when status of borbit changes */
coord borderheight = BORDERHEIGHT;
coord borderwidth  = BORDERWIDTH;

intl curpnm = 0;

coord error_line_dirty = 0; /* length of string currently on error line */


#if RISCOS
/* last set of scroll offsets we opened a window at */
gcoord old_scx = DONT_CARE;
gcoord old_scy = DONT_CARE;
#endif


/* ----------------------------- cursmov.c ------------------------------- */

coord rowsonscreen = 0;
coord scrbrc = 0; /* offset of PARTIAL column in horzvec, or LAST if all fit */
coord rowtoend = 0;


/* ----------------------------- numbers.c ------------------------------- */

uchar refs_in_this_slot = 0;        /* set by cplent() for slot references */
/* 2.21 */
uchar refs_in_this_sheet = 0;
/* 2.21 */

colt edtslr_col = 0; /* live whilst in expression */
rowt edtslr_row = 0;

BOOLEAN slot_in_buffer = FALSE;
BOOLEAN output_buffer = FALSE;
BOOLEAN global_recalc = FALSE;
BOOLEAN recalc_bit = FALSE;
BOOLEAN buffer_altered = FALSE;


/* ---------------------------- doprint.c -------------------------------- */

BOOLEAN wrpbit = TRUE;      /* wrap on */
BOOLEAN jusbit = TRUE;      /* justify on */
BOOLEAN txnbit = TRUE;      /* text/numbers */
BOOLEAN rcobit = TRUE;      /* recalculate mode */
BOOLEAN minbit = TRUE;      /* minus/brackets */
BOOLEAN borbit = TRUE;      /* drawing borders are on the screen */
BOOLEAN iowbit = FALSE;     /* insert rows/columns on wrap */

intl real_pagelength = 0;
intl real_pagcnt = DONT_CARE;



/* ----------------------------- dialog.c -------------------------------- */

intl encpln = 0;                /* page length - margins */
intl enclns = 0;                /* line spacing */
intl pagoff = 0;                /* page offset at top of screen */
intl pagnum = 0;                /* page number at top of screen */
intl filpof = 0;                /* page offset at top of file */
intl filpnm = 0;                /* page number at top of file */


/* ----------------------------- riscos.c -------------------------------- */

#if RISCOS
riscos_window main_window   = NULL;
#endif


/* ----------------------------------------------------------------------- */


#else   /* MANY_DOCUMENTS */


/* exported array of document pointers as well as list */

window_data *document_array[DOCHANDLE_MAX];


/* the template data to copy into every new document's window vars */

static window_data initial_window_data =
    {
    NULL,                       /* window_data *link */
    DONT_CARE,                  /* dochandle DocHandle */


/* ---------------------------- savload.c -------------------------------- */

    NULL,                       /* char *currentfilename */
    NULL,                       /* char *currentdirectory */

    #if RISCOS
    { DONT_CARE, DONT_CARE, DONT_CARE },    /* riscos_fileinfo currentfileinfo */
    #endif

    NULL,                       /* list_block *first_file */

    DONT_CARE,                  /* intl curfil */

    FALSE,                      /* BOOLEAN glbbit */
    FALSE,                      /* BOOLEAN xf_filealtered */


/* ------------------------- markers.c ----------------------------------- */

    FALSE,                      /* BOOLEAN start_block */

    { DONT_CARE, DONT_CARE },   /* SLR in_block */
    { DONT_CARE, DONT_CARE },   /* SLR start_bl */
    { DONT_CARE, DONT_CARE },   /* SLR end_bl */


/* -------------------------- constr.c ----------------------------------- */

    "",                         /* uchar linbuf[LIN_BUFSIZ] */
    0,                          /* intl lecpos */

    0,                          /* colt curcol */
    0,                          /* rowt currow */
    0,                          /* colt numcol */
    0,                          /* rowt numrow */


/* ---------------------------- slot.c ----------------------------------- */

    NULL,                       /* colp colstart */
    0,                          /* colt colsintable */


/* --------------------------- mcdiff.c ---------------------------------- */

    0,                          /* coord paghyt */
    0,                          /* coord pagwid */
    1,                          /* coord pagwid_plus1 */


/* ---------------------------- scdraw.c --------------------------------- */

    0,                          /* mhandle horzvec_mh */
    0,                          /* mhandle vertvec_mh */

    0,                          /* coord maxncol */
    0,                          /* coord maxnrow */

    FALSE,                      /* BOOLEAN out_screen */
    FALSE,                      /* BOOLEAN out_rebuildhorz */
    FALSE,                      /* BOOLEAN out_rebuildvert */
    FALSE,                      /* BOOLEAN out_forcevertcentre */
    FALSE,                      /* BOOLEAN out_below */
    FALSE,                      /* BOOLEAN out_rowout */
    FALSE,                      /* BOOLEAN out_rowout1 */
    FALSE,                      /* BOOLEAN out_currslot */

    0,                          /* coord rowout */
    0,                          /* coord rowout1 */

    FALSE,                      /* BOOLEAN xf_inexpression */

    TRUE,                       /* BOOLEAN xf_draweverything */
    FALSE,                      /* BOOLEAN xf_frontmainwindow */
    FALSE,                      /* BOOLEAN xf_drawcolumnheadings */
    FALSE,                      /* BOOLEAN xf_drawsome */
    FALSE,                      /* BOOLEAN xf_drawslotcoordinates */

    0,                          /* coord maximum_cols */
    0,                          /* coord maximum_rows */
    0,                          /* coord cols_available */
    0,                          /* coord rows_available */
    0,                          /* coord n_rowfixes */
    0,                          /* coord curcoloffset */
    0,                          /* coord currowoffset */
    0,                          /* colt oldcol */
    0,                          /* colt newcol */
    0,                          /* rowt oldrow */
    0,                          /* rowt newrow */

    0,                          /* intl movement */
    0,                          /* intl lescrl */

    0,                          /* coord last_thing_drawn */

    BORDERHEIGHT,               /* coord borderheight */
    BORDERWIDTH,                /* coord borderwidth */

    0,                          /* intl curpnm */

    0,                          /* coord error_line_dirty */

    #if RISCOS
    DONT_CARE,                  /* gcoord old_scx */
    DONT_CARE,                  /* gcoord old_scy */
    #endif


/* ----------------------------- cursmov.c ------------------------------- */

    0,                          /* coord rowsonscreen */
    0,                          /* coord scrbrc */
    0,                          /* coord rowtoend */


/* ----------------------------- numbers.c ------------------------------- */

    0,                          /* uchar refs_in_this_slot */
    0,                          /* uchar refs_in_this_sheet */

    DONT_CARE,                  /* colt edtslr_col */
    DONT_CARE,                  /* rowt edtslr_row */

    FALSE,                      /* BOOLEAN slot_in_buffer */
    FALSE,                      /* BOOLEAN output_buffer */
    FALSE,                      /* BOOLEAN global_recalc */
    FALSE,                      /* BOOLEAN recalc_bit */
    FALSE,                      /* BOOLEAN buffer_altered */


/* ---------------------------- doprint.c -------------------------------- */

    TRUE,                       /* BOOLEAN wrpbit */
    TRUE,                       /* BOOLEAN jusbit */
    TRUE,                       /* BOOLEAN txnbit */
    TRUE,                       /* BOOLEAN spare___ */
    TRUE,                       /* BOOLEAN rcobit */
    TRUE,                       /* BOOLEAN minbit */
    TRUE,                       /* BOOLEAN borbit */
    FALSE,                      /* BOOLEAN iowbit */

    DONT_CARE,                  /* intl real_pagelength */
    DONT_CARE,                  /* intl real_pagcnt */


/* ----------------------------- dialog.c -------------------------------- */

    0,                          /* intl encpln */
    0,                          /* intl enclns */
    0,                          /* intl pagoff */
    0,                          /* intl pagnum */
    0,                          /* intl filpof */
    0,                          /* intl filpnm */


/* ----------------------------- riscos.c -------------------------------- */

    #if RISCOS
    window_NULL,                /* riscos_window main_window */
    #endif


/* ----------------------------------------------------------------------- */

    #if RISCOS
    0,                          /* intl riscos_font_error */
    FALSE,                      /* BOOL old_xf_fileloaded */

    /* caret never at 0,0 */
    0,                          /* coord lastcursorpos_x */
    0,                          /* coord lastcursorpos_y */
    TRUE,                       /* BOOL xf_acquirecaret */
    #endif


/* ----------------------------------------------------------------------- */

    FALSE,                      /* BOOL file_is_help */

    #if RISCOS
    "",                         /* char numericslotcontents[] */
    NULL,                       /* dbox main_dbox */
    FALSE,                      /* BOOL grid_on */
    DONT_CARE,                  /* intl charvspace */
    #endif

    FALSE,                      /* BOOLEAN xf_interrupted */
    #if RISCOS
    0,                          /* BOOLEAN unused_bit_at_bottom */
    #endif


/* ----------------------------- dialog.c -------------------------------- */

    DONT_CARE,                  /* uchar d_save_format */

    DONT_CARE,                  /* uchar d_poptions_PL */
    DONT_CARE,                  /* uchar d_poptions_LS */
    DONT_CARE,                  /* uchar *d_poptions_PS */
    DONT_CARE,                  /* uchar d_poptions_TM */
    DONT_CARE,                  /* uchar d_poptions_HM */
    DONT_CARE,                  /* uchar d_poptions_FM */
    DONT_CARE,                  /* uchar d_poptions_BM */
    DONT_CARE,                  /* uchar d_poptions_LM */
    DONT_CARE,                  /* uchar *d_poptions_HE */
    DONT_CARE,                  /* uchar *d_poptions_FO */

    DONT_CARE,                  /* uchar *d_options_DE */
    DONT_CARE,                  /* uchar d_options_TN */
    DONT_CARE,                  /* uchar d_options_IW */
    DONT_CARE,                  /* uchar d_options_BO */
    DONT_CARE,                  /* uchar d_options_JU */
    DONT_CARE,                  /* uchar d_options_WR */
    DONT_CARE,                  /* uchar d_options_DP */
    DONT_CARE,                  /* uchar d_options_MB */
    DONT_CARE,                  /* uchar d_options_TH */
    DONT_CARE,                  /* uchar d_options_IR */
    DONT_CARE,                  /* uchar d_options_DF */
    DONT_CARE,                  /* uchar *d_options_LP */
    DONT_CARE,                  /* uchar *d_options_TP */
    DONT_CARE,                  /* uchar d_options_GR */

    DONT_CARE,                  /* uchar d_recalc_AM */
    DONT_CARE,                  /* uchar d_recalc_RC */
    DONT_CARE,                  /* uchar d_recalc_RI */
    DONT_CARE,                  /* uchar *d_recalc_RN */
    DONT_CARE,                  /* uchar *d_recalc_RB */

    #if RISCOS
    DONT_CARE,                  /* intl charvrubout_pos */
    DONT_CARE,                  /* intl textcell_xorg */
    DONT_CARE,                  /* intl textcell_yorg */
    #endif

    FALSE,                      /* BOOL recalc_forced */

    #if RISCOS
    0,                          /* colt old_numcol */
    0,                          /* rowt old_numrow */
    0,                          /* coord old_rowsonscreen */
    #endif

    FALSE,                      /* BOOL xf_fileloaded */


/* ----------------------------------------------------------------------- */

    0,                          /* intl rebuild_sheet_links */
    0,                          /* intl pict_on_screen */
    0,                          /* intl pict_was_on_screen */

    'P',                        /* optiontype initial_filetype */

    #if RISCOS
    0,                          /* uchar numericslotcontents_length */
    "",                         /* char slotcoordinates[] */
    FALSE,                      /* BOOL riscos_fonts */
    0,                          /* riscos_window fake_window */
    NULL,                       /* dbox fake_dbox */
    NULL,                       /* char *global_font */
    12 * 16,                    /* intl global_font_x (1/16th of a point) */
    12 * 16,                    /* intl global_font_y (1/16th of a point) */
    12000,                      /* intl global_font_leading (millipoints) */
    0,                          /* intl hbar_length */
    0,                          /* pict_currow */
    #endif

    0,                          /* intl old_lescroll */
    -1,                         /* intl dspfld_from */
    FALSE,                      /* BOOL quitseen */

    #if RISCOS
    0,                          /* intl curr_xext */
    0,                          /* intl curr_yext */
    0,                          /* intl delta_scx */
    0,                          /* intl delta_scy */
    #endif

    0,                          /* coord colsonscreen */
    0,                          /* coord n_colfixes */

    #if RISCOS
    DONT_CARE,                  /* intl charvrubout_neg */
    DONT_CARE,                  /* intl vdu5textoffset */
    DONT_CARE,                  /* intl fontbaselineoffset */
    #endif

    0,                          /* intl old_lecpos */

    #if defined(SHOW_CURRENT_ROW)
    FALSE,                      /* BOOLEAN out_rowborout */
    FALSE,                      /* BOOLEAN out_rowborout1 */

    0,                          /* coord rowborout */
    0,                          /* coord rowborout1 */
    #endif

/* ----------------------------------------------------------------------- */

    NULL                        /* the_last_thing */
    };  /* end of initial_window_data */


#endif  /* MANY_DOCUMENTS */


/************************************************
*                                               *
*  initialiser called only on program startup   *
*  to set up the initial template data          *
*                                               *
************************************************/

extern void
init_window_data_once(void)
{
#if defined(MANY_DOCUMENTS)
    dochandle doc;

    document_array[DOCHANDLE_NONE] = NO_DOCUMENT;

    for(doc = 1; doc < DOCHANDLE_MAX; doc++)
        document_array[doc] = NULL;

    /* don't use select_document() as this may check against documentlist */
    sb = &initial_window_data;
#endif

    dialog_initialise_once();       /* set default options correctly */

    /* ensure we don't mess the initial window data up accidentally */
    select_document(NO_DOCUMENT);


    /* some other once off inits */
    (void) init_stack();
}


/************************************************
*                                               *
* create a new, initialised document            *
*                                               *
* --out--                                       *
*   TRUE    ok                                  *
*   FALSE   failed, destroyed, error reported   *
*                                               *
************************************************/

extern BOOL
create_new_document(void)
{
    dochandle old_doc = current_document_handle();
    BOOL ok = FALSE;
    intl res;

#if !defined(MANY_DOCUMENTS)
    if(nDocuments != 0)
        {
        NewWindow_fn();     /* just reinitialise if not first time */
        return(TRUE);
        }
    else
        {
#else
    window_data *new_sb;

    #if defined(STARTTRACE)
        trace_on();
    #endif

    tracef0("************** create_new_document() **************\n");

    new_sb = (window_data *) alloc_ptr_using_cache(sizeof(window_data), &res);
    if(!new_sb)
        tracef0("Unable to claim space for new document window data\n");
    else
        {
        /* allocate a handle for this document */
        dochandle newdoc;

        for(newdoc = 1; newdoc < DOCHANDLE_MAX; newdoc++)
            if(document_array[newdoc] == NULL)
                break;

        #if TRACE
            if(newdoc == DOCHANDLE_MAX)
                reperr_fatal("Document handle allocation failed");
        #endif

        *new_sb = initial_window_data;      /* copy over template data */

        new_sb->DocHandle = newdoc;

        /* add to head of document list (new ones at top of menus etc.) */
        new_sb->link  = document_list;
        document_list = new_sb;

        document_array[newdoc] = new_sb;    /* note in array */

        sb = new_sb;                    /* select it as current */

#endif  /* MANY_DOCUMENTS */


        /* call various people to initialise now */

        if(TRUE                     /* any FALSE will abort sequence */
            &&  dialog_initialise()
            &&  constr_initialise()
            #if RISCOS
            &&  riscos_initialise() /* does screen_initialise() itself */
            #else
            &&  screen_initialise()
            #endif
            )
            ok = TRUE;

        if(ok)
            {
            #if MS
            front_doc = current_document_handle();
            #endif

            nDocuments++;
            }
        else
            {
            /* undo our work on this one & restore old doc on failure */
            destroy_current_document();
            select_document_using_handle(old_doc);
            }
        }

    if(!ok)
        reperr((res < 0) ? res : ERR_NOROOM, _unable_to_create_new_document_STR);

    tracef1("************** create_new_document() returns %s ***********\n",
                trace_boolstring(ok));

    #if defined(STARTTRACE)
    trace_off();
    #endif

    return(ok);
}


/************************************************
*                                               *
* create a new, initialised, untitled document  *
*                                               *
* --out--                                       *
*   TRUE    ok                                  *
*   FALSE   failed, destroyed, error reported   *
*                                               *
************************************************/

extern BOOL
create_new_untitled_document(void)
{
    dochandle old_doc = current_document_handle();
    BOOL ok;

    if(!create_new_document())
        return(FALSE);
    
    ok = set_untitled_document();

    if(!ok)
        {
        /* undo our work on this one & restore old doc on failure */
        destroy_current_document();
        select_document_using_handle(old_doc);
        }

    return(ok);
}


#if defined(TRACE_DOC)  /* macroed otherwise */

/****************************************
*                                       *
*  export the current document address  *
*                                       *
****************************************/

extern window_data *
current_document(void)
{
    tracef1("current_document() yields &%p\n", sb);
    return(sb);
}


/****************************************
*                                       *
*  export the current document handle   *
*                                       *
****************************************/

extern dochandle
current_document_handle(void)
{
    dochandle curdoc;

    #if !defined(MANY_DOCUMENTS)
        curdoc = DocHandle;
    #else
        curdoc = (sb != NO_DOCUMENT) ? sb->DocHandle : DOCHANDLE_NONE;
    #endif

    tracef1("current_document_handle() yields %d\n", curdoc);
    return(curdoc);
}

#endif  /* TRACE */


/********************************************
*                                           *
*       destroy the current document        *
*                                           *
* --out--                                   *
*   resources freed, no document selected   *
*                                           *
********************************************/

extern void
destroy_current_document(void)
{
#if !defined(MANY_DOCUMENTS)
    NewWindow_fn();                 /* just reinitialise */
#else
    dochandle doc = current_document_handle();
    intl i;
    window_data *pp;
    window_data *cp;

    tracef0("************** destroy_current_document() **************\n");

    #if MS
    if(front_doc == doc)
        front_doc = DOCHANDLE_NONE;
    #endif

    /* clear marked block */
    if(blkdochandle == doc)
        {
        blkdochandle = DOCHANDLE_NONE;
        blkstart.col = blkend.col = NO_COL;
        }

    /* clear search block */
    if(schdochandle == doc)
        schdochandle = DOCHANDLE_NONE;

    /* clear from saved position stack */
    for(i = 0; i < saved_index; i++)
        if(saved_pos[i].ref.doc == (docno) doc)
            saved_pos[i].ref.doc = (docno) DOCHANDLE_NONE;


    /* call various people to finalise now */

    #if RISCOS
    font_close_file(current_document_handle());

    draw_close_file(current_document_handle());

    graph_close_file(current_document_handle());
    #endif

    exp_close_file(current_document_handle());

    screen_finalise();

    constr_finalise();

    dialog_finalise();

    #if RISCOS
    riscos_finalise();
    #endif


    /* remove from document list */
    pp = (window_data *) &document_list;

    while((cp = pp->link) != NULL)
        if(cp == sb)
            {
            pp->link = cp->link;
            break;
            }
        else
            pp = cp;

    if(TRACE  &&  !cp)
        reperr_fatal("Failed to find current document");

    /* deallocate from handle table */
    document_array[sb->DocHandle] = NULL;

    /* free window data and deselect */
    free(sb);
    select_document(NO_DOCUMENT);

    nDocuments--;

    tracef0("*********** destroy_current_document() ended ***********\n");
#endif
}


/********************************************
*                                           *
* find out how many documents are modified  *
*                                           *
********************************************/

extern intl
documents_modified(void)
{
#if !defined(MANY_DOCUMENTS)
    return(xf_filealtered ? 1 : 0);
#else
    window_data *wdp = NULL;
    intl count = 0;

    /* loop over documents */
    while((wdp = next_document(wdp)) != NULL)
        if(wdp->Xxf_filealtered)
            count++;

    return(count);
#endif
}



/* find the leafname in a filename */

#if RISCOS

#if FALSE       /* use copy in WimpLib */

extern char *
leafname(const char *filename)
{
    char *leaf = (char *) filename + strlen(filename);  /* point to null */
    char ch;

    while(leaf > filename)
        if(((ch = *--leaf) == '.')  ||  (ch == ':'))
            return(leaf+1);

    return(leaf);
}

#endif

#elif MS

extern char *
leafname(const char *filename)
{
    char *leaf = (char *) filename + strlen(filename);  /* point to null */
    char ch;

    while(leaf > filename)
        if(((ch = *--leaf) == '\\')  ||  (ch == ':'))
            return(leaf+1);

    return(leaf);
}

#endif


/* lexical comparison on strings */

#if !MS

extern intl
stricmp(const char *a, const char *b)
{
    char c1, c2;

    while(TRUE)
        {
        c1 = *a++;
        c1 = tolower(c1);
        c2 = *b++;
        c2 = tolower(c2);

        if(c1 != c2)
            return(c1 - c2);

        if(c1 == 0)
            return(0);          /* no need to check c2 */
        }
}


extern intl
strnicmp(const char *a, const char *b, size_t n)
{
    char c1, c2;

    while(n-- > 0)
        {
        c1 = *a++;
        c1 = tolower(c1);
        c2 = *b++;
        c2 = tolower(c2);

        if(c1 != c2)
            return(c1 - c2);

        if(c1 == 0)
            return(0);          /* no need to check c2 */
        }

    return(0);
}


/* find first occurrence of b in a, or NULL, case insensitive */

extern char *
stristr(const char *a, const char *b)
{
    for(;;)
        {
        intl i;

        for(i = 0; ; i++)
            {
            if(b[i] == '\0')
                return((char *) a);

            if(tolower(a[i]) != tolower(b[i]))
                break;
            }

        if(*a++ == '\0')
            return(NULL);
        }
}

#endif  /* !MS */


/*****************************************
*                                        *
* derived from strncat                   *
* takes indirect count, which is updated *
*                                        *
*****************************************/

extern char *
strncatind(char *a, const char *b, size_t *np /*inout*/)
{
    size_t n = *np;                    /* change 1: added */
    char *p = a;
    while (*p != 0) p++;
    while (n-- > 0)
        if ((*p++ = *b++) == 0) break; /* change 2: was return a; */
    *p = 0;
    *np = n;                           /* change 3: added*/
    return a;
}


#if defined(TRACE_DOC)  /* else macroed */

/************************************************************************
*                                                                       *
*  search document list for document with given key = document handle   *
*                                                                       *
* --out--                                                               *
*   NULL            no document found with this handle                  *
*   NO_DOCUMENT     handle was DOCHANDLE_NONE                           *
*   window_data *   otherwise                                           *
*                                                                       *
************************************************************************/

extern window_data *
find_document_using_handle(dochandle doc)
{
    window_data *wdp;

    tracef1("find_document_using_handle(%d) ", doc);

    wdp = (doc >= DOCHANDLE_MAX) ? NULL : document_array[doc];

    tracef1("yields &%p\n", (int) wdp);
    return(wdp);
}

#endif  /* TRACE_DOC */


/****************************************************************
*                                                               *
*  search document list for document with given key = leafname  *
*                                                               *
****************************************************************/

extern dochandle
find_document_using_leafname(const char *name)
{
#if !defined(MANY_DOCUMENTS)
    IGNOREPARM(name);
    return(DOCHANDLE_NONE);
#else
    window_data *wdp = (window_data *) &document_list;
    window_data *foundp = NULL;
    intl count = 0;
    char *leaf = leafname(name);

    tracef1("find_document_using_leafname(%s)\n", name);

    while((wdp = wdp->link) != NULL)
        if(!stricmp(leafname(wdp->Xcurrentfilename), leaf))
            {
            foundp = wdp;
            count++;
            }

    if(count == 1)
        return(foundp->DocHandle);
    elif(count == 0)
        return(DOCHANDLE_NONE);
    else
        return(DOCHANDLE_SEVERAL);
#endif
}


#if RISCOS

/********************************************************************
*                                                                   *
* search document list for document with given key = window handle  *
*                                                                   *
********************************************************************/

extern window_data *
find_document_using_window(riscos_window w)
{
    window_data *wdp = (window_data *) &document_list;

    tracef1("find_document_using_window(%d) ", w);

    while((wdp = wdp->link) != NULL)
        if(wdp->Xmain_window == w)
            break;

    tracef1("yields &%p\n", (int) wdp);
    return(wdp);
}

#endif  /* RISCOS */


/****************************************************************
*                                                               *
*  search document list for document with given key = handle    *
*                                                               *
* --out--                                                       *
*   NULL means no document found with this handle               *
*   else leafname^                                              *
*                                                               *
****************************************************************/

extern char *
find_leafname_using_handle(dochandle doc)
{
#if !defined(MANY_DOCUMENTS)
    IGNOREPARM(name);
    return(NULL);
#else
    window_data *wdp = find_document_using_handle(doc);

    return((!wdp  ||  (wdp == NO_DOCUMENT)  ||  !(wdp->Xcurrentfilename))
                ? NULL
                : leafname(wdp->Xcurrentfilename));
#endif
}


extern void
front_document_using_handle(dochandle newdoc)
{
#if !defined(MANY_DOCUMENTS)
    IGNOREPARM(newdoc);
#else
    #if RISCOS
    dochandle doc = current_document_handle();

    if(newdoc != doc)
        select_document_using_handle(newdoc);

    /* must do NOW as we're unlikely to draw_screen() this document */
    riscos_frontmainwindow(TRUE);

    if(newdoc != doc)
        select_document_using_handle(doc);

    #elif MS

    select_document_using_handle(newdoc);

    xf_draweverything = TRUE;

    /* has side effect of making it current too (for the moment) */
    #endif
#endif
}


/************************************************************
*                                                           *
*  ensure all documents with modified buffers are updated   *
*                                                           *
************************************************************/

extern BOOL
mergebuf_all(void)
{
#if !defined(MANY_DOCUMENTS)
    return(mergebuf());
#else
    dochandle doc = current_document_handle();
    window_data *wdp = NULL;
    BOOL res = TRUE;

    while((wdp = next_document(wdp)) != NULL)
        {
        select_document(wdp);
        res &= mergebuf_nocheck();      /* ensure buffer changes to main data */
        filbuf();
        }

    select_document_using_handle(doc);

    return(res);            /* whether any mergebuf() failed */
#endif
}


/****************************************
*                                       *
*   return the next document handle     *
*                                       *
****************************************/

extern dochandle
next_document_handle(dochandle cdoc)
{
    window_data *np = (cdoc == DOCHANDLE_NONE)
                            ? document_list
                            : find_document_using_handle(cdoc)->link;
    dochandle ndoc  = (np == NULL) ? DOCHANDLE_NONE : np->DocHandle;

    tracef2("next_document_handle(&%p) yields &%p\n", cdoc, ndoc);
    return(ndoc);
}


#if defined(TRACE_DOC)  /* macroed otherwise */

/****************************************
*                                       *
*   return the next document address    *
*                                       *
* relies on documents not being         *
* created/deleted/moved between calls   *
*                                       *
****************************************/

extern window_data *
next_document(window_data *cp)
{
    window_data *np = (cp == NULL) ? document_list : cp->link;

    tracef2("next_document(&%p) yields &%p\n", cp, np);
    return(np);
}


/************************************
*                                   *
*  make the given document current  *
*                                   *
************************************/

extern void
select_document(window_data *new_sb)
{
    tracef1("selecting document &%p\n", new_sb);

    if(new_sb != NO_DOCUMENT)
        {
        window_data *wdp = NULL;
        BOOL found = FALSE;

        while((wdp = next_document(wdp)) != NULL)
            if(wdp == new_sb)
                {
                found = TRUE;
                break;
                }

        if(!found)
            reperr_fatal("Tried to select unknown document &%p", (int) new_sb);
        }

    sb = new_sb;
}


extern void
select_document_using_handle(dochandle doc)
{
    window_data *wdp = find_document_using_handle(doc);

    if(wdp == NULL)
        reperr_fatal("Document not found for handle %d", (int) doc);

    sb = wdp;
}


extern void
trace_sb(void)
{
    tracef1("current_document is &%p\n", sb);
}

#endif  /* TRACE_DOC */


/********************************
*                               *
*  set untitled document name   *
*                               *
********************************/

extern BOOL
set_untitled_document(void)
{
    char buffer[256];
    BOOL ok;

    sprintf(buffer, UNTITLED_ZD_STR, NextUntitledNumber);

    ok = str_set(&currentfilename, buffer);
    
    if(ok)
        NextUntitledNumber++;

    #if RISCOS
    riscos_settitlebar(currentfilename);
    #endif

    return(ok);
}

/* end of windvars.c */
