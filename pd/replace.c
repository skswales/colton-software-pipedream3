/* replace.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       replace.c - module that does replace for search and spell
 * Author:      RJM 11-Apr-1989
*/

#include "datafmt.h"


/* ----------------------------------------------------------------------- */

extern BOOL do_replace(uchar *replace_str);

/*static void delete_this_ch(void);*/
/*static BOOL next_slot_in_replace(void);*/
/*static BOOL replace_one_word(intl tcase);*/
/*static void insert_this_ch(uchar ch, BOOL *firstone, intl tcase);*/


/*
insert the char, perhaps updating sch_end_offset
*/

static void
insert_this_ch(uchar ch, BOOL *firstone, intl tcase)
{
    if(tcase != NOCASEINFO)
        {
        if(*firstone)
            {
            *firstone = FALSE;
            ch = (tcase & FIRSTUPPER) ? toupper(ch) : tolower(ch);
            }
        else
            ch = (tcase & SECONDUPPER) ? toupper(ch) : tolower(ch);
        }

    if((curcol == sch_pos_end.col)  &&  (currow == sch_pos_end.row))
        sch_end_offset++;

    /* inserted by RJM 15.3.89, don't know how the problem was caused */
    slot_in_buffer = TRUE;

    chrina(ch, TRUE);
}


/*
replace one word at current position with one word from next_replace_word
*/

static BOOL
replace_one_word(intl tcase)
{
    BOOL firstone = TRUE;
    uchar tiovbit = xf_iovbit;

    xf_iovbit = TRUE;

    for( ; *next_replace_word; next_replace_word++)
        {
        switch(*next_replace_word)
            {
            case DUMMYHAT:
                next_replace_word++;
                switch(toupper(*next_replace_word))
                    {
                    case '#':
                        if(*++next_replace_word  &&  isdigit(*next_replace_word))
                            {
                            intl offset;

                            if((offset = (intl) (*next_replace_word - '1')) < wild_strings)
                                {
                                /* insert the (offset+1)th string from wild_string */
                                uchar *tptr;
                                LIST *lptr = search_list(&wild_string, (word32) offset);

                                tptr = lptr ? lptr->value : UNULLSTR;

                                /* insert the string */
                                while(*tptr)
                                    {
                                    insert_this_ch(*tptr++, &firstone, tcase);
                                    if(been_error)
                                        goto HAS_BEEN_ERROR;
                                    }
                                }
                            }
                        break;

                    case '?':
                        if(*++next_replace_word  &&  isdigit(*next_replace_word))
                            {
                            intl offset;

                            if((offset = (intl) (*next_replace_word - '1')) < wild_queries)
                                insert_this_ch(wild_query[offset], &firstone, tcase);
                            break;
                            }

                    default:
                        break;
                    }
                break;

            case FUNNYSPACE:
                return(TRUE);

            default:
                insert_this_ch(*next_replace_word, &firstone, tcase);
                break;
            }

        HAS_BEEN_ERROR:

        if(been_error)
            {
            xf_iovbit = tiovbit;
            return(FALSE);
            }
        }

    sch_pos_stt.row = currow;
    sch_pos_stt.col = curcol;
    sch_stt_offset = lecpos-1;

    xf_iovbit = tiovbit;
    return(TRUE);
}


/*
in the middle of replacing, hit a slot break
next slot must exist, must be text and must not be blank
*/

static BOOL
next_slot_in_replace(void)
{
    slotp tslot;

    if(!mergebuf())
        return(FALSE);

    mark_row_border(currowoffset);
    mark_slot(travel_here());

    tslot = travel(curcol, ++currow);

    if(!tslot)
        return(FALSE);

    chknlr(curcol, ++currow);

    prccon(linbuf, tslot);

    lecpos = 0;

    slot_in_buffer = TRUE;
    return(TRUE);
}


/*
delete the char, perhaps updating sch_end_offset
*/

static void
delete_this_ch(void)
{
    if((curcol == sch_pos_end.col)  &&  (currow == sch_pos_end.row))
        sch_end_offset--;

    small_DeleteCharacter_fn();
}


/*
replace the found target with the result
do it word by word, perhaps matching case of each word
current position is start of search string
sch_pos_stt , sch_stt_offset is start of search string
sch_pos_end , sch_end_offset is end of search string
replace_str points at word to replacing word
*/

extern BOOL
do_replace(uchar *replace_str)
{
    BOOL first = TRUE;

    next_replace_word = replace_str ? replace_str : UNULLSTR;

    /* delete each word and replace with a word from the replace string */

    while(  (currow != sch_pos_end.row)  ||
            (curcol != sch_pos_stt.col)  ||
            (lecpos < sch_end_offset))
        {
        intl tcase = NOCASEINFO;

        /* move over spaces and line slot breaks */
        if(linbuf[lecpos] == SPACE)
            {
            if(first  ||  !*next_replace_word)
                delete_this_ch();
            else
                lecpos++;

            continue;
            }

        first = FALSE;

        if(!linbuf[lecpos])
            {
            if(!next_slot_in_replace())
                return(FALSE);

            continue;
            }

        /* look at the case of the word before it is deleted */
        if((d_search[SCH_CASE].option == 'Y')  &&  (isalpha(linbuf[lecpos]) || isalpha(linbuf[lecpos+1])))
            {
            tcase = 0;

            if(isupper(linbuf[lecpos]))
                tcase |= FIRSTUPPER;

            if(isupper(linbuf[lecpos+1]))
                tcase |= SECONDUPPER;
            }

        /* delete the word, only works in one slot */
        while(linbuf[lecpos] != '\0' && linbuf[lecpos] != SPACE &&
                    (currow != sch_pos_end.row || curcol != sch_pos_stt.col
                        || lecpos < sch_end_offset))
            delete_this_ch();

        if(*next_replace_word == FUNNYSPACE)
            next_replace_word++;

        if(!replace_one_word(tcase))
            return(FALSE);
        }

    /* add all the other words */
    while(*next_replace_word  &&  !been_error)
        {
        if( *next_replace_word == FUNNYSPACE)
            *next_replace_word = SPACE;

        if(!replace_one_word(NOCASEINFO))
            return(FALSE);
        }

    return(!been_error);
}

/* end of replace.c */
