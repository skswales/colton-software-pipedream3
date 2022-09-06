/* 123pd.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/**************************************
*                                     *
* program to convert a PipeDream file *
* into a LOTUS 123 (rel 2) .WK1 file  *
*                                     *
* MRJC                                *
* February 1988                       *
* SKS removed superfluous int i;      *
*    from writeslot                   *
**************************************/

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

/*
function declarations
*/

static intl compileexp(uchar *, intl, intl);
static void lexpr(void);
static void alterm(void);
static void blterm(void);
static void clterm(void);
static void dlterm(void);
static void elterm(void);
static void flterm(void);
static void glterm(void);
static void lterm(void);
static intl procfunc(oprp);
static void mochoose(intl *, intl);
static void moindex(intl *, intl);
static void element(void);
static intl dolotus(void);
static void ebout(uchar byt);
static void edout(double);
static void euwout(uword16 wrd);
static void ewout(word16 wrd);
static intl findcolumns(void);
static word16 makeabr(uword16 ref, intl csl);
static intl my_strnicmp(uchar *, uchar *, intl);
static intl nxtsym(void);
static uchar *procdcp(uchar *, uchar *, intl *);
static uchar *procpct(uchar *, uchar *, intl *);
static uchar huge *searchcon(uchar huge *startp, uchar huge *endp, uchar *conid);
static uchar huge *searchopt(uchar *);
static intl reccon(uchar *, double *, intl *, intl *, intl *, intl *);
#ifdef UTIL_PTL
static intl showrow(word32);
#endif
intl scnslr(uchar *, uword16 *, uword16 *);
static intl stox(uchar *, intl *);
static intl wrlcalcmode(void);
static intl wrlcalcord(void);
static intl wrcols(void);
static intl wrcolws(void);
static intl wrfooter(void);
static intl wrgraph(void);
static intl wrheader(void);
static intl wrlheadfoot(uchar *);
static intl wrhidvec(void);
static intl writecolrow(intl, intl);
static intl writedouble(double);
static intl writeins(lfip);
static intl writelabel(uchar *, intl, intl, intl);
static intl writeldate(intl, intl, intl, intl, intl);
static intl writelformat(intl, intl);
intl writelotus(FILE *, FILE *, intl, intl (*)(word32));
static intl writeslot(uchar *, intl, intl, uword16, intl);
static intl writesslot(intl, intl, intl, intl, uword16, intl);
static intl writeuword(uword16);
static intl writeword(word16);
static intl wrlmar(uchar *);
static intl wrlmargins(void);
static intl wrwindow1(void);

/***********************
*                      *
* lotus file structure *
*                      *
***********************/

#define NOPT 0xFF

static struct lfins lfstruct[] =
{
    L_BOF,          2, (uchar *) "\x6\x4",          2,    NOPT,       NULL,
    L_RANGE,        8, (uchar *) "",                0,    NOPT,       NULL,
    L_CPI,          6, (uchar *) "",                0,    NOPT,       NULL,
    L_CALCCOUNT,    1, (uchar *) "\x1",             1,    NOPT,       NULL,
    L_CALCMODE,     1, (uchar *) "",                0,    NOPT,wrlcalcmode,
    L_CALCORDER,    1, (uchar *) "",                0,    NOPT, wrlcalcord,
    L_SPLIT,        1, (uchar *) "",                0,    NOPT,       NULL,
    L_SYNC,         1, (uchar *) "",                0,    NOPT,       NULL,
    L_WINDOW1,     32, (uchar *) "",                0,    NOPT,  wrwindow1,
    L_COLW1,        3, (uchar *) "",                0,    NOPT,    wrcolws,
    L_HIDVEC1,     32, (uchar *) "",                0,    NOPT,   wrhidvec,
    L_CURSORW12,    1, (uchar *) "",                0,    NOPT,       NULL,
    L_TABLE,       25, (uchar *) "\xFF\xFF\x0\x0",  4,       1,       NULL,
    L_QRANGE,      25, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_PRANGE,       8, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_UNFORMATTED,  1, (uchar *) "",                0,    NOPT,       NULL,
    L_FRANGE,       8, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_SRANGE,       8, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_KRANGE,       9, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_KRANGE2,      9, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_RRANGES,     25, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_MATRIXRANGES,40, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_HRANGE,      16, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_PARSERANGES, 16, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_PROTEC,       1, (uchar *) "",                0,    NOPT,       NULL,
    L_FOOTER,     242, (uchar *) "",                0,    NOPT,   wrfooter,
    L_HEADER,     242, (uchar *) "",                0,    NOPT,   wrheader,
    L_SETUP,       40, (uchar *) "",                0,    NOPT,       NULL,
    L_MARGINS,     10, (uchar *) "",                0,    NOPT, wrlmargins,
    L_LABELFMT,     1, (uchar *) "\x27",            1,    NOPT,       NULL,
    L_TITLES,      16, (uchar *) "\xFF\xFF\x0\x0",  4,       0,       NULL,
    L_GRAPH,      439, (uchar *) "",                4,       0,    wrgraph,
    L_FORMULA,      0, (uchar *) "",                0,    NOPT,     wrcols,
    L_EOF,          0, (uchar *) "",                0,    NOPT,       NULL
};

/*
table of argument modifier functions
*/

static void (*argmod[])(intl *, intl) =
{
    mochoose,
    NULL,
    moindex,
    NULL,
};

/*
table of valid constructs for slots
*/

#define BIT_BRK 0x0001
#define BIT_CEN 0x0002
#define BIT_DCN 0x0004
#define BIT_DCF 0x0008
#define BIT_LFT 0x0010
#define BIT_CUR 0x0020
#define BIT_RYT 0x0040
#define BIT_EXP 0x0080

typedef struct constr *conp;

static struct constr
{
    uchar *conid;
    uword16 mask;
    uchar *(*proccons)(uchar *, uchar *, intl *);
}
constab[] =
{
    (uchar *) "V",    BIT_EXP,    NULL,
    (uchar *) "DF",   BIT_DCF,    NULL,
    (uchar *) "R",    BIT_RYT,    NULL,
    (uchar *) "C",    BIT_CEN,    NULL,
    (uchar *) "B",    BIT_BRK,    NULL,
    (uchar *) "D",    BIT_DCN,    procdcp,
    (uchar *) "LCR",        0,    NULL,
    (uchar *) "LC",   BIT_CUR,    NULL,
    (uchar *) "L",    BIT_LFT,    NULL,
    (uchar *) "TC",         0,    NULL,
    (uchar *) "PC",         0,    procpct,
    (uchar *) "F",          0,    NULL,
    (uchar *) "H",          0,    NULL,
    (uchar *) "JL",         0,    NULL,
    (uchar *) "JR",         0,    NULL,
    (uchar *) "P",          0,    NULL
};

