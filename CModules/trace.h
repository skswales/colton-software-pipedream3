/* trace.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       trace.h - allow tracing under RISC OS, no-op elsewhere
 * Author:      Stuart K. Swales 22-Mar-1989
*/

#ifndef __trace_h
#define __trace_h

#if RISCOS

#include "tracew.h"

#else

#ifndef TRACE
#define TRACE 0
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if defined(MS_TRACE)

#define tracef0(a) printf((a))
#define tracef1(a, b) printf((a), (b))
#define tracef2(a, b, c) printf((a), (b), (c))
#define tracef3(a, b, c, d) printf((a), (b), (c), (d))
#define tracef4(a, b, c, d, e) printf((a), (b), (c), (d), (e))
#define tracef5(a, b, c, d, e, f) printf((a), (b), (c), (d), (e), (f))
#define tracef6(a, b, c, d, e, f, g) printf((a), (b), (c), (d), (e), (f), (g))
#define tracef7(a, b, c, d, e, f, g, h) printf((a), (b), (c), (d), (e), (f), (g), (h))
#define tracef8(a, b, c, d, e, f, g, h, i) printf((a), (b), (c), (d), (e), (f), (g), (h), (i))

#define trace_string(a) ((a) ? (a) : ("<NULL>"))

#else

#define tracef0(a)
#define tracef1(a, b)
#define tracef2(a, b, c)
#define tracef3(a, b, c, d)
#define tracef4(a, b, c, d, e)
#define tracef5(a, b, c, d, e, f)
#define tracef6(a, b, c, d, e, f, g)
#define tracef7(a, b, c, d, e, f, g, h)
#define tracef8(a, b, c, d, e, f, g, h, i)

#endif

#define trace_on()
extern void (trace_on)(void);

#define trace_off()
extern void (trace_off)(void);

#define trace_is_on()              FALSE
extern BOOL (trace_is_on)(void);

#define trace_clearscreen()
extern void (trace_clearscreen)(void);


#define trace_boolstring(t)        NULL
extern const char *(trace_boolstring)(BOOL t);

#define trace_procedure_name(proc) NULL
/* DOS refuses to play wrt. trace_proc as a typedef (...) */

#define trace_wimp_event(e)        NULL
extern const char *(trace_wimp_event)(int e);

#define trace_wimp_message(action) NULL
extern const char *(trace_wimp_message)(int action);

#define trace_pause()
extern void (trace_pause)(void);

#endif /* RISCOS */


#if TRACE
#define vtracef0(t, a)                          if(t) tracef0(a)
#define vtracef1(t, a, b)                       if(t) tracef1(a, b)
#define vtracef2(t, a, b, c)                    if(t) tracef2(a, b, c)
#define vtracef3(t, a, b, c, d)                 if(t) tracef3(a, b, c, d)
#define vtracef4(t, a, b, c, d, e)              if(t) tracef4(a, b, c, d, e)
#define vtracef5(t, a, b, c, d, e, f)           if(t) tracef5(a, b, c, d, e, f)
#define vtracef6(t, a, b, c, d, e, f, g)        if(t) tracef6(a, b, c, d, e, f, g)
#define vtracef7(t, a, b, c, d, e, f, g, h)     if(t) tracef7(a, b, c, d, e, f, g, h)
#define vtracef8(t, a, b, c, d, e, f, g, h, i)  if(t) tracef8(a, b, c, d, e, f, g, h, i)
#else
/* Help poor Microsoft compiler not to generate JMP nextinstruction */
#define vtracef0(t, a)
#define vtracef1(t, a, b)
#define vtracef2(t, a, b, c)
#define vtracef3(t, a, b, c, d)
#define vtracef4(t, a, b, c, d, e)
#define vtracef5(t, a, b, c, d, e, f)
#define vtracef6(t, a, b, c, d, e, f, g)
#define vtracef7(t, a, b, c, d, e, f, g, h)
#define vtracef8(t, a, b, c, d, e, f, g, h, i)
#endif

#endif  /* __trace_h */

/* end of trace.h */
