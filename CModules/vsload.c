/* vsload.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*************************************
*                                    *
* code to interpret a ViewSheet file *
* MRJC                               *
* May 1989                           *
*                                    *
*************************************/

/* standard header files */
#include "flags.h"

#ifndef VIEWSHEET_OFF

/* external header */
#if ARTHUR || RISCOS
#include "ext.handlist"
#include "ext.vsload"
#else
#include "vsload.ext"
#include "handlist.ext"
#endif

/* local header file */
#include "vsload.h"

/*
function declarations
*/

extern intl isvsfile(FILE *fin);
extern intl loadvsfile(FILE *fin);
/*static intl my_strnicmp(uchar *string1, uchar *string2);*/
/*static uword16 readuword16(uchar *arg);*/
/*static word16 readword16(uchar *arg);*/
/*static char *vsdecodeslot(uchar *slot);*/
extern void vsfileend(void);
extern char *vstravel(intl col, intl row,
                      intl *type, intl *decp, intl *justright, intl *minus);
static intl xtos(uchar *string, intl x);

/*
static data
*/

static mhandle vsh = 0;
static char *outbuf = NULL;

struct vsfunc
    {
    char *name;
    uchar flags;
    };

#define BRACKET 0
#define NO_SUM 1

static struct vsfunc vsfuncs[] =
    {
    "abs(",     BRACKET,
    "acs(",     BRACKET,
    "asn(",     BRACKET,
    "atn(",     BRACKET,
    "average(",  NO_SUM,
    "choose(",  BRACKET,
    "col",      BRACKET,
    "cos(",     BRACKET,
    "deg(",     BRACKET,
    "exp(",     BRACKET,
    "if(",      BRACKET,
    "int(",     BRACKET,
    "ln(",      BRACKET,
    "log(",     BRACKET,
    "lookup(",   NO_SUM,
    "max(",      NO_SUM,
    "min(",      NO_SUM,
    "pi",       BRACKET,
    "rad(",     BRACKET,
    "read(",    BRACKET,
    "row",      BRACKET,
    "sgn(",     BRACKET,
    "sin(",     BRACKET,
    "sqr(",     BRACKET,
    "tan(",     BRACKET,
    "write(",   BRACKET,
    };

/************************************************
*                                               *
* say whether a file is a ViewSheet file or not *
*                                               *
************************************************/

extern intl
isvsfile(FILE *fin)
{
    struct vsfileheader vsfh;

    /* read in ViewSheet file header */
    if(fseek(fin, 0l, SEEK_SET) != 0)
        return(VSLOAD_ERR_CANTREAD);

    if(fread(&vsfh, 1, sizeof(struct vsfileheader), fin) <
                                    sizeof(struct vsfileheader))
        {
        if(ferror(fin))
            return(VSLOAD_ERR_CANTREAD);
        return(FALSE);
        }

    tracef3("[isvsfile curwin: %d, fileid: %d, scmode: %d]\n",
            vsfh.curwin, vsfh.fileid, vsfh.scmode);

    if(vsfh.curwin <= 108 &&
       vsfh.fileid == 0xDD &&
       (vsfh.scmode & 0x7F) <= 7)
        return(TRUE);

    return(FALSE);
}

/*******************************************************
*                                                      *
* load the ViewSheet file into memory ready for action *
*                                                      *
*******************************************************/

extern intl
loadvsfile(FILE *fin)
{
    word32 vsfsize;
    uchar *vsp;

    /* calculate file size */
    if(fseek(fin, 0l, SEEK_END) != 0)
        return(VSLOAD_ERR_CANTREAD);

    vsfsize = ftell(fin);

    /* allocate memory to receive it */
    if((vsh = list_allochandle(vsfsize)) == 0)
        return(VSLOAD_ERR_NOMEMORY);

    vsp = list_getptr(vsh);

    /* read in the file */
    if(fseek(fin, 0l, SEEK_SET) != 0)
        return(VSLOAD_ERR_CANTREAD);

    if(fread(vsp, 1, (intl) vsfsize, fin) < (intl) vsfsize)
        {
        vsfileend();
        return(VSLOAD_ERR_CANTREAD);
        }

    if((outbuf = list_allocptr((word32) VS_MAXSLOTLEN + 1)) == NULL)
        {
        vsfileend();
        return(VSLOAD_ERR_NOMEMORY);
        }

    return(((struct vsfileheader *) vsp)->maxrow);
}

