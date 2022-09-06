/* slotconv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       slotconv.c - slot to text conversion module
 * Author:      RJM 1987
*/

#include "datafmt.h"

#if RISCOS
#include "os.h"
#include "font.h"
#include "wimp.h"
#include "colourtran.h"

#include "ext.riscos"
#include "riscdraw.h"
#endif

/*
structure of an entry on
the find_string linked list
*/

typedef struct slrlistent *slrlep;

struct slrlistent
    {
    SLR slr;
    slrlep next;
    };

#if RISCOS

/*
entry in the font list
*/

struct font_entry
    {
    char *name;
    font handle;
    intl xsize;
    intl ysize;
    dochandle doc;
    };

typedef struct font_entry *fep;

#endif

extern BOOL ensure_global_font_valid(void);
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
extern void font_tidy_up(void);
extern uchar get_dec_field_from_opt(void);
extern intl is_font_change(char *ch);

static SLR *find_string_recurse(SLR *slotref, slrlep linked_slr);
static intl sprintnumber(uchar *array_start, uchar *array, slotp tslot,
                         BOOL eformat, intl fwidth);

#if RISCOS
extern intl font_charwid(font ch_font, intl ch);
extern os_error *font_complain(os_error *err);
extern intl font_stack(intl to_stack);
extern intl font_stringwid(font str_font, char *str);
extern intl font_unstack(intl stack_state);

static font font_cache(char *name, intl namlen, intl xs, intl ys);
static void font_cancel_hilites(char **pto);
static font font_derive(font handle, char *name, char *supplement, intl add);
static fep  font_find_block(font fonthan);
static font font_get_global(void);
static void font_insert_change(font handle, char **pto);
static intl font_strip(char *fontname, char *strip, char *newname);
static intl font_squash(intl ysize);
static void font_super_sub(intl hilite_n, char **pto);

extern BOOL print_complain(os_error *err);
#endif


/* ----------------------------------------------------------------------- */

#if RISCOS

#define NUM_HILITES 8
#define FONT_CHANGE 26
#define UL_CHANGE   25
#define COL_INVERT  18
#define COL_CHANGE  17
#define Y_SHIFT     11
#define X_SHIFT     9

static list_block *font_cache_list = NULL;
static font current_font = 0;
static font slot_font;
static char hilite_state[NUM_HILITES];
static font start_font = 0;
static intl current_shift = 0;
static intl old_height = 0;

#endif


/* ----------------------------------------------------------------------- */

static char THREE_PERCENT_D[]       = "%d.%d.%d";

static const char *
asc_months[] =
{
    month_January_STR,
    month_February_STR,
    month_March_STR,
    month_April_STR,
    month_May_STR,
    month_June_STR,
    month_July_STR,
    month_August_STR,
    month_September_STR,
    month_October_STR,
    month_November_STR,
    month_December_STR
};


/* ----------------------------------------------------------------------- */

/************************************************
*                                               *
*  copy from "from" to "to" expanding @ fields  *
*                                               *
*  MRJC added draw file handling 28/6/89        *
*  MRJC added font handling 30/6/89             *
*                                               *
************************************************/

