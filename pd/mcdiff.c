/* mcdiff.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       mcdiff.c - machine dependent routines for screen & keyboard
 * Author:      RJM August 1987
*/

/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include <locale.h>
#include "os.h"
#include "bbc.h"
#include "swinumbers.h"
#if !defined(Z88_OFF)
#include "ext.z88com"
#endif

#include "wimp.h"
#include "wimpt.h"
#include "font.h"

#include "kernel.h"

#if ARTHUR
#include "myclib:altkey.h"
#include "ext.asavesc"
#endif

#elif MS
#include <bios.h>
#include <graph.h>
#include <process.h>
#include <errno.h>
#if !defined(Z88_OFF)
#include "z88com.ext"
#endif

#else
    assert(0);
#endif


#include "datafmt.h"


#if RISCOS
#include "ext.riscos"
#include "riscdraw.h"
extern BOOL print_complain(os_error *err);
#endif


/* exported functions */

extern void ack_esc(void);
extern void bleep(void);
extern void clearkeyboardbuffer(void);
extern void clearscreen(void);          /* clear default screen, VDU 4 text */
extern void curon(void);
extern BOOL depressed(intl key);
extern BOOL depressed_ctrl(void);
extern BOOL depressed_shift(void);
extern void down_scroll(coord first_line);
extern void eraeol(void);
extern void init_mc(void);
extern void invert(void);
extern BOOL keyinbuffer(void);
extern void mycls(coord leftx, coord boty, coord rightx, coord topy);
extern void reset_mc(void);
extern coord scrnheight(void);
extern void setcolour(intl fore, intl back);
extern coord stringout(const char *str);
extern intl translate_received_key(intl c);
extern void up_scroll(coord first_line);
extern void wrchrep(uchar ch, coord i);

#if ARTHUR || RISCOS
extern intl fx_x(intl a, intl x, intl y);
extern intl fx_x2(intl a, intl x);
extern intl fx_y(intl a, intl x, intl y);
extern char *mysystem(const char *str);
extern void stampfile(const char *name, int filetype);
extern void wrch_h(char ch);
#endif

#if RISCOS
extern void clearmousebuffer(void);
extern intl getcolour(intl colour);
extern BOOL keyormouseinbuffer(void);
extern BOOL mouseinbuffer(void);
extern intl rdch_riscos(void);
extern void setbgcolour(intl colour);
extern void setfgcolour(intl colour);
extern void wrch_definefunny(intl ch);
extern void wrch_funny(intl ch);
extern void wrch_undefinefunnies(void);
#endif

#if ARTHUR
extern void bodge_funnies_for_master_font(uchar *str);
extern intl rdch_arthur(BOOL curs);
extern void setlogcolours(void);
#endif

#if MS || ARTHUR
extern void clip_menu(coord topx, coord topy, coord xlen, coord ylen);
extern size_t maxblock(void);
extern void init_screen_array(void);
extern void save_screen(coord topx, coord topy, coord xsize, coord ysize);
extern coord scrnwidth(void);
#endif

#if MS
extern void at(coord x, coord y);
extern void curoff(void);
extern void my_int86(int n, uchar ah, uchar al, uchar bh, uchar bl,
                            uchar ch, uchar cl, uchar dh, uchar dl);
extern intl rdch(BOOL cursor, BOOL doshow);
extern coord read_lineno(void);
extern void restore_screen(coord topx, coord topy, coord xlen, coord ylen);
extern void wrch(uchar x);
#endif


/* exported variables */


/* internal functions */

#if FALSE
#if ARTHUR
static uchar logcol(uchar col);
static void setjoepunterstate(void);
static void setpipedreamstate(void);
#endif

#if RISCOS
static void wrch__reallydefinefunny(intl ch, uchar bitshift);
#endif

#if ARTHUR
static intl fpe_check(void);
static char *mygets(char *array);
static void soft_stop(const char *err);
#endif

#if MS
static void swap_bytes(uchar *first, uchar *second);
#endif
#endif


#if RISCOS
#define logcol(colour) (d_colours[colour].option)
#endif


/* ----------------------------------------------------------------------- */

#if MS || ARTHUR
static BOOL couldnt_save = FALSE;
static word32 wintype = 0;
static winp lastbox = NULL;

#if ARTHUR
#define MAXBUFTRY   65536*8
#elif MS
#define MAXBUFTRY   32768
#endif

#if ARTHUR
uchar font_selected = OTHER_FONT;

#elif MS

static BOOL deepmode = FALSE;
static winp fastscreen = NULL;
static union REGS regs;         /* set up registers for PC ROM BIOS calls */
static uchar MS_smode = 3;      /* default screen mode */
static coord xpos;              /* current x pos */
static coord ypos;              /* current y pos */

static uchar text_atts = /* white_on_black */ (black << 4) + white;

/* cached screen height (st. at(0, scrnheight); moves to bottom line) */
static coord scrn_height = -1;

/* used to record screen data for dialog box operation */
static SCRLIN screen_array[25];

static coord oldtopx;
static coord oldtopy;
static coord oldxlen;
static coord oldylen;
#endif
#endif


#if ARTHUR || RISCOS
/* ----------------------- ARTHUR and RISCOS ----------------------------- */

/****************************************************
*                                                   *
*  Perform an OS_Byte call, returning the R1 value  *
*                                                   *
****************************************************/

extern intl
fx_x(intl a, intl x, intl y)
{
    _kernel_swi_regs rs;

    rs.r[0] = a;
    rs.r[1] = x;
    rs.r[2] = y;
    _kernel_swi(OS_Byte, &rs, &rs);

    return(rs.r[1]);
}


/****************************************************************
*                                                               *
* Perform an OS_Byte call, returning the R1 value, with R2 = 0  *
*                                                               *
****************************************************************/

extern intl
fx_x2(intl a, intl x)
{
    return(fx_x(a, x, 0));
}


/****************************************************
*                                                   *
*  Perform an OS_Byte call, returning the R2 value  *
*                                                   *
****************************************************/

extern intl
fx_y(intl a, intl x, intl y)
{
    _kernel_swi_regs rs;

    rs.r[0] = a;
    rs.r[1] = x;
    rs.r[2] = y;
    _kernel_swi(OS_Byte, &rs, &rs);

    return(rs.r[2]);
}


extern char *
mysystem(const char *str)
{
    os_error *err = os_cli(str);
    return(err ? err->errmess : NULL);
}


extern void
stampfile(const char *name, int filetype)
{
    char array[LIN_BUFSIZ];
    os_error *err;

    sprintf(array, SetType_Zs_Z3X_STR, name, filetype);

    if((err = os_cli(array)) != NULL)
        rep_fserr(err->errmess);
}


/************************************
*                                   *
*  write character with highlight   *
*                                   *
************************************/

/* We know how to redefine SPACE ! */
#define victimchar SPACE

