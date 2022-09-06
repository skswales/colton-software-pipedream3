/* execs.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       execs.c - module that does lots of backsplash commands
 * Author:      RJM August 1987
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"

#elif MS
#else
    assert(0);
#endif


#include "datafmt.h"


#if RISCOS
#include "ext.riscos"
#include "riscdialog.h"
#endif


/* exported functions */

extern void add_to_protect_list(uchar *ptr);
extern void alnsto(uchar justify, uchar mask, uchar bits);
extern BOOL bad_reference(colt tcol, rowt trow);
extern void clear_protect_list(void);
extern void delrwb(uchar flags);
extern intl do_delete_block(BOOL do_save);
extern void endeex(void);
extern BOOL get_marked_block(void);
extern void mark_to_end(coord rowonscr);
extern void prccml(uchar *array);
extern BOOL save_words(uchar *ptr);
extern intl stringcmp(const uchar *ptr1, const uchar *ptr2);
extern intl symbcmp(SYMB_TYPE *symb1, SYMB_TYPE *symb2);
extern void updref(colt mrksco, rowt mrksro, colt mrkeco, rowt mrkero,
                    colt coldiff, rowt rowdiff);
extern void updroi(uchar flags);


/* internal functions */

#if TRUE
/*static void alnst1(uchar mask, uchar bits);*/
/*static BOOL check_range(SLR *first, SLR *second);*/
static intl copyslot(dochandle fromwindow, colt cs_oldcol, rowt cs_oldrow, 
                     dochandle towindow,   colt tcol,      rowt trow, 
                     colt coldiff, rowt rowdiff, BOOL update_refs,
                     BOOL tree_mode);
/*static intl double_sgn(double num);*/
static void remove_deletion(word32 key);
/*static void update_marks(dochandle handle, SLR *slot, colt mrksco, colt mrkeco, rowt mrksro,
                            rowt mrkero, colt coldiff, rowt rowdiff);*/
/*static BOOL set_up_block(void);*/
static BOOL save_block_and_delete(BOOL is_deletion, BOOL do_save);
#endif

#define TREE_SORT_LIMIT 75

/* ----------------------------------------------------------------------- */

#define NO_JUSTIFY 0xFF

/***************************************************
*                                                  *
* check second range is null, or comes after first *
*                                                  *
***************************************************/

static BOOL
check_range(SLR *first, SLR *second)
{
    if(second->col == NO_COL)
        return(TRUE);

    return((second->col >= first->col)  &&  (second->row >= first->row));
}


extern BOOL
bad_reference(colt tcol, rowt trow)
{
    return((tcol == NO_COL)  ||  bad_col(tcol)  ||  bad_row(trow));
}


/* 
do the hard work in a replicate. Called by Cfunc, BRRfunc, BRDfunc
*/

static void
do_the_replicate(SLR *src_start, SLR *src_end,
                 SLR *res_start, SLR *res_end)
{
    colt src_col, tcol;
    rowt src_row, trow;
    dochandle cdoc = current_document_handle();
    intl errorval = 1;
    word32 slot_count, slot_pos;

    if(protected_slot_in_range(res_start, res_end))
        return;

    slot_count = ((word32) src_end->col - src_start->col + 1) *
                 ((word32) src_end->row - src_start->row + 1) *
                 ((word32) res_end->col - res_start->col + 1) *
                 ((word32) res_end->row - res_start->row + 1);
    slot_pos = 0;

    escape_enable();

    /* each column in source */
    for(src_col = src_start->col;
        !ctrlflag  &&  (src_col <= src_end->col);
        src_col++)
        {
        /* each row in source */
        for(src_row = src_start->row;
            !ctrlflag  &&  (src_row <= src_end->row);
            src_row++)
            {
            /* target block is already set up as current block, just need to initialize */
            init_block(res_start, res_end);

            while(!ctrlflag  &&  next_in_block(DOWN_COLUMNS))
                {
                tcol = in_block.col + src_col - src_start->col;
                trow = in_block.row + src_row - src_start->row;

                actind(ACT_COPY, (intl) ((100l * slot_pos++) / slot_count));

                if(tcol >= numcol)
                    if(!createcol(tcol))
                        {
                        errorval = ERR_NOROOM;
                        break;
                        }

                /* copy the slot but not to itself */

                if((src_row != trow)  ||  (src_col != tcol))
                    {
                    errorval = copyslot(cdoc, src_col, src_row,
                                        cdoc, tcol, trow, 
                                        tcol - src_col, trow - src_row, TRUE,
                                        slot_count <= TREE_SORT_LIMIT);

                    if(errorval < 0)
                        break;
                    }
                }

            if(errorval < 0)
                {
                reperr_null(errorval);
                break;
                }
            }
        }

    actind_end();

    slot_in_buffer = FALSE;

    global_recalc = recalc_bit = out_screen = TRUE;
    filealtered(TRUE);

    escape_disable();
}


static void
do_fill(BOOL down)
{
    SLR src_start, src_end;
    SLR res_start, res_end;

    if(xf_inexpression)
        {
        reperr_null(ERR_EDITINGEXP);
        return;
        }

    if(!mergebuf_nocheck())
        return;

    if(!MARKER_DEFINED())
        reperr_null((blkstart.col != NO_COL)
                            ? ERR_NOBLOCKINDOC
                            : ERR_NOBLOCK);

    if( (blkend.col == NO_COL)  ||
        (down ? (blkend.row == blkstart.row) : (blkend.col == blkstart.col)))
        {
        reperr_null(ERR_BAD_MARKER);
        return;
        }

    src_start = blkstart;

    if(down)
        {
        src_end.col = blkend.col;
        src_end.row = blkstart.row;

        res_start.col = blkstart.col;
        res_start.row = blkstart.row + 1;

        res_end.col = blkstart.col;
        res_end.row = blkend.row; 
        }
    else    
        {
        src_end.col = blkstart.col;
        src_end.row = blkend.row;

        res_start.col = blkstart.col + 1;
        res_start.row = blkstart.row;

        res_end.col = blkend.col;
        res_end.row = blkstart.row;
        }

    do_the_replicate(&src_start, &src_end,
                     &res_start, &res_end);
}


extern void
ReplicateDown_fn(void) 
{
    do_fill(TRUE);
}


extern void
ReplicateRight_fn(void) 
{
    do_fill(FALSE);
}


/************
*           *
* replicate *
*           *
************/

extern void
Replicate_fn(void)
{
    uchar array[LIN_BUFSIZ];
    SLR src_start, src_end;
    SLR res_start, res_end;
    uchar *ptr;

    if(xf_inexpression)
        {
        reperr_null(ERR_EDITINGEXP);
        return;
        }

    if(!mergebuf_nocheck())
        return;

    /* get two blocks from dialog box:
     *      first block in src_start, src_end
     *      second block in blkstart, blkend
    */
    (void) init_dialog_box(D_REPLICATE);

    /* write current block into the source range */

    if(MARKER_DEFINED())
        {
        ptr = array;

        ptr += writeref(ptr, 0, blkstart.col, blkstart.row);

        if(blkend.col == NO_COL)
            blkend = blkstart;

        if((blkend.col != blkstart.col)  ||  (blkend.row != blkstart.row))
            {
            *ptr++ = SPACE;
            ptr += writeref(ptr, 0, blkend.col, blkend.row);
            }

        if(!str_set(&d_replicate[0].textfield, array))
            return;
        }

    /* write current position into the target range */

    writeref(array, 0, curcol, currow);

    if(!str_set(&d_replicate[1].textfield, array))
        return;

    while(dialog_box(D_REPLICATE))
        {
        /* get source range */

        buff_sofar = (uchar *) d_replicate[0].textfield;
        if(buff_sofar == NULL)
            buff_sofar = UNULLSTR;
        src_start.col = getcol();
        src_start.row = getsbd()-1;
        src_end.col   = getcol();
        src_end.row   = getsbd()-1;

        /* get target range */

        buff_sofar = (uchar *) d_replicate[1].textfield;
        if(buff_sofar == NULL)
            buff_sofar = UNULLSTR;
        res_start.col = getcol();
        res_start.row = getsbd()-1;
        res_end.col   = getcol();
        res_end.row   = getsbd()-1;

        /* check first markers */

        if( bad_reference(res_start.col, res_start.row) ||
            bad_reference(src_start.col, src_start.row))
            {
            reperr_null(ERR_BAD_SLOT);
            continue;
            }

        /* make sure second marker is sensible */

        if( res_end.col == NO_COL)
            res_end = res_start;

        if( src_end.col == NO_COL)
            src_end = src_start;

        /* check not column of columns or row of rows */

        if( ((res_end.col-res_start.col > 0)  &&  (src_end.col-src_start.col > 0))  ||
            ((res_end.row-res_start.row > 0)  &&  (src_end.row-src_start.row > 0))  )
            {
            reperr_null(ERR_BAD_RANGE);
            continue;
            }

        /* check ranges point in right direction */
        if( !check_range(&res_start, &res_end)  ||
            !check_range(&src_start, &src_end)  )
            {
            reperr_null(ERR_BAD_RANGE);
            continue;
            }

        do_the_replicate(&src_start, &src_end,
                         &res_start, &res_end);

        out_rebuildvert = out_rebuildhorz = TRUE;

        if(dialog_box_ended())
            break;
        }
}


/*********************************************************
*                                                        *
* createslot at tcol, trow and copy srcslot to it        *
* update slot references, adding on coldiff and rowdiff  *
* works between windows, starts and finishes in towindow *
*                                                        *
*********************************************************/

#define isignbit(var) (var < 0 ? 1 : 0)

