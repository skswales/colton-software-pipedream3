/* mscpatch.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*************************************************
*                                                *
* SKS                                            *
* February 1990                                  *
*                                                *
*************************************************/

#include "flags.h"


#include "mscpatch.ext"


/* this module MUST be compiled using the same model as the library
 * we are going to link with as the compiler generates calls to
 * internal hidden functions such as __ftol to do it's nasty work.
*/
#if LLIB && !defined(M_I86LM)
#   error module mscpatch must be compiled using -AL
#endif


#undef ulshl
extern unsigned long EXTERNAL
ulshl(unsigned long ulval, int sval)
{
    return(ulval << sval);
}


#undef ulshr
extern unsigned long EXTERNAL
ulshr(unsigned long ulval, int sval)
{
    return(ulval >> sval);
}


#undef ftoi
extern int EXTERNAL
ftoi(double dval)
{
    return((int) dval);
}


#undef ftol
extern long EXTERNAL
ftol(double dval)
{
    return((long) dval);
}


/* end of mscpatch.c */