/* pd constants */
#define PD_REAL 1
#define PD_DATE 2
#define PD_INTEGER 3

/* arrays of column information */
static uchar huge *colend[LOTUS_MAXCOL];
static uchar huge *colcur[LOTUS_MAXCOL];
static intl colwid[LOTUS_MAXCOL];
uchar hidvec[32];

/* global default decimal places */
static intl decplc = 2;

/* end of options page */
static uchar huge *optend;

/* PD file details */
static uchar huge *pdf;
static uchar huge *pdend;
static unsigned long pdsize;

/* expression output buffer */
static uchar expbuf[512];
static uchar *expop;
static uchar *exppos;
static intl ecol;
static intl erow;
struct symbol csym;

/* symbol checker macro */
#define chknxs() ((csym.symno != SYM_BLANK) ? csym.symno : nxtsym())

/* current lotus instruction */
static lfip curlfi = NULL;

/* counter routine */
static intl (*counter)(word32);

/************************************************************************
*                                                                       *
* main loop for PipeDream to Lotus                                      *
* this main procedure is compiled if the constant                       *
* UTIL_PTL is defined:                                                  *
*                                                                       *
* UTIL_LTP creates stand-alone utility for Lotus to PipeDream           *
* UTIL_PTL creates stand-alone utility for PipeDream to Lotus           *
* INT_LPTPL creates internal code for converter using function lp_lptpl *
*                                                                       *
************************************************************************/

