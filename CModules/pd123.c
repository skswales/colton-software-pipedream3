/* pd123.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/**********************************************
*                                             *
* set of routines to convert between          *
* PipeDream files and Lotus 123 (rel 2) files *
*                                             *
* MRJC                                        *
* February 1988                               *
*                                             *
**********************************************/

/* standard header files */
#include "flags.h"

#ifndef LOTUS_OFF

/* external header */
#if ARTHUR || RISCOS
#include "ext.pd123"
    #if RISCOS
    #include "flex.h"
    #endif
#else
#include "pd123.ext"
#endif

/* local header file */
#include "pd123.h"

#define TYPE_MATCH 0
#define WIDTH_MATCH 1
#define NEXT_ROW 2

#define P_DATE 1
#define P_TEXT 2

#define MAXSTACK 100

/* exported functions */
extern intl flookup(uchar *);
extern intl foutc(intl, FILE *);
extern intl ichlotus(intl);
extern intl olookup(uchar *);
extern intl readlotus(FILE *, FILE *, intl, intl (*)(word32));
extern intl searchdefo(uchar *);

/* exported variables */
extern intl curpd;                 /* current PipeDream level */
extern intl errexp;                /* had error in expression */
extern FILE *fin;
extern FILE *fout;
extern intl maxcol;
extern intl poorfunc;              /* had a function that would't convert */

/* internal functions */
static intl addplusn(intl, intl);
static intl checkdate(intl *, long);
static intl chksym(void);
static intl compopr(const void *arg1, const void *arg2);
static uchar *convconst(intl, uchar huge *, intl, intl);
static void convslr(uchar *, intl *, uchar huge *, intl, intl, intl *, intl *);
static intl fachoose(intl *, uchar *, intl *, intl *);
static intl fadate(intl *, uchar *, intl *, intl *);
static intl fadateval(intl *, uchar *, intl *, intl *);
static intl faindex(intl *, uchar *, intl *, intl *);
static uchar huge *findrec(intl, intl, intl, intl);
static intl foutcr(FILE *);
static intl fptostr(uchar *, double);
static void freestk(void);
static intl lotusich(intl);
static intl outcon(uchar *);
static intl outdecplc(int);
static intl outstr(uchar *);
static intl readrange(void);
static double readdouble(uchar huge *);
static uword16 readuword16(uchar huge *);
static word16 readword16(uchar huge *);
static intl scnsym(void);
static intl startopt(optp);
static intl wrcalcmode(optp);
static intl wrcalcord(optp);
static intl wrdecplc(optp);
static intl wrheadfoot(optp);
static intl writedate(long);
static intl writeexp(uchar huge *, intl, intl);
static intl writeformat(uchar huge *, intl *);
static intl writeoptions(void);
static intl writepcol(intl, intl, intl);
static intl wrmar(optp, uword16);
static intl wrmargins(optp);
static intl wrminbrk(optp);
static intl wrsetmar(optp);
static intl wrtextn(optp);
static intl wrthousands(optp);
static intl wrwrapmo(optp);
static intl xtos(uchar *, intl);

/*******************************************
*                                          *
* table of characters for conversion       *
* between Z88 ISO character set and Lotus  *
* LICS character set                       *
*                                          *
* the ISO character is first, followed by  *
* the corresponding LICS character         *
*                                          *
*******************************************/

#if !MS

static intl isolics[] =
{
    129,    160,    /* these conversions are for reversibility */
    131,    164,
    154,    255,
    175,    174,
    184,    180,
    253,    247,

    160,    154,    /* hard space */
    164,    168,    /* currency symbol */
    168,    131,    /* diaresis */
    174,    184,    /* trademark */
    180,    129,    /* acute accent */
    247,    175,    /* divide by */
    255,    253,    /* y diaresis */
};

#else

/******************************************
*                                         *
* table of characters for conversion      *
* between IBM PC character set and Lotus  *
* LICS character set                      *
*                                         *
* the IBM character is first, followed by *
* the corresponding LICS character        *
*                                         *
******************************************/

static intl ibmlics[] =
{
    170,    173,    /* these conversions are for reversibility */
    176,    21,
    177,    168,
    178,    248,
    181,    20,
    182,    151,
    183,    227,
    186,    154,
    187,    158,
    188,    157,
    189,    156,
    190,    172,
    191,    155,
    196,    142,
    197,    143,
    198,    145,
    199,    128,
    201,    144,
    209,    152,
    214,    149,
    216,    164,
    220,    150,
    223,    159,
    224,    133,
    226,    131,
    228,    132,
    229,    134,
    231,    135,
    232,    138,
    233,    130,
    234,    136,
    235,    137,
    236,    141,
    238,    140,
    239,    139,
    244,    146,
    249,    148,
    251,    147,
    252,    129,

    128,    199,    /* C cedilla */
    129,    252,    /* u diaresis */
    130,    233,    /* e acute */
    131,    226,    /* a circumflex */
    132,    228,    /* a diaresis */
    133,    224,    /* a grave */
    134,    229,    /* a with circle */
    135,    231,    /* c cedilla */
    136,    234,    /* e circumflex */
    137,    235,    /* e diaresis */
    138,    232,    /* e grave */
    139,    239,    /* i diaresis */
    140,    238,    /* i circumflex */
    141,    236,    /* i grave */
    142,    196,    /* A diaresis */
    143,    197,    /* A with ring */
    144,    201,    /* E acute */
    145,    230,    /* a diphthong e */
    146,    198,    /* A diphthong E */
    147,    244,    /* o circumflex */
    148,    246,    /* o diaresis */
    149,    242,    /* o grave */
    150,    251,    /* u circumflex */
    151,    249,    /* u grave */
    152,    253,    /* y diaresis */
    153,    214,    /* O diaresis */
    154,    220,    /* U diaresis */
    155,    162,    /* cents */
    156,    163,    /* pounds */
    157,    165,    /* yen */
    158,    166,    /* pesetas */
    159,    160,    /* guilders */
    20,     182,    /* paragraph */
    160,    225,    /* a acute */
    161,    237,    /* i acute */
    162,    243,    /* o acute */
    163,    250,    /* u acute */
    164,    241,    /* n tilde */
    165,    209,    /* N tilde */
    166,    170,    /* feminine ordinal */
    167,    186,    /* masculine ordinal */
    168,    191,    /* inverted ? */
    171,    189,    /* 1/2 */
    172,    188,    /* 1/4 */
    173,    161,    /* inverted ! */
    174,    171,    /* left angle quotation */
    175,    187,    /* right angle quotation */
    21,     167,    /* section */
    225,    223,    /* German sharp s */
    227,    173,    /* pi */
    230,    181,    /* mu */
    237,    216,    /* O with stroke */
    241,    177,    /* +/- */
    242,    174,    /* >= */
    243,    190,    /* <= */
    246,    175,    /* divide by */
    248,    176,    /* degree sign */
    250,    183,    /* middle dot */
    253,    178,    /* superscript 2 */
};

#endif

/*****************************************
*                                        *
* table of PipeDream options             *
* and LOTUS equivalents                  *
*                                        *
* they are ordered as they appear in the *
* 123 file to avoid a lot of searching   *
*                                        *
*****************************************/

static struct optdef optqv[] =
{
    (uchar *) "AM",  L_CALCMODE,  0,  0xFF,   wrcalcmode,
    (uchar *) "RC", L_CALCORDER,  0,     0,   wrcalcord,
    (uchar *) "DP",   L_WINDOW1,  4,     2,   wrdecplc,
    (uchar *) "MB",   L_WINDOW1,  4,     0,   wrminbrk,
    (uchar *) "TH",   L_WINDOW1,  4,     0,   wrthousands,
    (uchar *) "FO",    L_FOOTER,  0,     0,   wrheadfoot,
    (uchar *) "HE",    L_HEADER,  0,     0,   wrheadfoot,
    (uchar *) "BM",   L_MARGINS,  8,     8,   wrmargins,
    (uchar *) "LM",   L_MARGINS,  0,     0,   wrmargins,
    (uchar *) "PL",   L_MARGINS,  4,    66,   wrmargins,
    (uchar *) "TM",   L_MARGINS,  6,     0,   wrmargins,
    (uchar *) "FM",           0,  0,     2,   wrsetmar,
    (uchar *) "HM",           0,  0,     2,   wrsetmar,
    (uchar *) "WR",           0,  0,   'N',   wrwrapmo,
    (uchar *) "TN",           0,  0,   'N',   wrtextn,
};

