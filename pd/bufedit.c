/* bufedit.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       bufedit.c - module that edits the line buffer & simple movements
 * Author:      RJM August 1987
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"
#include "ext.spell"
#include "font.h"
#elif MS
#include "spell.ext"
#else
    assert(0);
#endif


#include "datafmt.h"

#if RISCOS
#include "ext.riscos"
#endif


/* exported functions */

extern BOOL all_widths_zero(colt tcol1, colt tcol2);
extern BOOL block_highlight(BOOL delete, intl type);
extern void chkwrp(void);
extern void chrina(uchar ch, BOOL allow_check);
extern void delete_bit_of_line(intl stt_offset, intl length, BOOL save);
extern BOOL insert_string(const char *str, BOOL allow_check);
extern BOOL fnxtwr(void);
extern void force_next_col(void);
extern void inschr(uchar ch);
extern BOOL str_isblank(const char *str);


/* internal functions */

static void del_chkwrp(void);       /* check for reformat situation on deleting characters */
/*static void reformat_with_steady_cursor(coord splitpoint, intl nojmp);*/


/**********************
*                     *
* formatting routines *
*                     *
**********************/

static BOOL chkcfm(rowt);
static coord fndlbr(rowt row);
static uchar *colword(void);
static BOOL mergeinline(coord);
static void init_colword(rowt);
/*static BOOL fill_linbuf(rowt);*/
static uchar *word_on_new_line(void);

static intl font_line_break(char *str, intl wrap_point);

/* ----------------------------------------------------------------------- */

/* variables used in formatting */
static rowt curr_inrow;             /* input row for colword() */
static uchar *lastword;
static intl word_in;
static intl word_out;
static intl last_word_in;
static intl old_just;
static intl new_just;
static intl old_flags;
static intl cur_leftright;

static uchar lastwordbuff[LIN_BUFSIZ];

static rowt curr_outrow;            /* output row for mergeinline() */
static intl out_offset;
static intl lindif;                 /* number of rows displaced in format */
static BOOL firstword;


/****************************************
*                                       *
* insert character into buffer          *
* If nothing in buffer, decide whether  *
* it's a text or numeric slot           *
*                                       *
****************************************/

extern void
inschr(uchar ch)
{
    slotp tslot;

    if(macro_recorder_on)
        output_char_to_macro_file(ch);

    if(xf_inexpression  &&  (ch < SPACE))
        return;

    do  {                               /* only once. dummy loop for clean exit */
        tslot = travel_here();

        /* no slot and not in numeric mode */
        if(!tslot)
            {
            if(txnbit)
                break;
            }
        elif(is_protected_slot(tslot))
            {
            reperr_null(ERR_PROTECTED);
            return;
            }

        /* text slot or blank text slot and text mode */
        if(tslot  &&  ((tslot->type == SL_TEXT)  ||  (tslot->type == SL_PAGE)))
            if(txnbit  ||  !isslotblank(tslot))
                break;

        /* new numeric slot */
        if(!xf_inexpression)
            {
            seteex();
            *linbuf = '\0';
            if(ch < SPACE)
                return;
            }
        }
    while(FALSE);

    chrina(ch, TRUE);
}


/********************************
*                               *
*  insert character in buffer   *
*                               *
********************************/

extern void
chrina(uchar ch, BOOL allow_check)
{
    intl length;
    uchar *currpos;

    if(!slot_in_buffer)
        return;

    length  = strlen((char *) linbuf);
    currpos = linbuf + lecpos;

    if(lecpos > length)
        {
        /* pad with spaces */
        memset(linbuf+length, SPACE, (unsigned) (lecpos-length));
        length = lecpos;
        linbuf[length] = '\0';
        }

    if(xf_iovbit)                  /* insert mode, create a space */
        {
        if(length >= MAXFLD)
            {
            (void) mergebuf_nocheck();
            bleep();
            clearkeyboardbuffer();
            return;
            }

        memmove(currpos+1, currpos, (unsigned) (length-lecpos+1));
        }
    elif(lecpos == length)
        {
        if(length >= MAXFLD)
            {
            bleep();
            been_error = xf_flush = TRUE;
            return;
            }

        linbuf[length+1] = '\0';
        }

    *currpos = ch;
    output_buffer = buffer_altered = TRUE;

    if( dspfld_from == -1)
        dspfld_from = lecpos;

    ++lecpos;

    #if !defined(SPELL_OFF)
    /* check spelling? */
    if( allow_check  &&  (lecpos >= 2)                           &&
        !in_dialog_box                                           &&
        !spell_iswordc(ch)  &&  spell_iswordc(linbuf[lecpos-2])  )
            check_word();
    #endif
}


extern BOOL
insert_string(const char *str, BOOL allow_check)
{
    uchar t_xf_iovbit;
    char ch;

    t_xf_iovbit = xf_iovbit;
    xf_iovbit = TRUE;               /* Force insert mode */

    while((ch = *str++) != '\0')
        {
        chrina(ch, allow_check);

        if(been_error)
            break;
        }

    xf_iovbit = t_xf_iovbit;

    return(!been_error);
}


/********************
*                   *
* beginning of slot *
*                   *
********************/

extern void
StartOfSlot_fn(void)
{
    if(slot_in_buffer)
        lecpos = lescrl = 0;
}


/****************
*               *
*  end of slot  *
*               *
****************/