static void
bitstr(uchar *from,
       rowt row, uchar *to /*out*/, coord fwidth,
       BOOL expand_refs, BOOL expand_ats, BOOL expand_ctrl)
{
    coord length = 0;
    intl consume;
    slotp tslot;
    colt tcol;
    rowt trow;
    dochandle curhan;
    docno doc;
    char *tempto;
    #if RISCOS
    intl i = 0;

    /* start slot with global font */
    if(riscos_fonts)
        {
        start_font = 0;
        current_shift = old_height = 0;

        /* truncated at a later point */
        fwidth = 5000;
        }

    do { hilite_state[i] = 0; } while(++i < NUM_HILITES);
    #endif

    while(*from  &&  (length <= fwidth))
        {
        /* consume = not expanding @s, yet zero width field,
         * so eat up all trailing ats so they have no width
        */
        if(*from != '@')
            {
            if(!ishighlight(*from))
                {
                #if RISCOS
                /* check for 'ISO' control characters & Acorn redefs */
                if( expand_ctrl                                                                                         &&
                    ((*from < SPACE)                            ||
                     (!riscos_fonts  &&  (*from == DELETE))     ||
                     (riscos_fonts   &&  (*from >= 127)  &&  (*from < 160)  &&  !font_charwid(current_font, *from)))
                                                                                                                        )
                    {
                    to += sprintf(to, "[%.2x]", (intl) *from);
                    from++;
                    continue;
                    }
                #endif

                *to++ = *from++;
                length++;
                continue;
                }

            #if RISCOS
            /* process highlights */
            if(!riscos_fonts)
                {
                hilite_state[*from - FIRST_HIGHLIGHT] ^= 1;
                *to++ = *from++;
                continue;
                }

            /* turn highlights into font changes on RISCOS */
            switch(*from)
                {
                case HIGH_UNDERLINE:
                    tracef0("[bitstr highlight 1 underline]\n");

                    *to++ = UL_CHANGE;

                    /* underline pos */
                    *to++ = 242;

                    /* underline height */
                    if(!hilite_state[0])
                        {
                        if(sqobit)
                            *to++ = 10;
                        else
                            *to++ = 14;
                        }
                    else
                        *to++ = 0;

                    hilite_state[0] ^= 1;
                    break;


                case HIGH_BOLD:
                    {
                    fep fp;

                    tracef0("[bitstr highlight 2 bold]\n");

                    fp = font_find_block(current_font);

                    if(fp)
                        {
                        font newfont, newfonta;
                        char basefont[MAX_FILENAME], tempfont[MAX_FILENAME];

                        /* strip out any italic suffixes */
                        font_strip(fp->name, ".Italic", basefont);
                        strcpy(tempfont, basefont);
                        font_strip(tempfont, ".Oblique", basefont);
                        strcpy(tempfont, basefont);

                        newfont = 0;

                        if(hilite_state[1])
                            {
                            font_strip(tempfont, ".Bold", basefont);

                            /* try to derive the base font */
                            if((newfont = font_derive(current_font,
                                                      basefont,
                                                      "",
                                                      TRUE)) < 1)
                                if((newfont = font_derive(current_font,
                                                          basefont,
                                                          ".Medium",
                                                          TRUE)) < 1)
                                    if((newfont = font_derive(current_font,
                                                              basefont,
                                                              ".Roman",
                                                              TRUE)) < 1)
                                        newfont = font_derive(current_font,
                                                              basefont,
                                                              ".Standard",
                                                              TRUE);

                            /* try to get italic version */
                            if((newfont > 1)  &&  hilite_state[3])
                                {
                                if((newfonta = font_derive(newfont,
                                                           NULL,
                                                           ".Italic",
                                                           TRUE)) < 1)
                                    if((newfonta = font_derive(newfont,
                                                               NULL,
                                                               ".Oblique",
                                                               TRUE)) > 0)
                                        newfont = newfonta;                             
                                }
                            }
                        else
                            {
                            font_strip(tempfont, ".Medium", basefont);
                            strcpy(tempfont, basefont);
                            font_strip(tempfont, ".Roman", basefont);
                            strcpy(tempfont, basefont);
                            font_strip(tempfont, ".Standard", basefont);

                            /* try to get bold italic version */
                            if(hilite_state[3])
                                {
                                if((newfont = font_derive(current_font,
                                                          basefont,
                                                          ".Bold.Italic",
                                                          TRUE)) < 1)
                                    newfont = font_derive(current_font,
                                                          basefont,
                                                          ".Bold.Oblique",
                                                          TRUE);
                                }

                            /* at least get bold version */
                            if(newfont < 1)
                                newfont = font_derive(current_font,
                                                      basefont,
                                                      ".Bold",
                                                      TRUE);
                            }

                        if(newfont > 0)
                            {
                            font_insert_change(newfont, &to);
                            hilite_state[1] ^= 1;
                            }
                        else
                            hilite_state[1] = 0;
                        }

                    break;
                    }


                case HIGH_ITALIC:
                    {
                    font newfont;
                    intl add;

                    tracef0("[bitstr highlight 4 italic]\n");

                    add = !hilite_state[3];
                    if((newfont = font_derive(current_font, NULL,
                                              ".Italic", add)) < 1)
                        newfont = font_derive(current_font, NULL,
                                              ".Oblique", add);
                    if(newfont > 0)
                        {
                        font_insert_change(newfont, &to);
                        hilite_state[3] ^= 1;
                        }
                    else
                        hilite_state[3] = 0;

                    break;
                    }


                case HIGH_SUBSCRIPT:
                    font_super_sub(4, &to);
                    break;


                case HIGH_SUPERSCRIPT:
                    font_super_sub(5, &to);
                    break;


                /* other highlights disappear into a big pit */
                default:
                    break;
                }

            ++from;
            continue;

            #else
            *to++ = *from++;
            continue;
            #endif
            }

        /* skip leading @ */
        ++from;

        /* process an @ field */
        *to = '\0';
        consume = 0;
        switch(toupper(*from))
            {
            /* expand a slot reference */
            case SLRLDI:
                {
                ++from;

                /* get slot coordinates */
                doc = (docno) talps(from, sizeof(docno));
                from += sizeof(docno);
                tcol = (colt) talps(from, (intl) sizeof(colt));
                force_not_abs_col(tcol);
                from += sizeof(colt);
                trow = (rowt) talps(from, (intl) sizeof(rowt));
                force_not_abs_row(trow);
                from += sizeof(rowt);

                if(!expand_ats)
                    break;

                if(check_docvalid(doc) < 0)
                    tslot = NULL;
                else
                    {
                    curhan = current_document_handle();
                    switch_document(doc);
                    tslot = travel(tcol, trow);
                    select_document_using_handle(curhan);
                    }

                /* and expand it into current slot */
                if(expand_refs)
                    {
                    if(tslot)
                        expand_slot(tslot, row, to, fwidth, FALSE, FALSE, expand_ctrl);

                    #if TRACE && RISCOS
                    trace_system("memory %p + 30", to);
                    #endif

                    /* skip string and delete funny spaces */
                    tempto = to;
                    while(*to)
                        {
                        /* evict highlights and funny spaces */
                        if(*to < SPACE)
                            {
                            ++to;
                            continue;
                            }

                        *tempto++ = *to++;
                        ++length;
                        }

                    to = tempto;
                    }
                else
                    {
                    /* if we are not allowed to expand it because we are
                     * already expanding, just print slot reference
                     * it doesn't print $ for absolutes - should it?
                    */
                    coord reflength;

                    *to++ = '@';
                    reflength = writeref(to, doc, tcol, trow);
                    to += reflength;
                    length += reflength+1;
                    *to++ = '@';
                    }

                *to = '\0';
                }
                break;


            /* display today's date */
            case 'D':
                {
                struct tm *temp_time;
                DATE now;
                int month, mday, year;

                if(from[1] == '@')
                    {
                    ++from;

                    if(!expand_ats)
                        break;

                    time(&now);
                    temp_time = localtime(&now);

                    month = temp_time->tm_mon+1;
                    mday  = temp_time->tm_mday;
                    year  = temp_time->tm_year;

                    if(d_options_DF == 'T')
                        /* generate 21 Mar 88 */
                        sprintf((char *) to, Zd_Zs_Zd_STR, mday,
                                asc_months[month-1], year + 1900);
                    else
                        /* generate 21.3.88 or 3.21.88 */
                        sprintf((char *) to, THREE_PERCENT_D,
                                (d_options_DF == 'A') ? month : mday,
                                (d_options_DF == 'A') ? mday : month,
                                year);
                    }
                else
                    {
                    /* replace @ and continue if unrecognised */
                    *to++ = '@';
                    *to   = '\0';
                    length++;
                    }
                }
                break;


            /* display title in option page */
            case 'T':
                if(from[1] == '@')
                    {
                    ++from;

                    if(!expand_ats)
                        break;

                    if(!str_isblank(d_options_DE))
                        {
                        uchar *title = d_options_DE;

                        while(*title  &&  (length <= fwidth))
                            {
                            #if RISCOS
                            if(riscos_fonts  &&  (*title < SPACE))
                                {
                                ++title;
                                continue;
                                }
                            #endif

                            *to++ = *title++;
                            length++;
                            }
                        }
                    }
                else
                    {
                    /* replace @ and continue if unrecognised */
                    *to++ = '@';
                    length++;
                    }

                *to = '\0';
                break;


            /* display current page number */
            case 'P':
                if(from[1] == '@')
                    {
                    ++from;

                    if(!expand_ats)
                        break;

                    sprintf((char *) to, "%d", curpnm);     /* must be made valid by caller */
                    }
                else
                    {
                    /* replace @ and continue if unrecognised */
                    *to++   = '@';
                    *to     = '\0';
                    length++;
                    }
                break;


            /* display @s */
            case '@':
                if(!expand_ats)
                    break;

                while(*from == '@')
                    {
                    from++;
                    length++;
                    *to++ = '@';
                    }

                *to = '\0';
                break;


            #if RISCOS
            /* font change */
            case 'F':
                {
                char *name, *startto;
                intl len;
                double xp, yp;
                font fonthan;

                /* try to make sense of the font change */
                startto = to;
                if((len = readfxy('F', &from, &to, &name, &xp, &yp)) != 0)
                    {
                    to = startto;

                    /* read current size if none given */
                    if(xp == -1)
                        {
                        fep fp;

                        if((fp = font_find_block(current_font)) != NULL)
                            {
                            xp = fp->xsize;
                            yp = fp->ysize;
                            }
                        else
                            xp = yp = 12 * 16;
                        }
                    else
                        {
                        xp *= 16;
                        yp *= 16;
                        }

                    if(riscos_fonts &&
                       (fonthan = font_cache(name, len,
                                             (intl) xp,
                                             font_squash((intl) yp))) > 0)
                        {
                        /* add font change */
                        font_cancel_hilites(&to);
                        font_insert_change(fonthan, &to);
                        }
                    else
                        font_cancel_hilites(&to);

                    consume = 1;
                    }
                else
                    {
                    /* check for font revert */
                    tracef1("[bitstr @f@ str: %s]\n", trace_string(from));
                    if((from[1] == ':' && from[2] == '@') ||
                       from[1] == '@')
                        {
                        ++from;
                        if(*from == ':')
                            ++from;

                        font_cancel_hilites(&to);

                        if(riscos_fonts)
                            font_insert_change(slot_font, &to);

                        consume = 1;
                        *to = '\0';
                        break;
                        }

                    /* replace @ and continue if unrecognised */
                    *to++ = '@';
                    length++;
                    }

                *to = '\0';
                break;
                }
            #endif


            #if RISCOS
            /* draw file */
            case 'G':
                {
                LIST *lptr;
                char *name, *startto;
                intl len, err;
                double xp, yp;

                /* look up draw file; replace reference with
                error if there is one, otherwise delete the reference */
                startto = to;
                if(row != currow &&
                   (len = readfxy('G', &from, &to, &name, &xp, &yp)) != 0)
                    {
                    err = 0;
                    if((lptr = draw_search_cache(name, len, NULL)) != NULL)
                        err = ((drawfep) lptr->value)->error;
                    else
                        err = ERR_NOTFOUND;

                    to = startto;
                    if(err)
                        {
                        intl len;
                        char *errstr;

                        errstr = reperr_getstr(err);
                        len = strlen(errstr);
                        strcpy(to, errstr);
                        to += len;
                        length += len;
                        }
                    }
                else
                    {
                    /* replace @ and continue if unrecognised */
                    *to++ = '@';
                    length++;
                    }

                *to = '\0';
                break;
                }
            #endif


            default:
                if(!expand_ats)
                    break;

                /* check for a mailing list parameter */
                if(isdigit(*from))
                    {
                    word32 key = (word32) atoi((char *) from);
                    LIST *lptr = search_list(&first_macro, key);

                    if(lptr)
                        {
                        uchar *value = lptr->value;

                        while(*value  &&  (length <= fwidth))
                            {
                            *to++ = *value++;
                            length++;
                            }
                        }
                    elif(!sqobit)
                        {
                        /* on screen copy across blank parameters */
                        *to++ = '@';
                        *to   = '\0';
                        length++;
                        break;
                        }

                    /* munge digits */
                    while(isdigit(*from))
                        ++from;
                    }
                else
                    {
                    /* replace @ and continue if unrecognised */
                    *to++ = '@';
                    length++;
                    }

                *to = '\0';
                break;

            } /* end of switch */

        /* move "to" to end of string */
        while(*to && length <= fwidth)
            {
            ++to;
            ++length;
            }

        /* skip over @s on input */
        while(*from == '@')
            {
            ++from;
            if(!expand_ats && !consume)
                *to++ = '@';
            }
        }

    /* add final delimiter */
    *to = '\0';
}


