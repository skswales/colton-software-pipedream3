/* constr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       constr.c - module that creates the PipeDream data structure
 *              also handles things that fiddle with the
 *              structure in non-standard ways
 * Author:      RJM August 1987
*/

/* standard header files */
#include "flags.h"


#if RISCOS
#include "os.h"
#include "flex.h"

#elif ARTHUR
#include "altkey.h"

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
#include "riscmenu.h"
#endif


/* exported functions */

extern BOOL check_not_blank_sheet(void);
extern void constr_initialise_once(void);
extern BOOL constr_initialise(void);
extern void delfch(void);
extern void delfil(void);
extern BOOL dftcol(void);
extern void my_init_ref(uchar type);
extern uchar *my_next_ref(uchar *ptr, uchar type);
extern void newbul(void);
extern void newbuw(void);
extern void newfil(void);
extern void reset_filpnm(void);
extern BOOL store_bad_formula(uchar *ptr, colt tcol, rowt trow, intl error);
extern void str_clr(char **strvar);
extern BOOL str_set(char **strvar, const uchar *str);
extern void str_swap(char **a, char **b);


/* internal functions */

static BOOL re_init_file(void);


/* structures and parameters for sort */

#define SORT_FIELD struct _sort_field

SORT_FIELD
    {
    colt column;
    BOOL reverse;
    };

/* structure of entry in key list */
struct sortentry
    {
    rowt keyrow;
    rowt rowsinrec;
    };

typedef struct sortentry *sortep;


/* static data for sort */

static intl fields;
static SORT_FIELD sort_fields[SORT_FIELD_DEPTH];


/* ----------------------------------------------------------------------- */

#ifdef WATCH_ALLOCS
    #define TRACE_WATCH_ALLOCS  (TRACE && TRUE)
#else
    #define TRACE_WATCH_ALLOCS  FALSE
#endif


/**********************************
*                                 *
* store bad formula at tcol, trow *
*                                 *
**********************************/

extern BOOL
store_bad_formula(uchar *ptr, colt tcol, rowt trow, intl error)
{
    slotp tslot;

    /* don't remove graph ref */
    draw_tree_removeslot(tcol, trow);

    tslot = createslot(tcol, trow, strlen((char *) ptr) + 1, SL_BAD_FORMULA);

    if(!tslot)
        return(reperr_null(ERR_NOROOM));

    strcpy((char *) tslot->content.number.text, (char *) ptr);
    tslot->content.number.result.errno = error;
    return(TRUE);
}


/******************************
*                             *
* insert partial row of slots *
*                             *
******************************/

static BOOL
insertrow(colt scol, colt ecol, intl upd_flags)
{
    colt tcol;
    rowt trow;

    if(!mergebuf())
        return(FALSE);

    trow = currow;

    for(tcol = scol; tcol < ecol; tcol++)   
        if(!insertslotat(tcol, trow))
            {
            /* remove those we'd added */
            while(--tcol >= scol)
                killslot(tcol, trow);

            return(FALSE);
            }

    mark_to_end(currowoffset);
    out_rebuildvert = TRUE;

    updroi(upd_flags);

    /* callers of insertrow must do their own sendings */

    return(TRUE);
}


/***********************
*                      *
* insert row in column *
*                      *
***********************/

extern void
InsertRowInColumn_fn(void)
{ 
    (void) insertrow(curcol, curcol+1, U_ONE);

    #if RISCOS
    graph_send_split_blocks(curcol, currow, curcol+1, LARGEST_ROW_POSSIBLE);
    #endif
}


/*************
*            *
* insert row *
*            *
*************/

extern void
InsertRow_fn(void)
{
    (void) insertrow(0, numcol, U_ALL);

    #if RISCOS
    graph_send_split_blocks(0, currow, numcol, currow+1);
    #endif
}


/**************
*             *
* insert page *
*             *
**************/

extern void
InsertPageBreak_fn(void)
{
    slotp tslot;

    xf_flush = TRUE;

    while(dialog_box(D_INSPAGE))
        {
        tslot = insertrow(0, numcol, U_ALL)
                    ? graph_send_split_blocks(0, currow, numcol, currow+1),
                      createslot(0, currow, 1, SL_PAGE)
                    : NULL;

        if(!tslot)
            {
            dialog_box_end();
            reperr_null(ERR_NOROOM);
            return;
            }

        tslot->content.page.condval = (intl) d_inspage[0].option;

        out_screen = out_rebuildvert = TRUE;
        filealtered(TRUE);

        if(dialog_box_ended())
            break;
        }
}


