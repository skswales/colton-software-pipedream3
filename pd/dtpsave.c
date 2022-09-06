/* dtpsave.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       dtpsave.c - module that saves and loads dtp (1WP actually) files
 * Author:      RJM July 1989
*/

/* assumes format of 1WP files as described in 1st Word Plus
 * Software Developers Guide, Release 1.11 of June 1987
*/

/* standard header files */
#include "flags.h"


#if defined(DTP_EXPORT)

#include "datafmt.h"

#include "dtpsave.h"


/* exported functions */

extern void dtp_change_highlights(uchar new_byte, uchar old_byte);
extern intl dtp_convert(intl c, FILE *loadinput, uchar *field_separator, 
            uchar *justify, uchar *h_byte, uchar *pageoffset, uchar *type);
extern BOOL dtp_isdtpfile(const char *bytes);
extern void dtp_load_preinit(FILE *loadinput);
extern void dtp_load_postinit(void);
extern BOOL dtp_save_preamble(FILE *output);
extern BOOL dtp_save_slot(slotp tslot, colt tcol, rowt trow, FILE *output);


/* internal functions */

static intl dtp_get_ruler(FILE *loadinput);


/* ----------------------------------------------------------------------- */

static uchar dtp_highlights[] = 
{
    8,      /* PD underline */
    1,      /* PD bold */
    0,      
    4,      /* PD italics */
    32,     /* PD subscript */
    16,     /* PD superscript */
    0,      
    0       
};      


static BOOL
chkcfm_for_dtp(colt tcol, rowt trow, BOOL first)
{
    slotp tslot;

    if(chkrpb(trow))
        return(FALSE);

    tslot = travel(tcol, trow);

    if(tslot  &&  (tslot->type == SL_TEXT))
        {
        /* note: if protected bit set will return false */
        switch(tslot->justify)
            {
            case J_FREE:
            case J_LEFTRIGHT:
            case J_RIGHTLEFT:
                if(first  ||  (*tslot->content.text != SPACE))
                    return(TRUE);

            default:
                break;
            }
        }

    return(FALSE);
}


/*
save slot in dtp format

spaces should be soft spaces ($1E), could send out stretch spaces too but no 
    point for dtp?
trailing spaces indicate to 1wp that the two lines can be joined by formatting (yuk)
so spaces need to be stored and either suppressed or forced at eos

highlights get converted to 1wp highlights and get switched off at eos
whole highlight status gets output on all highlight changes
*/

extern BOOL
dtp_save_slot(slotp tslot, colt tcol, rowt trow, FILE *output)
{
    uchar *lptr = linbuf;
    uchar ch;
    uchar h_byte = DTP_NOHIGHLIGHTS;
    intl trailing_spaces = 0;

    plain_slot(tslot, tcol, trow, 'D', linbuf);

    /* output contents, dealing with highlight chars */
    while((ch = *lptr++) != '\0')
        {
        switch(ch)
            {
            case SPACE:
                ++trailing_spaces;
                break;


#if 0
            case '@':
                while(trailing_spaces)
                    {
                    /* beware that DTP_SPACE is probably highlight char */
                    if(!away_byte(DTP_SPACE, output))
                        return(FALSE);

                    --trailing_spaces;
                    }

                ch = *lptr++;

                if(toupper(ch) != '@')
                    {
                    lptr -= 2;
                    if(!away_byte('@', output))
                        return(FALSE);
                    }

                break;
#endif

            default:
                while(trailing_spaces)
                    {
                    /* beware that DTP_SPACE is probably highlight char */
                    if(!away_byte(DTP_SPACE, output))
                        return(FALSE);

                    --trailing_spaces;
                    }

                if(ishighlight(ch))
                    {
                    /* poke highlight byte with new highlight */
                    h_byte ^= dtp_highlights[ch-FIRST_HIGHLIGHT];

                    if(!away_byte('\x1B', output) || !away_byte(h_byte, output))
                        return(FALSE);
                    }
                elif(!away_byte(ch, output))
                    return(FALSE);

                break;
            }
        }

    /* switch all highlights off */
    if(h_byte != DTP_NOHIGHLIGHTS)
        if(!away_byte('\x1B', output)  ||  !away_byte(DTP_NOHIGHLIGHTS, output))  
            return(FALSE);

    /* if this and next lines can be formatted together, output space */
    if( chkcfm_for_dtp(tcol, trow, TRUE)    &&
        chkcfm_for_dtp(tcol, trow+1, FALSE) &&
        !away_byte(DTP_SPACE, output)       )
            return(FALSE);

    return(TRUE);
}