static intl
copyslot(dochandle fromwindow, colt cs_oldcol, rowt cs_oldrow, 
         dochandle towindow, colt tcol, rowt trow,
         colt coldiff, rowt rowdiff, BOOL update_refs, BOOL tree_mode)
{
    slotp newslot, oldslot;
    uchar *newtext, *oldtext;
    intl slotlen;
    uchar type;
    colt cref;
    rowt rref;
    intl res;

    oldslot = travel_externally(fromwindow, cs_oldcol, cs_oldrow);

    select_document_using_handle(towindow);

    tracef6("copyslot((%d %d %d) (%d %d %d)", fromwindow, cs_oldcol, cs_oldrow, towindow, tcol, trow);
    tracef4(" %d %d %s) oldslot = &%p\n", coldiff, rowdiff, trace_boolstring(update_refs), oldslot);
    #if TRACE && FALSE
    tracef0("[copyslot: source slot:]\n");
    trace_system("memory b &%p + 70", oldslot);
    #endif

    if(!oldslot)
        {
        graph_draw_tree_removeslot(tcol, trow);

        /* force in blank slot */
        res = createhole(tcol, trow);

        graph_send_slot(tcol, trow);

        return(res ? 1 : ERR_NOROOM);
        }

    type    = oldslot->type;
    slotlen = slotcontentssize(oldslot);

    newslot = travel(tcol, trow);
    if(newslot  &&  (newslot->flags & SL_REFS))
        graph_draw_tree_removeslot(tcol, trow);

    newslot = createslot(tcol, trow, slotlen, type);    /* was problem with moving */
    if(!newslot)
        return(ERR_NOROOM);

    tracef3("[copyslot: created a slot &%p of type %d, slotlen %d]\n", newslot, type, slotlen);

    oldslot = travel_externally(fromwindow, cs_oldcol, cs_oldrow);

    newslot->type    = oldslot->type;
    newslot->flags   = oldslot->flags;
    newslot->justify = oldslot->justify;

    switch(newslot->type)
        {
        case SL_TEXT:
            oldtext = oldslot->content.text;
            newtext = newslot->content.text;
            break;

        case SL_PAGE:
            newslot->content.page.condval = oldslot->content.page.condval;
            return(1);

    /*  case SL_NUMBER: */
        default:
            newslot->content.number.result = oldslot->content.number.result;
            newslot->content.number.format = oldslot->content.number.format;
            oldtext = oldslot->content.number.text;
            newtext = newslot->content.number.text;
            break;
        }

    /* copy over formula and update it */
    memcpy(newtext, oldtext, slotlen);
    #if TRACE && FALSE
    tracef0("[copyslot: copied slot:]\n");
    trace_system("memory b &%p + 70", newslot);
    #endif

    if(update_refs)
        {
        my_init_ref(newslot->type);

        while((newtext = my_next_ref(newtext, newslot->type)) != NULL)
            {
            newtext += sizeof(docno);

            cref = (colt) talps(newtext, (intl) sizeof(colt));

            if(!abs_col(cref))
                {
                colt tcol = cref;

                cref += coldiff;

                /* check for change in sign bit or other bits
                MRJC altered 17.4.89 for new # refs in dsums */
                if( (isignbit(tcol) != isignbit(cref))  ||
                    (tcol & BADCOLBIT)  ||  abs_col(cref))
                    cref = (tcol & COLNOBITS) | BADCOLBIT;
                }

            splat(newtext, (word32) cref, (intl) sizeof(colt));

            newtext += sizeof(colt);

            rref = (rowt) talps(newtext, (intl) sizeof(rowt));

            if(!abs_row(rref))
                {
                rowt trow = rref;

                rref += rowdiff;

                /* check for unwanted change in bits
                MRJC altered 17.4.89 for new # refs in dsums */
                if( (isignbit(trow) != isignbit(rref))  ||
                    (trow & BADROWBIT)  ||  abs_row(rref))
                    rref = (trow & ROWNOBITS) | BADROWBIT;
                }

            splat(newtext, (word32) rref, (intl) sizeof(rowt));
            newtext += sizeof(rowt);
            }
        }

    switch(newslot->type)
        {
        case SL_TEXT:
            if(draw_tree_str_insertslot(tcol, trow, tree_mode) < 0)
                tree_switchoff();
            break;

        #if FALSE   /* SL_PAGE never gets here */
        case SL_PAGE:
            break;
        #endif

        default:
            newslot->content.number.result.value = oldslot->content.number.result.value;
            newslot->content.number.format       = oldslot->content.number.format;
            if(tree_exp_insertslot(tcol, trow, tree_mode) < 0)
                tree_switchoff();
            break;
        }

    graph_send_slot(tcol, trow);

    return(1);
}


/****************
*               *
*  left align   *
*               *
****************/

extern void
LeftAlign_fn(void)
{
    alnsto(J_LEFT, (uchar) 0xFF, (uchar) 0);
}


/****************
*               *
* centre align  *
*               *
****************/

extern void
CentreAlign_fn(void)
{
    /* if block marked, do each slot in block */
    alnsto(J_CENTRE, (uchar) 0xFF, (uchar) 0);
}


/****************
*               *
*  right align  *
*               *
****************/

extern void
RightAlign_fn(void)
{
    alnsto(J_RIGHT, (uchar) 0xFF, (uchar) 0);
}


/****************
*               *
*  free align   *
*               *
****************/

extern void
FreeAlign_fn(void)
{
    alnsto(J_FREE, (uchar) 0xFF, (uchar) 0);
}


/************
*           *
* lcr align *
*           *
************/

extern void
LCRAlign_fn(void)
{
    alnsto(J_LCR, (uchar) 0xFF, (uchar) 0);
}


/************************************
*                                   *
* default format - set format to 0  *
*                                   *
************************************/

extern void
DefaultFormat_fn(void)
{
    alnsto(NO_JUSTIFY, 0, 0);
}


/********************************************
*                                           *
* mark the area of a given block for output *
* MRJC 13.7.89                              *
*                                           *
********************************************/

static void
mark_block_output(const SLR *bs)
{
    intl offset;

    if(bs->col == NO_COL)
        {
        out_currslot = TRUE;
        return;
        }

    if((offset = schrsc(bs->row)) != NOTFOUND)
        mark_to_end(offset);
    else
        out_screen = TRUE;
}


/********************************************************************
*                                                                   *
* set a marked block or the current slot with a format              *
* the justify byte is put into the justify field                    *
* in the slot (unless NO_JUSTIFY)                                   *
* mask & bits give the information to change the format             *
* some operations set bits, some clear, and some toggle bits        *
* this is done by masking out some bits and then exclusive-oring    *
* eg to clear  - mask with 0 and eor with 0                         *
*    to set    - mask with 0 and eor with 1                         *
*    to toggle - mask with 1 and eor with 1                         *
*                                                                   *
********************************************************************/

static void
alnsto_block(uchar justify, uchar mask, uchar bits, const SLR *bs, const SLR *be)
{
    slotp tslot;

    if(!mergebuf_nocheck())
        return;

    mark_block_output(bs);

    init_block(bs, be);

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        filealtered(TRUE);

        switch(justify)
            {
            case NO_JUSTIFY:
                break;

            case PROTECTED:
                /* set protected bit */
                orab(tslot->justify, PROTECTED);
                break;

            case J_BITS:        
                /* clear protected bit */
                andab(tslot->justify, J_BITS);
                break;

            default:
                /* set justify bits, retaining protected status */
                tslot->justify = (tslot->justify & CLR_J_BITS) | justify;
                break;
            }

        switch(tslot->type)
            {
            case SL_NUMBER:
            case SL_ERROR:
            case SL_STRVAL:
            case SL_INTSTR:
            case SL_DATE:
            case SL_BAD_FORMULA:
                 tslot->content.number.format =
                (tslot->content.number.format & mask) ^ bits;
                break;

            default:
                break;
            }
        }
}


extern void
alnsto(uchar justify, uchar mask, uchar bits)
{
    alnsto_block(justify, mask, bits, &blkstart, &blkend);
}


/*****************************************************************
*                                                                *
* alnst1 is like alnsto                                          *
* for sign minus, sign bracket and set decimal places            *
* if the slot is currently using option page defaults then those *
* defaults are copied into the slot                              *
*                                                                *
*****************************************************************/

static void
alnst1(uchar mask, uchar bits)
{
    slotp tslot;

    mark_block_output(&blkstart);

    init_marked_block();

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        filealtered(TRUE);

        switch(tslot->type)
            {
            uchar format;

            case SL_NUMBER:
            case SL_ERROR:
            case SL_STRVAL:
            case SL_INTSTR:
            case SL_DATE:
            case SL_BAD_FORMULA:
                    format = tslot->content.number.format;

                    if((format & F_DCP) == 0)
                        {
                        format |= (F_DCP | F_BRAC);
                        if(d_options_MB == 'M')
                            format &= (uchar) ~F_BRAC;
                        format &= ~F_DCPSID;
                        format |= get_dec_field_from_opt();
                        }

                    format &= mask;
                    format ^= bits;

                    tslot->content.number.format = format;
                    break;
            default:
                    break;
            }
        }
}


/************************************
*                                   *
* leading character                 *
* toggle lead char bit, set dcp bit *
*                                   *
************************************/

extern void
LeadingCharacters_fn(void)
{
    alnsto(NO_JUSTIFY, 0xFF /* & (uchar) ~F_DCP */ ,  F_LDS);
}


/************************************
*                                   *
* trailing character                *
* toggle lead char bit, set dcp bit *
*                                   *
************************************/

extern void
TrailingCharacters_fn(void)
{
    alnsto(NO_JUSTIFY, 0xFF /* & (uchar) ~F_DCP */ ,  F_TRS);
}


extern void
SignBrackets_fn(void)
{
    alnst1(0xFF & (uchar) ~F_BRAC, F_BRAC);
}


extern void
SignMinus_fn(void)
{
    alnst1(0xFF & (uchar) ~F_BRAC,  0);
}


extern void
DecimalPlaces_fn(void)
{
    while(dialog_box(D_DECIMAL))
        {
        alnst1((uchar) ~F_DCPSID, (d_decimal[0].option == 'F')
                                            ? (uchar) 0xF
                                            : d_decimal[0].option-'0');

        if(dialog_box_ended())
            break;
        }
}


/*****************************************************
*                                                    *
* do RETfunc, worrying about Insert on return status *
*                                                    *
*****************************************************/

extern void
Return_fn(void)
{
    uchar *ptr;
    slotp tslot;

    #if !defined(SPELL_OFF)
    /* check word perhaps */
    if(!xf_inexpression  &&  !in_dialog_box  &&  (lecpos > 0))
        {
        ptr = linbuf + lecpos;

        if(!spell_iswordc(*ptr)  &&  spell_iswordc(*(ptr-1)))
            check_word();
        }
    #endif

    if(d_options_IR == 'Y')
        {
        SplitLine_fn(); 

        /* remove justify bit from current slot */
        if( !xf_inexpression  &&  !in_dialog_box  && 
            ((tslot = travel_here()) != NULL))
                {           
                /* shirley we don't have to worry about PROTECTED bit,
                    cos if protected what are we doing poking slot?
                */
                uchar justify = tslot->justify & J_BITS;
                if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
                    /* set justify bits, retaining protected status */
                    tslot->justify = (tslot->justify & CLR_J_BITS) | J_FREE;
                }
        }

    /* finish expression editing or at row if at end of text */

    if(xf_inexpression)
        {
        if(buffer_altered)
            recalc_bit = TRUE;

        merexp();
        endeex();
        }
    else
        {
        if(!mergebuf())
            return;

        lecpos = lescrl = 0;

        tracef2("[Return_fn currow: %d, numrow: %d]\n", currow, numrow);

        if(currow + 1 >= numrow)
            {
            /* force blank slot in */
            if(!createhole(curcol, currow + 1))
                {
                *linbuf = '\0';
                reperr_null(ERR_NOROOM);
                return;
                }

            out_rebuildvert = TRUE;
            filealtered(TRUE);
            mark_row(currowoffset + 1);
            }

        mark_row_praps(currowoffset, OLD_ROW);

        CursorDown_fn();
        }
}


/***********************
*                      *
* convert string:      *
* converts \ to CMDLDI *
* \\ to \              *
* |x to ctrl-x         *
* || to |              *
*                      *
***********************/

extern void
prccml(uchar *array)
{
    uchar *from = array;
    uchar *to   = array;
    BOOL instring = FALSE;

    while(*from)
        {
        switch(*from)
            {
            case '|':
                if(instring)
                    {
                    if(from[1] == '\"')
                        from++;
                    *to++ = *from++;
                    break;
                    }

                from++;

                if(*from == '|'  ||  *from == '\"')
                    *to++ = *from;
                elif(isalpha(*from))
                    *to++ = toupper(*from) - 'A' + 1;

                from++;
                break;

            case '\"':
                /* remove leading spaces before strings */
                if(!instring)
                    while(to > array && *(to-1) == SPACE)
                        to--;

                instring = !instring;
                from++;
                /* and remove trailing spaces after strings */
                if(!instring)
                    while(*from == SPACE)
                        from++;

                break;

            case '\\':
                if(!instring)
                    {
                    from++;
                    if(*from == '\\')
                        *to++ = *from++;
                    else
                        *to++ = CMDLDI;
                    break;
                    }

                /* deliberate fall through */

            default:
                *to++ = *from++;
                break;
            }
        }

    *to = '\0';
}


