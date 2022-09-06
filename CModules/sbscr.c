/* sbscr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************
*                                         *
* routines to update screen memory in the *
* fast screen update test                 *
*                                         *
* MRJC February 1988                      *
* SKS changed sb_wsa to take const char * *
*                                         *
******************************************/

/* external flags */
#include <mctype.h>

/* standard header */
#include <csoft.h>

/* standard includes */
#include "include.h"

/* external header */
#include "fastsc.ext"

/* local header file */
#include "fastsc.h"

/*
function declarations
*/

void sb_filla(winp, uchar);
void sb_fillc(winp, uchar);
void sb_fillca(winp, uchar, uchar);
void sb_init();
void sb_getpos(int *, int *);
void sb_move(winp, int, int);
winp sb_new(int, int, int, int);
void sb_rest(winp);
void sb_save(winp);
void sb_scrl(winp, int, uchar);
void sb_set_scrl(winp, int, int, int, int);
void sb_show();
void sb_wa(winp, uchar, int);
void sb_wc(winp, uchar, int);
void sb_wca(winp, uchar, uchar, int);
int sb_wsa(winp, const char *, uchar);

/*
static data
*/

struct BUFFER Sbuf;                          /* control information */
union CELL near Scrnbuf[SB_ROWS][SB_COLS];   /* screen buffer array */
union CELL near Savebuf[SB_ROWS][SB_COLS];   /* save buffer */

/******************************************
*                                         *
* set attribute of all cells in a region, *
* leaving attribute undisturbed           *
*                                         *
******************************************/

void
sb_filla(win, attr)
winp win;
uchar attr;
{
    register int i, j;
    cellp c;

    for(i = win->r0; i <= win->r1; ++i)
        {
        for(j = win->c0, c = &Scrnbuf[i][j];
            j <= win->c1;
            ++j, c++)
                c->b.attr = attr;
        Sbuf.rowch[i] = 1;
        }
}

/***********************************************
*                                              *
* set all cells in a region to same character, *
* leaving attribute undisturbed                *
*                                              *
***********************************************/

void
sb_fillc(win, ch)
winp win;
uchar ch;
{
    register int i, j;
    cellp c;

    for(i = win->r0; i <= win->r1; ++i)
        {
        for(j = win->c0, c = &Scrnbuf[i][j];
            j <= win->c1;
            ++j, ++c)
                c->b.ch = ch;
        Sbuf.rowch[i] = 1;
        }
}

/*********************************************
*                                            *
* set all cells in a specified window to the *
* same character/attribute value             *
*                                            *
*********************************************/

void
sb_fillca(win, ch, attr)
winp win;
uchar ch;
uchar attr;
{
    register int i, j;
    unsigned int ca;
    cellp c;

    ca = (attr << 8) | ch;
    for(i = win->r0; i <= win->r1; ++i)
        {
        for(j = win->c0, c = &Scrnbuf[i][j];
            j <= win->c1;
            ++j, ++c)
                c->cap = ca;
        Sbuf.rowch[i] = 1;
        }
}

/**************************************************
*                                                 *
* routine to initialise the fast screen interface *
*                                                 *
**************************************************/

void
sb_init()
{
    register int i;

    /* set initial parameter values */
    Sbuf.row = Sbuf.col = 0;
    for(i = 0; i < Vcs.Vheight; ++i)
        Sbuf.rowch[i] = 0;
}

/***********************************
*                                  *
* read the current cursor position *
*                                  *
***********************************/

void
sb_getpos(xp, yp)
int *xp, *yp;
{
    *xp = Sbuf.col;
    *yp = Sbuf.row;
}

/************************************
*                                   *
* position the screen buffer cursor *
*                                   *
************************************/

void
sb_move(win, col, row)
winp win;
register int col, row;
{
    /* change nothing if request out of range */
    if(row < 0 || row > win->r1 - win->r0 ||
       col < 0 || col > win->c1 - win->c0)
        return;

    win->col = col;
    win->row = row;
    Sbuf.col = col + win->c0;
    Sbuf.row = row + win->r0;
}

/***********************************
*                                  *
* routine to allocate a new window *
*                                  *
* --out--                          *
* pointer to window                *
* NULL if not enough memory        *
*                                  *
***********************************/

