/* viewio.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       viewio.c - module that saves and loads VIEW files
 * Author:      RJM March 1989
*/

/* standard header files */
#include "flags.h"


#if defined(VIEW_IO)

#include "datafmt.h"

#include "viewio.h"


/* exported functions */

extern intl view_convert(intl c, FILE *loadinput, uchar *lastch,
                         BOOL *been_ruler, uchar *justify, uchar *type,
                         uchar *pageoffset);
extern uchar view_decode_h(intl second, intl third);
extern void view_load_preinit(void);
extern void view_load_postinit(void);
extern BOOL view_save_ruler_options(coord *rightmargin, FILE *output);
extern BOOL view_save_slot(slotp tslot, colt tcol, rowt trow,
                           FILE *output, coord *v_chars_sofar,
                           intl *splitlines, coord rightmargin);
extern BOOL view_save_stored_command(const char *command, FILE *output);


/* internal functions */

static intl view_get_ruler(FILE *loadinput);
static intl view_get_stored_command(FILE *loadinput, uchar *justify,
                                    uchar *type, uchar *pageoffset);


/* ----------------------------------------------------------------------- */

static BOOL pending_space;      /* early VIEW has habit of eating real spaces! */


#define OPT_TABLE struct _opt_table
OPT_TABLE
    {
    uchar p_OFFset;     /* offset in d_poptions */
    uchar v_default;
    uchar v_sc1;
    uchar v_sc2;
    uchar v_done;
    };


/* indexes for VIEW stored commands which need to be dealt with but which
 * don't have parallels in d_poptions 
*/

#define O_LJ    ((uchar) 128)
#define O_CE    ((uchar) 129)
#define O_RJ    ((uchar) 130)
#define O_PE    ((uchar) 131)
#define O_PB    ((uchar) 132)

static OPT_TABLE view_opts[] =
    {
    {   O_PL,   66,         'P',    'L',    FALSE   },
    {   O_HE,   255,        'D',    'H',    FALSE   },
    {   O_FO,   255,        'D',    'F',    FALSE   },
    {   O_LS,   0,          'L',    'S',    FALSE   },
    {   O_TM,   4,          'T',    'M',    FALSE   },
    {   O_HM,   4,          'H',    'M',    FALSE   },
    {   O_FM,   4,          'F',    'M',    FALSE   },
    {   O_BM,   4,          'B',    'M',    FALSE   },
    {   O_LM,   0,          'L',    'M',    FALSE   },
    {   O_PS,   255,        'S',    'R',    FALSE   },  /* convert start page to SR P n */
#define NO_OF_PAGE_OPTS 10 
    {   O_PE,   J_FREE,     'P',    'E',    FALSE   },
    {   O_PB,   J_FREE,     'P',    'B',    FALSE   },
    {   O_LJ,   J_LEFT,     'L',    'J',    FALSE   },
    {   O_CE,   J_CENTRE,   'C',    'E',    FALSE   },
    {   O_RJ,   J_RIGHT,    'R',    'J',    FALSE   }
    };
#define NO_OF_ALL_OPTS (sizeof(view_opts) / sizeof(OPT_TABLE)) 


static const char *highlight_table[] =
    {
    "\x1C",                         /* PD H1 */
    "\x1D\x1D\x1D",                 /* PD H2 */
    "",                             /* PD H3 */
    "\x1D\x1C\x1D",                 /* PD H4 */
    "\x1D\x1C",                     /* PD H5 */
    "\x1D\x1D",                     /* PD H6 */
    "\x1D\x1C\x1C",                 /* PD H7 */
    "",                             /* PD H8 */
    };
#define HTABLE_SIZE 8 