/****************
*               *
*  define key   *
*               *
****************/

extern void
DefineKey_fn(void)
{
    uchar array[LIN_BUFSIZ];
    word32 key;
    intl res;

    while(dialog_box(D_DEFKEY)) /* so we can see prev defn'n */
        {
        if(!d_defkey[1].textfield)
            *array = '\0';
        else
            strcpy((char *) array, d_defkey[1].textfield);

        prccml(array);      /* convert funny characters in array */

        key = (word32) d_defkey[0].option;

        /* remove old definition */
        delete_from_list(&first_key, (word32) d_defkey[0].option);

        /* add new one */
        res = add_to_list(&first_key, (word32) d_defkey[0].option, array, &res);

        if(res <= 0)
            {
            dialog_box_end();
            reperr_null(res ? res : ERR_NOROOM);
            }

        if(dialog_box_ended())
            break;
        }
}


/************************
*                       *
*  define function key  *
*                       *
************************/

extern void
DefineFunctionKey_fn(void)
{
    uchar array[LIN_BUFSIZ];
    word32 key = (word32) 0;
    word32 whichkey;
    intl res;

    while(dialog_box(D_DEF_FKEY))
        {
        if(!d_def_fkey[1].textfield)
            *array = '\0';
        else
            strcpy((char *) array, d_def_fkey[1].textfield);

        prccml(array);      /* convert funny characters in array */

        whichkey = (word32) d_def_fkey[0].option;

#if MS
        if(whichkey < 10)
            key = (word32) FUNC     - (whichkey);
        elif(whichkey < 40)
            key = (word32) SFUNC    - (whichkey-10);
#elif ARTHUR || RISCOS
        if(whichkey < 9)
            key = (word32) FUNC     - (whichkey);
        elif(whichkey < 12)
            key = (word32) FUNC10   - (whichkey-9);
        elif(whichkey < 21)
            key = (word32) SFUNC    - (whichkey-12);
        elif(whichkey < 24)
            key = (word32) SFUNC10  - (whichkey-21);
        elif(whichkey < 33)
            key = (word32) CFUNC    - (whichkey-24);
        elif(whichkey < 36)
            key = (word32) CFUNC10  - (whichkey-33);
        elif(whichkey < 45)
            key = (word32) CSFUNC   - (whichkey-36);
        elif(whichkey < 48)
            key = (word32) CSFUNC10 - (whichkey-45);
#if ARTHUR
        elif(whichkey < 57)
            key = (word32) ALTFUNC  - (whichkey-48);
        elif(whichkey < 60)
            key = (word32) ALTFUNC10 - (whichkey-57);
#endif
#endif

        /* remove old definition */
        delete_from_list(&first_key, key);

        /* add new definition */
        res = add_to_list(&first_key, key, array, &res);

        if(res <= 0)
            {
            dialog_box_end();
            reperr_null(res ? res : ERR_NOROOM);
            }

        if(dialog_box_ended())
            break;
        }
}


/********************
*                   *
*  define command   *
*                   *
********************/

extern void
DefineCommand_fn(void)
{
    char array[LIN_BUFSIZ];
    word32 key;
    char *src;
    char *dst;
    intl ch, res;

    while(dialog_box(D_DEF_CMD))
        {
        src = d_def_cmd[0].textfield;

        if(!src  ||  (strlen(src) > (sizeof(word32) * 8 / 5)))
            {
            bleep();
            continue;
            }

        dst = array;
        key = 0;

        do  {
            ch = *src++;
            ch = toupper(ch);
            if((ch >= 'A')  &&  (ch <= 'Z'))
                key = (key << 5) + ((word32) ch - 'A');
            else
                ch = '_';

            *dst++ = ch;
            }
        while(ch != '_');

        /* remove old definition */
        delete_from_list(&first_command_redef, key);

        src = d_def_cmd[1].textfield;

        if(src)
            {
            do  {
                ch = *src++;
                *dst++ = toupper(ch);   /* including terminating NUL */
                }
            while(ch);

            /* add new definition */
            res = add_to_list(&first_command_redef, key, array, &res);

            if(res <= 0)
                {
                dialog_box_end();
                reperr_null(res ? res : ERR_NOROOM);
                }
            }

        if(dialog_box_ended())
            break;
        }
}


/*********
*        *
* ESCAPE *
*        *
*********/

extern void
Escape_fn(void)
{
    ack_esc();

    if(xf_inexpression)
        endeex();
    #if ARTHUR
    else
        OSCommand_fn();
    #endif
}


/*******************************************************
*                                                      *
* pause until key press                                *
* length of pause is given by dialog box               *
* if 'p' pressed, pause until next key press           *
*                                                      *
* should this routine understand and execute commands? *
*                                                      *
*******************************************************/

extern void
Pause_fn(void)
{
    #if !RISCOS
    time_t starttime, lengthtime;
    #endif

    if(!dialog_box(D_PAUSE))
        return;

    dialog_box_end();

    #if !defined(HEADLINE_OFF)
    display_heading(-2);        /* put pause in menu bar */

    xf_drawmenuheadline = TRUE; /* restore headline afterwards */
    #endif

    #if RISCOS
    riscdialog_dopause(d_pause[0].option);
    #else
    clearkeyboardbuffer();

    starttime   = time(NULL);
    lengthtime  = (time_t) d_pause[0].option;

    while((time(NULL) - starttime) < lengthtime)
        {
        if(keyinbuffer())
            {
            intl c = rdch(FALSE, FALSE);

            clearkeyboardbuffer();

            if(toupper(c) == 'P')
                rdch(FALSE, FALSE);

            break;
            }
        }
    #endif
}


/************************************
*                                   *
* insert reference in editing line  *
*                                   *
************************************/

extern void
InsertReference_fn(void)
{
    uchar array[20];

    if(!xf_inexpression)
        {
        bleep();
        return;
        }

    /* expand column then row into array */
    writeref(array, 0, curcol, currow);

    insert_string(array, FALSE);
}


extern void
mark_to_end(coord rowonscr)
{
    out_below = TRUE;

    if( rowonscr < 0)
        rowonscr = 0;

    if( rowtoend > rowonscr)
        rowtoend = rowonscr;
}


/*************************************
*                                    *
* update references for inserted row *
* U_ONE = this col only              *
* U_ALL = all cols                   *
*                                    *
*************************************/

extern void
updroi(uchar flags)
{
    colt scol, ecol;

    rebnmr();

    scol = (flags == U_ONE) ? curcol : 0;
    ecol = (flags == U_ONE) ? scol   : LARGEST_COL_POSSIBLE;

    /* rows in inserted columns move down */
    updref(scol, currow, ecol, LARGEST_ROW_POSSIBLE, (colt) 0, (rowt) 1);
}


/************************************
*                                   *
* update references for deleted row *
* 0x80 = this col only              *
* 0xC0 = all cols                   *
*                                   *
************************************/

extern /* static */ void
delrwb(uchar flags)
{
    colt scol, ecol;

    rebnmr();

    scol = (flags == U_ONE) ? curcol : 0;
    ecol = (flags == U_ONE) ? scol   : LARGEST_COL_POSSIBLE;

    /* anything pointing to deleted slots in this row become bad */
    updref(scol, currow,     ecol, currow,               BADCOLBIT, (rowt) 0);

    /* rows in all those columns move up */
    updref(scol, currow + 1, ecol, LARGEST_ROW_POSSIBLE, (colt) 0, (rowt) -1);

    out_rebuildvert = TRUE;
    filealtered(TRUE);
}


/*************
*            *
* split line *
*            *
*************/

extern void
SplitLine_fn(void)
{
    intl bufflength, splitpoint;
    uchar tempchar;
    slotp tslot;
    BOOL actually_splitting = FALSE;
    colt tcol;
    rowt trow;

    if(xf_inexpression  ||  in_dialog_box  ||  !mergebuf())
        return;

/*  if((tslot = travel(curcol, currow)) != NULL  &&  tslot->type != SL_TEXT)
        return;
*/
    if(protected_slot(curcol, currow))
        return;

    tslot = travel_here();

    if(!tslot  ||  (tslot->type == SL_TEXT))
        {
        actually_splitting = TRUE;

        bufflength = strlen((char *) linbuf);
        splitpoint = min(bufflength, lecpos);
    
        tempchar = linbuf[splitpoint];  
        linbuf[splitpoint] = '\0';
    
        if( dspfld_from == -1)
            dspfld_from = splitpoint;

        buffer_altered = slot_in_buffer = TRUE;
    
        if(!mergebuf())
            return;
    
        /* leave lying around for merging later on */
        linbuf[splitpoint] = tempchar;
        memmove(linbuf, linbuf + splitpoint, (unsigned) (bufflength - splitpoint + 1));
        }

    /* if insert on wrap is row insert slots in this row for each column to the right,
     * and on the next row for each each column to the left, and for this one if we'll need
     * to move the rest of the line down
    */
    if(iowbit)
        {
        for(tcol = 0; tcol < numcol; ++tcol)
            {   
            trow = currow;
            if(tcol < curcol)
                ++trow;
            elif((tcol == curcol)  &&  actually_splitting)
                ++trow;

            if(!insertslotat(tcol, trow))
                {
                /* remove those added */
                while(--tcol >= 0)
                    {
                    trow = currow;
                    if(tcol < curcol)
                        ++trow;
                    elif((tcol == curcol)  &&  actually_splitting)
                        ++trow;

                    killslot(tcol, trow);
                    }

                return;
                }
            }

        mark_to_end(currowoffset);
        out_rebuildvert = TRUE;

        rebnmr();

        /* columns to right of curcol have moved down */
        updref(curcol, currow, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, (colt) 0, (rowt) 1);

        if(curcol > 0)
            /* columns to left of curcol have moved down */
            updref(0, currow+1, curcol-1, LARGEST_ROW_POSSIBLE, (colt) 0, (rowt) 1);

        graph_send_block(0, currow, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE);

        if(been_error)
            return;
        }
    /* insert on wrap is column, insert in just this column */
    else
        {   
        if(actually_splitting)
            inc(currow);

        InsertRowInColumn_fn();

        if(actually_splitting)
            dec(currow);
        }

    if(actually_splitting)
        {
        inc(currow);

        buffer_altered = slot_in_buffer = TRUE;
    
        if(!mergebuf_nocheck())
            return;

        dec(currow);
        }
} 


/*************
*            *
* join lines *
*            *
*************/

