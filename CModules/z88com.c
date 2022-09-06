/* z88com.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************
*                                                     *
* Package to communicate with the Z88 PCLINK II EPROM *
*                                                     *
* MRJC                                                *
* March 1988                                          *
*                                                     *
******************************************************/

/* standard header files */
#include "flags.h"

/* external header */
#if MS
#include "z88com.ext"
#else
#include "ext.z88com"
#endif

/* local header file */
#include "z88com.h"

#ifndef Z88_OFF

/*
function declarations
*/

extern intl binhex(intl);
extern intl hexbin(intl);
extern intl ibm_iso(intl);
extern intl iso_ibm(intl);
extern intl readack(void);
extern intl readchar(void);
extern intl readserial(void);
extern intl sendesc(char *);
extern intl sendpath(char *, char *, char *);
extern intl syncz88(void);
extern intl writeack(intl);
extern intl writeserial(intl);
extern intl z88_close(void);
extern intl z88_fclose(void);
extern char *z88_findfirst(char *, char);
extern char *z88_findnext(void);
extern intl z88_fopen(char *, char *);
extern intl z88_getbyte(void);
extern intl z88_geterr(void);
extern intl z88_open(intl);
extern intl z88_putbyte(intl);

/* internal functions */

static intl initport(intl);

/*************************************************
*                                                *
* IBM-ISO character table                        *
* ISO character first, and IBM equivalent second *
*                                                *
*************************************************/

#if MS
extern intl ibmiso[] =
{
    15,     181,    /* these conversions to allow reversibility */
    20,     191,
    21,     186,
    128,    214,
    129,    252,
    130,    235,
    131,    228,
    132,    229,
    133,    224,
    134,    231,
    135,    233,
    136,    236,
    137,    238,
    138,    234,
    139,    243,
    140,    242,
    141,    239,
    142,    199,
    143,    201,
    144,    216,
    145,    232,
    146,    209,
    147,    247,
    148,    249,
    149,    244,
    150,    251,
    151,    196,
    152,    255,
    153,    220,
    154,    223,
    155,    177,
    156,    178,
    157,    182,
    160,    226,
    166,    183,
    168,    198,
    173,    176,
    174,    187,
    175,    197,
    248,    188,
    253,    189,

    161,    173,    /* inverted ! */
    162,    155,    /* cents */
    163,    156,    /* pound */
    164,    15,     /* currency */
    165,    157,    /* yen */
    166,    124,    /* broken bar */
    167,    21,     /* paragraph */
    170,    166,    /* feminine ordinal */
    171,    174,    /* left angle quotation */
    172,    170,    /* not sign */
    176,    248,    /* degree sign */
    177,    241,    /* +/- */
    178,    253,    /* superscript 2 */
    181,    230,    /* mu */
    182,    20,     /* pilcrow */
    183,    250,    /* middle dot */
    186,    167,    /* masculine ordinal */
    187,    175,    /* right angle quotation */
    188,    172,    /* 1/4 */
    189,    171,    /* 1/2 */
    191,    168,    /* inverted question mark */
    196,    142,    /* A diaresis */
    197,    143,    /* A with ring */
    198,    146,    /* A diphthong E */
    199,    128,    /* C cedilla */
    201,    144,    /* E acute */
    209,    165,    /* N tilde */
    214,    153,    /* O diaresis */
    216,    237,    /* O stroke */
    220,    154,    /* U diaresis */
    223,    225,    /* German sharp S */
    224,    133,    /* a grave */
    225,    160,    /* a acute */
    226,    131,    /* a circumflex */
    228,    132,    /* a diaresis */
    229,    134,    /* a with ring */
    230,    145,    /* a diphthong e */
    231,    135,    /* c cedilla */
    232,    138,    /* e grave */
    233,    130,    /* e acute */
    234,    136,    /* e circumflex */
    235,    137,    /* e diaresis */
    236,    141,    /* i grave */
    237,    161,    /* i acute */
    238,    140,    /* i circumflex */
    239,    139,    /* i diaresis */
    241,    164,    /* n tilde */
    242,    149,    /* o grave */
    243,    162,    /* o acute */
    244,    147,    /* o circumflex */
    246,    148,    /* o diaresis */
    247,    246,    /* divide by */
    249,    151,    /* u grave */
    250,    163,    /* u acute */
    251,    150,    /* u circumflex */
    252,    129,    /* u diaresis */
    255,    152,    /* y diaresis */
};