extern intl
view_convert(intl c, FILE *loadinput, uchar *lastch, BOOL *been_ruler,
             uchar *justify, uchar *type, uchar *pageoffset)
{
    tracef2("[view_convert(%c %d)]\n", c, c);

    if(pending_space)
        {
        pending_space = FALSE;

        switch(c)
            {
            case CR:
            case LF:
            case SPACE:
                break;

            default:
                linbuf[lecpos++] = SPACE;
                break;
            }
        }

    switch(c)
        { 
        case VIEW_HIGH_UNDERLINE:
            c = HIGH_UNDERLINE;
            break;


        case VIEW_HIGH_BOLD:
            c = HIGH_BOLD;
            break;


        case VIEW_LEFT_MARGIN:
            c = TAB;
            break;


        case '|':
            if((c = myfgetc(loadinput)) < 0)
                return(c);

            switch(toupper(c))
                {
                case 'D':
                case 'P':
                    *lastch = linbuf[lecpos++] = '@';
                    break;

                default:
                    *lastch = linbuf[lecpos++] = '|';
                    break;
                }

            break;


        /* look for ruler or stored command at beginning of line */
        case VIEW_STORED_COMMAND:
            if(lecpos == 0)
                c = view_get_stored_command(loadinput, justify, type, pageoffset);

            break;


        case VIEW_RULER:
            if(inserting)
                *been_ruler = TRUE;

            if((lecpos == 0)  &&  !*been_ruler)
                {
                c = view_get_ruler(loadinput);
                *been_ruler = TRUE;
                }
            else
                {
                /* read and ignore ruler */
                do  {
                    if((c = myfgetc(loadinput)) < 0)
                        return(c);
                    }
                while((c != CR)  &&  (c != LF));

                c = 0;      /* ruler ignored - get more input */
                }

            tracef2("[view_convert returns %c %d]\n", c, c);
            return(c);


        case VIEW_SOFT_SPACE:
            if(*justify == J_FREE)
                {
                *justify = xf_leftright ? J_LEFTRIGHT : J_RIGHTLEFT;
                xf_leftright = !xf_leftright;
                d_options_JU = 'Y';
                }

            pending_space = TRUE;

            tracef2("[view_convert returns %c %d]\n", 0, 0);
            return(0);          /* soft space processed - get more input */


        default:
            break;
        }

    tracef2("[view_convert returns %c %d]\n", c, c);
    return(c);
}


/************************************************
*                                               *
*  find highlight sequence in highlight table   *
*  Check second and third characters            *
*                                               *
************************************************/

extern uchar
view_decode_h(intl second, intl third)
{
    uchar i;

    for(i = 0; i < HTABLE_SIZE; i++)
        {
        const char *ptr = highlight_table[i];

        if(!*ptr)
            return(0);

        if((*(ptr+1) == (char) second)  &&  (*(ptr+2) == (char) third))
            return(i + FIRST_HIGHLIGHT);
        }

    return(0);
}


/********************************
*                               *
* read ruler and set up columns *
*                               *
********************************/

static intl
view_get_ruler(FILE *loadinput)
{
    intl c, width;
    colt tcol = 0;
    coord wrappoint = 0;
    coord wrapcount = 0;

    if(((c = myfgetc(loadinput)) != '.')  ||  ((c = myfgetc(loadinput)) != '.')) 
        return(c);

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

                case '>':
                case '*':
                    if(!createcol(tcol))
                        return(-1);

                    set_width_and_wrap(tcol, width, 0);
                    goto NEXTCOL;

                case '<':
                    wrappoint = wrapcount+1;
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

    return(0);      /* ruler processed - get more input */
}


/********************************************************
*                                                       *
*  read stored command and set up any PipeDream flags   *
*                                                       *
********************************************************/

