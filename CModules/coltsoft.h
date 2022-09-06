/* coltsoft.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/***************************************************
*                                                  *
* definition of standard types for CTYPE           *
*                                                  *
* MRJC                                             *
* September 1989                                   *
*                                                  *
***************************************************/

/* standard constants */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef LF
#define LF 10
#endif
#ifndef CR
#define CR 13
#endif

/* standard types */
#if ARTHUR || RISCOS
#define uchar char
#elif MS || WINDOWS
#   if defined(_CHAR_UNSIGNED)
#       define uchar char
#   else
        typedef unsigned char uchar;
#   endif
#endif
#if ARTHUR || RISCOS || MS || WINDOWS
typedef signed char schar;

typedef int ints;
typedef int intl;

typedef short int word16;
typedef long int word32;
typedef unsigned short int uword16;
typedef unsigned long int uword32;

typedef void FARP memp;
typedef char FARP bytep;
#endif

/* type for worst case alignment */
#if ARTHUR || RISCOS
typedef int align;
#elif MS || WINDOWS
typedef uchar align;
#endif

#define BOOL intl

/* useful max/min macros */
#ifndef MAX
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

#ifndef max
#define max(A,B) ((A) > (B) ? (A) : (B))
#endif
#ifndef min
#define min(A,B) ((A) < (B) ? (A) : (B))
#endif

#define elif        else if

#define IGNOREPARM(p)   p=p

/* end of coltsoft.h */
