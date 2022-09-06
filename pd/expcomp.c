/* expcomp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/****************************
*                           *
* infix to RPN compiler     *
* for PipeDream expressions *
*                           *
* MRJC                      *
* May 1988                  *
*                           *
****************************/

#include "datafmt.h"

/* local header file */
#include "eval.h"

/*
function declarations
*/

extern intl exp_compile(uchar *, char *, BOOL *, intl);
extern intl exp_decompile(char *, uchar *);
extern intl exp_refersto_next(uchar **exp, intl *state, uchar **arg,
                              SLR *refto, SLR *reftoe);
extern uchar *exp_findslr(uchar *);
extern void exp_initslr(void);
extern intl exp_len(uchar *);
extern uword32 readuword(uchar *from, intl size);
extern intl slrbinstr(char *, uchar *);
extern void writeuword(uchar *to, uword32 num, intl size);

/*static intl chkspace(intl needed);*/
static intl chksym(void);
static uchar *convconst(intl, uchar *);
static void convslr(uchar *, intl *, uchar *, intl);
static intl dbase_refersto(uchar **exp, uchar **arg,
                           SLR *refto, SLR *reftoe);
/*static void docnout(docno doc);*/
static void ebout(uchar byt);
/*static void edout(double);*/
/*static void eluwout(uword32);*/
/*static void ettout(time_t);*/
/*static void ewout(word16 wrd);*/
/*static intl flookup(uchar *);*/
static intl fptostr(uchar *, double);
static void freestk(void);
static void laexpr(void);
static void laterm(void);
static void lbterm(void);
static void lcterm(void);
static void ldterm(void);
static void leterm(void);
static void lfterm(void);
static void lgterm(void);
static void lterm(void);
/*static void lzelement(void);*/
static intl nxtsym(void);
static intl procfunc(oprp);
static double readdouble(uchar *);
static time_t readtt(uchar *);
/*static uword32 readuword32(uchar *);*/
static word16 readword16(uchar *);
static intl reccon(uchar *, double *, intl *);
static intl scnslr(uchar *, docno *, uword32 *, uword32 *, intl);
static intl scnsym(void);
static void slrout(docno, uword32, uword32);
static intl stox(uchar *, uword32 *);
static intl str_refersto(uchar **exp, SLR *refto);

#if MS
static void euwout(uword16);
static uword16 readuword16(uchar *);
#endif

/*
* compiler IO variables
*/

static uchar *expstart;             /* start of output buffer */
static uchar *expop;                /* position in output */
static intl expolen;                /* maximum length of output */
static uchar *exppos;               /* position in input */
static struct symbol csym;          /* details of symbol scanned */
static BOOL slrbits;                /* slrbits accumulator */
static intl dbasearg;               /* processing argument to D... function */

/* symbol checker macro */
#define chknxs() ((csym.symt != SYM_BLANK) ? csym.symt : nxtsym())

/*
* decompiler variables
*/

#define MAXSTACK 100

static uchar *termp;               /* scanner index */
static intl curtok;                /* current symbol */
static uchar *argstk[MAXSTACK];    /* stack of arguments */
static intl argsp;                 /* argument stack pointer */
static oprp curopr;                /* current symbol structure */

/* findslr state */
static intl inrange;
static intl instring;

/* days in the month */
intl days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* constants for tree routines */
#define TREE_NORMAL 0
#define TREE_IN_STRING 1
#define TREE_IN_DBASE 2

/***********************************
*                                  *
* table of operators and functions *
*                                  *
***********************************/

struct oprdef oprtab[] =
{
    TYPE_CONST,      0, 0,  (uchar *) "",         NULL,      /* REAL */
    TYPE_CONST,      0, 0,  (uchar *) "",         NULL,      /* SLR */
    TYPE_CONST,      0, 0,  (uchar *) "",         NULL,      /* RANGE */
    TYPE_CONST,      0, 0,  (uchar *) "",         NULL,      /* WORD16 */
    TYPE_CONST,      0, 0,  (uchar *) "",         NULL,      /* WORD8 */
    TYPE_CONST,      0, 0,  (uchar *) "",         NULL,      /* STRINGS */
    TYPE_CONST,      0, 0,  (uchar *) "",         NULL,      /* STRINGD */
    TYPE_CONST,      0, 0,  (uchar *) "",         NULL,      /* DATE */
    TYPE_BRACKETS,   0, 0,  (uchar *) "",         NULL,      /* BRACKETS */
    TYPE_END,        0, 0,  (uchar *) "",         NULL,      /* END of expression */

    TYPE_UNARY,      1, 0,  (uchar *) "+",        c_uplus,   /* unary + */
    TYPE_UNARY,      1, 0,  (uchar *) "-",        c_umi,     /* unary - */

