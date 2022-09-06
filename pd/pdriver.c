/* pdriver.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       pdriver.c - Printer driver system for PipeDream
 * Author:      MRJC, RJM
*/

#include "datafmt.h"


/* exported functions */

extern void load_driver(void);


/* ----------------------------------------------------------------------- */

#if !defined(PRINT_OFF)

/********************************************
*                                           *
* read to specified character on this line  *
*                                           *
********************************************/

static BOOL
read_to_char(uchar find, FILE *input)
{
    int ch;

    while(((ch = myfgetc(input)) != (int) find)  &&  (ch != CR)  &&  (ch != LF))
        if(ch == EOF)
            return(FALSE);

    return((ch != CR)  &&  (ch != LF));
}


/****************************************************
*                                                   *
* encode the highlight field in a printer driver    *
* convert numbers, and funnies to ASCIIs            *
* a field containing a ? is special,                *
*   it means for each character do this string      *
*   ? is coded ESC ?                                *
* note can contain 0                                *
* so ESC is coded as ESC ESC and 0 as ESC 255       *
*                                                   *
****************************************************/

static BOOL
encode_highlight_field(uchar *buffer, uchar mask)
{
    uchar array[256];
    uchar *from = buffer;
    uchar *to = array;

    while(*from != '\0')
        {
        int ch;

        switch(toupper(*from))
            {
            case 'E':
                if((toupper(from[1]) == 'S')  &&  (toupper(from[2]) == 'C'))
                    {
                    from += 3;
                    ch = (int) ESCAPE;
                    }
                else
                    return(FALSE);
                break;

            /* a query by itself is a special field, set the relevant bit */

            case QUERY:
                *to++ = ESCAPE;
                ch = QUERY;
                if(from[1] == '\0')
                    from[2] = '\0';

                from += 2;
                h_query |= mask;
                break;

            case QUOTES:
                ch = (int) from[1];
                if(from[2] != QUOTES)
                    return(FALSE);

                from += 3;
                break;

            case '$':
            case '&':
                from++;
                if(!isxdigit(*from))
                    return(FALSE);

                sscanf((char *) from, "%x", &ch);
                for( ; isxdigit(*from); from++)
                    ;
                break;

            case SPACE:
            case COMMA:
                from++;
                continue;

            default:
                if((*from == '-')  ||  isdigit(*from))
                    {
                    if((ch = atoi((char *) from)) < 0)
                        ch += 256;
                    if(*from == '-')
                        from++;
                    for( ; isdigit(*from); from++)
                        ;
                    break;
                    }

                return(FALSE);
            }

        if(ch == 0)
            {
            *to   = ESCAPE;
            to[1] = 0xFF;
            to += 2;
            }
        elif((uchar) ch == ESCAPE)
            {
            *to = to[1] = ESCAPE;
            to += 2;
            }
        else
            *to++ = (uchar) ch;
        }

    *to = '\0';
    strcpy((char *) buffer, (char *) array);
    return(TRUE);
}

#endif  /* PRINT_OFF */


/****************************************************
*                                                   *
* load printer driver, name = d_driver[0].textfield *
*                                                   *
****************************************************/

