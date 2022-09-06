/* semantic.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*******************************
*                              *
* RJM                          *
* updated by MRJC May 1988 for *
* new expression compiler      *
*                              *
*******************************/

#include "datafmt.h"

/* local header file */
#include "eval.h"

/* exported functions */

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

/* single argument functions */

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
extern void c_loopc(SYMB_TYPE * popa);
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

/* other functions */

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

extern void clslnf(void);


/* internal functions */

/*static void calc_irr(SYMB_TYPE *popa, SYMB_TYPE *firstparm, intl parms,
                     double guess);*/
/*static void calc_npv(SYMB_TYPE *popa, SYMB_TYPE *firstparm, intl parms,
                     double interest);*/
static intl dmax_sum_min(SYMB_TYPE *popa, intl func);
static SYMB_TYPE *extract_from_range(SYMB_TYPE *popa, intl res,
                                     BOOL goingdown, BOOL accept_nearest,
                                     intl check_tree);
static time_t extract_time(SYMB_TYPE *popa);
static SYMB_TYPE *find_target_symb(SYMB_TYPE *popa);
static BOOL get_four_nums(SYMB_TYPE *popa, SYMB_TYPE **apop,
                          SYMB_TYPE **bpop, SYMB_TYPE **cpop, SYMB_TYPE **dpop);
static BOOL get_three_nums(SYMB_TYPE *popa, SYMB_TYPE **apop,
                           SYMB_TYPE **bpop, SYMB_TYPE **cpop);
static void irr_npv_body(SYMB_TYPE *popa, BOOL irr);
static intl look_up_symb(SYMB_TYPE *target_symb, BOOL accept_nearest,
                         BOOL goingdown);
static intl max_sum_min(SYMB_TYPE *popa, intl func, uchar *condition);
static FILE *position_file(intl fileno, intl x, intl y);


/****************
*               *
* linking files *
*               *
****************/

#if MS
// OLD_NORCROFT #pragma pack(2)
#endif

#define FILES_ALLOWED     5
#define LARGEST_LINK_FILE 255
#define LINK_IDENTIFIER   0xAA

#define LINKSTART struct _linkstart

LINKSTART
    {
    uchar one;
    uchar two;
    uchar three;
    intl  mrow;
    intl  mcol;
    uchar ident;
    };


#define LINK_FILE struct link_file

static LINK_FILE
    {
    FILE *fptr;
    intl filenumber;
    intl mcol;
    intl mrow;
    void *fbuf;
    }
file_table [FILES_ALLOWED];

#if MS
// PRAGMA_PACK #pragma pack()
#endif

static intl filesopen = 0;


/* ----------------------------------------------------------------------- */

extern void
c_add(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    if((popa->type) == SL_ERROR || (popb->type == SL_ERROR))
        {
        errorflag = EVAL_ERR_PROPAGATED;
        return;
        }

    if(popb->type == SL_BLANK)
        return;

    if(popa->type == SL_BLANK)
        {
        *popa = *popb;
        return;
        }

    if((popa->type == SL_DATE)  &&  (popb->type == SL_DATE))
        {
        errorflag = EVAL_ERR_MIXED_TYPES;
        return;
        }

      if(popa->type == SL_DATE)
        popa->value.date = popa->value.date +
                           ((time_t) ((long int) popb->value.num)) * UNITS_IN_DAY;
    elif(popb->type == SL_DATE)
        popa->value.date = popb->value.date +
                           ((time_t) ((long int) popa->value.num)) * UNITS_IN_DAY;
    else
        popa->value.num = popa->value.num + popb->value.num;

    popa->type = (uchar)
                 (((popb->type == SL_DATE)  ||  (popa->type == SL_DATE))
                 ? SL_DATE : SL_NUMBER);
}

extern void
c_and(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num = (double)
                      ((int) popb->value.num && (int) popa->value.num);
}

extern void
c_div(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    if(popa->value.num == 0.)
        errorflag = EVAL_ERR_DIVIDEBY0;
    else
        popa->value.num = popb->value.num /= popa->value.num;
}

extern void
c_eq(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num = (double) !symbcmp(popa, popb);
}

extern void
c_gt(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num = (double) (symbcmp(popa, popb) < 0);
}

extern void
c_gteq(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num = (double) (symbcmp(popa, popb) <= 0);
}

extern void
c_lt(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num = (double) (symbcmp(popa, popb) > 0);
}

extern void
c_lteq(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num = (double) (symbcmp(popa, popb) >= 0);
}


/*
* mod(a,b)
*/

extern void
c_mod(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop;

    bpop = poparg(POP_NORMAL);
    apop = poparg(POP_NORMAL);

    if((apop->type != SL_NUMBER) || (bpop->type != SL_NUMBER))
        {
        errorflag = EVAL_ERR_MIXED_TYPES;
        return;
        }

    popa->type = SL_NUMBER;

#if MS
    popa->value.num = fmod(apop->value.num, bpop->value.num);

#elif ARTHUR || RISCOS
    /* this accounts for the non-working fmod function on the ARM */
    {
    double temp, frac;

    if(bpop->value.num == 0.)
        temp = 0.;
    else
        temp = apop->value.num / bpop->value.num;

    frac = modf(temp, &temp);
    popa->value.num = frac * bpop->value.num;

    if((apop->value.num < 0.) && (popa->value.num > 0.))
        popa->value.num = -popa->value.num;

    if((apop->value.num > 0.) && (popa->value.num < 0.))
        popa->value.num = -popa->value.num;

    }
#endif
}

extern void
c_mul(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num *= popb->value.num;
}

extern void
c_neq(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num = (double) (symbcmp(popa, popb) != 0);
}

extern void
c_not(SYMB_TYPE *popa)
{
    popa->value.num = (double) !((int) popa->value.num);
}

extern void
c_or(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    popa->value.num = (double)
                      ((int) popb->value.num || (int) popa->value.num);
}