#endif

/*
global variables
*/

/* Z88 is initialised */
static intl inited = 0;

#if MS
/* port to use */
static intl port;
static intl portbase;
#endif

/* handle to name array */
static char (*namary)[Z88NAMSIZ];
static intl namcountz;
static intl curnam;

/* file open code */
static intl fopencode;

#define OPEN_WRITE 1
#define OPEN_READ 2
#define OPEN_EOF 3

/* last error encountered */
static intl lasterr;

/*
macroed functions
*/

#define checkinited() (inited ? 0 : Z88_ERR_INIT)
#define seterr(err) (lasterr = (err))


/******************************
*                             *
* convert binary to hex digit *
*                             *
******************************/

extern intl
binhex(ch)
intl ch;
{
    return(((ch) > 9) ? ((ch) + 'A' - 10) : ((ch) + '0'));
}


/******************************
*                             *
* convert hex digit to binary *
*                             *
******************************/

extern intl
hexbin(ch)
intl ch;
{
    return(((ch) > '9') ? ((ch) - 'A' + 10) : ((ch) - '0'));
}


/*****************************************
*                                        *
* convert ibm character to iso character *
*                                        *
*****************************************/

#if MS

extern intl
ibm_iso(ch)
intl ch;
{
    intl i;

    if((ch < 32) || (ch > 127))
        {
        for(i = 0; i < sizeof(ibmiso)/sizeof(intl); i += 2)
            {
            if(ibmiso[i + 1] == ch)
                return(ibmiso[i]);
            }
        }

    return(ch);
}

#endif


/*****************************
*                            *
* initialise serial port     *
*                            *
* IBM:                       *
*     --in--                 *
*     COM1: port = 0         *
*     COM2: port = 1         *
*                            *
*****************************/

static intl
initport(portno)
intl portno;
{
#if MS

    union REGS inregs, outregs;

    /* store port details */
    port = portno;
    if(port)
        portbase = COM2_BASE;
    else
        portbase = COM1_BASE;

    /* initialise serial port */
    inregs.h.ah = SERIAL_INIT;
    inregs.h.al = Z88_INIT;
    inregs.x.dx = port;
    int86(SERIAL_IO, &inregs, &outregs);

#elif ARTHUR || RISCOS

    IGNOREPARM(portno);

    /* flush RS423 buffers */
    os_swi2(OS_Byte, 0x15, 1);
    os_swi2(OS_Byte, 0x15, 2);

    /* set default RS423 state */
    os_swi3(OS_Byte, 181, 1, 0);

    /* set RS423 baud rates */
    os_swi2(OS_Byte, 7, 7);
    os_swi2(OS_Byte, 8, 7);

    /* set RS423 configuration */
    os_swi3(OS_Byte, 156, 0x14, 0xE3);

    /* enable RS423 */
    os_swi2(OS_Byte, 2, 2);

#endif

    return(0);
}


/*****************************************
*                                        *
* convert iso character to ibm character *
*                                        *
*****************************************/

#if MS

extern intl
iso_ibm(ch)
intl ch;
{
    intl i;

    if((ch <32) || (ch > 127))
        {
        for(i = 0; i < sizeof(ibmiso)/sizeof(intl); i += 2)
            {
            if(ibmiso[i] == ch)
                return(ibmiso[i + 1]);
            }
        }

    return(ch);
}

#endif


/**************************************
*                                     *
* read a byte from the port and       *
* send the NULL acknowledge byte back *
*                                     *
**************************************/

extern intl
readack(void)
{
    intl err, ch;

    if((ch = readserial()) < 0)
        return(ch);

    /* send acknowledge */
    if((err = writeserial(0)) != 0)
        return(err);

    return(ch);
}


/*********************************
*                                *
* read a character from the port *
*                                *
* --out--                        *
* result +ve: character or       *
*             ESCAPE (if > 255)  *
* result -ve: error              *
*                                *
*********************************/

extern intl
readchar(void)
{
    intl ch, bb;

    /* read byte and check for escape */
    if((ch = readack()) < 0)
        return(ch);

    if(ch != ESC)
        return(ch);

    /* read escape argument */
    if((ch = readack()) < 0)
        return(ch);

    if(ch != 'B')
        return(ch + ESCBAS);

    /* read binary bytes */
    if((ch = readack()) < 0)
        return(ch);
    bb = hexbin(ch) << 4;
    if((ch = readack()) < 0)
        return(ch);
    return(bb | hexbin(ch));
}