/*****************************************************
*                                                    *
* table of lotus operators and PipeDream equivalents *
*                                                    *
*****************************************************/

struct oprdef opreqv[] =
{
    LF_CONST,   LO_CONST,  0,      0, (uchar *) "",            0,
    LF_SLR,     LO_CONST,  0,      0, (uchar *) "",            0,
    LF_RANGE,   LO_CONST,  0,      0, (uchar *) "",            0,
    LF_END,     LO_END,    0,      0, (uchar *) "",            0,
    LF_BRACKETS,LO_BRACKETS,0,     0, (uchar *) "",            0,
    LF_INTEGER, LO_CONST,  0,      0, (uchar *) "",            0,
    LF_STRING,  LO_CONST,  0,      0, (uchar *) "",            0,
    LF_UMINUS,  LO_UNARY,  1,  PD_VP, (uchar *) "-",           0,
    LF_PLUS,    LO_BINARY, 2,  PD_VP, (uchar *) "+",           0,
    LF_MINUS,   LO_BINARY, 2,  PD_VP, (uchar *) "-",           0,
    LF_TIMES,   LO_BINARY, 2,  PD_VP, (uchar *) "*",           0,
    LF_DIVIDE,  LO_BINARY, 2,  PD_VP, (uchar *) "/",           0,
    LF_POWER,   LO_BINARY, 2,  PD_VP, (uchar *) "^",           0,
    LF_EQUALS,  LO_BINARY, 2,  PD_VP, (uchar *) "=",           0,
    LF_NOTEQUAL,LO_BINARY, 2,  PD_VP, (uchar *) "<>",          0,
    LF_LTEQUAL, LO_BINARY, 2,  PD_VP, (uchar *) "<=",          0,
    LF_GTEQUAL, LO_BINARY, 2,  PD_VP, (uchar *) ">=",          0,
    LF_LT,      LO_BINARY, 2,  PD_VP, (uchar *) "<",           0,
    LF_GT,      LO_BINARY, 2,  PD_VP, (uchar *) ">",           0,
    LF_AND,     LO_BINARY, 2,  PD_VP, (uchar *) "&",           0,
    LF_OR,      LO_BINARY, 2,  PD_VP, (uchar *) "|",           0,
    LF_NOT,     LO_UNARY,  1,  PD_VP, (uchar *) "!",           0,
    LF_UPLUS,   LO_UNARY,  1,  PD_VP, (uchar *) "+",           0,
    LF_NA,      LO_FUNC,   0,      0, (uchar *) "na",          0,
    LF_ERR,     LO_FUNC,   0,      0, (uchar *) "err",         0,
    LF_ABS,     LO_FUNC,   1,  PD_VP, (uchar *) "abs",         0,
    LF_INT,     LO_FUNC,   1,  PD_VP, (uchar *) "int",         0,
    LF_SQRT,    LO_FUNC,   1,  PD_VP, (uchar *) "sqr",         0,
    LF_LOG,     LO_FUNC,   1,  PD_VP, (uchar *) "log",         0,
    LF_LN,      LO_FUNC,   1,  PD_VP, (uchar *) "ln",          0,
    LF_PI,      LO_FUNC,   0,  PD_VP, (uchar *) "pi",          0,
    LF_SIN,     LO_FUNC,   1,  PD_VP, (uchar *) "sin",         0,
    LF_COS,     LO_FUNC,   1,  PD_VP, (uchar *) "cos",         0,
    LF_TAN,     LO_FUNC,   1,  PD_VP, (uchar *) "tan",         0,
    LF_ATAN2,   LO_FUNC,   1,   PD_3, (uchar *) "atan2",       0,
    LF_ATAN,    LO_FUNC,   1,  PD_VP, (uchar *) "atn",         0,
    LF_ASIN,    LO_FUNC,   1,  PD_VP, (uchar *) "asn",         0,
    LF_ACOS,    LO_FUNC,   1,  PD_VP, (uchar *) "acs",         0,
    LF_EXP,     LO_FUNC,   1,  PD_VP, (uchar *) "exp",         0,
    LF_MOD,     LO_FUNC,   2,  PD_PC, (uchar *) "mod",         0,
    LF_CHOOSE,  LO_FUNC,  -1,  PD_VP, (uchar *) "choose",      FU_CHOOSE,
    LF_ISNA,    LO_FUNC,   1,      0, (uchar *) "isna",        0,
    LF_ISERR,   LO_FUNC,   1,      0, (uchar *) "iserr",       0,
    LF_FALSE,   LO_FUNC,   0,  PD_VP, (uchar *) "0",           0,
    LF_TRUE,    LO_FUNC,   1,  PD_VP, (uchar *) "1",           0,
    LF_RAND,    LO_FUNC,   0,   PD_3, (uchar *) "rand",        0,
    LF_DATE,    LO_FUNC,   3,  PD_VP, (uchar *) "datef",       FU_DATE,
    LF_TODAY,   LO_FUNC,   0,  PD_PC, (uchar *) "date",        0,
    LF_PMT,     LO_FUNC,   3,  PD_PC, (uchar *) "pmt",         0,
    LF_PV,      LO_FUNC,   3,  PD_PC, (uchar *) "pv",          0,
    LF_FV,      LO_FUNC,   3,  PD_PC, (uchar *) "fv",          0,
    LF_IF,      LO_FUNC,   3,  PD_VP, (uchar *) "if",          0,
    LF_DAY,     LO_FUNC,   1,  PD_VP, (uchar *) "day",         0,
    LF_MONTH,   LO_FUNC,   1,  PD_VP, (uchar *) "month",       0,
    LF_YEAR,    LO_FUNC,   1,  PD_VP, (uchar *) "year",        0,
    LF_ROUND,   LO_FUNC,   2,   PD_3, (uchar *) "round",       0,
    LF_TIME,    LO_FUNC,   3,      0, (uchar *) "time",        0,
    LF_HOUR,    LO_FUNC,   1,      0, (uchar *) "hour",        0,
    LF_MINUTE,  LO_FUNC,   1,      0, (uchar *) "minute",      0,
    LF_SECOND,  LO_FUNC,   1,      0, (uchar *) "second",      0,
    LF_ISN,     LO_FUNC,   1,      0, (uchar *) "isnumber",    0,
    LF_ISS,     LO_FUNC,   1,      0, (uchar *) "isstring",    0,
    LF_LENGTH,  LO_FUNC,   1,      0, (uchar *) "length",      0,
    LF_VALUE,   LO_FUNC,   1,      0, (uchar *) "value",       0,
    LF_FIXED,   LO_FUNC,   1,      0, (uchar *) "fixed",       0,
    LF_MID,     LO_FUNC,   3,      0, (uchar *) "mid",         0,
    LF_CHR,     LO_FUNC,   1,      0, (uchar *) "char",        0,
    LF_ASCII,   LO_FUNC,   1,      0, (uchar *) "code",        0,
    LF_FIND,    LO_FUNC,   3,      0, (uchar *) "find",        0,
    LF_DATEVALUE,LO_FUNC,  1,  PD_VP, (uchar *) "datevalue",   FU_DATEVAL,
    LF_TIMEVALUE,LO_FUNC,  1,      0, (uchar *) "timevalue",   0,
    LF_CELLPOINTER,LO_FUNC,1,      0, (uchar *) "cellpointer", 0,
    LF_SUM,     LO_FUNC,  -1,  PD_VP, (uchar *) "sum",         0,
    LF_AVG,     LO_FUNC,  -1,  PD_PC, (uchar *) "avg",         0,
    LF_CNT,     LO_FUNC,  -1,  PD_VP, (uchar *) "count",       0,
    LF_MIN,     LO_FUNC,  -1,  PD_VP, (uchar *) "min",         0,
    LF_MAX,     LO_FUNC,  -1,  PD_VP, (uchar *) "max",         0,
    LF_VLOOKUP, LO_FUNC,   3,  PD_PC, (uchar *) "vlookup",     0,
    LF_NPV,     LO_FUNC,   2,  PD_PC, (uchar *) "npv",         0,
    LF_VAR,     LO_FUNC,  -1,  PD_PC, (uchar *) "var",         0,
    LF_STD,     LO_FUNC,  -1,  PD_PC, (uchar *) "std",         0,
    LF_IRR,     LO_FUNC,   2,  PD_PC, (uchar *) "irr",         0,
    LF_HLOOKUP, LO_FUNC,   3,  PD_PC, (uchar *) "hlookup",     0,
    LF_DSUM,    LO_FUNC,   3,      0, (uchar *) "dsum",        0,
    LF_DAVG,    LO_FUNC,   3,      0, (uchar *) "davg",        0,
    LF_DCNT,    LO_FUNC,   3,      0, (uchar *) "dsum",        0,
    LF_DMIN,    LO_FUNC,   3,      0, (uchar *) "dmin",        0,
    LF_DMAX,    LO_FUNC,   3,      0, (uchar *) "dmax",        0,
    LF_DVAR,    LO_FUNC,   3,      0, (uchar *) "dvar",        0,
    LF_DSTD,    LO_FUNC,   3,      0, (uchar *) "dstd",        0,
    LF_INDEX,   LO_FUNC,   3,  PD_VP, (uchar *) "index",       FU_INDEX,
    LF_COLS,    LO_FUNC,   1,      0, (uchar *) "cols",        0,
    LF_ROWS,    LO_FUNC,   1,      0, (uchar *) "rows",        0,
    LF_REPEAT,  LO_FUNC,   2,      0, (uchar *) "repeat",      0,
    LF_UPPER,   LO_FUNC,   1,      0, (uchar *) "upper",       0,
    LF_LOWER,   LO_FUNC,   1,      0, (uchar *) "lower",       0,
    LF_LEFT,    LO_FUNC,   2,      0, (uchar *) "left",        0,
    LF_RIGHT,   LO_FUNC,   2,      0, (uchar *) "right",       0,
    LF_REPLACE, LO_FUNC,   4,      0, (uchar *) "replace",     0,
    LF_PROPER,  LO_FUNC,   1,      0, (uchar *) "proper",      0,
    LF_CELL,    LO_FUNC,   2,      0, (uchar *) "cell",        0,
    LF_TRIM,    LO_FUNC,   1,      0, (uchar *) "trim",        0,
    LF_CLEAN,   LO_FUNC,   1,      0, (uchar *) "clean",       0,
    LF_S,       LO_FUNC,   1,      0, (uchar *) "s",           0,
    LF_V,       LO_FUNC,   1,      0, (uchar *) "v",           0,
    LF_STREQ,   LO_FUNC,   2,      0, (uchar *) "exact",       0,
    LF_CALL,    LO_FUNC,   1,      0, (uchar *) "call",        0,
    LF_INDIRECT,LO_FUNC,   1,      0, (uchar *) "indirect",    0,
    LF_RATE,    LO_FUNC,   3,  PD_PC, (uchar *) "rate",        0,
    LF_TERM,    LO_FUNC,   3,  PD_PC, (uchar *) "term",        0,
    LF_CTERM,   LO_FUNC,   3,  PD_PC, (uchar *) "cterm",       0,
    LF_SLN,     LO_FUNC,   3,  PD_PC, (uchar *) "sln",         0,
    LF_SOY,     LO_FUNC,   4,  PD_PC, (uchar *) "syd",         0,
    LF_DDB,     LO_FUNC,   4,  PD_PC, (uchar *) "ddb",         0,
};