extern void
c_power(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
    if((popa->value.num == 0.) &&
       (popb->value.num == 0.))
        {
        errorflag = EVAL_ERR_FPERROR;
        return;
        }

    if((popb->value.num < 0.) &&
       (floor(popa->value.num) !=
       popa->value.num))
        {
        errorflag = EVAL_ERR_FPERROR;
        return;
        }

    popa->value.num = pow(popb->value.num, popa->value.num);
}

extern void
c_sub(SYMB_TYPE *popa, SYMB_TYPE *popb)
{
      if(popa->type == SL_DATE)         /* both dates */
        {
        popa->value.num = (double) (difftime(popb->value.date,
                                             popa->value.date) / UNITS_IN_DAY);
        }
    elif(popb->type == SL_DATE)
        {
        popa->value.date = popb->value.date -
                                    ((time_t) ((long int) popa->value.num)) * UNITS_IN_DAY;
        }
    else
        {
        popa->value.num = popb->value.num - popa->value.num;
        }

    popa->type = (uchar) ((popb->type == popa->type)
                                    ? SL_NUMBER
                                    : SL_DATE);
}

extern void
c_umi(SYMB_TYPE *popa)
{
    popa->value.num = -popa->value.num;
}

extern void
c_uplus(SYMB_TYPE *popa)
{
    IGNOREPARM(popa);
}

/*****************
*                *
* Trig functions *
*                *
*****************/

extern void
c_abs(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num = fabs(popa->value.num);
}

extern void
c_acs(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num = acos(popa->value.num);
}

extern void
c_asn(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num = asin(popa->value.num);
}

extern void
c_atn(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num = atan(popa->value.num);
}

extern void
c_atn2(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop;

    bpop = poparg(POP_NORMAL);    /* bpop = x */
    apop = poparg(POP_NORMAL);    /* apop = y */

    if(apop->type != SL_NUMBER ||
       bpop->type != SL_NUMBER)
        {
        errorflag = ERR_BAD_EXPRESSION;
        return;
        }

    if(apop->value.num == 0. && bpop->value.num == 0.)
        {
        errorflag = ERR_BAD_EXPRESSION;
        return;
        }

    popa->type = SL_NUMBER;
    popa->value.num = atan2(apop->value.num, bpop->value.num);
}

extern void
c_cos(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num = cos(popa->value.num);
}

extern void
c_deg(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num *= 180/PI;
}

extern void
c_exp(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num = exp(popa->value.num);
}

extern void
c_int(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num = (double) (word32) popa->value.num;
}

extern void
c_ln(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    if(popa->value.num <= 0.)
        errorflag = EVAL_ERR_BAD_LOG;
    else
        popa->value.num = log(popa->value.num);
}

extern void
c_log(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    if(popa->value.num <= 0.)
        errorflag = EVAL_ERR_BAD_LOG;
    else
        popa->value.num = log10(popa->value.num);
}

extern void
c_loopc(SYMB_TYPE *popa)
{
    popa->type = SL_NUMBER;
    popa->value.num = (double) (itercount + 1);
}

extern void
c_pi(SYMB_TYPE *popa)
{
    popa->type = SL_NUMBER;
    popa->value.num = PI;
}

extern void
c_rad(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num *= PI / 180;
}

extern void
c_sgn(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    if(popa->value.num == 0.)
        return;

    popa->value.num = (popa->value.num < 0.) ? -1. : 1.;
}

extern void
c_sin(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    if(popa->value.num == PI)
        popa->value.num = 0.;
    else
        popa->value.num = sin(popa->value.num);
}

extern void
c_sqr(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    if(popa->value.num < 0.)
        {
        errorflag = EVAL_ERR_NEG_ROOT;
        return;
        }

    popa->value.num = sqrt(popa->value.num);
}

extern void
c_tan(SYMB_TYPE *popa)
{
    if(popa->type != SL_NUMBER)
        return;

    popa->value.num = tan(popa->value.num);
}


/*****************
*                *
* Time functions *
*                *
*****************/

extern void
c_day(SYMB_TYPE *popa)
{
    time_t temp_date = extract_time(popa);
    popa->value.num = (double) (localtime(&temp_date)->tm_mday);
}

extern void
c_month(SYMB_TYPE *popa)
{
    time_t temp_date = extract_time(popa);
    popa->value.num = (double) (localtime(&temp_date)->tm_mon+1);
}

extern void
c_today(SYMB_TYPE *popa)
{
    time_t ltime = time(NULL);
    struct tm *temp_time = localtime(&ltime);

    temp_time->tm_sec = temp_time->tm_min = temp_time->tm_hour = 0;
    plusab(temp_time->tm_year, YEAR_OFFSET);
    popa->value.date = mktime(temp_time);
    popa->type = SL_DATE;
}

extern void
c_monthdays(SYMB_TYPE *popa)
{
    time_t temp_date = extract_time(popa);
    struct tm *expanded_date = localtime(&temp_date);
    intl monthno = expanded_date->tm_mon;

    /* if it's February in leap year then 29 */
    popa->value.num = (double) ((monthno == 1 &&
                                expanded_date->tm_year % 4 == 0)
                                    ? 29
                                    : days[monthno]);
}

extern void
c_year(SYMB_TYPE *popa)
{
    time_t temp_date = extract_time(popa);
    popa->value.num = (double) (localtime(&temp_date)->tm_year-YEAR_OFFSET);
}


static time_t
extract_time(SYMB_TYPE *popa)
{
    if(popa->type != SL_DATE)
        {
        errorflag = EVAL_ERR_BAD_DATE;
        return(0);
        }

    popa->type = SL_NUMBER;
    return(popa->value.date);
}

/******************
*                 *
* other functions *
*                 *
******************/

#define MS_MAX   -1
#define MS_SUM    0
#define MS_MIN    1
#define MS_CHOOSE 2
#define MS_VAR    3
#define MS_COUNTS 4
#define MS_COUNT  5