extern void
EndOfSlot_fn(void)
{
    if(slot_in_buffer)
        lecpos = strlen((char *) linbuf);
}


/************
*           *
* cursor up *
*           *
************/

extern void
CursorUp_fn(void)
{
    if(!mergebuf())
        return;

    movement = CURSOR_UP;

    #if MS
    autorepeat = TRUE;
    #endif
}


/**************
*             *
* cursor down *
*             *
**************/

extern void
CursorDown_fn(void)
{
    if(!mergebuf())
        return;

    movement = CURSOR_DOWN;

    #if MS
    autorepeat = TRUE;
    #endif
}


/************
*           *
* screen up *
*           *
************/

extern void
PageUp_fn(void)
{
    if(!mergebuf())
        return;

    movement = CURSOR_SUP;

    #if MS
    autorepeat = TRUE;
    #endif
}


/**************
*             *
* screen down *
*             *
**************/

extern void
PageDown_fn(void)
{
    if(!mergebuf())
        return;

    movement = CURSOR_SDOWN;

    #if MS
    autorepeat = TRUE;
    #endif
}


/**************
*             *
* next column *
*             *
**************/

extern void
NextColumn_fn(void)
{
    if(!mergebuf())
        return;

    movement = CURSOR_NEXT_COL;
}


/******************
*                 *
* previous column *
*                 *
******************/

extern void
PrevColumn_fn(void)
{
    if(!curcoloffset  &&  !fstncx())
        {
        xf_flush = TRUE;
        return;
        }

    if(!mergebuf())
        return;

    movement = CURSOR_PREV_COL;
}



/****************
*               *
*  cursor left  *
*               *
****************/

extern void
CursorLeft_fn(void)
{
    uchar *ptr;

    if(!xf_inexpression  &&  !in_dialog_box  &&  (!slot_in_buffer  ||  !lecpos))
        {
        PrevColumn_fn();
        return;
        }

    if(lecpos > 0)
        {
        #if !defined(SPELL_OFF)
        /* check word perhaps */
        if(!in_dialog_box  &&  !xf_inexpression)
            {
            ptr = linbuf + lecpos;

            if(spell_iswordc(*ptr)  &&  !spell_iswordc(*(ptr-1)))
                check_word();
            }
        #endif

        --lecpos;
        }
}


/***************
*              *
* cursor right *
*              *
***************/

extern void
CursorRight_fn(void)
{
    uchar *ptr;

    if(!slot_in_buffer)
        {
        NextColumn_fn();
        return;
        }

    if(lecpos < MAXFLD)
        {
        #if !defined(SPELL_OFF)
        /* check word perhaps */
        if(!xf_inexpression  &&  !in_dialog_box  &&  (lecpos > 0))
            {
            ptr = linbuf + lecpos;

            if(!spell_iswordc(*ptr)  &&  spell_iswordc(*(ptr-1)))
                check_word();
            }
        #endif

        lecpos++;

#if FALSE
        /* this makes cursor jump to next column at end of current col */
        /* only for mouse cursor module */
        if(!xf_inexpression  &&  !in_dialog_box  &&  (lecpos > strlen((char *) linbuf))  &&  !lescrl)
            {
            /* are we at end of slot display? */
            if(lecpos > fwidth_of_slot_in_buffer())
                NextColumn_fn();
            }
#endif
        }
}


/******************************************************
*                                                     *
* count words in current file or marked block         *
* only counts words in text slots                     *
* slot references don't count, not even to text slots *
*                                                     *
******************************************************/

extern void
WordCount_fn(void)
{
    word32 count = 0;
    char array[LIN_BUFSIZ+1];
    slotp tslot;
    char ch;
    uchar *ptr;
    BOOL inword;

    if(!mergebuf_nocheck())
        return;

    /* if no block in this document look through whole file */

    if(MARKER_DEFINED())
        init_marked_block();
    else
        init_doc_as_block();

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        actind(ACT_BASHING, percent_in_block(DOWN_COLUMNS));

        if(tslot->type != SL_TEXT)
            continue;

        /* count words in this slot */

        ptr = tslot->content.text;
        inword = FALSE;

        while((ch = *ptr++) != '\0')
            {
            if(ch == SLRLDI)
                {
                ptr += SLRSIZE-1;
                continue;
                }

            if(ishighlight(ch))
                continue;

            if(ch != SPACE)
                {
                if(!inword)
                    {
                    inword = TRUE;
                    count++;
                    }
                }
            else
                inword = FALSE;
            }
        }

    actind_end();

    sprintf(array, Zld_Zs_found_STR,
            (long) count,
            (count == (word32) 1) ? word_STR : words_STR);

    #if RISCOS
    (void) str_set(&d_count[0].textfield, array);
    #elif MS || ARTHUR
    d_count[0].tag       = array;
    d_count[0].textfield = NULL;
    #endif

    if(dialog_box(D_COUNT))
        dialog_box_end();
}


/***************************
*                          *
* insert a space at cursor *
*                          *
***************************/

extern void
InsertSpace_fn(void)
{
    if(insert_string(" ", TRUE))
        CursorLeft_fn();
}


/********************************************************
*                                                       *
* insert character getting ASCII value from dialog box  *
*                                                       *
********************************************************/