winp
sb_new(left, top, width, height)
int left, top, width, height;
{
    winp new;

    /* allocate the data control structure */
    new = (winp) malloc(sizeof(struct REGION));
    if(new != NULL)
        {
        new->r0 = new->sr0 = top;
        new->r1 = new->sr1 = top + height - 1;
        new->c0 = new->sc0 = left;
        new->c1 = new->sc1 = left + width - 1;
        new->row = new->col = 0;
        }

    return(new);
}

/***********************************************
*                                              *
* restore the given region from the save array *
*                                              *
***********************************************/

void
sb_rest(win)
winp win;
{
    register int i, j;
    cellp c1, c2;

    for(i = win->r0; i <= win->r1; ++i)
        {
        for(j = win->c0, c1 = &Scrnbuf[i][j], c2 = &Savebuf[i][j];
            j <= win->c1;
            ++j)
                *c1++ = *c2++;

        Sbuf.rowch[i] = 1;
        }
}

/******************************************
*                                         *
* save the given region in the save array *
*                                         *
******************************************/

void
sb_save(win)
winp win;
{
    register int i, j;
    cellp c1, c2;

    for(i = win->r0; i <= win->r1; ++i)
        {
        for(j = win->c0, c1 = &Scrnbuf[i][j], c2 = &Savebuf[i][j];
            j <= win->c1;
            ++j)
                *c2++ = *c1++;
        }
}

/*************************************
*                                    *
* scroll a window n lines up or down *
*                                    *
*************************************/

void
sb_scrl(win, n, attr)
winp win;
int n;
uchar attr;
{
    register int row, col;
    cellp c1, c2;
    unsigned int ca = (attr << 8) | ' ';

    if(n == 0)
        {
        /* clear to spaces */
        sb_fillca(win, ' ', attr);
        }
    else if(n > 0)
        {
        /* scroll n rows up */
        for(row = win->sr0; row <= win->sr1 - n; ++row)
            {
            for(col = win->sc0, c1 = &Scrnbuf[row][col],
                c2 = &Scrnbuf[row + n][col];
                col <= win->sc1;
                ++col)
                    *c1++ = *c2++;
            Sbuf.rowch[row] = 1;
            }

        for( ; row <= win->sr1; ++row)
            {
            for(col = win->sc0, c1 = &Scrnbuf[row][col];
                col <= win->sc1;
                ++col, ++c1)
                    c1->cap = ca;
            Sbuf.rowch[row] = 1;
            }
        }
    else
        {
        /* scroll n rows down */
        n = -n;
        for(row = win->sr1; row >= win->sr0 + n; --row)
            {
            for(col = win->sc0, c1 = &Scrnbuf[row][col],
                c2 = &Scrnbuf[row - n][col];
                col <= win->sc1;
                ++col)
                    *c1++ = *c2++;
            Sbuf.rowch[row] = 1;
            }

        for( ; row >= win->sr0; --row)
            {
            for(col = win->sc0, c1 = &Scrnbuf[row][col];
                col <= win->sc1;
                ++col, ++c1)
                    c1->cap = ca;
            Sbuf.rowch[row] = 1;
            }
        }
}

/************************************************
*                                               *
* set the scroll region boundaries for a window *
*                                               *
************************************************/

void
sb_set_scrl(win, left, top, right, bottom)
winp win;
int left, top;
int right, bottom;
{
    if(top < 0 || left < 0 ||
           bottom > win->r1 - win->r0 + 1 || right > win->c1 - win->c0 + 1)
           return;
    win->sr0 = win->r0 + top;
    win->sc0 = win->c0 + left;
    win->sr1 = win->r0 + bottom - 1;
    win->sc1 = win->c0 + right - 1;
}

/***********************************************
*                                              *
* copy the screen buffer to the display memory *
*                                              *
***********************************************/

#define NBYTES (2 * Vcs.Vwidth)
#define CRT_LEN 0x44C;

/* macro to synchronise with vertical retrace period */
#define VSTAT 0x3DA
#define VRBIT 8
#define VSYNC   while((inp(VSTAT) & VRBIT) == VRBIT); \
                while((inp(VSTAT) & VRBIT) != VRBIT)

