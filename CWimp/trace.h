/* > h.trace */

/* Title:   trace.h
 * Purpose: centralised control for trace/debug output
 * Version: 0.1 WRS created
 *          0.2 SKS allow for trace setting without modifying this file
 *          0.3 SKS better definition of functions, use of const char *
 *          0.4 SKS added more functions
*/

/***************************************************************************
* This source file was written by Acorn Computers Limited. It is part of   *
* the "cwimp" library for writing applications in C for RISC OS. It may be *
* used freely in the creation of programs for Archimedes. It should be     *
* used with Acorn's C Compiler Release 2 or later.                         *
*                                                                          *
* No support can be given to programmers using this code and, while we     *
* believe that it is correct, no correspondence can be entered into        *
* concerning behaviour or bugs.                                            *
*                                                                          *
* Upgrades of this code may or may not appear, and while every effort will *
* be made to keep such upgrades upwards compatible, no guarantees can be   *
* given.                                                                   *
***************************************************************************/


#ifndef __wimplib__trace_h
#define __wimplib__trace_h


#ifndef TRACE
#define TRACE 0
#endif

/* This flag says if tracing is compiled in.
 * It should be used in conditional compilation statements
 * around tracing code that uses tracef(...).
 * Use cc foo.c -dTRACE to compile in trace information.
*/


#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


typedef void (*trace_proc)(void);


#if TRACE

#if defined(PEDANTIC)
#pragma -v1
#endif
/* check parameters for matching format */
extern int  tracef(const char *format, ...);
/* must only appear #if TRACE: see #else case below. */

extern void trace_system(const char *format, ...);
#if defined(PEDANTIC)
#pragma -v0
#endif


/* These forms can occur outside conditional compilation clauses:
 * they will produce no code if TRACE is not set.
*/

#define tracef0(a)                         tracef(a)
#define tracef1(a, b)                      tracef(a, b)
#define tracef2(a, b, c)                   tracef(a, b, c)
#define tracef3(a, b, c, d)                tracef(a, b, c, d)
#define tracef4(a, b, c, d, e)             tracef(a, b, c, d, e)
#define tracef5(a, b, c, d, e, f)          tracef(a, b, c, d, e, f)
#define tracef6(a, b, c, d, e, f, g)       tracef(a, b, c, d, e, f, g)
#define tracef7(a, b, c, d, e, f, g, h)    tracef(a, b, c, d, e, f, g, h)
#define tracef8(a, b, c, d, e, f, g, h, i) tracef(a, b, c, d, e, f, g, h, i)

extern void trace_on(void);
extern void trace_off(void);
extern BOOL trace_is_on(void);
extern void trace_clearscreen(void);
extern void trace_pause(void);
extern void trace_set_prefix(const char *prefix);

extern const char *trace_boolstring(BOOL t);
extern const char *trace_string(const char *str);
extern const char *trace_procedure_name(trace_proc proc);
extern const char *trace_wimp_eventcode(const int etype);
extern const char *trace_wimp_xevent(const int etype, const void *ep);
extern const char *trace_wimp_action(const int action);
extern const char *trace_wimp_xmessage(const void *msgp, BOOL send);

#else

/* No-trace versions */
/* tracef itself cannot be done as a macro. */

#define tracef0(a)
#define tracef1(a, b)
#define tracef2(a, b, c)
#define tracef3(a, b, c, d)
#define tracef4(a, b, c, d, e)
#define tracef5(a, b, c, d, e, f)
#define tracef6(a, b, c, d, e, f, g)
#define tracef7(a, b, c, d, e, f, g)
#define tracef8(a, b, c, d, e, f, g)


#define trace_on()
extern void (trace_on)(void);

#define trace_off()
extern void (trace_off)(void);

#define trace_is_on()              FALSE
extern BOOL (trace_is_on)(void);

#define trace_clearscreen()
extern void (trace_clearscreen)(void);

#define trace_pause()
extern void (trace_pause)(void);

#define trace_set_prefix(prefix)
extern void (trace_set_prefix)(const char *prefix);


#define trace_boolstring(t)         NULL
extern const char *(trace_boolstring)(BOOL t);

#define trace_string(str)           NULL
extern const char *(trace_string)(const char *str);

#define trace_procedure_name(proc)  NULL
extern const char *(trace_procedure_name)(trace_proc proc);

#define trace_wimp_eventcode(e)         NULL
extern const char *(trace_wimp_eventcode)(const int etype);

#define trace_wimp_xevent(e, ep)        NULL
extern const char *(trace_wimp_xevent)(const int etype, const void *ep);

#define trace_wimp_action(action)   NULL
extern const char *(trace_wimp_action)(const int action);

#define trace_wimp_xmessage(msgp, send) NULL
extern const char *(trace_wimp_xmessage)(const void *msgp, BOOL send);


#endif  /* TRACE */


#define trace_wimp_event(eventp)    trace_wimp_xevent(eventp->e, &eventp->data)
#define trace_wimp_message(msgp)    trace_wimp_xmessage(msgp, FALSE)


#endif  /* __wimplib__trace_h */


/* end of trace.h */