/****************************************************
*                                                   *
* expands lcr field here                            *
* finds three little strings to send off to bitstr  *
*                                                   *
****************************************************/

extern void
expand_lcr(uchar *from,
           rowt row, uchar *array /*out*/, coord fwidth,
           BOOL expand_refs, intl font_switch, BOOL expand_ctrl, BOOL compile_lcr)
{
    uchar delimiter = *from;
    uchar *to = array;
    intl i;
    char compiled[COMPILE_BUFSIZ];
    #if RISCOS
    uchar tbuf[PAINT_STRSIZ];
    #else
    uchar tbuf[LIN_BUFSIZ];
    #endif
    uchar *tptr;
    #if RISCOS
    intl old_riscos_fonts = font_stack(riscos_fonts);

    if(!font_switch)
        riscos_fonts = FALSE;

    if(riscos_fonts)
        {
        current_font = 0;

        if((slot_font = font_get_global()) > 0)
            font_insert_change(slot_font, &array);
        }
    #endif

    for(i = 0; i <= 2; i++)
        {
        if((delimiter == '@')  ||  !ispunct(delimiter))
            {
            if(compile_lcr)
                {
                compile_text_slot(compiled, from, NULL);
                tptr = compiled;
                }
            else
                tptr = from;

            bitstr(tptr,
                   row, to, fwidth,
                   expand_refs, TRUE, expand_ctrl);

            /* move to after end of string */
            while(*to)
                to += font_skip(to);
            ++to;
            break;
            }

        if(*from != delimiter)
            break;

        ++from;

        tptr = tbuf;

        while(*from  &&  (*from != delimiter))
            if(*from == SLRLDI)
                {
                memcpy(tptr, from, SLRSIZE);
                tptr += SLRSIZE;
                from += SLRSIZE;
                }
            else
                *tptr++ = *from++;

        *tptr = '\0';

        #if RISCOS
        /* revert to slot font before each section */
        if(riscos_fonts  &&  i)
            font_insert_change(slot_font, &to);
        #endif

        if(compile_lcr)
            {
            compile_text_slot(compiled, tbuf, NULL);
            tptr = compiled;
            }
        else
            tptr = tbuf;

        bitstr(tptr,
               row, to, fwidth,
               expand_refs, TRUE, expand_ctrl);

        tracef1("[expand_lcr: %s]\n", trace_string(to));

        while(*to)
            to += font_skip(to);
        ++to;
        }

    /* ensure three strings */
    memset(to, '\0', 3);

    /* restore old font state */
    #if RISCOS
    riscos_fonts = font_unstack(old_riscos_fonts);
    #endif
}


/****************************************************************************
*                                                                           *
* expand the contents of the slot into array                                *
* expand_refs says whether we are allowed to expand slot references inside  *
* the slot so text slots referring to themselves are not recursive          *
*                                                                           *
* returns the justification state                                           *
*                                                                           *
* note that strings returned by expand_slot in RISCOS can now contain       *
* font changes and thus embedded NULLs. Use the function font_skip() to     *
* work out the size of each character in the resulting string               *
*                                                                           *
****************************************************************************/