extern void
wrch_h(char ch)
{
    char array[10];
    char *ptr;
    char *toprow    = array + 1;
    char *midway    = array + 4;
    char *baseline  = array + 7;
    char *from;

    #if !defined(UNDERLINE_SPACES)
    if(ch == SPACE)
        {
        print_complain(bbc_vdu(SPACE));
        return;
        }
    #endif

    array[0] = ch;
    (void) os_word(10, array);                              /* read char definition */

    if(highlights_on & N_ITALIC)
        {
        ptr = toprow;
        do { *ptr = *ptr >> 1; } while(++ptr <= midway);    /* stagger top half */
        }

    if(highlights_on & N_BOLD)
        {
        ptr = toprow;
        do { *ptr = *ptr | (*ptr << 1); } while(++ptr <= baseline + 1); /* embolden */
        }

    if(highlights_on & N_SUPERSCRIPT)
        {
        from = toprow + 2;
        ptr = toprow + 1;
        do { *ptr++ = *from; from += 2; } while(ptr <= midway);

        do { *ptr++ = '\0'; } while(ptr <= baseline + 1);
        }
    elif(highlights_on & N_SUBSCRIPT)
        {
        from = baseline;
        ptr = baseline + 1;
        do { *ptr-- = *from; from -=2; } while(ptr > midway);

        do { *ptr-- = '\0'; } while(ptr >= toprow);
        }

    print_complain(bbc_vdu(bbc_MultiPurpose));              /* start to redefine char */
    print_complain(bbc_vdu(victimchar));

    ptr = toprow;
    do { print_complain(bbc_vdu(*ptr++)); } while(ptr <= baseline);

    if(highlights_on & N_UNDERLINE)
        *ptr ^= 0xFF;                                       /* EOR underline in */

    print_complain(bbc_vdu(*ptr));                          /* complete char redefinition */

    /* print character now that it's defined */
    print_complain(bbc_vdu(victimchar));

    /* redefine SPACE after abusing it so */
    print_complain(bbc_vduq(bbc_MultiPurpose, SPACE, 0,0,0,0,0,0,0,0));
}

#endif /* ARTHUR || RISCOS */


#if ARTHUR
/* ------------------------- ARTHUR only --------------------------------- */

/****************************
*                           *
*  Check if FPE is loaded   *
*                           *
****************************/

static char fpe_checkstring[] = "FPEmulator_Version";

static intl
fpe_check(void)
{
    return( (intl) os_swi2(OS_SWINumberFromString + ErrBit,  0,
                                (int) fpe_checkstring ) );
}


/****************************************************
*                                                   *
* input is offset of colour in d_colours dialog box *
* output is logical colour                          *
*                                                   *
****************************************************/

static uchar
logcol(uchar col)
{
    switch(col)
        {
        case MENU_FORE:
            return(5);

        case MENU_BACK:
            return(6);

        case MENU_HOT:
            return(3);

        default:
            return(col);
        }
}


static char *
mygets(char *array)
{
    _kernel_swi_regs rs;

    memset(array, '\0', LIN_BUFSIZ);

    rs[0] = (intl) array;
    rs[1] = LIN_BUFSIZ-1;
    rs[2] = SPACE;
    rs[3] = 255;                /* Allow chars in 32..255 into buffer */

    _kernel_swi(OS_ReadLine+ErrBit, &rs);

    return(strchr(array, CR));
}


static void
soft_stop(const char *err)
{
    printf("Error: %s\n", err);
    bleep();
    exit(EXIT_FAILURE);
}


static void
setjoepunterstate(void)
{
    ALT_term();             /* Turn off hot key translation */

    fx_x2(4, 0);            /* Cursor keys reenabled */

    fx_x2(221, 1);          /* Function key (F10-F12, Insert(F13)) bases */
    fx_x2(222, 0xD0);
    fx_x2(223, 0xE0);
    fx_x2(224, 0xF0);

    fx_x2(225, 1);          /* Function key (Print(F0), F1-F9) bases */
    fx_x2(226, 0x90);
    fx_x2(227, 0xA0);
    fx_x2(228, 0xB0);
}


static void
setpipedreamstate(void)
{
    bbc_vdu(bbc_DisablePrinter);    /* disable printer */
    bbc_vdu(bbc_TextToText);        /* text output, not graphics */
    bbc_vdu(bbc_PageOff);           /* disable page mode */
    bbc_vdu(bbc_MoveToStart);       /* join cursors */

    /* if we are on master font use characters 161 onwards for redefinition */
    if(fx_x(240, 0, 255) == 2)
        font_selected = MASTER_FONT;

    bbc_vduq(bbc_MultiPurpose, COLUMN_DOTS, 0, 0, 0, 0, 0,  0, 0, 128+32+8+2);
    bbc_vduq(bbc_MultiPurpose, DOWN_ARROW,  24,24,24,24,24, 90,60,24);

    #if FALSE
    bbc_vduq(bbc_MultiPurpose, UP_ARROW,    24,60,90,24,24, 24,24,24);
    bbc_vduq(bbc_MultiPurpose, LEFT_ARROW,  0, 0,32,96, 255,96,32,0);
    bbc_vduq(bbc_MultiPurpose, RIGHT_ARROW, 0, 0, 4, 6, 255,6, 4, 0);
    bbc_vduq(bbc_MultiPurpose, VERTBAR,     16,16,16,16,16, 16,16,16);
    bbc_vduq(bbc_MultiPurpose, HORIZBAR,    0, 0, 0, 0, 255,0, 0, 0);
    bbc_vduq(bbc_MultiPurpose, TOPLEFT,     0, 0, 0, 0, 31, 16,16,16);
    bbc_vduq(bbc_MultiPurpose, TOPRIGHT,    0, 0, 0, 0, 240,16,16,16);
    bbc_vduq(bbc_MultiPurpose, BOTLEFT,     16,16,16,16,31, 0, 0, 0);
    bbc_vduq(bbc_MultiPurpose, BOTRIGHT,    16,16,16,16,240,0, 0, 0);
    bbc_vduq(bbc_MultiPurpose, DROP_LEFT,   16,16,16,16, 31,16,16,16);
    bbc_vduq(bbc_MultiPurpose, DROP_MIDDLE, 16,16,16,16,255,0, 0, 0);
    bbc_vduq(bbc_MultiPurpose, DROP_RIGHT,  16,16,16,16,240,16,16,16);
    #endif

    setlogcolours();


    ALT_init(ALT);          /* Use Alt key as hot key */


    fx_x2(4, 2);            /* Switch off cursor editing */

    fx_x2(219, 9);          /* initialize TAB key */

    fx_x2(221, 2);          /*       F10 key base, returns 0,128+key */
    fx_x2(222, 2);          /* SHIFT F10 key base, returns 0,128+key */
    fx_x2(223, 2);          /* CTRL  F10 key base, returns 0,128+key */
    fx_x2(224, 2);          /* CS    F10 key base, returns 0,128+key */

    fx_x2(225, 2);          /*       F0 key base, returns 0,128+key */
    fx_x2(226, 2);          /* SHIFT F0 key base, returns 0,144+key */
    fx_x2(227, 2);          /* CTRL  F0 key base, returns 0,160+key */
    fx_x2(228, 2);          /* CS    F0 key base, returns 0,172+key */

    fx_x2(229, 0);          /* Ensure ESCAPE enabled */
}