static BOOL
dtp_head_foot(uchar *lcr_field, intl type, FILE *output)
{
    uchar array[32], expanded[PAINT_STRSIZ];
    uchar *second, *third;

    if(!str_isblank(lcr_field))
        {
        expand_lcr(lcr_field, -1, expanded, LIN_BUFSIZ, FALSE, FALSE, FALSE, FALSE);
        second = expanded + strlen(expanded) + 1;
        third  = second   + strlen(second)   + 1;

        sprintf(array, "\x1F%d", type);

        if( !away_string(array,     output) ||
            !away_string(expanded,  output) ||
            !away_byte  ('\x1F',    output) ||
            !away_string(second,    output) ||
            !away_byte  ('\x1F',    output) ||
            !away_string(third,     output) ||
            !away_byte  ('\x0A',    output) )
                return(FALSE);
        }

    return(TRUE);
}


/* 
 * Output DTP ruler.  Just output tab stops at each column position  
 * and right margin at column A right margin position
*/

static BOOL
dtp_ruler(FILE *output)
{
    coord rmargin, count;
    colt tcol = 0;

    if(!away_string("\0379[", output))
        return(FALSE);

    /* find first column on_screen and get its right margin */
    while(!colwidth(tcol))
        tcol++;

    rmargin = colwrapwidth(tcol);
    if(!rmargin)
        rmargin = colwidth(tcol);

    if( rmargin > DTP_LINELENGTH)
        rmargin = DTP_LINELENGTH;

    /* output dots */
    for(count=1, tcol=0; tcol<numcol && count < DTP_LINELENGTH; tcol++)
        {
        coord thiswidth = colwidth(tcol);
        coord scount = 0;

        /* already sent [ at start */
        if(tcol == 0)
            thiswidth--;

        for( ; scount < thiswidth-1 && count < DTP_LINELENGTH; scount++)
            if(!away_byte((uchar) ((count++ == rmargin-2) ? DTP_RMARGIN : '.'), output))
                return(FALSE);

        /* output tabstop unless it is last position */
        if( (tcol < numcol-1) && 
            !away_byte((uchar) ((count++ == rmargin-2) ? DTP_RMARGIN : DTP_TABSTOP), output))
                return(FALSE);
        }

    /* maybe still output right margin */
    if(count <= rmargin-2) 
        {
        for( ; count < rmargin-2; count++)
            if(!away_byte('.', output))
                return(FALSE);

        if(!away_byte(DTP_RMARGIN, output))         
            return(FALSE);
        }

    /* output pitch, ragged/justify, line spacing */
    if( !away_byte(DTP_PICA, output)                            ||
        !away_byte((d_options_JU == 'Y') ? '1' : '0', output)   ||
        !away_byte(d_poptions_LS + '0', output)                 )
            return(FALSE);

    return(away_eol(output));
}


extern BOOL
dtp_save_preamble(FILE *output)
{
    uchar array[LIN_BUFSIZ];

    /* send out start of dtp file */

    /* mandatory page layout */
    sprintf(array, "\0370%02d%02d%02d%02d%02d000\x00A",
                    d_poptions_PL % 100,
                    d_poptions_TM % 100,
                    d_poptions_HM % 100,
                    d_poptions_FM % 100,
                    d_poptions_BM % 100);

    if(!away_string(array, output))
        return(FALSE);

    /* optional header and footer */
    if( !dtp_head_foot(d_poptions_HE, 1, output)    ||
        !dtp_head_foot(d_poptions_FO, 2, output)    )
            return(FALSE);

    /* no footnote format */

    /* mandatory ruler line */
    if(!dtp_ruler(output))
        return(FALSE);

    return(TRUE);
}
 