extern void
InsertCharacter_fn(void)
{
    uchar ch;
    char array[2];

    while(dialog_box(D_INSCHAR))
        {
        ch = (uchar) d_inschar[0].option;

        tracef1("insert character got %d\n", ch);

        switch(ch)
            {
            case '\0':
/*          case CR:    */
/*          case LF:    */
            case SLRLDI:
            case FUNNYSPACE:
                bleep();
                break;

            default:
                *(array + 0) = ch;
                *(array + 1) = '\0';
                (void) insert_string(array, TRUE);
                break;
            }

        if(dialog_box_ended())
            break;
        }
}


/************************
*                       *
* delete char forwards  *
*                       *
************************/

/*
 * delete char forwards worrying about insert on return status
*/

extern void
DeleteCharacterRight_fn(void)
{
    xf_flush = TRUE;

    small_DeleteCharacter_fn();

    if( (lecpos >= strlen((char *) linbuf)) &&
        (d_options_IR == 'Y')               &&
        (currow < numrow - 1)               )
            JoinLines_fn();

    dspfld_from = lecpos;

    del_chkwrp(); 
}


extern void
small_DeleteCharacter_fn(void)
{
    if(!slot_in_buffer)
        return;

    output_buffer = TRUE;

    delete_bit_of_line(lecpos, 1, FALSE);
}


/********************************************************
*                                                       *
*  erase character to the left of the insertion point   *
*                                                       *
********************************************************/

extern void
DeleteCharacterLeft_fn(void)
{
    intl length;
    uchar *currpos;

    xf_flush = TRUE;

    /* if moving back over start of line */
    if(lecpos <= 0)
        {
        if((d_options_IR == 'Y')  &&  (currow > 0))
            {
            intl tlecpos;
            rowt trow;

            PrevWord_fn();
            currow--;
            filbuf();
            JoinLines_fn();
            tlecpos = lecpos;
            trow = currow;

            del_chkwrp();

            chknlr(curcol, currow = trow);
            lecpos = tlecpos;
            }
        return;
        }

    if(!slot_in_buffer)
        return;

    length = (intl) strlen((char *) linbuf);
    if(--lecpos >= length)
        return;

    currpos = linbuf + lecpos;

    /* in insert mode move text backwards,
    overtype mode replace with a space */
    if(xf_iovbit)
        memmove(currpos, currpos + 1, (unsigned) (length-lecpos));
    else
        *currpos = SPACE;

    buffer_altered = output_buffer = TRUE;
    del_chkwrp();
    dspfld_from = lecpos;
}


extern void
Underline_fn(void)
{
    inschr(FIRST_HIGHLIGHT);
}


extern void
Bold_fn(void)
{
    inschr(FIRST_HIGHLIGHT+1);
}


extern void
ExtendedSequence_fn(void)
{
    inschr(FIRST_HIGHLIGHT+2);
}


extern void
Italic_fn(void)
{
    inschr(FIRST_HIGHLIGHT+3);
}


extern void
Subscript_fn(void)
{
    inschr(FIRST_HIGHLIGHT+4);
}


extern void
Superscript_fn(void)
{
    inschr(FIRST_HIGHLIGHT+5);
}


extern void
AlternateFont_fn(void)
{
    inschr(FIRST_HIGHLIGHT+6);
}


extern void
UserDefinedHigh_fn(void)
{
    inschr(FIRST_HIGHLIGHT+7);
}


/***************************************
*                                      *
* insert or delete highlights in block *
*                                      *
***************************************/

typedef enum
{
    H_INSERT = FALSE,
    H_DELETE = TRUE
}
block_highlight_parm;


static void
highlight_words_on_line(BOOL delete, uchar h_ch)
{
    BOOL last_was_space = TRUE;
    uchar ch;

    lecpos = 0;

    while((ch = linbuf[lecpos]) != '\0')
        {
        if(ch == SLRLDI)
            {
            plusab(lecpos, SLRSIZE);
            continue;
            }

        if(delete)
            {
            if(ch == h_ch)
                delete_bit_of_line(lecpos, 1, FALSE);
            else
                ++lecpos;
            }
        else
            {
            if( ( last_was_space  &&  ((ch != SPACE) &&  ch))   ||
                (!last_was_space  &&  ((ch == SPACE) || !ch))   )
                {
                chrina(h_ch, FALSE);
                last_was_space = !last_was_space;
                }

            ++lecpos;
            }
        }
}


static void
block_highlight_core(BOOL delete)
{
    uchar h_ch = (d_inshigh[0].option - FIRST_HIGHLIGHT_TEXT) + FIRST_HIGHLIGHT;
    BOOL txf_iovbit = xf_iovbit;
    intl tlecpos = lecpos;
    slotp tslot;

    xf_iovbit = TRUE;       /* force insert mode */

    init_marked_block();

    while((tslot = next_slot_in_block(DOWN_COLUMNS)) != NULL)
        {
        actind(ACT_BASHING, percent_in_block(DOWN_COLUMNS));

        if((tslot->type != SL_TEXT)  ||  isslotblank(tslot))
            continue;

        prccon(linbuf, tslot);      /* decompile to linbuf */
        slot_in_buffer = TRUE;

        highlight_words_on_line(H_DELETE, h_ch);

        if(!delete)
            highlight_words_on_line(H_INSERT, h_ch);

        if(buffer_altered)
            {
            if(!merst1(in_block.col, in_block.row))
                break;

            out_screen = TRUE;
            filealtered(TRUE);
            }

        slot_in_buffer = FALSE;
        }

    actind_end();

    filbuf();
    lecpos = tlecpos;
    xf_iovbit = txf_iovbit;
}