/******************************
*                             *
* table of function addresses *
* for argument modifying      *
*                             *
******************************/

static intl (*argfuddle[])(intl *, uchar *, intl *, intl *) =
{
    fachoose,
    fadate,
    faindex,
    fadateval,
};

/* input and output files */
FILE *fin, *fout;

intl maxcol;

/* lotus file headers */
static uchar lfhead[] = "\x0\x0\x2\x0";
static uchar lfh123[] = "\x4\x4";
static uchar lfh123_2[] = "\x6\x4";

/* Global parameters for lotus file */
static intl sc, ec, sr, er;
static intl defcwid;
static intl foundeof;

/* array which contains the lotus file */
static uchar huge *lotusf;

/* current position in lotus file */
static uchar huge *curpos;

/* RPN recogniser variables */
static uchar huge *termp;          /* scanner index */
static intl cursym;                /* current symbol */
static uchar *argstk[MAXSTACK];    /* stack of arguments */
static intl argsp;                 /* argument stack pointer */
static oprp curopr;                /* current symbol structure */

/* days in the month */
static days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* PD level */
intl errexp;                /* had error in expression */
intl poorfunc;              /* had a function that would't convert */
intl curpd;                 /* current PipeDream level */

/************************************************************************
*                                                                       *
* main loop for Lotus to PipeDream                                      *
* this main procedure is compiled if the constant                       *
* UTIL_LTP is defined:                                                  *
*                                                                       *
* UTIL_LTP creates stand-alone utility for Lotus to PipeDream           *
* UTIL_PTL creates stand-alone utility for PipeDream to Lotus           *
* INT_LPTPL creates internal code for converter using function lp_lptpl *
*                                                                       *
************************************************************************/

#ifdef UTIL_LTP
int main(argc, argv)
int argc;
char **argv;
{
    unsigned long length;
    intl col, err;
    uchar huge *op;

    /* banner */
    printf("Lotus 123 to PipeDream converter\nColton Software 1988\n");

    /* argument checking */
    if(argc < 3)
        {
        printf("Not enough arguments\n");
        exit(0);
        }

    if((fin = fopen(*++argv, "rb")) == NULL)
        {
        printf("Can't open %s\n", *argv);
        exit(0);
        }

    if((fout = fopen(*++argv, "wb")) == NULL)
        {
        printf("Can't open %s\n", *argv);
        exit(0);
        }

    #if MS
    length = filelength(fileno(fin));
    #else
    if(fseek(fin, 0l, SEEK_END))
        return(PD123_ERR_FILE);
    length = ftell(fin);
    if(fseek(fin, 0l, SEEK_SET))
        return(PD123_ERR_FILE);
    #endif

    #ifdef MS_HUGE
    if(!(lotusf = halloc(length + 1, sizeof(uchar))))
    #else
    if(!(lotusf = malloc((unsigned int) length)))
    #endif
        {
        printf("Not enough memory for LOTUS file\n");
        exit(0);
        }

    /* read in LOTUS file */
    #ifdef MS_HUGE
    op = lotusf;
    while(!feof(fin))
        *op++ = (uchar) getc(fin);
    #else
    fread(lotusf, 1, (uword16) length, fin);
    #endif
    fclose(fin);

    /* initialise variables */
    curpos = NULL;
    errexp = 0;
    poorfunc = 0;
    curpd = PD_Z88;

    /* read in range of 123 file */
    if(!(err = readrange()))
        {
        /* write out options to PD file */
        err = writeoptions();

        /* write out columns */
        if(!err)
            {
            for(col = sc; (col <= ec) && !(foundeof && (col > maxcol)); ++col)
                {
                uchar scol[10];
                intl cr;

                if(err = writepcol(col, sr, er))
                    break;
                cr = xtos(scol, col);
                scol[cr] = '\0';
                printf("\rColumn: %s", scol);
                }
            printf("\n");
            }
        }

    switch(err)
        {
        case PD123_ERR_MEM:
            printf("Out of memory\n");
            break;
        case PD123_ERR_FILE:
            perror("File error");
            break;
        case PD123_ERR_BADFILE:
            printf("Bad Lotus file\n");
            break;
        default:
            break;
        }

    fclose(fout);
    #ifdef MS_HUGE
    hfree(lotusf);
    #else
    free(lotusf);
    #endif

    if(errexp)
         printf("%d bad expressions found\n", errexp);
    if(poorfunc)
        printf("Unable to convert %d functions\n", poorfunc);
    if(!err)
        printf("Conversion complete\n");
    else
        remove(*argv);

    printf("Press a key to continue");
    getch();
    return(0);
}
#endif


