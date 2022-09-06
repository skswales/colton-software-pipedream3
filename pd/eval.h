/* eval.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*********************************************
*                                            *
* local header file for PipeDream expression *
* compiler                                   *
* RJM                                        *
* MRJC May 1988                              *
*                                            *
*********************************************/

/*
operator numbers
*/

#define OPR_REAL    0
#define OPR_SLR     1
#define OPR_RANGE   2
#define OPR_WORD16  3
#define OPR_WORD8   4
#define OPR_STRINGS 5
#define OPR_STRINGD 6
#define OPR_DATE    7
#define OPR_BRACKETS 8
#define OPR_END     9

#define OPR_UPLUS   10
#define OPR_UMINUS  11

/* start of things to be looked up */
#define OPR_LKP     12

#define OPR_NOT     OPR_LKP
#define OPR_AND     13
#define OPR_TIMES   14
#define OPR_PLUS    15
#define OPR_MINUS   16
#define OPR_DIVIDE  17
#define OPR_LT      18
#define OPR_LTEQUAL 19
#define OPR_NOTEQUAL 20
#define OPR_EQUALS  21
#define OPR_GT      22
#define OPR_GTEQUAL 23
#define OPR_POWER   24
#define OPR_ABS     25
#define OPR_ACS     26
#define OPR_ASN     27
#define OPR_ATN     28
#define OPR_ATN2    29
#define OPR_AVG     30
#define OPR_CHOOSE  31
#define OPR_COL     32
#define OPR_COS     33
#define OPR_COUNT   34
#define OPR_CTERM   35
#define OPR_TODAY   36
#define OPR_DAVG    37
#define OPR_DAY     38
#define OPR_DCOUNT  39
#define OPR_DCOUNTA 40
#define OPR_DDB     41
#define OPR_DEG     42
#define OPR_DMAX    43
#define OPR_DMIN    44
#define OPR_DSTD    45
#define OPR_DSUM    46
#define OPR_DVAR    47
#define OPR_EXP     48
#define OPR_FV      49
#define OPR_HLOOKUP 50
#define OPR_IF      51
#define OPR_INDEX   52
#define OPR_INT     53
#define OPR_IRR     54
#define OPR_LN      55
#define OPR_LOG     56
#define OPR_LOOKUP  57
#define OPR_LOOPC   58
#define OPR_MAX     59
#define OPR_MIN     60
#define OPR_MOD     61
#define OPR_MONTH   62
#define OPR_MONTHDAYS 63
#define OPR_NPV     64
#define OPR_PI      65
#define OPR_PMT     66
#define OPR_PV      67
#define OPR_RAD     68
#define OPR_RAND    69
#define OPR_RATE    70
#define OPR_READ    71
#define OPR_ROUND   72
#define OPR_ROW     73
#define OPR_SGN     74
#define OPR_SIN     75
#define OPR_SLN     76
#define OPR_SQR     77
#define OPR_STD     78
#define OPR_SUM     79
#define OPR_SYD     80
#define OPR_TAN     81
#define OPR_TERM    82
#define OPR_VAR     83
#define OPR_VLOOKUP 84
#define OPR_WRITE   85
#define OPR_YEAR    86
#define OPR_OR      87

/*
types of operators
*/

#define TYPE_UNARY 1
#define TYPE_BINARY 2
#define TYPE_BINARYREL 3
#define TYPE_FUNC 4
#define TYPE_CONST 5
#define TYPE_END 6
#define TYPE_BRACKETS 7

/*
table of operators
*/

struct oprdef
{
    intl ftype;
    signed char nargs;
    uchar funbits;
    uchar *fname;
    void (*semaddr)();
};

/* definition of function bits (funbits) */
#define SLRBIT 1
#define DBASEBIT 2

typedef struct oprdef *oprp;

/*
scanner communication
*/

struct symbol
{
    intl symt;
    intl symno;
    double fpval;
    docno doc;
    uword32 stcol;
    uword32 strow;
    uword32 encol;
    uword32 enrow;
    uchar *stringp;
};

/* special symbol constants */
#define SYM_BAD -1
#define SYM_BLANK -2
#define SYM_OBRACKET -3
#define SYM_CBRACKET -4
#define SYM_COMMA -5
#define SYM_FUNC -6
#define SYM_BUFFULL -7

