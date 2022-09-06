/* eval.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/****************************************************
*                                                   *
* written by RJM                                    *
* May 1988 MRJC updated for new expression compiler *
* March 1989 MRJC tree calc added                   *
*                                                   *
****************************************************/

#include "datafmt.h"

/* local header file */
#include "eval.h"

#if RISCOS
#include "monotime.h"

#include "ext.riscos"
#endif


/* exported functions */

extern intl calshe(intl draw, intl state);
extern intl check_docvalid(docno doc);
extern void eval_slot(SYMB_TYPE *symb, intl check_tree);
extern void evasto(slotp tslot, colt tcol, rowt trow);
extern void evastoi(slotp tslot, colt tcol, rowt trow);
extern void exp_close_file(dochandle han);
extern void exp_eval(uchar *);
extern void exp_rename_file(void);
extern void growstack(void);
extern void inc_rows(uchar *expression);
extern intl init_dependent_docs(docno *doc, intl *index);
extern intl init_supporting_files(docno *doc, intl *index);
extern BOOL init_stack(void);
extern docno maybe_cur_docno(dochandle curhan);
extern docno next_dependent_doc(docno *doc, intl *index);
extern docno next_supporting_file(docno *doc, intl *index,
                                  dochandle *han, char **name);
extern SYMB_TYPE *next_in_range(BOOL goingdown,
                                BOOL single_line,
                                intl check_tree);
extern void pderror(intl number);
extern SYMB_TYPE *poparg(intl type_filter);
extern void pusharg(SYMB_TYPE *arg);
extern intl read_docname(char *str, docno *doc);
extern intl selrow(uchar *expression, rowt trow, BOOL increment_rows);
extern docno set_docno(dochandle han);
extern void setup_range(SYMB_TYPE *symb);
extern void switch_document(docno newdoc);
extern intl tree_build(void);
extern void tree_delete(void);
extern intl tree_exp_insertslot(colt col, rowt row, intl sort);
extern void tree_moveblock(colt scol, rowt srow, colt ecol, rowt erow);
extern void tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow);
extern void tree_removeslot(colt col, rowt row);
extern intl tree_str_insertslot(colt col, rowt row, intl sort);
extern void tree_swaprefs(rowt row1, rowt row2, colt scol, colt ecol);
extern void tree_switchoff(void);
extern void tree_updref(colt mrksco, rowt mrksro, colt mrkeco, rowt mrkero,
                 colt coldiff, rowt rowdiff);
extern BOOL true_condition(uchar *cond, BOOL increment_row);
extern intl write_docname(char *str, docno doc);


/* internal functions */

#if ARTHUR || RISCOS
static void jmp_capture(intl sig);
#elif MS
static void jmp_capture(intl sig, intl num);
#endif

/*static intl add_rngdependency(SLR *reftos, SLR *reftoe, SLR *by, intl sort);*/
/*static intl add_slrdependency(SLR *refto, SLR *by, intl sort);*/
static void bind_docs_to_handles(void);
/*static void change_doc(docno docto, docno docfrom);*/
/*static docno ensure_doc_in_list(char *name, dochandle han);*/
static void ensure_sheet_depends(docno docto, docno docby);
static docno find_doc_in_list(char *name, intl new_entry);
/*static intl flagerr(slotp sl, colt col, rowt row, intl errno);*/
static void free_doc(docno doc);
/*static void free_links(void);*/
/*static void inc_refs(uchar *expression, colt coldiff, rowt rowdiff);*/
static intl init_dependent_docs_given(docno *doc, intl *index);
static void init_doc_list(void);
/*static void iter_init(void);*/
static intl natural_calshe(intl state);
static intl natural_calshe_sub(char *linked_set, intl state);
static void prepare_for_recalc(void);
static intl rngcomp(const void *rng1, const void *rng2);
static intl rowcol_calshe(uchar order);
static intl search_for_slrdependent(SLR *refto);
static void set_handle(docno doc);
static intl slots_same_value(slotp sl1, slotp sl2, double *diff);
static intl slrcomp(const void *slr1, const void *slr2);
/*static void tree_build_sheet_links(void);*/
static intl tree_calc(colt col, rowt row);
static intl tree_calc_above(slotp sl, colt col, rowt row);
static void tree_calc_above_doc(docno doc);
static intl tree_calc_below(slotp sl, colt col, rowt row);
static intl tree_calcspecial(void);
/*static void tree_deletemarked(void);*/
static void tree_eval_slot(slotp sl, colt col, rowt row);
static void tree_mark_related_docs(dochandle han);
static void tree_rearrange(void);
static void tree_sheet_links_recurse(char *linked_set, char *linked_stop,
                                     docno curdoc);
static void tree_sort(void);
/* variables */

docno doc_index = 0;
colt col_index = 0;
colt range_col_1 = 0;
colt range_col_2 = 0; 
static colt colinc;

rowt row_index = 0;
rowt range_row_1 = 0; 
rowt range_row_2 = 0;
static rowt rowinc;

static SYMB_TYPE next_slot;

#define STACK_DEPTH_INC 10

intl stack_depth = 0;

SYMB_TYPE *stack     = NULL;    /* start of stack */
SYMB_TYPE *stack_ptr = NULL;    /* current pointer */
SYMB_TYPE *stack_one = NULL;    /* stack pointer with one argument */
SYMB_TYPE *stack_two = NULL;    /* stack pointer with two arguments */
SYMB_TYPE *stack_end = NULL;    /* end of stack */

intl errorflag = 0;             /* 0 = no error, n = error number */

#define NEST_LIMIT 20 
#define LIST_LIMIT 30

colt eval_col;                  /* col number of slot being evaluated */
rowt eval_row;                  /* row number of slot being evaluated */

static jmp_buf safepoint;

#define SLRBLKINC 300
#define RNGBLKINC 30

/* details of SLR block */
static mhandle slrblkh = 0;
static intl slrblkfree = 0;
static intl slrblksize = 0;
static uchar slrflags = 0;

/* details of range block */
static mhandle rngblkh = 0;
static intl rngblkfree = 0;
static intl rngblksize = 0;
static uchar rngflags = 0;

/* stack level check */
static void *stacklevel;

/* draw flag */
static intl drawflag;

/* total slots to calculate */
static word32 slotstocalc;

/* iteration flags */
static intl iterate;
static intl iterchangeflag;
static double iterchangeval;
static long iternumber;

long itercount;

/* corresponding array of pointers to leafnames */
uchar *nam_doc[MAX_DOCNO];

/* corresponding array of pointers to linked document list */
docno *lnk_doc[MAX_DOCNO];

/* corresponding array of numbers of linked documents */
uchar num_lnk[MAX_DOCNO];

/* corresponding array of document numbers */
docno num_doc[MAX_DOCNO];

/* corresponding array of document handles */
docno /*(dochandle)*/ han_doc[MAX_DOCNO];

/* corresponding array of error codes */
intl err_doc[MAX_DOCNO];

/* initialised flag */
static intl doc_inited = 0;

/* macro to get document name */
#define docname(han) num_doc[(han)] && nam_doc[num_doc[(han)]] \
                        ? trace_string(nam_doc[num_doc[(han)]]) \
                        : trace_string(find_leafname_using_handle((han)))

/* macro to print first 4 doc names */
#define trace6docs() tracef6("[1: %s, 2: %s, 3: %s, 4: %s, 5: %s, 6: %s]\n",\
                             trace_string(nam_doc[1]),\
                             trace_string(nam_doc[2]),\
                             trace_string(nam_doc[3]),\
                             trace_string(nam_doc[4]),\
                             trace_string(nam_doc[5]),\
                             trace_string(nam_doc[6]))

#if TRACE
static intl eval_level = 0;
#endif

/*****************************************************
*                                                    *
* add a dependency to the list of range dependencies *
*                                                    *
*****************************************************/

static intl
add_rngdependency(SLR *reftos, SLR *reftoe, SLR *by, intl sort)
{
    rngep rep;
    intl rix, res;
    mhandle newblkh;
    struct rngentry key;

    key.reftos_doc = reftos->doc;
    key.reftos_col = reftos->col;
    key.reftos_row = reftos->row;
    key.reftoe_doc = reftoe->doc;
    key.reftoe_col = reftoe->col;
    key.reftoe_row = reftoe->row;

    if(!rngblkh)
        {
        rngblkh = alloc_handle_using_cache((word32)
                                           (sizeof(struct rngentry) *
                                           RNGBLKINC));
        if(rngblkh <= 0)
            return(-1);

        rngblksize = RNGBLKINC;
        rngblkfree = rix = 0;
        tracef2("[range block size is now: %d, %d bytes]\n",
                rngblksize, sizeof(struct rngentry) * rngblksize);
        }
    else
        {
        if(sort)
            {
            rix = 0;
            rep = (rngep) list_getptr(rngblkh);

            while(rix < rngblkfree)
                {
                /* check for an identical entry */
                res = rngcomp(rep, &key);
                if(!res &&
                   rep->byslr_col == by->col &&
                   rep->byslr_row == by->row &&
                   rep->byslr_doc == by->doc
                  )
                    {
                    bicab(rep->entflags, SL_TOBEDEL);
                    return(1);
                    }

                if(res > 0)
                    break;

                ++rep;
                ++rix;
                }
            }
        else
            {
            rix = rngblkfree;
            rngflags |= SL_ALTERED;
            }
        }

    /* ensure we have space to add the dependency */
    if(rngblkfree + 1 >= rngblksize)
        {
        newblkh = realloc_handle_using_cache(rngblkh,
                                       (word32) (sizeof(struct rngentry) *
                                       (rngblksize + RNGBLKINC)));
        if(newblkh <= 0)
            return(-1);

        rngblkh = newblkh;
        rngblksize += RNGBLKINC;
        tracef2("[range block size is now: %d, %d bytes]\n",
                rngblksize, sizeof(struct rngentry) * rngblksize);
        }

    /* create a space to insert */
    rep = (rngep) list_getptr(rngblkh) + rix;
    memmove(rep + 1, rep, (rngblkfree - rix) * sizeof(struct rngentry));
    ++rngblkfree;

    /* copy in new dependency */
    rep->reftos_doc = reftos->doc;
    rep->reftos_col = reftos->col;
    rep->reftos_row = reftos->row;
    rep->reftoe_doc = reftoe->doc;
    rep->reftoe_col = reftoe->col;
    rep->reftoe_row = reftoe->row;
    rep->byslr_doc = by->doc;
    rep->byslr_col = by->col;
    rep->byslr_row = by->row;
    rep->entflags = 0;

    /* update sheet links */
    ensure_sheet_depends(rep->byslr_doc, rep->byslr_doc);
    ensure_sheet_depends(rep->reftos_doc, rep->byslr_doc);

    return(0);
}

/***************************************************
*                                                  *
* add a dependency to the list of slr dependencies *
*                                                  *
***************************************************/

static intl
add_slrdependency(SLR *refto, SLR *by, intl sort)
{
    slrep sep;
    intl six;
    mhandle newblkh;
    struct slrentry key;

    if(!slrblkh)
        {
        slrblkh = alloc_handle_using_cache((word32)
                                           (sizeof(struct slrentry) *
                                           SLRBLKINC));
        if(slrblkh <= 0)
            return(-1);

        slrblksize = SLRBLKINC;
        slrblkfree = 0;
        tracef2("[slr block size is now: %d, %d bytes]\n",
                slrblksize, sizeof(struct slrentry) * slrblksize); 
        }

    key.refto_doc = refto->doc;
    key.refto_col = refto->col;
    key.refto_row = refto->row;

    /* see if the reference is already there */
    if(sort)
        {
        if((six = search_for_slrdependent(refto)) >= 0)
            {
            sep = (slrep) list_getptr(slrblkh) + six;

            while(six < slrblkfree &&
                  !slrcomp(sep, &key))
                {
                if(sep->byslr_col == by->col &&
                   sep->byslr_row == by->row &&
                   sep->byslr_doc == by->doc
                  )
                    {
                    bicab(sep->entflags, SL_TOBEDEL);
                    return(1);
                    }

                ++sep;
                ++six;
                }
            }
        else
            {
            /* find a place to insert, tediously */
            for(six = 0, sep = list_getptr(slrblkh);
                six < slrblkfree;
                ++six, ++sep)
                if(slrcomp(sep, &key) > 0)
                    break;
            }
        }
    else
        {
        six = slrblkfree;
        slrflags |= SL_ALTERED;
        }

    /* ensure we have space to add the dependency */
    if(slrblkfree + 1 >= slrblksize)
        {
        newblkh = realloc_handle_using_cache(slrblkh,
                                       (word32) (sizeof(struct slrentry) *
                                       (slrblksize + SLRBLKINC)));
        if(newblkh <= 0)
            return(-1);

        slrblkh = newblkh;
        slrblksize += SLRBLKINC;
        tracef2("[slr block size is now: %d, %d bytes]\n",
                slrblksize, sizeof(struct slrentry) * slrblksize); 
        }

    /* create a space to insert */
    sep = (slrep) list_getptr(slrblkh) + six;
    memmove(sep + 1, sep, (slrblkfree - six) * sizeof(struct slrentry));
    ++slrblkfree;

    /* copy in new dependency */
    sep->refto_doc = refto->doc;
    sep->refto_col = refto->col;
    sep->refto_row = refto->row;
    sep->byslr_doc = by->doc;
    sep->byslr_col = by->col;
    sep->byslr_row = by->row;
    sep->entflags = 0;

    /* update sheet links */
    ensure_sheet_depends(sep->byslr_doc, sep->byslr_doc);
    ensure_sheet_depends(sep->refto_doc, sep->byslr_doc);

    return(0);
}

/************************************************************
*                                                           *
* calshe which selects the right routine for the calc order *
* and recalculates all sheets in memory which need it       *
*                                                           *
************************************************************/