/************************************************************************************
*                           1WP file loading routines
************************************************************************************/

/********************************************************
*                                                       *
* examine first few bytes of file to determine whether  *
* we have read a FirstWordPlus file                     *
*                                                       *
********************************************************/

extern BOOL
dtp_isdtpfile(const char *array)
{
    intl i;
    BOOL maybe_dtp = FALSE;

    if((*array == DTP_FORMAT_LINE)  &&  (array[1] == '0'))
        {
        /* could be 1WP page layout line.
         * If so next characters are ASCII digits giving page layout.
        */
        maybe_dtp = TRUE;

        for(i = 2; i < 12; i++)
            if(!isdigit(array[i]))
                {
                maybe_dtp = FALSE;
                break;
                }
        }

    return(maybe_dtp);
}


/****************************************
*                                       *
* get mandatory page layout and first   *
* ruler plus possible header and footer *
* ignores any other odds & sods         *
*                                       *
****************************************/

extern void
dtp_load_preinit(FILE *loadinput)
{
    uchar array[LIN_BUFSIZ];
    BOOL no_layout = TRUE;
    BOOL no_ruler = TRUE;
    intl c;

    do  {
        BOOL footer = TRUE;

        if((c = myfgetc(loadinput)) < 0)
            return;             

        if(c != DTP_FORMAT_LINE)
            continue;

        /* got a format line - process it */
        if((c = myfgetc(loadinput)) < 0)
            return;

        switch(c)       
            {
            case DTP_PAGE_LAYOUT:
                if(no_layout)
                    {
                    intl i;

                    no_layout = FALSE;

                    /* next ten bytes in ASCII decimal give margins */
                    if(fread(array, 1, 10, loadinput) != 10)
                        return;

                    /* convert to sensible bytes */
                    for(i=0; i<5; i++)
                        array[i] = (array[i*2] - '0') * 10 + array[i*2+1] - '0';

                    d_poptions_PL = *(array+0);
                    d_poptions_TM = *(array+1);
                    d_poptions_HM = *(array+2);
                    d_poptions_FM = *(array+3);
                    d_poptions_BM = *(array+4);
                    }
                break;


            case DTP_HEADER:                
                footer = FALSE;             
                /* deliberate fall-thru */

            case DTP_FOOTER:
                /* get three parameters. Force in '/', quick & nasty */
                    {
                    uchar *ptr = array+1;

                    *array = DTP_DELIMITER;

                    for(;;)
                        {
                        if((c = myfgetc(loadinput)) < 0)
                            return;             
                        
                        switch(c)
                            {
                            case DTP_FORMAT_LINE:
                                *ptr++ = DTP_DELIMITER;
                                break;

                            case CR:
                            case LF:
                                *ptr = '\0';
                                goto GOT_HEAD;

                            default:
                                *ptr++ = (uchar) c;
                                break;
                            }
                        }
                    GOT_HEAD:

                    str_set(footer  ? &d_poptions_FO
                                    : &d_poptions_HE,
                                array);
                    }
                break;


            case DTP_RULER:
                if(inserting)
                    no_ruler = FALSE;
                if(no_ruler)
                    {
                    no_ruler = FALSE;
                    c = dtp_get_ruler(loadinput);
                    break;
                    }
                /* deliberate fall-through if we don't want rulers */


            default:
                /* dunno wot this format line is so read past it */
                for(;;)
                    {
                    if((c = myfgetc(loadinput)) < 0)
                        return;             

                    if((c == CR)  ||  (c == LF))    
                        break;
                    }
                break;
            }
        }
    while(no_layout || no_ruler);
}


extern void
dtp_change_highlights(uchar new_byte, uchar old_byte)
{
    uchar diffs = new_byte ^ old_byte;  
    intl i;

    if(diffs)
        for(i = 0; i < 8; i++)
            if(dtp_highlights[i] & diffs)
                /* insert highlight i into linbuf */
                linbuf[lecpos++] = FIRST_HIGHLIGHT + i;
}