#ifdef UTIL_PTL
main(argc, argv)
int argc;
char **argv;
{
    intl col, err;
    uchar huge *op;

    /* banner */
    printf("PipeDream to Lotus 123 converter\nColton Software 1988\n");

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
    pdsize = filelength(fileno(fin));
    #else
    if(fseek(fin, 0l, SEEK_END))
        return(PD123_ERR_FILE);
    pdsize = ftell(fin);
    if(fseek(fin, 0l, SEEK_SET))
        return(PD123_ERR_FILE);
    #endif

    #ifdef MS_HUGE
    if(!(pdf = halloc(pdsize + 1, sizeof(uchar))))
    #else
    if(!(pdf = malloc((unsigned int) pdsize)))
    #endif
        {
        printf("Not enough memory for PD file\n");
        exit(0);
        }

    /* read in PD file */
    #ifdef MS_HUGE
    op = pdf;
    while(!feof(fin))
        *op++ = (uchar) getc(fin);
    #else
    fread(pdf, 1, (uword16) pdsize, fin);
    #endif
    pdend = pdf + pdsize;
    fclose(fin);

    /* set pd level */
    curpd = PD_Z88;
    counter = showrow;

    /* write out lotus file */
    err = dolotus();
    printf("\n");

    switch(err)
        {
        case PD123_ERR_MEM:
            printf("Out of memory\n");
            break;
        case PD123_ERR_FILE:
            perror("File error");
            break;
        case PD123_ERR_BADFILE:
            printf("Bad PipeDream file\n");
            break;
        case PD123_ERR_BIGFILE:
            printf("Too many rows or columns for Lotus\n");
            break;
        default:
            break;
        }

    fclose(fout);
    #ifdef MS_HUGE
    hfree(pdf);
    #else
    free(pdf);
    #endif

    if(!err && errexp)
        printf("%d bad expressions found\n", errexp);
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
* function to write a lotus file called from elsewhere         *
*                                                              *
* --in--                                                       *
* fin and fout are file pointers to open channels              *
* level specifies the level of the PD file                     *
* routine is the address of a routine to call to show activity *
* this returns non-zero if an abort is required                *
*                                                              *
***************************************************************/

#ifdef INT_LPTPL
intl
writelotus(inf, outf, level, routine)
FILE *inf, *outf;
intl level;
intl (*routine)(word32);
{
    intl err;

    #ifdef MS_HUGE
    intl col;
    uchar huge *op;
    #endif

    /* set level of conversion */
    curpd = level;

    /* save file pointers */
    fin = inf;
    fout = outf;

    /* set counter routine */
    counter = routine;

    #if MS
    pdsize = filelength(fileno(fin));
    #else
    if(fseek(fin, 0l, SEEK_END))
        return(PD123_ERR_FILE);
    pdsize = ftell(fin);
    if(fseek(fin, 0l, SEEK_SET))
        return(PD123_ERR_FILE);
    #endif

    #ifdef MS_HUGE
    if(!(pdf = halloc(pdsize + 1, sizeof(uchar))))
        return(PD123_ERR_MEM);
    #else
    #if RISCOS
    if(!flex_alloc((flex_ptr) &pdf, (intl) pdsize))
        return(PD123_ERR_MEM);
    #else
    if((pdf = malloc((unsigned int) pdsize)) == NULL)
        return(PD123_ERR_MEM);
    #endif
    #endif

    /* read in PD file */
    #ifdef MS_HUGE
    op = pdf;
    while(!feof(fin))
        *op++ = (uchar) getc(fin);
    #else
        #if MS
        fread(pdf, 1, (uword16) pdsize, fin);
        #else
        fread(pdf, 1, (size_t) pdsize, fin);
        #endif
    #endif
    pdend = pdf + pdsize;

    /* write out lotus file */
    err = dolotus();

    #ifdef MS_HUGE
    hfree(pdf);
    #else
    #if RISCOS
    flex_free((flex_ptr) &pdf);
    #else
    free(pdf);
    #endif
    #endif

    if(!err && (errexp || poorfunc))
        err = PD123_ERR_EXP;

    return(err);
}
#endif

/**************************************************
*                                                 *
* compile PipeDream expression into RPN for LOTUS *
*                                                 *
**************************************************/

static intl
compileexp(slot, col, row)
uchar *slot;
intl col, row;
{
    csym.symno = SYM_BLANK;
    csym.scandate = 0;
    expop = expbuf;
    exppos = slot;
    ecol = col;
    erow = row;

    lexpr();
    if(csym.symno != LF_END)
        return(0);
    ebout(LF_END);
    return(expop - expbuf);
}

/***************************************************
*                                                  *
* expression recogniser works by recursive descent *
* recognise |                                      *
*                                                  *
***************************************************/

static void
lexpr()
{
    alterm();
    while(chknxs() == LF_OR)
        {
        csym.symno = SYM_BLANK;
        alterm();
        ebout(LF_OR);
        }
    return;
}

/**************
*             *
* recognise & *
*             *
**************/

static void
alterm()
{
    blterm();
    while(chknxs() == LF_AND)
        {
        csym.symno = SYM_BLANK;
        blterm();
        ebout(LF_AND);
        }
    return;
}

/********************************
*                               *
* recognise =, <>, <, >, <=, >= *
*                               *
********************************/

static void
blterm()
{
    intl nxsym;

    clterm();
    do
        {
        switch(nxsym = chknxs())
            {
            case LF_EQUALS:
            case LF_NOTEQUAL:
            case LF_LT:
            case LF_GT:
            case LF_LTEQUAL:
            case LF_GTEQUAL:
                csym.symno = SYM_BLANK;
                break;
            default:
                return;
            }
        clterm();
        ebout((uchar) nxsym);
        }
    while(TRUE);
}

/*****************
*                *
* recognise +, - *
*                *
*****************/

static void
clterm()
{
    intl nxsym;

    dlterm();
    do
        {
        switch(nxsym = chknxs())
            {
            case LF_PLUS:
            case LF_MINUS:
                csym.symno = SYM_BLANK;
                break;
            default:
                return;
            }
        dlterm();
        ebout((uchar) nxsym);
        }
    while(TRUE);
}

/*****************
*                *
* recognise *, / *
*                *
*****************/

static void
dlterm()
{
    intl nxsym;

    elterm();
    do
        {
        switch(nxsym = chknxs())
            {
            case LF_TIMES:
            case LF_DIVIDE:
                csym.symno = SYM_BLANK;
                break;
            default:
                return;
            }
        elterm();
        ebout((uchar) nxsym);
        }
    while(TRUE);
}

/**************
*             *
* recognise ^ *
*             *
**************/

static void
elterm()
{
    flterm();
    while(chknxs() == LF_POWER)
        {
        csym.symno = SYM_BLANK;
        flterm();
        ebout(LF_POWER);
        }
    return;
}

/**************************
*                         *
* recognise unary +, -, ! *
*                         *
**************************/

static void
flterm()
{
    switch(chknxs())
        {
        case LF_PLUS:
            csym.symno = SYM_BLANK;
            flterm();
            ebout(LF_UPLUS);
            return;
        case LF_MINUS:
            csym.symno = SYM_BLANK;
            flterm();
            ebout(LF_UMINUS);
            return;
        case LF_NOT:
            csym.symno = SYM_BLANK;
            flterm();
            ebout(LF_NOT);
            return;
        default:
            glterm();
            return;
        }
}

/*****************************
*                            *
* recognise lterm or brackets *
*                            *
*****************************/

static void
glterm()
{
    if(chknxs() == SYM_OBRACKET)
        {
        csym.symno = SYM_BLANK;
        lexpr();
        if(chknxs() != SYM_CBRACKET)
            {
            csym.symno = SYM_BAD;
            return;
            }
        csym.symno = SYM_BLANK;
        ebout(LF_BRACKETS);
        }
    else
        {
        lterm();
        }
}

/************************************
*                                   *
* recognise constants and functions *
*                                   *
************************************/

static void
lterm()
{
    intl nxsym;
    uchar *c;

    switch(nxsym = chknxs())
        {
        case LF_CONST:
            csym.symno = SYM_BLANK;
            ebout(LF_CONST);
            edout(csym.fpval);
            return;

        case LF_SLR:
            csym.symno = SYM_BLANK;
            ebout(LF_SLR);
            ewout(makeabr(csym.stcol, ecol));
            ewout(makeabr(csym.strow, erow));
            return;

        case LF_INTEGER:
            csym.symno = SYM_BLANK;
            ebout(LF_INTEGER);
            ewout((word16) csym.fpval);
            return;

        case LF_STRING:
            csym.symno = SYM_BLANK;
            ebout(LF_STRING);
            c = csym.stringp;
            while(*c)
                ebout((uchar) ichlotus((intl) *c++));
            ebout('\0');
            return;

        case SYM_FUNC:
            csym.symno = SYM_BLANK;
            switch(opreqv[csym.ixf].nargs)
                {
                /* zero argument functions */
                case 0:
                    ebout(opreqv[csym.ixf].fno);
                    return;

                /* variable argument functions */
                case -1:
                    {
                    intl narg, fno;
                    oprp funp = &opreqv[csym.ixf];

                    fno = funp->fno;
                    narg = procfunc(funp);
                    ebout((uchar) fno);
                    ebout((uchar) narg);
                    return;
                    }

                /* fixed argument functions */
                default:
                    {
                    intl fno;
                    oprp funp = &opreqv[csym.ixf];

                    fno = funp->fno;
                    procfunc(funp);
                    ebout((uchar) fno);
                    return;
                    }
                }
        }
}

/************************************
*                                   *
* process a function with arguments *
*                                   *
************************************/

static intl
procfunc(funp)
oprp funp;
{
    intl narg = 0;
    void (*amodp)() = NULL;

    if(funp->argix)
        amodp = argmod[funp->argix - 1];

    if(chknxs() != SYM_OBRACKET)
        {
        csym.symno = SYM_BAD;
        return(0);
        }

    do
        {
        /* call argument modifier */
        if(amodp)
            (*amodp)(&narg, 0);
        csym.symno = SYM_BLANK;
        element();
        if(amodp)
            (*amodp)(&narg, 1);
        ++narg;
        }
    while(chknxs() == SYM_COMMA);

    if(chknxs() != SYM_CBRACKET)
        {
        csym.symno = SYM_BAD;
        return(narg);
        }

    if((funp->nargs >= 0) && (funp->nargs != (uchar) narg))
        return(csym.symno = SYM_BAD);

    csym.symno = SYM_BLANK;
    return(narg);
}

/**************************
*                         *
* modify choose arguments *
*                         *
**************************/

static void
mochoose(narg, prepost)
intl *narg, prepost;
{
    /* get to after first argument */
    if((*narg != 0) || !prepost)
        return;

    /* add minus 1 to argument */
    ebout(LF_INTEGER);
    euwout(1);
    ebout(LF_MINUS);
}

/*************************
*                        *
* modify index arguments *
*                        *
*************************/

static void
moindex(narg, prepost)
intl *narg, prepost;
{
    switch(*narg)
        {
        case 0:
            if(!prepost)
                {
                ++(*narg);
                ebout(LF_RANGE);
                ewout(makeabr(0, ecol));
                ewout(makeabr(0, erow));
                ewout(makeabr(LOTUS_MAXCOL - 1, ecol));
                ewout(makeabr(LOTUS_MAXROW - 1, erow));
                }
            break;

        case 1:
        case 2:
            if(prepost)
                {
                /* add minus 1 to argument */
                ebout(LF_INTEGER);
                euwout(1);
                ebout(LF_MINUS);
                }
            break;

        default:
            break;
        }
}

/*******************************************************
*                                                      *
* recognise an element of a list of function arguments *
*                                                      *
*******************************************************/

static void
element()
{
    if(chknxs() == LF_RANGE)
        {
        csym.symno = SYM_BLANK;
        ebout(LF_RANGE);
        ewout(makeabr(csym.stcol, ecol));
        ewout(makeabr(csym.strow, erow));
        ewout(makeabr(csym.encol, ecol));
        ewout(makeabr(csym.enrow, erow));
        return;
        }

    lexpr();
}

/******************************************
*                                         *
* use the master lotus structure table to *
* write out the lotus file                *
*                                         *
******************************************/

static intl
dolotus()
{
    intl err, i;

    if(!findcolumns())
        return(PD123_ERR_BADFILE);

    if(maxcol >= LOTUS_MAXCOL)
        return(PD123_ERR_BIGFILE);

    for(i = 0; i < sizeof(lfstruct)/sizeof(struct lfins); ++i)
        {
        curlfi = &lfstruct[i];

        if(curlfi->writefunc)
            {
            /* call special function for this instruction */
            if((err = (*curlfi->writefunc)()) != 0)
                return(err);
            }
        else
            {
            /* use table to write out default data */
            if((err = writeins(curlfi)) != 0)
                return(err);
            if(curlfi->length)
                {
                intl len = (intl) curlfi->length;
                intl dlen = (intl) curlfi->dlen;
                uchar *dp = curlfi->sdata;

                /* check for a pattern to be output */
                if(curlfi->pattern == NOPT)
                    {
                    /* output data */
                    while(dlen--)
                        {
                        if((err = foutc((intl) *dp++, fout)) != 0)
                            return(err);
                        --len;
                        }
                    }
                else
                    {
                    intl tlen = curlfi->pattern;

                    /* output leading nulls before pattern */
                    while(tlen--)
                        {
                        if((err = foutc(0, fout)) != 0)
                            return(err);
                        --len;
                        }

                    /* output as many patterns as possible */
                    while(dlen <= len)
                        {
                        tlen = dlen;
                        while(tlen--)
                            {
                            if((err = foutc((intl) *dp++, fout)) != 0)
                                return(err);
                            --len;
                            }
                        dp = curlfi->sdata;
                        }
                    }

                /* pad with trailing nulls */
                while(len--)
                    if((err = foutc(0, fout)) != 0)
                        return(err);
                }
            }
        }
    return(0);
}

/*************************************
*                                    *
* output byte to compiled expression *
*                                    *
*************************************/

static void
ebout(uchar byt)
{
    *expop++ = byt;
}

/***************************************
*                                      *
* output double to compiled expression *
*                                      *
***************************************/

static void
edout(fpval)
double fpval;
{
#if MS

    *(((double *) expop)++) = fpval;

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        double fpval;
        uchar fpbytes[8];
        } fp;
    intl i;

    fp.fpval = fpval;
    for(i = 4; i < 8; ++i)
        *expop++ = fp.fpbytes[i];
    for(i = 0; i < 4; ++i)
        *expop++ = fp.fpbytes[i];

#endif
}