/************************************
*                                   *
* read byte from serial port        *
* direct register access used since *
* BIOS calls don't always work      *
*                                   *
* --out--                           *
* -ve result is an error            *
*                                   *
************************************/

extern intl
readserial(void)
{
#if MS

    union REGS inregs, outregs;
    long starttime;

    /* wait for a byte to appear */
    starttime = time(NULL);
    do
        if(time(NULL) > starttime + TIME_RCV)
            return(Z88_ERR_BADCOM);
    while(!(inp(portbase + 5) & 1));

    return(inp(portbase));

#elif ARTHUR || RISCOS

    /* ARM registers */
    _kernel_swi_regs rs;
    intl ch;
    time_t start;

    /* check for a character in the RS423 buffer */
    start = time(NULL);
    do
        {
        rs.r[0] = 0x80;
        rs.r[1] = 254;
        _kernel_swi(OS_Byte, &rs, &rs);

        if(difftime(time(NULL), start) > TIME_RCV)
            return(Z88_ERR_BADCOM);
        }
    while((rs.r[2] == 0) && (rs.r[1] == 0));

    /* extract character from RS423 buffer */
    rs.r[0] = 0x91;
    rs.r[1] = 1;
    _kernel_swi(OS_Byte, &rs, &rs);
    ch = rs.r[2];

    return(ch);

#endif
}


/************************************
*                                   *
* send a string preceeded by ESCAPE *
*                                   *
************************************/

extern intl
sendesc(str)
char *str;
{
    intl err;

    if((err = writeack(ESC)) != 0)
        return(err);
    while(*str)
        if((err = writeack(*str++)) != 0)
            return(err);
    return(0);
}


/***********************
*                      *
* send path to the Z88 *
*                      *
***********************/

extern intl
sendpath(esc, path, esca)
char *esc, *path, *esca;
{
    intl err;

    if((err = sendesc(esc)) != 0)
        return(err);

    while(*path)
        if((err = writeack(*path++)) != 0)
            return(err);

    return(sendesc(esca));
}


/***********************
*                      *
* synchronise with Z88 *
*                      *
***********************/

extern intl
syncz88(void)
{
    intl err, i, ch;

    for(i = 0; i < 5; ++i)
        if((err = writeserial(5)) != 0)
            return(err);

    if((err = writeserial(6)) != 0)
        return(err);

    /* read response */
    for(i = 0; i < 10; ++i)
        {
        if((ch = readserial()) < 0)
            return(ch);
        if(ch == 6)
            break;
        }

    if(ch != 6)
        return(Z88_ERR_BADCOM);

    return(0);
}


/***********************************************
*                                              *
* write a byte to Z88 and wait for acknowledge *
*                                              *
***********************************************/

extern intl
writeack(ch)
intl ch;
{
    intl err;

    if((err = writeserial(ch)) != 0)
        return(err);

    if((ch = readserial()) < 0)
        return(ch);

    switch(ch)
        {
        case 0:
            return(0);
        case EZ88_NSP:
            return(Z88_ERR_Z_NSP);
        }

    return(0);
}


/****************************
*                           *
* write byte to serial port *
*                           *
****************************/

extern intl
writeserial(ch)
intl ch;
{
#if MS

    union REGS inregs, outregs;

    /* send byte */
    inregs.h.ah = SERIAL_WRITE;
    inregs.h.al = (uchar) ch;
    inregs.x.dx = port;
    int86(SERIAL_IO, &inregs, &outregs);

    /* check for an error */
    if(outregs.h.ah & 0x80)
        return(Z88_ERR_BADCOM);

#elif ARTHUR || RISCOS

    /* ARM registers */
    _kernel_swi_regs rs;
    intl oldstate;
    time_t start;

    /* check for space in RS423 buffer */
    start = time(NULL);
    do
        {
        rs.r[0] = 0x80;
        rs.r[1] = 253;
        _kernel_swi(OS_Byte, &rs, &rs);

        if(difftime(time(NULL), start) > TIME_XMT)
            return(Z88_ERR_BADCOM);
        }
    while((rs.r[2] == 0) && (rs.r[1] < 25));

    /* select RS423 output */
    rs.r[0] = 3;
    rs.r[1] = 0x57;
    _kernel_swi(OS_Byte, &rs, &rs);
    oldstate = rs.r[1];

    /* output character */
    os_swi1(OS_WriteC, ch);

    /* reselect old state */
    os_swi2(OS_Byte, 3, oldstate);

#endif

    return(0);
}