extern uchar
expand_slot(slotp tslot,
            rowt row, uchar *array /*out*/, coord fwidth,
            BOOL expand_refs, intl font_switch, BOOL expand_ctrl)
{
    slotp pslot = tslot;
    uchar justify = tslot->justify & J_BITS;
    uchar *array_start = array;
    uchar *from, *to, ch;
    BOOL loop_found;
    #if RISCOS
    intl old_riscos_fonts = font_stack(riscos_fonts);

    /* switch fonts off if requested */
    if(!font_switch)
        riscos_fonts = 0;

    if(riscos_fonts)
        {
        current_font = 0;

        if((slot_font = font_get_global()) > 0)
            font_insert_change(slot_font, &array);
        }
    #endif

    switch(tslot->type)
        {
        case SL_BAD_FORMULA:
        case SL_ERROR:
            tracef0("[expand_slot SL_ERROR]\n");
            strcpy((char *) array, 
                   reperr_getstr(tslot->content.number.result.errno));
            justify = J_LEFT;
            break;


        case SL_STRVAL:
            {
            loop_found = FALSE;

            /* we have reference to slot (or chain) containing string */
            tracef0("[expand_slot SL_STRVAL]\n");

            pslot = find_string(&(tslot->content.number.result.str_slot),
                                &loop_found);

            tracef1("[expand_slot SL_STRVAL pslot: %x]\n", (intl) pslot);
            if(!pslot)
                {
                if(loop_found)
                    {
                    justify = J_LEFT;
                    tslot->type = SL_ERROR;
                    tslot->content.number.result.errno = ERR_LOOP;
                    strcpy((char *) array, reperr_getstr(ERR_LOOP));
                    }
                else
                    {
                    justify = J_LEFT;
                    *array = '\0';
                    }
                break;
                }
            }

            /* falls through deliberately */

        case SL_INTSTR:
            /* pslot is slot with internal string, copy to array */

            tracef0("[expand_slot SL_INTSTR]\n");
            if(pslot->type == SL_TEXT)
                {
                bitstr(pslot->content.text,
                       row, array, fwidth,
                       expand_refs, TRUE, expand_ctrl);
                justify = pslot->justify & J_BITS;
                break;
                }
            elif(pslot->type != SL_INTSTR)
                from = reperr_getstr(ERR_AWAITRECALC);
            else
                from = pslot->content.number.text +
                       pslot->content.number.result.str_offset;

            to = array;
            do { ch = *from++; *to++ = ch; } while(ch);
            break;


        case SL_DATE:
            {
            struct tm *temp_time;
            int mday, month, year;

            tracef0("[expand_slot SL_DATE]\n");
            temp_time = localtime(&(tslot->content.number.result.resdate));
            month = temp_time->tm_mon+1;
            mday  = temp_time->tm_mday;
            year  = temp_time->tm_year;

            if(d_options_DF == 'T')
                /* generate 21 Mar 88 */
                sprintf((char *) array, Zd_ZP3s_Zd_STR,
                        mday, asc_months[month-1], year-YEAR_OFFSET);
            else
                /* generate 21.3.88 or 3.21.88 */
                sprintf((char *) array, THREE_PERCENT_D,
                    ((d_options_DF == 'A')) ? month : mday,
                    ((d_options_DF == 'A')) ? mday : month,
                    year-YEAR_OFFSET);

            if(justify == J_LCR)
                justify = J_LEFT;

            break;
            }


        case SL_NUMBER:
            {
            /* try to print number fully expanded - if not possible
             * try to print number in exponential notation - if not possible
             * print %
            */
            tracef0("[expand_slot SL_NUMBER]\n");

            #if RISCOS
            if(riscos_fonts)
                fwidth = ch_to_mp(fwidth);
            #endif

            if(sprintnumber(array_start, array,
                            tslot, FALSE, fwidth) >= fwidth)
                if(sprintnumber(array_start, array,
                                tslot, TRUE, fwidth) >= fwidth)
                    strcpy((char *) array, "% ");

            if(!expand_refs)
                {
                uchar *last = array + strlen((char *) array);

                if(*last == ' ')
                    *last = '\0';
                }

            if(justify == J_LCR)
                justify = J_LEFT;

            break;
            }


        case SL_TEXT:
            tracef0("[expand_slot SL_TEXT]\n");
            if(pslot->justify == J_LCR)
                expand_lcr(tslot->content.text,
                           row, array, fwidth,
                           expand_refs, font_switch, expand_ctrl, FALSE);
            else
                bitstr(tslot->content.text,
                       row, array, fwidth,
                       expand_refs, TRUE, expand_ctrl);
            break;


        case SL_PAGE:
            tracef0("[expand_slot SL_PAGE]\n");
            sprintf((char *) array, "~ %d", tslot->content.page.condval);
            justify = J_LEFT;
            break;


        default:
            break;
        }

    /* restore state of font flag */
    #if RISCOS
    riscos_fonts = font_unstack(old_riscos_fonts);
    #endif

    return(justify);
}


/************************************************
*                                               *
* follow string slot reference chain            *
* returning pointer to slot containing string   *
*                                               *
* MRJC edited 26.5.89 for EXT_REFS              *
* MRJC modified 5.6.89 to not use lists         *
*                                               *
************************************************/

extern slotp
find_string(SLR *slotref, BOOL *loop_found)
{
    SLR *resref;
    slotp sl;
    dochandle curhan;

    curhan = current_document_handle();

    if((resref = find_string_recurse(slotref, NULL)) == NULL)
        {
        *loop_found = TRUE;
        sl = NULL;
        }
    else
        {
        *loop_found = FALSE;
        sl = travel(resref->col, resref->row);
        }

    /* restore original document */
    select_document_using_handle(curhan);

    return(sl);
}


/************************************************
*                                               *
* follow a chain of string pointers, check for  *
* a loop by building a linked list on the stack *
* and return NULL if there is a loop, otherwise *
* the address of the slot reference of the      *
* resulting slot                                *
*                                               *
************************************************/

static SLR *
find_string_recurse(SLR *slotref, slrlep linked_slr)
{
    slotp sl;

    /* give up if document is invalid */
    if(check_docvalid(slotref->doc) < 0)
        return(slotref);

    tracef3("[find_string_recurse doc: %d, col: %d, row: %d]\n",
            slotref->doc, slotref->col, slotref->row);

    switch_document(slotref->doc);

    sl = travel(slotref->col, slotref->row);

    if(sl->type == SL_STRVAL)
        {
        struct slrlistent stacked_slr;
        slrlep lslr;

        /* search linked list on stack for this SLR */
        for(lslr = linked_slr; lslr; lslr = lslr->next)
            {
            if(lslr->slr.doc == slotref->doc &&
               lslr->slr.col == slotref->col &&
               lslr->slr.row == slotref->row)
                return(NULL);
            }

        stacked_slr.slr = *slotref;
        stacked_slr.next = linked_slr;

        /* wasn't on list; add it and recurse for next entry */
        return(find_string_recurse(&sl->content.number.result.str_slot,
                                   &stacked_slr));
        }

    return(slotref);
}


/*****************************************************
*                                                    *
* search font list for font/size combination; if not *
* there, add font to list                            *
*                                                    *
*****************************************************/

#if RISCOS