static void
highlight_block(BOOL delete, intl type)
{
    if(xf_inexpression)
        {
        reperr_null(ERR_EDITINGEXP);
        return;
        }

    if(!mergebuf_nocheck())
        return;

    while(dialog_box(type))
        {
        block_highlight_core(delete);

        if(dialog_box_ended())
            break;
        }
}


/*******************************
*                              *
* insert highlights into block *
*                              *
*******************************/

extern void
HighlightBlock_fn(void)
{
    highlight_block(H_INSERT, D_INSHIGH);
}


/*******************************
*                              *
* remove highlights from block *
*                              *
*******************************/

extern void
RemoveHighlights_fn(void)
{
    highlight_block(H_DELETE, D_REMHIGH);
}


/*********************
*                    *
* toggle insert flag *
*                    *
*********************/

extern void
InsertOvertype_fn(void)
{
    xf_iovbit = !xf_iovbit;

    insert_state_changed();
}


/***********************************
*                                  *
* move to next word in line buffer *
* if no next word return false     *
*                                  *
***********************************/

extern BOOL
fnxtwr(void)
{
    intl length = strlen((char *) linbuf);

    if(lecpos >= length)
        return(FALSE);

    while((lecpos < length)  &&  (linbuf[lecpos] != SPACE))
        lecpos++;

    while((lecpos < length)  &&  (linbuf[lecpos] == SPACE))
        lecpos++;

    return(TRUE);
}


/**************
*             *
* delete word *
*             *
**************/

extern void
DeleteWord_fn(void)
{
    intl templecpos = lecpos;

    if(!fnxtwr()  &&  !xf_inexpression  &&  !in_dialog_box)
        {
        /* delete line join */
        DeleteCharacterRight_fn();
        return;
        }

    delete_bit_of_line(templecpos, lecpos - templecpos, TRUE);

    lecpos = templecpos;

    output_buffer = TRUE;

    del_chkwrp();
}


extern void
delete_bit_of_line(intl stt_offset, intl length, BOOL save)
{
    char array[LIN_BUFSIZ];
    char *dst, *src;
    intl len = strlen(linbuf);

    if(!slot_in_buffer  ||  (stt_offset >= len))
        return;

    length = min(length, len);

    dst = linbuf + stt_offset;
    src = dst + length;

    if(src <= dst)
        return;

    if(save)
        {
        /* save word to paste list */
        memcpy(array, dst, length);
        array[length] = '\0';
        save_words(array);
        }

    buffer_altered = TRUE;

    /* compact up (+1 to copy NUL) */
    memmove(dst, src, strlen(src) + 1);
}


/************
*           *
* next word *
*           *
************/

extern void
NextWord_fn(void)
{
    if(slot_in_buffer  &&  fnxtwr())
        return;

    if(xf_inexpression)
        return;

    if(currow+1 < numrow)
        {
        lecpos = lescrl = 0;

        CursorDown_fn();

        #if MS
        autorepeat = FALSE;
        #endif

        return;
        }
}


/****************
*               *
* previous word *
*               *
****************/

extern void
PrevWord_fn(void)
{
    slotp tslot;

    if(slot_in_buffer  &&  (lecpos > 0))
        {
        /* move lecpos back to start of previous word in line buffer */

        while((lecpos > 0)  &&  (linbuf[lecpos-1] == SPACE))
            --lecpos;

        while((lecpos > 0)  &&  (linbuf[lecpos-1] != SPACE))
            --lecpos;

        return;
        }

    if(xf_inexpression  ||  !currow  ||  !mergebuf())
        return;

    /* move to end of previous row */

    tslot = travel(curcol, currow - 1);

    if(tslot  &&  (tslot->type == SL_TEXT))
        lecpos = strlen((char *) tslot->content.text);
    else
        lecpos = lescrl = 0;

    CursorUp_fn();

    #if MS
    autorepeat = FALSE;
    #endif
}


/************
*           *
* swap case *
*           *
************/

extern void
SwapCase_fn(void)
{
    int ch;

    if(!slot_in_buffer)
        return;

    /* if cursor off end of text move to next line */
    if(lecpos > strlen(linbuf))
        {
        NextWord_fn();
        return;
        }

    ch = (int) linbuf[lecpos];

    linbuf[lecpos] = (uchar) (isupper(ch)
                                ? tolower(ch)
                                : toupper(ch));

    buffer_altered = output_buffer = TRUE;

    dspfld_from = lecpos;

    if(lecpos < MAXFLD)
        lecpos++;

    #if MS
    autorepeat = FALSE;
    #endif
}


/*******************************************
*                                          *
* check if all widths are zero for columns *
* other than between tcol1 and tcol2       *
*                                          *
*******************************************/

extern BOOL
all_widths_zero(colt tcol1, colt tcol2)
{
    colt trycol;

    tracef3("all_widths_zero(%d, %d): numcol = %d\n", tcol1, tcol2, numcol);

    for(trycol = 0; trycol < numcol; trycol++)
        if(((trycol < tcol1)  ||  (trycol > tcol2))  &&  colwidth(trycol))
            return(FALSE);

    return(TRUE);
}


