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

#ifndef __mystandard__include_h
#define __mystandard__include_h

#if defined(RELEASED)
#define NDEBUG
#endif

/* standard includes for cwimp library */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>

#include "trace.h"

#include "os.h"
#include "misc.h"

#endif  /* __mystandard__include_h */

/* end of include.h */