extern intl
calshe(intl draw, intl state)
{
    intl res;
    dochandle curhan = current_document_handle();
    window_data *wdp = NULL;

    tracef0("[calshe]\r\n");

    if(state == CALC_RESTART)
        {
        tracef0("[** calshe restarted **]\r\n");
        tree_rearrange();
        bind_docs_to_handles();
        }

    /* loop thru all sheets */
    res = CALC_COMPLETED;

    while((wdp = next_document(wdp)) != NULL)
        {
        if(wdp->Xrecalc_forced || (wdp->Xrecalc_bit && wdp->Xrcobit))
            {
            tracef3("[calshe recalcing: %s, forced: %d, recalc_bit: %d]\n",
                    docname(wdp->DocHandle),
                    wdp->Xrecalc_forced,
                    wdp->Xrecalc_bit);
            select_document(wdp);

            drawflag = draw;

            switch(recalcorder())
                {
                case RECALC_ROWS:
                case RECALC_COLS:
                    res = rowcol_calshe(recalcorder());
                    break;

                case RECALC_NATURAL:
                default:
                    res = natural_calshe(state);
                    break;
                }

            switch(res)
                {
                case CALC_ABORTED:
                case CALC_TIMEOUT:
                case CALC_KEY:
                    select_document_using_handle(curhan);
                    tracef3("[recalc_bit for: %s is: %d, slotstocalc: %d]\r\n",
                            docname(sb->DocHandle), recalc_bit, slotstocalc);
                    return(res);
                default:
                    break;
                }
            }
        else
            tracef2("[recalc_bit for: %s is NULL, slotstocalc: %d]\r\n",
                    docname(wdp->DocHandle), slotstocalc);
        }

    select_document_using_handle(curhan);
    tracef3("[recalc_bit for: %s is: %d, slotstocalc: %d]\r\n",
            docname(sb->DocHandle), recalc_bit, slotstocalc);
    return(res);
}

/***************************************
*                                      *
* take the list of documents in memory *
* and find their handles               *
*                                      *
***************************************/

static void
bind_docs_to_handles(void)
{
    docno doc;

    if(!doc_inited)
        return;

    tracef0("[bind docs to handles]\n");

    for(doc = 0; doc < MAX_DOCNO; ++doc)
        if(nam_doc[doc])
            set_handle(doc);
}

/********************************************
*                                           *
* alter document numbers in sheets and tree *
* when a document number changes            *
*                                           *
********************************************/

static void
change_doc(docno docto, docno docfrom)
{
    dochandle curhan;
    docno doc;
    intl index, num_dep_docs, i;
    slotp tslot;
    uchar *rptr;
    docno dref;

    tracef2("[change_doc to: %d, from: %d]\n", docto, docfrom);

    curhan = current_document_handle();
    num_dep_docs = init_dependent_docs_given(&docfrom, &index);

    /* loop for each dependent sheet */
    do  {
        do  {
            if(num_dep_docs)
                {
                doc = next_dependent_doc(&docfrom, &index);
                --num_dep_docs;
                if(check_docvalid(doc) >= 0)
                    {
                    switch_document(doc);
                    break;
                    }
                }
            }
        while(num_dep_docs);

        tracef1("[change_doc searching: %s]\n",
                docname(current_document_handle()));

        /* loop for each linked spreadsheet */

        /* if no slot references is sheet, don't bother looking for them */
        if(refs_in_this_sheet)
            {
            /* for every slot in spreadsheet */
            init_doc_as_block();

            while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
                {
                /* slot got any SLRs ? */
                if(tslot->flags & SL_REFS)
                    {
                    tracef3("[change_doc found SLR in doc: %d, col: %d, row: %d]\n",
                            doc, in_block.col, in_block.row);

                    rptr = (tslot->type == SL_TEXT)
                                   ? tslot->content.text
                                   : tslot->content.number.text;

                    /* for each slot reference */
                    my_init_ref(tslot->type);

                    while((rptr = my_next_ref(rptr, tslot->type)) != NULL)
                        {
                        dref = (docno) talps(rptr, sizeof(docno));

                        tracef1("[change_doc dref: %d]\n", dref);

                        if(dref == docfrom)
                            splat(rptr, (word32) docto, sizeof(docno));

                        rptr += sizeof(docno) + sizeof(colt) + sizeof(rowt);
                        }
                    }
                }
            }
        }
    while(num_dep_docs);

    /* make sure we are back in original document */
    select_document_using_handle(curhan);

    /* update tree */
    if(slrblkh)
        {
        slrep sep;

        i = 0;
        sep = (slrep) list_getptr(slrblkh);
        while(i < slrblkfree)
            {
            if(sep->refto_doc == docfrom)
                sep->refto_doc = docto;
            if(sep->byslr_doc == docfrom)
                sep->byslr_doc = docto;
            ++i;
            ++sep;
            }

        slrflags |= SL_ALTERED;
        }

    if(rngblkh)
        {
        rngep rep;

        i = 0;
        rep = (rngep) list_getptr(rngblkh);
        while(i < rngblkfree)
            {
            if(rep->reftos_doc == docfrom)
                rep->reftos_doc = docto;
            if(rep->byslr_doc == docfrom)
                rep->byslr_doc = docto;
            ++i;
            ++rep;
            }

        rngflags |= SL_ALTERED;
        }

    tree_calc_above_doc(docto);
}

/********************************
*                               *
* check that document is loaded *
*                               *
********************************/

extern intl
check_docvalid(docno doc)
{
    /* reference to current document ? */
    if(!doc)
        return(1);

    if(han_doc[doc] == DOCHANDLE_NONE)
        return(ERR_BAD_SLOT);

    return(1);
}

/*******************************************************
*                                                      *
* given a document name, ensure that it is             *
* in the document list, and return its document number *
*                                                      *
*******************************************************/

static docno
ensure_doc_in_list(char *name, dochandle han)
{
    docno doc;
    intl new_entry;

    /* initialise arrays if necessary */
    if(!doc_inited)
        init_doc_list();

    /* check for valid name */
    if(!name)
        return(0);

    /* if we have the handle already, we must get a blank slot */
    new_entry = (han == DOCHANDLE_NONE) ? FALSE : TRUE;

    /* find an entry in the table */
    doc = find_doc_in_list(name, new_entry);
    if(!doc)
        return(DOCHANDLE_NONE);

    /* retrun if we already have a handle */
    if(han_doc[doc] != DOCHANDLE_NONE)
        {
        tracef1("[ensure_doc_in_list found existing document: %d]\n", doc);
        return(doc);
        }

    /* add new entry */
    if(!nam_doc[doc])
        {
        nam_doc[doc] = fixed_malloc(strlen(name) + 1);
        if(nam_doc[doc])
            strcpy(nam_doc[doc], name);
        }

    han_doc[doc] = han;
    err_doc[doc] = 0;

    if(han != DOCHANDLE_NONE)
        {
        num_doc[han] = doc;
        tree_calc_above_doc(doc);
        }

    set_handle(doc);

    tracef1("[ensure_doc_in_list allocated new document: %d]\n", doc);
    #ifdef TRACE
    trace6docs();
    #endif

    return(doc);
}

/**************************************************
*                                                 *
* given a reference from one document to another, *
* ensure that this dependency is recorded in the  *
* list of sheet dependencies                      *
*                                                 *
**************************************************/

static void
ensure_sheet_depends(docno docto, docno docby)
{
    intl i, num_docs;
    docno *linked_doc, *new_links;

    /* check if the dependency is there already */
    num_docs = num_lnk[docto];
    if(num_docs)
        for(i = 0, linked_doc = lnk_doc[docto];
            i < num_docs;
            ++i, ++linked_doc)
            if(*linked_doc == docby)
                return;

    tracef2("[ensure_sheet_depends docto: %d, docby %d]\r\n", 
            (intl) docto, (intl) docby);

    new_links = fixed_malloc(num_docs + 1);
    if(new_links)
        {
        memcpy(new_links, lnk_doc[docto], num_docs);
        new_links[num_docs] = docby;
        num_lnk[docto] += 1;
        fixed_dispose((void **) &lnk_doc[docto]);
        lnk_doc[docto] = new_links;
        }
}

/****************************************
*                                       *
* take a slot reference, and return the *
* resulting type                        *
*                                       *
****************************************/

extern void
eval_slot(SYMB_TYPE *symb, intl check_tree)
{
    slotp sl;
    intl err;
    dochandle curhan;

    if((doc_inited && (err = err_doc[symb->value.slot.doc]) != 0) ||
       (err = check_docvalid(symb->value.slot.doc)) < 0
      )
        {
        tracef3("[eval_slot %d, %d found error: %d]\n",
                symb->value.slot.col,
                symb->value.slot.row,
                err);
        symb->type = SL_ERROR;
        symb->value.symb = err;
        return;
        }

    curhan = current_document_handle();
    switch_document(symb->value.slot.doc);
    sl = travel(symb->value.slot.col, symb->value.slot.row);

    /* check that slots dependencies have been recalculated */
    if(check_tree && recalcorder() == RECALC_NATURAL)
        {
        if(sl)
            {
            /* create a new stack and flip to it */
            SYMB_TYPE *ostack     = stack;
            SYMB_TYPE *ostack_ptr = stack_ptr;
            SYMB_TYPE *ostack_one = stack_one;
            SYMB_TYPE *ostack_two = stack_two;
            intl ostack_depth     = stack_depth;
            colt oeval_col        = eval_col;
            rowt oeval_row        = eval_row;
            intl oerrorflag       = errorflag;
            jmp_buf osafepoint;

            if(init_stack())
                {
                memcpy(osafepoint, safepoint, sizeof(jmp_buf));
                tracef3("[eval_slot starting level: %d, slot: %d, %d]\n",
                        ++eval_level,
                        symb->value.slot.col,
                        symb->value.slot.row);

                tree_calc(symb->value.slot.col,
                          symb->value.slot.row);

                tracef3("[eval_slot completed level: %d, slot: %d, %d]\n",
                        eval_level--,
                        symb->value.slot.col,
                        symb->value.slot.row);
                
                fixed_free(stack);
                eval_col = oeval_col;
                eval_row = oeval_row;
                errorflag = oerrorflag;
                memcpy(safepoint, osafepoint, sizeof(jmp_buf));
                }
            else
                {
                select_document_using_handle(curhan);
                errorflag = EVAL_ERR_STACKOVER;
                return;
                }

            stack = ostack;
            stack_ptr = ostack_ptr;
            stack_one = ostack_one;
            stack_two = ostack_two;
            stack_depth = ostack_depth;
            }
        }

    select_document_using_handle(curhan);

    if(isslotblank(sl))
        {
        symb->type = SL_BLANK;
        return;
        }

    switch(sl->type)
        {
        case SL_STRVAL:
        case SL_INTSTR:
        case SL_TEXT:
            symb->type = SL_STRVAL;
            return;

        case SL_DATE:
            symb->value.date = sl->content.number.result.resdate;
            break;

        case SL_NUMBER:
            symb->value.num = sl->content.number.result.value;
            break;

        case SL_BAD_FORMULA:
        case SL_ERROR:
            /* return blank if self-reference to error */
            tracef4("[evastoi error: symb slot: %d, %d; eval slot: %d, %d]\n",
                    symb->value.slot.col,
                    symb->value.slot.row,
                    eval_col, eval_row);

            if(!((symb->value.slot.col == eval_col) &&
                 (symb->value.slot.row == eval_row)))
                {
                symb->type = SL_ERROR;
                symb->value.symb = EVAL_ERR_PROPAGATED;
                return;
                }

        default:
            symb->type = SL_NUMBER;
            symb->value.num = 0.;
            return;
        }

    symb->type = sl->type;
}


/***********************************************
*                                              *
* set up the signal handler, then call evastoi *
*                                              *
***********************************************/

extern void
evasto(slotp tslot, colt tcol, rowt trow)
{
    void (*oldfpe)(int);
    void (*oldstak)(int);

    /* set up floating point error handler */
    oldfpe = signal(SIGFPE, jmp_capture);
    #if RISCOS
    oldstak = signal(SIGSTAK, jmp_capture);
    #endif

    evastoi(tslot, tcol, trow);

    /* restore old handlers */
    signal(SIGFPE, oldfpe);
    #if RISCOS
    signal(SIGSTAK, oldstak);
    #endif
}


/*********************************************************
*                                                        *
* evaluate the expression in a slot and store the result *
*                                                        *
*********************************************************/

extern void
evastoi(slotp tslot, colt tcol, rowt trow)
{
    SYMB_TYPE *symb;
    int jmpval;

    eval_col = tcol;            /* col number of slot being evaluated */
    eval_row = trow;            /* row number of slot being evaluated */

    errorflag = 0;
    stack_ptr = stack;

    if((jmpval = setjmp(safepoint)) != 0)
        {
        pderror(jmpval);
        tracef4("[evastoi got setjmp level: %d, slot: %d, %d, jmpval: %d]\n",
                eval_level, eval_col, eval_row, jmpval);
        }
    else
        exp_eval(tslot->content.number.text);

    if(stack_ptr != stack_one)
        {
        while(stack_ptr > stack)
            (void) poparg(POP_NORMAL);
        tslot->content.number.result.errno = ERR_BAD_EXPRESSION;
        tslot->type = SL_ERROR;
        }

    switch((symb = poparg(POP_BLANK_0))->type)
        {
        case SL_INTSTR:
            tslot->content.number.result.str_offset =
                         (intl) (symb->value.str.ptr -
                         tslot->content.number.text);
            tslot->type = SL_INTSTR;
            break;

        case SL_STRVAL:
            tslot->content.number.result.str_slot = symb->value.slot;
            tslot->type = SL_STRVAL;
            break;

        case SL_DATE:
            tslot->content.number.result.resdate = symb->value.date;
            tslot->type = SL_DATE;
            break;

        case SL_NUMBER:
            tslot->content.number.result.value = symb->value.num;
            tslot->type = SL_NUMBER;
            break;

        case SL_ERROR:
            tslot->content.number.result.errno = symb->value.symb;
            tslot->type = SL_ERROR;
            break;

        default:
            break;
        }
}

/*******************************************
*                                          *
* remove all traces of a file when it      *
* is closed and the handle becomes invalid *
*                                          *
*******************************************/

extern void
exp_close_file(dochandle han)
{
    docno doc;
    dochandle curhan;

    tracef1("[exp_close_file han: %d]\n", han);

    #if TRACE
    trace6docs();
    #endif

    if((doc = num_doc[han]) != 0)
        {
        docno idoc;

        curhan = current_document_handle();

        if(check_docvalid(doc) >= 0)
            {
            switch_document(doc);
            tree_delete();
            }

        select_document_using_handle(curhan);

        tree_rearrange();

        /* search for another occurrence of the name */
        for(idoc = 1; idoc < MAX_DOCNO; ++idoc)
            if(idoc != doc &&
               nam_doc[idoc] &&
               !stricmp(nam_doc[idoc], nam_doc[doc]))
                {
                tracef2("[exp_close_file refs to: %d, going to: %d]\n",
                        doc, idoc);
                change_doc(idoc, doc);
                free_doc(doc);
                break;
                }
            #if TRACE
            else
                tracef2("[exp_close_file doc: %d, nam: %s]\n",
                        idoc, trace_string(nam_doc[idoc]));
            #endif

        num_doc[han] = 0;
        han_doc[doc] = DOCHANDLE_NONE;
        tracef0("[exp_close_file found and zeroed handle]\n");
        tree_calc_above_doc(doc);
        }
    #if TRACE
    else
        tracef0("[exp_close_file found null handle]\n");
    #endif

    #if TRACE
    trace6docs();
    #endif

    /* mark documents with the same name to be checked */
    tree_mark_related_docs(han);
}