/* error codes */
#define EVAL_ERR_IRR             EVAL_ERR_START - 0
#define EVAL_ERR_TOOMANYFILES    EVAL_ERR_START - 1
#define EVAL_ERR_BAD_LOG         EVAL_ERR_START - 2
#define EVAL_ERR_BAD_DATE        EVAL_ERR_START - 3
#define EVAL_ERR_LOOKUP          EVAL_ERR_START - 4
#define EVAL_ERR_MIXED_TYPES     EVAL_ERR_START - 5
#define EVAL_ERR_NEG_ROOT        EVAL_ERR_START - 6
#define EVAL_ERR_PROPAGATED      EVAL_ERR_START - 7
#define EVAL_ERR_DIVIDEBY0       EVAL_ERR_START - 8
#define EVAL_ERR_BAD_INDEX       EVAL_ERR_START - 9
#define EVAL_ERR_STACKOVER       EVAL_ERR_START - 10
#define EVAL_ERR_FPERROR         EVAL_ERR_START - 11
#define EVAL_ERR_CIRC            EVAL_ERR_START - 12
#define EVAL_ERR_CANTEXTREF      EVAL_ERR_START - 13
#define EVAL_ERR_MANYEXTREF      EVAL_ERR_START - 14

#define PI          (3.1415926535898)
#define MAX_STRLEN  (LIN_BUFSIZ-1)

#if ARTHUR || RISCOS || MS
#define UNITS_IN_DAY    ((time_t) 86400)
#endif

/*
instructions for poparg
*/

#define POP_NORMAL 0
#define NO_SUM_RANGE 1
#define KEEP_SLR 2
#define POP_BLANK_0 4

#define TREE_STACKLEVEL 12000
#define RISCOS_BACKGROUND 25

/*
special recalc type bits
*/

#define TREE_ALWAYS 0
#define TREE_MOVED 1
#define TREE_INDEX 2

/* maximum document number */
#define MAX_DOCNO DOCHANDLE_MAX

/* document number meaning bad or unselectable */
#define DOCNO_NOENTRY 255

/* length of external reference name */
#define EXT_REF_NAMLEN 255

/*
slr and range tree/table structures
not defined as SLR types to save space on Archie
*/

struct slrentry
{
    colt refto_col;
    colt byslr_col;
    rowt refto_row;
    rowt byslr_row;
    docno refto_doc;
    docno byslr_doc;
    uchar entflags;
};

typedef struct slrentry *slrep;

struct rngentry
{
    colt reftos_col;
    colt reftoe_col;
    colt byslr_col;
    rowt reftos_row;
    rowt reftoe_row;
    rowt byslr_row;
    docno reftos_doc;
    docno reftoe_doc;
    docno byslr_doc;
    uchar entflags;
};

typedef struct rngentry *rngep;

/*
declaration of variables in eval.c
*/

extern intl stack_depth;

extern SYMB_TYPE *stack;
extern SYMB_TYPE *stack_ptr;
extern SYMB_TYPE *stack_one;
extern SYMB_TYPE *stack_two;
extern SYMB_TYPE *stack_end;

extern intl errorflag;              /* 0=no error, n = error number */
 
extern colt eval_col;               /* col number of slot being evaluated */
extern rowt eval_row;               /* row number of slot being evaluated */

extern colt col_index;
extern colt range_col_1; 
extern colt range_col_2;

extern rowt row_index;
extern rowt range_row_1;
extern rowt range_row_2;

extern word32 itercount;

/*
function declarations
*/

/*
eval.c
*/

extern void growstack(void);
extern SYMB_TYPE *next_in_range(BOOL goingdown,
                                BOOL single_line,
                                intl check_tree);
extern void pderror(intl);
extern SYMB_TYPE *poparg(intl type_filter);
extern void pusharg(SYMB_TYPE *);
extern void setup_range(SYMB_TYPE *);
extern BOOL true_condition(uchar *cond, BOOL increment_row);

/*
expcomp.c
*/

extern intl days[];
extern struct oprdef oprtab[];

extern intl exp_refersto_next(uchar **exp, intl *state, uchar **arg,
                              SLR *refto, SLR *reftoe);
extern intl slrbinstr(char *, uchar *);

/*
semantic.c
*/