/********************************
*                               *
* send Z88 away after a session *
*                               *
********************************/

extern intl
z88_close(void)
{
    intl err, ch;

    /* check it has been initialised */
    if((err = checkinited()) != 0)
        return(seterr(err));

    /* free name memory */
    free(namary);
    inited = 0;

    /* tell Z88 to go away */
    if((err = syncz88()) != 0)
        return(seterr(err));
    if((err = sendesc("Q")) != 0)
        return(seterr(err));
    if((ch = readchar()) < 0)
        return(seterr(err));
    if(ch != (ESCBAS + 'Y'))
        return(seterr(Z88_ERR_BADCOM));

    return(0);
}


/********************
*                   *
* close file on Z88 *
*                   *
********************/

extern intl
z88_fclose(void
)
{
    intl err;

    /* check initialised */
    if((err = checkinited()) != 0)
        return(seterr(err));

    switch(fopencode)
        {
        /* use up the rest of the characters */
        case OPEN_READ:
            fopencode = 0;
            do
                if((err = z88_getbyte()) < 0)
                    return(err);
            while(err != (ESCBAS + 'Z'));
            return(0);

        case OPEN_EOF:
            fopencode = 0;
            return(0);

        case OPEN_WRITE:
            fopencode = 0;
            if((err = sendesc("E")) != 0)
                return(seterr(err));
            if((err = sendesc("Z")) != 0)
                return(seterr(err));
            return(0);

        default:
            fopencode = 0;
            return(seterr(Z88_ERR_OPEN));
        }

    return(0);
}


/********************************************
*                                           *
* return first name or directory for a path *
*                                           *
********************************************/

extern char *
z88_findfirst(path, type)
char *path, type;
{
    intl err, ch, namix, cc;
    char *c;

    /* check it has been initialised */
    if((err = checkinited()) != 0)
        {
        seterr(err);
        return(NULL);
        }

    /* synchronise with Z88 */
    if((err = syncz88()) != 0)
        {
        seterr(err);
        return(NULL);
        }

    /* ask z88 for the data */
    switch(type)
        {
        case Z88_DEVS:
            if((err = sendesc("H")) != 0)
                {
                seterr(err);
                return(NULL);
                }
            break;
        case Z88_DIRS:
            if((err = sendpath("D", path, "Z")) != 0)
                {
                seterr(err);
                return(NULL);
                }
            break;
        case Z88_FILES:
            if((err = sendpath("N", path, "Z")) != 0)
                {
                seterr(err);
                return(NULL);
                }
            break;
        }

    if((ch = readchar()) < 0)
        {
        seterr(err);
        return(NULL);
        }

    namix = 0;
    /* have we got a name ? */
    while(ch == (ESCBAS + 'N'))
        {
        c = &namary[namix][0];
        cc = 0;
        do
            {
            if((ch = readchar()) < 0)
                {
                seterr(ch);
                return(NULL);
                }
            if(ch < ESCBAS)
                {
                *c++ = (char) ch;
                if(++cc >= Z88NAMSIZ - 1)
                    {
                    seterr(Z88_ERR_BADCOM);
                    return(NULL);
                    }
                }
            }
        while(ch < ESCBAS);
        *c++ = '\0';

        if(namix++ >= MAXNAM)
            {
            seterr(Z88_ERR_TOOMANY);
            return(NULL);
            }
        }

    /* check we terminated correctly */
    if(ch != (ESCBAS + 'Z'))
        {
        seterr(Z88_ERR_BADCOM);
        return(NULL);
        }

    /* fudge around the devices to move RAM.- to the end */
    if((type == Z88_DEVS) && (namix >= 2))
        {
        char tstr[Z88NAMSIZ];

        strncpy(tstr, &namary[0][0], Z88NAMSIZ);
        strncpy(&namary[0][0], &namary[namix - 1][0], Z88NAMSIZ);
        strncpy(&namary[namix - 1][0], tstr, Z88NAMSIZ);
        }

    namcountz = namix;
    curnam = 0;
    return(z88_findnext());
}


/***************************
*                          *
* return next name in list *
*                          *
***************************/

extern char *
z88_findnext()
{
    intl err;

    /* check it has been initialised */
    if((err = checkinited()) != 0)
        {
        seterr(err);
        return(NULL);
        }

    if(curnam == namcountz)
        {
        curnam = namcountz = 0;
        return(NULL);
        }

    return(&namary[curnam++][0]);
}