static BOOL
do_widths_admin_start(intl width)
{
    uchar array[20];

    d_width[0].option = (optiontype) width;
    writecol(array, curcol);

    return(str_set(&d_width[1].textfield, array));
}


static BOOL
do_widths_admin_core(colt *tcol1, colt *tcol2)
{
    buff_sofar = (uchar *) d_width[1].textfield;

    /* assumes buff_sofar set */
    if((*tcol1 = getcol()) == NO_COL)
        *tcol1 = curcol;

    /* get second column in range */
    if((*tcol2 = getcol()) == NO_COL)
        *tcol2 = *tcol1;

    return((*tcol2 < numcol)  &&  (*tcol1 <= *tcol2));
}


extern void
ColumnWidth_fn(void)
{
    colt tcol, tcol1, tcol2;
    intl *widp, *wwidp;
    BOOL badparm;

    if(!do_widths_admin_start(colwidth(curcol)))
        return;

    #if MS || ARTHUR
    d_width[0].tag = New_width_STR;
    #endif

    while(dialog_box(D_WIDTH))
        {
        badparm = !do_widths_admin_core(&tcol1, &tcol2);

        /* width of zero special */
        if(!badparm  &&  (d_width[0].option == 0))
            {
            /* if setting current column to zero width force slot out of buffer */
            if((curcol >= tcol1)  &&  (curcol <= tcol2))
                if(!mergebuf())
                    {
                    dialog_box_end();
                    break;
                    }

            /* must have some other columns possibly visible */
trace_on();
            badparm = all_widths_zero(tcol1, tcol2);
tracef1("all_widths_zero %d\n", badparm);
trace_off();

            /* can't set fixed cols to zero width */
            if(!badparm)
                {
                tcol = tcol1;
                do  {
                    if(incolfixes(tcol))
                        badparm = TRUE;
                    }
                while(++tcol <= tcol2);
                }
            }

        if(badparm)
            {
            reperr_null(ERR_BAD_PARM);
            continue;
            }

        tcol = tcol1;
        do  {
            readpcolvars(tcol, &widp, &wwidp);
            *widp = d_width[0].option;
            }
        while(++tcol <= tcol2);

        /* if the current width now 0 move ahead to first non-zero column */

        for( ; !colwidth(curcol)  &&  (curcol < numcol-1); ++curcol)
            movement = ABSOLUTE;

        /* if there aren't any ahead, move back to one */

        for( ; !colwidth(curcol); --curcol)
            movement = ABSOLUTE;

        newcol = curcol;
        newrow = currow;

        xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
        filealtered(TRUE);

        if(dialog_box_ended())
            break;
        }
}


extern void
RightMargin_fn(void)
{
    colt tcol, tcol1, tcol2;
    intl *widp, *wwidp;

    if(!do_widths_admin_start(colwrapwidth(curcol)))
        return;

    #if MS || ARTHUR
    d_width[0].tag = New_right_margin_position_STR;
    #endif

    while(dialog_box(D_MARGIN))
        {
        if(!do_widths_admin_core(&tcol1, &tcol2))
            {
            reperr_null(ERR_BAD_PARM);
            continue;
            }

        tcol = tcol1;
        do  {
            readpcolvars(tcol, &widp, &wwidp);
            *wwidp = d_width[0].option;
            }
        while(++tcol <= tcol2);

        xf_drawcolumnheadings = out_screen = out_rebuildhorz = TRUE;
        filealtered(TRUE);

        if(dialog_box_ended())
            break;
        }
}


/****************************************************************************
*                                                                           *
*  WL and WR decrement (left) and increment (right) the current wrap width  *
*                                                                           *
****************************************************************************/

extern void
MoveMarginLeft_fn(void)
{
    coord wid = colwrapwidth(curcol);

    if(wid > 2)     /* this to follow 6502 which can't get to 1 */
        dstwrp(curcol, wid-1);

    xf_flush = TRUE;
}


extern void
MoveMarginRight_fn(void)
{
    dstwrp(curcol,colwrapwidth(curcol) + 1);

    xf_flush = TRUE;
}


/***************************************************************************/


/********************************************************
*                                                       *
* fill linbuf with words from colword                   *
* returns TRUE if it has done some formatting           *
* startrow is the row to at which insertions should be  *
* made in other columns for iow                         *
*                                                       *
********************************************************/