extern void
JoinLines_fn(void)
{
    slotp thisslot, nextslot, tslot;
    intl thislen, nextlen;
    BOOL actually_joining = TRUE;
    uchar temparray[LIN_BUFSIZ];
    BOOL allblank = TRUE;
    colt tcol;
    rowt trow;

    if(xf_inexpression  ||  in_dialog_box  ||  !mergebuf_nocheck())
        return;

    /* can't join this slot to next one if this slot exists and not null or text */
    thisslot = travel_here();

    if(thisslot  &&  (thisslot->type != SL_TEXT))
        return;

    /* can only join non-textual next slot if this slot null */
    nextslot = travel(curcol, currow + 1);
    if(nextslot  &&  (nextslot->type != SL_TEXT))
        {
        if(thisslot)
            return;

        actually_joining = FALSE;       
        }

    if(protected_slot_in_block(curcol, currow, curcol, currow+1))
        return;

    if(actually_joining)
        {
        thislen = strlen((char *) linbuf);
        memmove((char *) temparray, (char *) linbuf, (unsigned) thislen);
    
        inc(currow);

        filbuf();
    
        nextlen = strlen((char *) linbuf);
        if(thislen + nextlen >= MAXFLD)
            {
            dec(currow);
            slot_in_buffer = FALSE;
            filbuf();
            reperr_null(ERR_LINETOOLONG);
            return;
            }
        
        memmove((char *) (linbuf+thislen), (char *) linbuf, (unsigned) nextlen+1);
        memmove((char *) linbuf, (char *) temparray, (unsigned) thislen);
        buffer_altered = slot_in_buffer = TRUE;
        dec(currow);
    
        if(!mergebuf())
            return;
        }

    dont_save = TRUE;

    /* wrap set to rows? */
    if(iowbit)
        {
        /* look to see if all this row bar the current column is blank.
         * If so joinlines can delete a whole row, perhaps
         * be careful of numeric slots masquerading as blanks
         * for slots to left needs to split on following line
        */
        allblank = TRUE;

        for(tcol = 0; tcol < numcol; tcol++)
            {
            trow = currow;
            if(tcol < curcol)
                ++trow;
            elif((tcol == curcol)  &&  actually_joining)
                continue;           

            tslot = travel(tcol, trow);
            if(!isslotblank(tslot)  ||  (tslot  &&  (tslot->type != SL_TEXT)))
                {
                allblank = FALSE;
                break;
                }
            }

        if(allblank)
            {
            for(tcol = 0; tcol < numcol; tcol++)
                {
                trow = currow;
                if(tcol < curcol)
                    ++trow;
                elif((tcol == curcol) && actually_joining)
                    ++trow;

                killslot(tcol, trow);
                }

            rebnmr();                           /* check numrow */

            /* columns to left of curcol */
            /* mark references to deleted slots bad */
            updref(0, currow+1, curcol-1, currow, BADCOLBIT, (rowt) 0);

            /* update refs to slots moving up */
            updref(0, currow+1, curcol-1, LARGEST_ROW_POSSIBLE, (colt) 0, (rowt) -1);

            /* curcol and columns to right */
            /* mark references to deleted slots bad */
            updref(curcol, currow, LARGEST_COL_POSSIBLE, currow, BADCOLBIT, (rowt) 0);

            /* update refs to slots moving up */
            updref(curcol, currow, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, (colt) 0, (rowt) -1);

            /* bit of a panic dump */
            graph_send_block(0, currow, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE);

            out_rebuildvert = xf_flush = TRUE;
            filealtered(TRUE);
            mark_to_end(currowoffset-1);
            }
        elif(actually_joining)
            {
            inc(currow);
            DeleteRowInColumn_fn();     /* get rid of slot */
            InsertRowInColumn_fn();     /* and leave a hole */
            dec(currow);
            }
        }
    else
        {
        if(actually_joining)
            inc(currow);

        DeleteRowInColumn_fn();

        if(actually_joining)
            dec(currow);
        }

    dont_save = FALSE;
}



/***********************
*                      *
* delete row in column *
*                      *
***********************/

extern void
DeleteRowInColumn_fn(void)
{
    colt tcol = curcol;
    rowt trow = currow;

    xf_flush = TRUE;

    if(protected_slot(tcol, trow))
        return;

    /* save to list first */
    if(!dont_save)
        save_words(linbuf);

    if(!mergebuf_nocheck())
        {
        buffer_altered = slot_in_buffer = FALSE;
        return;
        }

    graph_draw_tree_removeslot(tcol, trow);

    killslot(tcol, trow);

    delrwb(U_ONE);

    #if RISCOS
    graph_send_split_blocks(tcol, trow, tcol+1, LARGEST_ROW_POSSIBLE);
    #endif

    mark_to_end(currowoffset);
}


/****************
*               *
* insert column *
*               *
****************/

extern void
InsertColumn_fn(void)
{
    colt tcol = curcol;

    if(!mergebuf())
        return;

    if(!inscolentry(tcol))
        return;

    rebnmr();

    updref(tcol, (rowt) 0, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, 1, (rowt) 0);

    #if RISCOS
    graph_send_split_blocks(tcol, 0, tcol+1, LARGEST_ROW_POSSIBLE);
    #endif

    lecpos = lescrl = 0;

    xf_flush = out_rebuildhorz = xf_drawcolumnheadings = out_screen = TRUE;
    filealtered(TRUE);
}


/****************
*               *
* delete column *
*               *
****************/

extern void
DeleteColumn_fn(void)
{
    colt tcol = curcol;
    SLR bs, be;
    BOOL res;
    dochandle block_handle = blkdochandle;

    if(!mergebuf())
        return;

    /* don't delete if only column */
    if((numcol <= 1)  ||  (tcol >= numcol))
        return;

    /* don't delete if only visible column */
    if(all_widths_zero(tcol, tcol))
        return;

    if(protected_slot_in_block(tcol, (rowt) 0, tcol, numrow-1))
        return;

    bs = blkstart;
    be = blkend;

    blkdochandle = current_document_handle();
    blkstart.col = tcol;
    blkstart.row = (rowt) 0;
    blkend.col   = tcol;
    blkend.row   = numrow - 1;

    res = save_block_and_delete(TRUE, TRUE);

    blkdochandle = block_handle;
    blkstart     = bs;
    blkend       = be;

    if(res)
        {
        /* data deleted, now close up the emptied column */
        delcolentry(tcol, 1);
    
        /* update for columns moving left */
        updref(tcol+1, (rowt) 0, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, (colt) -1, (rowt) 0);
    
        /* and send the data NOW */
        graph_send_split_blocks(tcol, 0, tcol+1, LARGEST_ROW_POSSIBLE);

        chknlr(tcol, currow);
        lecpos = 0;

        xf_flush = out_rebuildhorz = xf_drawcolumnheadings = out_screen = TRUE;
        filealtered(TRUE);
        }
}


/****************************************************************************
block delete, copy, move
****************************************************************************/

/***********************************************
*                                              *
* see if a slot reference (SLR) needs updating *
*                                              *
***********************************************/

static void
update_marks(dochandle handle, SLR *slot, colt mrksco, colt mrkeco, rowt mrksro, rowt mrkero,
                colt coldiff, rowt rowdiff)
{
    if(handle != current_document_handle())
        return;

    if((slot->row > numrow-1)  &&  (slot->row < LARGEST_ROW_POSSIBLE))
        slot->row = numrow-1;

    if((slot->col > numcol-1)  &&  (slot->col < LARGEST_COL_POSSIBLE))
        slot->col = numcol-1;

    if( slot->col <= mrkeco  &&  slot->col >= mrksco  &&
        slot->row <= mrkero  &&  slot->row >= mrksro)
        {
        plusab(slot->row, rowdiff);
        plusab(slot->col, coldiff);
        }
}

/**********************************************
*                                             *
* update references in spreadsheet for a move *
* updated for external refs MRJC 30.5.89      *
* updated for draw files MRJC 28.6.89         *
*                                             *
**********************************************/

extern void
updref(colt mrksco, rowt mrksro,
       colt mrkeco, rowt mrkero,
       colt coldiff, rowt rowdiff)
{
    dochandle curhan;
    docno curdoc, doc;
    intl index, num_dep_docs;
    slotp tslot;
    uchar *rptr;

    tracef0("[updref]\n");

    if((num_dep_docs = init_dependent_docs(&curdoc, &index)) != 0)
        {
        curhan = current_document_handle();

        /* loop for each dependent sheet */
        do  {
            do  {
                if(num_dep_docs)
                    {
                    doc = next_dependent_doc(&curdoc, &index);
                    --num_dep_docs;
                    if(check_docvalid(doc) >= 0)
                        {
                        switch_document(doc);
                        break;
                        }
                    }
                doc = curdoc;
                }
            while(num_dep_docs);

            tracef1("[updref for doc: %d]\n", doc);

            /* loop for each linked spreadsheet */

            /* for every slot in spreadsheet */
            init_doc_as_block();

            while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
                {
                /* check for slots which have moved */
                if( (doc == curdoc)             &&
                    (in_block.row >= mrksro)    &&
                    (in_block.row <= mrkero)    &&
                    (in_block.col >= mrksco)    &&
                    (in_block.col <= mrkeco)    )
                    mark_slot_as_moved(tslot);

                /* slot got any SLRs ? */
                if(tslot->flags & SL_REFS)
                    {
                    rptr = (tslot->type == SL_TEXT)
                                   ? tslot->content.text
                                   : tslot->content.number.text;
    
                    /* for each slot reference */
                    my_init_ref(tslot->type);

                    while((rptr = my_next_ref(rptr, tslot->type)) != NULL)
                        {
                        docno dref;
                        colt cref;
    
                        dref = (docno) talps(rptr, sizeof(docno));
                        rptr += sizeof(docno);
    
                        cref = (colt) talps(rptr, sizeof(colt));
                        /* check for document and column range */
                        if( ((!dref && (doc == curdoc)) || (dref && (dref == curdoc))) &&
                            (cref & COLNOBITS) >= mrksco &&
                            (cref & COLNOBITS) <= mrkeco)
                            {
                            rowt rref;
    
                            rref = (rowt) talps(rptr + sizeof(colt),
                                                sizeof(rowt));
    
                            if((rref & ROWNOBITS) >= mrksro &&
                               (rref & ROWNOBITS) <= mrkero)
                                {
                                /* and the row reference, update them */
    
                                if(coldiff != 0)
                                    {
                                    colt upd_newcol;
    
                                    upd_newcol = (coldiff == BADCOLBIT)
                                                    ? (cref | BADCOLBIT)
                                                    : (cref + coldiff);
    
                                    splat(rptr,
                                          (word32) upd_newcol,
                                          sizeof(colt));
                                    }

                                if(rowdiff != 0  &&  coldiff != BADCOLBIT)
                                    splat(rptr + sizeof(colt),
                                          (word32) (rref+rowdiff),
                                          sizeof(rowt));
    
                                filealtered(TRUE);
                                mark_slot(tslot);
                                if(coldiff == BADCOLBIT)
                                    orab(tslot->flags, SL_MUSTRECALC);
                                }
                            }
                        rptr += sizeof(colt) + sizeof(rowt);
                        }
                    }
                }
            }
        while(num_dep_docs);

        /* make sure we are back in original document */
        select_document_using_handle(curhan);
        }

    update_marks(blkdochandle,  &blkanchor, mrksco, mrkeco, mrksro, mrkero, 
                                                    coldiff, rowdiff);
    update_marks(blkdochandle,  &blkstart, mrksco, mrkeco, mrksro, mrkero, 
                                                    coldiff, rowdiff);
    update_marks(blkdochandle,  &blkend,   mrksco, mrkeco, mrksro, mrkero, 
                                                    coldiff, rowdiff);

    update_marks(schdochandle,  &sch_stt,  mrksco, mrkeco, mrksro, mrkero, 
                                                    coldiff, rowdiff);
    update_marks(schdochandle,  &sch_end,  mrksco, mrkeco, mrksro, mrkero, 
                                                    coldiff, rowdiff);

    tree_updref(mrksco, mrksro, mrkeco, mrkero, coldiff, rowdiff);
    #if RISCOS
    draw_updref(mrksco, mrksro, mrkeco, mrkero, coldiff, rowdiff);
    graph_updref(mrksco, mrksro, mrkeco, mrkero, coldiff, rowdiff);
    #endif

    recalc_bit = TRUE;
}


/************
*           *
*  options  *
*           *
************/

extern void
Options_fn(void)
{
    do  {
        (void) dialog_box(D_OPTIONS);

        update_variables();

        filealtered(TRUE);
        }
    while(!dialog_box_ended());
}