extern void
bodge_funnies_for_master_font(uchar *str)
{
    if(font_selected == MASTER_FONT)
        {
        for( ; *str != '\0'; str++)
            if(*str >= FIRST_FUNNY && *str <= LAST_FUNNY)
                *str += MASTER_FONT - OTHER_FONT;
            elif(*str == 0x9C)      /* pound sign */
                *str = 96;
        }
}


/********************************************
*                                           *
*  set logical colours to physical colours  *
*                                           *
********************************************/

extern void
setlogcolours(void)
{
    intl i;

    for(i = dialog_head[D_COLOURS].items-1; i >= 0; i--)
        {
        uchar col = d_colours[i].option;
        if(col < 16)
            bbc_palette(logcol(i), col, 0,0,0);
        else
            {
            uchar first, second, third, bright;
            bright = ((col & 64) ? 0x01 : 0) + ((col & 128) ? 0x02 : 0);
            first  = ((col &  1) ? 0x07 : 0) + ((col &   2) ? 0x38 : 0);
            second = ((col &  4) ? 0x07 : 0) + ((col &   8) ? 0x38 : 0);
            third  = ((col & 16) ? 0x07 : 0) + ((col &  32) ? 0x38 : 0);
            bbc_palette(logcol(i), 16, first << bright, second << bright, third << bright);
            }
        }
}

#endif /* ARTHUR */


#if RISCOS
/* --------------------------- RISCOS only ------------------------------- */

#define MOUSE_BUFFER_ID         246
#define MOUSE_BUFFER_EVENT_SIZE 9

extern void
clearmousebuffer(void)
{
    fx_x2(15, MOUSE_BUFFER_ID);
}


static int escape_level = 0;

extern void
escape_enable(void)
{
    if(escape_level++ == 0)
        fx_x2(229, 0);          /* Ensure ESCAPE enabled */
}


/****************************************
*                                       *
* get Wimp colour for given PD colour   *
*                                       *
****************************************/

extern intl
getcolour(intl colour)
{
    return(logcol(colour));
}


extern BOOL
keyormouseinbuffer(void)
{
    if(keyinbuffer())
        return(TRUE);

    return(mouseinbuffer());
}


/************************************************************
*                                                           *
* mouse event going to arrive soon (not neccesarily to us)? *
*                                                           *
************************************************************/

extern BOOL
mouseinbuffer(void)
{
    int res = _kernel_osbyte(128, MOUSE_BUFFER_ID, 0);

    if(TRACE  &&  (res & 0xFFFF))
        tracef1("mouse buffer has %d bytes in it\n", res & 0xFFFF);

    return((res & 0xFFFF) > MOUSE_BUFFER_EVENT_SIZE);
}


/********************************************
*                                           *
* set background colour to specified colour *
* for things that are never inverted and    *
* always printing using current foreground  *
*                                           *
********************************************/

extern void
setbgcolour(intl colour)
{
    riscos_setcolour(logcol(colour), TRUE); /* bg */
}


/********************************************
*                                           *
* set foreground colour to specified colour *
* for things that are never inverted and    *
* always printing over current background   *
*                                           *
********************************************/

extern void
setfgcolour(intl colour)
{
    riscos_setcolour(logcol(colour), FALSE); /* fg */
}


static uword32 chardefined = 0;

/* shift factors to get bits in the chardefined bitset */

typedef enum
{
    shf_COLUMN_DOTS,
    shf_DOWN_ARROW,
#if FALSE
    shf_UP_ARROW,
    shf_LEFT_ARROW,
    shf_RIGHT_ARROW,
    shf_VERTBAR,
    shf_HORIZBAR,
    shf_TOPLEFT,
    shf_TOPRIGHT,
    shf_BOTLEFT,
    shf_BOTRIGHT,
    shf_DROP_LEFT,
    shf_DROP_MIDDLE,
    shf_DROP_RIGHT,
#endif
    shf_last
}
charshiftfactors;

static uchar oldchardef[shf_last][10];  /* bbc_MultiPurpose, ch, def[8] */


/****************************************************
*                                                   *
* define funny character:                           *
*                                                   *
* read current definition to oldchardef[][],        *
* redefine desired character and mark as defined    *
* note that we can't read all chardefs at start     *
* as punter may change alphabet (font) at any time  *
*                                                   *
****************************************************/

static void
wrch__reallydefinefunny(intl ch, uchar bitshift)
{
    oldchardef[bitshift][0] = (uchar) bbc_MultiPurpose;
    oldchardef[bitshift][1] = (uchar) ch;
    (void) os_word(10, &oldchardef[bitshift][1]);   /* read old definition */

    switch(ch)
        {
        case COLUMN_DOTS:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch,  0,  0,  0,  0,  0,  0,  0, 128+32+8+2));
            break;

        case DOWN_ARROW:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch, 24, 24, 24, 24, 24, 90, 60, 24));
            break;

#if FALSE
        case UP_ARROW:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch, 24, 60, 90, 24, 24, 24, 24, 24));
            break;

        case LEFT_ARROW:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch,  0,  0, 32, 96,255, 96, 32,  0));
            break;

        case RIGHT_ARROW:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch,  0,  0,  4,  6,255,  6,  4,  0));
            break;

        case VERTBAR:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch, 16, 16, 16, 16, 16, 16, 16, 16));
            break;

        case HORIZBAR:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch,  0,  0,  0,  0,255,  0,  0,  0));
            break;

        case TOPLEFT:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch,  0,  0,  0,  0, 31, 16, 16, 16));
            break;

        case TOPRIGHT:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch,  0,  0,  0,  0,240, 16, 16, 16));
            break;

        case BOTLEFT:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch, 16, 16, 16, 16, 31,  0,  0,  0));
            break;

        case BOTRIGHT:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch, 16, 16, 16, 16,240,  0,  0,  0));
            break;

        case DROP_LEFT:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch, 16, 16, 16, 16, 31, 16, 16, 16));
            break;

        case DROP_MIDDLE:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch, 16, 16, 16, 16,255,  0,  0,  0));
            break;

        case DROP_RIGHT:
            wimpt_safe(bbc_vduq(bbc_MultiPurpose, ch, 16, 16, 16, 16,240, 16, 16, 16));
            break;
#endif
        }

    chardefined |= (((uword32) 1) << bitshift);
}


/************************************************
*                                               *
*  ensure funny character defined and print it  *
*                                               *
************************************************/

extern void
wrch_funny(intl ch)
{
    wrch_definefunny(ch);
    wimpt_safe(bbc_vdu(ch));
}


/************************************************
*                                               *
*  define funny character for subsequent print  *
*                                               *
*  if character is not yet defined then         *
*  define it and mark it as defined             *
*                                               *
************************************************/