    TYPE_UNARY,      1, 0,  (uchar *) "!",        c_not,
    TYPE_BINARY,     2, 0,  (uchar *) "&",        c_and,
    TYPE_BINARY,     2, 0,  (uchar *) "*",        c_mul,
    TYPE_BINARY,     2, 0,  (uchar *) "+",        c_add,
    TYPE_BINARY,     2, 0,  (uchar *) "-",        c_sub,
    TYPE_BINARY,     2, 0,  (uchar *) "/",        c_div,
    TYPE_BINARYREL,  2, 0,  (uchar *) "<",        c_lt,
    TYPE_BINARYREL,  2, 0,  (uchar *) "<=",       c_lteq,
    TYPE_BINARYREL,  2, 0,  (uchar *) "<>",       c_neq,
    TYPE_BINARYREL,  2, 0,  (uchar *) "=",        c_eq,
    TYPE_BINARYREL,  2, 0,  (uchar *) ">",        c_gt,
    TYPE_BINARYREL,  2, 0,  (uchar *) ">=",       c_gteq,
    TYPE_BINARY,     2, 0,  (uchar *) "^",        c_power,
    TYPE_FUNC,       1, 0,  (uchar *) "abs",      c_abs,
    TYPE_FUNC,       1, 0,  (uchar *) "acs",      c_acs,
    TYPE_FUNC,       1, 0,  (uchar *) "asn",      c_asn,
    TYPE_FUNC,       1, 0,  (uchar *) "atn",      c_atn,
    TYPE_FUNC,       2, 0,  (uchar *) "atn2",     c_atn2,
    TYPE_FUNC,      -1, 0,  (uchar *) "avg",      c_avg,
    TYPE_FUNC,      -1, 0,  (uchar *) "choose",   c_choose,
    TYPE_FUNC,      -1, 1,  (uchar *) "col",      c_col,
    TYPE_FUNC,       1, 0,  (uchar *) "cos",      c_cos,
    TYPE_FUNC,      -1, 0,  (uchar *) "count",    c_count,
    TYPE_FUNC,       3, 0,  (uchar *) "cterm",    c_cterm,
    TYPE_FUNC,       0, 0,  (uchar *) "date",     c_today,
    TYPE_FUNC,       2, 2,  (uchar *) "davg",     c_davg,
    TYPE_FUNC,       1, 0,  (uchar *) "day",      c_day,
    TYPE_FUNC,       2, 2,  (uchar *) "dcount",   c_dcount,
    TYPE_FUNC,       2, 2,  (uchar *) "dcounta",  c_dcounta,
    TYPE_FUNC,       4, 0,  (uchar *) "ddb",      c_ddb,
    TYPE_FUNC,       1, 0,  (uchar *) "deg",      c_deg,
    TYPE_FUNC,       2, 2,  (uchar *) "dmax",     c_dmax,
    TYPE_FUNC,       2, 2,  (uchar *) "dmin",     c_dmin,
    TYPE_FUNC,       2, 2,  (uchar *) "dstd",     c_dstd,
    TYPE_FUNC,       2, 2,  (uchar *) "dsum",     c_dsum,
    TYPE_FUNC,       2, 2,  (uchar *) "dvar",     c_dvar,
    TYPE_FUNC,       1, 0,  (uchar *) "exp",      c_exp,
    TYPE_FUNC,       3, 0,  (uchar *) "fv",       c_fv,
    TYPE_FUNC,       3, 0,  (uchar *) "hlookup",  c_hlookup,
    TYPE_FUNC,       3, 0,  (uchar *) "if",       c_if,
    TYPE_FUNC,       2, 1,  (uchar *) "index",    c_index,
    TYPE_FUNC,       1, 0,  (uchar *) "int",      c_int,
    TYPE_FUNC,       2, 0,  (uchar *) "irr",      c_irr,
    TYPE_FUNC,       1, 0,  (uchar *) "ln",       c_ln,
    TYPE_FUNC,       1, 0,  (uchar *) "log",      c_log,
    TYPE_FUNC,       3, 0,  (uchar *) "lookup",   c_lookup,
    TYPE_FUNC,       0, 1,  (uchar *) "loopc",    c_loopc,
    TYPE_FUNC,      -1, 0,  (uchar *) "max",      c_max,
    TYPE_FUNC,      -1, 0,  (uchar *) "min",      c_min,
    TYPE_FUNC,       2, 0,  (uchar *) "mod",      c_mod,
    TYPE_FUNC,       1, 0,  (uchar *) "month",    c_month,
    TYPE_FUNC,       1, 0,  (uchar *) "monthdays",c_monthdays,
    TYPE_FUNC,       2, 0,  (uchar *) "npv",      c_npv,
    TYPE_FUNC,       0, 0,  (uchar *) "pi",       c_pi,
    TYPE_FUNC,       3, 0,  (uchar *) "pmt",      c_pmt,
    TYPE_FUNC,       3, 0,  (uchar *) "pv",       c_pv,
    TYPE_FUNC,       1, 0,  (uchar *) "rad",      c_rad,
    TYPE_FUNC,      -1, 1,  (uchar *) "rand",     c_rand,
    TYPE_FUNC,       3, 0,  (uchar *) "rate",     c_rate,
    TYPE_FUNC,       3, 1,  (uchar *) "read",     c_read,
    TYPE_FUNC,       2, 0,  (uchar *) "round",    c_round,
    TYPE_FUNC,      -1, 1,  (uchar *) "row",      c_row,
    TYPE_FUNC,       1, 0,  (uchar *) "sgn",      c_sgn,
    TYPE_FUNC,       1, 0,  (uchar *) "sin",      c_sin,
    TYPE_FUNC,       3, 0,  (uchar *) "sln",      c_sln,
    TYPE_FUNC,       1, 0,  (uchar *) "sqr",      c_sqr,
    TYPE_FUNC,      -1, 0,  (uchar *) "std",      c_std,
    TYPE_FUNC,      -1, 0,  (uchar *) "sum",      c_sum,
    TYPE_FUNC,       4, 0,  (uchar *) "syd",      c_syd,
    TYPE_FUNC,       1, 0,  (uchar *) "tan",      c_tan,
    TYPE_FUNC,       3, 0,  (uchar *) "term",     c_term,
    TYPE_FUNC,      -1, 0,  (uchar *) "var",      c_var,
    TYPE_FUNC,       3, 0,  (uchar *) "vlookup",  c_vlookup,
    TYPE_FUNC,       4, 1,  (uchar *) "write",    c_write,
    TYPE_FUNC,       1, 0,  (uchar *) "year",     c_year,
    TYPE_BINARY,     2, 0,  (uchar *) "|",        c_or,
};

/***************************************************************************
*                                                                          *
* Grammar:                                                                 *
*                                                                          *
*    <expr>  := <aterm>  { <group6><aterm> }                               *
*    <aterm> := <bterm>  { <group5><bterm> }                               *
*    <bterm> := <cterm>  { <group4><cterm> }                               *
*    <cterm> := <dterm>  { <group3><dterm> }                               *
*    <dterm> := <eterm>  { <group2><eterm> }                               *
*    <eterm> := <fterm>  { <group1><fterm> }                               *
*    <fterm> := { <unary><fterm> } | <gterm>                               *
*    <gterm> := <term>  | (<expr>)                                         *
*    <term>  := slotref | constant | <function> | string                   *
*    <function> := <identa> | <identb> (expr) | <identc> (<list>)          *
*    <list>  := <element> {,<element>}                                     *
*    <element> := range | <expr>                                           *
*                                                                          *
*    <unary> := + | - | !                                                  *
*    <group1> := ^                                                         *
*    <group2> := * | / | %                                                 *
*    <group3> := + | -                                                     *
*    <group4> := = | <> | < | > | <= | >=                                  *
*    <group5> := &                                                         *
*    <group6> := |                                                         *
*    <identb> := abs | acs | asn | atn | cos | day | deg | dmonth | exp |  *
*                int | ln | log | month | rad |                            *
*                sgn | sin | sqr | tan | year                              *
*                                                                          *
*    <identc> := avg | choose | count | if | index | lookup |              *
*                max | min | mod | read | round | sum | write | hlookup    *
*                cterm | ddb | fv | irr| npv | pmt | pv | rate | sln | std *
*                syd | term | var | vlookup                                *
*                                                                          *
*                davg| dsum | dmax | dmin | dcount | dvar | dstd           *
*                                                                          *
***************************************************************************/

/****************************************
*                                       *
* compile PipeDream expression into RPN *
*                                       *
****************************************/

#if RISCOS
static jmp_buf compiler_safepoint;

#pragma -s1

static void
compiler_jmp_capture(int sig)
{
    longjmp(compiler_safepoint, sig);
}

#pragma -s0

#endif  /* RISCOS */

extern intl
exp_compile(uchar *buffer, char *slot, BOOL *slrflag, intl maxlen)
{
    intl res;
    #if RISCOS
    void (*oldstak)(int) = NULL;
    #endif

    /* default to no slrs */
    *slrflag = 0;

    /* return zero for blank slot */
    if(!*slot)
        return(0);

    #if RISCOS
    if(setjmp(compiler_safepoint))
        res = EVAL_ERR_STACKOVER;
    else
    #endif
        {
        #if RISCOS
        oldstak = signal(SIGSTAK, compiler_jmp_capture);
        #endif

        csym.symt = SYM_BLANK;
        expstart = buffer;
        expop = buffer;
        expolen = maxlen;
        exppos = (uchar *) slot;
        slrbits = 0;
        dbasearg = 0;

        laexpr();

        if(csym.symt != OPR_END)
            res = ERR_BAD_EXPRESSION;
        else
            {
            ebout(OPR_END);
            if(expop - expstart >= expolen)
                res = ERR_BAD_EXPRESSION;
            else
                {
                *slrflag = slrbits;
                res = expop - buffer;
                }
            }
        }

    #if RISCOS
    signal(SIGSTAK, oldstak);
    #endif

    return(res);
}


/***********************************
*                                  *
* decompile from RPN to plain text *
*                                  *
***********************************/