extern void
c_avg(SYMB_TYPE *popa)
{
    intl count = max_sum_min(popa, MS_SUM, NULL);

    if(count == 0)
        errorflag = EVAL_ERR_DIVIDEBY0;
    else if(popa->type == SL_NUMBER)
        popa->value.num /= count;
}

extern void
c_choose(SYMB_TYPE *popa)
{
    max_sum_min(popa, MS_CHOOSE, NULL);
}

/*******************************************************
*                                                      *
* if col is supplied an argument, it returns           *
* the col number of the slot reference in the argument *
*                                                      *
*******************************************************/

extern void
c_col(SYMB_TYPE *popa)
{
    if(popa->value.symb)
        {
        SYMB_TYPE *apop;

        apop = poparg(KEEP_SLR);

        if(errorflag)
            return;

        if(apop->type != SL_SLOTR)
            {
            errorflag = ERR_BAD_EXPRESSION;
            return;
            }
        popa->value.num = (double) (apop->value.slot.col + 1);
        }
    else
        popa->value.num = (double) (eval_col + 1);

    popa->type = SL_NUMBER;
}

extern void
c_count(SYMB_TYPE *popa)
{
    popa->value.num = (double) max_sum_min(popa, MS_COUNT, NULL);
    popa->type = SL_NUMBER;
}

extern void
c_davg(SYMB_TYPE *popa)
{
    intl count = dmax_sum_min(popa, MS_SUM);

    if(count == 0)
        errorflag = EVAL_ERR_DIVIDEBY0;
    else if(popa->type == SL_NUMBER)
        popa->value.num /= count;
}

extern void
c_dcount(SYMB_TYPE *popa)
{
    popa->value.num = (double) dmax_sum_min(popa, MS_COUNT);
    popa->type = SL_NUMBER;
}

extern void
c_dcounta(SYMB_TYPE *popa)
{
    popa->value.num = (double) dmax_sum_min(popa, MS_COUNTS);
    popa->type = SL_NUMBER;
}

extern void
c_dmax(SYMB_TYPE *popa)
{
    dmax_sum_min(popa, MS_MAX);
}

extern void
c_dmin(SYMB_TYPE *popa)
{
    dmax_sum_min(popa, MS_MIN);
}


extern void
c_dstd(SYMB_TYPE *popa)
{
    dmax_sum_min(popa, MS_VAR);
    if((popa->type == SL_NUMBER) && (popa->value.num >= 0.))
        popa->value.num = sqrt(popa->value.num);
}

extern void
c_dvar(SYMB_TYPE *popa)
{
    dmax_sum_min(popa, MS_VAR);
}

extern void
c_dsum(SYMB_TYPE *popa)
{
    dmax_sum_min(popa, MS_SUM);
}

extern void
c_if(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;

    apop = poparg(POP_NORMAL);
    bpop = poparg(POP_NORMAL);
    errorflag = 0;
    cpop = poparg(POP_NORMAL);

    if(errorflag)
        return;

    if(cpop->type != SL_NUMBER)
        {
        errorflag = EVAL_ERR_MIXED_TYPES;
        return;
        }

    if(cpop->value.num)
        *popa = *bpop;
    else
        *popa = *apop;
}

extern void
c_index(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop;

    apop = poparg(POP_NORMAL);
    bpop = poparg(POP_NORMAL);

    if(errorflag)
        return;

    if((apop->type == SL_NUMBER) && (bpop->type == SL_NUMBER))
        {
        apop->value.slot.row = ((rowt) apop->value.num) - 1;
        apop->value.slot.col = ((colt) bpop->value.num) - 1;
        apop->value.slot.doc = 0;

        if(!bad_reference(apop->value.slot.col, apop->value.slot.row))
            {
            eval_slot(apop, TRUE);

            *popa = *apop;
            return;
            }
        }

    errorflag = EVAL_ERR_BAD_INDEX;
}

extern void
c_hlookup(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop;
    SYMB_TYPE *target_symb;
    intl res, offset;

    if((target_symb = find_target_symb(popa)) == NULL || errorflag)
        return;
    if((res = look_up_symb(target_symb, TRUE, FALSE)) < 0)
        return;

    /* check its a horizontal range,
    setup apop as the parallel range and extract */
    bpop = poparg(POP_NORMAL);
    if(bpop->type != SL_NUMBER || (offset = (intl) bpop->value.num) < 0)
        {
        errorflag = ERR_BAD_EXPRESSION;
        return;
        }

    apop = poparg(NO_SUM_RANGE);
    apop->value.range.first.col = range_col_1;
    apop->value.range.second.col = range_col_2;
    apop->value.range.first.row = apop->value.range.second.row
                                = range_row_1 + offset;

    *popa = *extract_from_range(apop, res, FALSE, TRUE, TRUE);
    stack_ptr--;
}

extern void
c_lookup(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop;
    SYMB_TYPE *target_symb;
    intl res;

    if((target_symb = find_target_symb(popa)) == NULL || errorflag)
        return;
    if((res = look_up_symb(target_symb, FALSE, TRUE)) < 0)
        return;

    /* now read from last range */

    apop = poparg(NO_SUM_RANGE);   /* look for range */
    if(apop->type != SL_RANGE)
        errorflag = ERR_BAD_RANGE;

    if(errorflag)
        return;

    *popa = *extract_from_range(apop, res, TRUE, FALSE, FALSE);
    stack_ptr -= 2;
}

extern void
c_max(SYMB_TYPE *popa)
{
    max_sum_min(popa, MS_MAX, NULL);
}

extern void
c_min(SYMB_TYPE *popa)
{
    max_sum_min(popa, MS_MIN, NULL);
}

/************************************************
*                                               *
* if rand is supplied an argument, it seeds     *
* the random number generator with the argument *
*                                               *
************************************************/