/**********************************
*                                 *
* evaluate compiled expression,   *
* leaving the result on the stack *
*                                 *
* the theme of this routine is    *
* SPEED                           *
*                                 *
**********************************/

extern void
exp_eval(uchar *exp)
{
    uchar *ep = exp;
    oprp curop;
    intl token;
    SYMB_TYPE *popa, *popb;
    SYMB_TYPE symbol;

    while((token = (intl) *ep++) != OPR_END)
        {
        curop = &oprtab[token];

        switch(curop->ftype)
            {
            case TYPE_CONST:
                {
                switch(token)
                    {
                    case OPR_REAL:
                        {
                        #if MS

                        symbol.value.num = *(((double *) ep)++);

                        #elif ARTHUR || RISCOS
                        union
                            {
                            double fpval;
                            uchar bytes[sizeof(double)];
                            } f;
                        uchar *b = f.bytes;

                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        symbol.value.num = f.fpval;
                        #endif

                        symbol.type = SL_NUMBER;
                        break;
                        }

                    case OPR_SLR:
                        {
                        #if MS

                        symbol.value.slot.doc = *(((docno *) ep)++);

                        symbol.value.slot.col = (*(((colt *) ep)++)) &
                                                (COLNOBITS | BADCOLBIT);
                        symbol.value.slot.row = (*(((rowt *) ep)++)) &
                                                (ROWNOBITS | BADROWBIT);

                        #elif ARTHUR || RISCOS
                        uchar *b;

                        symbol.value.slot.doc = *ep++;

                        /* move column across */
                        b = (uchar *) &symbol.value.slot.col;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++ & 
                               ((uchar) ((COLNOBITS | BADCOLBIT) >> 24));

                        /* move row across */
                        b = (uchar *) &symbol.value.slot.row;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++ &
                               ((uchar) ((ROWNOBITS | BADROWBIT) >> 24));
                        #endif

                        /* check for a bad reference */
                        if((symbol.value.slot.col & BADCOLBIT) ||
                           (symbol.value.slot.row & BADROWBIT))
                            {
                            symbol.type = SL_ERROR;
                            symbol.value.symb = ERR_BAD_EXPRESSION;
                            break;
                            }

                        symbol.type = SL_SLOTR;
                        break;
                        }

                    case OPR_RANGE:
                        {
                        #if MS

                        symbol.value.range.first.doc =
                        symbol.value.range.second.doc =
                            *(((docno *) ep)++);

                        symbol.value.range.first.col =
                            (*(((colt *) ep)++)) & (COLNOBITS | BADCOLBIT);
                        symbol.value.range.first.row =
                            (*(((rowt *) ep)++)) & (ROWNOBITS | BADROWBIT);

                        ((docno *) ep)++;

                        symbol.value.range.second.col =
                            (*(((colt *) ep)++)) & (COLNOBITS | BADCOLBIT);
                        symbol.value.range.second.row =
                            (*(((rowt *) ep)++)) & (ROWNOBITS | BADROWBIT);

                        #elif ARTHUR || RISCOS
                        uchar *b;

                        symbol.value.range.first.doc =
                        symbol.value.range.second.doc = *ep++;

                        /* move column across */
                        b = (uchar *) &symbol.value.range.first.col;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++ &
                               ((uchar) ((COLNOBITS | BADCOLBIT) >> 24));

                        /* move row across */
                        b = (uchar *) &symbol.value.range.first.row;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++ &
                               ((uchar) ((ROWNOBITS | BADROWBIT) >> 24));

                        ep++;

                        /* move column across */
                        b = (uchar *) &symbol.value.range.second.col;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++ &
                               ((uchar) ((COLNOBITS | BADCOLBIT) >> 24));

                        /* move row across */
                        b = (uchar *) &symbol.value.range.second.row;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++;
                        *b++ = *ep++ &
                               ((uchar) ((ROWNOBITS | BADROWBIT) >> 24));

                        #endif

                        /* check for a bad reference */
                        if((symbol.value.range.first.col & BADCOLBIT) ||
                           (symbol.value.range.first.row & BADROWBIT) ||
                           (symbol.value.range.second.col & BADCOLBIT) ||
                           (symbol.value.range.second.row & BADROWBIT))
                            {
                            symbol.type = SL_ERROR;
                            symbol.value.symb = ERR_BAD_EXPRESSION;
                            break;
                            }

                        symbol.type = SL_RANGE;
                        break;
                        }

                    case OPR_WORD16:
                        {
                        #if MS

                        symbol.value.num = (double) *(((word16 *) ep)++);

                        #elif ARTHUR || RISCOS
                        union
                            {
                            word16 ival;
                            uchar bytes[sizeof(word16)];
                            } i;
                        uchar *b = i.bytes;

                        *b++ = *ep++;
                        *b++ = *ep++;
                        symbol.value.num = (double) i.ival;
                        #endif

                        symbol.type = SL_NUMBER;
                        break;
                        }

                    case OPR_WORD8:
                        {
                        symbol.value.num = (double) *ep++;
                        symbol.type = SL_NUMBER;
                        break;
                        }

                    case OPR_STRINGD:
                    case OPR_STRINGS:
                        {
                        symbol.value.str.ptr = ep;
                        while(*ep)
                            if(*ep++ == OPR_SLR)
                                ep += SLRSIZE - 1;

                        symbol.value.str.length = ep - symbol.value.str.ptr;
                        symbol.type = SL_INTSTR;
                        ++ep;
                        break;
                        }

                    case OPR_DATE:
                        {
                        union
                            {
                            time_t date;
                            uchar bytes[sizeof(time_t)];
                            } d;
                        uchar *b = d.bytes;
                        intl i;

                        for(i = 0; i < sizeof(time_t); ++i)
                            *b++ = *ep++;

                        symbol.value.date = d.date;
                        symbol.type = SL_DATE;
                        break;
                        }
                    }

                pusharg(&symbol);
                break;
                }

            case TYPE_BINARYREL:
                {
                /* deal with blanks, making them 0 */
                popa = poparg(POP_BLANK_0);
                popb = poparg(POP_BLANK_0);

                if(errorflag)
                    {
                    pderror((intl) errorflag);
                    break;
                    }

                /* call semantic routine */
                (*curop->semaddr)(popa, popb);

                /* all relational operators return numbers */
                popa->type = SL_NUMBER;

                pusharg(popa);
                break;
                }

            case TYPE_BINARY:
                {
                /* special code to handle the most common
                case: two doubles on the stack */
                popa = poparg(POP_BLANK_0);
                popb = poparg(POP_BLANK_0);

                if(errorflag)
                    {
                    pderror((intl) errorflag);
                    break;
                    }

                /* special case for binaries with two doubles */
                if((popa->type == SL_NUMBER) && (popb->type == SL_NUMBER))
                    {
                    switch(token)
                        {
                        case OPR_PLUS:
                            plusab(popb->value.num, popa->value.num);
                            stack_ptr = popa;
                            goto endwhile;

                        case OPR_MINUS:
                            minusab(popb->value.num, popa->value.num);
                            stack_ptr = popa;
                            goto endwhile;

                        case OPR_TIMES:
                            timesab(popb->value.num, popa->value.num);
                            stack_ptr = popa;
                            goto endwhile;

                        case OPR_DIVIDE:
                            /* check for divide by zero */
                            if(popa->value.num == 0.)
                                break;
                            popb->value.num /= popa->value.num;
                            stack_ptr = popa;
                            goto endwhile;

                        default:
                            break;
                        }
                    }

                switch(popb->type)
                    {
                    case SL_INTSTR:
                    case SL_STRVAL:
                        pusharg(popb);
                        goto endwhile;

                    case SL_DATE:
                        switch(token)
                            {
                            case OPR_PLUS:
                            case OPR_MINUS:
                                break;
                            default:
                                pderror(EVAL_ERR_MIXED_TYPES);
                                goto endwhile;
                            }
                        break;

                    default:
                        break;
                    }

                switch(popa->type)
                    {
                    case SL_INTSTR:
                    case SL_STRVAL:
                        pusharg(popa);
                        goto endwhile;

                    case SL_DATE:
                        switch(token)
                            {
                            case OPR_PLUS:
                                break;

                            case OPR_MINUS:
                                if(popb->type == SL_DATE)
                                    break;

                            default:
                                pderror(EVAL_ERR_MIXED_TYPES);
                                goto endwhile;
                            }
                        break;

                    default:
                        break;
                    }

                /* call semantic routine */
                (*curop->semaddr)(popa, popb);

                pusharg(popa);
                break;
                }

            case TYPE_FUNC:
                {
                /* set argument count */
                switch(curop->nargs)
                    {
                    case 0:
                        popa = &symbol;
                        break;

                    /* only one argument */
                    case 1:
                        {
                        /* pop argument */
                        popa = poparg(POP_NORMAL);

                        if(errorflag)
                            {
                            pderror(errorflag);
                            goto endwhile;
                            }
                        break;
                        }

                    /* variable number of arguments */
                    case -1:
                        symbol.value.symb = *ep++;
                        symbol.type = SL_LIST;
                        popa = &symbol;
                        break;

                    /* fixed number of arguments */
                    default:
                        symbol.value.symb = curop->nargs;
                        symbol.type = SL_LIST;
                        popa = &symbol;
                        break;
                    }

                (*curop->semaddr)(popa);

                if(errorflag)
                    {
                    pderror(errorflag);
                    break;
                    }

                pusharg(popa);
                break;
                }

            /* a unary operator */
            case TYPE_UNARY:
                {
                /* deal with blanks, making them 0 */
                popa = poparg(POP_BLANK_0);

                if(errorflag)
                    {
                    pderror(errorflag);
                    break;
                    }

                switch(popa->type)
                    {
                    case SL_INTSTR:
                    case SL_STRVAL:
                        pusharg(popa);
                        goto endwhile;

                    case SL_DATE:
                        pderror(EVAL_ERR_MIXED_TYPES);
                        goto endwhile;

                    default:
                        break;
                    }

                (*curop->semaddr)(popa);

                pusharg(popa);
                break;
                }

            /* just ignore the brackets token */
            case TYPE_BRACKETS:
                break;
            }

    endwhile:

        continue;
        } /* [end of while loop] */
}

/********************************
*                               *
* update sheet link information *
* when a file is renamed        *
*                               *
********************************/

extern void
exp_rename_file(void)
{
    docno curdoc, doc;
    char *newname;

    #if TRACE
    trace6docs();
    #endif

    /* check if document is referenced in table */
    if((curdoc = maybe_cur_docno(DOCHANDLE_NONE)) != DOCNO_NOENTRY)
        {
        tracef1("[exp_rename_file docno: %d]\r\n", (intl) curdoc);
        newname = find_leafname_using_handle(current_document_handle());

        /* check for same name */
        if(nam_doc[curdoc]  &&  !stricmp(newname, nam_doc[curdoc]))
            return;

        fixed_dispose((void **) &nam_doc[curdoc]);

        /* are there unresolved references with this name ? */
        doc = find_doc_in_list(newname, FALSE);
        if(doc  &&  (han_doc[doc] == DOCHANDLE_NONE))
            {
            change_doc(curdoc, doc);
            free_doc(doc);
            }

        /* set new name and reset document handle */
        nam_doc[curdoc] = fixed_malloc(strlen(newname) + 1);
        if(nam_doc[curdoc])
            strcpy(nam_doc[curdoc], newname);

        set_handle(curdoc);
        }

    #if TRACE
    trace6docs();
    #endif

    tree_mark_related_docs(current_document_handle());
}

/********************************
*                               *
* search document list for name *
*                               *
********************************/

static docno
find_doc_in_list(char *name, intl new_entry)
{
    docno doc, new;

    /* search for name */
    for(doc = 1, new = 0; doc < MAX_DOCNO; ++doc)
        if(nam_doc[doc])
            {
            /* compare names */
            if(!stricmp(nam_doc[doc], name))
                {
                /* if we don't need a new entry, return this one */
                if(!new_entry)
                    return(doc);

                /* make sure the entry is not used */
                if(han_doc[doc] == DOCHANDLE_NONE)
                    return(doc);
                }
            }
        else if(!new)
            new = doc;

    return(new ? new : 0);
}

/**************************
*                         *
* flag an error in a slot *
*                         *
**************************/

static intl
flagerr(slotp sl, colt col, rowt row, intl errno)
{
    if(sl->flags & SL_MUSTRECALC)
        {
        --slotstocalc;
        bicab(sl->flags, SL_MUSTRECALC);
        }

    orab(sl->flags, SL_ALTERED | SL_RECALCED | SL_VISITED);
    sl->type = SL_ERROR;
    sl->content.number.result.errno = errno;
    errorflag = errno;
    xf_drawsome = TRUE;

    if(drawflag == CALC_DRAW)
        draw_one_altered_slot(col, row);

    graph_send_slot(col, row);

    tracef1("[flagerr: %d]\n", errno);
    return(-1);
}

/*****************************************************
*                                                    *
* graunchy jumpy errors get sent here.               *
* Clear stack, stick on error and jump to safe-point *
*                                                    *
*****************************************************/

#if RISCOS
#pragma -s1
#endif

static void
#if ARTHUR || RISCOS
jmp_capture(intl sig)
#else
jmp_capture(intl sig, intl num) /* MS library passes more info */
#endif
{
    int ret_error;

    signal(sig, jmp_capture);   /* reset signal trapping */

    #if MS
    _fpreset();                 /* reset floating point */
    #endif

    #if TRACE
    if(sig != SIGSTAK)
        tracef3("[jmp_capture caught level: %d, slot: %d, %d]\n",
                eval_level, eval_col, eval_row);
    #endif

    stack_ptr = stack;

    switch(sig)
        {
        #if RISCOS
        case SIGSTAK:
            ret_error = (int) EVAL_ERR_STACKOVER;
            break;
        #endif

    /*  case SIGFPE: */
        default:
            ret_error = (int) EVAL_ERR_FPERROR;
            break;
        }

    longjmp(safepoint, ret_error);
}

#if RISCOS
#pragma -s0
#endif


/***********************************
*                                  *
* remove a document from the table *
* and free its resources           *
*                                  *
***********************************/

static void
free_doc(docno doc)
{
    tracef2("[free_doc name: %s, doc: %d]\r\n", nam_doc[doc], (intl) doc);

    if(han_doc[doc])
        {
        num_doc[han_doc[doc]] = 0;
        han_doc[doc] = DOCHANDLE_NONE;
        }

    fixed_dispose((void **) &nam_doc[doc]);
    fixed_dispose((void **) &lnk_doc[doc]);

    num_lnk[doc] = 0;
    err_doc[doc] = 0;
}