static font
font_cache(char *name, intl namlen, intl xs, intl ys)
{
    char namebuf[MAX_FILENAME];
    LIST *lptr;
    fep fp;
    font fonthan;
    intl res;
    char *cachedname;

    *namebuf = '\0';
    strncat(namebuf, name, namlen);
    
    tracef3("[font_cache %s, x:%d, y:%d ************]\n",
            trace_string(namebuf), xs, ys);

    /* search list for entry */
    for(lptr = first_in_list(&font_cache_list);
        lptr;
        lptr = next_in_list(&font_cache_list))
        {
        fp = (fep) lptr->value;
        if( !stricmp(namebuf, fp->name)  &&
            (fp->xsize == xs)  &&
            (fp->ysize == ys)  &&
            (fp->doc == current_document_handle()))
            {
            tracef0("[font_cache found font]\n");
            break;
            }
        }

    /* create entry for font */
    if(!lptr)
        {
        /* ask font manager for handle */
        if(font_find(namebuf, xs, ys, 0, 0, &fonthan))
            return(-1);

        tracef4("[font_cache found font: %s, size: %d, %d, handle: %d]\n",
                trace_string(namebuf), xs, ys, fonthan);

        if((lptr = add_list_entry(&font_cache_list,
                                  sizeof(struct font_entry), &res)) == NULL)
            {
            if(res < 0)
                reperr_null(res);
            return(-1);
            }

        /* set key before we do the next alloc so it can find it again */
        lptr->key = 1;

        if((cachedname = alloc_ptr_using_cache(namlen + 1, &res)) == NULL)
            {
            delete_from_list(&font_cache_list, 1);
            if(res < 0)
                reperr_null(res);
            return(-1);
            }

        /* find it again after the alloc */
        lptr = search_list(&font_cache_list, 1);

        /* change key so we don't find it here again! */
        lptr->key = 0;

        fp = (fep) lptr->value;
        fp->name = cachedname;
        strcpy(fp->name, namebuf);
        fp->handle = fonthan;
        fp->xsize = xs;
        fp->ysize = ys;
        fp->doc = current_document_handle();
        }
    
    tracef1("[font_cache returning handle: %d]\n", fp->handle);
    return(fp->handle);
}

#endif


/**********************************************
*                                             *
* calculate shift for super/subs based on     *
* old font height and whether we are printing *
*                                             *
**********************************************/

#if RISCOS

static intl
font_calshift(intl hilite_n, intl height)
{
    /* convert to 72000's */
    height = (height * 1000) / 16;

    switch(hilite_n)
        {
        /* subs */
        case 4:
            return(sqobit ? -(height / 2) : 0);

        /* super */
        case 5:
            return(sqobit ? height / 2 : height / 4);

        /* this should never happen */
        default:
            return(0);
            break;
        }
}

#endif


/*********************************************
*                                            *
* cancel all hilites activated at the moment *
*                                            *
*********************************************/

#if RISCOS

static void
font_cancel_hilites(char **pto)
{
    intl i;
    char *to;

    tracef0("[font_cancel_hilites]\n");

    to = *pto;

    if(!riscos_fonts)
        {
        for(i = 0; i < NUM_HILITES; ++i)
            if(hilite_state[i])
                {
                *to++ = FIRST_HIGHLIGHT + i;
                hilite_state[i] = 0;
                }
        }
    else
        {
        if(hilite_state[0])
            {   
            *to++ = UL_CHANGE;
            *to++ = 0;
            *to++ = 0;
            }

        if(hilite_state[4])
            {
            font_super_sub(4, &to);
            hilite_state[5] = 0;
            }

        if(hilite_state[5])
            font_super_sub(5, &to);

        for(i = 0; i < NUM_HILITES; ++i)
            hilite_state[i] = 0;
        }

    *pto = to;
}

#endif

/**************************************************
*                                                 *
* return the width of a character in a given font *
*                                                 *
**************************************************/

#if RISCOS

extern intl
font_charwid(font ch_font, intl ch)
{
    char tbuf[LIN_BUFSIZ];
    char *tptr = tbuf;

    tracef3("[font_charwid: handle = %d, char = %c, %d]\n", ch_font, ch, ch);

    font_insert_change(ch_font, &tptr);

    *tptr++ = ch;
    *tptr   = '\0';

    return(font_width(tbuf));
}

#endif


/**************************************
*                                     *
* release font handles for a document *
*                                     *
**************************************/

#if RISCOS

extern void
font_close_file(dochandle doc)
{
    LIST *lptr;

    do  {
        for(lptr = first_in_list(&font_cache_list);
            lptr;
            lptr = next_in_list(&font_cache_list))
            {
            fep fp;

            fp = (fep) lptr->value;

            if((doc == DOCHANDLE_NONE)  ||  (doc == fp->doc))
                {
                lptr->key = 1;              /* key to be deleted with */
                font_lose(fp->handle);
                dispose((void **) &fp->name);
                delete_from_list(&font_cache_list, 1);
                break;
                }
            }
        }
    while(lptr);
}

#endif


/*********************************************
*                                            *
* complain when a fonty error is encountered *
*                                            *
*********************************************/

#if RISCOS

extern os_error *
font_complain(os_error *err)
{
    if(err)
        {
        if(riscos_printing)
            print_complain(err);
        else
            {
            reperr(ERR_FONTY, err->errmess);
            riscos_fonts = FALSE;
            riscos_font_error = TRUE;
            xf_draweverything = TRUE;   /* screen is in a mess */
            xf_interrupted = TRUE;      /* we have work to do! */
            }
        }

    return(err);
}

#endif


/*************************************************
*                                                *
* given a font handle and a string, derive a new *
* font name and try to cache it                  *
*                                                *
*************************************************/

#if RISCOS

static font
font_derive(font handle, char *name, char *supplement, intl add)
{
    fep fp;
    char namebuf[MAX_FILENAME];

    tracef2("[font_derive handle: %d, add: %s]\n",
            handle, trace_string(supplement));

    if((fp = font_find_block(handle)) == NULL)
        return(-1);

    /* should we add or remove the supplement ? */
    if(add)
        {
        if(!name)
            strcpy(namebuf, fp->name);
        else
            strcpy(namebuf, name);
        strcat(namebuf, supplement);
        }
    else
        if(font_strip(fp->name, supplement, namebuf) < 0)
            return(-1);

    tracef1("[font_derive_font about to cache: %s]\n", trace_string(namebuf));

    return(font_cache(namebuf, strlen(namebuf), fp->xsize, fp->ysize));
}

#endif

/******************************************************
*                                                     *
* expand a string including font information ready    *
* for line break calculation - @ fields are converted *
* to the relevant number of @s, font information is   *
* inserted; expects riscos_fonts to be TRUE           *
* and knows ctrl chars are displayed expanded         *
*                                                     *
******************************************************/

#if RISCOS

extern void
font_expand_for_break(char *to, char *from)
{
    current_font = 0;

    if((slot_font = font_get_global()) > 0)
        font_insert_change(slot_font, &to);

    bitstr(from, -1, to, 0, FALSE, FALSE, TRUE);
}

#endif


/*******************************************************
*                                                      *
* return pointer to a font block given the font handle *
*                                                      *
*******************************************************/

#if RISCOS

static fep
font_find_block(font fonthan)
{
    LIST *lptr;

    tracef1("[font_find_block: %d]\n", fonthan);

    for(lptr = first_in_list(&font_cache_list);
        lptr;
        lptr = next_in_list(&font_cache_list))
        {
        fep fp;

        fp = (fep) lptr->value;

        if(fp->handle == fonthan)
            {
            tracef1("[font_find_block found: %d]\n", fonthan);
            return(fp);
            }
        }

    tracef1("[font_find_block %d failed]\n", fonthan);
    return(NULL);
}

#endif

/***********************
*                      *
* load the global font *
*                      *
***********************/

#if RISCOS