extern void
c_rand(SYMB_TYPE *popa)
{
    static intl seeded = 0;

    if(popa->value.symb)
        {
        SYMB_TYPE *apop;

        apop = poparg(POP_NORMAL);

        if(apop->type != SL_NUMBER)
            {
            errorflag = ERR_BAD_EXPRESSION;
            return;
            }

        if(!seeded)
            {
            if(apop->value.num != 0.)
                srand((unsigned int) apop->value.num);
            else
                srand((unsigned int) time(NULL));

            seeded = 1;
            }
        }

    popa->type = SL_NUMBER;
#if RISCOS && TRUE
    /* Bug in 3.40 Shared C library means big rand() comes back */
    popa->value.num = ((double) (rand() & RAND_MAX)) / ((double) RAND_MAX);
#else
    popa->value.num = ((double) rand()) / ((double) RAND_MAX);
#endif
}

/****************************************************************************
*                                                                           *
* read(fileno, x, y)                                                        *
* reads pdn.lnk where n = fileno x = x coordinate in file, y = y coordinate *
* checks all three are numbers and returns result in popa                   *
*                                                                           *
****************************************************************************/

extern void
c_read(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;
    FILE *input;
    uchar array[20];

    cpop = poparg(POP_NORMAL);    /* cpop = y */
    bpop = poparg(POP_NORMAL);    /* bpop = x */
    apop = poparg(POP_NORMAL);    /* apop = fileno */

    if(apop->type != SL_NUMBER ||
       bpop->type != SL_NUMBER ||
       cpop->type != SL_NUMBER)
        {
        errorflag = EVAL_ERR_MIXED_TYPES;
        return;
        }

    /* get the file open, check the column and row, and position the pointer */
    if((input = position_file((intl) (apop->value.num),
                              (intl) (bpop->value.num),
                              (intl) (cpop->value.num))) == NULL)
        return;

    if(fread(array, sizeof(uchar), sizeof(double), input) != sizeof(double))
        {
        tracef0("failed to read from given position in linking file\n");
        errorflag = ERR_CANNOTREAD;
        }

    popa->type = SL_NUMBER;
    popa->value.num = *((double *) array);
}

/*******************************************************
*                                                      *
* if row is supplied an argument, it returns           *
* the row number of the slot reference in the argument *
*                                                      *
*******************************************************/

extern void
c_row(SYMB_TYPE *popa)
{
    if(popa->value.symb)
        {
        SYMB_TYPE *apop;

        apop = poparg(KEEP_SLR);

        if(errorflag)
            return;

        if(apop->type != SL_SLOTR)
            {
            errorflag = ERR_BAD_EXPRESSION;
            return;
            }
        popa->value.num = (double) (apop->value.slot.row + 1);
        }
    else
        popa->value.num = (double) (eval_row + 1);

    popa->type = SL_NUMBER;
}

extern void
c_round(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop;
    double result, base;
    BOOL negative = FALSE;

    bpop = poparg(POP_NORMAL);    /* bpop = number of places */
    apop = poparg(POP_NORMAL);    /* apop = number to be rounded */

    if(apop->type != SL_NUMBER ||
       bpop->type != SL_NUMBER)
        {
        errorflag = EVAL_ERR_MIXED_TYPES;
        return;
        }

    if((result = apop->value.num) < 0.)
        {
        negative = TRUE;
        result = -result;
        }

    base = pow(10.0, bpop->value.num);
    result = floor(result * base + 0.5) / base;

    popa->type = SL_NUMBER;
    popa->value.num = negative ? -result : result;
}

extern void
c_std(SYMB_TYPE *popa)
{
    max_sum_min(popa, MS_VAR, NULL);
    if((popa->type == SL_NUMBER) && (popa->value.num >= 0.))
        popa->value.num = sqrt(popa->value.num);
}

extern void
c_sum(SYMB_TYPE *popa)
{
    max_sum_min(popa, MS_SUM, NULL);
}

extern void
c_var(SYMB_TYPE *popa)
{
    max_sum_min(popa, MS_VAR, NULL);
}

extern void
c_vlookup(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop;
    SYMB_TYPE *target_symb;
    intl res, offset;

    if((target_symb = find_target_symb(popa)) == NULL || errorflag)
        return;
    if((res = look_up_symb(target_symb, TRUE, TRUE)) < 0)
        return;

    /* check its a vertical range,
    setup apop as the parallel range and extract */
    bpop = poparg(POP_NORMAL);
    if(bpop->type != SL_NUMBER || (offset = (intl) bpop->value.num) < 0)
        {
        errorflag = ERR_BAD_EXPRESSION;
        return;
        }

    apop = poparg(NO_SUM_RANGE);
    apop->value.range.second.col = apop->value.range.first.col
                                 = range_col_1 + offset;
    apop->value.range.first.row = range_row_1;
    apop->value.range.second.row = range_row_2;

    *popa = *extract_from_range(apop, res, TRUE, TRUE, TRUE);
    stack_ptr--;
}

/*****************************************************************************
*                                                                            *
* write(fileno, x, y, value)                                                 *
* writes pdn.lnk where n = fileno x = x coordinate in file, y = y coordinate *
* checks all 4 are numbers and returns result in popa                        *
*                                                                            *
*****************************************************************************/

extern void
c_write(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop, *dpop;
    FILE *output;

    dpop = poparg(POP_NORMAL);    /* dpop = value */
    cpop = poparg(POP_NORMAL);    /* cpop = y */
    bpop = poparg(POP_NORMAL);    /* bpop = x */
    apop = poparg(POP_NORMAL);    /* apop = fileno */

    if(apop->type != SL_NUMBER ||
       bpop->type != SL_NUMBER ||
       cpop->type != SL_NUMBER ||
       dpop->type != SL_NUMBER)
        {
        errorflag = EVAL_ERR_MIXED_TYPES;
        return;
        }

    /* get the file open, check the column and row,
    and position the pointer */
    if((output = position_file((intl) apop->value.num,
                               (intl) bpop->value.num,
                               (intl) cpop->value.num)) == NULL)
        return;

    if(fwrite((void *) &dpop->value.num,
                       sizeof(uchar),
                       sizeof(double),
                       output) != sizeof(double))
        {
        tracef0("failed to write to given position in linking file\n");
        errorflag = ERR_CANNOTWRITE;
        }
    else
        tracef0("write to linking file ok\n");

    popa->type = SL_NUMBER;
    popa->value.num = dpop->value.num;
}