/*****************************************************************
*                                                                *
* growstack finds space for STACK_DEPTH_INC more elements on the *
* argument stack. This shouldn't fall over when realloc fails.   *
* It should either use fixed size stack or not expand stack and  *
* report stack overflow                                          *
*                                                                *
*****************************************************************/

extern void
growstack(void)
{
    intl offset = stack_ptr - stack;
    SYMB_TYPE *ptr;

    /* trap any floating point errors */

    stack_depth += STACK_DEPTH_INC;

    ptr = fixed_realloc(stack, stack_depth * sizeof(SYMB_TYPE));

    /* if no room for new array, unwind the stack, put on stack overflow
     * and longjump to safety
    */
    if(!ptr)
        {
        stack_ptr = stack;
        longjmp(safepoint, (int) EVAL_ERR_STACKOVER);
        }
    else
        stack = ptr;

    /* set key addresses */
    stack_ptr = stack + offset;
    stack_one = stack + 1;
    stack_two = stack_one + 1;
    stack_end = stack + stack_depth;
}

/**********************************************
*                                             *
* free the document links ready for a rebuild *
*                                             *
**********************************************/

static void
free_links(void)
{
    intl i;

    /* clear list of links */
    for(i = 0; i < MAX_DOCNO; ++i)
        {
        /* free memory used by list of links */
        if(lnk_doc[i])
            {
            fixed_free(lnk_doc[i]);
            lnk_doc[i] = NULL;
            }

        num_lnk[i] = 0;
        }
}

/******************************************
*                                         *
* increment slot references in expression *
*                                         *
******************************************/

static void
inc_refs(uchar *expression, colt coldiff, rowt rowdiff)
{
    uchar *ptr = expression;
    colt tcol;
    rowt trow;

    exp_initslr();

    while((ptr = exp_findslr(ptr)) != NULL)
        {
        ptr += sizeof(docno);

        if(coldiff)
            {
            tcol = (colt) talps(ptr, sizeof(colt));
            if(!abs_col(tcol))
                splat(ptr, (word32) (tcol + coldiff), sizeof(colt));
            }
        ptr += sizeof(colt);

        if(rowdiff)
            {
            trow = talps(ptr, sizeof(rowt));
            if(!abs_row(trow))
                splat(ptr, (word32) (trow + rowdiff), sizeof(rowt));
            }
        ptr += sizeof(rowt);
        }
}

/**************************************
*                                     *
* increment row numbers in expression *
*                                     *
**************************************/

extern void
inc_rows(uchar *expression)
{
    inc_refs(expression, (colt) 0, (rowt) 1);
}

/*********************************************
*                                            *
* initialise the dependent document sequence *
* for the current document                   *
*                                            *
*********************************************/

extern intl
init_dependent_docs(docno *doc, intl *index)
{
    if((*doc = maybe_cur_docno(DOCHANDLE_NONE)) == DOCNO_NOENTRY)
        return(0);

    return(init_dependent_docs_given(doc, index));
}

/*********************************************
*                                            *
* initialise the dependent document sequence *
* for the given document                     *
*                                            *
*********************************************/

static intl
init_dependent_docs_given(docno *doc, intl *index)
{
    tracef1("[init_dep_docs_given doc: %d]\n", *doc);

    *index = 0;
    if(lnk_doc[*doc])
        {
        tracef2("[init_dep_docs_given doc: %d, links: %d]\n",
                *doc, num_lnk[*doc]);
        return(num_lnk[*doc]);
        }

    return(0);
}

/***************************
*                          *
* initialise document list *
*                          *
***************************/

static void
init_doc_list(void)
{
    intl i;

    for(i = 0; i < MAX_DOCNO; ++i)
        {
        nam_doc[i] = NULL;
        lnk_doc[i] = NULL;
        num_lnk[i] = 0;
        num_doc[i] = 0;
        err_doc[i] = 0;
        han_doc[i] = DOCHANDLE_NONE;
        }

    doc_inited = 1;
}

/****************************************
*                                       *
* initialise the sequence of supporting *
* files for the current file            *
*                                       *
* --in--                                *
* if doc is NULL, returns all files     *
* which are supporting files            *
*                                       *
****************************************/

extern intl
init_supporting_files(docno *doc, intl *index)
{
    if(doc && ((*doc = maybe_cur_docno(DOCHANDLE_NONE)) == DOCNO_NOENTRY))
        return(0);

    tree_rearrange();
    bind_docs_to_handles();
    *index = 1;
    return(1);
}

/***********************
*                      *
* initialize stack and *
* return success flag  *
*                      *
***********************/

extern BOOL
init_stack(void)
{
    stack_depth = STACK_DEPTH_INC;
    stack = fixed_malloc(stack_depth * sizeof(SYMB_TYPE));
    if(!stack)
        return(FALSE);

    /* set key addresses */
    stack_ptr = stack;
    stack_one = stack + 1;
    stack_two = stack_one + 1;
    stack_end = stack + stack_depth;
    return(TRUE);
}

/**********************************
*                                 *
* initialise values for iteration *
*                                 *
**********************************/

static void
iter_init(void)
{
    /* set iteration values */
    if((iterate = iteration()) != 0)
        {
        iternumber = (iterations_number() < 0)
                            ? 1l
                            : iterations_number();
        iterchangeval = (iterations_change() < 0.)
                            ? .001
                            : iterations_change();
        itercount = 0;
        }
    else
        iternumber = itercount = 0;
}

/*********************************************
*                                            *
* return the current document number if the  *
* document is in the table, otherwise return *
* a number that won't match a valid document *
* number                                     *
*                                            *
*********************************************/

extern docno
maybe_cur_docno(dochandle curhan)
{
    if(!doc_inited)
        return(DOCNO_NOENTRY);

    curhan = (curhan == DOCHANDLE_NONE) ? current_document_handle() : curhan;
    if(num_doc[curhan])
        return(num_doc[curhan]);

    return(DOCNO_NOENTRY);
}

/*****************************************
*                                        *
* calshe that does natural recalculation *
*                                        *
*****************************************/

static intl
natural_calshe(intl state)
{
    static char linked_set[MAX_DOCNO];
    static dochandle curhan;
    char linked_stop[MAX_DOCNO];
    docno i;
    intl res;
    void (*oldfpe)(int);
    void (*oldstak)(int);

    /* detect change of document and prepare anew */
    if(state == CALC_CONTINUE && curhan != current_document_handle())
        state = CALC_RESTART;

    /* save current document */
    curhan = current_document_handle();

    #if TRACE
    trace6docs();
    #endif

    /* work out linked set of sheets */
    if(state == CALC_RESTART)
        {
        docno curdoc;

        /* clear linked set */
        for(i = 0; i < MAX_DOCNO; ++i)
            linked_set[i] = linked_stop[i] = 0;

        /* build linked set */
        if((curdoc = maybe_cur_docno(DOCHANDLE_NONE)) != DOCHANDLE_NONE)
            tree_sheet_links_recurse(linked_set, linked_stop, curdoc);
        }

    /* set error handlers */
    oldfpe = signal(SIGFPE, jmp_capture);
    #if RISCOS
    oldstak = signal(SIGSTAK, jmp_capture);
    #endif

    res = natural_calshe_sub(linked_set, state);

    switch(res)
        {
        case CALC_ABORTED:
        case CALC_COMPLETED:
            /* clear recalc bits of completed sheets */
            for(i = 0; i < MAX_DOCNO; ++i)
                {
                if(i)
                    {
                    if(!linked_set[i])
                        continue;

                    if(check_docvalid(i) >= 0 &&
                       (i != maybe_cur_docno(curhan)))
                        {
                        switch_document(i);
                        recalc_bit = global_recalc = recalc_forced = FALSE;
                        tracef1("[clearing recalc_bit for: %s]\r\n",
                                docname(sb->DocHandle));
                        }
                    }
                else
                    {
                    select_document_using_handle(curhan);
                    recalc_bit = global_recalc = recalc_forced = FALSE;
                    tracef1("[clearing recalc_bit for: %s]\r\n",
                            docname(sb->DocHandle));
                    }
                }

            #if MS || ARTHUR
            ack_esc();
            #endif

            break;

        case CALC_TIMEOUT:
        case CALC_KEY:
        default:
            break;
        }

    actind_end();

    /* restore old handlers */
    signal(SIGFPE, oldfpe);
    #if RISCOS
    signal(SIGSTAK, oldstak);
    #endif

    return(res);
}


/**********************************
*                                 *
* calshe subroutine for structure *
*                                 *
**********************************/

static intl
natural_calshe_sub(char *linked_set, intl state)
{
    static docno i, doc_block = MAX_DOCNO;
    static SLR curpos;
    long startslots;
    intl timer;
    slotp sl;
    dochandle curhan;

    #if RISCOS
    monotime_t started;
    #endif

    #if TRACE
    eval_level = 0;
    #endif

    /* load current document handle */
    curhan = current_document_handle();

    /* load iteration values */
    iter_init();

    /* loop for each iteration */
    do  {
        iterchangeflag = 0;

        /* prepare each sheet for recalculation */
        if(state == CALC_RESTART)
            {
            for(slotstocalc = 0, i = 0; i < MAX_DOCNO; ++i)
                {
                if(i)
                    {
                    if(!linked_set[i])
                        continue;

                    if(check_docvalid(i) >= 0 && (i != maybe_cur_docno(curhan)))
                        {
                        switch_document(i);
                        prepare_for_recalc();
                        tree_calcspecial();
                        }
                    }
                else
                    {
                    /* prepare current document */
                    select_document_using_handle(curhan);
                    prepare_for_recalc();
                    tree_calcspecial();
                    }
                }

            /* clear range bits */
            if(rngblkh)
                {
                rngep rep;
                intl ix;

                for(rep = list_getptr(rngblkh), ix = 0;
                    ix < rngblkfree;
                    ++ix, ++rep)
                        bicab(rep->entflags, SL_VISITED | SL_RECALCED);
                }
            }

        /* loop through sheets while there
        are any slots to be recalculated */
        while(slotstocalc)
            {
            if(state == CALC_RESTART)
                i = 0;

            while(i < MAX_DOCNO && slotstocalc)
                {
                if(i && (i != maybe_cur_docno(curhan)))
                    {
                    if(!linked_set[i] || check_docvalid(i) < 0)
                        {
                        ++i;
                        continue;
                        }

                    switch_document(i);
                    }
                else
                    /* do current document on index of 0 */
                    select_document_using_handle(curhan);

                tracef2("[nat_cal_sub doing: %s, slotstocalc: %d]\r\n",
                        docname(curhan), (intl) slotstocalc);

                timer = 0;
                stacklevel = &curhan;
                startslots = slotstocalc;

                #if RISCOS
                started = monotime();
                #endif

                /* reset block if new document */
                if(i != doc_block || state == CALC_RESTART)
                    {
                    init_doc_as_block();
                    doc_block = i;
                    }
                else
                    {
                    in_block = curpos;
                    start_block = TRUE;
                    }

                while(((sl = next_slot_in_block(DOWN_COLUMNS)) != NULL) &&
                      slotstocalc)
                    {
                    timer += 5;
                    if(sl->flags & SL_MUSTRECALC)
                        {
                        tracef2("[nat_cal_sub tree_calc col: %d, row: %d]\r\n",
                                (intl) in_block.col, (intl) in_block.row);
                        tree_calc(in_block.col, in_block.row);

                        /* do activity indicator if switched on */
                        if(drawflag == CALC_ACT)
                            {
                            timer += 50;

                            if(timer > TIMER_DELAY * 100)
                                {
                                timer = 0;

                                startslots = max(startslots, slotstocalc);

                                if(!actind(ACT_CALCULAT,
                                    (intl) ((startslots - slotstocalc) *
                                                100l / startslots)))
                                    return(CALC_ABORTED);
                                }
                            }

                        #if MS
                        if(ctrlflag)
                            return(CALC_ABORTED);
                        #endif

                        #if MS || RISCOS
                        if( (drawflag != CALC_NOINTORDRAW)  &&
                            keyormouseinbuffer())
                            {
                            curpos = in_block;
                            return(CALC_KEY);
                            }
                        #endif

                        #if RISCOS
                        if( (drawflag != CALC_NOINTORDRAW)  &&
                            (monotime_diff(started) > RISCOS_BACKGROUND))
                            {
                            curpos = in_block;
                            return(CALC_TIMEOUT);
                            }
                        #endif
                        }
                    }

                /* next document */
                ++i;
                }
            i = 0;
            }

        --iternumber;
        ++itercount;
        state = CALC_RESTART;
        }
    while(iterate  &&  iternumber  &&  iterchangeflag);

    return(CALC_COMPLETED);
}


/******************************************************
*                                                     *
* return the next dependent file in the sequence;     *
* call init_dependent_file to initialise the sequence *
*                                                     *
******************************************************/

extern docno
next_dependent_doc(docno *doc, intl *index)
{
    if(*doc == DOCNO_NOENTRY)
        return(0);

    if(lnk_doc[*doc])
        {
        if(*index >= num_lnk[*doc])
            {
            tracef2("[next_dep_doc ended index: %d, links: %d]\n",
                    *index, num_lnk[*doc]);
            return(0);
            }

        tracef1("[next_dep_doc returning: %d]\n", lnk_doc[*doc][*index]);
        return(lnk_doc[*doc][(*index)++]);
        }

    tracef0("[next_dep_doc found no links]\n");

    return(0);
}

/**********************************************
*                                             *
* return the next supporting file in the list *
* of supporting files; init_supporting_files  *
* must be called to start the sequence        *
*                                             *
**********************************************/

extern docno
next_supporting_file(docno *doc, intl *index, dochandle *han, char **name)
{
    if(doc && (*doc == DOCNO_NOENTRY))
        return(0);

    while(*index < MAX_DOCNO)
        {
        intl i, nl;
        docno *dp;

        if(nam_doc[*index])
            for(i = 0, nl = num_lnk[*index], dp = lnk_doc[*index];
                i < nl;
                ++i, ++dp)
                if((*dp == *doc) || (!doc && (*dp != *index)))
                    {
                    if(han)
                        *han = han_doc[*index];
                    if(name)
                        *name = nam_doc[*index];
                    return((docno) (*index)++);
                    }

        ++(*index);
        }

    return(0);
}

/**********************************
*                                 *
* return the next slot in a range *
*                                 *
**********************************/