static intl
view_get_stored_command(FILE *loadinput, uchar *justify, uchar *type, uchar *pageoffset)
{
    intl c, ch1, ch2;
    intl a_index = 0;
    uchar array[LIN_BUFSIZ];

    ch1 = myfgetc(loadinput);
    if(!isupper(ch1))
        return(ch1);

    ch2 = myfgetc(loadinput);
    if(!isupper(ch2))
        return(ch2);

    tracef2("[view_get_stored_command: Got stored command %c%c]\n", ch1, ch2);

    do  {
        c = myfgetc(loadinput);

        tracef2("got char %c %d for stored command\n", c, c);

        switch(c)
            {
            case EOF:
                return(-1);

            case CR:
            case LF:
                c = '\0';

            default:
                if(a_index >= LIN_BUFSIZ)
                    return(-1);

                array[a_index++] = (uchar) c;
                break;
            }
        }
    while(c);


    update_dialog_from_windvars(D_POPTIONS);

    for(a_index = 0; a_index < NO_OF_ALL_OPTS; a_index++)
        {
        intl add_in = 0;
        intl number, option;

        tracef2("Comparing with stored command %c%c\n",
                view_opts[a_index].v_sc1, view_opts[a_index].v_sc2);

        if((ch1 == view_opts[a_index].v_sc1)  &&  (ch2 == view_opts[a_index].v_sc2))
            {
            tracef2("Matched stored command %c%c\n", ch1, ch2);

            /* only store first occurrence of option */
            if(!view_opts[a_index].v_done)
                {
                view_opts[a_index].v_done = TRUE;

                option = view_opts[a_index].p_OFFset;

                switch(option)
                    {
                    case O_HE:
                    case O_FO:
                        (void) str_set(&d_poptions[option].textfield, array);
                        break;


                    case O_LS:
                        add_in = 1;

                    case O_PL:
                    case O_TM:
                    case O_HM:
                    case O_FM:
                    case O_BM:
                    case O_LM:
                        number = atoi(array) + add_in;

                        if( number < 0)
                            number = 0;

                        d_poptions[option].option = (uchar) number;
                        break;


                    case O_PB:
                        /* get status, page breaks always on unless
                         * 0* or off* after leading spaces
                        */
                        {
                        uchar *ptr = array;

                        while(*ptr++ == SPACE)
                            ;

                        if( !*--ptr  || 
                            ((toupper(*(ptr+1)) == 'O') &&
                             (toupper(*(ptr+2)) == 'F') &&
                             (toupper(*(ptr+3)) == 'F') ))
                                d_poptions[O_PL].option = 0;
                            }

                        break;


                    case O_PE:
                        /* set page break */
                        *type = SL_PAGE;
                        *pageoffset = (uchar) atoi(array);

                        /* falls through */

                /*  case O_CE:  */
                /*  case O_LJ:  */
                /*  case O_RJ:  */
                    default:
                        /* set justify state and get line */
                        *justify = view_opts[a_index].v_default;
                        lecpos = strlen(strcpy(linbuf, array));
                        view_opts[a_index].v_done = FALSE;      /* can do many times */
                        c = CR;
                        break;
                    }
                }

            update_windvars_from_dialog(D_POPTIONS);

            return(c);
            }
        }


    /* unrecognised stored command - just put into linbuf */

    update_windvars_from_dialog(D_POPTIONS);

    *linbuf   = (uchar) ch1;
    linbuf[1] = (uchar) ch2;
    lecpos = strlen(strcpy(linbuf + 2, array)) + 2;

    return(CR);
}


/********************************************************
*                                                       *
* fudge top and bottom margins if no header or footer   *
* don't leave line spacing set to 0                     *
*                                                       *
********************************************************/

extern void
view_load_postinit(void)
{
    tracef0("[view_load_postinit()]\n");

    if(str_isblank(d_poptions_HE))
        d_poptions_TM++;

    if(str_isblank(d_poptions_FO))
        d_poptions_FM++;

    if( d_poptions_LS == 0)
        d_poptions_LS = 1;
}


/********************************************************
*                                                       *
*   initialise stored command table and d_poptions      *
*                                                       *
********************************************************/