/**********************************************
*                                             *
* check if we have a sheet, if not create one *
*                                             *
**********************************************/

extern BOOL
check_not_blank_sheet(void)
{
    if(!numrow)
        {
        if(!createslot((colt) 0, (rowt) 0, 1, SL_TEXT))
            return(FALSE);

        filealtered(FALSE);
        }

    return(TRUE);
}


/********************************
*                               *
*  destroy this document's data *
*                               *
********************************/

extern void
constr_finalise(void)
{
    tracef0("constr_finalise()\n");

    delfil();                       /* delete all slots in the file */

    killcoltab();                   /* delete the column structure */

    str_clr(&currentfilename);
    str_clr(&currentdirectory);

    delete_list(&first_file);
}


/************************************
*                                   *
* initialise data for this document *
*                                   *
************************************/

extern BOOL
constr_initialise(void)
{
    tracef0("constr_initialise()\n");

    newbul();

    if(!re_init_file())
        return(FALSE);

    return(dftcol());
}


/*************************************************************************
*                                                                       *
* initialise data for this program: load pd.ini and remember defaults   *
*                                                                       *
*************************************************************************/

extern void
constr_initialise_once(void)
{
    char array[MAX_FILENAME];
    BOOL ok;

    tracef0("constr_initialise_once()\n");

    if(create_new_document())
        {
        ok = add_path(array, INITFILE_STR, FALSE)
                    ? excloa(array)
                    : FALSE;

        delfil();

        /* save away default column structure in any case */
        /* one will have been created by create_new_document() */
        savecoltab();

        /* save away default options if ok */

        if(ok)
            save_options_to_list();

        destroy_current_document();

        #if RISCOS
        /* restore place where next window will appear */
        riscos_resetwindowpos();
        #endif
        }

    #if !defined(PRINT_OFF)
    load_driver(); 
    #endif

    /* some tickable things may be updated or not yet initialised */
    #if !defined(SPELL_OFF)
    check_state_changed();
    #endif
    insert_state_changed();
    menu_state_changed();
}


/***************************
*                          *
* reset variables for file *
*                          *
***************************/

extern void
newbul(void)
{
    reset_filpnm();
    newbuw();
}


/********************************
*                               *
* reset variables for sub-file  *
*                               *
********************************/

extern void
newbuw(void)
{
    killcoltab();

    str_clr(&currentfilename);

    #if RISCOS
        riscos_settype(&currentfileinfo, PIPEDREAM_FILETYPE);
        riscos_readtime(&currentfileinfo);
    #endif


    #if !defined(MANY_DOCUMENTS)
        /* initialize vectors */

        if(horzvec_mh)
            {
            SCRCOL *cptr        = horzvec();
            SCRCOL *last_cptr   = cptr + maximum_cols;

            while(cptr <= last_cptr)
                {
                cptr->colno     = 0;
                cptr++->flags   = LAST;
                }
            }

        if(vertvec_mh)
            {
            SCRROW *rptr        = vertvec();
            SCRROW *last_rptr   = rptr + maximum_rows;

            while(rptr <= last_rptr)
                {
                rptr->flags     = LAST;
                rptr->page      = 0;
                rptr++->rowno   = 0;
                }
            }
    #endif

    rowsonscreen = colsonscreen = 0;
    n_rowfixes = n_colfixes = 0;

    curcol = 0;
    currow = 0;

    numcol = colsintable = 0;
    rebnmr();

    pagnum = curpnm = 1;
    pagoff = 1;

    lecpos = lescrl = 0;
    *linbuf = '\0';

    refs_in_this_sheet = FALSE;
}


/***********
*          *
* new file *
*          *
***********/

extern void
newfil(void)
{
/*  update_variables(); done in re_init_file*/      /* update variables from option page */
    delfil();               /* delete all slots */
    clslnf();               /* close link files */

    newbuw();               /* initialize variables */
    re_init_file();         /* reinitialize file */
}


static BOOL
re_init_file(void)
{
    /* create new option list from def_option_list */

    #if !defined(MANY_DOCUMENTS)
    init_dialog_box(D_OPTIONS);
    init_dialog_box(D_POPTIONS);
    init_dialog_box(D_RECALC);

    recover_options_from_list();
    #endif

    update_variables();

    /* get new colstart and copy from def_colstart */
    /* note that the first time round this does not create any structure */
    return(restcoltab());
}