extern SYMB_TYPE *
next_in_range(BOOL goingdown, BOOL single_line, intl check_tree)
{
    if(goingdown)
        {
        if(single_line && col_index != range_col_1)
            return(NULL);
        if(col_index > range_col_2)
            return(NULL);
        }
    else
        {
        if(single_line && row_index > range_row_1)
            return(NULL);
        if(row_index > range_row_2)
            return(NULL);
        }

    next_slot.value.slot.doc = doc_index;
    next_slot.value.slot.col = col_index;
    next_slot.value.slot.row = row_index;

    eval_slot(&next_slot, check_tree);

    if(goingdown)
        {
        colinc = 0;
        rowinc = 1;
        if(++row_index > range_row_2)
            {
            rowinc = range_row_1 - row_index + 1;
            colinc = 1;
            row_index = range_row_1;
            col_index++;
            }
        }
    else
        {
        colinc = 1;
        rowinc = 0;
        if(++col_index > range_col_2)
            {
            colinc = range_col_1 - col_index + 1;
            rowinc = 1;
            col_index = range_col_1;
            row_index++;
            }
        }

    return(&next_slot);
}

/*****************************************
*                                        *
* pderror pushes an error onto the stack *
*                                        *
*****************************************/

extern void
pderror(intl number)
{
    SYMB_TYPE symbol;

    symbol.type = SL_ERROR;
    symbol.value.symb = number;
    errorflag = 0;
    pusharg(&symbol);
}

/**************************************************************************
*                                                                         *
* poparg pops an argument from the stack and returns a pointer to it      *
* A range argument in most cases means "sum the values in the slots", but *
* in some functions (max, min, lookup, ...) means "replace the range with *
* a succession of slots". Consequently the caller of poparg must specify  *
* whether ranges are to be passed back as a total or a range              *
*                                                                         *
**************************************************************************/

extern SYMB_TYPE *
poparg(intl type_filter)
{
    if(--stack_ptr < stack)
        {
        errorflag = ERR_BAD_EXPRESSION;
        stack_ptr = stack;
        pusharg(NULL);
        }

    /* loop for SLOTR case */
    while(TRUE)
        {
        switch(stack_ptr->type)
            {
            case SL_SLOTR:
                if(!(type_filter & KEEP_SLR))
                    {
                    eval_slot(stack_ptr, FALSE);
                    continue;
                    }
                else
                    goto endpop;

            case SL_BLANK:
                if(type_filter & POP_BLANK_0)
                    {
                    stack_ptr->type = SL_NUMBER;
                    stack_ptr->value.num = 0.;
                    }
                goto endpop;

            case SL_ERROR:
                errorflag = stack_ptr->value.symb;
                goto endpop;

            case SL_RANGE:
                if(!(type_filter & NO_SUM_RANGE))
                    {
                    SYMB_TYPE *apop;

                    setup_range(stack_ptr);
                    stack_ptr->type = SL_NUMBER;
                    stack_ptr->value.num = 0.;

                    while((apop = next_in_range(TRUE, FALSE, FALSE)) != NULL)
                        {
                        /* account for a blank */
                        if(apop->type == SL_BLANK)
                            {
                            apop->type = SL_NUMBER;
                            apop->value.num = 0.;
                            }

                        /* make numbers efficient */
                        if((apop->type == SL_NUMBER) && (stack_ptr->type == SL_NUMBER))
                            {
                            plusab(stack_ptr->value.num, apop->value.num);
                            continue;
                            }

                        c_add(stack_ptr, apop);
                        }
                    }
                goto endpop;

            default:
                goto endpop;
            }
        }

    endpop:

    return(stack_ptr);
}

/****************************************************************************
*                                                                           *
* pusharg pushes an argument (of type SYMB_TYPE) onto the argument stack    *
* The stack is a dynamically defined array of SYMB_TYPE structures.         *
* It starts off at 0 deep and when it becomes full gets copied to new array *
* STACK_DEPTH_INC deeper.                                                   *
*                                                                           *
****************************************************************************/

extern void
pusharg(SYMB_TYPE *arg)
{
    /* check valid date */
    if(arg->type == SL_DATE  &&  localtime(&(arg->value.date)) == NULL)
        {
        arg->type = SL_ERROR;
        arg->value.symb = EVAL_ERR_BAD_DATE;
        }

    if(errorflag)
        {
        pderror((intl) errorflag);
        return;
        }

    if(stack_ptr == stack_end)
        growstack();

    *stack_ptr++ = *arg;
}

/******************************************
*                                         *
* read document name and return document  *
* number - creating entry in the document *
* list if there is not one there already  *
*                                         *
* --out--                                 *
* number of characters read inc []        *
*                                         *
******************************************/

extern intl
read_docname(char *str, docno *doc)
{
    char tstr[EXT_REF_NAMLEN + 1];
    intl count;

    /* default if nothing read */
    *doc = 0;

    /* read in name to temporary buffer */
    while(*str == ' ')
        ++str;

    if(*str++ != '[')
        return(0);

    for(count = 0;
        *str && (*str != ']') && (count < EXT_REF_NAMLEN);
        ++count, ++str)
        tstr[count] = *str;

    if(!count || *str != ']')
        return(0);

    tstr[count++] = '\0';

    /* make sure the document is in the list,
    and get a document number for it */
    *doc = ensure_doc_in_list(tstr, DOCHANDLE_NONE);

    return(count + 1);
}

/********************************************
*                                           *
* prepare for a tree recalculation by       *
* counting the total slots to be calculated *
* in a group of linked sheets, and clearing *
* the visited/recalced bits                 *
*                                           *
********************************************/