/**********************************************
*                                             *
* output unsigned word to compiled expression *
*                                             *
**********************************************/

static void
euwout(uword16 wrd)
{
#if MS

    *(((uword16 *) expop)++) = wrd;

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        uword16 uword;
        uchar uwbytes[2];
        } uw;

    uw.uword = wrd;
    *expop++ = uw.uwbytes[0];
    *expop++ = uw.uwbytes[1];

#endif
}

/********************************************
*                                           *
* output signed word to compiled expression *
*                                           *
********************************************/

static void
ewout(word16 wrd)
{
#if MS

    *(((word16 *) expop)++) = wrd;

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        word16 word;
        uchar wbytes[2];
        } w;

    w.word = wrd;
    *expop++ = w.wbytes[0];
    *expop++ = w.wbytes[1];

#endif
}

/**********************************************
*                                             *
* search loaded PD file for column constructs *
* and note their positions in the array       *
*                                             *
**********************************************/

static intl
findcolumns()
{
    uchar huge *fp = pdf;
    intl i, lastcol = -1;

    maxcol = 0;
    for(i = 0; i < LOTUS_MAXCOL; ++i)
        {
        colcur[i] = NULL;
        colwid[i] = 0;
        }

    while((fp = searchcon(fp, pdend, (uchar *) "CO:")) != 0)
        {
        intl col, cw, ww, cr1, cr2;
        uchar huge *tp = fp, huge *fpend = fp - 4;
        uchar tstr[12];

        for(i = 0; i < 12; ++i)
            tstr[i] = *tp++;

        cr1 = stox(tstr, &col);

        i = sscanf((char *) tstr + cr1, ",%d,%d%n", &cw, &ww, &cr2);
        if((i == 2) && (*(fp + cr1 + cr2) == '%'))
            {
            fp += cr1 + cr2 + 1;
            maxcol = col + 1;
            colcur[col] = fp;
            colwid[col] = cw;
            if(lastcol != -1)
                colend[lastcol] = fpend;
            lastcol = col;
            }
        }

    if(lastcol != -1)
        colend[lastcol] = pdend;
    return(maxcol);
}

/*****************************************************
*                                                    *
* make a lotus form absolute/relative slot reference *
*                                                    *
*****************************************************/

static word16
makeabr(uword16 ref, intl csl)
{
    return(ref & 0x8000 ? (word16) (ref & 0x3FFF)
                        : ((word16) (((intl) ref - csl)) & 0x3FFF)
                        | 0x8000);
}

/***********************************
*                                  *
* strnicmp for ANSI only compilers *
*                                  *
***********************************/

static intl
my_strnicmp(string1, string2, count)
uchar *string1, *string2;
intl count;
{
    #if MS
    return(strnicmp(string1, string2, count));
    #else

    intl i;
    uchar a, b;

    IGNOREPARM(count);
    for(i = 0; i < strlen((char *) string1); ++i)
        {
        a = toupper(*string1);
        b = toupper(*string2);

        if(a != b)
            return((a > b) ? 1 : -1);

        ++string1;
        ++string2;
        }

    return(0);

    #endif
}

/************************************
*                                   *
* scan a symbol from the expression *
*                                   *
************************************/