extern void
wrch_definefunny(intl ch)
{
    char bitshift;

    switch(ch)
        {
        case COLUMN_DOTS:
            bitshift = shf_COLUMN_DOTS;
            break;

        case DOWN_ARROW:
            bitshift = shf_DOWN_ARROW;
            break;

#if FALSE
        case UP_ARROW:
            bitshift = shf_UP_ARROW;
            break;

        case LEFT_ARROW:
            bitshift = shf_LEFT_ARROW;
            break;

        case RIGHT_ARROW:
            bitshift = shf_RIGHT_ARROW;
            break;

        case VERTBAR:
            bitshift = shf_VERTBAR;
            break;

        case HORIZBAR:
            bitshift = shf_HORIZBAR;
            break;

        case TOPLEFT:
            bitshift = shf_TOPLEFT;
            break;

        case TOPRIGHT:
            bitshift = shf_TOPRIGHT;
            break;

        case BOTLEFT:
            bitshift = shf_BOTLEFT;
            break;

        case BOTRIGHT:
            bitshift = shf_BOTRIGHT;
            break;

        case DROP_LEFT:
            bitshift = shf_DROP_LEFT;
            break;

        case DROP_MIDDLE:
            bitshift = shf_DROP_MIDDLE;
            break;

        case DROP_RIGHT:
            bitshift = shf_DROP_RIGHT;
            break;
#endif

        default:
            return; /* non-funny char, so exit */
        }

    if(!(chardefined & (((uword32) 1) << bitshift)))
        wrch__reallydefinefunny(ch, bitshift);
}


/************************************************
*                                               *
* undefine funny characters that have been      *
* defined this time round and mark as undefined *
*                                               *
************************************************/

extern void
wrch_undefinefunnies(void)
{
    uword32 bitshift = 0;
    uword32 bitmask;

    while((chardefined != 0)  &&  (bitshift < 32))
        {
        bitmask  = ((uword32) 1) << bitshift;

        if(chardefined & bitmask)
            {
            tracef1("undefining char %d\n", oldchardef[bitshift][1]);
            wimpt_safe(os_swi2(OS_WriteN, (int) &oldchardef[bitshift][0], 10));
            chardefined ^= bitmask;
            }

        bitshift++;
        }
}

#endif /* RISCOS */


#if MS
/* --------------------------- MS-DOS only ------------------------------- */

static void
swap_bytes(uchar *first, uchar *second)
{
    uchar temp = *first;

    *first  = *second;
    *second = temp;
}


#if !RISCOS  &&  FALSE
/* this is only grubby and for testing grid code temporarily on PC */

extern void draw_grid_vbar()
{
    /* go back one and draw | */
    at(xpos-1, ypos);   
    wrch('|');
}
#endif


/********************************
*                               *
* set xpos and ypos for wrch()  *
*                               *
********************************/

extern void
at(coord x, coord y)
{
    if(fastdraw)
        {
        sb_move(fastscreen, x, y);
#if 0
        xpos = x;
        ypos = y;
#endif
        }
    else
        {
        _settextposition(y+1, x+1);
        ypos = y;
        xpos = x;
        }
}


/********************
*                   *
*  turn cursor off  *
*                   *
********************/

extern void
curoff(void)
{
/*  _displaycursor(_GCURSOROFF); */

/*  if(Vcs.Vcard == CARD_EGA)
        my_int86(0x10, 1,0,0,0,1,0,0,0);
    else
*/
        my_int86(0x10, 1,0,0,0,0x20,0,0,0);
}


/****************
*               *
*  Deep Screen  *
*               *
****************/

extern void
DeepScreen_fn(void)
{
    if(fastdraw)
        {
        intl oldhyt = scrn_height;

        if(!mergebuf_all())
            return;

        deepmode = !deepmode;

        setvmode(MS_smode, multiple_pages, deepmode);

        if((scrn_height = Vcs.Vheight - 1) != oldhyt)
            {
            #if defined(MANY_DOCUMENTS)
            dochandle doc = current_document_handle();
            window_data *tb = NULL;
            #endif

            while((tb = next_document(tb)) != NULL)
                {
                select_document(tb);

                paghyt = scrn_height;

                reinit_rows();
                xf_draweverything = TRUE;
                }

            select_document_using_handle(doc);
            }
        }
    else
        reperr_null(ERR_EGA);
}


extern void
my_int86(int n, uchar ah, uchar al, uchar bh, uchar bl,
                uchar ch, uchar cl, uchar dh, uchar dl)
{
    regs.h.ah = ah;
    regs.h.al = al;
    regs.h.bh = bh;
    regs.h.bl = bl;
    regs.h.ch = ch;
    regs.h.cl = cl;
    regs.h.dh = dh;
    regs.h.dl = dl;

    int86(n, &regs, &regs);
}


extern coord
read_lineno(void)
{
    my_int86(0x10,3,0, (uchar) Vcs.Vpage,0,0,0,0,0);
    return((coord) regs.h.dh);
}


/*
 * Save part of screen and restore it. Used for menus on PC
 * Try brute force 'n' ignorance approach
 * 25*2*80
*/


extern void
restore_screen(coord topx, coord topy, coord xlen, coord ylen)
{
    coord y;

    setcolour(FORE, BACK);

    for(y = topy; y < topy+ylen; y++)
        {
        at(topx, y);
        ospca(xlen);
        }

    for(y = topy; y < topy+ylen; y++)
        {
        uchar *cptr;
        uchar *aptr;
        uchar *eol;

        at(topx, y);

        cptr = screen_array[y].ch+topx;
        eol = cptr+xlen;
        aptr = screen_array[y].att+topx;

        for( ; cptr < eol; cptr++, aptr++)
            {
            text_atts = *aptr;
            wrch(*cptr);
            }
        }

    xf_drawmenuheadline = TRUE;
}


extern void
wrch(uchar x)
{
    if(fastdraw)
        {
        sb_wca(fastscreen, x, text_atts, 1);
#if 0
        xpos++;
#endif
        }
    else
        {
        /* on PC ROM BIOS you can specify the colours of the character
         * and you can move on the cursor.  But you can't do both.
         * Well, who'd want to?
         * So, we use service 9 to output characters,
         * then move the cursor on.
        */

/* write character and attribute */
        regs.h.ah = 9;
        regs.h.al = (uchar) x;
        regs.h.bh = (uchar) Vcs.Vpage;
        regs.h.bl = text_atts;
        regs.x.cx = 1;
        int86(0x10, &regs, &regs);

/* write cursor pos */
        regs.h.ah = 2;
        regs.h.bh = (uchar) Vcs.Vpage;
        regs.h.dh = (uchar) ypos;
        regs.h.dl = (uchar) ++xpos;
        int86(0x10, &regs, &regs);

        _settextposition(ypos+1, xpos+1);
        }
}

#endif /* MS */


#if MS || ARTHUR
/* ------------------------ MS-DOS and ARTHUR ---------------------------- */

#define calcwintype(a, b, c, d)  (((word32) a) * ((word32) c) * ((word32) b) * ((word32) d))
static word32 (calcwintype)(coord a, coord b, coord c, coord d);