/***************************************************************
*                                                              *
* function to read a lotus file called from elsewhere          *
*                                                              *
* --in--                                                       *
* fin and fout are file pointers to open channels              *
* level specifies the level of the PD file                     *
* routine is the address of a routine to call to show activity *
* this returns non-zero if an abort is required                *
*                                                              *
***************************************************************/

#ifdef INT_LPTPL
extern intl
readlotus(inf, outf, level, routine)
FILE *inf, *outf;
intl level;
intl (*routine)(word32);
{
    unsigned long length;
    intl col, err;

    #ifdef MS_HUGE
    uchar huge *op;
    #endif

    /* set level of conversion */
    curpd = level;

    /* save file pointers */
    fin = inf;
    fout = outf;

    #if MS
    length = filelength(fileno(fin));
    #else
    if(fseek(fin, 0l, SEEK_END))
        return(PD123_ERR_FILE);
    length = ftell(fin);
    if(fseek(fin, 0l, SEEK_SET))
        return(PD123_ERR_FILE);
    #endif

    #ifdef MS_HUGE
    if((lotusf = halloc(length + 1, sizeof(uchar))) == NULL)
        return(PD123_ERR_MEM);
    #else
    #if RISCOS
    if(!flex_alloc((flex_ptr) &lotusf, (intl) length))
        return(PD123_ERR_MEM);
    #else
    if((lotusf = malloc((unsigned int) length)) == NULL)
        return(PD123_ERR_MEM);
    #endif
    #endif

    /* read in LOTUS file */
    #ifdef MS_HUGE
    op = lotusf;
    while(!feof(fin))
        *op++ = (uchar) getc(fin);
    #else
        #if MS
        fread(lotusf, 1, (uword16) length, fin);
        #else
        fread(lotusf, 1, (size_t) length, fin);
        #endif
    #endif

    /* initialise variables */
    curpos = NULL;
    errexp = 0;
    poorfunc = 0;

    /* read in range of 123 file */
    if((err = readrange()) == 0)
        {
        /* write out options to PD file */
        err = writeoptions();

        /* write out columns */
        if(!err)
            for(col = sc; (col <= ec) && !(foundeof && (col > maxcol)); ++col)
                {
                if((err = writepcol(col, sr, er)) != 0)
                    break;

                /* call activity routine and check for abort */
                if(routine)
                    if((*routine)((word32) ((col - sc) * 100 / (maxcol + 1))))
                        break;
                }
        }

    #ifdef MS_HUGE
    hfree(lotusf);
    #else
    #if RISCOS
    flex_free((flex_ptr) &lotusf);
    #else
    free(lotusf);
    #endif
    #endif

    if(!err && (errexp || poorfunc))
        err = PD123_ERR_EXP;

    return(err);
}
#endif


/**************************************
*                                     *
* function to add "+n" to an argument *
*                                     *
**************************************/

static intl
addplusn(arg, n)
intl arg, n;
{
    uchar nstr[10];
    uchar *tstr, *cele;
    intl lcele, ln;

    /* add +1 to argument */
    cele = argstk[argsp - arg];
    lcele = strlen((char *) cele);
    ln = sprintf((char *) nstr, "+%d", n);
    if((tstr = malloc(lcele + ln + 1)) == NULL)
        return(PD123_ERR_MEM);
    strcpy((char *) tstr, (char *) cele);
    strncpy((char *) (tstr + lcele), (char *) nstr, ln);
    tstr[lcele + ln] = '\0';
    argstk[argsp - arg] = tstr;
    free(cele);
    return(0);
}


/***********************************************
*                                              *
* handle special formats for dates and things  *
*                                              *
* if no more output for a slot is required,    *
* specflg is set to zero                       *
*                                              *
***********************************************/

static intl
checkdate(specflg, value)
intl *specflg;
long value;
{
    switch(*specflg)
        {
        case 0:
            if(fprintf(fout, "%%V%%") < 0)
                return(PD123_ERR_FILE);
            break;
        case P_TEXT:
            break;
        case P_DATE:
            *specflg = 0;
            if(fprintf(fout, "%%V%%") < 0)
                return(PD123_ERR_FILE);
            return(writedate(value));
        }
    *specflg = -1;
    return(0);
}


/*******************
*                  *
* symbol lookahead *
*                  *
*******************/

static intl
chksym(void)
{
    cursym = *termp;
    curopr = (struct oprdef *) bsearch((uchar *) &cursym, (uchar *) opreqv,
                                       sizeof(opreqv)/sizeof(struct oprdef),
                                       sizeof(struct oprdef), compopr);
    return(cursym);
}


/********************
*                   *
* compare operators *
*                   *
********************/

static int
compopr(const void *arg1, const void *arg2)
{
    uchar ch1 = *((uchar *) arg1);
    uchar ch2 = *((uchar *) arg2);

    if(ch1 < ch2)
        return(-1);

    if(ch1 == ch2)
        return(0);

    return(1);
}


/***************************************
*                                      *
* convert current constant to a string *
*                                      *
* string is allocated with malloc      *
* and must be freed when not needed    *
*                                      *
***************************************/

static uchar *
convconst(opr, arg, col, row)
intl opr;
uchar huge *arg;
intl col, row;
{
    intl reslen;
    uchar resstr[256], *res;
    uchar *c;

    switch(opr)
        {
        case LF_CONST:
            reslen = fptostr(resstr, readdouble(arg));
            break;

        case LF_SLR:
            reslen = 0;
            convslr(resstr, &reslen, arg, col, row, NULL, NULL);
            break;

        case LF_RANGE:
            {
            intl cc = 0, cr = 0;

            reslen = 0;
            convslr(resstr, &reslen, arg, col, row, &cc, &cr);
            convslr(resstr, &reslen, arg + 4, col, row, &cc, &cr);
            break;
            }

        case LF_INTEGER:
            {
            intl ival;

            ival = (intl) readword16(arg);
            reslen = sprintf((char *) resstr, "%d", ival);
            break;
            }

        case LF_STRING:
            c = resstr;
            *c++ = '"';
            for(reslen = 0; *arg; reslen++)
                *c++ = (uchar) lotusich((intl) *arg++);
            *c++= '"';
            reslen += 2;
            break;
        }

    resstr[reslen] = '\0';
    if((res = malloc(reslen + 1)) != NULL)
        strncpy((char *) res, (char *) resstr, reslen + 1);
    return(res);
}


/**************
*             *
* convert SLR *
*             *
**************/

static void
convslr(resstr, reslen, arg, col, row, cc, cr)
uchar *resstr, huge *arg;
intl *reslen, col, row, *cc, *cr;
{
    intl cdol = 0, rdol = 0, tlen = 0;
    uword16 c, r;
    uchar tstr[50];

    c = readuword16(arg);
    arg += 2;
    if(!(c & 0x8000))
        cdol = 1;
    else
        c = (c + col) & 0xFF;

    r = readuword16(arg);
    arg += 2;
    if(!(r & 0x8000))
        rdol = 1;
    else
        r = (r + row) & 0x3FFF;
    ++r;

    if(cdol)
        {
        *(tstr + tlen) = '$';
        ++tlen;
        }
    tlen += xtos(tstr + tlen, c);

    if(rdol)
        *(tstr + tlen++) = '$';
    tlen += sprintf((char *) (tstr + tlen), "%d", r);

    /* save actual addresses for comparison */
    if(!*reslen && cc)
        *cc = c;
    if(!*reslen && cr)
        *cr = r;

    /* if on second ref., check for highest and swap */
    if(*reslen && ((cc && (c < *cc)) || (cr && (r < *cr))))
        {
        memmove(resstr + tlen, resstr, *reslen);
        strncpy((char *) resstr, (char *) tstr, tlen);
        }
    else
        {
        /* add second string */
        strncpy((char *) (resstr + *reslen), (char *) tstr, tlen);
        }
    *reslen += tlen;
}


/*********************************
*                                *
* fuddle the arguments to choose *
*                                *
*********************************/

static intl
fachoose(narg, argsep, nobrk, noname)
intl *narg;
uchar *argsep;
intl *nobrk;
intl *noname;
{
    IGNOREPARM(argsep);
    IGNOREPARM(nobrk);
    IGNOREPARM(noname);

    /* add +1 to first argument */
    return(addplusn(*narg, 1));
}