static font
font_get_global(void)
{
    font glob_font;

    tracef1("[font_get_global: %s]\n", trace_string(global_font));

    if((glob_font = font_cache(global_font,
                               strlen(global_font),
                               global_font_x,
                               font_squash(global_font_y))) <= 0)
        {
        riscos_fonts = FALSE;
        riscos_font_error = TRUE;
        }

    return(glob_font);
}

#endif

/********************************************
*                                           *
* insert a font change given a font handle; *
* keep track of the current font            *
*                                           *
********************************************/

#if RISCOS

static void
font_insert_change(font handle, char **pto)
{
    tracef1("[font_insert_change: handle = %d]\n", handle);

    if(handle)
        {
        *(*pto)++ = FONT_CHANGE;
        *(*pto)++ = (char) handle;
        current_font = handle;  
        }
}

#endif


/*****************************************************
*                                                    *
* insert a font colour inversion given a font handle *
*                                                    *
*****************************************************/

#if RISCOS

static void
font_insert_colour_inversion(font_state *fp, char **pto)
{
    intl curbg, curfg, curoffset;
    intl newbg, newfg, newoffset;

    tracef0("[font_insert_colour_inversion]\n");

    curbg = fp->back_colour;
    curfg = fp->fore_colour;
    curoffset = fp->offset;

    tracef3("[inversion: bg %d, fg %d, offset %d]\n", curbg, curfg, curoffset);

    if(log2bpp == 3)
        {
        /* bg ignored by font manager in 256 colour modes,
         * but preserve to reinvert later on
        */
        newbg = curfg;
        newfg = curbg;
        newoffset = curoffset;
        }
    else
        {
        newbg = curfg + ((signed char) curoffset);
        newfg = curbg + ((signed char) curoffset);
        newoffset = -((signed char) curoffset);
        }

    tracef3("[inversion: bg' %d, fg' %d, offset' %d]\n", newbg, newfg, newoffset);

    fp->back_colour = newbg;
    fp->fore_colour = newfg;
    fp->offset = newoffset;

    *(*pto)++ = COL_INVERT;
    *(*pto)++ = (char) newbg;
    *(*pto)++ = (char) newfg;
    *(*pto)++ = (char) newoffset;
}

#endif


/****************************************
*                                       *
* insert a y shift into the font string *
*                                       *
****************************************/

#if RISCOS

static void
font_insert_shift(intl type, intl shift, char **pto)
{
    *(*pto)++ = (uchar) type;
    *(*pto)++ = (uchar) (shift & 0xFF);
    *(*pto)++ = (uchar) ((shift >> 8) & 0xFF);
    *(*pto)++ = (uchar) ((shift >> 16) & 0xFF);
    plusab(current_shift, shift);
}

#endif


/****************************************************
*                                                   *
* skip over font changes and other mess in a string *
*                                                   *
****************************************************/

extern intl
font_skip(char *at)
{
    #if RISCOS

    if(riscos_fonts)
        switch(*at)
            {
            case FONT_CHANGE:
            case COL_CHANGE:
                return(2);

            case UL_CHANGE:
                return(3);

            case COL_INVERT:
            case X_SHIFT:
            case Y_SHIFT:
                return(4);
            }
    #endif

    return(1);
}


/***************************************
*                                      *
* given a font y size, squash it down  *
* to maximum line height on the screen *
*                                      *
***************************************/

#if RISCOS

static intl
font_squash(intl ysize)
{
    intl leading_16p;

    if(sqobit)
        return(ysize);

    leading_16p = (global_font_leading * 16) / 1000;
    return(ysize > leading_16p ? leading_16p : ysize);
}

#endif


/*******************************************************
*                                                      *
* return a value for riscos_fonts to save on the stack *
*                                                      *
*******************************************************/

#if RISCOS

extern intl
font_stack(intl to_stack)
{
    return(to_stack);
}

#endif


/***********************************************
*                                              *
* return the width of a string in a given font *
*                                              *
***********************************************/

#if RISCOS

extern intl
font_stringwid(font str_font, char *str)
{
    char tbuf[PAINT_STRSIZ];
    char *tptr = tbuf;

    font_insert_change(str_font, &tptr);

    while(*str)
        if(is_font_change(str))
            {
            intl n_char = font_skip(str);

            memcpy(tptr, str, n_char);
            tptr += n_char;
            str += n_char;
            }
        else            
            *tptr++ = *str++;

    *tptr = '\0';

    return(font_width(tbuf));
}

#endif


/***********************************
*                                  *
* strip a section from a font name *
*                                  *
***********************************/

#if RISCOS

static intl
font_strip(char *fontname, char *strip, char *newname)
{
    char *place;

    /* look for supplement in name */
    if((place = stristr(fontname, strip)) == NULL)
        {
        strcpy(newname, fontname);
        return(-1);
        }

    *newname = '\0';
    strncat(newname, fontname, place - fontname);
    strcat(newname, place + strlen(strip));
    return(0);
}

#endif


/***************************************
*                                      *
* copy a string into a buffer ignoring *
* unwanted non-fonty characters        *
*                                      *
***************************************/

#if RISCOS

static intl
font_strip_copy(char *to, char *from)
{
    char *oldto = to;

    while(*from)
        {
        if(riscos_fonts && *from < SPACE)
            {
            ++from;
            continue;
            }

        *to++ = *from++;
        }

    *to++ = '\0';

    return(to - oldto);
}

#endif


/*************************************************************
*                                                            *
* handle super/subscripts                                    *
* switching off a super/subscript reverts to the font active *
* when the first super/subs was switched on - this includes  *
* bold and italic - which are cancelled by a super/subs off  *
*                                                            *
*************************************************************/

#if RISCOS

static void
font_super_sub(intl hilite_n, char **pto)
{
    /* off highlight of a pair - revert to old state */
    if(hilite_state[hilite_n])
        {
        font_insert_shift(Y_SHIFT, -current_shift, pto);
        font_insert_change(start_font, pto);
        start_font = 0;
        old_height = 0;

        /* reset hilight states */
        hilite_state[4] = hilite_state[5] = 0;
        }
    else
        {
        /* already in a super/subs ? */
        if(start_font)
            {
            font_insert_shift(Y_SHIFT,
                              font_calshift(hilite_n, old_height)
                                                - current_shift,
                              pto);
            hilite_state[hilite_n] ^= 1;
            }
        else
            /* not in either super or subs */
            {
            fep fp = font_find_block(current_font);

            if(fp)
                {
                font subfont;
    
                if((subfont = font_cache(fp->name,
                                         strlen(fp->name),
                                         (fp->xsize * 3) / 4,
                                         (fp->ysize * 3) / 4)) > 0)
                    {
                    old_height = fp->ysize;
                    start_font = fp->handle;

                    font_insert_change(subfont, pto);
                    font_insert_shift(Y_SHIFT, font_calshift(hilite_n,
                                                             old_height), pto);

                    hilite_state[hilite_n] ^= 1;
                    }
                }
            }
        }
}

#endif


/***************************
*                          *
* release all font handles *
*                          *
***************************/

#if RISCOS

extern void
font_tidy_up(void)
{
    font_close_file(DOCHANDLE_NONE);
}

#endif