static intl
nxtsym()
 {
    uchar cc = *exppos;
    uchar fstr[25], *fs;
    intl cr, co = 0, ts;

    /* special date scanning */
    switch(csym.scandate--)
        {
        case 7:
            return(csym.symno = SYM_OBRACKET);
        case 6:
            csym.fpval = (double) csym.yr;
            return(csym.symno = LF_INTEGER);
        case 5:
            return(csym.symno = SYM_COMMA);
        case 4:
            csym.fpval = (double) csym.mon;
            return(csym.symno = LF_INTEGER);
        case 3:
            return(csym.symno = SYM_COMMA);
        case 2:
            csym.fpval = (double) csym.day;
            return(csym.symno = LF_INTEGER);
        case 1:
            return(csym.symno = SYM_CBRACKET);

        default:
            break;
        }

    /* check for end of expression */
    if(!cc)
        return(csym.symno = LF_END);

    /* check for constant */
    if(isdigit(cc) || (cc == '.'))
        {
        if((ts = reccon(exppos, &csym.fpval, &csym.day,
                                             &csym.mon,
                                             &csym.yr, &cr)) != 0)
            {
            exppos += cr;
            switch(ts)
                {
                case PD_DATE:
                    csym.scandate = 7;
                    flookup((uchar *) "datef");
                    csym.symno = SYM_FUNC;
                    break;
                case PD_INTEGER:
                    csym.symno = LF_INTEGER;
                    break;
                case PD_REAL:
                    csym.symno = LF_CONST;
                    break;
                }
            return(csym.symno);
            }
        }

    /* check for slot reference/range */
    if(isalpha(cc) || (cc == '$'))
        {
        if((cr = scnslr(exppos, &csym.stcol, &csym.strow)) != 0)
            {
            exppos += cr;
            /* check for another SLR to make range */
            if((cr = scnslr(exppos, &csym.encol, &csym.enrow)) != 0)
                {
                exppos += cr;
                return(csym.symno = LF_RANGE);
                }

            return(csym.symno = LF_SLR);
            }
        }

    /* check for function */
    if(isalpha(cc))
        {
        fs = fstr;
        do
            {
            *fs++ = tolower(cc);
            ++co;
            cc = *(++exppos);
            }
        while((isalpha(cc) || isdigit(cc)) && (co < 24));

        *fs = '\0';
        flookup(fstr);
        if(csym.symno != SYM_BAD)
            return(csym.symno = SYM_FUNC);
        return(SYM_BAD);
        }

    /* check for string */
    if((cc == '"') || (cc == '\''))
        {
        fs = exppos + 1;
        while(*fs && (*fs != cc))
            ++fs;
        if(*fs != cc)
            return(SYM_BAD);
        *fs = '\0';
        csym.stringp = exppos + 1;
        exppos = fs + 1;
        return(csym.symno = LF_STRING);
        }

    /* check for special operators */
    switch(cc)
        {
        case '(':
            ++exppos;
            return(csym.symno = SYM_OBRACKET);
        case ')':
            ++exppos;
            return(csym.symno = SYM_CBRACKET);
        case ',':
            ++exppos;
            return(csym.symno = SYM_COMMA);
        default:
            break;
        }

    /* check for operator */
    exppos += olookup(exppos);
    return(csym.symno);
}

/***********************************
*                                  *
* process decimal places construct *
*                                  *
***********************************/

static uchar *
procdcp(constr, body, dcp)
uchar *constr, *body;
intl *dcp;
{
    sscanf((char *) body, "%d", dcp);
    return(constr);
}

/****************************
*                           *
* process percent construct *
*                           *
****************************/

static uchar *
procpct(constr, body, dcp)
uchar *constr, *body;
intl *dcp;
{
    IGNOREPARM(body);
    IGNOREPARM(dcp);
    /* return pointer one past 1st percent
    to leave a percent character behind */
    return(constr + 1);
}

/*******************************
*                              *
* search PD file for construct *
*                              *
*******************************/

static uchar huge *
searchcon(startp, endp, conid)
uchar huge *startp, huge *endp;
uchar *conid;
{
    intl tlen, conlen = strlen((char *) conid);
    uchar *tp, huge *ip;

    if(endp > startp)
        {
        do
            {
            if(*startp++ == '%')
                {
                tlen = conlen;
                tp = conid;
                ip = startp;
                do
                    {
                    if(toupper(*ip) != toupper(*tp))
                        break;
                    ++ip;
                    ++tp;
                    }
                while(--tlen);
                if(!tlen)
                    return(ip);
                }
            }
        while(startp < endp);
        }

    return(NULL);
}

/****************************
*                           *
* search PD file for option *
*                           *
****************************/

static uchar huge *
searchopt(optid)
uchar *optid;
{
    uchar huge *curp = pdf, huge *endp = optend ? optend : pdend;
    uchar huge *oldp = pdf;
    intl tlen, optlen = strlen((char *) optid);
    uchar *tp, huge *ip;

    while((curp = searchcon(curp, endp, (uchar *) "OP")) != 0)
        {
        if(*curp++ != '%')
            continue;

        tlen = optlen;
        tp = optid;
        ip = curp;
        do
            {
            if(toupper(*ip) != toupper(*tp))
                break;
            ++ip;
            ++tp;
            }
        while(--tlen);

        if(!tlen)
            return(ip);

        oldp = curp;
        }

    /* set pointer to last option */
    optend = oldp;
    return(NULL);
}

/***************************************
*                                      *
* recognise a constant and classify    *
*                                      *
* --out--                              *
* 0 - no constant found                *
* PD_DATE found date - in day, mon, yr *
* PD_INTEGER integer - in fpval        *
* PF_REAL double - in fpval            *
*                                      *
***************************************/

static intl
reccon(slot, fpval, day, mon, yr, cs)
uchar *slot;
double *fpval;
intl *day, *mon, *yr, *cs;
{
    intl res;
    uchar tstr[25], *tp;

    /* check for date without brackets */
    if((res = sscanf((char *) slot, "%d.%d.%d%n", day, mon, yr, cs)) == 3)
        {
        #if MS
        /* check for sscanf bug */
        if(!(*(slot + *cs - 1)))
            --(*cs);
        #endif
        return(PD_DATE);
        }

    /* check for date with brackets */
    if((res = sscanf((char *) slot, "(%d.%d.%d)%n", day, mon, yr, cs)) == 3)
        {
        /* check for sscanf bug */
        #if MS
        if(!(*(slot + *cs - 1)))
            --(*cs);
        #endif
        return(PD_DATE);
        }

    /* a fuddle cos sscanf doesn't get ".5", needs "0.5" */
    tp = tstr;
    if(*slot == '.')
        *tp++ = '0';
    strncpy((char *) tp, (char *) slot, 25);
    if((res = sscanf((char *) tstr, "%lf%n", fpval, cs)) > 0)
        {
        /* account for inserted zero */
        if(tp != tstr)
            --(*cs);

        /* check for sscanf bug */
        #if MS
        if(!(*(slot + *cs - 1)))
            --(*cs);
        #endif

        if((floor(*fpval) == *fpval) &&
           (*fpval < 32767.) && (*fpval > -.32767))
            return(PD_INTEGER);
        else
            return(PD_REAL);
        }

    /* ensure that cs is zero - some sscanfs
    don't set it to zero if the scan nowt */
    *cs = 0;
    return(0);
}

/********************************************
*                                           *
* show the current row number on the screen *
*                                           *
********************************************/

#ifdef UTIL_PTL