/********************************************
*                                           *
* fuddle the arguments to the date function *
*                                           *
********************************************/

static intl
fadate(narg, argsep, nobrk, noname)
intl *narg;
uchar *argsep;
intl *nobrk;
intl *noname;
{
    uchar *temp;
    intl tmp, i;

    IGNOREPARM(narg);

    /* check that we have three literal numbers */
    for(i = 0; i < 3; ++i)
        if(sscanf((char *) argstk[argsp - i - 1], "%d", &tmp) != 1)
            return(0);

    /* swap year and day */
    temp = argstk[argsp - 1];
    argstk[argsp - 1] = argstk[argsp - 3];
    argstk[argsp - 3] = temp;

    /* return dot separator */
    *noname = 1;
    *nobrk = 1;
    *argsep = '.';
    return(0);
}


/***********************************************
*                                              *
* fuddle the arguments to the dateval function *
*                                              *
***********************************************/

static intl
fadateval(narg, argsep, nobrk, noname)
intl *narg;
uchar *argsep;
intl *nobrk;
intl *noname;
{
    uchar tstr[25], *nele;
    intl day, mon, yr, cr;

    IGNOREPARM(narg);
    IGNOREPARM(argsep);

    /* check that we have a literal string */
    if(sscanf((char *) argstk[argsp - 1], "\"%d.%d.%d.\"", &day, &mon, &yr) != 3)
        return(0);

    cr = sprintf((char *) tstr, "%d.%d.%d", day, mon, yr);
    tstr[cr] = '\0';

    /* get rid of first argument */
    if((nele = malloc(strlen((char *) tstr) + 1)) == NULL)
        return(PD123_ERR_MEM);

    free(argstk[argsp - 1]);
    strcpy((char *) nele, (char *) tstr);
    argstk[argsp - 1] = nele;
    *noname = 1;
    *nobrk = 1;
    return(0);
}


/*********************************************
*                                            *
* fuddle the arguments to the index function *
*                                            *
*********************************************/

static intl
faindex(narg, argsep, nobrk, noname)
intl *narg;
uchar *argsep;
intl *nobrk;
intl *noname;
{
    intl err;
    uword16 col = 0, row = 0;

    IGNOREPARM(argsep);
    IGNOREPARM(nobrk);
    IGNOREPARM(noname);

    /* read the first slr in the first argument */
    scnslr(argstk[argsp - 3], &col, &row);

    /* get rid of first argument */
    free(argstk[argsp - 3]);

    /* shift up the other two arguments */
    argstk[argsp - 3] = argstk[argsp - 2];
    argstk[argsp - 2] = argstk[argsp - 1];
    --argsp;

    /* decrement argument count */
    *narg -= 1;

    /* adjust remaining arguments */
    if((err = addplusn(2, row + 1)) != 0)
        return(err);
    return(addplusn(1, col + 1));
}


/*******************************************************
*                                                      *
* locate a record of a given type in the lotus file    *
*                                                      *
* --in--                                               *
* type contains type of record to locate               *
* flag indicates type of search:                       *
*   0 specific type                                    *
*   1 column width record                              *
*   2 next row after a given row                       *
*                                                      *
* --out--                                              *
* pointer to record body                               *
* NULL if record not found                             *
*                                                      *
*******************************************************/

static uchar huge *
findrec(type, aflag, col, row)
intl type, aflag, col, row;
{
    uchar huge *startpos, huge *atpos, huge *datapos;
    uword16 opcode, length;

    /* load start of file */
    if(!curpos)
        curpos = lotusf;

    /* remember start position */
    startpos = curpos;

    /* search for required opcode */
    do
        {
        /* read opcode */
        atpos = curpos;
        opcode = readuword16(atpos);
        atpos += 2;
        length = readuword16(atpos);
        atpos += 2;

        switch(aflag)
            {
            case TYPE_MATCH:
                if(opcode == type)
                    return(atpos);
                break;

            case WIDTH_MATCH:
                if(opcode == type)
                    {
                    intl c;

                    datapos = atpos;

                    c = (intl) readuword16(atpos);
                    atpos += 2;
                    if(col == c)
                        return(datapos);
                    }
                break;

            case NEXT_ROW:
                if(opcode == L_INTEGER ||
                   opcode == L_NUMBER ||
                   opcode == L_LABEL ||
                   opcode == L_FORMULA)
                    {
                    intl c, r;

                    /* skip format byte */
                    datapos = atpos++;
                    c = (intl) readuword16(atpos);
                    atpos += 2;
                    r = (intl) readuword16(atpos);
                    atpos += 2;

                    /* set maximum column found */
                    maxcol = c > maxcol ? c : maxcol;
                    if((c == col) && (r > row))
                        return(datapos);
                    }
                break;
            }

            /* advance to next record */
            if(opcode == L_EOF)
                {
                if(aflag == NEXT_ROW)
                    {
                    foundeof = 1;
                    return(NULL);
                    }
                curpos = lotusf;
                }
            else
                {
                curpos += length + 4;
                }
        }
    while(curpos != startpos);

    return(NULL);
}


/**********************************
*                                 *
* lookup function in master table *
*                                 *
**********************************/

extern intl
flookup(func)
uchar *func;
{
    intl i;

    for(i = 0; i < sizeof(opreqv)/sizeof(struct oprdef); ++i)
        {
        oprp op = &opreqv[i];

        if(op->ftype != LO_FUNC)
            continue;

        if(!strcmp((char *) func, (char *) op->pdeqv))
            {
            csym.ixf = i;
            csym.symno = op->fno;
            return(strlen((char *) op->pdeqv));
            }
        }

    return(csym.symno = SYM_BAD);
}


/*********************************************************
*                                                        *
* output character to file and check for errors properly *
*                                                        *
*********************************************************/

extern intl
foutc(ch, file)
intl ch;
FILE *file;
{
    if(putc(ch, file) != EOF)
        return(0);

    if(ferror(file))
        return(PD123_ERR_FILE);
    return(0);
}


/*******************************************
*                                          *
* output CRLF to file and check for errors *
*                                          *
*******************************************/

static intl
foutcr(file)
FILE *file;
{
    intl err;

    if((err = foutc(CR, file)) != 0)
        return(err);
    return(foutc(LF, file));
}


/********************************************************
*                                                       *
* convert floating point number to string for PipeDream *
*                                                       *
********************************************************/

static intl
fptostr(resstr, fpval)
uchar *resstr;
double fpval;
{
    uchar *exp, *exps, sign;
    intl reslen;

    reslen = sprintf((char *) resstr, "%.15g", fpval);
    resstr[reslen] = '\0';

    /* search for exponent and remove leading zeroes because
    they confuse the Z88; remove the + for good measure */
    if((exp = (uchar *) strstr((char *) resstr, "e")) != NULL)
        {
        sign = *(++exp);
        exps = exp;
        if(sign == '-')
            {
            ++exp;
            ++exps;
            }
        if(sign == '+')
            ++exp;
        while(*exp == '0')
            ++exp;
        strncpy((char *) exps, (char *) exp, reslen - (exp - resstr));
        reslen = reslen - (exp - exps);
        resstr[reslen] = '\0';
        }

    return(reslen);
}


/*****************************
*                            *
* free all elements on stack *
*                            *
*****************************/

static void
freestk(void)
{
    while(argsp)
        free(argstk[--argsp]);
}


/*****************************************************
*                                                    *
* convert international character to lotus character *
*                                                    *
*****************************************************/

extern intl
ichlotus(ch)
intl ch;
{
    intl i;

    if((ch < 32) || (ch > 127))
        {
        #if MS
        for(i = 0; i < sizeof(ibmlics)/sizeof(intl); i += 2)
            if(ibmlics[i] == ch)
                return(ibmlics[i + 1]);
        #else
        for(i = 0; i < sizeof(isolics)/sizeof(intl); i += 2)
            if(isolics[i] == ch)
                return(isolics[i + 1]);
        #endif
        }

    return(ch);
}


/*****************************************************
*                                                    *
* convert lotus character to international character *
*                                                    *
*****************************************************/

