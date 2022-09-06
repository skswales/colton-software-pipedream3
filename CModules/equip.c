/* equip.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************
*                                         *
* routines to check what equipment exists *
* MRJC February 1988                      *
*                                         *
******************************************/

/* standard includes */
#include "flags.h"

/* external header */
#include "fastsc.ext"

/* local header file */
#include "fastsc.h"

/*
function declarations
*/

extern int check_ega(void);
extern void getcur(int *, int *, int);
extern void getstate(void);
extern void getstates(int);
extern int memchk(unsigned int, unsigned int);
extern void putcur(int, int, int);
extern void putstate(void);
extern void setctype(int, int);
extern void setpage(int);
extern void setvmode(int, int, int);

/*
variable definitions
*/

/* video limit table */
int Maxrow[] =
{
    /* CGA modes */
    25, 25, 25, 25, 25, 25, 25, 25,
    /* MDA mode */
    25,
    /* PCjr modes */
    25, 25, 25,
    /* not used */
    -1, -1,
    /* EGA modes */
    25, 25, 25, 25
};

/* table of special modes */
int Smaxrow[] =
{
    /* EGA */
    43,
    /* MCGA */
    50,
    /* VGA */
    50
};

/* video page table */
int Maxpag[] =
{
    /* CGA modes */
    8, 8, 4, 4, 1, 1, 1,
    /* MDA mode */
    1,
    /* PCjr modes */
    0, 0, 0,
    /* not used */
    -1, -1,
    /* EGA modes */
    8, 4, 1, 1
};

struct Vstate Vcs;
static struct Vstate Vos;

/**********************************************
*                                             *
* function to return information about an EGA *
*                                             *
**********************************************/

#define EGA_INFO 0x10
#define NMODES 2
#define NMEMSIZ 4

int
check_ega()
{
    int mode, memsize;
    union REGS inregs, outregs;

    /* request EGA information */
    inregs.h.ah = ALT_FUNCTION;
    inregs.h.bl = EGA_INFO;
    int86(VIDEO_IO, &inregs, &outregs);

    memsize = outregs.h.bl;
    mode = outregs.h.bh;

    /* return nonzero if EGA installed */
    if((memsize >= 0) && (memsize < NMEMSIZ) &&
       (mode >= 0) && (mode < NMODES))
            return(1);
    return(0);
}

/*********************************
*                                *
* read cursor position from BIOS *
*                                *
*********************************/

void
getcur(x, y, page)
int *x, *y, page;
{
    union REGS inregs, outregs;

    inregs.h.ah = GET_CUR;
    inregs.h.bh = (uchar) page & 0xFF;
    int86(VIDEO_IO, &inregs, &outregs);

    *x = (int) outregs.h.dl;
    *y = (int) outregs.h.dh;
}

/******************************************************
*                                                     *
* get state of video adapter, forcing no special mode *
*                                                     *
******************************************************/

void
getstate()
{
    getstates(0);

    /* save current video state */
    Vos = Vcs;
}

/*****************************
*                            *
* get state of video adapter *
*                            *
*****************************/