extern intl
exp_decompile(char *buffer, uchar *ep)
{
    intl oldsp;

    /* set scanner index */
    termp = ep;
    argsp = 0;

    chksym();
    do
        {
        switch(curopr->ftype)
            {
            case TYPE_CONST:
                if((argstk[argsp++] = convconst(curtok, termp + 1)) == 0)
                    return(ERR_NOROOM);
                if(argsp == MAXSTACK)
                    return(EVAL_ERR_STACKOVER);
                break;

            case TYPE_END:
                break;

            case TYPE_BRACKETS:
                {
                uchar *nele, *oele, *c;

                if(!argsp)
                    {
                    freestk();
                    return(ERR_BAD_EXPRESSION);
                    }

                oele = argstk[--argsp];

                /* add two for brackets and null */
                nele = fixed_malloc(strlen((char *) oele) + 2 + 1);
                if(!nele)
                    return(ERR_NOROOM);

                c = nele;
                *c++ = '(';
                strcpy((char *) c, (char *) oele);
                c += strlen((char *) oele);
                *c++ = ')';
                *c++ = '\0';
                fixed_free(oele);
                argstk[argsp++] = nele;
                break;
                }

            case TYPE_UNARY:
                {
                uchar *nele, *oele, *c;
                intl addlen;

                if(!argsp)
                    {
                    freestk();
                    return(ERR_BAD_EXPRESSION);
                    }

                oele = argstk[--argsp];
                addlen = strlen((char *) curopr->fname);
                /* extra for new operator and null */
                nele = fixed_malloc(strlen((char *) oele) + addlen + 1);
                if(!nele)
                    return(ERR_NOROOM);

                c = nele;
                strcpy((char *) c, (char *) curopr->fname);
                c += addlen;
                strcpy((char *) c, (char *) oele);
                c += strlen((char *) oele);
                *c++ = '\0';
                fixed_free(oele);
                argstk[argsp++] = nele;
                break;
                }

            case TYPE_BINARYREL:
            case TYPE_BINARY:
                {
                uchar *ele1, *ele2, *nele, *c;
                intl addlen, lele1, lele2;

                if(argsp < 2)
                    {
                    freestk();
                    return(ERR_BAD_EXPRESSION);
                    }

                ele2 = argstk[--argsp];
                ele1 = argstk[--argsp];
                lele1 = strlen((char *) ele1);
                lele2 = strlen((char *) ele2);
                addlen = strlen((char *) curopr->fname);
                /* two arguments, operator and null */
                nele = fixed_malloc(lele1 + lele2 + addlen + 1);
                if(!nele)
                    return(ERR_NOROOM);

                c = nele;
                strcpy((char *) c, (char *) ele1);
                c += lele1;
                strcpy((char *) c, (char *) curopr->fname);
                c += addlen;
                strcpy((char *) c, (char *) ele2);
                c += lele2;
                *c++ = '\0';
                fixed_free(ele1);
                fixed_free(ele2);
                argstk[argsp++] = nele;
                break;
                }

            case TYPE_FUNC:
                {
                intl narg, i, tlen, argc;
                uchar *nele, *c;

                /* work out number of arguments */
                if((narg = curopr->nargs) == -1)
                    narg = (intl) *(termp + 1);

                if(argsp < narg)
                    {
                    freestk();
                    return(ERR_BAD_EXPRESSION);
                    }

                tlen = 0;
                if(narg)
                    {
                    /* add up the length of all the arguments */
                    for(i = 1; i <= narg; ++i)
                        tlen += strlen((char *) argstk[argsp - i]);
                    /* add in space for commas and function brackets */
                    tlen += narg - 1 + 2;
                    }

                /* add length of name, null */
                tlen += strlen((char *) curopr->fname) + 1;
                nele = fixed_malloc(tlen);
                if(!nele)
                    return(ERR_NOROOM);

                c = nele;
                strcpy((char *) c, (char *) curopr->fname);
                c += strlen((char *) curopr->fname);
                argc = narg;
                if(narg)
                    {
                    *c++ = '(';
                    while(argc)
                        {
                        uchar *carg = argstk[argsp - (argc--)];
                        strcpy((char *) c, (char *) carg);
                        c += strlen((char *) carg);
                        if(argc)
                            *c++ = ',';
                        fixed_free(carg);
                        }
                    *c++ = ')';
                    }
                *c++ = '\0';
                argsp -= narg;
                argstk[argsp++] = nele;
                break;
                }
            }

        if(curtok == OPR_END)
            break;
        scnsym();
        }
    while(TRUE);

    strcpy(buffer, (char *) argstk[0]);
    oldsp = argsp;
    freestk();
    return((oldsp == 1) ? 0 : (ERR_BAD_EXPRESSION));
}

/*********************************************
*                                            *
* return the references in a slot one by one *
*                                            *
*********************************************/

extern intl
exp_refersto_next(uchar **exp, intl *state,
                  uchar **arg, SLR *refto, SLR *reftoe)
{
    intl res;

    termp = *exp;

    switch(*state)
        {
        /* initialise */
        case -1:
            *arg = NULL;
            break;

        /* in string */
        case TREE_IN_STRING:
            if((res = str_refersto(exp, refto)) >= 0)
                return(res);
            termp = *exp;
            *arg = NULL;
            break;

        /* in database function */
        case TREE_IN_DBASE:
            if((res = dbase_refersto(exp, arg, refto, reftoe)) >= 0)
                return(res);
            termp = *exp;
            *arg = NULL;
            break;
        }

    *state = TREE_NORMAL;
    chksym();

    do
        {
        docno doc;
        uchar *termpt;

        /* make copy of start of atom */

        switch(curtok)
            {
            case OPR_SLR:
                termpt = termp + 1;
                if((doc = (docno) readuword(termpt, sizeof(docno))) == 0)
                    doc = ensure_cur_docno();

                refto->doc = doc;
                termpt += sizeof(docno);

                refto->col = (colt) readuword(termpt, sizeof(colt)) &
                                    (COLNOBITS | BADCOLBIT);
                termpt += sizeof(colt);
                refto->row = (rowt) readuword(termpt, sizeof(rowt)) &
                                    (ROWNOBITS | BADROWBIT);

                *exp = termpt + sizeof(rowt);

                if(bad_col(refto->col) || bad_row(refto->row))
                    break;

                *arg = NULL;
                return(OPR_SLR);

            case OPR_RANGE:
                /* save possible dbase function argument */
                *arg = termp;
                termpt = termp + 1;

                if((doc = (docno) readuword(termpt, sizeof(docno))) == 0)
                    doc = ensure_cur_docno();

                refto->doc = reftoe->doc = doc;
                termpt += sizeof(docno);

                refto->col = (colt) readuword(termpt, sizeof(colt)) &
                                    (COLNOBITS | BADCOLBIT);
                termpt += sizeof(colt);
                refto->row = (rowt) readuword(termpt, sizeof(rowt)) &
                                    (ROWNOBITS | BADROWBIT);
                termpt += sizeof(rowt) + sizeof(docno);

                reftoe->col = (colt) readuword(termpt, sizeof(colt)) &
                                    (COLNOBITS | BADCOLBIT);
                termpt += sizeof(colt);
                reftoe->row = (rowt) readuword(termpt, sizeof(rowt)) &
                                    (ROWNOBITS | BADROWBIT);

                *exp = termpt + sizeof(rowt);

                if(bad_col(refto->col) || bad_row(refto->row) ||
                   bad_col(reftoe->col) || bad_row(reftoe->row))
                    break;

                return(OPR_RANGE);

            case OPR_STRINGS:
            case OPR_STRINGD:
                {
                /* look ahead for a dbase function */
                *exp = termp;
                while(*termp)
                    if(*termp++ == OPR_SLR)
                        termp += SLRSIZE - 1;

                ++termp;
                chksym();
                res = 0;

                /* work out dependencies for database functions */
                if(curopr->funbits & DBASEBIT)
                    {
                    /* check we have correct syntax */
                    if(*arg && **arg == OPR_RANGE)
                        {
                        if((res = dbase_refersto(exp, arg,
                                                 refto, reftoe)) >= 0)
                            {
                            *state = TREE_IN_DBASE;
                            return(res);
                            }
                        }
                    }

                termp = *exp;
                *arg = NULL;
                if((res >= 0)  &&  ((res = str_refersto(exp, refto)) >= 0))
                    {
                    *state = TREE_IN_STRING;
                    return(res);
                    }

                /* no refs - skip string and continue */
                termp = *exp;
                *arg = NULL;
                chksym();
                continue;
                }

            /* slots with col and row functions
            are evaluated when they move */
            case OPR_COL:
            case OPR_ROW:
                scnsym();
                *exp = termp;
                refto->doc = ensure_cur_docno();
                refto->col = -1;
                refto->row = TREE_MOVED;
                *arg = NULL;
                return(OPR_COL);

            /* slots with read and write and rand
            functions are always evaluated */
            case OPR_READ:
            case OPR_WRITE:
            case OPR_RAND:
            case OPR_LOOPC:
                scnsym();
                *exp = termp;
                refto->doc = ensure_cur_docno();
                refto->col = -1;
                refto->row = TREE_ALWAYS;
                *arg = NULL;
                return(OPR_READ);

            /* index and h/v lookup dependencies are followed
            dynamically at evaluation time */
            case OPR_INDEX:
            case OPR_HLOOKUP:
            case OPR_VLOOKUP:
                scnsym();
                *exp = termp;
                refto->doc = ensure_cur_docno();
                refto->col = -1;
                refto->row = TREE_INDEX;
                *arg = NULL;
                return(OPR_INDEX);

            case OPR_END:
                return(OPR_END);
            }

        scnsym();
        *arg = NULL;
        }
    while(TRUE);
}