/***************************************************
*                                                  *
* reset the page numbers at the start of this file *
*                                                  *
***************************************************/

extern void
reset_filpnm(void)
{
    if(!glbbit || curfil == 0)
        filpnm = curpnm = pagnum = filpof = pagoff = 1;

    if(!str_isblank(d_poptions_PS))
        filpnm = curpnm = pagnum = atoi(d_poptions_PS);
}


/******************
*                 *
* free file chain *
*                 *
******************/

extern void
delfch(void)
{
    glbbit = FALSE;
    delete_list(&first_file);
}


/***************************
*                          *
* delete all slots in file *
*                          *
***************************/

extern void
delfil(void)
{
    tree_delete();

    delcol(0, numcol);

    #if !defined(MANY_DOCUMENTS)
    buffer_altered = slot_in_buffer = FALSE;
    #endif
}


/*********************************************************
*                                                        *
* set up default cols, a-f, wrapwidth descending from 72 *
*                                                        *
*********************************************************/

extern BOOL
dftcol(void)
{
    if(!numcol)
        {
        if(!createcol(5))
            return(FALSE);

        dstwrp(0, 72);

        filealtered(FALSE);
        }

    return(TRUE /*check_not_blank_sheet()*/);
}


/******
*     *
* new *
*     *
******/

extern void
NewWindow_fn(void)
{ 
#if defined(MANY_DOCUMENTS)  &&  defined(MANY_WINDOWS)

    if(create_new_untitled_document())
        ;   /* draw_screen(); --- no need to do explicitly here */
#else

    if(xf_inexpression)
        {
        reperr_null(ERR_EDITINGEXP);
        return;
        }

    if(!mergebuf())
        return;

    if(!save_existing())
        return;

    saved_index = 0;
    newfil();
    str_clr(&currentfilename);
    str_clr(&d_save[0].textfield);
    d_save_FORMAT = 'P';        /* reset default type to PipeDream */

    reset_filpnm();
    dftcol();
    delfch();
    #if !defined(SPELL_OFF)
    del_spellings();
    #endif

    xf_drawmenuheadline = out_screen = out_rebuildvert = out_rebuildhorz = TRUE;
    filealtered(FALSE);

#endif
}


/****************************************
*                                       *
* work out whether this is a new record *
*                                       *
****************************************/

static intl
isnewrec(slotp sl)
{
    /* test multi-row sort option */
    if(d_sort[SORT_MULTI_ROW].option != 'Y')
        return(TRUE);

    if(!sl  ||  isslotblank(sl))
        return(FALSE);

    #if 0
    /* SKS removed 21.3.90 'cos RJM complained */
    if((sl->type == SL_TEXT)  &&  (sl->content.text[0] == SPACE))
        return(FALSE);
    #endif

    return(TRUE);
}

extern void
my_init_ref(uchar type)
{
    if(type != SL_TEXT)
        exp_initslr();
}


/***************************************************
*                                                  *
* get next slot ref in either text or numeric slot *
*                                                  *
***************************************************/

extern uchar *
my_next_ref(uchar *ptr, uchar type)
{
    if(type == SL_TEXT) 
        {
        for( ; *ptr != '\0'; ptr++)
            if(*ptr == SLRLDI)
                return(ptr+1);
        return(NULL);
        }
    else
        return(exp_findslr(ptr));
}


/********************************
*                               *
* compare two rows when sorting *
*                               *
********************************/

static jmp_buf sortpoint;

static intl
rowcomp(const void *srt1, const void *srt2)
{
    colt col;
    SYMB_TYPE slot1, slot2;
    intl res;

    tracef2("[rowcomp(&%p, &%p)]\n", srt1, srt2);
    col = 0;
    do  {
        /* make long sorts escapeable */
        if(ctrlflag)
            longjmp(sortpoint, 1);

        slot1.value.slot.doc = slot2.value.slot.doc = 0;

        slot1.value.slot.col = slot2.value.slot.col = sort_fields[col].column;

        slot1.value.slot.row = ((sortep) srt1)->keyrow;
        slot2.value.slot.row = ((sortep) srt2)->keyrow;

        /* convert from references to contents */
        eval_slot(&slot1, FALSE);
        eval_slot(&slot2, FALSE);

        res = symbcmp(&slot1, &slot2);

        /* if equal at this column, loop */
        if(res)
            break;
        }
    while(++col < fields);

    /* were they equal ? */
    if(!res)
        return(0);

    if(sort_fields[col].reverse)
        res = (res > 0) ? -1 : 1;

    return(res);
}