static void
prepare_for_recalc(void)
{
    slotp sl;

    tracef1("[prepare_for_recalc: %s in]\r\n",
            docname(current_document_handle()));

    init_doc_as_block();

    while((sl = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        bicab(sl->flags, SL_VISITED | SL_CIRC | SL_RECALCED);

        /* copy iterate bit to mustrecalc */
        if(sl->flags & SL_ITERATE)
            {
            orab(sl->flags, SL_MUSTRECALC);

            if(!iterate)
                bicab(sl->flags, SL_ITERATE);
            }

        if(sl->type == SL_STRVAL)
            /* string values must be recalced each time */
            orab(sl->flags, SL_MUSTRECALC);
        elif(sl->type == SL_ERROR)
            {
            /* always recalculate certain errors */

            if( ( sl->content.number.result.errno == EVAL_ERR_PROPAGATED)           ||
                ((sl->content.number.result.errno == EVAL_ERR_CIRC)  &&  iterate)   )
                {
                sl->type = SL_NUMBER;
                orab(sl->flags, SL_MUSTRECALC);
                sl->content.number.result.value = 0.;
                }
            }

        /* count total slots to be calculated */
        if(sl->flags & SL_MUSTRECALC)
            ++slotstocalc;
        }

    tracef0("[prepare_for_recalc out]\r\n");
}

/********************************************
*                                           *
* routine for bsearch to compare two ranges *
*                                           *
********************************************/

static intl
rngcomp(const void *rng1, const void *rng2)
{
    if(((rngep) rng1)->reftos_doc > ((rngep) rng2)->reftos_doc)
        return(1);
    if(((rngep) rng1)->reftos_doc < ((rngep) rng2)->reftos_doc)
        return(-1);

    if(((rngep) rng1)->reftos_col > ((rngep) rng2)->reftos_col)
        return(1);
    if(((rngep) rng1)->reftos_col < ((rngep) rng2)->reftos_col)
        return(-1);

    if(((rngep) rng1)->reftos_row > ((rngep) rng2)->reftos_row)
        return(1);
    if(((rngep) rng1)->reftos_row < ((rngep) rng2)->reftos_row)
        return(-1);

    if(((rngep) rng1)->reftoe_col > ((rngep) rng2)->reftoe_col)
        return(1);
    if(((rngep) rng1)->reftoe_col < ((rngep) rng2)->reftoe_col)
        return(-1);

    if(((rngep) rng1)->reftoe_row > ((rngep) rng2)->reftoe_row)
        return(1);
    if(((rngep) rng1)->reftoe_row < ((rngep) rng2)->reftoe_row)
        return(-1);

    return(0);
}

/****************************************************
*                                                   *
* calshe which does row and column type calculation *
*                                                   *
****************************************************/

static intl
rowcol_calshe(uchar order)
{
    slotp sl;
    BOOL direction = (order == RECALC_COLS);
    rowt timer = 0;
    intl returncode = CALC_COMPLETED;
    void (*oldfpe)(int);
    void (*oldstak)(int);

    /* set up floating point error handler */
    oldfpe = signal(SIGFPE, jmp_capture);
    #if RISCOS
    oldstak = signal(SIGSTAK, jmp_capture);
    #endif

    /* initialise iteration */
    iter_init();

    /* iteration loop */
    do  {
        iterchangeflag = 0;

        init_doc_as_block();

        while((sl = next_slot_in_block(direction)) != NULL)
            {
            /* worry about activity indicator.  Note that every
             * blank slot visited is worth one time increment, but
             * every actual slot many more. This copes with recalculating
             * across empty columns
            */
            if( (drawflag != CALC_NOINTORDRAW)  &&
                (timer++ > 100 * TIMER_DELAY))
                {
                timer = 0;

                if(!actind(ACT_CALCULAT, percent_in_block(direction)))
                    {
                    actind_end();
                    #if MS || ARTHUR
                    ack_esc();
                    #endif
                    reperr_null(ERR_ESCAPE);
                    recalc_bit = recalc_forced = FALSE;
                    returncode = CALC_ABORTED;
                    goto EXIT_POINT;
                    }
                }

            timer += 100;

            #if MS || RISCOS
            if(ctrlflag  ||  ((drawflag != CALC_NOINTORDRAW)  &&  keyormouseinbuffer()))
                {
                intl was_ctrlflag = ctrlflag;

                if(was_ctrlflag)
                    ack_esc();

                returncode = was_ctrlflag ? CALC_ABORTED : CALC_KEY;
                goto EXIT_POINT;
                }
            #endif

            if((sl->flags & SL_REFS)  ||  global_recalc)
                {
                struct slot valsave;

                if(iterate)
                    valsave = *sl;

                switch(sl->type)
                    {
                    case SL_DATE:
                    case SL_INTSTR:
                    case SL_STRVAL:
                    case SL_NUMBER:
                    case SL_ERROR:
                        evastoi(sl, in_block.col, in_block.row);

                    /* deliberate fall through */

                    case SL_TEXT:
                        if(sl->flags & SL_REFS)
                            {
                            orab(sl->flags, SL_ALTERED);
                            xf_drawsome = TRUE;
                            }

                    if(drawflag == CALC_DRAW)
                        draw_one_altered_slot(in_block.col, in_block.row);

                    graph_send_slot(in_block.col, in_block.row);
                    break;


                /*  case SL_BAD_FORMULA:    */
                    default:
                        break;
                    }

                if(iterate)
                    if(!slots_same_value(&valsave, sl, &iterchangeval))
                        iterchangeflag = 1;
                }
            }

        --iternumber;
        ++itercount;
        }
    while(iterate  &&  iternumber  &&  iterchangeflag);

    global_recalc = FALSE;

EXIT_POINT:

    recalc_bit = recalc_forced = FALSE;

    actind_end();

    /* restore old handlers */
    signal(SIGFPE, oldfpe);
    #if RISCOS
    signal(SIGSTAK, oldstak);
    #endif

    return(returncode);
}


/**************************************
*                                     *
* search the table of slot references *
* for a reference to the given slot   *
*                                     *
**************************************/

static intl
search_for_slrdependent(SLR *refto)
{
    slrep ssep, sep;
    struct slrentry key;
    intl six;

    if(!slrblkh)
        return(-1);

    ssep = (slrep) list_getptr(slrblkh);
    key.refto_doc = refto->doc;
    key.refto_col = refto->col;
    key.refto_row = refto->row;

    /* search for reference */
    sep = bsearch(&key,
                  ssep,
                  slrblkfree,
                  sizeof(struct slrentry),
                  slrcomp);
    if(!sep)
        return(-1);

    /* step back to start of all refs the same */
    for(six = sep - ssep;
        six > 0 && (sep - 1)->refto_row == refto->row &&
                   (sep - 1)->refto_col == refto->col &&
                   (sep - 1)->refto_doc == refto->doc
                   ;
        --sep, --six);

    return(six);
}

/*****************************************************
*                                                    *
* check if row selected                              *
* evaluate the formula, replacing any rows with trow *
*                                                    *
*****************************************************/

extern intl
selrow(uchar *expression, rowt trow, BOOL increment_rows)
{
    SYMB_TYPE *apop;
    int jmpval;

    eval_col = 0;
    eval_row = trow;

    errorflag = 0;
    stack_ptr = stack;

    /* on graunchy errors return to here with an error */
    if((jmpval = setjmp(safepoint)) != 0)
        pderror(jmpval);
    else
        exp_eval(expression);

    if(increment_rows)
        inc_rows(expression);

    if(stack_ptr != stack_one)
        return(FALSE);

    apop = poparg(POP_NORMAL);
    if(apop->type != SL_NUMBER)
        return(FALSE);

    return(apop->value.num != 0.);
}


/**************************************
*                                     *
* set document number for this handle *
*                                     *
**************************************/

extern docno
set_docno(dochandle han)
{
    docno doc;

    if(num_doc[han])
        return(num_doc[han]);

    doc = ensure_doc_in_list(find_leafname_using_handle(han), han);
    return(doc);
}

/*****************************************
*                                        *
* set the handle for a document and mark *
* for recalc if the handle changes       *
*                                        *
*****************************************/

static void
set_handle(docno doc)
{
    dochandle han, oldhan;
    intl olderr;

    oldhan = han_doc[doc];
    olderr = err_doc[doc];
    han = find_document_using_leafname(nam_doc[doc]);
    tracef1("[set_handle found handle: %d]\n", han);

    if(han == DOCHANDLE_SEVERAL)
        err_doc[doc] = EVAL_ERR_MANYEXTREF;
    else if(han == DOCHANDLE_NONE)
        {
        if(oldhan)
            num_doc[oldhan] = 0;
        han_doc[doc] = DOCHANDLE_NONE;
        err_doc[doc] = EVAL_ERR_CANTEXTREF;
        }
    else
        {
        han_doc[doc] = han;
        err_doc[doc] = 0;
        num_doc[han] = doc;
        }

    if(oldhan != han_doc[doc] || olderr != err_doc[doc])
        {
        tracef1("[set_handle doing tree_calc_above for: %d]\n", doc);
        tree_calc_above_doc(doc);
        }
}

/**************************************************************************
*                                                                         *
* range expansion: on encountering a range the function setup_range is    *
* called which prepares for the returning of values in the range.  Then   *
* successive calls to next_in_range return pointers to the elements. NULL *
* is returned when no more elements                                       *
*                                                                         *
**************************************************************************/

extern void
setup_range(SYMB_TYPE *symb)
{
    RANGE *t_range = &symb->value.range;

    col_index = range_col_1 = t_range->first.col;
    row_index = range_row_1 = t_range->first.row;
                range_col_2 = t_range->second.col;
                range_row_2 = t_range->second.row;
    doc_index = t_range->first.doc;
}

/*******************************************
*                                          *
* given two slot pointers, work out if the *
* value of the two slot are different      *
*                                          *
*******************************************/

static intl
slots_same_value(slotp sl1, slotp sl2, double *diff)
{
    if(sl1->type != sl2->type)
        return(FALSE);

    switch(sl1->type)
        {
        case SL_INTSTR:
            if(sl1->content.number.result.str_offset !=
               sl2->content.number.result.str_offset)
                return(FALSE);
            break;

        case SL_STRVAL:
            if((sl1->content.number.result.str_slot.col !=
                sl2->content.number.result.str_slot.col) ||
               (sl1->content.number.result.str_slot.row !=
                sl2->content.number.result.str_slot.row))
                return(FALSE);
            break;

        case SL_DATE:
            if(sl1->content.number.result.resdate !=
               sl2->content.number.result.resdate)
                return(FALSE);
            break;

        case SL_BLANK:
            break;

        case SL_NUMBER:
            if(!diff)
                {
                if(sl1->content.number.result.value !=
                   sl2->content.number.result.value)
                    return(FALSE);
                }
            else
                if(fabs(sl1->content.number.result.value -
                        sl2->content.number.result.value) > iterchangeval)
                    return(FALSE);
            break;

        case SL_ERROR:
            if(sl1->content.number.result.errno !=
               sl2->content.number.result.errno)
                return(FALSE);

        default:
            return(FALSE);
        }

    return(TRUE);
}

/*********************************************
*                                            *
* routine for bsearch to compare two columns *
*                                            *
*********************************************/

static intl
slrcomp(const void *slr1, const void *slr2)
{
    if(((slrep) slr1)->refto_doc > ((slrep) slr2)->refto_doc)
        return(1);
    if(((slrep) slr1)->refto_doc < ((slrep) slr2)->refto_doc)
        return(-1);

    if(((slrep) slr1)->refto_col > ((slrep) slr2)->refto_col)
        return(1);
    if(((slrep) slr1)->refto_col < ((slrep) slr2)->refto_col)
        return(-1);

    if(((slrep) slr1)->refto_row > ((slrep) slr2)->refto_row)
        return(1);
    if(((slrep) slr1)->refto_row < ((slrep) slr2)->refto_row)
        return(-1);
    return(0);
}

/********************************************
*                                           *
* switch to a new document given its number *
*                                           *
********************************************/

extern void
switch_document(docno newdoc)
{
    if(!newdoc || (newdoc == maybe_cur_docno(DOCHANDLE_NONE)))
        return;

    select_document_using_handle((dochandle) han_doc[newdoc]);
}

/***********************************
*                                  *
* build a tree for the whole sheet *
*                                  *
***********************************/

extern intl
tree_build(void)
{
    slotp sl;
    intl timer, sort;

    if(recalcorder() != RECALC_NATURAL)
        return(0);

    sort = (slrblkh || rngblkh) ? TRUE : FALSE;

    timer = 0;

    init_doc_as_block();

    while((sl = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        if(timer++ > 20 * TIMER_DELAY)
            {
            timer = 0;
            actind(ACT_CALCULAT, percent_in_block(DOWN_COLUMNS));
            }

        switch(sl->type)
            {
            case SL_TEXT:
                if(tree_str_insertslot(in_block.col,
                                       in_block.row,
                                       sort) < 0)
                    return(-1);
                break;

            default:
                if(tree_exp_insertslot(in_block.col,
                                       in_block.row,
                                       sort) < 0)
                    return(-1);
                break;
            }
        }

    actind_end();

    return(0);
}

/*****************************************************
*                                                    *
* set up the table of inter-sheet links and          *
* delete any unused names from the master name table *
*                                                    *
*****************************************************/

static void
tree_build_sheet_links(void)
{
    dochandle curhan;
    window_data *wdp;
    intl i, links_freed;
    slotp sl;
    uchar *rptr;
    docno docto;

    if(!doc_inited)
        init_doc_list();

    tracef0("[tree_build_sheet_links]\n");

    wdp = NULL;
    links_freed = 0;
    curhan = current_document_handle();

    /* first deal with un-natural sheets which don't have
    their slot references in the tree - find them in the sheet */
    while((wdp = next_document(wdp)) != NULL)
        {
        if((wdp->Xd_recalc_RC != RECALC_NATURAL) &&
           wdp->Xrebuild_sheet_links)
            {
            if(!links_freed)
                {
                free_links();
                links_freed = 1;
                }

            select_document(wdp);
            
            tracef1("[tree_build_sheet_links ROWCOL: %s]\r\n",
                    docname(wdp->DocHandle));

            /* for every slot in spreadsheet */
            init_doc_as_block();

            while((sl = next_slot_in_block(DOWN_COLUMNS)) != NULL)
                {
                /* slot got any SLRs ? */
                if(sl->flags & SL_REFS)
                    {
                    rptr = (sl->type == SL_TEXT)
                                   ? sl->content.text
                                   : sl->content.number.text;

                    /* for each slot reference */
                    for(my_init_ref(sl->type);
                        (rptr = my_next_ref(rptr, sl->type)) != NULL;)
                        {
                        ensure_sheet_depends(ensure_cur_docno(),
                                             ensure_cur_docno());
                        if((docto = (docno) talps(rptr, sizeof(docno))) != 0)
                            ensure_sheet_depends(docto, ensure_cur_docno());

                        rptr += sizeof(docno) + sizeof(colt) + sizeof(rowt);
                        }
                    }
                }

            wdp->Xrebuild_sheet_links = FALSE;
            }
        }

    select_document_using_handle(curhan);

    if(slrblkh && (slrflags & SL_ALTERED) ||
       rngblkh && (rngflags & SL_ALTERED) ||
       links_freed)
        {
        if(!links_freed)
            {
            free_links();
            links_freed = 1;
            }

        if(slrblkh)
            {
            slrep sep;

            for(i = 0, sep = (slrep) list_getptr(slrblkh);
                i < slrblkfree;
                ++i, ++sep)
                {
                /* make sure refto and refby are in list */
                ensure_sheet_depends(sep->byslr_doc, sep->byslr_doc);
                ensure_sheet_depends(sep->refto_doc, sep->byslr_doc);
                }
            }

        if(rngblkh)
            {
            rngep rep;

            for(i = 0, rep = (rngep) list_getptr(rngblkh);
                i < rngblkfree;
                ++i, ++rep)
                {
                /* make sure refto and refby are in list */
                ensure_sheet_depends(rep->byslr_doc, rep->byslr_doc);
                ensure_sheet_depends(rep->reftos_doc, rep->byslr_doc);
                }
            }
        }

    /* delete unused names from master table */
    if(links_freed)
        for(i = 0; i < MAX_DOCNO; ++i)
            if(nam_doc[i] && !lnk_doc[i])
                free_doc((docno) i);

    tracef0("[sheet_links out]\r\n");
}

/*************************************************************
*                                                            *
* recalculate the given slot - first all the slots           *
* on which it depends, then all the slots which depend on it *
*                                                            *
*************************************************************/

static intl
tree_calc(colt col, rowt row)
{
    slotp sl;
    intl res;

    /* evaluate this slot */
    sl = travel(col, row);

    if(sl && !(sl->flags & SL_VISITED))
        {
        if((res = tree_calc_below(sl, col, row)) != 0)
            tree_calc_above(sl, col, row);
        }
    else
        res = 0;

    return(res);
}

/**************************************************
*                                                 *
* recalculate the tree above the given slot - all *
* slots which depend on this slot                 *
*                                                 *
**************************************************/

static intl
tree_calc_above(slotp sl, colt col, rowt row)
{
    SLR refto;
    intl ix;
    slotp tsl;
    dochandle curhan;

    /* has document any references ? */
    if((refto.doc = maybe_cur_docno(DOCHANDLE_NONE)) == DOCNO_NOENTRY)
        return(0);

    /* evaluate dependents */
    refto.col = col;
    refto.row = row;
    curhan = current_document_handle();

    if(slrblkh)
        {
        if((ix = search_for_slrdependent(&refto)) >= 0)
            {
            slrep sep = (slrep) list_getptr(slrblkh) + ix;

            while(ix < slrblkfree &&
                  sep->refto_row == refto.row &&
                  sep->refto_col == refto.col &&
                  sep->refto_doc == refto.doc &&
                  check_docvalid(sep->byslr_doc) >= 0)
                {
                switch_document(sep->byslr_doc);

                tracef3("[tree_calc_above doc: %d, col: %d, row: %d]\n",
                        sep->refto_doc, sep->refto_col, sep->refto_row);

                if((tsl = travel(sep->byslr_col, sep->byslr_row)) != NULL)
                    {
                    if(!(tsl->flags & SL_RECALCED))
                        {
                        if(!(tsl->flags & SL_MUSTRECALC))
                            {
                            orab(tsl->flags, SL_MUSTRECALC);
                            ++slotstocalc;
                            recalc_bit = TRUE;
                            }
                        }
                    }
                ++sep;
                ++ix;

                select_document_using_handle(curhan);
                }
            }
        }

    /* search range table for dependents */
    if(rngblkh)
        {
        rngep rep;

        ix = 0;
        rep = (rngep) list_getptr(rngblkh);

        while(ix < rngblkfree)
            {
            if(col >= rep->reftos_col &&
               row >= rep->reftos_row &&
               col <= rep->reftoe_col &&
               row <= rep->reftoe_row &&
               rep->reftos_doc == refto.doc &&
               check_docvalid(rep->byslr_doc) >= 0)
                {
                switch_document(rep->byslr_doc);

                tsl = travel(rep->byslr_col, rep->byslr_row);
                if(tsl)
                    {
                    if(!(tsl->flags & SL_RECALCED))
                        {
                        if(!(tsl->flags & SL_MUSTRECALC))
                            {
                            orab(tsl->flags, SL_MUSTRECALC);
                            ++slotstocalc;
                            recalc_bit = TRUE;
                            }
                        }
                    }

                select_document_using_handle(curhan);
                }
            ++rep;
            ++ix;
            }
        }

    if(sl->flags & SL_MUSTRECALC)
        {
        bicab(sl->flags, SL_MUSTRECALC);
        --slotstocalc;
        }

    orab(sl->flags, SL_VISITED);

    return(0);
}

/****************************
*                           *
* mark all references to a  *
* given document for recalc *
*                           *
****************************/

static void
tree_calc_above_doc(docno doc)
{
    intl i;
    dochandle curhan;
    slotp tsl;

    curhan = current_document_handle();

    if(slrblkh)
        {
        slrep sep = list_getptr(slrblkh);

        for(i = 0; i < slrblkfree; ++sep, ++i)
            {
            if(sep->refto_doc == (docno) doc &&
               check_docvalid(sep->byslr_doc) >= 0)
                {
                switch_document(sep->byslr_doc);

                tsl = travel(sep->byslr_col, sep->byslr_row);
                if(tsl)
                    {
                    orab(tsl->flags, SL_MUSTRECALC);
                    recalc_bit = TRUE;
                    }
                }
            }
        }

    if(rngblkh)
        {
        rngep rep = list_getptr(rngblkh);

        for(i = 0; i < rngblkfree; ++rep, ++i)
            {
            if(rep->reftos_doc == (docno) doc &&
               check_docvalid(rep->byslr_doc) >= 0)
                {
                switch_document(rep->byslr_doc);

                tsl = travel(rep->byslr_col, rep->byslr_row);
                if(tsl)
                    {
                    orab(tsl->flags, SL_MUSTRECALC);
                    recalc_bit = TRUE;
                    }
                }
            }
        }

    select_document_using_handle(curhan);
}

/**********************************************
*                                             *
* recalculate the tree below the given slot - *
* all slots on which the given slot depends   *
*                                             *
**********************************************/

static intl
tree_calc_below(slotp sl, colt col, rowt row)
{
    intl calced, index;

    calced = index = 0;

    tracef2("[tree_calc_below %d, %d]\n", col, row);

    /* check for circular reference */
    if(sl->flags & SL_CIRC)
        {
        if(sl->type == SL_TEXT)
            return(1);

        if(iterate)
            {
            if(!(sl->flags & SL_RECALCED))
                {
                struct slot valsave;

                valsave = *sl;
                tree_eval_slot(sl, col, row);
                if(!slots_same_value(&valsave, sl, &iterchangeval))
                    iterchangeflag = 1;
                }

            /* update slots to be recalculated */
            if(sl->flags & SL_MUSTRECALC)
                {
                bicab(sl->flags, SL_MUSTRECALC);
                --slotstocalc;
                }

            sl->flags = (sl->flags & ~SL_CIRC) | SL_VISITED | SL_ITERATE;

            return(1);
            }
        else
            return(flagerr(sl, col, row, EVAL_ERR_CIRC));
        }

    /* check stack is OK */
    tracef3("[stack difference: %d, startsp: %x, sp: %x]\n",
            (char *) stacklevel - (char *) &calced, stacklevel, &calced);
    if((char *) stacklevel - (char *) &calced >= TREE_STACKLEVEL)
        return(flagerr(sl, col, row, EVAL_ERR_STACKOVER));

    orab(sl->flags, SL_CIRC);

    if(!(sl->flags & SL_VISITED))
        {
        dochandle curhan = current_document_handle();

        if(sl->flags & SL_REFS)
            {
            /* work down the tree to the leaves */
            uchar *exp, *arg;
            intl state, res;
            SLR refto, reftoe, ix;

            if(sl->type != SL_TEXT)
                {
                state = -1;
                exp = sl->content.number.text;

                do
                    {
                    select_document_using_handle(curhan);

                    res = exp_refersto_next(&exp, &state, &arg,
                                            &refto, &reftoe);

                    switch(res)
                        {
                        case OPR_SLR:
                            if(check_docvalid(refto.doc) < 0)
                                break;
                            switch_document(refto.doc);

                            if((res = tree_calc(refto.col, refto.row)) < 0)
                                {
                                calced = 1;
                                goto quitcirc;
                                }
                            calced |= res;
                            break;

                        case OPR_RANGE:
                            {
                            rngep rep = NULL;

                            /* check if the range has been processed before -
                            if so, just read the flags from that time to save
                            scanning down every slot in the range again */
                            if(rngblkh)
                                {
                                rngep srep;
                                struct rngentry key;
                                intl rix;

                                srep = (rngep) list_getptr(rngblkh);
                                key.reftos_doc = refto.doc;
                                key.reftos_col = refto.col;
                                key.reftos_row = refto.row;
                                key.reftoe_doc = reftoe.doc;
                                key.reftoe_col = reftoe.col;
                                key.reftoe_row = reftoe.row;

                                /* search for reference */
                                if((rep = bsearch(&key,
                                                  srep,
                                                  rngblkfree,
                                                  sizeof(struct rngentry),
                                                  rngcomp)) != NULL)
                                    {
                                    /* step back to start of all
                                    ranges the same */
                                    for(rix = rep - srep;
                                        rix > 0 &&
                                        (rep - 1)->reftos_row == refto.row &&
                                        (rep - 1)->reftos_col == refto.col &&
                                        (rep - 1)->reftoe_row == reftoe.row &&
                                        (rep - 1)->reftoe_col == reftoe.col &&
                                        (rep - 1)->reftos_doc == refto.doc;
                                        --rep, --rix);

                                    if(rep->entflags & SL_VISITED)
                                        {
                                        calced |= rep->entflags & SL_RECALCED;
                                        break;
                                        }
                                    }
                                }

                            if(check_docvalid(refto.doc) < 0)
                                break;
                            switch_document(refto.doc);

                            ix.col = refto.col;
                            do
                                {
                                ix.row = refto.row;
                                do
                                    {
                                    if((res = tree_calc(ix.col, ix.row)) < 0)
                                        {
                                        calced = 1;
                                        goto quitcirc;
                                        }
                                    calced |= res;
                                    ++ix.row;
                                    }
                                while(ix.row <= reftoe.row);
                                ++ix.col;
                                }
                            while(ix.col <= reftoe.col);

                            /* set flags for range to avoid it again */
                            if(rep)
                                orab(rep->entflags, SL_VISITED | (calced ? SL_RECALCED : 0));

                            break;
                            }

                        case OPR_INDEX:
                            index = 1;
                            break;
                        }
                    }
                while(res != OPR_END);
                }
            else
                {
                tracef0("[tree_calc_below got string slot]\n");
                exp = sl->content.text;
                my_init_ref(SL_TEXT);
                while((exp = my_next_ref(exp, SL_TEXT)) != NULL)
                    {
                    /* for each slot reference */
                    refto.doc = (docno) talps(exp, sizeof(docno));
                    exp += sizeof(docno);

                    refto.col = (colt) (talps(exp, sizeof(colt)) &
                                    (COLNOBITS | BADCOLBIT));
                    exp += sizeof(colt);

                    refto.row = (rowt) (talps(exp, sizeof(rowt)) &
                                    (ROWNOBITS | BADROWBIT));
                    exp += sizeof(rowt);

                    tracef3("[tree_calc_below string doc: %d, col: %d, row: %d]\n",
                            refto.doc, refto.col, refto.row);

                    if(bad_col(refto.col) || bad_row(refto.row))
                        continue;

                    if(check_docvalid(refto.doc) < 0)
                        continue;

                    switch_document(refto.doc);

                    if((res = tree_calc(refto.col, refto.row)) < 0)
                        {
                        calced = 1;
                        goto quitcirc;
                        }

                    calced |= res;
                    }
                }
            }

    quitcirc:

        select_document_using_handle(curhan);

        if(!(sl->flags & SL_RECALCED))
            {
            struct slot valsave;

            if(calced || (sl->flags & SL_MUSTRECALC))
                {
                if(iterate)
                    valsave = *sl;

                tree_eval_slot(sl, col, row);
                calced = 1;

                if(iterate)
                    if(!slots_same_value(&valsave, sl, &iterchangeval))
                        iterchangeflag = 1;
                }
            else if(index)
                {
                /* evaluate slot and check for a changed value */
                valsave = *sl;
                tracef2("[tree_calc_below index in slot: %d, %d]\n",
                        col, row);
                tree_eval_slot(sl, col, row);
                tracef2("[tree_calc_below after index in slot: %d, %d]\n",
                        col, row);
                if(slots_same_value(&valsave,
                                    sl,
                                    iterate ? &iterchangeval : NULL))
                    sl->flags = valsave.flags;
                else
                    {
                    calced = 1;
                    if(iterate)
                        iterchangeflag = 1;
                    }
                }
            }
        }

    /* update slot status */
    if(sl->flags & SL_MUSTRECALC)
        {
        bicab(sl->flags, SL_MUSTRECALC);
        --slotstocalc;
        }

    sl->flags = (sl->flags & ~SL_CIRC) | SL_VISITED;

    #if 0
    --stacklevel;
    #endif

    return(calced);
}

/********************************************
*                                           *
* recalculate the special slots in the tree *
*                                           *
********************************************/

static intl
tree_calcspecial(void)
{
    intl ix, calcflag;
    slotp sl;
    slrep sep;
    docno curdoc;

    if(!slrblkh)
        return(0);

    /* check if document has an entry in the table */
    if((curdoc = maybe_cur_docno(DOCHANDLE_NONE)) == DOCNO_NOENTRY)
        return(0);

    for(ix = 0, sep = (slrep) list_getptr(slrblkh);
        ix < slrblkfree && sep->refto_col == -1;
        ++sep, ++ix)
        {
        if(sep->refto_doc == curdoc)
            {
            calcflag = 0;

            if((sl = travel(sep->byslr_col, sep->byslr_row)) != NULL)
                {
                tracef2("[tree_calcspecial slot: %d, %d]\n",
                        sep->byslr_col, sep->byslr_row);

                if(!(sl->flags & SL_MUSTRECALC))
                    {
                    switch((intl) sep->refto_row)
                        {
                        case TREE_ALWAYS:
                            calcflag = 1;
                            break;

                        case TREE_MOVED:
                            if(sl->flags & SL_MOVED)
                                {
                                tracef2("[tree_calcspecial slot moved: %d, %d]\n",
                                        sep->byslr_col, sep->byslr_row);
                                calcflag = 1;
                                }
                            break;

                        case TREE_INDEX:
                            tree_calc(sep->byslr_col, sep->byslr_row);
                            break;
                        }
                    }

                if(calcflag)
                    {
                    orab(sl->flags, SL_MUSTRECALC);
                    ++slotstocalc;
                    recalc_bit = TRUE;
                    }
                }
            }
        }

    return(0);
}

/******************************************************
*                                                     *
* delete the dependency tree for the current document *
*                                                     *
******************************************************/

extern void
tree_delete(void)
{
    tracef0("[tree_delete]\n");

    tree_removeblock(0, (rowt) 0, numcol - 1, numrow - 1);
}

/********************************************************
*                                                       *
* remove all entries marked for deletion from the trees *
*                                                       *
********************************************************/

static void
tree_deletemarked(void)
{
    intl i;
    mhandle newhandle;

    if(slrblkh && (slrflags & SL_TOBEDEL))
        {
        slrep ssep, isep, osep;

        ssep = isep = osep = (slrep) list_getptr(slrblkh);

        for(i = 0; i < slrblkfree; ++i, ++isep)
            {
            if(!(isep->entflags & SL_TOBEDEL))
                {
                if(osep != isep)
                    *osep = *isep;

                ++osep;
                }
            #if TRACE
            else            
                tracef2("[tree_deletemarked slr refby: %d, %d]\n",
                        isep->byslr_col, isep->byslr_row);
            #endif
            }

        slrblkfree = osep - ssep;
        slrflags = (slrflags & ~SL_TOBEDEL) | SL_ALTERED;

        /* free some memory */
        if(slrblksize - slrblkfree > SLRBLKINC)
            if((newhandle = list_reallochandle(slrblkh, (word32)
                                               (slrblkfree + SLRBLKINC) *
                                               sizeof(struct slrentry))) > 0)
                {
                slrblkh = newhandle;
                slrblksize = slrblkfree + SLRBLKINC;
                tracef2("[slr block size has shrunk: %d, %d bytes]\n",
                        slrblksize, sizeof(struct slrentry) * slrblksize); 
                }
        }

    if(rngblkh && (rngflags & SL_TOBEDEL))
        {
        rngep srep, irep, orep;

        srep = irep = orep = (rngep) list_getptr(rngblkh);

        for(i = 0; i < rngblkfree; ++i, ++irep)
            {
            if(!(irep->entflags & SL_TOBEDEL))
                {
                if(orep != irep)
                    *orep = *irep;

                ++orep;
                }
            #if TRACE
            else            
                tracef2("[tree_deletemarked range refby: %d, %d]\n",
                        irep->byslr_col, irep->byslr_row);
            #endif
            }

        rngblkfree = orep - srep;
        rngflags = (rngflags & ~SL_TOBEDEL) | SL_ALTERED;

        /* free some memory */
        if(rngblksize - rngblkfree > RNGBLKINC)
            if((newhandle = list_reallochandle(rngblkh, (word32)
                                               (rngblkfree + RNGBLKINC) *
                                               sizeof(struct rngentry))) > 0)
                {
                rngblkh = newhandle;
                rngblksize = rngblkfree + RNGBLKINC;
                tracef2("[range block size has shrunk: %d, %d bytes]\n",
                        rngblksize, sizeof(struct rngentry) * rngblksize);
                }
        }
}

/**************************************
*                                     *
* actually recalculate the given slot *
*                                     *
**************************************/

static void
tree_eval_slot(slotp sl, colt col, rowt row)
{
    intl calced;

    tracef2("[tree_eval_slot %d, %d]\n", col, row);

    calced = 0;

    switch(sl->type)
        {
        case SL_DATE:
        case SL_INTSTR:
        case SL_STRVAL:
        case SL_NUMBER:
        case SL_ERROR:
            evastoi(sl, col, row);
            calced = 1;
            break;

        case SL_TEXT:
            if(sl->flags & SL_REFS)
                calced = 1;
            break;

        default:
            break;
        }

    if(calced)
        {
        orab(sl->flags, SL_ALTERED);
        xf_drawsome = TRUE;

        if(drawflag == CALC_DRAW)
            draw_one_altered_slot(col, row);

        graph_send_slot(col, row);
        }

    sl->flags = (sl->flags | SL_RECALCED) & ~SL_MOVED;
}


/*******************************************************
*                                                      *
* insert all the dependencies for a slot into the tree *
*                                                      *
*******************************************************/

extern intl
tree_exp_insertslot(colt col, rowt row, intl sort)
{
    uchar *exp, *arg;
    intl state, res, exp_off, arg_off;
    SLR refto, reftoe, by;
    slotp sl;

    if(recalcorder() != RECALC_NATURAL)
        {
        rebuild_sheet_links = TRUE;
        return(0);
        }

    sl = travel(col, row);
    orab(sl->flags, SL_MUSTRECALC);
    if(!(sl->flags & SL_REFS))
        return(0);

    tracef3("[tree_exp_insertslot slot: %d, %d, %x]\n",
            col, row, (intl) sl);

    by.doc = ensure_cur_docno();
    by.col = col;
    by.row = row;
    state = -1;
    exp = sl->content.number.text;

    do
        {
        res = exp_refersto_next(&exp, &state, &arg, &refto, &reftoe);

        /* stop at the end */
        if(res == OPR_END)
            break;

        /* convert pointers to offsets since slots may move */
        exp_off = exp - sl->content.number.text;
        if(arg)
            arg_off = arg - sl->content.number.text;

        switch(res)
            {
            case OPR_COL:
            case OPR_READ:
            case OPR_SLR:
            case OPR_INDEX:
                if(add_slrdependency(&refto, &by, sort) < 0)
                    return(-1);
                break;

            case OPR_RANGE:
                if(add_rngdependency(&refto, &reftoe, &by, sort) < 0)
                    return(-1);
                break;
            }

        /* reload pointers after tree addition */
        sl = travel(col, row);
        exp = sl->content.number.text + exp_off;
        if(arg)
            arg = sl->content.number.text + arg_off;
        }
    while(TRUE);

    return(0);
}

/*********************************************************
*                                                        *
* find documents with the same name as the supplied      *
* document, and mark them and dependent slots for recalc *
*                                                        *
*********************************************************/

static void
tree_mark_related_docs(dochandle han)
{
    docno doc;
    char *name;

    tracef1("[tree_mark_related_docs: %s]\n",
            trace_string(find_leafname_using_handle(han)));

    /* find documents related by name */
    if((name = find_leafname_using_handle(han)) != NULL)
        {
        dochandle curhan;

        tree_rearrange();
        curhan = current_document_handle();

        for(doc = 1; doc < MAX_DOCNO; ++doc)
            if(nam_doc[doc] && !stricmp(nam_doc[doc], name))
                {
                if(check_docvalid(doc) >= 0)
                    {
                    switch_document(doc);
                    tracef1("[tree_mark_related_docs marked related doc: %d]\n",
                            doc);
                    recalc_bit = TRUE;
                    tree_calc_above_doc(doc);
                    }
                }
        select_document_using_handle(curhan);
        }
}

/*****************************************************************
*                                                                *
* this routine marks all slots dependent on slots within the     *
* given block for recalculation - as needed when slots are moved *
* remove a block of slots and their dependencies from the tree   *
*                                                                *
*****************************************************************/

extern void
tree_moveblock(colt scol, rowt srow, colt ecol, rowt erow)
{
    intl i;
    slotp sl;
    dochandle curhan;
    docno curdoc;

    /* check if document has table entry */
    if((curdoc = maybe_cur_docno(DOCHANDLE_NONE)) == DOCNO_NOENTRY)
        return;

    if(recalcorder() != RECALC_NATURAL)
        rebuild_sheet_links = TRUE;

    curhan = current_document_handle();

    if(slrblkh)
        {
        slrep sep;

        i = 0;
        sep = (slrep) list_getptr(slrblkh);
        while(i < slrblkfree)
            {
            if(sep->refto_col >= scol &&
               sep->refto_col <= ecol &&
               sep->refto_row >= srow &&
               sep->refto_row <= erow &&
               sep->refto_doc == curdoc &&
               check_docvalid(sep->byslr_doc) >= 0)
                {
                switch_document(sep->byslr_doc);

                sl = travel(sep->byslr_col, sep->byslr_row);
                if(sl)
                    orab(sl->flags, SL_MUSTRECALC);
                }

            ++i;
            ++sep;
            }
        }

    if(rngblkh)
        {
        rngep rep;

        i = 0;
        rep = (rngep) list_getptr(rngblkh);
        while(i < rngblkfree)
            {
            if(rep->reftos_col <= ecol &&
               rep->reftoe_col >= scol &&
               rep->reftos_row <= erow &&
               rep->reftoe_row >= srow &&
               rep->reftos_doc == curdoc &&
               check_docvalid(rep->byslr_doc) >= 0)
                {
                switch_document(rep->byslr_doc);

                sl = travel(rep->byslr_col, rep->byslr_row);
                if(sl)
                    orab(sl->flags, SL_MUSTRECALC);
                }

            ++i;
            ++rep;
            }
        }

    select_document_using_handle(curhan);
}

/****************
*               *
* sort out tree *
*               *
****************/

static void
tree_rearrange(void)
{
    tree_deletemarked();
    tree_sort();

    tree_build_sheet_links();

    bicab(slrflags, SL_ALTERED);
    bicab(rngflags, SL_ALTERED);
}

/***************************************************************
*                                                              *
* remove a block of slots and their dependencies from the tree *
*                                                              *
***************************************************************/

extern void
tree_removeblock(colt scol, rowt srow, colt ecol, rowt erow)
{
    intl i;
    slotp sl;
    dochandle curhan;
    docno curdoc;

    /* check if document has table entry */
    if((curdoc = maybe_cur_docno(DOCHANDLE_NONE)) == DOCNO_NOENTRY)
        return;

    if(recalcorder() != RECALC_NATURAL)
        rebuild_sheet_links = TRUE;

    curhan = current_document_handle();

    if(slrblkh)
        {
        slrep sep;

        i = 0;
        sep = (slrep) list_getptr(slrblkh);
        while(i < slrblkfree)
            {
            if(sep->refto_col >= scol &&
               sep->refto_col <= ecol &&
               sep->refto_row >= srow &&
               sep->refto_row <= erow &&
               sep->refto_doc == curdoc &&
               check_docvalid(sep->byslr_doc) >= 0)
                {
                switch_document(sep->byslr_doc);

                sl = travel(sep->byslr_col, sep->byslr_row);
                if(sl)
                    orab(sl->flags, SL_MUSTRECALC);
                }

            if(sep->byslr_col >= scol &&
               sep->byslr_col <= ecol &&
               sep->byslr_row >= srow &&
               sep->byslr_row <= erow &&
               sep->byslr_doc == curdoc)
                {
                orab(sep->entflags, SL_TOBEDEL);
                slrflags |= SL_TOBEDEL;
                }

            ++i;
            ++sep;
            }
        }

    if(rngblkh)
        {
        rngep rep;

        i = 0;
        rep = (rngep) list_getptr(rngblkh);
        while(i < rngblkfree)
            {
            if(rep->reftos_col <= ecol &&
               rep->reftoe_col >= scol &&
               rep->reftos_row <= erow &&
               rep->reftoe_row >= srow &&
               rep->reftos_doc == curdoc &&
               check_docvalid(rep->byslr_doc) >= 0)
                {
                switch_document(rep->byslr_doc);

                sl = travel(rep->byslr_col, rep->byslr_row);
                if(sl)
                    orab(sl->flags, SL_MUSTRECALC);
                }

            if(rep->byslr_col >= scol &&
               rep->byslr_col <= ecol &&
               rep->byslr_row >= srow &&
               rep->byslr_row <= erow &&
               rep->byslr_doc == curdoc)
                {
                orab(rep->entflags, SL_TOBEDEL);
                rngflags |= SL_TOBEDEL;
                }

            ++i;
            ++rep;
            }
        }

    select_document_using_handle(curhan);
}

/***************************************************
*                                                  *
* remove a slot and its dependencies from the tree *
*                                                  *
***************************************************/

extern void
tree_removeslot(colt col, rowt row)
{
    tree_removeblock(col, row, col, row);
}

/******************************************************
*                                                     *
* recurse over sheet links tree to build a linked set *
*                                                     *
******************************************************/

static void
tree_sheet_links_recurse(char *linked_set, char *linked_stop, docno curdoc)
{
    intl index;
    docno doc;

    if(curdoc == DOCNO_NOENTRY || linked_stop[curdoc])
        return;

    linked_stop[curdoc] = 1;

    /* read all dependent files */
    index = 0;
    while((doc = next_dependent_doc(&curdoc, &index)) != 0)
        {
        linked_set[doc] = 1;
        tracef2("[tree_s_l_r dependent: %d, %s]\n",
                doc, docname(han_doc[doc]));
        if(doc != curdoc)
            tree_sheet_links_recurse(linked_set, linked_stop, doc);
        }

    /* read all supporting files */
    index = 1;
    while((doc = next_supporting_file(&curdoc, &index, NULL, NULL)) != 0)
        {
        linked_set[doc] = 1;
        tracef2("[tree_s_l_r supporting: %d, %s]\n",
                doc, docname(han_doc[doc]));
        if(doc != curdoc)
            tree_sheet_links_recurse(linked_set, linked_stop, doc);
        }
}

/*************************************************************
*                                                            *
* sort the row and column trees after a general modification *
*                                                            *
*************************************************************/

static void
tree_sort(void)
{
    if(slrblkh && (slrflags & SL_ALTERED))
        {
        tracef0("[sort slr]\r\n");

        qsort(list_getptr(slrblkh),
              slrblkfree,
              sizeof(struct slrentry),
              slrcomp);
        }

    if(rngblkh && (rngflags & SL_ALTERED))
        {
        tracef0("[sort range]\r\n");

        qsort(list_getptr(rngblkh),
              rngblkfree,
              sizeof(struct rngentry),
              rngcomp);
        }
}

/*********************************************
*                                            *
* add references to the tree for a text slot *
*                                            *
*********************************************/

extern intl
tree_str_insertslot(colt col, rowt row, intl sort)
{
    uchar *c;
    SLR refto, by;
    slotp sl;

    if(recalcorder() != RECALC_NATURAL)
        {
        rebuild_sheet_links = TRUE;
        return(0);
        }

    sl = travel(col, row);
    orab(sl->flags, SL_MUSTRECALC);

    if(!(sl->flags & SL_REFS))
        return(0);

    by.doc = ensure_cur_docno();
    by.col = col;
    by.row = row;
    c = sl->content.text;
    my_init_ref(SL_TEXT);
    while((c = my_next_ref(c, SL_TEXT)) != NULL)
        {
        intl off;

        /* for each slot reference */
        if((refto.doc = (docno) talps(c, sizeof(docno))) == 0)
            refto.doc = ensure_cur_docno();
        c += sizeof(docno);

        refto.col = (colt) (talps(c, sizeof(colt)) & (COLNOBITS | BADCOLBIT));
        c += sizeof(colt);
        refto.row = (rowt) (talps(c , sizeof(rowt)) & (ROWNOBITS | BADROWBIT));
        c += sizeof(rowt);

        /* calculate current offset */
        off = c - sl->content.text;

        if(!(bad_col(refto.col) || bad_row(refto.row)))
            if(add_slrdependency(&refto, &by, sort) < 0)
                return(-1);

        c = travel(col, row)->content.text + off;
        }

    return(0);
}

/*******************************************
*                                          *
* swap references in the tree for two rows *
*                                          *
*******************************************/

extern void
tree_swaprefs(rowt row1, rowt row2,
              colt scol, colt ecol)
{
    intl i;
    rowt diff1, diff2;
    docno curdoc;

    /* check if document has an entry in the table */
    if((curdoc = maybe_cur_docno(DOCHANDLE_NONE)) == DOCNO_NOENTRY)
        return;

    diff1 = row2 - row1;
    diff2 = row1 - row2;

    if(slrblkh)
        {
        slrep sep;

        for(i = 0, sep = list_getptr(slrblkh);
            i < slrblkfree;
            ++i, ++sep)
            {
            if(sep->refto_col >= scol &&
               sep->refto_col <= ecol &&
               sep->refto_doc == curdoc)
                {
                if(sep->refto_row == row1)
                    plusab(sep->refto_row, diff1);
                elif(sep->refto_row == row2)
                    plusab(sep->refto_row, diff2);
                }

            if(sep->byslr_col >= scol &&
               sep->byslr_col <= ecol &&
               sep->byslr_doc == curdoc)
                {
                if(sep->byslr_row == row1)
                    {
                    tracef4("[tree_swaprefs: %d, %d becomes %d, %d]\n",
                            sep->byslr_col, sep->byslr_row,
                            sep->byslr_col, sep->byslr_row + diff1);
                    plusab(sep->byslr_row,  diff1);
                    }
                elif(sep->byslr_row == row2)
                    {
                    tracef4("[tree_swaprefs: %d, %d becomes %d, %d]\n",
                            sep->byslr_col, sep->byslr_row,
                            sep->byslr_col, sep->byslr_row + diff2);
                    plusab(sep->byslr_row, diff2);
                    }
                }
            }
        }

    if(rngblkh)
        {
        rngep rep;

        for(i = 0, rep = list_getptr(rngblkh);
            i < rngblkfree;
            ++i, ++rep)
            {
            if(rep->reftos_doc == curdoc)
                {
                if(rep->reftos_col >= scol &&
                   rep->reftos_col <= ecol)
                    {
                    if(rep->reftos_row == row1)
                        plusab(rep->reftos_row, diff1);
                    elif(rep->reftos_row == row2)
                        plusab(rep->reftos_row, diff2);
                    }

                if(rep->reftoe_col >= scol &&
                   rep->reftoe_col <= ecol)
                    {
                    if(rep->reftoe_row == row1)
                        plusab(rep->reftoe_row, diff1);
                    elif(rep->reftoe_row == row2)
                        plusab(rep->reftoe_row, diff2);
                    }
                }

            if(rep->byslr_col >= scol &&
               rep->byslr_col <= ecol &&
               rep->byslr_doc == curdoc)
                {
                if(rep->byslr_row == row1)
                    plusab(rep->byslr_row, diff1);
                elif(rep->byslr_row == row2)
                    plusab(rep->byslr_row, diff2);
                }

            }
        }
}

/*********************************
*                                *
* switch off tree after an error *
*                                *
*********************************/

extern void
tree_switchoff(void)
{
    tree_delete();

    /* force other recalc (default) */
    d_recalc_RC = 'C';

    /* report error */
    reperr_null(ERR_NOTREE);
    been_error = FALSE;
}

/********************************
*                               *
* update references in the tree *
*                               *
********************************/

extern void
tree_updref(colt mrksco, rowt mrksro,
            colt mrkeco, rowt mrkero,
            colt coldiff, rowt rowdiff)
{
    intl i, badflag;
    docno curdoc;

    /* check if document has an entry */
    if((curdoc = maybe_cur_docno(DOCHANDLE_NONE)) == DOCNO_NOENTRY)
        return;

    if(coldiff == BADCOLBIT || rowdiff == BADROWBIT)
        badflag = 1;
    else
        badflag = 0;

    if(slrblkh)
        {
        slrep sep;

        for(i = 0, sep = list_getptr(slrblkh);
            i < slrblkfree;
            ++i, ++sep)
            {
            if(sep->refto_row >= mrksro &&
               sep->refto_col >= mrksco &&
               sep->refto_row <= mrkero &&
               sep->refto_col <= mrkeco &&
               sep->refto_doc == curdoc)
                {
                if(badflag)
                    {
                    orab(sep->entflags, SL_TOBEDEL);
                    slrflags |= SL_TOBEDEL;
                    }
                else
                    {
                    plusab(sep->refto_row, rowdiff);
                    plusab(sep->refto_col, coldiff);
                    }
                }

            if(sep->byslr_row >= mrksro &&
               sep->byslr_col >= mrksco &&
               sep->byslr_row <= mrkero &&
               sep->byslr_col <= mrkeco &&
               sep->byslr_doc == curdoc)
                {
                if(badflag)
                    {
                    orab(sep->entflags, SL_TOBEDEL);
                    slrflags |= SL_TOBEDEL;
                    }
                else
                    {
                    plusab(sep->byslr_row, rowdiff);
                    plusab(sep->byslr_col, coldiff);
                    }
                }
            }
        }

    if(rngblkh)
        {
        rngep rep;

        for(i = 0, rep = list_getptr(rngblkh);
            i < rngblkfree;
            ++i, ++rep)
            {
            if(rep->reftos_doc == curdoc)
                {
                if(rep->reftos_row >= mrksro &&
                   rep->reftos_col >= mrksco &&
                   rep->reftos_row <= mrkero &&
                   rep->reftos_col <= mrkeco)
                    {
                    if(badflag)
                        {
                        orab(rep->entflags, SL_TOBEDEL);
                        rngflags |= SL_TOBEDEL;
                        }
                    else
                        {
                        plusab(rep->reftos_row, rowdiff);
                        plusab(rep->reftos_col, coldiff);
                        }
                    }

                if(rep->reftoe_row >= mrksro &&
                   rep->reftoe_col >= mrksco &&
                   rep->reftoe_row <= mrkero &&
                   rep->reftoe_col <= mrkeco)
                    {
                    if(badflag)
                        {
                        orab(rep->entflags, SL_TOBEDEL);
                        rngflags |= SL_TOBEDEL;
                        }
                    else
                        {
                        plusab(rep->reftoe_row, rowdiff);
                        plusab(rep->reftoe_col, coldiff);
                        }
                    }
                }

            if(rep->byslr_row >= mrksro &&
               rep->byslr_col >= mrksco &&
               rep->byslr_row <= mrkero &&
               rep->byslr_col <= mrkeco &&
               rep->byslr_doc == curdoc)
                {
                if(badflag)
                    {
                    orab(rep->entflags, SL_TOBEDEL);
                    rngflags |= SL_TOBEDEL;
                    }
                else
                    {
                    plusab(rep->byslr_row, rowdiff);
                    plusab(rep->byslr_col, coldiff);
                    }
                }
            }
        }
}

/************************************************************************
*                                                                       *
* called by Dxxx and row selection to say whether this item is included *
*                                                                       *
************************************************************************/

extern BOOL
true_condition(uchar *cond, BOOL increment_refs)
{
    SYMB_TYPE *old_one = stack_ptr + 1;
    SYMB_TYPE *apop;

    exp_eval(cond);
    if(stack_ptr != old_one)
        errorflag = ERR_BAD_EXPRESSION;

    apop = poparg(POP_NORMAL);
    if(apop->type != SL_NUMBER)
        errorflag = ERR_BAD_EXPRESSION;

    if(increment_refs)
        inc_refs(cond, colinc, rowinc);

    /* restore old variables */
    return(apop->value.num != 0.);
}

/***********************************
*                                  *
* write document name, given docno *
*                                  *
***********************************/

extern intl
write_docname(char *str, docno doc)
{
    intl len;

    if(!doc)
        return(0);

    *str++ = '[';
    strcpy(str, nam_doc[doc]);
    len = strlen(nam_doc[doc]);
    str += len;
    *str++ = ']';
    *str++ = '\0';

    return(len + 2);
}

/* end of eval.c */