static intl
showrow(row)
word32 row;
{
    printf("\rRow: %ld", (long) row);
    return(0);
}

#endif

/************************
*                       *
* scan a slot reference *
*                       *
************************/

intl
scnslr(exppos, col, row)
uchar *exppos;
uword16 *col, *row;
{
    uchar *c = exppos;
    uword16 absc = 0, absr = 0;
    intl cr, res;
    intl tc, tr;

    if(*c == '$')
        {
        ++c;
        absc = 0x8000;
        }

    if((cr = stox(c, &tc)) == 0)
        return(0);
    *col = (uword16) (tc & 0xFF);

    c += cr;
    if(*c == '$')
        {
        ++c;
        absr = 0x8000;
        }

    if(!isdigit(*c))
        return(0);

    res = sscanf((char *) c, "%d%n", &tr, &cr);
    if((res < 1) || !cr)
        return(0);

    *row = (uword16) (tr & 0x3FFF);

    #if MS
    /* check for sscanf bug */
    if(!(*(c + cr - 1)))
        --cr;
    #endif

    --(*row);
    *col |= absc;
    *row |= absr;

    return(c - exppos + cr);
}

/*******************************************
*                                          *
* convert column string into column number *
*                                          *
*******************************************/

static intl
stox(string, col)
uchar *string;
intl *col;
{
    intl cr = 0, i, tcol;

    i = toupper(*string) - 'A';
    ++string;
    if((i >= 0) && (i <= 25))
        {
        tcol = i;
        cr = 1;
        i = toupper(*string) - 'A';
        ++string;
        if((i >= 0) && (i <= 25))
            {
            tcol = (tcol + 1) * 26 + i;
            cr = 2;
            }
        *col = tcol;
        }

    return(cr);
}

/*****************************
*                            *
* write out calculation mode *
*                            *
*****************************/

static intl
wrlcalcmode()
{
    intl err;
    uchar huge *optp;
    intl ofmt;

    optp = searchopt((uchar *) "AM");
    if(optp)
        ofmt = (toupper(*optp) == 'M') ? 0 : 0xFF;
    else
        ofmt = searchdefo((uchar *) "AM");

    if((err = writeins(curlfi)) != 0)
        return(err);
    return(foutc(ofmt, fout));
}

/******************************
*                             *
* write out calculation order *
*                             *
******************************/

static intl
wrlcalcord()
{
    intl err, ofmt;
    uchar huge *optp;

    optp = searchopt((uchar *) "RC");
    if(optp)
        {
        switch(toupper(*optp))
            {
            case 'C':
                ofmt = 1;
                break;
            case 'R':
                ofmt = 0xFF;
                break;
            default:
            case 'N':
                ofmt = 0;
                break;
            }
        }
    else
        ofmt = searchdefo((uchar *) "RC");

    if((err = writeins(curlfi)) != 0)
        return(err);
    return(foutc(ofmt, fout));
}

/***************************************
*                                      *
* write out all the column information *
*                                      *
***************************************/

static intl
wrcols()
{
    intl err, row = 0, col, didacol, i;
    intl dcp, slotbits;
    uchar huge *c, *constr;
    uchar slot[256], cc, *op;

    do
        {
        didacol = 0;
        for(col = 0; col < maxcol; ++col)
            {
            if(((c = colcur[col]) != 0) && (c < colend[col]))
                {
                constr = NULL;
                op = slot;
                slotbits = dcp = 0;
                while(cc = *c++, (cc != LF) && (cc != CR))
                    {
                    if(cc == '%')
                        {
                        /* look up a construct */
                        if(constr && (op - constr < 25))
                            {
                            conp pcons = NULL;

                            for(i = 0;
                                i < sizeof(constab)/sizeof(constr);
                                ++i)
                                {
                                uchar *c1 = constr + 1;
                                uchar *c2 = constab[i].conid;

                                while(isalpha(*c1) && (toupper(*c1) == *c2))
                                    {
                                    ++c1;
                                    ++c2;
                                    if(!*c2)
                                        {
                                        pcons = &constab[i];
                                        break;
                                        }
                                    }

                                *op = '\0';
                                if(pcons)
                                    {
                                    if(pcons->proccons)
                                        op = (*pcons->proccons)(constr,
                                                                c1,
                                                                &dcp);
                                    else
                                        op = constr;
                                    slotbits |= pcons->mask;
                                    break;
                                    }
                                }

                            if(!pcons)
                                *op++ = cc;
                            constr = NULL;
                            }
                        else
                            {
                            constr = op;
                            *op++ = cc;
                            }
                        }
                    else
                        {
                        *op++ = cc;
                        }
                    }

                *op++ = '\0';
                if((cc == LF) && (*c == CR))
                    ++c;
                else if((cc == CR) && (*c == LF))
                    ++c;

                /* save position in column */
                colcur[col] = c;

                /* now write out slot contents */
                if(strlen((char *) slot))
                    if((err = writeslot(slot, col, row, slotbits, dcp)) != 0)
                        return(err);
                didacol = 1;
                }
            }
        ++row;

        /* check for the file getting too big */
        if(row >= LOTUS_MAXROW)
            return(PD123_ERR_BIGFILE);

        if(counter)
            if((*counter)((word32) ((colcur[0] - pdf) * 100) /
                                    (colend[0] - pdf + 1)))
                break;
        }
    while(didacol);
    return(0);
}

/*********************************
*                                *
* write out column width records *
*                                *
*********************************/

static intl
wrcolws()
{
    intl err, i;

    for(i = 0; i < maxcol; ++i)
        {
        intl cwid = colwid[i];

        /* weed out hidden cols and default widths */
        if(!cwid || (cwid == 9))
            continue;

        if((err = writeins(curlfi)) != 0)
            return(err);
        if((err = writeuword(i)) != 0)
            return(err);
        if((err = foutc((uchar) cwid, fout)) != 0)
            return(err);
        }
    return(0);
}

/*******************
*                  *
* write out footer *
*                  *
*******************/

static intl
wrfooter()
{
    return(wrlheadfoot((uchar *) "FO"));
}

/*************************
*                        *
* write out graph record *
*                        *
*************************/

static intl
wrgraph()
{
    static uchar part1[] = "\xFF\xFF\x0\x0";
    static uchar part2[] = "\x4\x0\x0\x3\x3\x3\x3\x3\x3";
    static uchar part3[] = "\x71\x71\x1\x0\x0\x0";
    intl l1 = 4, l2 = 9, l3 = 6;
    intl err, i, x;
    uchar *c;

    if((err = writeins(curlfi)) != 0)
        return(err);

    for(i = 0; i < 26; ++i)
        for(x = 0, c = part1; x < l1; ++x)
            if((err = foutc(*c++, fout)) != 0)
                return(err);

    for(i = 0, c = part2; i < l2; ++i)
        if((err = foutc(*c++, fout)) != 0)
            return(err);

    for(i = 0; i < 320; ++i)
        if((err = foutc(0, fout)) != 0)
            return(err);

    for(i = 0, c = part3; i < l3; ++i)
        if((err = foutc(*c++, fout)) != 0)
            return(err);

    return(0);
}