extern void
clip_menu(coord topx, coord topy, coord xlen, coord ylen)
{

#if ARTHUR
    if(lastbox != NULL)
        {
        arch_rest(lastbox);
        arch_free(lastbox);
        lastbox = NULL;
        }
    elif(couldnt_save)
        {
        couldnt_save = FALSE;
        if(wintype == calcwintype(topx,topy,xlen,ylen))
            xf_draweverything = TRUE;
        elif(!sqobit)
            new_screen();
        }

    if(ylen == 0)
        setcolour(FORE, BACK);
    else
        {
        lastbox = arch_new(topx, topy, xlen, ylen);
        if(lastbox == NULL)
            {
            couldnt_save = TRUE;
            wintype = calcwintype(topx,topy,xlen,ylen);
            }
        elif(arch_save(lastbox) < 0)
                {
                arch_free(lastbox);
                lastbox = NULL;
                couldnt_save = TRUE;
                wintype = calcwintype(topx,topy,xlen,ylen);
                }
        else
            {
            couldnt_save = FALSE;
            wintype = 0;
            }
        }

    return;

#elif MS
    coord dont_bother_after = calrad(rowsonscreen);

    if(fastdraw)
        {
        if(lastbox != NULL)
            {
            sb_rest(lastbox);
            sb_free(lastbox);
            lastbox = NULL;
            }
        elif(couldnt_save)
            {
            couldnt_save = FALSE;
            if(wintype == calcwintype(topx,topy,xlen,ylen))
                xf_draweverything = TRUE;
            else
                new_screen();
            }

        if(ylen == 0)
            setcolour(FORE, BACK);
        else
            {
            lastbox = sb_new(topx, topy, xlen, ylen);
            if(lastbox == NULL)
                {
                couldnt_save = TRUE;
                wintype = calcwintype(topx,topy,xlen,ylen);
                }
            else
                {
                wintype = (word32) 0;
                sb_save(lastbox);
                }
            }
        return;
        }

    setcolour(FORE, BACK);


    /* try no overlap */

    if(ylen == 0 && xf_draweverything)
        ;
    elif(ylen==0  ||  topx > oldtopx+oldxlen  ||  oldtopx > topx+xlen)
        {
        if(dont_bother_after < oldylen)
            {
            mycls(oldtopx, oldtopy+oldylen-1, oldtopx+oldxlen-1, oldtopy);
            oldylen = dont_bother_after;
            }
        restore_screen(oldtopx, oldtopy, oldxlen, oldylen);
        }
    else
        {

        /* some overlap. first it spaces over clipped area cos this is fast
            (on PC) and prettier, even though total process is slower
        */

        coord newboty = MIN(oldylen,ylen) + oldtopy -1;
        coord oldrightx = oldtopx+oldxlen-1;

        if(oldtopx < topx)                              /* clip it to left */
            mycls(oldtopx, newboty, topx-1, oldtopy);

        if(oldtopx + oldxlen > topx+xlen)               /* clip it to right */
            mycls(topx+xlen, newboty, oldrightx, oldtopy);

        if(oldylen > ylen)                                  /* clip bottom */
            mycls(oldtopx, topy+oldylen-1, oldrightx, topy+ylen);
        /* now do the restore */

        if(dont_bother_after < oldylen)
            oldylen = dont_bother_after;

        if(oldtopx < topx)
            restore_screen(oldtopx, oldtopy, topx-oldtopx, MIN(oldylen,ylen));
        if(oldtopx + oldxlen > topx+xlen)
            restore_screen(topx+xlen, oldtopy, oldtopx+oldxlen-(topx+xlen),MIN(oldylen,ylen));
        if(oldylen > ylen)
            restore_screen(oldtopx, topy+ylen, oldxlen, oldylen-ylen);
        }

    oldtopx = topx;
    oldtopy = topy;
    oldxlen = xlen;
    oldylen = ylen;

#endif
}


/****************************************************
*                                                   *
* Do Operating System calls                         *
*                                                   *
* Under MS-DOS just calls a new command interpreter *
*                                                   *
****************************************************/

extern void
OSCommand_fn(void)
{
    if(xf_inexpression)
        {
        reperr_null(ERR_EDITINGEXP);
        return;
        }

    clslnf();       /* close linking files */

#if MS
    if(!multiple_pages)
        {
        clearscreen();
        setcolour(FORE, BACK);
        sb_show();
        }
    putstate();

    printf("\n\nEnter EXIT to return to %s\n", applicationname);

    #if defined(MS_HUGE)
    release();
    #endif

    while(system("COMMAND"))
        {
        clearkeyboardbuffer();

        switch(errno)
            {
            case ENOENT:
                printf("\nInsert system disc and press a key or Esc to return to %s",
                        applicationname);
                if(rdch(FALSE, FALSE) == ESCAPE)
                    break;
                continue;

            default:
                printf("\nCannot run COMMAND.COM - press a key to continue ");
                rdch(FALSE, FALSE);
                break;
            }

        break;
        }

    #if defined(MS_HUGE)
    capture(&ctrlflag);
    #endif

    getstate();
    Vcs.Vsync = sync;
    if(fastdraw)
        setvmode(MS_smode, multiple_pages, deepmode);

    xf_draweverything = TRUE;
    curoff();

#elif ARTHUR
        {
        intl smode = fx_y(135, 0, 0);

/* 2.21 */
        ack_esc();                  /* Cancel the ESCAPE that got us here */
/* 2.21 */

        clearscreen();              /* sets xf_draweverything */
        at(0,0);
        printf("Press ESCAPE to return to %s\n\n", applicationname);

        setjoepunterstate();

        for(;;)
            {
            char array[LIN_BUFSIZ];
            char *errmess;

/* 2.21 replaced similar section*/
            BOOL nostring = FALSE;

            if(!ctrlflag)           /* Maybe ESC from command just executed */
                {
                curon();
                wrch('*');
                nostring = (mygets(array) == NULL);
                }

            if(nostring || ctrlflag)
                {
                ack_esc();          /* Ensure clear for when we return */

                if(fx_y(135, 0, 0) != smode)
                    bbc_mode(smode);

                setpipedreamstate();

                curoff();
                return;
                }

            if((errmess = (uchar *) mysystem((char *) array)) != NULL)
                puts((char *) errmess);

            curoff();
/* 2.21 */
            }
        }
#endif
}


/****************************************
*                                       *
* Depends on machine type for constant  *
*                                       *
****************************************/

extern size_t
maxblock(void)
{
    size_t bufsize = MAXBUFTRY;
    void *buffer;

    do  {
        buffer = malloc(bufsize);
        bufsize /= 2;
        }
    while(!buffer  &&  (bufsize > 0x100));

    dispose(&buffer);

    return(bufsize);
}


extern void
init_screen_array(void)
{
    clip_menu(0, 0, 0, 0);

    #if MS
    memset(screen_array, FIRST_HIGHLIGHT, sizeof(screen_array));
    #endif
}


extern void
save_screen(coord topx, coord topy, coord xsize, coord ysize)
{
#if ARTHUR
    clip_menu(topx, topy, xsize, ysize); /* Simple, eh ? */

#elif MS
    coord x,y;

    if(fastdraw)
        {
        clip_menu(topx, topy, xsize, ysize);
        return;
        }

    for(y = topy; y < topy+ysize; y++)
        {
        uchar *cptr = screen_array[y].ch  + topx;
        uchar *aptr = screen_array[y].att + topx;
        uchar *eol  = cptr + xsize;

        for(x = topx; cptr < eol; cptr++, aptr++, x++)
            {
            /* if the character isn't FIRST_HIGHLIGHT, this one's been set */

            if(*cptr != FIRST_HIGHLIGHT)
                continue;

            at(x,y);

            regs.h.ah = 8;      /* read character and attribute */
            regs.h.bh = (uchar) Vcs.Vpage;
            int86(0x10, &regs, &regs);

            *cptr = regs.h.al;
            *aptr = regs.h.ah;
            }
        }
#endif
}