/********************
*                   *
*  recalc options   *
*                   *
********************/

extern void
RecalcOptions_fn(void)
{
    uchar old_calc = d_recalc_RC;

    while(dialog_box(D_RECALC))
        {
          if((old_calc == 'N')  &&  (d_recalc_RC != 'N'))
            tree_delete();
        elif((old_calc != 'N')  &&  (d_recalc_RC == 'N'))
            {
            if(tree_build() < 0)
                tree_switchoff();
            else 
                recalc_bit = TRUE;
            }

        old_calc = d_recalc_RC;

        update_variables();

        filealtered(TRUE);

        if(dialog_box_ended())
            break;
        }
}


/*
return the maxmimum change between iterations

returns negative value if error
*/

double
iterations_change(void)
{
    double res = -1.;

    if(str_isblank(d_recalc_RB))
        return(-1.);

    sscanf(d_recalc_RB, "%lf", &res);
    return(res);
}


/****************
*               *
*  recalculate  *
*               *
****************/

extern void
Recalculate_fn(void)
{
    if(!mergebuf_nocheck())
        return;

    recalc_bit = recalc_forced = TRUE;

    #if !RISCOS
    calshe(CALC_DRAW, CALC_RESTART);
    #endif
}


/****************************
*                           *
*  select display colours   *
*                           *
****************************/

extern void
Colours_fn(void)
{
    while(dialog_box(D_COLOURS))
        {
        #if MS
        setcolour(FORE, BACK);
        clearscreen();

        #elif RISCOS

        window_data *wdp = NULL;
        dochandle doc = current_document_handle();

        while((wdp = next_document(wdp)) != NULL)
            {
            select_document(wdp);
            riscos_invalidatemainwindow();
            }

        select_document_using_handle(doc);
        #endif

        if(dialog_box_ended())
            {
            #if RISCOS
            /* must force ourselves to set caret position
             * in order to set new caret colour AFTER killing
             * the menu tree
            */
            xf_acquirecaret = TRUE;
            #endif
            break;
            }
        }
}


/****************
*               *
*  exec a file  *
*               *
****************/

extern void
DoMacroFile_fn(void)
{
    if(!mergebuf())
        return;

    if(in_execfile)
        {
        reperr_null(ERR_BAD_PARM);
        return;
        }

    while(dialog_box(D_EXECFILE))
        {
        if(str_isblank(d_execfile[0].textfield))
            {
            reperr_null(ERR_BAD_NAME);
            continue;
            }
        else
            do_execfile(d_execfile[0].textfield);

        /* bodge required as dialog_box_ended considers this too */
        exec_filled_dialog = FALSE;

        if(dialog_box_ended())
            break;
        }
}


/********************************************************
*                                                       *
* About PipeDream function:                             *
* output copyright message and the registration number  *
*                                                       *
********************************************************/

extern void
About_fn(void)
{
    while(dialog_box(D_ABOUT))
        {
        /* nothing at all */

        if(dialog_box_ended())
            break;
        }
}


/********************
*                   *
*  Leave PipeDream  *
*                   *
********************/

extern void
Quit_fn(void)
{
    intl nmodified;

    if(!mergebuf_all())
        return;

    nmodified = documents_modified();

    if(nmodified)
        #if RISCOS
        if(!riscos_quit_okayed(nmodified))
        #else
        if(!save_existing())        /* <<< pc version needs multiexit ok */
        #endif
            return;

    /* close macro file & stamp it */
    if(macro_recorder_on)
        RecordMacroFile_fn();

    exit(EXIT_SUCCESS); /* trapped by various atexit()ed functions */
}


/************************************************************************
*                                                                       *
* assumes strings are compiled but SLRs will cause havoc                *
* copes with ^? and ^# for single and multiple wild characters in ptr1  *
* difference with other string comparison - ^# matches spaces           *
* returns -1 for ptr1 < ptr2, 0 for equal, 1 for ptr1 > ptr2            *
*                                                                       *
************************************************************************/

extern intl
stringcmp(const uchar *ptr1, const uchar *ptr2)
{
    const uchar *x, *y, *ox, *oy;
    uchar ch;
    BOOL wild_x;
    intl pos_res;

    tracef2("stringcmp('%s', '%s')]\n", ptr1, ptr2);

    y = ptr2;

    /* must skip leading hilites in template string for final rejection */
    while(ishighlight(*ptr1))
        ++ptr1;
    x = ptr1 - 1;

STAR:
    /* skip a char & hilites in template string */
    do { ch = *++x; } while(ishighlight(ch));
    tracef1("[stringcmp STAR (x hilite skipped): x -> '%s']\n", x);

    wild_x = (ch == '^');
    if(wild_x)
        ++x;

    oy = y;

    /* loop1: */
    for(;;)
        {
        while(ishighlight(*y))
            ++y;

        /* skip a char & hilites in second string */
        do { ch = *++oy; } while(ishighlight(ch));
        tracef1("[stringcmp loop1 (oy hilite skipped): oy -> '%s']\n", oy);

        ox = x;

        /* loop3: */
        for(;;)
            {
            if(wild_x)
                switch(*x)
                    {
                    case '#':
                        tracef0("[stringcmp loop3: ^# found in first string: goto STAR to skip it & hilites]\n");
                        goto STAR;

                    case '^':
                        tracef0("[stringcmp loop3: ^^ found in first string: match as ^]\n");
                        wild_x = FALSE;

                    default:
                        break;
                    }

            tracef3("[stringcmp loop3: x -> '%s', y -> '%s', wild_x %s]\n", x, y, trace_boolstring(wild_x));

            /* are we at end of y string? */
            if(*y == '\0')
                {
                tracef1("[stringcmp: end of y string: returns %d\n", (*x == '\0') ? 0 : 1);
                if(*x == '\0')
                    return(0);      /* equal */
                else
                    return(1);      /* first bigger */
                }

            /* see if characters at x and y match */
            pos_res = toupper(*x) - toupper(*y);
            if(pos_res)
                {
                /* single character wildcard at x? */
                if(!wild_x  ||  (*x != '?')  ||  (*y == SPACE))
                    {
                    y = oy;
                    x = ox;

                    if(x == ptr1)
                        {
                        tracef1("[stringcmp: returns %d]\n", pos_res);
                        return(pos_res);
                        }

                    tracef0("[stringcmp: chars differ: restore ptrs & break to loop1]\n");
                    break;
                    }
                }

            /* characters at x and y match, so increment x and y */
            tracef0("[stringcmp: chars at x & y match: ++x, ++y & hilite skip both & keep in loop3]\n");
            do { ch = *++x; } while(ishighlight(ch));

            wild_x = (ch == '^');
            if(wild_x)
                ++x;

            do { ch = *++y; } while(ishighlight(ch));
            }
        }
}



/****************************************
*                                       *
* return 1 for > 0, 0 for 0, -1 for < 0 *
*                                       *
****************************************/

static intl
double_sgn(double num)
{
    return((num > (double) 0)
                    ? 1
                    : (num < (double) 0)
                                ? -1
                                : 0);
}


/*
compare two symbs
return +ve if first bigger, 0 if equal, -ve if second bigger
if we are comparing a string with wildcards against one without
the one with wildcards must be symb1

three types of strings need comparing:
intstr: a slot containing a formula whose result is a substring of the formula
        symb contains the formula, the offset in the formula and the length
strval: a slot containing a formula whose result is a reference to a slot
        containing a strval or an intstr or a text slot.  find_string finds
        the slot containing the intstr or text slot, NULL indicates loop
*/

extern intl
symbcmp(SYMB_TYPE *symb1, SYMB_TYPE *symb2)
{
    SYMB_TYPE tsymb1, tsymb2;
    uchar *ptr1, *ptr2;
    intl res;
    uchar *ep1, *ep2, ch;
    BOOL loop_found;
    slotp pslot;

    /* unless they're both STRVALs or INTSTRs, if they're of different type
     * they're different
    */
    if(symb1->type != symb2->type)
        if( ((symb1->type != SL_STRVAL)  &&  (symb1->type != SL_INTSTR))    ||
            ((symb2->type != SL_STRVAL)  &&  (symb2->type != SL_INTSTR))    )
                return(((intl) symb2->type) - ((intl) symb1->type));

    ptr1 = ptr2 = NULL;

    /* if they are STRVALs set up ptr1 and ptr2 to point at the strings */

    if(symb1->type == SL_STRVAL)
        {
        loop_found = FALSE;
        pslot = find_string(&symb1->value.slot, &loop_found);

        if(!pslot)
            {
            if(loop_found)
                {
                tsymb1.type = SL_ERROR;
                tsymb1.value.symb = ERR_LOOP;
                symb1 = &tsymb1;
                }
            else
                ptr1 = UNULLSTR;
            }
        elif(pslot->type == SL_TEXT)
            ptr1 = pslot->content.text;
        else
            ptr1 = pslot->content.number.text + pslot->content.number.result.str_offset;
        }

    if(symb2->type == SL_STRVAL)
        {
        loop_found = FALSE;
        pslot = find_string(&symb2->value.slot, &loop_found);

        if(!pslot)
            {
            if(loop_found)
                {
                tsymb2.type = SL_ERROR;
                tsymb2.value.symb = ERR_LOOP;
                symb2 = &tsymb2;
                }
            else
                ptr2 = UNULLSTR;
            }
        elif(pslot->type == SL_TEXT)
            ptr2 = pslot->content.text;
        else
            ptr2 = pslot->content.number.text + pslot->content.number.result.str_offset;
        }


    /* so they're of the same type, INTSTR & STRVAL forgiving */

    switch(symb1->type)
        {
        /* NB. double_sgn must be used here to generate the sign.
         * Casting to int is not enough cos fractions get cast to 0
        */
        case SL_DATE:
            return(double_sgn(difftime(symb1->value.date,symb2->value.date)));

        case SL_NUMBER:
            return(double_sgn(symb1->value.num - symb2->value.num));

        case SL_INTSTR:
        case SL_STRVAL:
            {
            /* ignore leading/trailing spaces:
             * temporarily patch end of string with NUL
            */
            if(!ptr1)       /* it's a INTSTR */
                ptr1 = symb1->value.str.ptr;

            do  {
                ch = *ptr1++;
                }
            while(ch == SPACE);
            ep1 = --ptr1;

            do  {
                ch = *ep1++;
                }
            while(ch);
            --ep1;

            do  {
                ch = *--ep1;
                }
            while(ch == SPACE);

            if(*++ep1 == SPACE)
                *ep1 = '\0';
            else
                ep1 = NULL;


            if(!ptr2)
                ptr2 = symb2->value.str.ptr;

            do  {
                ch = *ptr2++;
                }
            while(ch == SPACE);
            ep2 = --ptr2;

            do  {
                ch = *ep2++;
                }
            while(ch);
            --ep2;

            do  {
                ch = *--ep2;
                }
            while(ch == SPACE);

            if(*++ep2 == SPACE)
                *ep2 = '\0';
            else
                ep2 = NULL;


trace_on();
            res = stringcmp(ptr1, ptr2);
trace_off();


            /* repair trailing spaces */

            if(ep1)
                *ep1 = SPACE;

            if(ep2)
                *ep2 = SPACE;

            return(res);
            }


        default:
            return(0);
        }
}


/**************************
*                         *
* delete the marked block *
*                         *
**************************/