void
sb_show()
{
    struct SREGS segregs;
    register int r, c;
    int n, count;
    unsigned int dest_seg, offset, pages;
    unsigned int far *buflp;

    buflp = (int far *) CRT_LEN;
    pages = *buflp >> 4;

    segread(&segregs);

    /* work out destination segment from page number */
    if(Vcs.Vmode == 7)
        dest_seg = MDA_SEG;
    else
        dest_seg = CGA_SEG + pages * Vcs.Vpage;

    /* determine extent of changes */
    for(r = 0, n = 0; r < Vcs.Vheight; ++r)
        if(Sbuf.rowch[r])
            ++n;

    offset = 0;
    if(n <= 4)
        {
        /* copy only rows that contain changes */
        for(r = 0; n && (r < Vcs.Vheight); ++r)
            {
            if(Sbuf.rowch[r])
                {
                if(Vcs.Vsync)
                    /* copy blocks during vertical retrace */
                    VSYNC;
                movedata(segregs.ds, (unsigned) Scrnbuf + offset, dest_seg,
                                     offset, NBYTES);
                Sbuf.rowch[r] = 0;
                --n;
                }
            offset += NBYTES;
            }
        }
    else
        {
        /* just do block move if no VSYNC */
        if(!Vcs.Vsync)
            {
            movedata(segregs.ds, (unsigned) Scrnbuf, dest_seg, 0,
                     Vcs.Vheight * Vcs.Vwidth * 2);
            }
        else
            {
            /* copy the entire buffer, 3 rows at a time, Vsynced */
            count = 3 * NBYTES;
            for(r = 0; r < Vcs.Vheight - 1; r += 3)
                {
                VSYNC;
                movedata(segregs.ds, (unsigned) Scrnbuf + offset, dest_seg,
                                     offset, count);
                offset += count;
                }
            VSYNC;
            movedata(segregs.ds, (unsigned) Scrnbuf + offset, dest_seg,
                                 offset, NBYTES);

            }

        /* mark all done */
        for(r = 0; r < Vcs.Vheight; ++r)
            Sbuf.rowch[r] = 0;
        }

    putcur(Sbuf.col, Sbuf.row, Vcs.Vpage);
}

/******************************************
*                                         *
* write an attribute to the screen buffer *
*                                         *
******************************************/

void
sb_wa(win, attr, n)
winp win;
uchar attr;
int n;
{
    register int i;
    int row, col;
    cellp c;

    i = n;
    row = win->r0 + win->row;
    col = win->c0 + win->col;

    c = &Scrnbuf[row][col + i];
    while(c--, i--)
        c->b.attr = attr;

    win->col += n;

    /* mark the changed region */
    Sbuf.rowch[row] = 1;
}

/*****************************************
*                                        *
* write a character to the screen buffer *
*                                        *
*****************************************/

void
sb_wc(win, ch, n)
winp win;
uchar ch;
int n;
{
    register int i;
    int row, col;
    cellp c;

    i = n;
    row = win->r0 + win->row;
    col = win->c0 + win->col;

    c = &Scrnbuf[row][col + i];
    while(c--, i--)
        c->b.ch = ch;

    win->col += n;

    /* mark the changed region */
    Sbuf.rowch[row] = 1;
}

/********************************************************
*                                                       *
* write a character/attribute pair to the screen buffer *
*                                                       *
********************************************************/

void
sb_wca(win, ch, attr, n)
winp win;
uchar ch;
uchar attr;
int n;
{
    register int i;
    int row, col;
    cellp c;
    int ca = (attr << 8) | ch;

    i = n;
    row = win->r0 + win->row;
    col = win->c0 + win->col;

    c = &Scrnbuf[row][col + i];
    while(c--, i--)
        c->cap = ca;

    win->col += n;

    /* mark the changed region */
    Sbuf.rowch[row] = 1;
}

/***********************************
*                                  *
* write a string with an attribute *
*                                  *
* --out--                          *
* number of characters written     *
*                                  *
***********************************/

int
sb_wsa(win, string, attr)
winp win;
const char *string;
uchar attr;
{
    register int n = 0;
    int row, col;
    cellp c;

    row = win->r0 + win->row;
    col = win->c0 + win->col;

    c = &Scrnbuf[row][col];
    while(*string)
        {
        c->b.ch = *string++;
        c->b.attr = attr;
        ++c;
        ++n;
        }

    win->col += n;

    /* mark the changed region */
    Sbuf.rowch[row] = 1;

    return(n);
}

/* end of sbscr.c */