/************************
*                       *
*  create linking file  *
*                       *
************************/

#if RISCOS
#define LINKFILE_BUFSIZ 256
#else
#define LINKFILE_BUFSIZ 256
#endif

extern void
CreateLinkingFile_fn(void)
{
    char prefix[MAX_FILENAME];
    char filename[MAX_FILENAME];
    char fbuf[LINKFILE_BUFSIZ];
    FILE *update_put;
    long items;
    long i;
    double zero = (double) 0;
    LINKSTART start_of_file;

    get_prefix(prefix, TRUE);

    while(dialog_box(D_CREATE))
        {
        if( (d_create[0].option == 0)   ||
            (d_create[1].option == 0)   ||
            (d_create[2].option == 0)   )
            {
            reperr_null(ERR_BAD_PARM);
            continue;
            }

        clslnf();

        recalc_bit = TRUE;

        /* get filename in form %spd%d.lnk */
        sprintf(filename, ZS_LINKFILE_ZD_STR, prefix, (int) d_create[0].option);

        /* check if it exists and shouldn't be overwritten */
        if(!checkoverwrite(filename))
            return;

        update_put = myfopen(filename, write_str);

        if(!update_put)
            {
            dialog_box_end();
            reperr(ERR_CANNOTOPEN, filename);
            return;
            }

        mysetvbuf(update_put, fbuf, LINKFILE_BUFSIZ);

        start_of_file.one   = 'R';
        start_of_file.two   = 'J';
        start_of_file.three = 'M';
        start_of_file.mrow  = (uchar) d_create[2].option;
        start_of_file.mcol  = (uchar) d_create[1].option;
        start_of_file.ident = LINK_IDENTIFIER;

        if(fwrite(&start_of_file,
                  sizeof(uchar),
                  sizeof(LINKSTART),
                  update_put) != sizeof(LINKSTART))
            {
            myfclose(update_put);
            dialog_box_end();
            reperr_null(ERR_CANNOTWRITE);
            return;
            }

        /* expand file filling with -1 */
        items = ((long) d_create[2].option) * ((long) d_create[1].option);

        for(i = sizeof(LINKSTART); i < sizeof(LINKSTART) + items; i++)
            if(fwrite((void *) &zero, sizeof(uchar), sizeof(double),
                                    update_put) != sizeof(double))
                {
                myfclose(update_put);
                dialog_box_end();
                reperr_null(ERR_CANNOTWRITE);
                return;
                }

        myfclose(update_put);

        if(dialog_box_ended())
            break;
        }
}


/********************************
*                               *
*  close all the linking files  *
*                               *
********************************/

extern void
clslnf(void)
{
    LINK_FILE *lptr;

    while(filesopen > 0)
        {
        lptr = file_table + (--filesopen);
        myfclose(lptr->fptr);
        fixed_dispose(&lptr->fbuf);
        }

    /* must leave filesopen at 0 when all closed */
}


/***************************************************************************
*                                                                          *
* do a database function                                                   *
* this justs sets up a condition for max_sum_min to examine                *
* NULL means all values in range                                           *
* all database functions just have two parameters, a range and a condition *
*                                                                          *
***************************************************************************/

static intl
dmax_sum_min(SYMB_TYPE *popa, intl func)
{
    SYMB_TYPE *apop;
    uchar compiled[LIN_BUFSIZ];
    char tobecompiled[LIN_BUFSIZ];
    char *i, *o;
    intl len, instring;
    BOOL slrflag;

    /* get the condition */
    if((apop = poparg(POP_NORMAL))->type != SL_INTSTR)
        {
        errorflag = ERR_BAD_EXPRESSION;
        return(0);
        }

    /* take the internal string from apop and compile it */
    slrbinstr(tobecompiled, apop->value.str.ptr);

    /* swap around the hashes and dollars */
    i = o = tobecompiled;
    instring = 0;
    do
        {
        switch(*i)
            {
            case '\'':
            case '\"':
                instring ^= 1;
                *o++ = *i;
                break;
            case '#':
                if(!instring)
                    *o++ = '$';
                else
                    *o++ = *i;
                break;
            case '$':
                if(!instring)
                    break;
                *o++ = *i;
                break;

            default:
                *o++ = *i;
                break;
            }
        }
    while(*i++);

    if((len = exp_compile(compiled, tobecompiled,
                          &slrflag, LIN_BUFSIZ-1)) < 0)
        {
        errorflag = ERR_BAD_EXPRESSION;
        return(0);
        }

    /* tell max_sum_min to read just the one parameter */
    popa->value.symb = 1;
    return(max_sum_min(popa, func, *compiled == OPR_END ? NULL : compiled));
}


/**************************************************
*                                                 *
* extract the ith element from the range in popa, *
* and return a pointer to it                      *
*                                                 *
**************************************************/

static SYMB_TYPE *
extract_from_range(SYMB_TYPE *popa, intl res, BOOL goingdown,
                    BOOL accept_nearest, intl check_tree)
{
    for(setup_range(popa) ; res >= 0; res--)
        {
        popa = next_in_range(goingdown, accept_nearest, check_tree);
        if(!popa)
            {
            errorflag = ERR_BAD_RANGE;
            return(NULL);
            }
        }
    return(popa);
}


/************************************************************
*                                                           *
* extract the target symb from two places down on the stack *
*                                                           *
************************************************************/

static SYMB_TYPE *
find_target_symb(SYMB_TYPE *popa)
{
    SYMB_TYPE *old_stack;
    SYMB_TYPE *target_symb;

    IGNOREPARM(popa);

    /* read the top but two item off stack */
    old_stack = stack_ptr;
    stack_ptr -= 2;
    target_symb = poparg(POP_NORMAL);
    stack_ptr = old_stack;
    return((target_symb->type == SL_ERROR) ? NULL : target_symb);
}