/******************************************************
*                                                     *
* return the unstacked riscos_fonts value, accounting *
* for a font error in the meantime                    *
*                                                     *
******************************************************/

#if RISCOS

extern intl
font_unstack(intl stack_state)
{
    if(riscos_font_error)
        return(FALSE);

    return(stack_state);
}

#endif


extern uchar
get_dec_field_from_opt(void)
{
    uchar decimals = d_options_DP;

    return((decimals == 'F') ? (uchar) 0xF : (uchar) (decimals-'0'));
}


/************************************************************
*                                                           *
* check to see if character is a font change or a highlight *
*                                                           *
************************************************************/

extern intl
is_font_change(char *ch)
{
    #if RISCOS
    if(riscos_fonts)
        {
        switch(*ch)
            {
            case FONT_CHANGE:
            case COL_CHANGE:
            case COL_INVERT:
            case UL_CHANGE:
            case X_SHIFT:
            case Y_SHIFT:
                return(1);
            }

        return(0);
        }
    #endif

    return(ishighlight(*ch));
}
                            

/****************************************************************
*                                                               *
* sprintnumber does a print of the number in tslot into array   *
* it prints lead chars, '-' | '(', number, ')', trail chars     *
* if eformat is FALSE it prints number in long form             *
* if eformat is TRUE  it prints number in e format              *
* it returns the number of non-space characters printed         *
* the format is picked up from the slot or from the option page *
*                                                               *
****************************************************************/

static uchar float_e_format[]       = "%g";
static uchar decimalsformat[]       = "%.0f";
static uchar decimals_e_format[]    = "%.0e";
#define PLACES_OFFSET 2

static intl
sprintnumber(uchar *array_start, uchar *array,
             slotp tslot, BOOL eformat, intl fwidth)
{
    uchar floatformat[8];   /* written to to create "%0.nng" format below */
    uchar *formatstr, *nextprint;
    uchar decimals, format;
    double value;
    BOOL negative, brackets;
    uchar *tptr;
    uchar ch;
    intl len, width;
    char t_lead[10], t_trail[10]; 
    #if RISCOS
    intl digitwid_mp, dotwid_mp, thswid_mp, bracwid_mp;
    #endif

    negative = brackets = FALSE;
    nextprint = array;

    #if RISCOS
    if(riscos_fonts)
        {
        digitwid_mp = font_charwid(current_font, '0');
        dotwid_mp = font_charwid(current_font, '.');

        if(d_options_TH != TH_BLANK)
            switch(d_options_TH)
                {
                case TH_DOT:
                    thswid_mp = dotwid_mp;
                    dotwid_mp = font_charwid(current_font, ',');
                    break;
                case TH_COMMA:
                    thswid_mp = font_charwid(current_font, ',');
                    break;
                default:
                    thswid_mp = font_charwid(current_font, ' ');
                    break;
                }
        }
    #endif

    format = tslot->content.number.format;
    value  = tslot->content.number.result.value;
#if 0
    if(value > 1e132)
        eformat = TRUE;
#endif

    if(value < 0.)
        {
        value = 0. - value;
        negative = TRUE;
        }

    brackets = (format & F_DCP)
                    ? (format & F_BRAC)
                    : (d_options_MB == 'B');

    #if RISCOS
    if(brackets  &&  riscos_fonts)
        bracwid_mp = font_charwid(current_font, ')');
    #endif

    decimals = (format & F_DCP)
                    ? (format & F_DCPSID)
                    : get_dec_field_from_opt();

    /* process leading and trailing characters */
    if(format & F_LDS)
        {
        if(d_options_LP)
            {
            #if RISCOS
            if(riscos_fonts)
                font_strip_copy(t_lead, d_options_LP);
            else
            #endif
                strcpy(t_lead, d_options_LP);
            }
        else
            *t_lead = '\0';
        }

    if(format & F_TRS)
        {
        if(d_options_TP)
            {
            #if RISCOS
            if(riscos_fonts)
                font_strip_copy(t_trail, d_options_TP);
            else
            #endif
                strcpy(t_trail, d_options_TP);
            }
        else
            *t_trail = '\0';
        }

    if(decimals == 0xF)
        {
        if(eformat)
            formatstr = float_e_format;
        else
            {
            intl padding, logval;

            #if RISCOS
            if(riscos_fonts)
                {
                padding  = brackets ? bracwid_mp : 0;
                padding += negative ?
                                (brackets ? bracwid_mp
                                          : font_charwid(current_font, '-'))
                                    : 0;
                }
            else
            #endif
                {
                padding =  brackets ? 1 : 0;
                padding += negative ? 1 : 0;
                }

            if(format & F_LDS)
                {
                #if RISCOS
                if(riscos_fonts)
                    padding += font_stringwid(current_font, t_lead);
                else
                #endif
                    padding += strlen(t_lead);
                }

            if(format & F_TRS)
                {
                #if RISCOS
                if(riscos_fonts)
                    padding += font_stringwid(current_font, t_trail);
                else
                #endif
                    padding += strlen(t_trail);
                }

            formatstr = floatformat;

            logval = (intl) ((value == 0.) ? 0. : log10(value));

            if(d_options_TH != TH_BLANK)
                {
                #if RISCOS
                if(riscos_fonts)
                    padding += (logval / 3) * thswid_mp;
                else
                #endif
                    padding += logval / 3;
                }

            width = fwidth - padding;
            #if RISCOS
            if(!riscos_fonts)
            #endif
                width -= 1;

            if(value != floor(value))
                {
                /* if displaying fractional part, allow  for . */
                #if RISCOS
                if(riscos_fonts)
                    {
                    if((width / digitwid_mp) > logval)
                        width -= dotwid_mp;

                    /* if too small */
                    if(logval < 0 && -logval > (width / digitwid_mp))
                        return(INT_MAX);
                    }
                else
                #endif
                    {
                    if(width > logval)
                        width--;

                    /* if too small */
                    if(logval < 0 && -logval > width)
                        return(INT_MAX);
                    }
                }

            /* account for leading zero */
            if(value < 1.)
                {
                #if RISCOS
                if(riscos_fonts)
                    width -= digitwid_mp;
                else
                #endif
                    width -= 1;
                }

            /* calculate space for digits */
            #if RISCOS
            if(riscos_fonts)
                {
                tracef2("[sprintnumber width_mp: %d, width_digit: %d]\n",
                        width, (intl) width / digitwid_mp);
                width /= digitwid_mp;
                }
            #endif

            sprintf((char *) formatstr, "%%.%dg", width);
            }
        }
    else
        {
        formatstr = eformat ? decimals_e_format
                            : decimalsformat;

        formatstr[PLACES_OFFSET] = (uchar) (decimals + '0');
        }

    /* print leadin characters */
    if(format & F_LDS)
        {
        strcpy(nextprint, t_lead);
        nextprint += strlen(t_lead);
        }

    *nextprint = '\0';

    /* print number */
    tptr = nextprint;
    if(negative)
        *nextprint++ = (uchar) (brackets ? '(' : '-');

    len = sprintf((char *) nextprint, (char *) formatstr, value);

    tracef2("[sprintnumber formatstr: %s, result: %s]\n",
            trace_string(formatstr), trace_string(nextprint));

    /* work out thousands separator */
    if(d_options_TH != TH_BLANK && !eformat)
        {
        char *dotp, separator;
        intl beforedot;

        for(beforedot = 0, dotp = nextprint; *dotp != DOT && *dotp; ++dotp)
            ++beforedot;

        switch(d_options_TH)
            {
            case TH_COMMA:
                separator = COMMA;
                break;
            case TH_DOT:
                separator = DOT;
                if(*dotp)
                    *dotp = COMMA;
                break;
            default:
                separator = SPACE;
                break;
            }

        while(beforedot > 3)
            {
            intl inc;

            inc = ((beforedot - 1) % 3) + 1;
            nextprint += inc;
            len -= inc;

            memmove(nextprint + 1, nextprint, len + 1);
            *nextprint++ = separator;

            beforedot -= inc;
            }
        }

    while(*nextprint)
        ++nextprint;

    if(negative && brackets)
        strcpy((char *) nextprint++, ")");

    /* get rid of superfluous E padding */
    /*if(eformat)*/
        {
        while((ch = *tptr++) != '\0')
            if(ch == 'e')
                {
                if(*tptr == '+')
                    {
                    memmove(tptr, tptr+1, (unsigned) strlen((char *) tptr));
                    nextprint--;
                    }
                elif(*tptr == '-')
                    tptr++;

                while((*tptr == '0')  &&  (*(tptr+1) != '\0'))
                    {
                    memmove(tptr, tptr+1, (unsigned) strlen((char *) tptr));
                    nextprint--;
                    }
                break;
                }
        }

    /* print trailing characters */
    if(format & F_TRS)
        {
        strcpy(nextprint, t_trail);
        nextprint += strlen(t_trail);
        }

    *nextprint = '\0';

    /* do font width before we add funny space */
    #if RISCOS
    if(riscos_fonts)
        {
        intl expect_wid, xshift = 0;

        tracef1("[sprintnumber string width is: %d]\n",
                font_width(array_start));

        /* work out x shift to add to align numbers at the point */
        if(decimals != 0xF)
            {
            char *dot, *comma, *point;
            intl widpoint;

            dot = strrchr(array, '.');
            comma = strrchr(array, ',');

            if(dot || comma)
                {
                point = max(dot, comma);
                widpoint = font_stringwid(current_font, point + 1);
                expect_wid = decimals * digitwid_mp;
                if(format & F_TRS)
                    expect_wid += font_stringwid(current_font, t_trail);
                if(brackets)
                    expect_wid += bracwid_mp;
                if(widpoint <= expect_wid)
                    xshift = expect_wid - widpoint;
                }
            }

        /* increase shift for positive (bracketed) numbers */
        if(!xshift && brackets && !negative)
            xshift += bracwid_mp;

        if(xshift)
            {
            tracef2("[sprintnumber shift: %d, expected: %d]\n",
                    xshift, expect_wid);

            font_insert_shift(X_SHIFT, xshift, &nextprint);
            }

        *nextprint = '\0';
        width = font_width(array_start);
        }
    #endif

    /* add spaces on end, but don't count them in length */
    #if RISCOS
    if(!riscos_fonts)
    #endif
        if(brackets && !negative)
            {
            *nextprint++ = FUNNYSPACE;
            *nextprint = '\0';
            }

    #if RISCOS
    if(riscos_fonts)
        {
        tracef2("[sprintnumber returns width: %d, fwidth is: %d]\n",
                width, fwidth);
        return(width);
        }
    else
    #endif
        {
        tracef2("[sprintnumber returns width: %d, fwidth is: %d]\n",
                nextprint - array, fwidth);
        return(nextprint - array);
        }
}