extern void
view_load_preinit(void)
{
    intl count, offset, def;

    tracef0("[view_load_preinit()]\n");

    update_dialog_from_windvars(D_POPTIONS);

    for(count = 0; count < NO_OF_PAGE_OPTS; count++)
        {
        offset = view_opts[count].p_OFFset;
        def    = view_opts[count].v_default;

        view_opts[count].v_done = FALSE;

        if(def == 255)
            (void) str_set(&d_poptions[offset].textfield, NULL);
        else
            d_poptions[offset].option = (uchar) def;
        }

    update_windvars_from_dialog(D_POPTIONS);

    pending_space = FALSE;
}


/********************************************
*                                           *
* save VIEW ruler and general file options  *
*                                           *
********************************************/

extern BOOL
view_save_ruler_options(coord *rightmargin, FILE *output)
{
    coord rmargin, count;
    colt tcol;
    BOOL res = TRUE;

    /* Output VIEW ruler.  Just output tab stops at each column position  
     * and right margin at column A right margin position
    */
    if(!away_byte(VIEW_RULER, output)  ||  !away_string("..", output))
        return(FALSE);

    /* find first column on_screen and get its right margin */
    for(tcol=0; colwidth(tcol) == 0; tcol++)
        ;

    if((rmargin = colwrapwidth(tcol)) == 0)
        rmargin = colwidth(tcol);

    /* nothing silly, thank you */
    if( rmargin > 130)
        rmargin = 130;

    *rightmargin = rmargin;

    /* output dots */
    for(count = tcol = 0; (tcol < numcol)  &&  (count < 130); tcol++)
        {
        coord thiswidth = colwidth(tcol);
        coord scount = 0;

        for( ; scount < thiswidth-1 && count < 130; scount++)
            if(!away_byte((uchar) ((count++ == rmargin-2) ? '<' : '.'), output))
                return(FALSE);

        /* output tabstop unless it is last position */
        if(tcol < numcol-1 && 
                !away_byte((uchar) ((count++ == rmargin-2) ? '<' : '*'), output))
            return(FALSE);
        }

    /* maybe still output right margin */
    if(count <= rmargin-2) 
        {
        for( ; count < rmargin-2; count++)
            if(!away_byte('.', output))
                return(FALSE);

        if(!away_byte('<', output))
            return(FALSE);
        }

    if(!away_eol(output))
        return(FALSE);


    /* output page information */

    update_dialog_from_windvars(D_POPTIONS);

    for(count = 0; res  &&  (count < NO_OF_PAGE_OPTS); count++)
        {
        intl offset;
        uchar array[20];
        BOOL out_stored = FALSE;
        BOOL noheader;
        BOOL nofooter;
        intl margin = -1;

        /* set up 2 letter stored command */
        *array   = view_opts[count].v_sc1;
        array[1] = view_opts[count].v_sc2;

        offset = view_opts[count].p_OFFset;

        switch(offset)
            {
            case O_HE:
                /* if there isn't a header we need to subtract one from
                    either the top margin or header margin
                */
                noheader = str_isblank(d_poptions[O_HE].textfield);

                if(!noheader)
                    {
                    strcpy(array+2, d_poptions[O_HE].textfield);
                    out_stored = TRUE;
                    }

                break;


            case O_FO:
                /* footers like headers */
                nofooter = str_isblank(d_poptions[O_FO].textfield);

                if(!nofooter)
                    {
                    strcpy(array+2, d_poptions[O_FO].textfield);
                    out_stored = TRUE;
                    }

                break;


            case O_TM:
            case O_HM:
                margin = d_poptions[offset].option;

                if(noheader  &&  (margin > 0))
                    {
                    /* no header so subtract one from margin */
                    noheader = FALSE;
                    margin--;
                    }

                /* don't bother if same as VIEW default */
                if(view_opts[count].v_default == (uchar) margin)
                    margin = -1;

                break;


            case O_FM:
            case O_BM:
                margin = d_poptions[offset].option;

                if(nofooter && margin > 0)
                    {
                    nofooter = FALSE;
                    margin--;
                    }

                /* don't bother if same as VIEW default */
                if(view_opts[count].v_default == (uchar) margin)
                    margin = -1;

                break;


            case O_PL:
            case O_LM:
                /* PD and VIEW the same */
                if(view_opts[count].v_default != d_poptions[offset].option)
                    margin = d_poptions[offset].option;

                break;


            case O_LS:
                /* PD line spacing one greater than VIEW line spacing 
                 * Don't bother if LS=0 in PD
                */
                if(d_poptions[O_LS].option > 1)
                    margin = d_poptions[O_LS].option - 1;

                break;


            case O_PS:
                /* start page - output SR P n */
                if(!str_isblank(d_poptions[O_PS].textfield))
                    {
                    sprintf(array+2, "P %s", d_poptions[O_PS].textfield);
                    out_stored = TRUE;
                    }

                break;


            default:
                break;
            }

        /* if parameter has been set, build stored command */
        if(margin >= 0)
            {
            sprintf(array+2, "%d", margin);
            out_stored = TRUE;
            }

        if(out_stored)
            if(!view_save_stored_command(array, output)  ||  !away_eol(output))
                res = FALSE;
        }

    update_windvars_from_dialog(D_POPTIONS);

    return(res);
}