/************************************
*                                   *
* read an unsigned word from memory *
*                                   *
************************************/

static uword16
readuword16(uchar * arg)
{
#if MS

    return(*((uword16 *) arg));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    intl i;
    union
        {
        uword16 uword;
        uchar uwbytes[2];
        } uw;

    for(i = 0; i < 2; ++i)
        uw.uwbytes[i] = *arg++;

    return(uw.uword);

#endif
}

/*********************************
*                                *
* read a signed word from memory *
*                                *
*********************************/

static word16
readword16(uchar *arg)
{
#if MS

    return(*((word16 *) arg));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    intl i;
    union
        {
        word16 word;
        uchar wbytes[2];
        } w;

    for(i = 0; i < 2; ++i)
        w.wbytes[i] = *arg++;

    return(w.word);

#endif
}

/***********************************
*                                  *
* strnicmp for ANSI only compilers *
*                                  *
***********************************/

static intl
my_strnicmp(uchar *string1, uchar *string2)
{
    #if MS
    return(strnicmp(string1, string2, strlen(string1)));
    #else

    intl i, count;
    uchar a, b;

    for(i = 0, count = strlen((uchar *) string1);
        i < count;
        ++i, ++string1, ++string2)
        {
        a = toupper(*string1);
        b = toupper(*string2);

        if(a != b)
            return((a > b) ? 1 : -1);
        }

    return(0);

    #endif
}

/******************************************
*                                         *
* decode a ViewSheet slot into plain text *
*                                         *
******************************************/

#define SLR_START 1

static char *
vsdecodeslot(uchar *slot)
{
    char *op;
    uchar *ip;
    char rangebuf[25];
    uchar bracstac[255];
    intl slrcount, rangeix, bracix, i;

    slrcount = rangeix = 0;
    op = outbuf;
    ip = slot;

    bracstac[0] = BRACKET;
    bracix = 1;

    while(TRUE)
        {
        if(isalpha(*ip))
            {
            for(i = 0; i < sizeof(vsfuncs) / sizeof(struct vsfunc); ++i)
                if(!my_strnicmp(vsfuncs[i].name, ip))
                    break;

            tracef1("[vsfuncs[i].name: %s]\r\n", vsfuncs[i].name);
            tracef1("[ip: %s]\r\n", ip);
            tracef1("[i: %d]\r\n", i);

            if(i < sizeof(vsfuncs) / sizeof(struct vsfunc))
                {
                if(!my_strnicmp("average(", ip))
                    {
                    strcpy(op, "avg(");
                    op += 4;
                    }
                else
                    {
                    strcpy(op, vsfuncs[i].name);
                    op += strlen(vsfuncs[i].name);
                    tracef0("[copied]\r\n");
                    }

                ip += strlen(vsfuncs[i].name);

                bracstac[bracix++] = vsfuncs[i].flags;
                continue;
                }
            }
        else if(*ip == '(')
            {
            bracstac[bracix] = bracstac[bracix - 1];
            bracix++;
            }
        else if(*ip == ')')
            {
            if(bracix)
                --bracix;
            }
        else if(*ip == SLR_START)
            {
            intl col, row;

            col = (intl) *(++ip);
            row = (intl) *(++ip);
            rangeix += xtos(rangebuf + rangeix, col);
            rangeix += sprintf(rangebuf + rangeix, "%d", row + 1);
            rangebuf[rangeix] = '\0';
            if(++slrcount == 2)
                {
                if(bracix && bracstac[bracix - 1] != NO_SUM)
                    {
                    strcpy(op, "sum(");
                    op += 4;
                    strcpy(op, rangebuf);
                    op += rangeix;
                    strcpy(op, ")");
                    op += 1;
                    slrcount = rangeix = 0;
                    }
                }

            ++ip;
            continue;
            }

        if(slrcount)
            {
            strcpy(op, rangebuf);
            op += rangeix;
            slrcount = rangeix = 0;
            }

        if(!(*op++ = *ip++))
            break;
        }

    return(outbuf);
}