static BOOL
fill_linbuf(rowt startrow)
{
    firstword = TRUE;

    /* colword() sets up lastword to point to next word */
    while(colword() != NULL)
        {
        uchar *to;
        slotp tslot;
        coord breakpoint;

        firstword = FALSE;

        /* don't join words together from joined lines */
        to = linbuf + lecpos;
        if(lecpos > 0  &&  *(to - 1) != SPACE)
            {
            lecpos++;
            *to++ = SPACE;
            }

        /* copy in word.  any slot references need decompiling */
        for( ; *lastword != SPACE  &&  *lastword != '\0'; lecpos++)
            if(*lastword != SLRLDI)
                *to++ = *lastword++;
            else
                {
                /* decompile slot ref */
                uchar *oldto;
                colt c;
                rowt r;
                docno d;

                oldto = to;
                ++lastword;

                d = (docno) talps(lastword, sizeof(docno));
                lastword += sizeof(docno);

                c = (colt) talps(lastword, sizeof(colt));
                r = (rowt) talps(lastword += sizeof(colt),
                                 sizeof(rowt)) & ROWNOBITS;

                to += writeref(to, d, c, r);
                lastword += sizeof(rowt);
                plusab(lecpos, (to - oldto) - 1);
                }

        /* and spaces after word */
        for( ; *lastword == SPACE ; lecpos++)
            *to++ = *lastword++;

        *to = '\0';
        ++word_out;

        if((breakpoint = fndlbr(curr_outrow)) > 0)
            {
            new_just = jusbit ? (cur_leftright ? J_LEFTRIGHT : J_RIGHTLEFT)
                              : J_FREE;

            /* this line too long so merge it in */
            if(!mergeinline(breakpoint))
                return(TRUE);

            tslot = travel(curcol, curr_outrow - 1);

            /* set to justify perhaps */
            if(jusbit  &&  ((tslot->justify & J_BITS) == J_FREE))
                {
                /* set new justify status, keeping protected bit */
                tslot->justify = (tslot->justify & CLR_J_BITS) | new_just;
                cur_leftright = !cur_leftright;
                }
            }
        }

    linbuf[lecpos + 1] = '\0';
    new_just = J_FREE;

    if(firstword)
        /* no words to format */
        return(FALSE);
    else
        /* don't set to justify cos last line in paragraph */
        mergeinline(lecpos);

    /* must output below here if a registration difference */
    if(lindif)
        {
        mark_to_end(currowoffset);
        out_rebuildvert = TRUE;
        }
    else
        rebnmr();

    tracef0("[fill_linbuf about to do adjustment after reformat]\n");

    /* if insert on wrap, need to insert lindif rows in other columns */
    if(lindif > 0)
        {
        if(iowbit)
            {
            colt tcol;

            for(tcol = 0; tcol < numcol; tcol++)
                {
                intl i;

                if(tcol != curcol)
                    for(i = 0; i < lindif; i++)
                        insertslotat(tcol, startrow);
                }

            /* update numrow */
            rebnmr();

            graph_send_block(0, startrow, numcol, numrow);

            /* update for insert in all columns */
            updref(0, startrow, LARGEST_COL_POSSIBLE, LARGEST_ROW_POSSIBLE, 0, (rowt) lindif);
            }
        else
            {
            /* update numrow */
            rebnmr();

            /* update for insert in this column only */
            updref(curcol, startrow, curcol, LARGEST_ROW_POSSIBLE, 0, (rowt) lindif);
            }
        }
    /* paragraph is now shorter so fill in some lines */
    else
        {
        /* only pad with blank rows if insert on wrap is rows */
        if(iowbit)
            {
            for( ; lindif < 0; lindif++)
                {
                BOOL rowblank = TRUE;
                colt tcol;

                /* try not to pad if rest of line is blank */
                for(tcol = 0; tcol < numcol; tcol++)
                    if(tcol != curcol && !isslotblank(travel(tcol,
                                                             curr_outrow)))
                        {
                        rowblank = FALSE;
                        break;
                        }

                if(rowblank)
                    {
                    for(tcol = 0; tcol < numcol; tcol++)
                        if(tcol != curcol)
                            killslot(tcol, curr_outrow);
                    delrwb(U_ALL);
                    mark_row(currowoffset - 1);
                    }
                else
                    insertslotat(curcol, curr_outrow);
                }

            rebnmr();
            }
        }

    tracef1("[fill_linbuf numrow: %d]\n", numrow);
    return(TRUE);
}


/************************************************************************
*                                                                       *
* merge in line from linbuf taking care of curr_inrow                   *
* line contains words too many so only write out up to the split point  *
* and then copy extra words to start of linbuf                          *
*                                                                       *
************************************************************************/

static BOOL
mergeinline(coord splitpoint)
{
    uchar *from, *to;
    uchar breakpoint = linbuf[splitpoint];
    slotp sl;
    rowt temp_row;

    linbuf[splitpoint] = '\0';
    insertslotat(curcol, curr_outrow);
    curr_inrow++;
    lindif++;

    buffer_altered = slot_in_buffer = TRUE;
    temp_row = currow;
    currow = curr_outrow;
    if(!mergebuf_nocheck())
        return(FALSE);
    currow = temp_row;

    /* move to new row */
#if 0
    currow++;       
#endif
    curr_outrow++;
    linbuf[splitpoint] = breakpoint;

    tracef5("[currow: %d, inrow: %d, outrow: %d, old_just: %d, new_just: %d]\n",
            currow, curr_inrow, curr_outrow,
            old_just, new_just);

    /* work out if line was actually altered */
    if( (old_flags & SL_ALTERED)    ||
        !(!lindif                               &&
         (curr_inrow == curr_outrow)            &&
         (word_in <= 1)                         &&
         (word_out - word_in == last_word_in)   &&
         (old_just == new_just)                 )
      )
        {
        sl = travel(curcol, curr_outrow - 1);
        if(sl)
            {
            orab(sl->flags, SL_ALTERED);
            xf_drawsome = TRUE;
            tracef0("[slot marked altered]\n");
            }
        }

    /* move word left at end of line back to start */
    from = linbuf + splitpoint;
    to = linbuf;

    /* set word out counter */
    if(*from)
        word_out = 1;
    else
        word_out = 0;

    while(*from)
        {
        if(*from == SLRLDI)
            {
            memmove(to, from, SLRSIZE);
            to += SLRSIZE;
            from += SLRSIZE;
            }
        else
            *to++ = *from++;
        }

    *to = '\0';
    lecpos = to - linbuf;
    return(TRUE);
}