/*************************
*                        *
* find slr in expression *
*                        *
*************************/

extern uchar *
exp_findslr(uchar *exp)
{
    termp = exp;

    /* in a range ? */
    if(inrange)
        {
        inrange = 0;
        return(termp);
        }

    /* in a string ? */
    if(instring)
        {
        while(*termp)
            if(*termp++ == OPR_SLR)
                return(termp);

        ++termp;
        instring = 0;
        }

    chksym();
    do
        {
        switch(curtok)
            {
            case OPR_SLR:
                return(termp + 1);

            case OPR_RANGE:
                inrange = 1;
                return(termp + 1);

            case OPR_STRINGS:
            case OPR_STRINGD:
                while(*termp)
                    {
                    if(*termp++ == OPR_SLR)
                        {
                        instring = 1;
                        return(termp);
                        }
                    }
                ++termp;
                chksym();
                continue;

            case OPR_END:
                return(NULL);

            default:
                break;
            }

        scnsym();
        }
    while(TRUE);
}

/************************
*                       *
* initialise slr finder *
*                       *
************************/

extern void
exp_initslr(void)
{
    inrange = instring = 0;
}

/******************************
*                             *
* return length of expression *
*                             *
******************************/

extern intl
exp_len(uchar *exp)
{
    termp = exp;

    chksym();
    while(curtok != OPR_END)
        scnsym();

    return(termp - exp + 1);
}

/*************************************
*                                    *
* check that we can output to buffer *
*                                    *
*************************************/

static intl
chkspace(intl needed)
{
    if(expop - expstart + needed > expolen)
        {
        csym.symt = SYM_BUFFULL;
        return(0);
        }

    return(1);
}

/*******************
*                  *
* symbol lookahead *
*                  *
*******************/

static intl
chksym(void)
{
    curtok = (intl) *termp;
    curopr = &oprtab[curtok];
    return(curtok);
}

/****************************************
*                                       *
* convert current constant to a string  *
*                                       *
* string is allocated with fixed_malloc *
* and must be freed when not needed     *
*                                       *
****************************************/

static uchar *
convconst(intl opr, uchar *arg)
{
    intl reslen;
    uchar resstr[256], *res;
    uchar *c;
    uchar delim = '"';

    switch(opr)
        {
        case OPR_REAL:
            reslen = fptostr(resstr, readdouble(arg));
            break;

        case OPR_SLR:
            reslen = 0;
            convslr(resstr, &reslen, arg, TRUE);
            break;

        case OPR_RANGE:
            {
            reslen = 0;
            convslr(resstr, &reslen, arg, TRUE);
            convslr(resstr, &reslen, arg + SLRSIZE - 1, FALSE);
            break;
            }

        case OPR_WORD16:
            {
            intl ival;

            ival = (intl) readword16(arg);
            reslen = sprintf((char *) resstr, "%d", ival);
            break;
            }

        case OPR_WORD8:
            {
            intl ival;

            ival = (intl) *arg;
            reslen = sprintf((char *) resstr, "%d", ival);
            break;
            }

        case OPR_STRINGS:
            delim = '\'';

        /* note fall thru */

        case OPR_STRINGD:
            {
            intl slen;

            c = resstr;
            *c++ = delim;
            slen = slrbinstr((char *) c, arg);
            c += slen;
            *c++= delim;
            reslen = 2 + slen;
            break;
            }

        case OPR_DATE:
            {
            struct tm tt;
            time_t ti;
            intl month, mday;

            ti = readtt(arg);
            tt = *localtime(&ti);

            month = tt.tm_mon + 1;
            mday = tt.tm_mday;
            reslen = sprintf((char *) resstr, "%d.%d.%d",
                             ((d_options_DF == 'A')) ? month : mday,
                             ((d_options_DF == 'A')) ? mday : month,
                             tt.tm_year - YEAR_OFFSET);
            break;
            }
        }

    resstr[reslen] = '\0';
    res = fixed_malloc(reslen + 1);
    if(res)
        strncpy((char *) res, (char *) resstr, reslen + 1);
    return(res);
}

/************************
*                       *
* convert SLR to string *
*                       *
************************/

static void
convslr(uchar *resstr, intl *reslen, uchar *arg, intl first)
{
    intl cdol, rdol, tlen, chash, rhash;
    uword32 c, r;
    uchar tstr[50];
    docno doc;

    cdol = rdol = chash = rhash = tlen = 0;

    /* read document reference if first in range */
    if((doc = (docno) readuword(arg, sizeof(docno))) != 0)
        if(first)
            tlen = write_docname(tstr, doc);

    arg += sizeof(docno);

    c = readuword(arg, sizeof(colt));
    r = readuword(arg + sizeof(colt), sizeof(rowt));
    arg += sizeof(colt) + sizeof(rowt);

    if(c & ABSCOLBIT)
        cdol = 1;
    if(c & ABSEVALCOLBIT)
        chash = 1;

    if(r & ABSROWBIT)
        rdol = 1;
    if(r & ABSEVALROWBIT)
        rhash = 1;

    if((c & BADCOLBIT) || (r & BADROWBIT))
        *(tstr + tlen++) = '%';

    c &= COLNOBITS;
    r &= ROWNOBITS;
    ++r;

    if(chash)
        *(tstr + tlen++) = '#';
    if(cdol)
        *(tstr + tlen++) = '$';
    tlen += writecol(tstr + tlen, (colt) c);

    if(rhash)
        *(tstr + tlen++) = '#';
    if(rdol)
        *(tstr + tlen++) = '$';
    tlen += sprintf((char *) (tstr + tlen), "%ld", r);

    /* add second string */
    strncpy((char *) (resstr + *reslen), (char *) tstr, tlen);

    *reslen += tlen;
}

/******************************************************
*                                                     *
* routine to return references in a database function *
*                                                     *
******************************************************/