static intl
lotusich(ch)
intl ch;
{
    intl i;

    if((ch < 32) || (ch > 127))
        {
        #if MS
        for(i = 0; i < sizeof(ibmlics)/sizeof(intl); i += 2)
            if(ibmlics[i + 1] == ch)
                return(ibmlics[i]);
        #else
        for(i = 0; i < sizeof(isolics)/sizeof(intl); i += 2)
            if(isolics[i + 1] == ch)
                return(isolics[i]);
        #endif
        }

    return(ch);
}


/*****************************************
*                                        *
* lookup operator in master table        *
*                                        *
* --out--                                *
* returns size in characters of operator *
*                                        *
*****************************************/

extern intl
olookup(opr)
uchar *opr;
{
    intl i;

    for(i = 0; i < sizeof(opreqv)/sizeof(struct oprdef); ++i)
        {
        oprp op = &opreqv[i];

        if((op->ftype != LO_BINARY) && (op->fno != LF_NOT))
            continue;

        if(!strncmp((char *) opr, (char *) op->pdeqv, strlen((char *) op->pdeqv)))
            {
            csym.ixf = i;
            csym.symno = op->fno;
            return(strlen((char *) op->pdeqv));
            }
        }

    return(csym.symno = SYM_BAD);
}


/****************************
*                           *
* output construct to file  *
*                           *
* --in--                    *
* pointer to construct text *
*                           *
****************************/

static intl
outcon(cons)
uchar *cons;
{
    intl err;

    if((err = foutc((intl) '%', fout)) != 0)
        return(err);

    if((err = outstr(cons)) != 0)
        return(err);

    if((err = foutc((intl) '%', fout)) != 0)
        return(err);

    return(0);
}


/******************************************
*                                         *
* write decimal place construct to output *
*                                         *
******************************************/

static intl
outdecplc(decplc)
intl decplc;
{
    intl err;

    if((err = outstr((uchar *) "%D")) != 0)
        return(err);
    if(fprintf(fout, "%d", decplc) < 0)
        return(PD123_ERR_FILE);
    return(foutc('%', fout));
}


/************************
*                       *
* output string to file *
*                       *
************************/

static intl
outstr(str)
uchar *str;
{
    intl err;

    while(*str)
        if((err = foutc((intl) *str++, fout)) != 0)
            return(err);
    return(0);
}


/*************************
*                        *
* read LOTUS file limits *
*                        *
*************************/

static intl
readrange(void)
{
    uchar huge *rec;
    intl i;

    /* check start of file */
    if(memcmp(lotusf, lfhead, 4))
        return(PD123_ERR_BADFILE);
    if(memcmp(lotusf + 4, lfh123, 2) && memcmp(lotusf + 4, lfh123_2, 2))
        return(PD123_ERR_BADFILE);

    if((rec = findrec(L_RANGE, TYPE_MATCH, 0, 0)) != NULL)
        {
        sc = (intl) readuword16(rec);
        rec += 2;
        sr = (intl) readuword16(rec);
        rec += 2;
        ec = (intl) readuword16(rec);
        rec += 2;
        er = (intl) readuword16(rec);
        rec += 2;
        }
    else
        {
        return(PD123_ERR_BADFILE);
        }

    if((sc == -1) || (ec == -1) || (ec == 0) || (er == 0))
        {
        sc = sr = 0;
        ec = LOTUS_MAXCOL - 1;
        er = LOTUS_MAXROW - 1;
        }

    maxcol = -1;
    foundeof = 0;

    /* read default column width */
    if((rec = findrec(L_WINDOW1, TYPE_MATCH, 0, 0)) != NULL)
        {
        rec += 6;
        defcwid = (intl) readuword16(rec);
        rec += 2;
        }
    else
        {
        defcwid = 0;
        }

    /* read hidden column vector */
    if((rec = findrec(L_HIDVEC1, TYPE_MATCH, 0, 0)) != NULL)
        {
        for(i = 0; i < 32; ++i)
            hidvec[i] = *rec++;
        }
    else
        {
        for(i = 0; i < 32; ++i)
            hidvec[i] = 0;
        }
    return(0);
 }


/****************************
*                           *
* read a double from memory *
*                           *
****************************/

static double
readdouble(arg)
uchar huge *arg;
{
#if MS

    return(*((double *) arg));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    intl i;
    union
        {
        double fpval;
        uchar fpbytes[8];
        } fp;

    for(i = 4; i < 8; ++i)
        fp.fpbytes[i] = *arg++;
    for(i = 0; i < 4; ++i)
        fp.fpbytes[i] = *arg++;

    return(fp.fpval);

#endif
}


/************************************
*                                   *
* read an unsigned word from memory *
*                                   *
************************************/

static uword16
readuword16(arg)
uchar huge *arg;
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
readword16(arg)
uchar huge *arg;
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


/**************
*             *
* RPN scanner *
*             *
**************/

static intl
scnsym(void)
{
    if(cursym != -1)
        {
        /* work out how to skip symbol */
        ++termp;
        switch(curopr->ftype)
            {
            case LO_CONST:
                switch(cursym)
                    {
                    case LF_CONST:
                        termp += 8;
                        break;
                    case LF_SLR:
                        termp += 4;
                        break;
                    case LF_RANGE:
                        termp += 8;
                        break;
                    case LF_INTEGER:
                        termp += 2;
                        break;
                    case LF_STRING:
                        while(*termp++)
                            ;
                        break;
                    default:
                        break;
                    }
                break;

            case LO_FUNC:
                /* skip argument count */
                if(curopr->nargs == -1)
                    ++termp;
                break;

            default:
                break;
            }
        }
    return(chksym());
}


/**********************************
*                                 *
* search default table for option *
*                                 *
**********************************/

extern intl
searchdefo(optid)
uchar *optid;
{
    intl i;

    for(i = 0; i < sizeof(optqv)/sizeof(struct optdef); ++i)
        if(!strcmp((char *) optid, (char *) optqv[i].optstr))
            return((intl) optqv[i].deflt);
    return(0);
}


/********************************
*                               *
* write out option start and id *
*                               *
********************************/

static intl
startopt(op)
optp op;
{
    intl err;

    if((err = outcon((uchar *) "OP")) != 0)
        return(err);
    if(fprintf(fout, "%s", op->optstr) < 0)
        return(PD123_ERR_FILE);
    return(0);
}


/*****************************
*                            *
* write out calculation mode *
*                            *
*****************************/

static intl
wrcalcmode(op)
optp op;
{
    uchar huge *rec;
    intl err;
    uchar cm;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
        {
        cm = *(rec + op->offset);
        if(!cm)
            {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = foutc('M', fout)) != 0)
                return(err);
            if((err = foutcr(fout)) != 0)
                return(err);
            }
        }
    return(0);
}


/******************************
*                             *
* write out calculation order *
*                             *
******************************/

static intl
wrcalcord(op)
optp op;
{
    uchar huge *rec, calc;
    char calco;
    intl err;

      if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
        {
        calc = *(rec + op->offset);
        if((err = startopt(op)) != 0)
            return(err);
        if(curpd >= PD_3)
            {
            switch(calc)
                {
                case 0xFF:
                    calco = 'R';
                    break;
                case 1:
                    calco = 'C';
                    break;
                default:
                case 0:
                    calco = 'N';
                    break;
                }
            if((err = foutc(calco, fout)) != 0)
                return(err);
            }
        else if((err = foutc(((calc == 0xFF) ? 'R' : 'C'), fout)) != 0)
            return(err);
        if((err = foutcr(fout)) != 0)
            return(err);
        }
    return(0);
}


/***************************
*                          *
* write out decimal places *
*                          *
***************************/

static intl
wrdecplc(op)
optp op;
{
    uchar huge *rec;
    intl err, decplc;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
        {
        rec += op->offset;

        if((*rec & L_FMTTYPE) != L_SPECL)
            {
            decplc = (intl) (*rec & L_DECPLC);
            if(decplc != op->deflt)
                {
                if((err = startopt(op)) != 0)
                    return(err);
                if(fprintf(fout, "%d", decplc) < 0)
                    return(PD123_ERR_FILE);
                if((err = foutcr(fout)) != 0)
                    return(err);
                }
            }
        else
            {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = foutc('F', fout)) != 0)
                return(err);
            if((err = foutcr(fout)) != 0)
                return(err);
            }

        }
    return(0);
}