extern void c_add(SYMB_TYPE *, SYMB_TYPE *);
extern void c_and(SYMB_TYPE *, SYMB_TYPE *);
extern void c_div(SYMB_TYPE *, SYMB_TYPE *);
extern void c_eq(SYMB_TYPE *, SYMB_TYPE *);
extern void c_gt(SYMB_TYPE *, SYMB_TYPE *);
extern void c_gteq(SYMB_TYPE *, SYMB_TYPE *);
extern void c_lt(SYMB_TYPE *, SYMB_TYPE *);
extern void c_lteq(SYMB_TYPE *, SYMB_TYPE *);
extern void c_mod(SYMB_TYPE *);
extern void c_mul(SYMB_TYPE *, SYMB_TYPE *);
extern void c_neq(SYMB_TYPE *, SYMB_TYPE *);
extern void c_not(SYMB_TYPE *);
extern void c_or(SYMB_TYPE *, SYMB_TYPE *);
extern void c_power(SYMB_TYPE *, SYMB_TYPE *);
extern void c_sub(SYMB_TYPE *, SYMB_TYPE *);
extern void c_umi(SYMB_TYPE *);
extern void c_uplus(SYMB_TYPE *);

/*
single argument functions
*/

extern void c_abs(SYMB_TYPE *);
extern void c_acs(SYMB_TYPE *);
extern void c_asn(SYMB_TYPE *);
extern void c_atn(SYMB_TYPE *);
extern void c_atn2(SYMB_TYPE *);
extern void c_cos(SYMB_TYPE *);
extern void c_deg(SYMB_TYPE *);
extern void c_exp(SYMB_TYPE *);
extern void c_int(SYMB_TYPE *);
extern void c_ln(SYMB_TYPE *);
extern void c_log(SYMB_TYPE *);
extern void c_loopc(SYMB_TYPE *);
extern void c_pi(SYMB_TYPE *);
extern void c_rad(SYMB_TYPE *);
extern void c_sgn(SYMB_TYPE *);
extern void c_sin(SYMB_TYPE *);
extern void c_sqr(SYMB_TYPE *);
extern void c_tan(SYMB_TYPE *);

extern void c_day(SYMB_TYPE *);
extern void c_month(SYMB_TYPE *);
extern void c_monthdays(SYMB_TYPE *);
extern void c_today(SYMB_TYPE *);
extern void c_year(SYMB_TYPE *);

/*
other functions
*/

extern void c_avg(SYMB_TYPE *);
extern void c_choose(SYMB_TYPE *);
extern void c_col(SYMB_TYPE *);
extern void c_count(SYMB_TYPE *);
extern void c_davg(SYMB_TYPE *);
extern void c_dcount(SYMB_TYPE *);
extern void c_dcounta(SYMB_TYPE *);
extern void c_dmax(SYMB_TYPE *);
extern void c_dmin(SYMB_TYPE *);
extern void c_dstd(SYMB_TYPE *);
extern void c_dsum(SYMB_TYPE *);
extern void c_dvar(SYMB_TYPE *);
extern void c_if(SYMB_TYPE *);
extern void c_index(SYMB_TYPE *);
extern void c_hlookup(SYMB_TYPE *);
extern void c_lookup(SYMB_TYPE *);
extern void c_max(SYMB_TYPE *);
extern void c_min(SYMB_TYPE *);
extern void c_rand(SYMB_TYPE *);
extern void c_read(SYMB_TYPE *);
extern void c_round(SYMB_TYPE *);
extern void c_row(SYMB_TYPE *);
extern void c_std(SYMB_TYPE *);
extern void c_sum(SYMB_TYPE *);
extern void c_var(SYMB_TYPE *);
extern void c_vlookup(SYMB_TYPE *);
extern void c_write(SYMB_TYPE *);

extern void c_cterm(SYMB_TYPE *);
extern void c_ddb(SYMB_TYPE *);
extern void c_fv(SYMB_TYPE *);
extern void c_irr(SYMB_TYPE *);
extern void c_npv(SYMB_TYPE *);
extern void c_pmt(SYMB_TYPE *);
extern void c_pv(SYMB_TYPE *);
extern void c_rate(SYMB_TYPE *);
extern void c_sln(SYMB_TYPE *);
extern void c_syd(SYMB_TYPE *);
extern void c_term(SYMB_TYPE *);

/* end of eval.h */