/*******************
*                  *
* write out header *
*                  *
*******************/

static intl
wrheader()
{
    return(wrlheadfoot((uchar *) "HE"));
}

/*****************************
*                            *
* write out header or footer *
*                            *
*****************************/

static intl
wrlheadfoot(optid)
uchar *optid;
{
    intl err, len;
    uchar huge *optp;
    uchar delim, co, tstr[10];

    if((err = writeins(curlfi)) != 0)
        return(err);

    len = curlfi->length;
    optp = searchopt(optid);
    if(optp)
        {
        delim = *optp++;

        while((*optp != CR) && (*optp != LF) && (optp < pdend) && len)
            {
            co = *optp++;
            if(co == delim)
                {
                co = '|';
                }
            else if(co == '@')
                {
                tstr[0] = co;
                tstr[1] = *optp;
                tstr[2] = *(optp + 1);

                if(!my_strnicmp(tstr, (uchar *) "@D@", 3))
                    {
                    co = '@';
                    optp += 2;
                    }
                else if(!my_strnicmp(tstr, (uchar *) "@P@", 3))
                    {
                    co = '#';
                    optp += 2;
                    }
                }

            if((err = foutc((len--, ichlotus(co)), fout)) != 0)
                return(err);
            }
        }

    while(len--)
        if((err = foutc(0, fout)) != 0)
            return(err);

    return(0);
}

/**************************************
*                                     *
* write out the hidden columns record *
*                                     *
**************************************/

static intl
wrhidvec()
{
    intl err, i;

    if((err = writeins(curlfi)) != 0)
        return(err);
    for(i = 0; i < 32; ++i)
        if((err = foutc((uchar) hidvec[i], fout)) != 0)
            return(err);
    return(0);
}

/********************************
*                               *
* write out column and row pair *
*                               *
********************************/

static intl
writecolrow(col, row)
intl col, row;
{
    intl err;

    if((err = writeuword(col)) != 0)
        return(err);
    return(writeuword(row));
}

/***************************************
*                                      *
* write a double out to the lotus file *
*                                      *
***************************************/

static intl
writedouble(fpval)
double fpval;
{
    intl err, i;
    union
        {
        double fpval;
        char fpbytes[8];
        } fp;

    fp.fpval = fpval;

#if MS

    for(i = 0; i < 8; ++i)
        if((err = foutc(fp.fpbytes[i], fout)) != 0)
            return(err);

#elif ARTHUR || RISCOS

    for(i = 4; i < 8; ++i)
        if((err = foutc(fp.fpbytes[i], fout)) != 0)
            return(err);

    for(i = 0; i < 4; ++i)
        if((err = foutc(fp.fpbytes[i], fout)) != 0)
            return(err);

#endif

    return(0);
}

/******************************************
*                                         *
* write out a lotus structure instruction *
*                                         *
******************************************/

static intl
writeins(curlfi)
lfip curlfi;
{
    intl err;

    if((err = writeuword((uword16) curlfi->opcode)) != 0)
        return(err);
    return((err = writeuword((uword16) curlfi->length)) != 0);
}

/******************************
*                             *
* write out a slot as a label *
*                             *
******************************/

static intl
writelabel(slot, col, row, mask)
uchar *slot;
intl col, row, mask;
{
    intl err;
    uchar align;

    /* write out label */
    if((err = writeuword(L_LABEL)) != 0)
        return(err);
    /* length: overhead + align + contents + null */
    if((err = writeuword((uword16) (5 + 1 + strlen((char *) slot) + 1))) != 0)
        return(err);
    if((err = foutc(0xFF, fout)) != 0)
        return(err);
    if((err = writecolrow(col, row)) != 0)
        return(err);

    /* default left alignment */
    align = '\'';
    if(mask & BIT_RYT)
        align = '"';
    if(mask & BIT_CEN)
        align = '^';

    if((err = foutc((intl) align, fout)) != 0)
        return(err);

    while(*slot)
        if((err = foutc(ichlotus((intl) *slot++), fout)) != 0)
            return(err);

    return(foutc(0, fout));
}

/*******************
*                  *
* write out a date *
*                  *
*******************/

static intl
writeldate(day, mon, yr, col, row)
intl day, mon, yr, col, row;
{
    intl err, i;

    /* write out length and date format */
    if((err = writeuword(L_FORMULA)) != 0)
        return(err);
    if((err = writeuword(26)) != 0)
        return(err);
    if((err = foutc(L_PROT | L_SPECL | L_DDMMYY, fout)) != 0)
        return(err);
    if((err = writecolrow(col, row)) != 0)
        return(err);

    /* write out value */
    for(i = 0; i < 4; ++i)
        if((err = writeuword(0)) != 0)
            return(err);

    /* write out @DATE(yr, mon, day) expression */
    if((err = writeuword(11)) != 0)
        return(err);
    if((err = foutc(LF_INTEGER, fout)) != 0)
        return(err);
    if((err = writeuword(yr)) != 0)
        return(err);
    if((err = foutc(LF_INTEGER, fout)) != 0)
        return(err);
    if((err = writeuword(mon)) != 0)
        return(err);
    if((err = foutc(LF_INTEGER, fout)) != 0)
        return(err);
    if((err = writeuword(day)) != 0)
        return(err);

    if((err = foutc(LF_DATE, fout)) != 0)
        return(err);
    if((err = foutc(LF_END, fout)) != 0)
        return(err);

    return(0);
}

/*************************************************
*                                                *
* write out lotus format byte given mask and dcp *
*                                                *
*************************************************/

static intl
writelformat(mask, dcp)
intl mask, dcp;
{
    intl ofmt = -1;

    /* ensure sensible value in dcp */
    if(!(mask & BIT_DCN))
        {
        if(decplc == -1)
            dcp = 2;
        else
            dcp = decplc;
        }

    /* priority: GENERAL>BRACKETS>DEC.PLACES */
    /* check for decimal places */
    if(mask & BIT_DCN)
        ofmt = L_FIXED | dcp;

    /* check for brackets needed */
    if(mask & BIT_BRK)
        {
        if(mask & BIT_CUR)
            ofmt = L_CURCY | dcp;
        else
            ofmt = L_COMMA | dcp;
        }

    /* check for general format */
    if(mask & BIT_DCF)
        ofmt = L_SPECL | L_GENFMT;

    if(ofmt == -1)
        ofmt = L_SPECL | L_DEFAULT;

    return(foutc(L_PROT | ofmt, fout));
}