void
getstates(special)
int special;
{
    int egaflag = 0;
    union REGS inregs, outregs;

    /* flag no card */
    Vcs.Vcard = 0;

    /* check for PS/2 */
    inregs.h.ah = 0x1A;
    inregs.h.al = 0;
    int86(VIDEO_IO, &inregs, &outregs);

    if(outregs.h.al == 0x1A)
        /* we have VGA/PS/2 */
        {
        switch(outregs.h.bl)
            {
            case 1: /* MDA */
                Vcs.Vcard = CARD_MDA;
                break;
            case 2: /* CGA */
                Vcs.Vcard = CARD_CGA;
                break;
            case 4: /* EGA */
            case 5: /* EGA */
                Vcs.Vcard = CARD_EGA;
                break;
            case 7: /* VGA */
            case 8:
                Vcs.Vcard = CARD_VGA;
                break;
            case 0xA: /* MCGA */
            case 0xB:
            case 0xC:
                Vcs.Vcard = CARD_MCGA;
                break;
            default:
                Vcs.Vcard = CARD_MDA;
                break;
            }
        }
    else
        {
        /* find type of display adapter */
        if(check_ega())
            {
            Vcs.Vcard = CARD_EGA;
            ++egaflag;
            }
        else
            {
            if(memchk(MDA_SEG, 0))
                Vcs.Vcard = CARD_MDA;
            else
                Vcs.Vcard = CARD_CGA;
            }
        }

    inregs.h.ah = GET_STATE;
    int86(VIDEO_IO, &inregs, &outregs);

    /* save video values */
    Vcs.Vmode = outregs.h.al & 0x7F;
    Vcs.Vwidth = outregs.h.ah;
    Vcs.Vpage = outregs.h.bh;
    Vcs.Vspecial = special;

    /* double check for MDA mode */
    if((Vcs.Vcard == CARD_MDA) && (Vcs.Vmode != 7))
        Vcs.Vcard = egaflag ? CARD_EGA : CARD_CGA;

    /* read cursor position */
    getcur(&Vcs.Vcurx, &Vcs.Vcury, Vcs.Vpage);

    /* read attribute */
    inregs.h.ah = READ_CHAR_ATTR;
    inregs.h.bh = (uchar) Vcs.Vpage;
    int86(VIDEO_IO, &inregs, &outregs);
    Vcs.Vattr = (int) outregs.h.ah;

    /* check for special modes */
    if(!Vcs.Vspecial)
        Vcs.Vheight = Maxrow[Vcs.Vmode];
    else
        Vcs.Vheight = Smaxrow[Vcs.Vcard - CARD_EGA];

    /* set maximum height variable */
    if(Vcs.Vcard < CARD_EGA)
        Vcs.Vheightmax = Maxrow[Vcs.Vmode];
    else
        Vcs.Vheightmax = Smaxrow[Vcs.Vcard - CARD_EGA];

    Vcs.Vmaxpag = Maxpag[Vcs.Vmode];

    /* default no Vsync */
    Vcs.Vsync = 0;

    /* read cursor state */
    inregs.h.bh = (uchar) Vcs.Vpage;
    inregs.h.ah = GET_CUR;
    int86(VIDEO_IO, &inregs, &outregs);
    Vcs.Vcurstart = outregs.h.ch & 0xF;
    Vcs.Vcurend = outregs.h.cl & 0xF;
}

/***************************************
*                                      *
* look for RAM at a specified location *
*                                      *
* --out--                              *
* returns nonzero if memory found      *
*                                      *
***************************************/

int
memchk(seg, os)
unsigned int seg;
unsigned int os;
{
    uchar tstval, oldval, newval;
    unsigned int ds;
    struct SREGS segregs;

    /* get value of current data segment */
    segread(&segregs);
    ds = segregs.ds;

    /* save contents of test location */
    movedata(seg, os, ds, (unsigned) &oldval, 1);

    /* copy a known value into test location */
    tstval = 0xFC;
    movedata(ds, (unsigned) &tstval, seg, os, 1);

    /* read test value back and compare to value written */
    movedata(seg, os, ds, (unsigned) &newval, 1);
    if(newval != tstval)
        return(0);

    /* restore original contents of test location */
    movedata(ds, (unsigned) &oldval, seg, os, 1);

    return(1);
}

/***************************
*                          *
* set BIOS cursor position *
*                          *
***************************/

void
putcur(x, y, page)
int x, y, page;
{
    union REGS inregs, outregs;

    /* position output cursor */
    inregs.h.ah = CUR_POS;
    inregs.h.bh = (uchar) page & 0x7;
    inregs.h.dh = (uchar) y & 0xFF;
    inregs.h.dl = (uchar) x & 0xFF;
    int86(VIDEO_IO, &inregs, &outregs);
}

/*****************************************
*                                        *
* return video settings to stored values *
*                                        *
*****************************************/

#define INFO 0x487;     /* video BIOS info byte */