extern intl
do_delete_block(BOOL do_save)
{
    intl res;

    tracef0("do_delete_block()\n");

    if(protected_slot_in_range(&blkstart, &blkend))
        return(-1);

    res = save_block_and_delete(TRUE, do_save);

    if(res)
        {
        /* panic dump */
        graph_send_block(0, 0, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE);

        /* delete markers */
        blkstart.col = NO_COL;      
        }

    return(res);
}


/***************
*              *
* delete block *
*              *
***************/

extern void
DeleteBlock_fn(void)
{
    /* remember top-left of block for repositioning */
    BOOL caret_in_block = inblock(curcol, currow);
    colt col = blkstart.col;
    rowt row = blkstart.row;

    /* fault marked blocks not in this document */
    if((blkdochandle != current_document_handle())  &&  (col != NO_COL))
        {
        reperr_null(ERR_NOBLOCKINDOC);
        return;
        }

    do_delete_block(TRUE);      /* does mergebuf() eventually */

    if(caret_in_block)
        {
        /* move to top left of block */
        chknlr(col, row);
        lecpos = 0;
        }

    rstpag();

    recalc_bit = out_rebuildvert = out_screen = TRUE;
    filealtered(TRUE);
}


/*************
*            *
* delete row *
*            *
*************/

extern void
DeleteRow_fn(void)
{
    SLR bs, be, ba;
    dochandle bh;
    BOOL res = TRUE;

    xf_flush = TRUE;

    if(protected_slot_in_block(0, currow, numcol-1, currow))
        return;

    if(!mergebuf())
        {
        buffer_altered = slot_in_buffer = xf_inexpression = FALSE;
        return;
        }

    if(!dont_save)
        {       
        bs = blkstart;
        be = blkend;
        ba = blkanchor;
        bh = blkdochandle;

        blkstart.col = 0;
        blkstart.row = currow;
        blkend.col   = numcol - 1;
        blkend.row   = currow;
        blkdochandle = current_document_handle();

        res = save_block_and_delete(TRUE, TRUE);

        if(res)
            {
            graph_send_split_blocks(0, currow, LARGEST_COL_POSSIBLE, currow+1);

            if(blkdochandle == bh)
                {
                /* if deleting row above start of block then move block up */
                if(bs.row > currow)
                    {
                    if( ba.row == bs.row--)
                        ba.row = bs.row;
                    }

                /* if deleting row in middle of or last row of block then
                 * move end of block up, and anchor too possibly
                */
                if(be.row >= currow)
                    {
                    if( ba.row == be.row--)
                        ba.row = be.row;
                    }

                /* if last row of block has been deleted, remove block */
                if(be.row < bs.row)
                    bs.col = NO_COL;
                }
            }       

        blkstart     = bs;
        blkend       = be;
        blkanchor    = ba;
        blkdochandle = bh;
        }

    if(res)
        {
        out_rebuildvert = TRUE;
        filealtered(TRUE);
        mark_to_end(currowoffset);
        }
}


/*
ensure valid block
*/

static BOOL
set_up_block(void)
{
    if(!mergebuf())
        return(FALSE);

    if(blkstart.col == NO_COL)
        return(reperr_null(ERR_NOBLOCK));

    if(blkend.col == NO_COL)
        {
        tracef0("[set_up_block forcing single mark to block]\n");
        blkend = blkstart;
        }

    tracef4("[set_up_block ok: start %d %d; end %d %d]\n", blkstart.col, blkstart.row, blkend.col, blkend.row);

    return(TRUE);
}


/*************
*            *
* move block *
*            *
*************/

extern void
MoveBlock_fn(void)
{
    colt coldiff, oldcolumn, newcolumn, colsno;
    colt newcursorcol = NO_COL;
    rowt rowdiff, rowsno, newcursorrow;
    SLR fromstart, fromend, fromanchor;
    dochandle towindow, fromwindow;
    intl res;

    towindow = current_document_handle();
    fromwindow = blkdochandle;
    newcursorrow = currow;

    if(!set_up_block())
        return;

    /* make document containing block current document */
    select_document_using_handle(fromwindow);

    if(protected_slot_in_range(&blkstart, &blkend))
        return;

    if(fromwindow == towindow)
        {
        rowdiff = currow - blkstart.row;
        coldiff = curcol - blkstart.col;
        }
    else
        {
        rowdiff = BADROWBIT;
        coldiff = BADCOLBIT;
        }

    rowsno = blkend.row - blkstart.row + 1;
    colsno = blkend.col - blkstart.col + 1;

    /* check for overlap states:
        any situation where the block moves right
        but doesn't clear its own width

        if the cursor is in the block, don't bother doing anything
    */

    if(fromwindow == towindow)
        {
        if((curcol > blkstart.col)  &&  (curcol <= blkend.col))
            {
            reperr_null(ERR_OVERLAP);
            return;
            }

        /* inblock checks that blkdochandle is current window */
        if(inblock(curcol, currow))
            return;

        if((curcol == blkstart.col)  &&  (currow > blkend.row))
            newcursorrow = currow - (blkend.row - blkstart.row + 1);
        }

    #if MS || ARTHUR
    ack_esc();
    #endif

    /* now move the block, column by column */

    fromanchor = blkanchor;
    fromstart = blkstart;
    fromend = blkend;

    /* must start in towindow for variable initialisation */
    select_document_using_handle(towindow);

    escape_enable();

    for(newcolumn = curcol, oldcolumn = blkstart.col;
        !ctrlflag  &&  (newcolumn < curcol + colsno);
        newcolumn++, oldcolumn++)
        {
        rowt trowdiff; 

        /* must start in fromwindow */
        select_document_using_handle(fromwindow);

        trowdiff = rowdiff;

        blkstart.col = oldcolumn;
        blkstart.row = fromstart.row;
        blkend.col = oldcolumn;
        blkend.row = fromend.row;

        select_document_using_handle(towindow);

        if(newcursorcol == NO_COL)
            newcursorcol = newcolumn;

        /* check if new block is above and aligned with old cos rowdiff needs
            to take account of old block moving down
        */
        if(newcolumn == oldcolumn && currow <= blkstart.row && 
                                                        fromwindow == towindow)
            trowdiff -= rowsno;

        /*
            link each column out, and in to the new place

            new window is curhan, old window is blkdochandle
        */

        /* tell tree to recalculate missing slots */
        tree_moveblock(oldcolumn, blkstart.row, oldcolumn, blkend.row);

        res = moveslots(towindow, newcolumn, currow, 
                        fromwindow, oldcolumn, blkstart.row, rowsno);

        if(res < 0)
            {
            reperr_null(res);
            select_document_using_handle(towindow);
            break;
            }

        /* update references for the slots getting shunted down
         * note that this can change blkstart.row and blkend.row
         */
        select_document_using_handle(towindow);
        updref(newcolumn, currow, newcolumn, LARGEST_ROW_POSSIBLE, 0, rowsno);

        graph_send_split_blocks(newcolumn, currow, newcolumn+1, LARGEST_ROW_POSSIBLE);

        /* update references for the column that disappears
         * this can update the block (in the case when block moves up) so
         * save the old block coords first, saved before this loop starts
        */
        select_document_using_handle(fromwindow);
        updref(oldcolumn, blkstart.row, oldcolumn, blkend.row, coldiff, trowdiff);

        /* update references for the slots moving up, under old block */
        select_document_using_handle(fromwindow);
        updref(oldcolumn, fromend.row+1, oldcolumn, LARGEST_ROW_POSSIBLE, 0, -rowsno);

        graph_send_split_blocks(oldcolumn, fromstart.row, oldcolumn+1, LARGEST_ROW_POSSIBLE);

        select_document_using_handle(towindow);
        }

    escape_disable();

    /* draw from window */

    if(towindow != fromwindow)
        {
        select_document_using_handle(fromwindow);

        /* the mergebuf was in the to window */
        rebnmr();
        slot_in_buffer = FALSE;
        recalc_bit = out_screen = out_rebuildvert = TRUE;
        filealtered(TRUE);
        draw_screen();
        select_document_using_handle(towindow);
        rebnmr();
        }   

    /* move the cursor to the start of the block */
    tracef2("[MBfunc newcursorcol: %d, newcursorrow: %d]\n",
            newcursorcol, newcursorrow);

    chknlr(newcursorcol, newcursorrow);

    /* set block for new position */
    blkdochandle = towindow;
    blkstart.col = newcursorcol;    
    blkstart.row = newcursorrow;
    blkend.col = blkstart.col + (fromend.col - fromstart.col);
    blkend.row = blkstart.row + (fromend.row - fromstart.row);

    blkanchor.col = blkstart.col + fromanchor.col - fromstart.col;
    blkanchor.row = blkstart.row + fromanchor.row - fromstart.row;

    recalc_bit = out_screen = out_rebuildvert = TRUE;
    filealtered(TRUE);
}


/****************
*               *
*  copy block   *
*               *
****************/

extern void
CopyBlock_fn(void)
{
    colt coldiff, newcolumn, colsno;
    rowt rowdiff, rowsno;
    colt tcol;
    rowt roff;
    intl errorval = 1;
    dochandle fromwindow, towindow;
    word32 slot_count;

    if(!set_up_block())
        return;

    fromwindow  = blkdochandle;
    towindow    = current_document_handle();

    rowdiff = currow - blkstart.row;
    coldiff = curcol - blkstart.col;
    rowsno  = blkend.row - blkstart.row + 1;
    colsno  = blkend.col - blkstart.col + 1;

    slot_count = (word32) rowsno * (word32) colsno;

    /* check for overlap states:
     * any situation where the new block overlaps the existing block
     * or rows above it, within the same columns.  The exception
     * is where the blocks are aligned in the same columns perfectly
    */
    if( fromwindow == towindow &&
        currow <= blkend.row &&
        ((curcol > blkstart.col && curcol <= blkend.col) ||
        (curcol+colsno-1 >= blkstart.col && curcol+colsno-1 < blkend.col)))
        {
        reperr_null(ERR_OVERLAP);
        return;
        }

    /* create a gap in the column to copy the slots to
     * this might stretch the new blkstart, so remember to copy only
     * rowsno slots
    */
    for(newcolumn = curcol; newcolumn < curcol+colsno; newcolumn++)
        for(roff = 0; roff < rowsno; roff++)
            if(!insertslotat(newcolumn, currow))
                return;

    /* update references for the gap just created */
    updref(curcol, currow, curcol+colsno-1, LARGEST_ROW_POSSIBLE, 0, rowsno);

    #if RISCOS
    graph_send_split_blocks(curcol, currow, curcol+colsno, LARGEST_ROW_POSSIBLE);
    #endif

    /* check if new block is above and aligned with old cos rowdiff needs
     * to take account of old block moving down
    */
    if((fromwindow == towindow)  &&  (currow <= blkstart.row)  &&  (curcol == blkstart.col))
        rowdiff -= rowsno;

    out_screen = out_rebuildvert = out_rebuildhorz = global_recalc = TRUE;

    #if MS || ARTHUR
    ack_esc();
    #endif

    escape_enable();

    /* copy column by column */

    for(newcolumn = curcol, tcol = blkstart.col;
        !ctrlflag  &&  (tcol <= blkend.col);
        tcol++, newcolumn++)
        {
        for(roff = 0; !ctrlflag  &&  (roff < rowsno); roff++)
            {
            errorval = copyslot(fromwindow, tcol, blkstart.row+roff, 
                                towindow, newcolumn, currow+roff, 
                                coldiff, rowdiff, TRUE,
                                slot_count <= TREE_SORT_LIMIT);

            if(errorval < 0)
                goto BREAKOUT;
            }
        }

BREAKOUT:

    if(errorval < 0)
        reperr_null(errorval);

    escape_disable();

    recalc_bit = TRUE;
}