/***************************************************
*                                                  *
* open a linking file and put it in the file table *
*                                                  *
***************************************************/

static LINK_FILE *
getfile(intl fileno)
{
    char filename[MAX_FILENAME];
    LINK_FILE *lptr;
    LINKSTART start_of_file;
    intl i;

    tracef1("[getfile(%d)]\n", fileno);

    for(i = 0, lptr = file_table; i < filesopen; ++i, ++lptr)
        {
        tracef2("[comparing fileno %d with stored fileno %d]\n", fileno, lptr->filenumber);
        if(fileno == lptr->filenumber)
            {
            tracef1("[found in table at &%p]\n", lptr);
            return(lptr);
            }
        }

    if(filesopen == FILES_ALLOWED)
        {
        errorflag = EVAL_ERR_TOOMANYFILES;
        return(NULL);
        }

    sprintf(filename, ZS_LINKFILE_ZD_STR, NULLSTR, fileno);

    if(!add_path(filename, NULL, TRUE))
        return(NULL);

    lptr = file_table + filesopen;
    tracef3("[saving new file %d at &%p, index %d]\n", fileno, lptr, filesopen);
    lptr->filenumber = fileno;
    lptr->fptr       = myfopen(filename, update_str);

    if(lptr->fptr)
        {
        lptr->fbuf = fixed_malloc(LINKFILE_BUFSIZ);

        mysetvbuf(lptr->fptr, lptr->fbuf, LINKFILE_BUFSIZ);

        if( (fread(&start_of_file,
                   sizeof(uchar),
                   sizeof(LINKSTART),
                   lptr->fptr) == sizeof(LINKSTART))  &&
            (start_of_file.ident == LINK_IDENTIFIER)  )
            {
            tracef2("[opened linking file: %d col(s) %d row(s)]\n", start_of_file.mrow, start_of_file.mcol);

            lptr->mrow = start_of_file.mrow;
            lptr->mcol = start_of_file.mcol;
    
            ++filesopen;

            return(lptr);
            }
        else
            {
            tracef0("failed to read linking file/not a linking file\n");
            myfclose(lptr->fptr);
            fixed_dispose(&lptr->fbuf);
            }
        }
    else
        tracef0("failed to open linking file\n");

    errorflag = ERR_CANNOTOPEN;
    return(NULL);
}

/********************************************************************
*                                                                   *
* look the symbol up on the range one down on the stack             *
* return the offset into the range it was found, -1 if not in range *
*                                                                   *
********************************************************************/

static intl
look_up_symb(SYMB_TYPE *target_symb, BOOL accept_nearest,
                    BOOL goingdown)
{
    SYMB_TYPE *apop;
    SYMB_TYPE *old_stack;
    intl offset;

    /* read the top but one item off stack */
    old_stack = stack_ptr;
    stack_ptr--;

    apop = poparg(NO_SUM_RANGE);   /* look for range */
    if(errorflag || apop->type != SL_RANGE)
        {
        errorflag = ERR_BAD_RANGE;
        stack_ptr--;
        return(-1);
        }

    setup_range(apop);
    for(offset = 0 ; ; offset++)
        {
        apop = next_in_range(goingdown, accept_nearest, FALSE);
        if(apop == NULL)
            {
            if(accept_nearest && target_symb->type == SL_NUMBER)
                {
                offset--;
                break;
                }

            errorflag = EVAL_ERR_LOOKUP;
            stack_ptr--;
            return(-1);
            }

        /* compare apop with target_symb */
        if(symbcmp(target_symb, apop) == 0)
            break;

        if(accept_nearest && apop->type == SL_NUMBER &&
          target_symb->type == SL_NUMBER)
            if(apop->value.num > target_symb->value.num)
                {
                /* gone too far in hlookup or vlookup, return previous */
                if(offset == 0)
                    errorflag = EVAL_ERR_LOOKUP;
                else
                    offset--;
                break;
                }
        }

    stack_ptr = old_stack;
    return(offset);
}

/****************************************************************************
*                                                                           *
* max_sum_min reads arguments from the stack and returns the minimum, sum   *
* or maximum of them in popa.  sign is -1 for max, 0 for sum or 1 for min.  *
* This sets up the initial result and deals with the max and min cases. The *
* sum case is handled separately.  It returns the number of numeric slots   *
* it has processed, ie count                                                *
*                                                                           *
* The condition specifies which slots in the specified range are to be      *
* included in the calculation. NULL specifies all slots, otherwise a string *
* which gets passed to selrow                                               *
*                                                                           *
* The result is returned in popa                                            *
* On entry popa->value.symb is number of arguments                          *
*                                                                           *
****************************************************************************/