static intl
dbase_refersto(uchar **exp, uchar **arg, SLR *refto, SLR *reftoe)
{
    colt colscope, scol, ecol;
    rowt rowscope, srow, erow;
    uchar *c;

    c = *arg + 2;

    /* read scope of range */
    scol = (colt) (readuword(c, sizeof(colt)) & (COLNOBITS | BADCOLBIT));
    c += sizeof(colt);
    srow = (rowt) (readuword(c, sizeof(rowt)) & (ROWNOBITS | BADROWBIT));
    c += sizeof(rowt) + sizeof(docno);
    ecol = (colt) (readuword(c, sizeof(colt)) & (COLNOBITS | BADCOLBIT));
    c += sizeof(colt);
    erow = (rowt) (readuword(c, sizeof(rowt)) & (ROWNOBITS | BADROWBIT));

    /* keep bad bits for the scopes */
    colscope = ((ecol & COLNOBITS) - (scol & COLNOBITS)) |
                (ecol & BADCOLBIT) | (scol & BADCOLBIT);
    rowscope = ((erow & ROWNOBITS) - (srow & ROWNOBITS)) |
                (erow & BADROWBIT) | (srow & BADROWBIT);

    tracef2("[dbase_refers colscope: %d, rowscope: %d]\n", colscope, rowscope);

    /* step through the condition string, adding a
    range dependency for every slot reference found */
    c = *exp;
    while(*c)
        {
        uword32 colbits, rowbits;

        if(*c++ == OPR_SLR)
            {
            /* read document number */
            docno doc;

            if((doc = (docno) readuword(c++, sizeof(docno))) == 0)
                doc = ensure_cur_docno();

            refto->doc = reftoe->doc = doc;

            colbits = readuword(c, sizeof(colt));
            rowbits = readuword(c + sizeof(colt), sizeof(rowt));

            tracef2("[dbase_refers colbits: %x, rowbits: %x]\n",
                    colbits, rowbits);

            *exp = c = c + sizeof(colt) + sizeof(rowt);

            refto->col = (colt) (colbits & (COLNOBITS | BADCOLBIT));
            refto->row = (rowt) (rowbits & (ROWNOBITS | BADROWBIT));

            tracef2("[dbase_refers refto->col: %d, refto->row: %d]\n",
                    refto->col, refto->row);

            tracef1("[bad_row(refto->row): %d]\n", bad_row(refto->row));

            /* check for a bad reference */
            if(bad_col(refto->col) || bad_row(refto->row) ||
               bad_col(colscope) || bad_row(rowscope))
                continue;

            /* if reference is fixed, return just an SLR reference */
            if((colbits & ABSEVALCOLBIT) && (rowbits & ABSEVALROWBIT))
                return(OPR_SLR);

            /* return a range dependency */
            reftoe->col = refto->col + ((colbits & ABSEVALCOLBIT)
                                        ? (colt) 0
                                        : colscope);
            reftoe->row = refto->row + ((rowbits & ABSEVALROWBIT)
                                        ? (rowt) 0
                                        : rowscope);

            tracef2("[dbase_refers reftoe->col: %d, reftoe->row: %d]\n",
                    reftoe->col, reftoe->row);

            return(OPR_RANGE);
            }
        }

    *exp = c + 1;
    return(-1);
}

#if defined(UNUSED)

/************************************************
*                                               *
* output document number to compiled expression *
*                                               *
************************************************/

static void
docnout(docno doc)
{
    if(chkspace(sizeof(docno)))
        {
        writeuword(expop, (uword32) doc, sizeof(docno));
        expop += sizeof(docno);
        }
}

#endif

/*************************************
*                                    *
* output byte to compiled expression *
*                                    *
*************************************/

static void
ebout(uchar byt)
{
    if(chkspace(sizeof(uchar)))
        *expop++ = byt;
}

/***************************************
*                                      *
* output double to compiled expression *
*                                      *
***************************************/

static void
edout(double fpval)
{
#if MS

    if(chkspace(sizeof(double)))
        *(((double *) expop)++) = fpval;

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        double fpval;
        uchar fpbytes[sizeof(double)];
        } fp;
    intl i;
    uchar *b;

    if(chkspace(sizeof(double)))
        {
        fp.fpval = fpval;
        b = fp.fpbytes;
        for(i = 0; i < sizeof(double); ++i)
            *expop++ = *b++;
        }

#endif
}

#if defined(UNUSED)

/***************************************************
*                                                  *
* output long unsigned word to compiled expression *
*                                                  *
***************************************************/

static void
eluwout(wrd)
uword32 wrd;
{
#if MS

    if(chkspace(sizeof(uword32)))
        *(((uword32 *) expop)++) = wrd;

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        uword32 uword;
        uchar uwbytes[sizeof(uword32)];
        } uw;
    intl i;
    uchar *b;

    if(chkspace(sizeof(uword32)))
        {
        uw.uword = wrd;
        b = uw.uwbytes;
        for(i = 0; i < sizeof(uword32); i++)
            *expop++ = *b++;
        }

#endif
}

#endif

/***************************************
*                                      *
* output time_t to compiled expression *
*                                      *
***************************************/

static void
ettout(time_t tm)
{
#if MS

    if(chkspace(sizeof(time_t)))
        *(((time_t *) expop)++) = tm;

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        time_t time;
        uchar tmbytes[sizeof(time_t)];
        } t;
    intl i;
    uchar *b;

    if(chkspace(sizeof(time_t)))
        {
        t.time = tm;
        b = t.tmbytes;
        for(i = 0; i < sizeof(time_t); i++)
            *expop++ = *b++;
        }

#endif
}

/**********************************************
*                                             *
* output unsigned word to compiled expression *
*                                             *
**********************************************/

#if MS

static void
euwout(wrd)
uword16 wrd;
{
#if MS

    if(chkspace(sizeof(uword16)))
        *(((uword16 *) expop)++) = wrd;

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        uword16 uword;
        uchar uwbytes[sizeof(uword16)];
        } uw;
    intl i;
    uchar *b;

    if(chkspace(sizeof(uword16)))
        {
        uw.uword = wrd;
        b = uw.uwbytes;
        for(i = 0; i < sizeof(uword16); i++)
            *expop++ = *b++;
        }

#endif
}

#endif

/********************************************
*                                           *
* output signed word to compiled expression *
*                                           *
********************************************/

static void
ewout(word16 wrd)
{
#if MS

    if(chkspace(sizeof(word16)))
        *(((word16 *) expop)++) = wrd;

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        word16 word;
        uchar wbytes[sizeof(word16)];
        } w;
    intl i;
    uchar *b;

    if(chkspace(sizeof(word16)))
        {
        w.word = wrd;
        b = w.wbytes;
        for(i = 0; i < sizeof(word16); ++i)
            *expop++ = *b++;
        }

#endif
}

/********************
*                   *
* compare operators *
*                   *
********************/

static int
flookcomp(const void *arg1, const void *arg2)
{
    oprp opr1 = (oprp) arg1;
    oprp opr2 = (oprp) arg2;

    return(strcmp(  (char *) (opr1->fname) ,
                    (char *) (opr2->fname) ));
}

/**********************************
*                                 *
* lookup function in master table *
*                                 *
**********************************/

static intl
flookup(uchar *func)
{
    struct oprdef tempop;
    oprp curopr;

    tempop.fname = func;
    curopr = (oprp) bsearch(&tempop,
                            &oprtab[OPR_LKP],
                            sizeof(oprtab) / sizeof(struct oprdef) - OPR_LKP,
                            sizeof(struct oprdef),
                            flookcomp);

    if(!curopr)
        return(csym.symt = SYM_BAD);

    csym.symt = SYM_FUNC;
    csym.symno = curopr - oprtab;
    return(strlen((char *) func));
}