/***********************************************************
*                                                          *
* collect word from slot or next slot                      *
* return address of word or NULL if break in formatting    *
* enters with lastword set to NULL the first time,         *
* points to space after last word read subsequently        *
*                                                          *
***********************************************************/

static uchar *
colword(void)
{
    slotp sl;
    uchar *next_word;

    if(!lastword)
        next_word = word_on_new_line();
    else
        {
        /* look for another word on same line */
        while(*lastword == SPACE)
            lastword++;

        if(*lastword)
            next_word = lastword;
        else
            {
            /* no more words on this line so delete the slot */
            if((sl = travel(curcol, curr_inrow)) != NULL)
                {
                old_just = (sl->justify & J_BITS);
                old_flags = sl->flags;
                }

            killslot(curcol, curr_inrow);
            lindif--;
            next_word = word_on_new_line();
            }
        }

    if(next_word)
        ++word_in;

    return(next_word);
}


static uchar *
word_on_new_line(void)
{
    slotp tslot = travel(curcol, curr_inrow);

    last_word_in = word_in;
    word_in = 0;

    if(!tslot  ||  !chkcfm(curr_inrow))
        return(NULL);

    memcpy(lastwordbuff, tslot->content.text, slotcontentssize(tslot));     /* includes terminator */
    lastword = lastwordbuff;
    
    return(((*lastword == SPACE  &&  !firstword)  ||  *lastword == '\0')
                ? NULL
                : lastword);
}


/*******************
*                  *
* format paragraph *
*                  *
*******************/

extern void
FormatParagraph_fn(void)
{
    if(!wrpbit)
        return;

    /* if can't format, just move cursor down */
    if(!chkcfm(currow))
        {
        CursorDown_fn();
        #if MS
        autorepeat = FALSE;
        #endif
        return;
        }

    if(!mergebuf_nocheck())
        return;

    init_colword(currow);
    fill_linbuf(currow + 1);
    chknlr(curcol, (curr_outrow >= numrow)
                            ? numrow - 1
                            : curr_outrow);
}


/**************************************************************************
*                                                                         *
* reformat from this line to end of paragraph, leaving cursor where it is *
*                                                                         *
**************************************************************************/

static void
reformat_with_steady_cursor(coord splitpoint, intl nojmp)
{
    intl tlecpos;
    rowt trow;

    if(!mergebuf_nocheck())
        return;

    /* save current row and position */
    trow = currow;
    tlecpos = lecpos;

    init_colword(currow);
    fill_linbuf(currow + 1);

    /* if cursor was past line break, move down to next line,
    subtracting size of split part to get new position */
    if(tlecpos >= splitpoint && !nojmp)
        {
        /* this does not cater for weird case
        where new word may jump two lines */
        lecpos = tlecpos - splitpoint;
        ++trow;
        }
    else
        /* otherwise, stay where we were */
        lecpos = tlecpos;

    chknlr(curcol, trow);
}


/*
just done a deletion so try a reformat
*/

static void
del_chkwrp(void)
{
    slotp sl;
    intl splitpoint;

    /* do a reformat if insert on return */
    if( !wrpbit  ||  (d_options_IR != 'Y')  ||
        in_dialog_box  ||  xf_inexpression  ||
        !chkcfm(currow))
            return;

    /* get length of line */
    if((splitpoint = fndlbr(currow)) == 0)
        return;

    /* if there is no split on the line, check to
     * see if we can add the first word on the next line
    */
    if(splitpoint < 0)
        {
        if( chkcfm(currow + 1) &&
            (sl = travel(curcol, currow + 1)) != NULL)
            {
            intl wrapwidth = chkolp(travel_here(), curcol, currow);
            char *c;
            #if RISCOS
            char tbuf[LIN_BUFSIZ * 4];

            if(riscos_fonts)
                {
                /* get string with font changes and @@@@s */
                font_expand_for_break(tbuf, sl->content.text);
                c = tbuf;
                }
            else
            #endif
                c = sl->content.text;

            while(*c && *c != SPACE)
                c += font_skip(c);

            #if RISCOS
            if(riscos_fonts)
                {
                intl word_wid;

                *c = '\0';
                word_wid = font_width(tbuf);

                wrapwidth *= charwidth * x_scale;
                if(wrapwidth - -splitpoint < word_wid)
                    return;
                }
            else
            #endif
                if(wrapwidth - -splitpoint < c - sl->content.text)
                    return;
            }
        else
            return;

        /* cancel any hope of a split */
        splitpoint = 0;
        }

    reformat_with_steady_cursor(splitpoint, TRUE);
}


/************************
*                       *
* check for and do wrap *
*                       *
************************/

extern void
chkwrp(void)
{
    coord splitpoint;

    if(!wrpbit  ||  !chkcfm(currow))
        return;

    if((splitpoint = fndlbr(currow)) > 0)
        reformat_with_steady_cursor(splitpoint, FALSE);
}


/************************
*                       *
* check can format slot *
*                       *
************************/