/******************************
*                             *
* write out header and footer *
*                             *
******************************/

static intl
wrheadfoot(op)
optp op;
{
    uchar huge *rec;
    intl err;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
        {
        rec += op->offset;
        if(*rec)
            {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = foutc('|', fout)) != 0)
                return(err);
            while(*rec)
                {
                /* check for lotus page number */
                if(*rec == '#')
                    {
                    if((err = outstr((uchar *) "@P@")) != 0)
                        return(err);
                    ++rec;
                    }
                /* check for lotus date */
                else if(*rec == '@')
                    {
                    if((err = outstr((uchar *) "@D@")) != 0)
                        return(err);
                    ++rec;
                    }
                else if((err = foutc(lotusich((intl) *rec++), fout)) != 0)
                    return(err);
                }
            if((err = foutcr(fout)) != 0)
                return(err);
            }
        }
    return(0);
}


/*************************
*                        *
* write out a date to PD *
*                        *
*************************/

static intl
writedate(dateno)
long dateno;
{
    intl month, day, lasta, leap;
    long dayno;
    intl year;

    if(dateno > 73049)
        {
        day = month = year = 99;
        }
    else
        {
        dayno = year = month = day = 0;
        year = -1;
        leap = 0;
        while(dayno < dateno)
            {
            ++year;
            leap = (year & 3) ? 0 : 1;

            for(month = 0; (dayno < dateno) && (month < 12); ++month)
                {
                lasta = days[month];
                if(leap && (month == 1))
                    ++lasta;
                dayno += lasta;
                }
            }

        day = (intl) (dateno - (dayno - lasta));
        year %= 100;
        }

    if(fprintf(fout, "%d.%d.%d", day, month, year) < 0)
        return(PD123_ERR_FILE);
    return(0);
}


/***********************************
*                                  *
* write out lotus expression to pd *
*                                  *
***********************************/

static intl
writeexp(ep, col, row)
uchar huge *ep;
intl col, row;
{
    intl err, oldsp, nobrk;

    /* set scanner index */
    termp = ep;
    argsp = nobrk = 0;

    chksym();
    do
        {
        switch(curopr->ftype)
            {
            case LO_CONST:
                if((argstk[argsp++] =
                            convconst(cursym, termp + 1, col, row)) == 0)
                    return(PD123_ERR_MEM);
                if(argsp == MAXSTACK)
                    return(PD123_ERR_EXP);
                break;

            case LO_END:
                break;

            case LO_BRACKETS:
                {
                if(!nobrk)
                    {
                    uchar *nele, *oele, *c;

                    if(!argsp)
                        {
                        freestk();
                        return(PD123_ERR_EXP);
                        }

                    oele = argstk[--argsp];

                    /* add two for brackets and null */
                    if((nele = malloc(strlen((char *) oele) + 2 + 1)) == NULL)
                        return(PD123_ERR_MEM);

                    c = nele;
                    *c++ = '(';
                    strcpy((char *) c, (char *) oele);
                    c += strlen((char *) oele);
                    *c++ = ')';
                    *c++ = '\0';
                    free(oele);
                    argstk[argsp++] = nele;
                    }
                nobrk = 0;
                break;
                }

            case LO_UNARY:
                {
                uchar *nele, *oele, *c;
                intl addlen;

                if(!argsp)
                    {
                    freestk();
                    return(PD123_ERR_EXP);
                    }

                oele = argstk[--argsp];
                addlen = strlen((char *) curopr->pdeqv);
                /* extra for new operator and null */
                if((nele = malloc(strlen((char *) oele) + addlen + 1)) == NULL)
                    return(PD123_ERR_MEM);

                c = nele;
                strcpy((char *) c, (char *) curopr->pdeqv);
                c += addlen;
                strcpy((char *) c, (char *) oele);
                c += strlen((char *) oele);
                *c++ = '\0';
                free(oele);
                argstk[argsp++] = nele;
                if(!curopr->ltpok || (curpd < curopr->ltpok))
                    ++poorfunc;
                break;
                }

            case LO_BINARY:
                {
                uchar *ele1, *ele2, *nele, *c;
                intl addlen, lele1, lele2;

                if(argsp < 2)
                    {
                    freestk();
                    return(PD123_ERR_EXP);
                    }

                ele2 = argstk[--argsp];
                ele1 = argstk[--argsp];
                lele1 = strlen((char *) ele1);
                lele2 = strlen((char *) ele2);
                addlen = strlen((char *) curopr->pdeqv);
                /* two arguments, operator and null */
                if((nele = malloc(lele1 + lele2 + addlen + 1)) == NULL)
                    return(PD123_ERR_MEM);

                c = nele;
                strcpy((char *) c, (char *) ele1);
                c += lele1;
                strcpy((char *) c, (char *) curopr->pdeqv);
                c += addlen;
                strcpy((char *) c, (char *) ele2);
                c += lele2;
                *c++ = '\0';
                free(ele1);
                free(ele2);
                argstk[argsp++] = nele;
                if(!curopr->ltpok || (curpd < curopr->ltpok))
                    ++poorfunc;
                break;
                }

            case LO_FUNC:
                {
                intl narg, i, tlen, argc, noname = 0;
                uchar *nele, *c;
                uchar argsep = ',';

                /* work out number of arguments */
                if((narg = curopr->nargs) == -1)
                    narg = (intl) *(termp + 1);

                if(argsp < narg)
                    {
                    freestk();
                    return(PD123_ERR_EXP);
                    }

                /* call argument fuddler */
                if(curopr->argix)
                    if((err = (*argfuddle[curopr->argix - 1])(&narg,
                                                              &argsep,
                                                              &nobrk,
                                                              &noname)) != 0)
                        return(err);

                tlen = 0;
                if(narg)
                    {
                    /* add up the length of all the arguments */
                    for(i = 1; i <= narg; ++i)
                        tlen += strlen((char *) argstk[argsp - i]);
                    /* add in space for commas and function brackets */
                    tlen += narg - 1;
                    if(!nobrk)
                        tlen += 2;
                    }

                /* add length of name, null */
                if(!noname)
                    tlen += strlen((char *) curopr->pdeqv);
                ++tlen;
                if((nele = malloc(tlen)) == NULL)
                    return(PD123_ERR_MEM);

                c = nele;
                if(!noname)
                    {
                    strcpy((char *) c, (char *) curopr->pdeqv);
                    c += strlen((char *) curopr->pdeqv);
                    }
                argc = narg;
                if(narg)
                    {
                    if(!nobrk)
                        *c++ = '(';
                    while(argc)
                        {
                        uchar *carg = argstk[argsp - (argc--)];
                        strcpy((char *) c, (char *) carg);
                        c += strlen((char *) carg);
                        if(argc)
                            *c++ = argsep;
                        free(carg);
                        }
                    if(!nobrk)
                        *c++ = ')';
                    }
                *c++ = '\0';
                argsp -= narg;
                argstk[argsp++] = nele;
                if(!curopr->ltpok || (curpd < curopr->ltpok))
                    ++poorfunc;
                nobrk = 0;
                break;
                }
            }

        if(cursym == LF_END)
            break;
        scnsym();
        }
    while(TRUE);

    err = outstr(argstk[0]);
    oldsp = argsp;
    freestk();
    return(err ? err : (oldsp == 1) ? 0 : PD123_ERR_EXP);
}


/*********************************
*                                *
* write out format details to PD *
*                                *
*********************************/