/****************************************************************************
*                                                                           *
* save slot in VIEW format                                                  *
* may only output ~132 chars on line, so at 130 print CR and set warning    *
*                                                                           *
****************************************************************************/

extern BOOL
view_save_slot(slotp tslot, colt tcol, rowt trow, FILE *output,
               coord *v_chars_sofar, intl *splitlines, coord rightmargin)
{
    uchar *lptr;
    uchar justify = tslot->justify & J_BITS;

    if((justify == J_LEFTRIGHT)  ||  (justify == J_RIGHTLEFT))
        {
        /* set null terminator */
        memset(linbuf, '\0', LIN_BUFSIZ-1);
        /* expand justified line into linbuf, not forgetting terminator */
        justifyline(tslot->content.text, rightmargin - *v_chars_sofar, justify, linbuf);
        }
    else
        /* write just text or formula part of slot out */
        plain_slot(tslot, tcol, trow, VIEW_CHAR, linbuf);

    /* output contents, dealing with highlight chars */
    for(lptr = linbuf; *lptr; lptr++)
        {
        uchar ch = *lptr;

        if(*v_chars_sofar >= 130)
            {
            *splitlines += 1;
            *v_chars_sofar = 0;
            if(!away_eol(output))
                return(FALSE);
            }

        switch(ch)
            {
            case '@':
                ch = *++lptr;

                switch(toupper(ch))
                    {
                    case 'P':
                    case 'D':
                        if(!away_byte('|', output)  ||  !away_byte(ch, output))
                            return(FALSE);

                        while(*(lptr+1) == '@')
                            lptr++;
                        break;


                    case '@':
                        lptr++;

                    default:
                        lptr--;
                        if(!away_byte('@', output))
                            return(FALSE);
                        break;
                    }

                break;


            /* soft spaces inserted by justify line are 0x1A which is H3 */
            case TEMP_SOFT_SPACE:
                if(!away_byte(VIEW_SOFT_SPACE, output))
                    return(FALSE);

                break;


            default:
                if(ishighlight(ch))
                    {
                    if(!away_string(highlight_table[ch - FIRST_HIGHLIGHT], output))
                        return(FALSE);
                    }
                elif(!away_byte(ch, output))
                    return(FALSE);

                break;
            }
        }

    return(TRUE);
}


/****************************************
*                                       *
*  output stored command to VIEW file   *
*                                       *
****************************************/

extern BOOL
view_save_stored_command(const char *command, FILE *output)
{
    return(away_byte((uchar) VIEW_STORED_COMMAND, output)  &&  away_string(command, output));
}

#endif  /* VIEW_IO */

/* end of viewio.c */