/***************************************
*                                      *
* offset of last text column on screen *
* NB: should return 79 in mode 3       *
*                                      *
***************************************/

extern coord
scrnwidth(void)
{
#if ARTHUR
    return((coord) bbc_vduvar(bbc_ScrRCol));

#elif MS
    if(fastdraw)
        return(Vcs.Vwidth - 1);
    else
        {
        my_int86(0x10, (uchar) 0x0F,0,0,0,0,0,0,0);
        MS_smode = regs.h.al;               /* screen mode */
        return((coord) (regs.h.ah - 1));    /* width */
        }
#endif
}

#endif /* MS || ARTHUR */


/* ------------------- Ones that are basically similar ------------------- */

extern void
ack_esc(void)
{
    tracef0("ack_esc()\n");

    if(ctrlflag)
        {
        #if ARTHUR || RISCOS
        fx_x2(126, 0);
        #else
        rdch(FALSE, FALSE);
        #endif
        ctrlflag = 0;
        }
}


extern void
bleep(void)
{
#if ARTHUR || RISCOS
    wimpt_safe(bbc_vdu(bbc_Bell));
#elif MS
    putchar('\a');
#endif
}


extern void
clearscreen(void)
{
    setcolour(FORE, BACK);

#if RISCOS
    tracef0("clearscreen()\n");
    wimpt_safe(bbc_vdu(bbc_TextToText));        /* Ensure VDU 4 printing */
    wimpt_safe(bbc_vdu(bbc_DefaultWindow));
    wimpt_safe(bbc_cls());

#elif ARTHUR
    bbc_cls();

#elif MS
    if(fastdraw)
        sb_fillca(fastscreen, SPACE, text_atts);
    else
        _clearscreen(_GCLEARSCREEN);
#endif

    curoff();
    xf_draweverything = TRUE;
}


extern void
clearkeyboardbuffer(void)
{
#if ARTHUR || RISCOS
    fx_x2(15, 1);

    clearmousebuffer();
#else
    while(keyinbuffer())
        getch();
#endif
}


/********************
*                   *
*  turn cursor on   *
*                   *
********************/

extern void
curon(void)
{
#if RISCOS
    /* do nothing */

#elif ARTHUR
    bbc_vduq(bbc_MultiPurpose, 0,10, 0x60,0,0,0,0,0,0); /* block cursor */

#elif MS
/* _displaycursor(_GCURSORON); */

    my_int86(0x10, 1,0,0,0,0,13,0,0);
#endif
}


/* returns TRUE if key is depressed */

extern BOOL
depressed(intl key)
{
#if ARTHUR || RISCOS

    return(fx_x(129, key, 0xFF) == (intl) 0xFF);

#elif MS

    return((_bios_keybrd(_KEYBRD_SHIFTSTATUS) & key) != 0);

#endif
}


extern BOOL
depressed_ctrl(void)
{
    return(depressed(CTRL));
}


extern BOOL
depressed_shift(void)
{
    return(depressed(SHIFT));
}


/************************************************
*                                               *
*  scroll from first_line to bos down one line  *
*                                               *
************************************************/

extern void
down_scroll(coord first_line)
{
#if RISCOS
    tracef1("down_scroll(%d)\n", first_line);

    scroll_textarea(0, paghyt, pagwid_plus1, first_line, 1);

#elif ARTHUR
/* on Acorn doo-dahs do hard scroll if possible */
    if(first_line != 0)
        wimpt_safe(bbc_vduq(bbc_DefTextWindow, 0, paghyt, pagwid, first_line));
    at(0, 0);                               /* go to top of window */
    wimpt_safe(bbc_vdu(bbc_MoveUpOne));     /* scroll */
    wimpt_safe(bbc_vdu(bbc_DefaultWindow));

#elif MS
    if(fastdraw)
        {
        sb_set_scrl(fastscreen, 0, first_line, Vcs.Vwidth, Vcs.Vheight);
        sb_scrl(fastscreen, -1, text_atts);
        }
    else
        my_int86(0x10, 7,1,7,0,(uchar) first_line,0,(uchar) paghyt, (uchar) pagwid);
#endif
}


/*******************************
*                              *
* erase to end of current line *
*                              *
*******************************/

extern void
eraeol(void)
{
#if RISCOS
    tracef0("!!! eraeol() !!!\n");
    /* do nothing */

#elif ARTHUR
    coord x = bbc_pos();
    coord y = bbc_vpos();

    mycls(x, y, pagwid, y);
    at(x, y);

#elif MS
/* in MESSDOS write spaces to eol */
    coord x;
    coord y;

    if(fastdraw)
        sb_getpos(&x, &y);
    else
        x = xpos;

    ospca(pagwid_plus1 - x);
    at(x, ypos);
#endif
}


/********************************************************************
*                                                                   *
* On RISC OS, disable ESCAPE condition generation.                  *
* On all systems, report an error if such a condition was present   *
* and clear that condition. Returns the old condition.              *
*                                                                   *
********************************************************************/

extern BOOL
escape_disable_nowinge(void)
{
    BOOL was_ctrlflag;

    #if RISCOS
    if(--escape_level == 0)
        fx_x2(229, 1);          /* Ensure ESCAPE disabled */
    #endif

    was_ctrlflag = ctrlflag;

    if(was_ctrlflag)
        ack_esc();              /* clears ctrlflag */

    return(was_ctrlflag);
}


extern BOOL
escape_disable(void)
{
    BOOL was_ctrlflag = escape_disable_nowinge();

    if(was_ctrlflag)
        reperr_null(ERR_ESCAPE);

    return(was_ctrlflag);
}


/********************************************************
*                                                       *
* invert the text fg and bg colours                     *
* The current state is maintained by currently_inverted *
*                                                       *
********************************************************/

extern void
invert(void)
{
#if RISCOS
    currently_inverted = !currently_inverted;

    riscos_invert();
#elif ARTHUR
    currently_inverted = !currently_inverted;

    if(in_dialog_box)
        setcolour(MENU_FORE, MENU_BACK);
    else
        setcolour(FORE, BACK);
#elif MS
    intl fore, back;

    fore = (in_dialog_box) ? MENU_FORE : FORE;
    back = (in_dialog_box) ? MENU_BACK : BACK;

    swap_bytes(&d_colours[fore].option, &d_colours[back].option);
    setcolour(fore, back);

    currently_inverted = currently_inverted ? 0 : 63;

#endif
}


/****************
*               *
* macroed in MS *
*               *
****************/

extern BOOL
keyinbuffer(void)
{
#if ARTHUR || RISCOS
    int carry = 0;
    _kernel_swi_regs rs;

    _kernel_swi_c(OS_ReadEscapeState, &rs, &rs, &carry); /* When we do get() we'll get ESC */
    if(carry != 0)
        return(TRUE);

    rs.r[0] = 152; /* Examine keyboard buffer */
    rs.r[1] = 0;
    rs.r[2] = 0;
    _kernel_swi_c(OS_Byte, &rs, &rs, &carry);
    return(carry == 0);

#elif MS
    return((BOOL) (_bios_keybrd(_KEYBRD_READY) != 0));

#endif
}