static intl
writeformat(fmtp, specflg)
uchar huge *fmtp;
intl *specflg;
{
    intl err, decplc;

    /* numbers are always right aligned */
    if((err = outcon((uchar *) "R")) != 0)
        return(err);

    decplc = *fmtp & L_DECPLC;
    switch(*fmtp & L_FMTTYPE)
        {
        case L_CURCY:
            if((err = outcon((uchar *) "LC")) != 0)
                return(err);
            if((err = outcon((uchar *) "B")) != 0)
                return(err);
            if((err = outdecplc(decplc)) != 0)
                return(err);
            break;
        case L_PERCT:
            if((err = outcon((uchar *) "TC")) != 0)
                return(err);
            if((err = outdecplc(decplc)) != 0)
                return(err);
            break;
        default:
        case L_COMMA:
            if((err = outcon((uchar *) "B")) != 0)
                return(err);
            if((err = outdecplc(decplc)) != 0)
                return(err);
            break;
        case L_FIXED:
        case L_SCIFI:
            if((err = outdecplc(decplc)) != 0)
                return(err);
            break;
        case L_SPECL:
            switch(decplc)
                {
                /* general format */
                case L_GENFMT:
                    if((err = outcon((uchar *) "DF")) != 0)
                        return(err);
                    break;
                /* dates */
                case L_DDMMYY:
                case L_DDMM:
                case L_MMYY:
                case L_DATETIME:
                case L_DATETIMES:
                case L_DATEINT1:
                case L_DATEINT2:
                    *specflg = P_DATE;
                    break;
                /* text */
                case L_TEXT:
                    *specflg = P_TEXT;
                    break;
                default:
                    break;
                }
            break;
        }

    return(0);
}


/*******************************************
*                                          *
* read options page equivalents from LOTUS *
* and write to output                      *
*                                          *
*******************************************/

static intl
writeoptions()
{
    intl count, err;

    /* loop for each option */
    for(count = 0; count < sizeof(optqv)/sizeof(struct optdef); ++count)
        if((err = (*optqv[count].wropt)(&optqv[count])) != 0)
            return(err);

    return(0);
}


/***************************
*                          *
* write out a column to PD *
*                          *
***************************/

static intl
writepcol(col, sro, ero)
intl col, sro, ero;
{
    intl err, cw, row;
    uchar strcol[5];
    uchar huge *rec;

    /* write out construct */
    if((err = outstr((uchar *) "%CO:")) != 0)
        return(err);

    strcol[xtos(strcol, col)] = '\0';
    if((err = outstr(strcol)) != 0)
        return(err);

    /* reset pointer to start */
    curpos = NULL;
    if((rec = findrec(L_COLW1, WIDTH_MATCH, col, 0)) != NULL)
        {
        rec += 2;
        cw = (intl) *rec;
        }
    else
        {
        cw = defcwid;
        }

    /* check hidden vector for an entry */
    if(hidvec[col >> 3] & (1 << (col & 7)))
        cw = 0;

    if(fprintf(fout, ",%d,72%%", cw) < 0)
        return(PD123_ERR_FILE);

    /* output all the rows */
    row = sro - 1;
    while(row <= ero)
        {
        uword16 opcode;
        intl count, oldrow;
        uchar huge *fmtp;

        if((rec = findrec(0, NEXT_ROW, col, row)) == NULL)
            break;

        /* read opcode */
        fmtp = rec;
        rec -= 4;
        opcode = readuword16(rec);
        rec += 2;

        /* read row number */
        rec += 5;
        oldrow = row;
        row = (intl) readuword16(rec);
        rec += 2;
        count = row - oldrow - 1;

        /* output blank rows to pd file */
        while(count--)
            if((err = foutcr(fout)) != 0)
                return(err);

        /* deal with different slot types */
        switch(opcode)
            {
            case L_INTEGER:
                {
                intl intval, specflg = 0;

                if((err = writeformat(fmtp, &specflg)) != 0)
                    return(err);

                intval = (intl) readword16(rec);
                rec += 2;
                if((err = checkdate(&specflg, (long) intval)) != 0)
                    return(err);
                if(specflg)
                    if(fprintf(fout, "%d", intval) < 0)
                        return(PD123_ERR_FILE);
                break;
                }

            case L_NUMBER:
                {
                intl specflg = 0;
                double fpval;
                uchar resstr[25];

                if((err = writeformat(fmtp, &specflg)) != 0)
                    return(err);

                fpval = readdouble(rec);

                #if ARTHUR || RISCOS
                if(fpval < LONG_MAX)
                    if((err = checkdate(&specflg, (long) (fpval + .5))) != 0)
                        return(err);
                #else
                if((err = checkdate(&specflg, (long) (fpval + .5))) != 0)
                    return(err);
                #endif

                if(specflg)
                    {
                    fptostr(resstr, fpval);
                    if((err = outstr(resstr)) != 0)
                        return(err);
                    }
                break;
                }

            case L_LABEL:
                {
                intl rep = FALSE, width = cw;
                uchar huge *startlab;

                /* deal with label alignment byte */
                switch(*rec++)
                    {
                    case '\'':
                        break;
                    case '"':
                        if((err = outcon((uchar *) "R")) != 0)
                            return(err);
                        break;
                    case '^':
                        if((err = outcon((uchar *) "C")) != 0)
                            return(err);
                        break;
                    case '\\':
                        rep = TRUE;
                        break;
                    }

                startlab = rec;
                do
                    {
                    while(*rec)
                        {
                        if((err = foutc(lotusich((intl) *rec++), fout)) != 0)
                            return(err);
                        --width;
                        }
                    rec = startlab;
                    }
                while(rep && (width > 0));

                break;
                }

            case L_FORMULA:
                {
                intl specflg = 0;

                if((err = writeformat(fmtp, &specflg)) != 0)
                    return(err);

                if((err = outcon((uchar *) "V")) != 0)
                    return(err);

                if((err = writeexp(rec + 10, col, row)) != 0)
                    {
                    if(err != PD123_ERR_EXP)
                        return(err);
                    else
                        errexp += 1;
                    }

                break;
                }
            }

        if((err = foutcr(fout)) != 0)
            return(err);
        }
    return(0);
}


/***************************
*                          *
* write out a margin value *
*                          *
***************************/

static intl
wrmar(optp op, uword16 value)
{
    intl err;

    if(value != op->deflt)
        {
        if((err = startopt(op)) != 0)
            return(err);
        if(fprintf(fout, "%d", value) < 0)
            return(PD123_ERR_FILE);
        if((err = foutcr(fout)) != 0)
            return(err);
        }
    return(0);
}


/****************************
*                           *
* write out margin settings *
*                           *
****************************/

static intl
wrmargins(op)
optp op;
{
    uchar huge *rec;
    uword16 value;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
        {
        rec += op->offset;
        value = *rec++;
        return(wrmar(op, value));
        }
    return(0);
}


/*******************************
*                              *
* write out minus and brackets *
*                              *
*******************************/

static intl
wrminbrk(op)
optp op;
{
    uchar huge *rec;
    intl err;
    intl minbrk;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
        {
        rec += op->offset;
        minbrk = (*rec & L_FMTTYPE) == L_CURCY ? 1 : 0;
        if(minbrk)
            {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = foutc('B', fout)) != 0)
                return(err);
            if((err = foutcr(fout)) != 0)
                return(err);
            }
        }
    return(0);
}


/******************************
*                             *
* write out set margin values *
*                             *
******************************/

static intl
wrsetmar(op)
optp op;
{
    return(wrmar(op, 2));
}


/******************************
*                             *
* write out text/numbers flag *
*                             *
******************************/

static intl
wrtextn(op)
optp op;
{
    intl err;

    if(curpd >= PD_PC)
        {
        if((err = startopt(op)) != 0)
            return(err);
        if((err = foutc((intl) op->deflt, fout)) != 0)
            return(err);
        return(foutcr(fout));
        }

    return(0);
}


/**********************
*                     *
* write out thousands *
*                     *
**********************/

static intl
wrthousands(op)
optp op;
{
    uchar huge *rec, thous;
    intl err;

    if((rec = findrec(op->opcode, TYPE_MATCH, 0, 0)) != NULL)
        {
        rec += op->offset;
        thous = *rec & L_FMTTYPE;
        if(thous == L_COMMA || thous == L_CURCY)
            {
            if((err = startopt(op)) != 0)
                return(err);
            if((err = foutc('1', fout)) != 0)
                return(err);
            if((err = foutcr(fout)) != 0)
                return(err);
            }
        }
    return(0);
}

/**********************
*                     *
* write out wrap mode *
*                     *
**********************/

static intl
wrwrapmo(op)
optp op;
{
    intl err;

    if((err = startopt(op)) != 0)
        return(err);
    if((err = foutc((intl) op->deflt, fout)) != 0)
        return(err);
    return(foutcr(fout));
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
xtos(string, x)
uchar *string;
intl x;
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

/* end of pd123.c */