/*************************************************************
*                                                            *
* Fast sort                                                  *
* MRJC May 1989                                              *
* This code uses a new algorithm to build up a separate list *
* of the keys to be sorted, keeping 'records' together, and  *
* then sorting the list of keys, followed by swapping the    *
* rows in the main spreadsheet to correspond with the sorted *
* list. 'Records' start with a non-blank entry in a column - *
* the key, and include all following blank rows upto the     *
* next non-blank entry                                       *
*                                                            *
* sort a block of slots by a specified or the current column *
* update slot references to slots in block by default        *
*                                                            *
*************************************************************/

extern void
SortBlock_fn(void)
{
    rowt i, n, row;
    BOOL updaterefs;
    intl nrecs, rec, nrows;
    colt pkeycol;
    mhandle sortblkh, sortrowblkh;
    sortep sortblkp, sortp;
    rowt *sortrowblkp, *rowtp;
    char array[20];

    if(!mergebuf())
        return;

    /* check two markers set in this document & more than one row in block */
    if(!MARKER_DEFINED())
        {
        reperr_null((blkstart.col != NO_COL)
                            ? ERR_NOBLOCKINDOC
                            : ERR_NOBLOCK);
        return;
        }

    if(blkend.row == blkstart.row)
        {
        reperr_null(ERR_NOBLOCK);
        return;
        }

    /* first, get the user's requirements */

    (void) init_dialog_box(D_SORT);

    /* if this column is in the block, give it to the dialog box */
    if((curcol >= blkstart.col)  &&  (curcol <= blkend.col))
        {
        writecol(array, curcol);
        if(!str_set(&d_sort[SORT_FIELD_COLUMN].textfield, array))
            return;
        }

    while(dialog_box(D_SORT))
        {
        /* read the fields in - note that variable fields ends up as number read */
        fields = 0;

        do  {
            buff_sofar = d_sort[fields * 2 + SORT_FIELD_COLUMN].textfield;
            sort_fields[fields].column = getcol();
            if(sort_fields[fields].column == NO_COL)
                break;
            sort_fields[fields].reverse = (d_sort[fields * 2 + SORT_FIELD_ASCENDING].option == 'N');
            }
        while(++fields < SORT_FIELD_DEPTH);

        if(!fields)
            {
            reperr_null(ERR_BAD_COL);
            continue;
            }

        updaterefs  = (d_sort[SORT_UPDATE_REFS].option == 'Y');

        n = blkend.row - blkstart.row + 1;

        /* do the actual sort */

        sortblkh = sortrowblkh = 0;

        /* we did a merge above */
        slot_in_buffer = FALSE;

        /* load column number of primary key */
        pkeycol = sort_fields[0].column;

        /* count the number of records to be sorted */
        for(i = blkstart.row + 1, nrecs = 1; i <= blkend.row; ++i)
            if(isnewrec(travel(pkeycol, i)))
                ++nrecs;

        /* allocate array to be sorted */
        sortblkh = alloc_handle_using_cache(sizeof(struct sortentry) *
                                            (word32) nrecs);
        if(sortblkh <= 0)
            {
            dialog_box_end();
            reperr_null(ERR_NOROOM);
            goto endpoint;
            }

        /* allocate row table */
        sortrowblkh = alloc_handle_using_cache(sizeof(rowt) *
                            (word32) (blkend.row - blkstart.row + 1));
        if(sortrowblkh <= 0)
            {
            dialog_box_end();
            reperr_null(ERR_NOROOM);
            goto endpoint;
            }

        #if MS || ARTHUR
        ack_esc();
        #endif

        escape_enable();

        /* switch on indicator */
        actind(ACT_SORT, 0);

        /* load pointer to array */
        sortblkp = list_getptr(sortblkh);

        /* load array with records */
        for(i = blkstart.row, sortp = sortblkp; i <= blkend.row; ++i, ++sortp)
            {
            tracef2("[SortBlock: adding &%p, row %d]\n", sortp, i);
            sortp->keyrow = i;

            while(++i <= blkend.row)
                if(isnewrec(travel(pkeycol, i)))
                    break;

            sortp->rowsinrec = (i--) - sortp->keyrow;
            }

        tracef2("[SortBlock: sorting array &%p, %d elements]\n", sortblkp, nrecs);

        if(setjmp(sortpoint))
            goto endpoint;
        else
            /* sort array */
            qsort(sortblkp, nrecs, sizeof(struct sortentry), rowcomp);

        /* build table of rows to be sorted */
        sortrowblkp = list_getptr(sortrowblkh);

        /* for each record */
        for(sortp = sortblkp, rec = 0, rowtp = sortrowblkp;
            rec < nrecs;
            ++rec, ++sortp)
            /* for each row in the record */
            for(row = sortp->keyrow, n = sortp->rowsinrec;
                n;
                --n, ++row, ++rowtp)
                *rowtp = row;

        /* free sort array */
        list_disposehandle(&sortblkh);

        /* exchange the rows in the spreadsheet */
        /* NB. difference between pointers still valid after dealloc */
        nrows = rowtp - sortrowblkp;
        for(n = 0; n < nrows; ++n)
            {
            if(!actind(ACT_SORT, (intl) ((100 * n) / nrows)))
                goto endpoint;

            /* on RISCOS, check that we have reasonable memory free
             * before a row exchange, to stop the exchange falling over
             * in the middle of a row - it will be happy, but the user
             * probably won't!
            */
            #if RISCOS
            if( !flex_extrastore()  &&
                (flex_storefree() < MAXPOOLSIZE)  &&
                ((flex_storefree() + list_howmuchpoolspace()) < MAXPOOLSIZE))
                {
                dialog_box_end();
                reperr_null(ERR_NOROOM);
                goto endpoint;
                }
            #endif

            /* must re-load row block pointer each time, ahem */
            rowtp = ((rowt *) list_getptr(sortrowblkh)) + n;
            row = n + blkstart.row;
            if(*rowtp != row)
                {
                /* do physical swap */
                if(!swap_rows(row,
                              *rowtp,
                              blkstart.col,
                              blkend.col,
                              updaterefs))
                    {
                    reperr_null(ERR_NOROOM);
                    goto endpoint;
                    }
                else
                    /* update list of unsorted rows */
                    {
                    rowt nn;
                    rowt *nrowtp;

                    /* must re-load row block pointer each time, ahem */
                    rowtp = ((rowt *) list_getptr(sortrowblkh)) + n;

                    for(nn = n + 1, nrowtp = rowtp + 1;
                        nn < nrows;
                        ++nn, ++nrowtp)
                        if(*nrowtp == row)
                            {
                            *nrowtp = *rowtp;
                            break;
                            }
                    }
                }
            }

    endpoint:

        /* free sort array */
        list_disposehandle(&sortblkh);

        /* free row table array */
        list_disposehandle(&sortrowblkh);

        /* release any redundant storage */
        garbagecollect();

        actind(DEACTIVATE, NO_ACTIVITY);

        out_screen = recalc_bit = TRUE;
        filealtered(TRUE);

        if(escape_disable())
            dialog_box_end();

        if(dialog_box_ended())
            break;
        }
}