/****************************
*                           *
* clear rectangle on screen *
*                           *
****************************/

extern void
mycls(coord leftx, coord boty, coord rightx, coord topy)
{
#if RISCOS
    tracef4("!!! mycls(%d, %d, %d, %d) !!!\n", leftx, boty, rightx, topy);
    /* do nothing */

#elif ARTHUR
    BOOL need_to_reset = FALSE;

    if(currently_inverted)
        {
        invert();
        need_to_reset = TRUE;
        }

    bbc_vduq(bbc_DefTextWindow, leftx, boty, rightx, topy);
    bbc_cls();
    bbc_vdu(bbc_DefaultWindow);         /* reset origin */

    if(need_to_reset)
        invert();

#elif MS
    if(fastdraw)
        {
        sb_set_scrl(fastscreen, leftx, topy, rightx-leftx+1, boty+1);
        sb_scrl(fastscreen, boty-topy+1, text_atts);
        }
    else
        my_int86(0x10, 6,0,text_atts,0,(uchar) topy, (uchar) leftx,
                    (uchar) boty, (uchar) rightx);

#endif
}


/********************************************************
*                                                       *
*  Initialise machine and library state for PipeDream   *
*                                                       *
********************************************************/

static BOOL mc__initialised = FALSE;

extern void
init_mc(void)
{
#if ARTHUR || RISCOS

#if RISCOS

/* screen variables are determined by the opened window */

/* FP will have been ensured by !Run file on RISC OS */

#elif ARTHUR

    bbc_vdu(bbc_DefaultWindow);         /* get default window for size */
    paghyt       = scrnheight();
    pagwid_plus1 = scrnwidth();
    pagwid       = pagwid_plus1 - 1;

    if(pagwid_plus1 < 79)
        soft_stop("needs 80 column mode");

    if(fpe_check())
        soft_stop("fpe not loaded");

    /* the second parameter in the following call is the width of the edit
     * dialog box, cos this is the largest thing that needs to be saved
    */
    if(arch_init(1, 75, 20))
        soft_stop("not enough memory in this mode");

    setpipedreamstate();

#endif

    setlocale(LC_CTYPE, DefaultLocale_STR);
    /* use LC_ALL when we know what it does */

    (void) add_to_list(&first_key, HOMEKEY, "\031ctc\x0D", NULL);       /* Top Of Column */
    (void) add_to_list(&first_key, ENDKEY,  "\031cbc\x0D", NULL);       /* Bottom Of Column */

#elif MS
    getstate();

    Vcs.Vsync = sync;

    if(Vcs.Vcard != CARD_MDA)
        MS_smode = 3;               /* always use mode 3 */
    else
        MS_smode = 7;

    if(fastdraw)
        {
        setvmode(MS_smode, multiple_pages, deepmode);
        sb_init();
        fastscreen = sb_new(0, 0, Vcs.Vwidth, Vcs.Vheightmax);

        #if !defined(MANY_DOCUMENTS)
            paghyt       = Vcs.Vheight  - 1;
            pagwid_plus1 = Vcs.Vwidth   - 1;
        #endif
        }
    else
        {
        my_int86(0x10, 0,MS_smode,0,0,0,0,0,0);

        #if !defined(MANY_DOCUMENTS)
        paghyt       = scrnheight();
        pagwid_plus1 = scrnwidth()  - 1;
        #endif
        }

    #if !defined(MANY_DOCUMENTS)
    pagwid = pagwid_plus1 - 1;
    #endif

    (void) add_to_list(&first_key, SLEFTCURSOR,   "\031cpw\x0D", NULL);
    (void) add_to_list(&first_key, SRIGHTCURSOR,  "\031cnw\x0D", NULL);
    (void) add_to_list(&first_key, CSUPCURSOR,    "\031csu\x0D", NULL);
    (void) add_to_list(&first_key, CSDOWNCURSOR,  "\031csd\x0D", NULL);
    (void) add_to_list(&first_key, CSLEFTCURSOR,  "\031csl\x0D", NULL);
    (void) add_to_list(&first_key, CSRIGHTCURSOR, "\031csr\x0D", NULL);

#endif

    (void) add_to_list(&first_key, INSERTKEY,       "\031u\x0D",   NULL);       /* Insert Space */
    (void) add_to_list(&first_key, CTRL_TAB,        "\031cfc\x0D", NULL);       /* First Column */
    (void) add_to_list(&first_key, CTRL_SHIFT_TAB,  "\031clc\x0D", NULL);       /* Last Column */

    (void) add_to_list(&first_key, RUBOUT              + SHIFT_ADDED,   "\031g\x0D",   NULL);   /* Delete Character Right */
    (void) add_to_list(&first_key, RUBOUT + CTRL_ADDED,                 "\031y\x0D",   NULL);   /* Delete Row */
    (void) add_to_list(&first_key, RUBOUT + CTRL_ADDED + SHIFT_ADDED,   "\031edc\x0D", NULL);   /* Delete Column */

    mc__initialised = TRUE;
}


extern void
reset_mc(void)
{
    if(mc__initialised)
        {
        mc__initialised = FALSE;

        clslnf();

        #if !defined(SPELL_OFF)
        close_user_dictionaries();
        #endif

        #if !defined(Z88_OFF)
        if(Z88_on)
            z88_close();
        #endif

        #if RISCOS

        #elif ARTHUR
        setjoepunterstate();

        d_colours[FORE].option = white;
        d_colours[BACK].option = black;
        clearscreen();
        curon();

        clearkeyboardbuffer();
        #elif MS
        clearscreen();
        sb_show_if_fastdraw();
        putstate();

        clearkeyboardbuffer();
        #endif
        }
}


/****************************************************************************
*                                                                           *
* rdch macroed on ARTHUR to rdch_arthur                                     *
* rdch macroed on RISCOS to rdch_riscos                                     *
*                                                                           *
* SKS notes that we can't use signal(SIGINT) as we rely on being able to    *
* read the two bytes of a function key sequence and one or both may be      *
* purged by the C runtime library after keyinbuffer() or the first get()    *
*                                                                           *
****************************************************************************/

#if RISCOS

extern intl
rdch_riscos(void)
{
    intl c, res;

    c = bbc_get() & 0xFF;   /* C from OS_ReadC returned in 0x100 */

    res = c ? c : 0 - (bbc_get() & 0xFF);

    return(translate_received_key(res));
}

#elif ARTHUR

extern intl
rdch_arthur(BOOL curs)
{
    intl c, res;

    if(curs)
        curon();

    do  {
        c = bbc_get() & 0xFF;   /* C from OS_ReadC returned in 0x100 */
        }
        while(c == SLRLDI); /* Don't allow punter to type CTRL-Y */

    if(curs)
        curoff();

/* SKS removed fx124 as this could destroy the next char (see above) */

    res = c ? c : 0 - (bbc_get() & 0xFF);

    return(translate_received_key(res));
}

