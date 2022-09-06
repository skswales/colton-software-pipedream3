/* fastsc.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/****************************************************
*                                                   *
* local header file for fast screen update routines *
* MRJC February 1988                                *
*                                                   *
****************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <signal.h>
#include <conio.h>
#include <dos.h>
#include <memory.h>

/* BIOS interrupts */

#define VIDEO_IO        0x10
#define EQUIP_CK        0x11
#define VIDEO_INIT      0x1D
#define GRAPHICS        0x1F

/* video routine numbers */

#define SET_MODE        0
#define CUR_TYPE        1
#define CUR_POS         2
#define GET_CUR         3
#define LPEN_POS        4
#define SET_PAGE        5
#define SCROLL_UP       6
#define SCROLL_DN       7
#define READ_CHAR_ATTR  8
#define WRITE_CHAR_ATTR 9
#define WRITE_CHAR      10
#define PALETTE         11
#define WRITE_DOT       12
#define READ_DOT        13
#define WRITE_TTY       14
#define GET_STATE       15
#define ALT_FUNCTION    18

#define MDA_SEG 0xB000
#define CGA_SEG 0xB800

/* screen buffer constants */
#define SB_ROWS 50
#define SB_COLS 80

/*
screen character/attribute buffer element definition
*/

struct BYTEBUF
{
    uchar ch;       /* character */
    uchar attr;     /* attribute */
};

union CELL
{
    struct BYTEBUF b;
    unsigned int cap;     /* character/attribute pair */
};

/* alias for pointer to cell */
typedef union CELL *cellp;

/*
screen buffer control structure
*/

struct BUFFER
{
    /* current position */
    int row, col;

    /* flag for each row to indicate if changed */
    int rowch[SB_ROWS];
};

/*
function declarations
*/

/* equip.c */

extern struct Vstate Vcs;
extern struct Vstate Vos;

extern void getcur(int *, int *, int);
extern void getstate();
extern void putcur(int, int, int);
extern void putstate();
extern void setctype(int, int);
extern void setpage(int);
extern void setvmode(int, int, int);

/* screen.c */

extern void sb_filla(winp, uchar);
extern void sb_fillc(winp, uchar);
extern void sb_fillca(winp, uchar, uchar);
extern void sb_init();
extern void sb_getpos(int *, int *);
extern void sb_move(winp, int, int);
extern winp sb_new(int, int, int, int);
extern void sb_rest(winp);
extern void sb_save(winp);
extern void sb_scrl(winp, int, uchar);
extern void sb_set_scrl(winp, int, int, int, int);
extern void sb_show();
extern void sb_wa(winp, uchar, int);
extern void sb_wc(winp, uchar, int);
extern void sb_wca(winp, uchar, uchar, int);
extern int sb_wsa(winp, const char *, uchar);

/* end of fastsc.h */