/****************************************************
*                                                   *
* do a string assignment                            *
* if the length of the new string is 0, set to NULL *
* if cannot allocate new string, return FALSE       *
*                                                   *
****************************************************/

extern BOOL
str_set(char **strvar, const uchar *str)
{
    intl len, res;
    char *ptr;

    ptr = *strvar;

    if(ptr)
        {
        /* variable already set to this? */
        if(str  &&  !strcmp(ptr, str))
            {
            vtracef2(TRACE_WATCH_ALLOCS, "str_set(&%p) already \"%s\"\n", strvar, str);
            return(TRUE);
            }

        dispose((void **) strvar);
        }

    len = str ? strlen((const char *) str) : 0;

    if(!len)
        return(TRUE);

    *strvar = ptr = alloc_ptr_using_cache((word32) len + 1, &res);

    if(!ptr)
        return(reperr_null((res < 0) ? res : ERR_NOROOM));

    strcpy(ptr, (const char *) str);

    vtracef2(TRACE_WATCH_ALLOCS, "str_set(&%p) := \"%s\"\n", strvar, str);
    return(TRUE);
}


/****************************
*                           *
* clear a str_set() string  *
*                           *
****************************/

extern void
str_clr(char **strvar)
{
    dispose((void **) strvar);
}


/********************************
*                               *
*  swap two str_set() strings   *
*                               *
********************************/