void
putstate()
{
    union REGS inregs, outregs;
    uchar far *info = (uchar far *) INFO;

    /* reprogram character size if was special */
    if(Vcs.Vspecial)
        {
        /* load video BIOS 8x14 characters into char gen */
        inregs.h.ah = 0x11;
        inregs.h.al = 0x11;
        inregs.h.bl = 0;
        int86(VIDEO_IO, &inregs, &outregs);

        /* select 350 lines display on VGA/MCGA */
        if((Vcs.Vcard == CARD_VGA) || (Vcs.Vcard == CARD_MCGA))
            {
            inregs.h.ah = 0x12;
            inregs.h.al = 0x1;
            inregs.h.bl = 0x30;
            int86(VIDEO_IO, &inregs, &outregs);
            }
        }

    /* switch mode */
    if((Vos.Vmode != Vcs.Vmode) || Vcs.Vspecial)
        {
        inregs.h.ah = SET_MODE;
        inregs.h.al = (uchar) Vos.Vmode | 0x80;
        int86(VIDEO_IO, &inregs, &outregs);
        }

    /* mask off the no-clear bit */
    *info = *info & 0x7F;

    /* if we changed page, reselect old display page */
    if(Vos.Vpage != Vcs.Vpage)
        {
        setpage(Vos.Vpage);
        putcur(Vos.Vcurx, Vos.Vcury, Vos.Vpage);
        }
    else
        /* clear screen */
        {
        inregs.h.ah = SCROLL_UP;
        inregs.h.al = 0;
        inregs.h.bh = (uchar) Vos.Vattr;
        inregs.h.bl = 0;
        inregs.x.cx = 0;
        inregs.h.dh = (uchar) Vos.Vheight - 1;
        inregs.h.dl = (uchar) Vos.Vwidth - 1;
        int86(VIDEO_IO, &inregs, &outregs);
        }

    /* reprogram cursor */
    setctype(Vos.Vcurstart, Vos.Vcurend);

    /* copy across old values */
    Vcs = Vos;
}

/*******************
*                  *
* set cursor state *
*                  *
*******************/

void
setctype(start, end)
int start, end;
{
    union REGS inregs, outregs;

    inregs.h.ah = CUR_TYPE;
    inregs.h.ch = (uchar) start & 0xF;
    inregs.h.cl = (uchar) end & 0xF;
    int86(VIDEO_IO, &inregs, &outregs);
}

/*******************
*                  *
* set display page *
*                  *
*******************/

void
setpage(page)
int page;
{
    union REGS inregs, outregs;

    /* switch display page */
    inregs.h.ah = SET_PAGE;
    inregs.h.al = (uchar) page;
    int86(VIDEO_IO, &inregs, &outregs);
}

/*****************
*                *
* set video mode *
*                *
*****************/

void
setvmode(mode, npage, special)
int mode, npage, special;
{
    union REGS inregs, outregs;

    /* special allowed only in mode 3 on EGA, MCGA, VGA */
    if((mode != 3) || (Vcs.Vcard < CARD_EGA))
        special = 0;

    if((mode != Vcs.Vmode) || (special != Vcs.Vspecial))
        {
        /* create special modes */
        if(special)
            {
            /* special handling for different cards */
            switch(Vcs.Vcard)
                {
                /* 50 line mode on VGA */
                case CARD_VGA:
                    /* select 400 line vertical resolution */
                    inregs.h.ah = 0x12;
                    inregs.h.al = 0x2;
                    inregs.h.bl = 0x30;
                    int86(VIDEO_IO, &inregs, &outregs);

                /* note fall thru */

                /* 43 line mode for EGA,
                   50 line mode for MCGA */
                case CARD_EGA:
                case CARD_MCGA:
                    /* select mode 3 */
                    inregs.h.ah = SET_MODE;
                    inregs.h.al = 3 | 0x80;
                    int86(VIDEO_IO, &inregs, &outregs);

                    /* load video BIOS 8x8 characters into char gen */
                    inregs.h.ah = 0x11;
                    inregs.h.al = 0x12;
                    inregs.h.bl = 0;
                    int86(VIDEO_IO, &inregs, &outregs);
                    break;
                }

            /* set cursor position in character matrix */
            setctype(Vos.Vcurstart, Vos.Vcurend);

            /* use alternate video BIOS print routine */
            inregs.h.ah = 0x12;
            inregs.h.bl = 0x20;
            int86(VIDEO_IO, &inregs, &outregs);
            }
        else
            {
            inregs.h.ah = SET_MODE;
            inregs.h.al = (uchar) mode | 0x80;
            int86(VIDEO_IO, &inregs, &outregs);
            }

        /* update new values */
        getstates(special);
        }

    /* select new page if possible to keep user information */
    if(npage && (Vcs.Vmaxpag > 1))
        {
        if(Vos.Vpage)
            Vcs.Vpage = 0;
        else
            Vcs.Vpage = 1;
        setpage(Vcs.Vpage);
        }
}

/* end of equip.c */