static word32 start_pos_on_stack = 0;   /* one before the first one on stack */
static word32 latest_word_on_stack = 0;

#define words_allowed ((word32) (d_deleted[0].option))


/* things on deleted_words list > BLOCK_OFFSET are blocks, else words */
#define BLOCK_OFFSET ((word32) 65536)


/* 
move a block of slots 
assumes: no overlap between old block and new block
*/

static intl
move_slots_with_no_overlap( colt tocol,     rowt torow, 
                            colt fromcol,   rowt fromrow,
                            colt csize,     rowt rsize)
{
    colt tcol;
    dochandle curhan = current_document_handle();
    intl res = 1;

    escape_enable();

    for(tcol = 0; !ctrlflag  &&  (tcol < csize); tcol++)
        {
        res = moveslots(/* to */    curhan, tocol   + tcol, torow, 
                        /* from */  curhan, fromcol + tcol, fromrow, 
                        /* no of rows */ rsize);
        if(res < 0)
            break;
        }

    if(escape_disable())
        res = ERR_ESCAPE;

    if(res < 0)
        return(res);

    rebnmr();

    /* update references for displaced block */
    updref(tocol, torow, tocol + csize - 1, LARGEST_ROW_POSSIBLE, (colt) 0, rsize);

    /*  update references for the block that moves  */
    updref(fromcol, fromrow, fromcol + csize - 1, fromrow + rsize - 1, tocol-fromcol, torow-fromrow);

    /* update references for the slots moving up, under old block */
    updref(fromcol, fromrow + rsize, fromcol + csize - 1, LARGEST_ROW_POSSIBLE, (colt) 0, -rsize);

    return(res);
}


/* 
copy a block of slots to a new column of to the right
if it fails it must tidy up the world as if nothing happened
*/

static intl
copy_slots_to_eoworld(colt fromcol, rowt fromrow, 
                      colt csize, rowt rsize)
{
    colt o_numcol = numcol;
    colt coldiff  = numcol - fromcol;
    colt tcol;
    rowt trow;
    intl errorval = 1;
    dochandle cdoc = current_document_handle();

    tracef0("[copy_slots_to_eoworld]\n");

    for(tcol = 0; !ctrlflag  &&  (tcol < csize); tcol++)
        {
        tracef2("[copy_slots_to_eoworld, tcol: %d, numcol: %d]\n",
                tcol, numcol);

        /* copy from fromcol + tcol to o_numcol + tcol */
        /* create a gap in the column to copy the slots to  */
        for(trow = 0; !ctrlflag  &&  (trow < rsize); trow++)
            {
            errorval = copyslot(cdoc, fromcol  + tcol, fromrow + trow, 
                                cdoc, o_numcol + tcol, trow, 
                                coldiff, -fromrow, FALSE, FALSE);

            if(errorval < 0)
                break;
            }

        if(errorval < 0)
            break;

        /* make sure it's compact */
        pack_column(o_numcol + tcol);
        }

    if((errorval > 0)  &&  ctrlflag)
        errorval = ERR_ESCAPE;

    if(errorval < 0)
        /* tidy up mess */
        delcolandentry(o_numcol, numcol - o_numcol);

    tracef0("[exit copy_slots_to_eoworld]\n");
    return(errorval);
}


static void
ensure_paste_list_clipped(void)
{
    /* remove all duffo entries from paste list */

    while(latest_word_on_stack - start_pos_on_stack > words_allowed)
        remove_deletion(++start_pos_on_stack);
}


/* save block of slots on to the deleted block stack
 * returns TRUE if successfully saved or user wants to proceed anyway
 * if cannot save block it leaves world as it found it and puts out dialog
 * box asking if user wants to proceed.  If so it returns TRUE, otherwise FALSE
*/

static BOOL
save_block_and_delete(BOOL is_deletion, BOOL do_save)
{
    SLR bs, be, curpos;
    BOOL res = TRUE;
    intl arraysize, copyres, save_active;
    LIST *lptr = NULL;
    colp delete_colstart;
    colt delete_size_col;
    rowt delete_size_row;
    word32 key;
    saved_block_descriptor *sbdp;   
    intl mres;

    tracef0("[save_block_and_delete]\n");

    if(!set_up_block())
        return(FALSE);

    bs = blkstart;
    be = blkend;        /* after forcing sensible block! */
    
    delete_size_col = be.col - bs.col + 1;
    delete_size_row = be.row - bs.row + 1;

    curpos.col = numcol;
    curpos.row = 0;

    escape_enable();

    save_active = do_save && (words_allowed > 0);

    if(save_active) 
        {
        /* get new colstart first of all */
        arraysize = sizeof(struct colentry) * delete_size_col;
        delete_colstart = alloc_ptr_using_cache(arraysize, &mres);

        if(delete_colstart)
            /* put block on deleted_words list */
            lptr = add_list_entry(&deleted_words, sizeof(saved_block_descriptor), &mres);

        if(lptr)
            {
            lptr->key = key = ++latest_word_on_stack + BLOCK_OFFSET;

            sbdp = (saved_block_descriptor *) lptr->value;
    
            tracef4("[save_block_and_delete saving block: key %d colstart &%p, cols %d, rows %d]\n",
                    lptr->key, delete_colstart, delete_size_col, delete_size_row);

            sbdp->del_colstart  = delete_colstart;
            sbdp->del_col_size  = delete_size_col;
            sbdp->del_row_size  = delete_size_row;
            }

        /* if those worked, copy slots to a parallel structure in this sheet */
        if(delete_colstart  &&  lptr)
            copyres = copy_slots_to_eoworld(bs.col, bs.row, 
                                            delete_size_col, delete_size_row);
        else
            copyres = (mres < 0) ? mres : ERR_NOROOM;

        if(copyres < 0)
            {
            /* copy slots failed - might be escape or memory problem */

            if(lptr)
                {
                latest_word_on_stack--;
                delete_from_list(&deleted_words, key);
                }

            dispose((void **) &delete_colstart);

            if((copyres != ERR_ESCAPE)  &&  is_deletion)
                {
                (void) init_dialog_box(D_SAVE_DELETED);

                res = dialog_box(D_SAVE_DELETED);

                if(res)
                    {
                    dialog_box_end();

                    res = (d_save_deleted[0].option == 'Y');

                    /* continue with deletion */
                    save_active = FALSE;
                    }
                }
            else
                res = reperr_null((copyres != ERR_NOROOM) ? copyres : ERR_CANTSAVEPASTEBLOCK);

            if(!res)
                goto FINISH_OFF;
            }

        /* may have said 'Yes' to continue deletion */
        if(save_active)
            {
            ensure_paste_list_clipped();

            tracef3("[save_block_and_delete, numcol: %d, curpos.col: %d, *cptr: &%p]\n",
                    numcol, curpos.col, delete_colstart);

            tracef0("[save_block_and_delete: block copied - now curpos.col..numcol-1]\n");
            }
        }


    /* if it's a deletion, delete it and update refs for a move */
    if(is_deletion)
        {
        colt tcol;
        rowt i;

        /* tell the tree about the slots getting deleted */
        graph_draw_tree_removeblock(bs.col, bs.row, be.col, be.row);

        #if 1
        if(save_active)
            tracef0("[updref first for slots moved to eow]\n");
            updref(bs.col, bs.row, be.col, be.row, curpos.col - bs.col, -bs.row);
        #endif

        tracef0("[updref slots below which have moved up]\n");
        updref(bs.col, be.row + 1, be.col, LARGEST_ROW_POSSIBLE, 0, -delete_size_row);

        tracef0("[emptying the deleted block]\n");
        for(tcol = bs.col; tcol <= be.col; tcol++)
            for(i = 0; !ctrlflag  &&  i < delete_size_row; i++)
                {
                tracef2("[killing slot col %d row %d]\n", tcol, bs.row);
                killslot(tcol, bs.row);
                }

        tracef0("[recalcing number of rows]\n");
        rebnmr();

        if(save_active) 
            /* patch the slot_refs so block thinks it's at A1 */
            block_updref(curpos.col, -curpos.col);
        }
    elif(save_active)
        /* how to make internal references update in copy to paste list case ?? */
        block_updref(curpos.col, -curpos.col);
    else
        bleep();

    if(save_active)
        {
        tracef0("[removing copied columns from sheet]\n");

        /* lose the columns from curpos.col to numcol */
        graph_draw_tree_removeblock(curpos.col, 0,
                                    numcol - 1, delete_size_row - 1);
    
        list_unlockpools();

        deregcoltab();

        /* reset number of columns after unlocking and deregistering all new ones */
        numcol = curpos.col;

        /* take a copy of the deregistered end segment to delete_colstart */
        memcpy(delete_colstart, colstart + numcol, arraysize);

        regcoltab();
        }
    
    /* sort out numrow cos columns disappearing */
    rebnmr();

    if(is_deletion)
        /* anything left in the file pointing to the disappeared block is bad */
        updref(curpos.col, 0, curpos.col + delete_size_col - 1, delete_size_row - 1, 0, BADROWBIT);

FINISH_OFF:

    res = res && !escape_disable();

    blkstart = bs;
    blkend   = be;

    tracef0("[exit save_block_and_delete]\n");

    return(res);
}


/************************
*                       *
* recover deleted block *
*                       *
************************/

static BOOL
recover_deleted_block(saved_block_descriptor *sbdp)
{
    colp delete_colstart = sbdp->del_colstart;
    colt delete_size_col = sbdp->del_col_size;
    rowt delete_size_row = sbdp->del_row_size;
    colt new_numcol;
    dochandle bh;
    SLR bs, be;
    BOOL res;

    bh = blkdochandle;
    bs = blkstart;
    be = blkend;

    blkdochandle = current_document_handle();

    new_numcol = max(numcol, curcol + delete_size_col);
    
    /* put block at column starting at numcol */
    if(!createcol(new_numcol + delete_size_col - 1))
        {
        tracef0("failed to create enough columns for block: leave on list\n");
        return(reperr_null(ERR_CANTLOADPASTEBLOCK));
        }

    /* copy column lists into end of colstart */
    deregcoltab();
    memcpy(colstart + new_numcol, delete_colstart, sizeof(struct colentry) * delete_size_col);
    regcoltab();

    rebnmr();

    /* may be slot refs in extra columns */
    refs_in_this_sheet = TRUE;

    /* poke its slot refs so that columns are sensible */
    block_updref(new_numcol, new_numcol);

    /* move it to cursor position */
    res = move_slots_with_no_overlap(curcol, currow, new_numcol, (rowt) 0,
                                     delete_size_col, delete_size_row);

    if(res < 0)
        {
        /* copy residual, maybe partially mashed, end of colstart back into column lists */
        deregcoltab();
        memcpy(delete_colstart, colstart + new_numcol, sizeof(struct colentry) * delete_size_col);
        regcoltab();

        /* delete added columns */
        delcolentry(new_numcol, numcol - new_numcol);

        res = reperr_null(ERR_CANTLOADPASTEBLOCK);
        }
    else
        {
        /* delete any leftover contents and structure */
        delcolandentry(new_numcol, numcol - new_numcol);

        free(delete_colstart);

        res = TRUE;
        }

    rebnmr();

    blkdochandle = bh;
    blkstart     = bs;
    blkend       = be;

    out_screen = out_rebuildvert = out_rebuildhorz = global_recalc = TRUE;

    return(res);
}


/************************************************************
*                                                           *
* copy block to buffer                                      *
*                                                           *
* if block not in current buffer switch windows temporarily *
*                                                           *
************************************************************/

