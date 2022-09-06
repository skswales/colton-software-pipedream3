/* myassert.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Defines the assert(exp) macro */

#if (RISCOS || WINDOWS) && !defined(NDEBUG)

#if WINDOWS
#define assert(exp)             \
    {                           \
    if(!(exp))                  \
        {                       \
        char szBuffer[64];      \
        sprintf(szBuffer, "Assertion failed: " #exp ", file " __FILE__ ", line %d", __LINE__);      \
        if(IDCANCEL == MessageBox(NULL, szBuffer, "Assertion failure", MB_OKCANCEL | MB_ICONHAND))  \
            FatalExit(-1);      \
        }                       \
    }
#elif RISCOS
#ifndef __wimpt_h
#include "wimpt.h"
#endif
#define assert(exp)             \
    {                           \
    if(!(exp))                  \
        {                       \
        int out;                \
        os_error e;             \
        e.errnum = 0;           \
        sprintf(e.errmess, "Assertion failed: " #exp ", file " __FILE__ ", line %d", __LINE__);                             \
        (void) os_swi3r(0x400DF | os_X, (int) &e, wimp_EOK | wimp_ECANCEL, (int) wimpt_programname(), NULL, &out, NULL);    \
        if(out & wimp_ECANCEL)  \
            wimpt_abort(&e);    \
        }                       \
    }
#endif

#elif !defined(NDEBUG) 
/* use standard assert definition */

#include <assert.h>

#else   /* NDEBUG */

#define assert(exp)

#endif  /* NDEBUG */

/* end of myassert.h */