extern void
str_swap(char **a, char **b)
{
    char *t = *b;
    *b = *a;
    *a = t;
}


/************************************************
*                                               *
* dispose of an object, clearing the pointer    *
*                                               *
************************************************/

#if MS

#undef dispose

extern void
dispose(void **aa)
{
    void *a = *aa;

    if(a)
        {
        *aa = NULL;
        free(a);
        }
}

#endif /* MS */


#if defined(MANY_DOCUMENTS)  &&  defined(MANY_WINDOWS)

#if RISCOS

extern BOOL
same_name_warning(const char *filename, const char *mess)
{
    const char *leafp = leafname(filename);
    char *namep;
    char  buffer[256];
    docno doc;
    intl  index;
    dochandle dhan;

    tracef1("leafname is %s\n", leafp);

    init_supporting_files(NULL, &index);

    while((doc = next_supporting_file(NULL, &index, &dhan, &namep)) != 0)
        {
        tracef2("loading: %s, got supporting file %s\n", leafp, namep);

        if(doc == maybe_cur_docno(DOCHANDLE_NONE))
            continue;

        /* may be registered supported file but not loaded */
        if((dhan != DOCHANDLE_NONE)  &&  !stricmp(namep, leafp))
            {
            sprintf(buffer, mess, filename);
            return(riscdialog_query(buffer) == riscdialogquery_YES);
            }
        }

    return(TRUE);
}


extern BOOL
dependent_files_warning(void)
{
    docno curdoc, doc;
    intl  index;

    init_dependent_docs(&curdoc, &index);

    while((doc = next_dependent_doc(&curdoc, &index)) != 0)
        if(doc != curdoc)
            return(riscdialog_query(close_dependent_files_winge_STR) == riscdialogquery_YES);

    return(TRUE);
}

#endif  /* RISCOS */


extern void
close_window(void)
{
    #if RISCOS
    BOOL hadcaret = (main_window == caret_window);
    #else
    BOOL hadcaret = TRUE;
    #endif
    dochandle ndoc = next_document_handle(current_document_handle());

    destroy_current_document();

    /* if was off end, try first - won't be the deleted one! */
    if(ndoc == DOCHANDLE_NONE)
        ndoc = next_document_handle(ndoc);

    if(hadcaret  &&  (ndoc != DOCHANDLE_NONE))
        {
        select_document_using_handle(ndoc);
        #if RISCOS
        xf_frontmainwindow = TRUE;
        #else
        front_doc = ndoc;
        xf_draweverything = TRUE;
        #endif
        }
    else
        if(MS  &&  create_new_document())
            ;   /* draw_screen() --- no need to do explicitly here */
}


/****************
*               *
* close window  *
*               *
****************/

extern void
CloseWindow_fn(void)
{
    if(!mergebuf_nocheck())
        return;

    filbuf();

    if( save_existing()
        #if RISCOS
        &&  dependent_files_warning()
        &&  dependent_links_warning()
        #endif
        )
        close_window();
}


/****************
*               *
*  next window  *
*               *
****************/

extern void
NextWindow_fn(void)
{
    dochandle cdoc = current_document_handle();
    dochandle ndoc = next_document_handle(cdoc);

    /* end of list, start again */
    if( ndoc == DOCHANDLE_NONE)
        ndoc = next_document_handle(ndoc);

    select_document_using_handle(ndoc);

    /* front even if current - may not be at front */
    #if RISCOS
    xf_frontmainwindow = TRUE;
    #elif MS
    front_doc = ndoc;
    xf_draweverything = TRUE;
    #endif
}

#endif


/************************
*                       *
*  tidy up memory use   *
*                       *
************************/

extern void
TidyUp_fn(void)
{
    dochandle cdoc = current_document_handle();
    dochandle ndoc = DOCHANDLE_NONE;

    while((ndoc = next_document_handle(ndoc)) != DOCHANDLE_NONE)
        {
        select_document_using_handle(ndoc);

        garbagecollect();

        #if RISCOS
        draw_tidy_up();
        #endif
        }

    select_document_using_handle(cdoc);

    #if !defined(SPELL_OFF)
    close_user_dictionaries();

    while(spell_freemem())
        ;
    #endif

    list_freepoolspace(-1L);

    #if RISCOS
    alloc_tidy_up();

    riscmenu_tidy_up();
    #endif
}

/* end of constr.c */