/*****************************************************
*                                                    *
* free any resources used by the ViewSheet converter *
*                                                    *
*****************************************************/

extern void
vsfileend(void)
{
    if(vsh)
        {
        list_deallochandle(vsh);
        vsh = 0;
        }

    if(outbuf)
        {
        list_deallocptr(outbuf);
        outbuf = NULL;
        }
}

/***********************************
*                                  *
* return a pointer to textual slot *
* contents of a ViewSheet slot     *
*                                  *
***********************************/

extern char *
vstravel(intl col, intl row, intl *type,
         intl *decp, intl *justright, intl *minus)
{
    uchar *vsp, *vsdp, *rtbp, *slotcont;
    struct vsfileheader *vsfp;
    struct rowtabentry *rixp;
    uword16 coloff;

    if(!vsh)
        return(NULL);

    vsp = list_getptr(vsh);
    vsfp = (struct vsfileheader *) vsp;

    if(row >= (intl) vsfp->maxrow)
        return(NULL);

    /* calculate pointer to data area */
    vsdp = vsp + sizeof(struct vsfileheader);

    rtbp = vsdp + readword16(&vsfp->rtbpn1);
    rixp = (struct rowtabentry *) (rtbp + row * 3);
    if(col >= (intl) rixp->colsinrow)
        return(NULL);

    coloff = readuword16(rtbp + readword16(&rixp->offtoco1) + 2 * col);

    /* check for null pointer */
    if(!coloff)
        return(NULL);

    /* work out type from top bit */
    if(coloff & 0x8000)
        *type = VS_TEXT;
    else
        *type = VS_NUMBER;

    coloff &= 0x7FFF;

    /* calculate final slot pointer */
    slotcont = vsdp + readword16(&vsfp->ctbpn1) + coloff;

    if(*type == VS_NUMBER)
        {
        uchar formatb;

        formatb = *(slotcont + 5);
        if((formatb & 0x7F) == 0x7F)
            {
            /* default to FRM */
            *decp = -2;
            *minus = TRUE;
            *justright = TRUE;
            }
        else
            {
            if(formatb & 0x40)
                *decp = formatb & 0xF;
            else
                *decp = -2;

            if(formatb & 0x20)
                *justright = FALSE;
            else
                *justright = TRUE;

            if(formatb & 0x10)
                *minus = FALSE;
            else
                *minus = TRUE;
            }

        return(vsdecodeslot(slotcont + 6));
        }

    /* bash off right-justify label bit */
    if(*slotcont & 0x80)
        *justright = TRUE;
    else
        *justright = FALSE;

    strcpy(outbuf, slotcont);
    *outbuf &= 0x7F;

    return(outbuf);
}

/*****************************
*                            *
* convert column to a string *
*                            *
* --out--                    *
* length of resulting string *
*                            *
*****************************/

static intl
xtos(uchar *string, intl x)
{
    uchar *c = string;
    register intl digit2;
    register intl digit1;

    digit2 = x / 26;
    digit1 = x - digit2 * 26;

    if(digit2)
        *c++ = (uchar) ((digit2 - 1) + (intl) 'A');
    *c++ = (uchar) (digit1 + (intl) 'A');
    return(c - string);
}

#endif

/* end of vsload.c */