/*************************
*                        *
* open a file on the Z88 *
*                        *
*************************/

extern intl
z88_fopen(name, code)
char *name, *code;
{
    intl err;

    /* check initialised */
    if((err = checkinited()) != 0)
        return(seterr(err));

    /* check for already open */
    if((fopencode == OPEN_READ) || (fopencode == OPEN_WRITE))
        return(seterr(Z88_ERR_OPEN));

    switch(toupper(*code))
        {
        /* open a file for reading */
        case 'R':
            if((err = syncz88()) != 0)
                return(seterr(err));
            if((err = sendpath("G", name, "Z")) != 0)
                return(seterr(err));

            fopencode = OPEN_READ;
            break;

        /* open a file for writing */
        case 'W':
            if((err = syncz88()) != 0)
                return(seterr(err));
            if((err = sendesc("S")) != 0)
                return(seterr(err));

            if((err = sendpath("N", name, "F")) != 0)
                return(seterr(err));
            fopencode = OPEN_WRITE;
            break;

        /* an error */
        default:
            return(seterr(Z88_ERR_ARG));
        }

    return(0);
}


/****************************************
*                                       *
* get a byte inside a file from the Z88 *
*                                       *
****************************************/

extern intl
z88_getbyte(void)
{
    intl err, ch;

    /* check initialised */
    if((err = checkinited()) != 0)
        return(seterr(err));

    /* check if we've had EOF */
    if(fopencode == OPEN_EOF)
        return(seterr(Z88_ERR_EOF));

    /* check open for reading */
    if(fopencode != OPEN_READ)
        return(seterr(Z88_ERR_OPEN));

    /* read character */
    if((ch = readchar()) < 0)
        return(seterr(ch));

    /* check for EOF */
    if(ch == (ESCBAS + 'Z'))
        {
        fopencode = OPEN_EOF;
        return(seterr(Z88_ERR_EOF));
        }

    /* check for ESCAPE code at wrong point */
    if(ch >= ESCBAS)
        return(seterr(Z88_ERR_BADCOM));

    #if MS
    return(iso_ibm(ch));
    #else
    return(ch);
    #endif
}

/************************************
*                                   *
* return error number of last error *
*                                   *
************************************/

extern intl
z88_geterr(void)
{
    return(lasterr);
}

/*****************************
*                            *
* open Z88 for communication *
*                            *
*****************************/

extern intl
z88_open(portno)
intl portno;
{
    intl err, ch;

    if(inited)
        return(0);

    namcountz = curnam = 0;
    fopencode = 0;

    /* initialise serial port */
    if((err = initport(portno)) != 0)
        return(seterr(err));

    if((err = syncz88()) != 0)
        return(seterr(err));
    if((err = sendesc("A")) != 0)
        return(seterr(err));

    if((ch = readchar()) < 0)
        return(ch);

    if(ch != (ESCBAS + 'Y'))
        return(seterr(Z88_ERR_BADCOM));

    /* allocate name array */
    if((namary = malloc(MAXNAM * Z88NAMSIZ * sizeof(char))) == NULL)
        return(seterr(Z88_ERR_MEM));

    inited = 1;
    return(seterr(0));
}


/***************************************
*                                      *
* send a byte inside a file to the Z88 *
*                                      *
***************************************/

extern intl
z88_putbyte(ch)
intl ch;
{
    intl err;
    char tstr[4];

    /* check initialised */
    if((err = checkinited()) != 0)
        return(seterr(err));

    /* check open for writing */
    if(fopencode != OPEN_WRITE)
        return(seterr(Z88_ERR_OPEN));

    /* send character */
    #if MS
    ch = ibm_iso(ch);
    #endif

    if((ch > 0x1F) &&
        (ch < 0x80) ||
        (ch == 0x9) ||
        (ch == 0xA) ||
        (ch == 0xD))
        {
        if((err = writeack(ch)) != 0)
            return(seterr(err));
        }
    else
        {
        tstr[0] = 'B';
        tstr[1] = (uchar) binhex((ch >> 4) & 0xF);
        tstr[2] = (uchar) binhex(ch & 0xF);
        tstr[3] = '\0';
        if((err = sendesc(tstr)) != 0)
            return(seterr(err));
        }

    return(0);
}

#endif

/* end of z88com.c */