/********************************************************
*                                                       *
* convert floating point number to string for PipeDream *
*                                                       *
********************************************************/

static intl
fptostr(uchar *resstr, double fpval)
{
    uchar *exp, *exps, sign;
    intl reslen;

    reslen = sprintf((char *) resstr, "%.15g", fpval);
    resstr[reslen] = '\0';

    /* search for exponent and remove leading zeroes because
    they are confusing; remove the + for good measure */
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
        fixed_free(argstk[--argsp]);
}

/***************************************************
*                                                  *
* expression recogniser works by recursive descent *
* recognise |                                      *
*                                                  *
***************************************************/

static void
laexpr(void)
{
    #if TRACE
    int a;
    tracef1("[laexpr(): sp ~= &%p\n", &a);
    #endif

    laterm();
    while(chknxs() == OPR_OR)
        {
        csym.symt = SYM_BLANK;
        laterm();
        ebout(OPR_OR);
        }
    return;
}

/**************
*             *
* recognise & *
*             *
**************/

static void
laterm(void)
{
    lbterm();
    while(chknxs() == OPR_AND)
        {
        csym.symt = SYM_BLANK;
        lbterm();
        ebout(OPR_AND);
        }
    return;
}

/********************************
*                               *
* recognise =, <>, <, >, <=, >= *
*                               *
********************************/