/********************************
*                               *
* get the ruler from a 1WP file *
*                               *
********************************/

static intl
dtp_get_ruler(FILE *loadinput)
{
    intl width, i, c;
    colt tcol = 0;
    coord wrappoint = 0;
    coord wrapcount = 0;

    if(!createcol(0))
        return(-1); 

    do  {
        width = 1;

        do  {
            wrapcount++;

            c = myfgetc(loadinput);

            switch(c)
                {
                case EOF:
                    return(-1);
    

                case CR:
                case LF:
                    c = CR;

                case ']':
                    /* fudge width one more so column heading doesn't coincide with 
                        with right margin
                    */
                    width++;

                case '[':
                    /* set a new column at the left margin if not 0 position */
                    if(wrapcount == 1)
                        break;                          
                    /* deliberate fall thru */

                case '#':
                case '\x7F':
                    /* got a tabstop */
                    if(!createcol(tcol))
                        return(-1);

                    set_width_and_wrap(tcol, width, 0); 

                    if(c != ']')
                        goto NEXTCOL;   

                    wrappoint = wrapcount+1;

                    for(i = 0; ; i++)
                        {
                        if((c = myfgetc(loadinput)) < 0)
                            return(c);

                        if((c == CR)  ||  (c == LF))
                            {
                            c = CR;
                            break;
                            }

                        /* look for justification and line spacing */
                        if(i == 1)
                            {
                            /* '0' is ragged, '1' is justify */
                            if(c == '0')
                                d_options_JU = 'N';
                            elif(c == '1')
                                d_options_JU = 'Y';
                            }

                        /* line spacing ? */
                        if(i == 2)
                            d_poptions_LS = c - '0';
                        }
                    break;


                default:
                    break;  
                }

            width++;
            }
        while(c != CR);


    NEXTCOL:
        tcol++;
        }
    while(c != CR);


    /* set wrap point for all columns */
    for(tcol = 0; (wrappoint > 0)  &&  (tcol < numcol); tcol++)
        {
        width = colwidth(tcol);
        set_width_and_wrap(tcol, width, wrappoint);
        wrappoint -= width;
        }

    return(c);
}


/*
munge input characters from 1WP file
returns 0 if dealt with else char
*/

extern intl
dtp_convert(intl c, FILE *loadinput, uchar *field_separator, 
            uchar *justify, uchar *h_byte, uchar *pageoffset, uchar *type)
{
    switch(c)
        {
        case DTP_INDENT_SPACE:
            /* got an indent so put in new column */                                                
            c = *field_separator;                       
            break;


        case DTP_STRETCH_SPACE:
            /* this is what VIEW calls a soft space */
            if(*justify == J_FREE)
                {
                *justify = xf_leftright ? J_LEFTRIGHT : J_RIGHTLEFT;
                xf_leftright = !xf_leftright;
                d_options_JU = 'Y';
                }

            return(0);


        case DTP_SOFT_SPACE:
            /* this is what VIEW calls a hard space */
            c = SPACE;
            break;


        case DTP_ESCAPE_CHAR:
            if((c = myfgetc(loadinput)) < 0)
                break;

            /* if top-bit set itsa a highlight change
             * else ignore the rest of the line
            */
            if(c & 0x80)
                {
                /* highlight changes */
                dtp_change_highlights((uchar) c, *h_byte);
                *h_byte = c;
                return(0);
                }
            else
                do  {
                    if((c = myfgetc(loadinput)) < 0)
                        return(c);
                    }
                while((c != CR)  &&  (c != LF));

            break;


        case DTP_SOFT_PAGE:
            if((c = myfgetc(loadinput)) < 0)
                break;

            /* c is condition strangely encoded */
            if(c > 0x80)
                *pageoffset = 256 - c;
            else
                *pageoffset = c - 16;

            /* deliberate fall-thru */

        case DTP_HARD_PAGE:                 
            *type = SL_PAGE;
            c = CR;     /* force onto new line */
            break;


        default:    
            break;
        }

    return(c);
}

#endif /* DTP_EXPORT */

/* end of dtpsave.c */