static BOOL
chkcfm(rowt trow)
{
    slotp tslot;

    if(xf_inexpression  ||  chkrpb(trow))
        return(FALSE);

    tslot = travel(curcol, trow);

    if(!tslot)
        return(!str_isblank(linbuf));

    if(tslot->type == SL_TEXT)
        {
        /* note: if protected bit set will return false */
        switch(tslot->justify)
            {
            case J_FREE:
            case J_LEFTRIGHT:
            case J_RIGHTLEFT:
                    return(TRUE);
            default:
                    break;
            }
        }

    return(FALSE);
}


/************************************************************
*                                                           *
* determine whether string consists of anything but spaces  *
*                                                           *
************************************************************/

extern BOOL
str_isblank(const char *str)
{
    char ch;

    if(!str)
        return(TRUE);

    while((ch = *str++) == SPACE)
        ;

    return((BOOL) (ch == '\0'));
}


/****************************************************
*                                                   *
* find line break point in linbuf                   *
* needs to take account of highlights and @ fields  *
*                                                   *
* MRJC modified 7.7.89 for fonts                    *
*                                                   *
****************************************************/

static coord
fndlbr(rowt row)
{
    uchar *tptr;
    intl width;
    coord startofword, wrapwidth;

    wrapwidth = chkolp(travel(curcol, row), curcol, row) - 1;

    #if RISCOS
    if(riscos_fonts)
        {
        char tbuf[LIN_BUFSIZ * 4], *break_point;
        intl split_count;

        /* get string with font changes and @@@@s */
        font_expand_for_break(tbuf, linbuf);
        split_count = font_line_break(tbuf, wrapwidth);

        if(split_count <= 0)
            return(split_count);

        /* find that gap at which we broke */
        for(break_point = linbuf;
            split_count && *break_point;
            split_count--)
            {
            while(*break_point && *break_point != SPACE)
                ++break_point;

            if(*break_point == SPACE)
                ++break_point;
            }

        while(*break_point == SPACE)
            ++break_point;

        return(break_point - linbuf);
        }
    #endif

    startofword = 0;

    /* skip leading spaces */
    for(width = 0, tptr = linbuf; *tptr == SPACE; tptr++)
        ++width;

    /* for each word */
    while(*tptr)
        {
        startofword = tptr - linbuf;

        /* walk past word - updating for @fields */
        while(*tptr  &&  (*tptr != SPACE))
            if(*tptr == '@')
                {
                /* length of @ field is number of trailing @s */
                tptr++;
                switch(toupper(*tptr))
                    {
                    case '@':
                        /* n @s to start yield (n-1) @s out */
                        break;

                    case 'D':
                    case 'T':
                    case 'P':
                        if(tptr[1] == '@')
                            tptr++;
                        else
                            tptr--;
                        break;

                    /* a rather simplistic consume here */
                    case 'G':
                    case 'F':
                        {
                        char *c;

                        /* skip over non @ part */
                        c = tptr;                       
                        while(*c  &&  (*c != '@'))
                            ++c;

                        if(!*c)
                            break;

                        /* skip over @s - GF have zero width */
                        while(*c  &&  (*c == '@'))
                            ++c;
                        tptr = c;
                        break;
                        }

                    case SLRLDI:
                        tptr += SLRSIZE;
                        break;

                    default:
                        /* is it macro parameter? */
                        if(isdigit(*tptr))
                            while(isdigit(*tptr))
                                ++tptr;
                        else
                            /* don't recognize following
                             * so count @ anyway
                            */
                            tptr--;
                        break;
                    }

                /* read past last @, updating width */
                while(*tptr++ == '@')
                    width++;
                tptr--;
                }
            else
                {
                if(!ishighlight(*tptr))
                    width++;

                tptr++;
                }

        /* end of word too far over so return offset of start of word */
        tracef3("[fndlbr width: %d, wrapwidth: %d, tptr: %s]\n",
                width, wrapwidth, tptr);

        if(width > wrapwidth  &&  startofword)
            return(startofword);

        /* walk past spaces */
        while(*tptr == SPACE)
            {
            tptr++;
            width++;
            }
        }

    return(-width);
}


/*********************************************
*                                            *
* calculate the line break point of a string *
*                                            *
*********************************************/

#if RISCOS

static intl
font_line_break(char *str, intl wrap_point)
{
    font_string fs;
    intl wrap_mp;

    wrap_mp = wrap_point * charwidth * x_scale;

    fs.s = str;
    fs.x = wrap_mp;
    fs.y = INT_MAX;
    fs.split = SPACE;
    fs.term = INT_MAX;

    font_strwidth(&fs);
    tracef6("[font_line_break str: %s, wrap: %d, width: %d, split: %d, break: %d,%s]\n",
            str, wrap_mp, fs.x, fs.split, fs.term, str + fs.term);

    /* check if we stopped before the end of the line */
    if(!fs.term || !str[fs.term])
        return(-fs.x);

    return(fs.split + 1);
}

#endif


/*
initialise word collector
*/

static void
init_colword(rowt trow)
{
    slotp sl;

    curr_inrow = curr_outrow = trow;
    out_offset = 0;
    lastword = NULL;
    lindif = 0;
    last_word_in = word_in = word_out = 0;
    old_just = old_flags = 0;
    lecpos = 0;
    dspfld_from = -1;

    /* set up initial left/right state */
    sl = travel(curcol, trow);
    cur_leftright = sl ? ((sl->justify & J_BITS) == J_LEFTRIGHT) : TRUE;
}

/* end of bufedit.c */