static intl
max_sum_min(SYMB_TYPE *popa, intl func, uchar *condition)
{
    intl i, count, nargs, processed;
    BOOL inrange, first;
    SYMB_TYPE *apop, result, *firstparm;
    double x_squared_sum = 0.;

    result.type = SL_NUMBER;
    result.value.num = 0.;

    count = popa->value.symb;
    firstparm = stack_ptr - count + 1;

    if(func == MS_VAR)
        x_squared_sum = 0.;

    /*
    There are count parameters on stack for us to examine.  Unfortunately
    they are the wrong way up, ie the first popped is the last we want.
    So we have to fudge the stack pointer to read them off.
    */

    for(i = 0, first = TRUE, inrange = FALSE, processed = 0, nargs = 0;
        i < count;
        i++)
        {
        stack_ptr = firstparm + i;

        apop = poparg(NO_SUM_RANGE);

        if(errorflag)
            {
            result.type = SL_ERROR;
            result.value.symb = errorflag;
            continue;
            }

        do
            {
            intl this_datum_type;

            /* database function must have a range,
            a whole range, and nothing but a range
            ranges get expanded to slots */
            if(apop->type == SL_RANGE)
                {
                inrange = TRUE;
                setup_range(apop);
                }
            else if(!inrange && condition)
                {
                errorflag = ERR_BAD_EXPRESSION;
                return(0);
                }

            this_datum_type = apop->type;
            if(inrange)
                {
                if((apop = next_in_range(TRUE, FALSE, FALSE)) == NULL)
                    {
                    inrange = FALSE;
                    break;
                    }

                this_datum_type = apop->type;

                /* for each range element check it satisfies the database condition */
                if(condition && !true_condition(condition, TRUE))
                    continue;
                }

            if((this_datum_type != SL_NUMBER) &&
               (func != MS_CHOOSE) &&
               (func != MS_COUNTS))
                continue;

            processed++;

            if(first)
                {
                switch(func)
                    {
                    case MS_CHOOSE:
                        /* first value off is index into list */
                        if(apop->type != SL_NUMBER)
                            /* read the args but don't set anything */
                            nargs = -1;
                        else
                            {
                            processed = 0;
                            nargs = (intl) apop->value.num;
                            }
                        break;

                    case MS_VAR:
                        result = *apop;
                        x_squared_sum = result.value.num * result.value.num;
                        break;

                    default:
                        result = *apop;
                        break;
                    }

                first = FALSE;
                continue;
                }

            /* latest result in result, new result in apop */
            switch(func)
                {
                case MS_CHOOSE:
                    if(processed == nargs)
                        result = *apop;
                    break;

                case MS_VAR:
                    /* result holds sum of x, so fall thru to MS_SUM */
                    x_squared_sum += apop->value.num * apop->value.num;

                    /* deliberate fall-thru */

                case MS_SUM:
                    result.value.num += apop->value.num;
                    break;

                case MS_MIN:
                    if(symbcmp(&result, apop) > 0)
                        result = *apop;
                    break;

                case MS_MAX:
                    if(symbcmp(&result, apop) < 0)
                        result = *apop;
                    break;

                case MS_COUNT:
                case MS_COUNTS:
                    break;

                default:
                    break;
                }
            }
        while(inrange);
        }

    switch(func)
        {
        case MS_VAR:

            if(result.type != SL_ERROR)
                {
                /* result contains sum of x,
                x_squared_sum contains sum of the x-squareds */
                if(processed < 2)
                    {
                    errorflag = ERR_BAD_EXPRESSION;
                    return(0);
                    }
                result.value.num = processed * x_squared_sum -
                                   result.value.num * result.value.num;
                result.value.num /= (processed * (processed - 1));
                }
            break;

        case MS_CHOOSE:

            if((nargs <= 0) || (nargs > processed))
                errorflag = EVAL_ERR_BAD_INDEX;
            break;
        }

    *popa = result;

    stack_ptr = firstparm - 1;
    return(processed);
}

/******************************************************************
*                                                                 *
* open a linking file and move to position x,y                    *
* the file may be open already                                    *
* if an error occurs display a message and return null, otherwise *
* return the file pointer                                         *
*                                                                 *
******************************************************************/

static FILE *
position_file(intl fileno, intl x, intl y)
{
    LINK_FILE *lptr;

    trace_on();
    tracef3("[position_file(%d, %d, %d)]\n", fileno, x, y);
    trace_off();

    if((x < 1)  ||  (y < 1)  ||  (fileno < 1)  ||  (fileno > LARGEST_LINK_FILE))
        {
        errorflag = ERR_BAD_PARM;
        return(NULL);
        }

    trace_on();
    lptr = getfile(fileno);
    trace_off();
    if(!lptr)
        {
        if(!errorflag)
            errorflag = ERR_CANNOTOPEN;
        return(NULL);
        }

    if((x > lptr->mcol)  ||  (y > lptr->mrow))
        {
        errorflag = EVAL_ERR_BAD_INDEX;
        return(NULL);
        }

    if(fseek(lptr->fptr,
             sizeof(LINKSTART) + sizeof(double) * (((long)y-1)*((long)(lptr->mcol))+((long)x-1)),
             SEEK_SET))
        {
        tracef0("failed to seek in linking file\n");
        errorflag = EVAL_ERR_BAD_INDEX;
        return(NULL);
        }

    return(lptr->fptr);
}

/************************************************************************
*                                                                       *
* 1-2-3 financial functions.                                            *
*                                                                       *
* on entry to all of these popa->type points to number of args          *
* has to return numeric result in popa                                  *
*                                                                       *
* See 1-2-3 Reference manual pp 264-272                                 *
*                                                                       *
************************************************************************/

extern void
c_cterm(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;

    if(!get_three_nums(popa, &apop, &bpop, &cpop))
        return;

    /*  result = ln(fv/pv) / ln(1+int) */
    popa->value.num = log(bpop->value.num /
                      cpop->value.num) /
                      log(1. + apop->value.num);
}

extern void
c_ddb(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop, *dpop;
    double cost, salvage, value, curperiod;
    intl life, period, i;

    if(!get_four_nums(popa, &apop, &bpop, &cpop, &dpop))
        return;

    value = cost = apop->value.num;
    salvage      = bpop->value.num;
    life         = (intl) cpop->value.num;
    period       = (intl) dpop->value.num;

    if(cost < 0. ||
       salvage > cost ||
       life < 1 ||
       period < 1 ||
       period > life)
        {
        errorflag = ERR_BAD_PARM;
        return;
        }
    for(i = 0; i < period; i++)
        {
        curperiod = (value * 2) / life;
        if(value - curperiod < salvage)
            curperiod = value - salvage;
        value -= curperiod;
        }

    popa->value.num = curperiod;
}

extern void
c_fv(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;

    if(!get_three_nums(popa, &apop, &bpop, &cpop))
        return;

    /* pmt * ((1+int)^n-1) / int */

    popa->value.num = apop->value.num *
                      (pow(1. + bpop->value.num, cpop->value.num) - 1) /
                      bpop->value.num;
}

extern void
c_irr(SYMB_TYPE *popa)
{
    irr_npv_body(popa, TRUE);
}