extern void
CopyBlockToPasteList_fn(void)
{
    dochandle cdoc = current_document_handle();

    if(blkstart.col == NO_COL)
        {
        reperr_null(ERR_NOBLOCK);
        return;
        }

    select_document_using_handle(blkdochandle);
    save_block_and_delete(FALSE, TRUE);
    select_document_using_handle(cdoc);
}


/* 
delete a numbered entry from the deletions list

if keyed entry exists it is string, otherwise key+BLOCK_OFFSET is ptr to block 
*/

static void
remove_deletion(word32 key)
{
    LIST *lptr;
    saved_block_descriptor *sbdp;
    colp cptr;
    colt csize;

    tracef4("[remove_deletion key: %d, start_pos: %d, latest: %d, allowed: %d]\n",
            key, start_pos_on_stack, latest_word_on_stack, words_allowed);

    /* try removing as a string */
    if(delete_from_list(&deleted_words, key))
        {
        tracef0("[remove_deletion removed string]\n");
        return;
        }

    /* try removing as a block */
    lptr = search_list(&deleted_words, key + BLOCK_OFFSET);
    if(lptr)
        {
        tracef2("[remove_deletion lptr: %x, lptr->value: %x]\n",
                (intl) lptr, (intl) lptr->value);

        sbdp    = (saved_block_descriptor *) lptr->value;

        cptr    = sbdp->del_colstart;
        csize   = sbdp->del_col_size;
        
        tracef2("[remove_deletion cptr: %x, csize: %x]\n",
                (intl) cptr, (intl) csize);

        /* must register before deletion */
        regtempcoltab(cptr, csize);

        tracef0("[remove_deletion about to delcolstart]\n");
        delcolstart(cptr, csize);

        tracef0("[remove_deletion about to delete_from_list]\n");
        delete_from_list(&deleted_words, key + BLOCK_OFFSET);

        tracef0("[remove_deletion done delete_from_list]\n");
        }

    tracef0("[exit remove_deletion]\n");
}


/****************************
*                           *
* set the paste list depth  *
*                           *
****************************/

extern void
PasteListDepth_fn(void)
{
    while(dialog_box(D_DELETED))
        {
        #if 1
        ensure_paste_list_clipped();
        #else
        word32 i;

        for(i = latest_word_on_stack - words_allowed;
            i > start_pos_on_stack;
            i--)
                remove_deletion(i);

        if(i == start_pos_on_stack)
            start_pos_on_stack = latest_word_on_stack - words_allowed;
        #endif

        if(dialog_box_ended())
            break;
        }

/* printf("latest=%ld,oldest=%ld,allowed=%d",latest_word_on_stack,start_pos_on_stack,(int) words_allowed);rdch(1,1); */
}


/***********************
*                      *
* save string to stack *
*                      *
***********************/

extern BOOL
save_words(uchar *ptr)
{
    intl res;

    if(been_error)
        return(FALSE);

    if(words_allowed == (word32) 0)
        return(TRUE);

    /* save deleted bit */

    if((res = add_to_list(&deleted_words, ++latest_word_on_stack, ptr, &res)) <= 0)
        reperr_null(res ? res : ERR_NOROOM);

    if(been_error)
        {
        --latest_word_on_stack;
        return(FALSE);
        }

    ensure_paste_list_clipped();

    return(TRUE);
}


/****************************
*                           *
* paste word back into text *
*                           *
****************************/

extern void
Paste_fn(void)
{
    LIST *lptr;
    SLR oldpos;
    intl tlecpos;

    xf_flush = TRUE;

    if(latest_word_on_stack <= start_pos_on_stack)
        {
        latest_word_on_stack = start_pos_on_stack = 0;
        bleep();
        return;
        }

    /* find the top entry on the stack */
    lptr = search_list(&deleted_words, latest_word_on_stack);

    if(!lptr)
        {
        /* no word on list for this number - is there a block? */
        lptr = search_list(&deleted_words, latest_word_on_stack + BLOCK_OFFSET);

        if(!lptr)
            {
            /* no block either so go home */
            --latest_word_on_stack;
            return;
            }
        elif(!xf_inexpression)
            {
            if(!mergebuf())
                return;

            if(recover_deleted_block((saved_block_descriptor *) lptr->value))
                /* delete entry from stack */
                delete_from_list(&deleted_words, BLOCK_OFFSET + latest_word_on_stack--);

            return;
            }
        else
            {
            /* won't recover block when editing expression */
            reperr_null(ERR_EDITINGEXP);
            return;
            }
        }

    /* insert word: save old position */
    oldpos.col = curcol;
    oldpos.row = currow;
    tlecpos = lecpos;

    if(!insert_string(lptr->value, TRUE))
        return;

    chkwrp();

    /* now move cursor back */
    if(!in_dialog_box  &&  !mergebuf_nocheck())
        return;

    chknlr(oldpos.col, oldpos.row);
    lecpos = tlecpos;

    /* delete entry from stack */
    delete_from_list(&deleted_words, latest_word_on_stack--);
}


/************************
*                       *
* delete to end of slot *
*                       *
************************/

extern void
DeleteToEndOfSlot_fn(void)
{
    slotp tslot;

    if(protected_slot(curcol, currow))
        return;

    mark_row(currowoffset);

    if(!slot_in_buffer)
        {
        /* this is to delete non-text slots */
        tslot = travel_here();

        if(tslot  &&  (tslot->type != SL_PAGE))
            {
            /* save formula to word list */
            EditExpression_fn();
            save_words(linbuf);     
            recalc_bit = TRUE;
            merexp();
            endeex();
            }

        buffer_altered = output_buffer = TRUE;
        lecpos = lescrl = 0;
        *linbuf = '\0';
        slot_in_buffer = TRUE;
        (void) mergebuf_nocheck();
        recalc_bit = TRUE;
        return;
        }

    if(strlen((char *) linbuf) > lecpos)
        save_words(linbuf + lecpos);

    linbuf[lecpos] = '\0';

    if(dspfld_from == -1)
        dspfld_from = lecpos;

    buffer_altered = output_buffer = TRUE;

    if(!in_dialog_box)
        mergebuf();
}


/*****************************************************************************
*
*                               protection
*
*****************************************************************************/

/********************************
*                               *
*  see if a slot is protected   *
*                               *
********************************/

extern BOOL
test_protected_slot(colt tcol, rowt trow)
{
    slotp tslot = travel(tcol, trow);

    return(tslot  &&  is_protected_slot(tslot));
}


extern BOOL
protected_slot(colt tcol, rowt trow)
{
    if(test_protected_slot(tcol, trow))
        return(!reperr_null(ERR_PROTECTED));

    return(FALSE);
}


/************************************************************
*                                                           *
*  verify no protect slots in block: if there are - winge   *
*                                                           *
*  note that this corrupts in_block.col & in_block.row      *
*                                                           *
************************************************************/

extern BOOL
protected_slot_in_range(const SLR *bs, const SLR *be)
{
    slotp tslot;

    init_block(bs, be);

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        if(is_protected_slot(tslot))
            return(!reperr_null(ERR_PROTECTED));

    return(FALSE);
}


extern BOOL
protected_slot_in_block(colt firstcol, rowt firstrow, colt lastcol, rowt lastrow)
{
    SLR bs;
    SLR be;

    bs.col = firstcol;
    bs.row = firstrow;
    be.col = lastcol;
    be.row = lastrow;

    return(protected_slot_in_range(&bs, &be));
}


extern void
ClearProtectedBlock_fn(void)
{
    alnsto(J_BITS, (uchar) 0xFF, (uchar) 0);
}


static void
set_protected_block(const SLR *bs, const SLR *be)
{
    alnsto_block(PROTECTED, (uchar) 0xFF, (uchar) 0, bs, be);
}


extern void
SetProtectedBlock_fn(void)
{
    set_protected_block(&blkstart, &blkend);
}


extern void
setprotectedstatus(slotp tslot)
{
    if(tslot  &&  (tslot->justify & PROTECTED))
        {
        currently_protected = TRUE;

        if(currently_inverted)
            setcolour(PROTECTC, FORE);
        else
            setcolour(FORE, PROTECTC);
        }
    else
        currently_protected = FALSE;
}


/*
check that the slots between firstcol and lastcol in the row are protected
 - but not the previous slot or the subsequent one
*/

static BOOL
check_prot_range(rowt trow, colt firstcol, colt lastcol)
{
    if((firstcol > 0)  &&  test_protected_slot(firstcol-1, trow))
        return(FALSE);

    while(firstcol <= lastcol)
        if(!test_protected_slot(firstcol++, trow))
            return(FALSE);

    /* check slot following lastcol */
    return(!test_protected_slot(firstcol, trow));
}


/*
save all blocks of protected slots to the file

algorithm for getting block is to find top-left
    then horizontal extent is all slots marked to the right
    vertical extent is rows beneath with all cols as first row marked
        but not the slot to the left or the one to the right.

this ensures that a marked slot has already been saved if the slot
to the left is marked.
OK Ya.
*/

extern void
save_protected_bits(FILE *output)
{
    #if !defined(SAVE_OFF)
    slotp tslot;
    SLR last;
    uchar array[100];
    uchar *ptr;

    last.col = (colt) 0;
    last.row = (rowt) 0;

    init_doc_as_block();

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        /* if have already saved further down the block */
        if((in_block.col == last.col)  &&  (last.row > in_block.row))
            continue;

        if(is_protected_slot(tslot))
            {
            /* if slot to left is protected, this slot already saved */
            /* honest */
            if((in_block.col > 0)  &&  test_protected_slot(in_block.col-1, in_block.row))
                continue;

            last.col = in_block.col + 1;
            while(test_protected_slot(last.col, in_block.row))
                last.col++;
            last.col--;

            /* look to see how many rows down are similar - save them too */
            last.row = in_block.row + 1;
            while(check_prot_range(last.row, in_block.col, last.col))
                last.row++;
            last.row--;

            /* now lastcol is one past the last column */
            ptr = array;
            ptr += writeref(ptr, 0, in_block.col, in_block.row);
            ptr += writeref(ptr, 0, last.col, last.row);
            *ptr = '\0';

            (void) str_set(&d_protect[0].textfield, array);
            save_opt_to_file(output, d_protect, 1);

            /* last doubles as remembering the last top-left */
            last.col = in_block.col;
            last.row++;
            }
        }
    #endif  /* SAVE_OFF */
}


/* incoming option in d_protect[0] from file being loaded
 * add to list of protected blocks
*/

extern void
add_to_protect_list(uchar *ptr)
{
    intl res;

    static word32 block_count = 0;

    res = add_to_list(&protected_blocks, ++block_count, ptr, &res);

    if(res <= 0)
        {
        reperr_null(res ? res : ERR_NOROOM);
        if(res)
            been_error = FALSE;     /* shouldn't affect validity of loaded file if wally error */
        }
}


/*
clear the protect list, setting the slot protection
*/

extern void
clear_protect_list(void)
{
    SLR bs, be;
    LIST *lptr;

    for(lptr = first_in_list(&protected_blocks);
        lptr;
        lptr = next_in_list(&protected_blocks))
        {
        /* read two slot references out of lptr->value */

        buff_sofar = lptr->value;
        
        bs.col = getcol();
        bs.row = getsbd()-1;
        be.col = getcol();
        be.row = getsbd()-1;

        /* and mark the block */
        set_protected_block(&bs, &be);
        }

    delete_list(&protected_blocks);
}

/* end of execs.c */
