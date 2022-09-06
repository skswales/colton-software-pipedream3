/* include.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       include.h - standard headers
 * Author:      MRJC, RJM, SKS
 * History:
 *  0.01    31-Jan-89   SKS derived from incarth.h and include.h
 *  0.02    01-Feb-89   SKS added #include <setjmp.h> to MS case
 *  0.03    22-Feb-89   SKS changed reference to newctype.h
 *  0.04    06-Mar-89   SKS added #include <stdarg.h> to A&R case
 *  0.05    09-Mar-89   SKS added #ifdef TARGET_IS_ARM for cc319 compiler
 *                          moaning about Non-ANSI #includes
 *  0.06    17-Mar-89   SKS added #include <assert.h> so assert(0) works!
 *  0.07    22-Mar-89   SKS added #include "myclib:mytrace.h"
*/

#ifndef __include_h
#define __include_h

/* standard header */

#if ARTHUR || RISCOS
#include "coltsoft.h"
#else
#include <coltsoft.h>
#endif


/* standard includes for all programs */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 199901L
/* C99 headers */
#include <stdint.h>
#include <stdbool.h>
#define HAS_C99 1
#endif
#endif

#if !defined(HAS_C99)
#include "c99types.h"
#endif

#include "myassert.h"

#include "report.h"

#include    "trace.h"

#if ARTHUR || RISCOS
/* new allocation */
#include "ext.alloc"
#else
#include "alloc.ext"
#endif


#if ARTHUR || RISCOS
/* standard includes for Archimedes ARTHUR and RISCOS programs */

/* new memory move */
#include "copymem.h"

#if defined(WATCH_ALLOCS)
#include "<WimpLib$Dir>.tralloc.h"
#endif


#elif MS
/* standard includes for MS-DOS programs */

#include <conio.h>
#include <dos.h>
#include <memory.h>
#include <malloc.h>
#include <io.h>

#endif

#endif  /* __include_h */

/* end of include.h */