#if RISCOS

extern void
expand_current_slot_in_fonts(char *to /*out*/, BOOL partial, intl *this_fontp /*out*/)
{
    intl offset = strlen(linbuf);
    const char *from = (const char *) linbuf + min(lescrl, offset);
    intl split = lecpos - lescrl;
    char ch;
    font_state f;
    const char *p1 = (word_to_invert ? (const char *) linbuf + lecpos : NULL);
    const char *p2 = (word_to_invert ? p1 + strlen(word_to_invert) : NULL);

    current_font = 0;

    if((slot_font = font_get_global()) > 0)
        font_insert_change(slot_font, &to);

    if(this_fontp)
        *this_fontp = slot_font;

    if(p1)
        {
        tracef3("[word to invert is &%p &%p %s]\n", p1, p2, word_to_invert);

        /* find out what colours we must use */

        if(log2bpp == 3)
            {
            font fh;
            wimp_paletteword bg, fg;
            int offset;

            fh = slot_font;
            bg.word = rgb_for_wimpcolour(current_bg);
            fg.word = rgb_for_wimpcolour(current_fg);
            offset = 14;
            tracef2("[expand_current_slot (256) has RGB bg %8.8X, fg %8.8X]\n", bg.word, fg.word);
            font_complain(colourtran_returnfontcolours(&fh, &bg, &fg, &offset));
            f.fore_colour = fg.word;
            f.offset = offset;
            tracef2("[expand_current_slot (256) gets fg %d, offset %d]\n", fg.word, offset);

            fh = slot_font;
            bg.word = rgb_for_wimpcolour(current_fg);
            fg.word = rgb_for_wimpcolour(current_bg);
            offset = 14;
            tracef2("[expand_current_slot (256) has RGB bg %8.8X, fg %8.8X]\n", bg.word, fg.word);
            font_complain(colourtran_returnfontcolours(&fh, &bg, &fg, &offset));
            f.back_colour = fg.word;
            tracef2("[expand_current_slot (256) gets bg %d, offset %d]\n", fg.word, offset);
            }
        else
            {
            font_complain(font_current(&f));
            tracef3("[expand_current_slot gets bg %d, fg %d, offset %d]\n", f.back_colour, f.fore_colour, f.offset);
            }
        }

    while((split > 0)  ||  !partial)
        {
        if(p1  &&  ((p1 == from)  ||  (p2 == from)))
            font_insert_colour_inversion(&f, &to);

        ch = *from;

        if(!ch)
            {
            if(split > 0)
                {
                /* must pad with spaces - kill inversion check too */
                ch = SPACE;
                p1 = NULL;
                }
            else
                break;
            }
        else
            from++;

        /* check for highlights, 'ISO' control characters & Acorn redefs */
        if(ishighlight(ch))
            {
            *to++ = '[';
            *to++ = (ch - FIRST_HIGHLIGHT) + FIRST_HIGHLIGHT_TEXT;
            *to++ = ']';
            }
        elif((ch < SPACE)  ||
             ((ch >= 127)  &&  (ch < 160)  &&  !font_charwid(current_font, ch)))
            {
            to += sprintf(to, "[%.2x]", ch);
            }
        elif(ch)
            *to++ = ch;

        split--;
        }

    *to = '\0';
}


extern BOOL
ensure_global_font_valid(void)
{
    return(font_get_global() > 0);
}

#endif /* RISCOS */

/* end of slotconv.c */