extern void
load_driver(void)
{
#if !defined(PRINT_OFF)
    char *name;
    FILE *input;
    int newch;
    BOOL firstfield = TRUE;
    char array[MAX_FILENAME];
    uchar linarray[LIN_BUFSIZ];
    char buffer[256];
    intl error = 0;
    uchar res;

    off_at_cr = 0xFF;           /* default all highlights get switched off */
    h_query = h_waiting = 0;    /* no query fields, no highlights waiting */

    /* kill old driver */

    delete_list(&highlight_list);
    micbit = driver_loaded = FALSE;
    send_linefeeds = DEFAULT_LF;    /* drivers output LF with CR by default */
    hmi_as_text = FALSE;            /* drivers output HMI as byte by default */

    name = d_driver[0].textfield;

    if(str_isblank(name))
        return;

    res = add_path(array, name, FALSE)
                ? find_file_type(array)
                : '\0';

    if(res != 'T')
        error = (res == '\0') ? ERR_NOTFOUND : ERR_NOTTABFILE;

    if(error)
        {
        reperr(error, name);
        str_clr(&d_driver[0].textfield);
        return;
        }

    input = myfopen(array, read_str);
    if(!input)
        {
        reperr(ERR_CANNOTOPEN, name);
        return;
        }

    mysetvbuf(input, buffer, sizeof(buffer));


    /* for each field */

    for(;;)
        {
        uchar *ptr = linarray;
        word32 saveparm = 0;
        int delim;
        uchar ch;
        intl res;

        while((newch = getfield(input, linarray, TRUE)) != TAB)
            if(newch == EOF)
                {
                driver_loaded = TRUE;
                goto ENDPOINT;
                }

        firstfield = FALSE;

        if(str_isblank(linarray))
            {
            read_to_char(CR, input);
            continue;
            }

        /* we have the first field in a record */

        switch(toupper(*ptr))
            {
            case 'H':           /* either highlight or HMI */
                ++ptr;
                if(isdigit(*ptr))
                    {
                    int h_no = (int) (*ptr - '1');
                    uchar mask = (uchar) (1 << h_no);

                    delim = getfield(input, linarray, TRUE);

                    if(((delim == TAB)  ||  (delim == CR))  &&  !str_isblank(linarray))
                        {
                        /* highlight on field in linarray */

                        if(!encode_highlight_field(linarray, mask))
                            {
                            linarray[15] = '\0';
                            reperr(ERR_BAD_PARM, (char *) linarray);
                            goto ENDPOINT;
                            }

                        res = add_to_list(&highlight_list, ((word32) h_no) + FIRST_HIGHLIGHT, linarray, &res);

                        if(res <= 0)
                            {
                            reperr_null(res ? res : ERR_NOROOM);
                            goto ENDPOINT;
                            }
                        }
                    else
                        break;

                    /* if we have a query field, don't read the others */
                    if(h_query & mask)
                        {
                        read_to_char(CR, input);
                        break;
                        }

                    delim = getfield(input, linarray, TRUE);

                    if(((delim == TAB)  ||  (delim == CR))  &&  !str_isblank(linarray))
                        {
                        /* use the highlight number + 256 as key for off */

                        if(!encode_highlight_field(linarray, 0))
                            {
                            linarray[15] = '\0';
                            reperr(ERR_BAD_PARM, (char *) linarray);
                            goto ENDPOINT;
                            }

                        res = add_to_list(&highlight_list, ((word32) h_no) + FIRST_HIGHLIGHT + 256, linarray, &res);

                        if(res <= 0)
                            {
                            reperr_null(res ? res : ERR_NOROOM);
                            goto ENDPOINT;
                            }
                        }
                    else
                        break;

                    delim = getfield(input, linarray, TRUE);

                    if((delim == TAB)  ||  (delim == CR))
                        {
                        /* switch off at CR ? */
                        if(toupper(*linarray) == 'N')
                            off_at_cr &= ~mask;     /* clear bit */
                        else
                            off_at_cr |= mask;      /* set bit */
                        }
                    }
                elif((toupper(*ptr) == 'M')  &&  (toupper(ptr[1]) == 'I'))
                    {
                    ch = toupper(ptr[2]);

                    if(ch == 'T')
                        {
                        delim = getfield(input, linarray, TRUE);

                        if((delim == TAB)  ||  (delim == CR))
                            hmi_as_text = (toupper(*linarray) == 'Y');
                        }
                    else
                        saveparm =  (ch == 'P') ?   HMI_P :
                                    (ch == 'S') ?   HMI_S :
                                    (ch == 'O') ?   HMI_O :
                                                    (word32) 0;
                    break;
                    }

                read_to_char(CR, input);
                break;


            case 'P':           /* should be printer on or off */
                ptr++;
                if(toupper(*ptr) != 'O')
                    break;

                ptr++;
                if(toupper(*ptr) == 'N')
                    saveparm = P_ON;
                elif((toupper(*ptr) == 'F')  &&  (toupper(ptr[1]) == 'F'))
                    saveparm = P_OFF;
                break;


            case 'E':           /* should be EP - end page */
                if(toupper(ptr[1]) == 'P')
                    saveparm = E_P;
                break;


            case 'L':           /* should be LF, on or off */
                if(toupper(ptr[1]) == 'F')
                    {
                    delim = getfield(input, linarray, TRUE);

                    if((delim == TAB)  ||  (delim == CR))
                        {
                        ch = toupper(*linarray);

                        send_linefeeds = (ch == 'N') ?  FALSE :
                                         (ch == 'Y') ?  TRUE  :
                                                        DEFAULT_LF;
                        }
                    }
                break;


            case QUOTES:    /* character translation */
                if(ptr[2] != QUOTES)
                    break;

                saveparm = (word32) ptr[1];
                break;


            default:    /* unrecognised, don't fault cos of future expansion */
                break;
            }

        if( saveparm
            &&  (((delim = getfield(input, linarray, TRUE)) == TAB)  ||  (delim == CR))
            &&  !str_isblank(linarray))
            {
            if(!encode_highlight_field(linarray, 0))
                {
                linarray[15] = '\0';
                reperr(ERR_BAD_PARM, (char *) linarray);
                goto ENDPOINT;
                }

            res = add_to_list(&highlight_list, saveparm, linarray, &res);

            if(res <= 0)
                {
                reperr_null(res ? res : ERR_NOROOM);
                goto ENDPOINT;
                }
            }
        }

ENDPOINT:
    myfclose(input);

#endif  /* PRINT_OFF */
}

/* end of pdriver.c */
