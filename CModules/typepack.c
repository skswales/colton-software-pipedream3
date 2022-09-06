/* typepack.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*************************************************
*                                                *
* library module for type packing and unpacking  *
* MRJC                                           *
* November 1989                                  *
*                                                *
*************************************************/

/* external flags */
#include "flags.h"

/* external header */
#if MS || WINDOWS
#include "typepack.ext"
#else
#include "ext.typepack"
#endif

extern double   EXTERNAL readdouble(bytep arg);
extern uword32  EXTERNAL readuword(bytep from, intl size);
extern uword16  EXTERNAL readuword16(bytep arg);
extern uword32  EXTERNAL readuword32(bytep arg);
extern word16   EXTERNAL readword16(bytep arg);

extern intl     EXTERNAL writedouble(bytep to, double dval);
extern intl     EXTERNAL writeuword(bytep to, uword32 word, intl size);
extern intl     EXTERNAL writeuword16(bytep to, uword16 word);
extern intl     EXTERNAL writeuword32(bytep to, uword32 word);
extern intl     EXTERNAL writeword16(bytep to, word16 word);

/****************************
*                           *
* read a double from memory *
*                           *
****************************/

#if 0
/* compiled off cos generates error at the moment 10/11/89 */

extern double EXTERNAL
readdouble(bytep arg)
{
#if MS || WINDOWS

    return(*((const double FARP) arg));

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

#endif

/************************************
*                                   *
* read an unsigned word from memory *
*                                   *
************************************/

extern uword16 EXTERNAL
readuword16(bytep arg)
{
#if MS || WINDOWS

    return(*((const uword16 FARP) arg));

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

/******************************************
*                                         *
* read an unsigned word of specified size *
*                                         *
******************************************/

extern uword32 EXTERNAL
readuword(bytep from, intl size)
{
    uword32 res;
    intl i;

    for(res = 0, i = size - 1; i >= 0; i--)
        {
        res = ulshl(res, 8);
        res |= from[i];
        }

    return(res);
}

/*****************************************
*                                        *
* read an unsigned long word from memory *
*                                        *
*****************************************/

extern uword32 EXTERNAL
readuword32(bytep arg)
{
#if MS || WINDOWS

    return(*((const uword32 FARP) arg));

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

/*********************************
*                                *
* read a signed word from memory *
*                                *
*********************************/

extern word16 EXTERNAL
readword16(bytep arg)
{
#if MS || WINDOWS

    return(*((const word16 FARP) arg));

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

/****************
*               *
* output double *
*               *
****************/

#if 0
/* compiled off cos generates error at the moment 15/11/89 */

extern intl EXTERNAL
writedouble(bytep to, double dval)
{
#if MS || WINDOWS

    *((double FARP) to) = dval;
    return(sizeof(double));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        double fpval;
        uchar fpbytes[sizeof(double)];
        } fp;
    intl i;
    uchar *b;

    fp.fpval = dval;
    b = fp.fpbytes;
    for(i = 0; i < sizeof(double); ++i)
        *to++ = *b++;

    return(sizeof(double));

#endif
}

#endif

/********************************************
*                                           *
* write out unsigned word of specified size *
*                                           *
********************************************/

extern intl EXTERNAL
writeuword(bytep to, uword32 word, intl size)
{
    intl i = size;

    while(i--)
        {
        *to++ = (char) (word & 0xFF);
        word = ulshr(word, 8);
        }

    return(size);
}

/*****************
*                *
* output uword16 *
*                *
*****************/

extern intl EXTERNAL
writeuword16(bytep to, uword16 word)
{
#if MS || WINDOWS

    *((uword16 FARP) to) = word;
    return(sizeof(uword16));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        uword16 uword;
        uchar uwbytes[sizeof(uword16)];
        } uw;
    intl i;
    uchar *b;

    uw.uword = word;
    b = uw.uwbytes;
    for(i = 0; i < sizeof(uword16); i++)
        *to++ = *b++;

    return(sizeof(uword16));

#endif
}

/*****************
*                *
* output uword32 *
*                *
*****************/

extern intl EXTERNAL
writeuword32(bytep to, uword32 word)
{
#if MS || WINDOWS

    *((uword32 FARP) to) = word;
    return(sizeof(uword32));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        uword32 uword;
        uchar uwbytes[sizeof(uword32)];
        } uw;
    intl i;
    uchar *b;

    uw.uword = word;
    b = uw.uwbytes;
    for(i = 0; i < sizeof(uword32); i++)
        *to++ = *b++;

    return(sizeof(uword32));

#endif
}

/****************
*               *
* output word16 *
*               *
****************/

extern intl EXTERNAL
writeword16(bytep to, word16 word)
{
#if MS || WINDOWS

    *((word16 FARP) to) = word;
    return(sizeof(word16));

#elif ARTHUR || RISCOS

    /* this for the ARM */
    union
        {
        word16 word;
        uchar wbytes[sizeof(word16)];
        } w;
    intl i;
    uchar *b;

    w.word = word;
    b = w.wbytes;
    for(i = 0; i < sizeof(word16); ++i)
        *to++ = *b++;

    return(sizeof(word16));

#endif
}

/* end of typepack.c */