/*********************************************************
*                                                        *
* net present value                                      *
* first parameter is interest rate, then range of values *
*                                                        *
*********************************************************/

extern void
c_npv(SYMB_TYPE *popa)
{
    irr_npv_body(popa, FALSE);
}

extern void
c_pmt(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;

    if(!get_three_nums(popa, &apop, &bpop, &cpop))
        return;

    /* prin * int / (1-(int+1)^(-n)) */
    popa->value.num = apop->value.num * bpop->value.num /
                      (1. - pow(bpop->value.num + 1., - cpop->value.num));
}

extern void
c_pv(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;

    if(!get_three_nums(popa, &apop, &bpop, &cpop))
        return;

    /* pmt * (1-(1+int)^(-n) / int */
    popa->value.num = apop->value.num *
                      (1. - pow(1. + bpop->value.num, - cpop->value.num))
                      / bpop->value.num;
}

extern void
c_rate(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;

    if(!get_three_nums(popa, &apop, &bpop, &cpop))
        return;

    /*  (fv / pv) ^ (1/n) -1  */

    popa->value.num = pow((apop->value.num /
                      bpop->value.num), 1. / cpop->value.num) - 1.;
}

extern void
c_sln(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;

    if(!get_three_nums(popa, &apop, &bpop, &cpop))
        return;

    popa->value.num = (apop->value.num - bpop->value.num)
                      / cpop->value.num;
}

extern void
c_syd(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop, *dpop;

    if(!get_four_nums(popa, &apop, &bpop, &cpop, &dpop))
        return;

    /* (c-s) * (n-p+1) / (n*(n+1)/2) */

    popa->value.num = ((apop->value.num - bpop->value.num) *
                       (cpop->value.num - dpop->value.num + 1.)) /
                      ((cpop->value.num * (cpop->value.num + 1.) / 2.));
}

extern void
c_term(SYMB_TYPE *popa)
{
    SYMB_TYPE *apop, *bpop, *cpop;

    if(!get_three_nums(popa, &apop, &bpop, &cpop))
        return;

    /* ln(1+(fv * int/pmt)) / ln(1+int)   */
    popa->value.num = log(1. +
                      (cpop->value.num * bpop->value.num / apop->value.num)) /
                      log(1. + bpop->value.num);
}

static void
calc_npv(SYMB_TYPE *popa, SYMB_TYPE *firstparm, intl parms, double interest)
{
    SYMB_TYPE *apop;
    intl i;
    double current_interest = 1.;

    popa->value.num = 0.;

    /* get elements off stack */
    for(i = 1; i < parms; i++)
        {
        BOOL inrange = FALSE;

        stack_ptr = firstparm+i;
        apop = poparg(NO_SUM_RANGE);
        if(errorflag)
            return;

        do  {
            if(apop->type == SL_RANGE)
                {
                inrange = TRUE;
                setup_range(apop);
                }

            if(inrange)
                {
                apop = next_in_range(TRUE, FALSE, FALSE);
                if(!apop)
                    {
                    inrange = FALSE;
                    break;
                    }
                }

            current_interest *= interest;
            plusab(popa->value.num, apop->value.num / current_interest);
            }
        while(inrange);
        }
}

static void
calc_irr(SYMB_TYPE *popa, SYMB_TYPE *firstparm, intl parms, double guess)
{
    double lastnpvr, lastguess;
    intl i;

    lastguess = guess / 2;
    lastnpvr = 1.;

    for(i = 0; i < 20; i++)
        {
        double temp, npvr;

        calc_npv(popa, firstparm, parms, guess);
        npvr = popa->value.num;

        if(errorflag)
            return;

        if(fabs(npvr) < .0000001)
            break;
        temp = guess;
        guess -= npvr * (guess - lastguess) / (npvr - lastnpvr);

        lastnpvr = npvr;
        lastguess = temp;
        }

    if(i >= 20)
        errorflag = EVAL_ERR_IRR;
    else
        popa->value.num = (guess - 1);
}

static BOOL
get_four_nums(SYMB_TYPE *popa, SYMB_TYPE **apop, SYMB_TYPE **bpop, SYMB_TYPE **cpop, SYMB_TYPE **dpop)
{
    if(!get_three_nums(popa, bpop, cpop, dpop))
        return(FALSE);

    *apop = poparg(POP_NORMAL);

    if(errorflag || (*apop)->type != SL_NUMBER)
        {
        errorflag = ERR_BAD_PARM;
        return(FALSE);
        }

    return(TRUE);
}

static BOOL
get_three_nums(SYMB_TYPE *popa, SYMB_TYPE **apop, SYMB_TYPE **bpop, SYMB_TYPE **cpop)
{
    *cpop = poparg(POP_NORMAL);
    *bpop = poparg(POP_NORMAL);
    *apop = poparg(POP_NORMAL);
    if(errorflag || (*apop)->type != SL_NUMBER ||
       (*bpop)->type != SL_NUMBER || (*cpop)->type != SL_NUMBER)
        {
        errorflag = ERR_BAD_PARM;
        return(FALSE);
        }

    popa->type = SL_NUMBER;
    return(TRUE);
}

static void
irr_npv_body(SYMB_TYPE *popa, BOOL irr)
{
    SYMB_TYPE *apop;
    double interest;
    SYMB_TYPE *firstparm;
    intl count;

    /* deal with stack - parameters come in in inverse order so read them
    from bottom of stack upwards */
    count = popa->value.symb;
    firstparm = stack_ptr-count + 1;

    /* get first parameter which is interest rate */

    stack_ptr = firstparm;
    apop = poparg(POP_NORMAL);
    if(apop->type != SL_NUMBER)
        {
        errorflag = ERR_BAD_PARM;
        return;
        }
    interest = apop->value.num + 1.;

    if(irr)
        calc_irr(popa, firstparm, count, interest);
    else
        calc_npv(popa, firstparm, count, interest);

    popa->type = SL_NUMBER;
    stack_ptr = firstparm-1;
}

/* end of semantic.c */