/*******************************
*                              *
* write out slot to lotus file *
*                              *
*******************************/

static intl
writeslot(uchar *slot, intl col, intl row, uword16 mask, intl dcp)
{
    intl err, res, day, mon, yr, cs, len;
    uchar *ep;

    if(!(mask & BIT_EXP))
        {
        if((err = writelabel(slot, col, row, mask)) != 0)
            return(err);
        }
    else
        {
        double fpval;

        /* try to interpret the slot as an integer/float/date */
        res = reccon(slot, &fpval, &day, &mon, &yr, &cs);

        /* check characters were scanned and
        that we got to the end of the slot */
        if(!cs || (*(slot + cs)))
            res = 0;

        switch(res)
            {
            case PD_REAL:
                /* output floating value */
                if((err = writesslot(L_NUMBER, 13, col, row, mask, dcp)) != 0)
                    return(err);

                return(writedouble(fpval));
                break;

            case PD_INTEGER:
                /* output integer */
                if((err = writesslot(L_INTEGER, 7, col, row, mask, dcp)) != 0)
                    return(err);

                return(writeword((word16) fpval));
                break;

            case PD_DATE:
                return(writeldate(day, mon, yr, col, row));
                break;

            default:
                /* must be expression */
                if((len = compileexp(slot, col, row)) != 0)
                    {
                    if((err = writesslot(L_FORMULA, len + 15, col, row, mask, dcp)) != 0)
                        return(err);

                    if((err = writedouble(0.)) != 0)
                        return(err);
                    ep = expbuf;
                    writeuword(len);
                    while(len--)
                        if((err = foutc(*ep++, fout)) != 0)
                            return(err);
                    }
                else
                    {
                    /* write out bad expression as a label */
                    if((err = writelabel(slot, col, row, mask)) != 0)
                        return(err);
                    ++errexp;
                    }
                break;
            }
        }

    return(0);
}

/**************************
*                         *
* write out start of slot *
*                         *
**************************/

static intl
writesslot(intl opc, intl len, intl col, intl row, uword16 mask, intl dcp)
{
    intl err;

    if((err = writeuword(opc)) != 0)
        return(err);
    if((err = writeuword(len)) != 0)
        return(err);
    if((err = writelformat(mask, dcp)) != 0)
        return(err);
    return(writecolrow(col, row));
}

/***********************************************
*                                              *
* write an unsigned word out to the lotus file *
*                                              *
***********************************************/

static intl
writeuword(uword16 aword)
{
    intl err;
    union
        {
        uword16 uword;
        uchar uwbytes[2];
        } uw;

    uw.uword = aword;
    if((err = foutc(uw.uwbytes[0], fout)) != 0)
        return(err);
    return(foutc(uw.uwbytes[1], fout));
}

/********************************************
*                                           *
* write a signed word out to the lotus file *
*                                           *
********************************************/

static intl
writeword(word16 aword)
{
    intl err;
    union
        {
        word16 word;
        char wbytes[2];
        } w;

    w.word = aword;
    if((err = foutc(w.wbytes[0], fout)) != 0)
        return(err);
    return(foutc(w.wbytes[1], fout));
}

/***************************
*                          *
* write out a margin value *
*                          *
***************************/

static intl
wrlmar(optid)
uchar *optid;
{
    intl i;
    uchar huge *optp;
    intl curv;
    uchar tstr[5];

    optp = searchopt(optid);
    if(optp)
        {
        for(i = 0; i < 5; ++i)
            tstr[i] = *optp++;
        if(sscanf((char *) tstr, "%d", &curv) < 1)
            optp = NULL;
        }

    if(!optp)
        curv = searchdefo(optid);

    return(writeuword(curv));
}

/************************
*                       *
* write out the margins *
*                       *
************************/

static intl
wrlmargins()
{
    intl err;

    if((err = writeins(curlfi)) != 0)
        return(err);

    if((err = wrlmar((uchar *) "LM")) != 0)
        return(err);
    if((err = writeuword(76)) != 0)
        return(err);
    if((err = wrlmar((uchar *) "PL")) != 0)
        return(err);
    if((err = wrlmar((uchar *) "TM")) != 0)
        return(err);
    return(wrlmar((uchar *) "BM"));
}

/********************************
*                               *
* write out window1 information *
*                               *
********************************/

static intl
wrwindow1()
{
    intl err, i;
    uchar huge *optp;
    intl ofmt, widleft, ncols;

    if((err = writeins(curlfi)) != 0)
        return(err);
    /* current cursor column */
    if((err = writeuword(0)) != 0)
        return(err);
    /* current cursor row */
    if((err = writeuword(0)) != 0)
        return(err);

    /* work out format byte */
    optp = searchopt((uchar *) "DP");
    if(optp)
        {
        if(toupper(*optp) == 'F')
            {
            ofmt = L_SPECL | L_GENFMT;
            decplc = -1;
            }
        else
            {
            uchar huge *tp = optp;
            uchar tstr[12];

            for(i = 0; i < 12; ++i)
                tstr[i] = *tp++;

            sscanf((char *) tstr, "%d", &decplc);
            ofmt = decplc | L_FIXED;
            optp = searchopt((uchar *) "MB");
            if(optp && (toupper(*optp) == 'B'))
                ofmt = decplc | L_COMMA;
            }
        }
    else
        {
        ofmt = searchdefo((uchar *) "DP");
        }

    /* write out format byte */
    if((err = foutc(ofmt, fout)) != 0)
        return(err);
    /* write out padder */
    if((err = foutc(0, fout)) != 0)
        return(err);
    /* write out default column width */
    if((err = writeuword(9)) != 0)
        return(err);

    /* calculate columns on screen and set up hidden vector */
    widleft = 76;
    for(i = ncols = 0; i < maxcol; ++i)
        {
        if(!colwid[i])
            {
            hidvec[(i >> 3)] |= (1 << (i & 7));
            }
        else
            {
            if(widleft > 0)
                {
                ++ncols;
                widleft -= colwid[i];
                }
            }
        }

    /* write out number of cols on screen */
    if((err = writeuword(ncols)) != 0)
        return(err);
    /* number of rows */
    if((err = writeuword(20)) != 0)
        return(err);

    /* col/row left/top, #title col/rows,
    title col/row left/top */
    for(i = 0; i < 6; ++i)
        if((err = writeuword(0)) != 0)
            return(err);

    /* border width column/row */
    if((err = writeuword(4)) != 0)
        return(err);
    if((err = writeuword(4)) != 0)
        return(err);

    /* window width */
    if((err = writeuword(76)) != 0)
        return(err);
    /* padding */
    return(err = writeuword(0));
}

#endif

/* #endif for big #ifndef LOTUS_OFF */

/* end of 123pd.c */