static void
lbterm(void)
{
    intl nxsym;

    lcterm();
    do
        {
        switch(nxsym = chknxs())
            {
            case OPR_EQUALS:
            case OPR_NOTEQUAL:
            case OPR_LT:
            case OPR_GT:
            case OPR_LTEQUAL:
            case OPR_GTEQUAL:
                csym.symt = SYM_BLANK;
                break;
            default:
                return;
            }
        lcterm();
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
lcterm(void)
{
    intl nxsym;

    ldterm();
    do
        {
        switch(nxsym = chknxs())
            {
            case OPR_PLUS:
            case OPR_MINUS:
                csym.symt = SYM_BLANK;
                break;
            default:
                return;
            }
        ldterm();
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
ldterm(void)
{
    intl nxsym;

    leterm();
    do
        {
        switch(nxsym = chknxs())
            {
            case OPR_TIMES:
            case OPR_DIVIDE:
                csym.symt = SYM_BLANK;
                break;
            default:
                return;
            }
        leterm();
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
leterm(void)
{
    lfterm();
    while(chknxs() == OPR_POWER)
        {
        csym.symt = SYM_BLANK;
        lfterm();
        ebout(OPR_POWER);
        }
    return;
}

/**************************
*                         *
* recognise unary +, -, ! *
*                         *
**************************/

static void
lfterm(void)
{
    switch(chknxs())
        {
        case OPR_PLUS:
            csym.symt = SYM_BLANK;
            lfterm();
            ebout(OPR_UPLUS);
            return;
        case OPR_MINUS:
            csym.symt = SYM_BLANK;
            lfterm();
            ebout(OPR_UMINUS);
            return;
        case OPR_NOT:
            csym.symt = SYM_BLANK;
            lfterm();
            ebout(OPR_NOT);
            return;
        default:
            lgterm();
            return;
        }
}

/******************************
*                             *
* recognise lterm or brackets *
*                             *
******************************/

static void
lgterm(void)
{
    if(chknxs() == SYM_OBRACKET)
        {
        csym.symt = SYM_BLANK;
        laexpr();
        if(chknxs() != SYM_CBRACKET)
            {
            csym.symt = SYM_BAD;
            return;
            }
        csym.symt = SYM_BLANK;
        ebout(OPR_BRACKETS);
        }
    else
        lterm();
}

/************************************
*                                   *
* recognise constants and functions *
*                                   *
************************************/

static void
lterm(void)
{
    intl nxsym;

    switch(nxsym = chknxs())
        {
        case OPR_REAL:
            csym.symt = SYM_BLANK;
            ebout(OPR_REAL);
            edout(csym.fpval);
            return;

        case OPR_SLR:
            csym.symt = SYM_BLANK;
            ebout(OPR_SLR);
            slrout(csym.doc, csym.stcol, csym.strow);
            return;

        case OPR_WORD16:
            csym.symt = SYM_BLANK;
            ebout(OPR_WORD16);
            ewout((word16) csym.fpval);
            return;

        case OPR_WORD8:
            csym.symt = SYM_BLANK;
            ebout(OPR_WORD8);
            ebout((uchar) csym.fpval);
            return;

        case OPR_STRINGS:
        case OPR_STRINGD:
            {
            uchar delim, nestdelim;
            uchar *c = csym.stringp;
            uword32 col, row;
            intl scanned, nested = 0, swapdelim = 0;
            docno doc;

            csym.symt = SYM_BLANK;

            if(nxsym == OPR_STRINGS)
                {
                ebout(OPR_STRINGS);
                delim = '\'';
                nestdelim = '\"';
                }
            else
                {
                ebout(OPR_STRINGD);
                delim = '\"';
                nestdelim = '\'';
                }

            while(nested || (*c != delim))
                {
                if(*c == delim)
                    {
                    --nested;
                    swapdelim = 1;
                    }

                if(*c == nestdelim)
                    {
                    ++nested;
                    swapdelim = 1;
                    }

                if(swapdelim)
                    {
                    delim = (uchar) ((delim == '\'') ? '\"' : '\'');
                    nestdelim = (uchar) ((delim == '\'') ? '\"' : '\'');
                    swapdelim = 0;
                    }

                if(!nested && dbasearg &&
                   ((scanned = scnslr(c, &doc, &col, &row, TRUE)) != 0))
                    {
                    c += scanned;
                    ebout(OPR_SLR);
                    slrout(doc, col, row);
                    }
                else
                    {
                    ebout(*c++);
                    }
                }

            ebout('\0');
            return;
            }

        case OPR_DATE:
            csym.symt = SYM_BLANK;
            ebout(OPR_DATE);
            ettout((time_t) csym.fpval);
            return;

        case SYM_FUNC:
            {
            intl fno = csym.symno;
            oprp funp = &oprtab[fno];

            if(funp->funbits & SLRBIT)
                slrbits = 1;

            csym.symt = SYM_BLANK;
            switch(funp->nargs)
                {
                /* zero argument functions */
                case 0:
                    ebout((uchar) fno);
                    return;

                /* variable argument functions */
                case -1:
                    {
                    intl narg;

                    narg = procfunc(funp);
                    ebout((uchar) fno);
                    ebout((uchar) narg);
                    return;
                    }

                /* fixed argument functions */
                default:
                    {
                    procfunc(funp);
                    ebout((uchar) fno);
                    return;
                    }
                }
            }

        default:
            csym.symt = SYM_BAD;
            return;
        }
}

/*******************************************************
*                                                      *
* recognise an element of a list of function arguments *
*                                                      *
*******************************************************/

static void
lzelement(void)
{
    if(chknxs() == OPR_RANGE)
        {
        csym.symt = SYM_BLANK;
        ebout(OPR_RANGE);

        slrout(csym.doc, csym.stcol, csym.strow);
        slrout(csym.doc, csym.encol, csym.enrow);
        return;
        }

    laexpr();
}

/************************************
*                                   *
* scan a symbol from the expression *
*                                   *
************************************/

#define MAX_SYM 25

static intl
nxtsym(void)
 {
    uchar cc = *exppos;
    uchar fstr[MAX_SYM + 1], *fs, *funp, fc;
    intl cr, co, ts;

    /* skip blanks */
    while(cc == ' ')
        cc = *++exppos;

    /* check for end of expression */
    if(!cc)
        return(csym.symt = OPR_END);

    /* check for constant */
    if(isdigit(cc) || (cc == '.'))
        {
        if((ts = reccon(exppos, &csym.fpval, &cr)) >= 0)
            {
            exppos += cr;
            return(csym.symt = ts);
            }
        }

    /* check for function */
    co = 0;
    if(isalpha(cc))
        {
        fs = fstr;
        funp = exppos;
        fc = cc;
        do
            {
            *fs++ = tolower(fc);
            ++co;
            fc = *(++funp);
            }
        while((isalpha(fc) || isdigit(fc)) && (co < MAX_SYM));

        *fs = '\0';
        flookup(fstr);
        if(csym.symt != SYM_BAD)
            {
            exppos = funp;
            return(csym.symt);
            }
        }

    /* check for slot reference/range */
    if(isalpha(cc) || (cc == '$') || (cc == '['))
        {
        if((cr = scnslr(exppos, &csym.doc,
                        &csym.stcol, &csym.strow, FALSE)) != 0)
            {
            docno dummy;

            exppos += cr;
            /* check for another SLR to make range */
            if((cr = scnslr(exppos, &dummy,
                            &csym.encol, &csym.enrow, FALSE)) != 0)
                {
                if(((csym.stcol & COLNOBITS) > (csym.encol & COLNOBITS)) ||
                   ((csym.strow & ROWNOBITS) > (csym.enrow & ROWNOBITS)))
                    return(csym.symt = SYM_BAD);
                exppos += cr;
                return(csym.symt = OPR_RANGE);
                }

            return(csym.symt = OPR_SLR);
            }

        return(csym.symt = SYM_BAD);
        }

    /* check for string */
    if((cc == '"') || (cc == '\''))
        {
        fs = exppos + 1;
        while(*fs && (*fs != cc))
            ++fs;
        if(*fs != cc)
            return(SYM_BAD);
        csym.stringp = exppos + 1;
        exppos = fs + 1;
        return(csym.symt = (cc == '"' ? OPR_STRINGD : OPR_STRINGS));
        }

    /* check for special operators */
    switch(cc)
        {
        case '(':
            ++exppos;
            return(csym.symt = SYM_OBRACKET);
        case ')':
            ++exppos;
            return(csym.symt = SYM_CBRACKET);
        case ',':
            ++exppos;
            return(csym.symt = SYM_COMMA);
        default:
            break;
        }

    /* check for operator */
    if(!ispunct(cc))
        return(csym.symt = SYM_BAD);

    fs = fstr;
    co = 0;
    while(ispunct(cc) && co < MAX_SYM)
        {
        *fs++ = cc;
        cc = *(++exppos);
        ++co;

        switch(cc)
            {
            case '>':
            case '<':
            case '=':
                continue;
                break;

            default:
                break;
            }

        break;
        }
    *fs++ = '\0';
    flookup(fstr);
    return((csym.symt == SYM_FUNC)
            ? csym.symt = csym.symno
            : csym.symt);
}

/************************************
*                                   *
* process a function with arguments *
*                                   *
************************************/

static intl
procfunc(oprp funp)
{
    intl narg = 0;

    if(chknxs() != SYM_OBRACKET && funp->nargs > 0)
        {
        csym.symt = SYM_BAD;
        return(0);
        }

    if(csym.symt == SYM_OBRACKET)
        {
        do
            {
            if((funp->funbits & DBASEBIT) && (narg == 1))
                dbasearg = 1;
            else
                dbasearg = 0;

            csym.symt = SYM_BLANK;
            lzelement();
            ++narg;
            }
        while(chknxs() == SYM_COMMA);

        if(chknxs() != SYM_CBRACKET)
            {
            csym.symt = SYM_BAD;
            return(narg);
            }

        if((funp->nargs >= 0) && (funp->nargs != (uchar) narg))
            return(csym.symt = SYM_BAD);

        csym.symt = SYM_BLANK;
        }

    return(narg);
}

/****************************
*                           *
* read a double from memory *
*                           *
****************************/

static double
readdouble(uchar *arg)
{
#if MS

    return(*((double *) arg));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        double fpval;
        uchar fpbytes[sizeof(double)];
        } fp;
    intl i;
    uchar *b = fp.fpbytes;

    for(i = 0; i < sizeof(double); ++i)
        *b++ = *arg++;

    return(fp.fpval);

#endif
}

/****************************
*                           *
* read a time_t from memory *
*                           *
****************************/

static time_t
readtt(uchar *arg)
{
#if MS

    return(*((time_t *) arg));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        time_t word;
        uchar wbytes[sizeof(time_t)];
        } uw;
    intl i;
    uchar *b = uw.wbytes;

    for(i = 0; i < sizeof(time_t); ++i)
        *b++ = *arg++;

    return(uw.word);

#endif
}

/************************************
*                                   *
* read an unsigned word from memory *
*                                   *
************************************/

#if MS

static uword16
readuword16(arg)
uchar *arg;
{
#if MS

    return(*((uword16 *) arg));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        uword16 uword;
        uchar uwbytes[sizeof(uword16)];
        } uw;
    intl i;
    uchar *b = uw.uwbytes;

    for(i = 0; i < sizeof(uword16); ++i)
        *b++ = *arg++;

    return(uw.uword);

#endif
}

#endif

/******************************************
*                                         *
* read an unsigned word of specified size *
*                                         *
******************************************/

extern uword32
readuword(uchar *from, intl size)
{
    uword32 res;
    intl i;

    for(res = 0, i = size - 1; i >= 0; i--)
        {
        res <<= 8;
        res += from[i];
        }

    return(res);
}

#if defined(UNUSED)

/*****************************************
*                                        *
* read an unsigned long word from memory *
*                                        *
*****************************************/

static uword32
readuword32(arg)
uchar *arg;
{
#if MS

    return(*((uword32 *) arg));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        uword32 uword;
        uchar uwbytes[sizeof(uword32)];
        } uw;
    intl i;
    uchar *b = uw.uwbytes;

    for(i = 0; i < sizeof(uword32); ++i)
        *b++ = *arg++;

    return(uw.uword);

#endif
}

#endif

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
    union
        {
        word16 word;
        uchar wbytes[sizeof(word16)];
        } w;
    intl i;
    uchar *b = w.wbytes;

    for(i = 0; i < sizeof(word16); ++i)
        *b++ = *arg++;

    return(w.word);

#endif
}

/*****************************************
*                                        *
* recognise a constant and classify      *
*                                        *
* --out--                                *
* -1 - no constant found                 *
* otherwise type returned                *
*                                        *
*****************************************/

static intl
reccon(uchar *slot, double *fpval, intl *cs)
{
    intl res, i, day, mon, yr;
    uchar tstr[25], *tp;
    struct tm t_time;
    time_t temp;

    t_time.tm_year = t_time.tm_hour = t_time.tm_isdst =
    t_time.tm_min = t_time.tm_sec = 0;

    /* check for date */
    res = sscanf((char *) slot, "%d.%d.%d%n",
                 ((d_options_DF == 'A')) ? (&t_time.tm_mon)
                                         : (&t_time.tm_mday),
                 ((d_options_DF == 'A')) ? (&t_time.tm_mday)
                                         : (&t_time.tm_mon),
                 &t_time.tm_year, cs);

    if(res == 3)
        {
        #if MS
        /* check for sscanf bug */
        if(!(*(slot + *cs - 1)))
            --(*cs);
        #endif

        t_time.tm_year = (t_time.tm_year % 1900 % 120) + YEAR_OFFSET;
        t_time.tm_mon--;

        day = t_time.tm_mday;
        mon = t_time.tm_mon;
        yr = t_time.tm_year;

        temp = mktime(&t_time);
        t_time = *localtime(&temp);

        /* check for a valid date */
        if((t_time.tm_mon < 0) || (t_time.tm_mon > 11))
            return(SYM_BAD);

        i = t_time.tm_year / 4;
        i = ((i * 4) == t_time.tm_year) && (t_time.tm_mon == 1)
          ? days[t_time.tm_mon] + 1
          : days[t_time.tm_mon];

        if(t_time.tm_mday > i)
            return(SYM_BAD);

        if((day != t_time.tm_mday) ||
           (mon != t_time.tm_mon) ||
           (yr != t_time.tm_year))
            return(SYM_BAD);

        *fpval = (double) temp;
        return(OPR_DATE);
        }

    /* a fuddle cos sscanf doesn't get ".5", needs "0.5" */
    if(*slot == '.')
        {
        *tstr = '0';
        strncpy((char *) (tstr + 1), (char *) slot, 25);
        tp = tstr;
        }
    else
        {
        tp = slot;
        }

    res = sscanf((char *) tp, "%lf%n", fpval, cs);
    if(res > 0)
        {
        /* account for inserted zero */
        if(tp == tstr)
            --(*cs);

        #if MS
        /* check for sscanf bug */
        if(!(*(slot + *cs - 1)))
            --(*cs);
        #endif

        if(floor(*fpval) == *fpval)
            {
            if(*fpval < 256.)
                return(OPR_WORD8);
            else if((*fpval < 32768.) && (*fpval > -.32768))
                return(OPR_WORD16);
            }

        return(OPR_REAL);
        }

    return(-1);
}

/**************
*             *
* RPN scanner *
*             *
**************/

static intl
scnsym(void)
{
    if(curtok != -1)
        {
        /* work out how to skip symbol */
        ++termp;
        switch(curopr->ftype)
            {
            case TYPE_CONST:
                switch(curtok)
                    {
                    case OPR_REAL:
                        termp += sizeof(double);
                        break;
                    case OPR_SLR:
                        termp += SLRSIZE - 1;
                        break;
                    case OPR_RANGE:
                        termp += (SLRSIZE - 1) * 2;
                        break;
                    case OPR_WORD16:
                        termp += sizeof(word16);
                        break;
                    case OPR_WORD8:
                        termp += sizeof(uchar);
                        break;

                    case OPR_STRINGS:
                    case OPR_STRINGD:
                        while(*termp)
                            if(*termp++ == OPR_SLR)
                                termp += SLRSIZE - 1;
                        ++termp;
                        break;

                    case OPR_DATE:
                        termp += sizeof(time_t);
                        break;

                    default:
                        break;
                    }
                break;

            case TYPE_BINARYREL:
            case TYPE_BINARY:
            case TYPE_UNARY:
            case TYPE_BRACKETS:
                break;

            case TYPE_FUNC:
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

/************************
*                       *
* scan a slot reference *
*                       *
************************/

static intl
scnslr(uchar *exppos, docno *doc, uword32 *col, uword32 *row, intl dbase)
{
    uchar *c = exppos;
    uword32 absc = 0, absr = 0;
    intl cr, res;

    while(*c == ' ')
        ++c;

    /* read in the document name */
    if(*c == '[')
        {
        if((cr = read_docname(c, doc)) == 0)
            return(0);
        c += cr;
        }
    else
        *doc = 0;

    if(dbase && (*c == '#'))
        {
        ++c;
        absc |= ABSEVALCOLBIT;
        }

    if(*c == '$')
        {
        ++c;
        absc |= ABSCOLBIT;
        }

    if((cr = stox(c, col)) == 0)
        return(0);

    c += cr;
    if(dbase && (*c == '#'))
        {
        ++c;
        absr |= ABSEVALROWBIT;
        }

    if(*c == '$')
        {
        ++c;
        absr |= ABSROWBIT;
        }

    if(*col >= LARGEST_COL_POSSIBLE)
        return(0);

    if(!isdigit(*c))
        return(0);

    res = sscanf((char *) c, "%ld%n", row, &cr);
    if((res < 1) || !cr)
        return(0);

    #if MS
    /* check for sscanf bug */
    if(!(*(c + cr - 1)))
        --cr;
    #endif

    --(*row);
    if(*row >= LARGEST_ROW_POSSIBLE)
        return(0);

    *col = (*col & LARGEST_COL_POSSIBLE) | absc;
    *row = (*row & LARGEST_ROW_POSSIBLE) | absr;

    /* set had slr flag */
    slrbits = 1;
    return(c - exppos + cr);
}

/*****************************************************
*                                                    *
* convert to a string from binary, which may contain *
* slot references                                    *
*                                                    *
*****************************************************/

extern intl
slrbinstr(char *result, uchar *input)
{
    uchar *in = input;
    char *out = result;
    intl slrlen;

    do
        {
        if(*in == OPR_SLR)
            {
            slrlen = 0;
            convslr((uchar *) out, &slrlen, ++in, TRUE);
            in += SLRSIZE - 1;
            out += slrlen;
            tracef1("[slrbinstr result: %s]\n", trace_string(result));
            }
        else
            {
            *out++ = (char) *in;
            if(!*in++)
                break;
            }
        }
    while(TRUE);

    return(out - result - 1);
}

/*************************************************
*                                                *
* output a slot reference to the compiled string *
*                                                *
*************************************************/

static void
slrout(docno doc, uword32 col, uword32 row)
{
    if(chkspace(SLRSIZE - 1))
        {
        writeuword(expop, (word32) doc, sizeof(docno));
        expop += sizeof(docno);

        writeuword(expop, col, sizeof(colt));
        expop += sizeof(colt);
        writeuword(expop, row, sizeof(rowt));
        expop += sizeof(rowt);
        }
}

/*******************************************
*                                          *
* convert column string into column number *
*                                          *
*******************************************/

static intl
stox(uchar *string, uword32 *col)
{
    intl cr = 0, i, tcol;

    i = toupper(*string) - 'A';
    ++string;
    if((i >= 0) && (i <= 25))
        {
        tcol = i;
        cr = 1;
        do
            {
            i = toupper(*string) - 'A';
            ++string;
            if((i < 0) || (i > 25))
                break;
            tcol = (tcol + 1) * 26 + i;
            ++cr;
            }
        while(TRUE);
        *col = tcol;
        }

    return(cr);
}

/*******************************************
*                                          *
* routine to return references in a string *
*                                          *
*******************************************/

static intl
str_refersto(uchar **exp, SLR *refto)
{
    uchar *str;

    str = *exp;

    while(*str)
        {
        if(*str++ == OPR_SLR)
            {
            if((refto->doc = (docno) readuword(str, sizeof(docno))) == 0)
                refto->doc = ensure_cur_docno();

            str += sizeof(docno);

            refto->col = (colt) readuword(str, sizeof(colt)) &
                                            (COLNOBITS | BADCOLBIT);
            str += sizeof(colt);
            refto->row = (rowt) readuword(str, sizeof(rowt)) &
                                            (ROWNOBITS | BADROWBIT);
            str += sizeof(rowt);

            if(bad_col(refto->col) || bad_row(refto->row))
                continue;

            *exp = str;
            return(OPR_SLR);
            }
        }

    *exp = str + 1;
    return(-1);
}

/***********************************************
*                                              *
* write out an unsigned word of specified size *
*                                              *
***********************************************/

extern void
writeuword(uchar *to, uword32 num, intl size)
{
    intl i;

    for(i = 0; i < size; i++, to++)
        {
        *to = (uchar) (num & 0xFF);
        num >>= 8;
        }
}

/* end of expcomp.c */