#elif MS

extern intl
rdch(BOOL cursor, BOOL doshow)
{
    intl res;
    unsigned int c;
    uchar ch;

    if(fastdraw && doshow)
        sb_show();

    if(cursor)
        curon();

    do
        {
        c  = _bios_keybrd(_KEYBRD_READ);
        ch = (uchar) (c & 0xFF);
        }
    while(ch == SLRLDI);

    if(cursor)
        curoff();

    res = ch ? (intl) ch : 0 - (intl) (c >> 8);

    return(translate_received_key(res));
}

#endif


/*************************************************************
*                                                            *
* offset of bottom text line on screen                       *
* NB at(0, scrnheight) should put cursor at bottom of screen *
*                                                            *
*************************************************************/

extern coord
scrnheight(void)
{
#if RISCOS || ARTHUR
    return((coord) bbc_vduvar(bbc_ScrBCol));

#elif MS
    if(scrn_height == -1)
        {
        clearscreen();
    
        /* to find height, go to top and go down to bottom, counting screen lines */
        for(;;)
            {
            putchar('\n');
            my_int86(0x10,3,0,(uchar) Vcs.Vpage,0,0,0,0,0);  /* get cursor pos */
            if(((intl) regs.h.dh) > scrn_height)
                scrn_height = (intl) regs.h.dh;
            else
                break;
            }
        }

    return(scrn_height);
#endif
}


/***********************************************
*                                              *
* set fg and bg colour to those specified      *
* or the other way round if currently_inverted *
*                                              *
***********************************************/

extern void
setcolour(intl fore, intl back)
{
#if RISCOS
    if(currently_inverted)
        {
          if(fore == FORE  &&  back == BACK)
            {
            fore = BACK;
            back = FORE;
            }
        elif(fore == BACK  &&  back == FORE)
            {
            fore = FORE;
            back = BACK;
            }
        }
#endif

#if RISCOS
    riscos_setcolours(logcol(back), logcol(fore));

#elif MS
    fore = d_colours[fore].option;
    back = d_colours[back].option;

    text_atts = (uchar) ((back << 4) + fore);
    _settextcolor((short) fore);
    _setbkcolor((long) back);

#endif
}


/*******************************************
*                                          *
* output string in current highlight state *
* printf on PC won't print inverse         *
*                                          *
* returns number of characters printed     *
*                                          *
*******************************************/

extern coord
stringout(const char *str)
{
    coord len;

#if RISCOS
    char ch;
    len = strlen(str);
    #if TRUE
    while((ch = *str++) != '\0')
        sndchr(ch);
    #else
    if(highlights_on)
        while((ch = *str++) != '\0')
            wrch_h(ch);
    else
        print_complain(os_swi1(OS_Write0 | XOS_MASK, (int) str));
    #endif

#elif ARTHUR
    char ch;
    len = strlen(str);
    if(highlights_on)
        while((ch = *str++) != '\0')
            wrch_h(ch);
    else
        fputs(str, stdout); /* eats ctrl chars */

#elif MS
    if(fastdraw)
        {
        len = sb_wsa(fastscreen, str, text_atts);
        #if 0
        xpos += len;
        #endif
        }
    else
        {
        len = strlen(str);
        xpos += len;
        _outtext((char *) str);
        }

#endif

    return(len);
}


/************************************************************
*                                                           *
*  fiddle around with obtained key value to get functions   *
*  that host OS does not provide as distinct key presses    *
*                                                           *
************************************************************/

extern intl
translate_received_key(intl c)
{
#if RISCOS || ARTHUR

    if(c == LEFTDELETE)
        {
        c = RUBOUT;

        if(depressed_shift())
            c |= SHIFT_ADDED;
        }

    elif(c == RUBOUT)
        {
        if(depressed_shift())
            c |= SHIFT_ADDED;

        if(depressed_ctrl())
            c |= CTRL_ADDED;
        }

#elif MS

    if(c == DELETEKEY)
        {
        c = RUBOUT;

        if(depressed_shift())
            c |= SHIFT_ADDED;
        }

    elif(c == RUBOUT)
        {
        if(depressed_shift())
            c |= SHIFT_ADDED;

        if(depressed_ctrl())
            c |= CTRL_ADDED;
        }

    elif((c == CLEFTCURSOR)  &&  depressed_shift())
        c = CSLEFTCURSOR;

    elif((c == CRIGHTCURSOR)  &&  depressed_shift())
        c = CRIGHTCURSOR;

    elif((c == LEFTCURSOR)  &&  depressed_shift())
        c = SLEFTCURSOR;

    elif((c == RIGHTCURSOR)  &&  depressed_shift())
        c = SRIGHTCURSOR;

#endif

    tracef1("translated key to %4.4X\n", c);

    return(c);
}


/********************************************
*                                           *
* scroll from first_line to bos up one line *
*                                           *
********************************************/

extern void
up_scroll(coord first_line)
{
#if RISCOS
    tracef1("up_scroll(%d)\n", first_line);

    scroll_textarea(0, paghyt, pagwid_plus1, first_line, -1);

#elif ARTHUR
    if(first_line != 0)
        wimpt_safe(bbc_vduq(bbc_DefTextWindow, 0, paghyt, pagwid, first_line));
    at(0, paghyt - first_line);
    wimpt_safe(bbc_vdu(bbc_MoveDownOne));
    wimpt_safe(bbc_vdu(bbc_DefaultWindow));

#elif MS
    if(fastdraw)
        {
        sb_set_scrl(fastscreen, 0, first_line, Vcs.Vwidth, Vcs.Vheight);
        sb_scrl(fastscreen, 1, text_atts);
        }
    else
        my_int86(0x10, 6, 1, 7, 0, (uchar) first_line, 0,
                                            (uchar) paghyt, (uchar) pagwid);

#endif
}


/******************************************************
*                                                     *
* Draw i of one char to screen.                       *
*                                                     *
* To speed up drawing on PC use BIOS routines for     *
* multiple character draw and move cursor on i places *
*                                                     *
******************************************************/

extern void
wrchrep(uchar ch, coord i)
{
#if !defined(PRINT_OFF)
    if(sqobit)
        {
        while(i-- > 0)
            prnout(ch);
        return;
        }
#endif

#if MS
    if(i <= 0)
        return;

    if(fastdraw)
        {
        sb_wca(fastscreen, ch, text_atts, i);
#if 0
        xpos += i;
#endif
        }
    else
        {
        regs.h.ah = 9;
        regs.h.al = ch;
        regs.h.bh = (uchar) Vcs.Vpage;
        regs.h.bl = text_atts;
        regs.x.cx = i;
        int86(0x10, &regs, &regs);

        regs.h.ah = 2;
        regs.h.bh = (uchar) Vcs.Vpage;
        regs.h.dh = (uchar) ypos;
        regs.h.dl = (uchar) (xpos += i);
        int86(0x10, &regs, &regs);

        _settextposition(ypos+1, xpos+1);
        }

#elif ARTHUR || RISCOS
    while(i-- > 0)
        print_complain(bbc_vdu(ch));

#endif
}

/* end of mcdiff.c */
