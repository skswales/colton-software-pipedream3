/* -> c.wimpt */

/* Title:   wimpt.c
 * Purpose: provides low-level Wimp functionality
 * Version: 0.4
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

/* Change List:
 *
 * APT: 01-12-88:  Escape handler must be set to ignore escapes
 * SKS: 25-Jan-89: Made exception handlers spool backtrace to cwimp$dumpfile
 *                 if this variable exists (cf. m2$dump under PainArse)
 * SKS: 16-Feb-89: Made fatal error handler do backtrace to cwimp$dumpfile
 *                 Made fatal error and exception handlers freopen stderr
 *                 for cleaner termination. static'ed some functions
 *                 Removed wimpt__setmouserect
 * SKS: 07-Mar-89: Removed wimpt_setprogramname; const'ed some functions
 * SKS: 14-Mar-89: Added initial stderr redirection: notes that it'd be
 *                 nice to trap stderr output & reporterror it ...
 * SKS: 21-Mar-89: More trapping for stderr cases: optional backtrace on exit
 * SKS: 03-Apr-89: Better exit()s, shorter code/strings
 * SKS: 25-Apr-89: Modified escape handler to allow termination (debug only)
 * SKS: 28-Sep-89: Added default stack overflow handler
*/

#define BOOL int
#define TRUE 1
#define FALSE 0

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

#include "trace.h"

#include "os.h"
#include "bbc.h"
#include "akbd.h"
#include "wimp.h"
#include "wimpt.h"
#include "misc.h"
#include "win.h"


/* ----------------------------------------------------------------------- */

static int              wimpt__fake_waiting = 0;
static wimp_eventstr    wimpt__fake_event;
static wimp_eventstr    wimpt__last_event;
static wimpt_atentry_t  wimpt__atentryproc = NULL;
static wimpt_atexit_t   wimpt__atexitproc  = NULL;


extern wimpt_atentry_t
wimpt_atentry(wimpt_atentry_t pfnNewProc)
{
    wimpt_atentry_t pfnOldProc = wimpt__atentryproc;
    wimpt__atentryproc = pfnNewProc;
    return(pfnOldProc);
}


extern wimpt_atexit_t
wimpt_atexit(wimpt_atexit_t pfnNewProc)
{
    wimpt_atexit_t pfnOldProc = wimpt__atexitproc;
    wimpt__atexitproc = pfnNewProc;
    return(pfnOldProc);
}


extern os_error *
wimpt_poll(wimp_emask mask, wimp_eventstr *result /*out*/)
{
    os_error *err;

    #if defined(TRACE_POLL)
    trace_on();
    #endif

    tracef3("\n[wimpt_poll(&%X, &%p): fake_waiting = %s]\n", mask, result, trace_boolstring(wimpt__fake_waiting));

    if(wimpt__fake_waiting)
        {
        *result = wimpt__fake_event;
        wimpt__fake_waiting = 0;
        tracef1("[wimpt_poll returns faked event %s]\n", trace_wimp_event(result));
        }
    else
        {
        if(wimpt__atexitproc)           /* pragmatic fix - make general in due course */
            wimpt__atexitproc();

        /* programs don't generally want these */
        mask |= wimp_EMPTRLEAVE | wimp_EMPTRENTER;

        do  {
            if(win_idle_event_claimer() != win_IDLE_OFF)
                {
                /* ensure we get null events */
                mask &= ~wimp_EMNULL;
                tracef1("[wimpt_poll must get idle requests: mask := %X]\n", mask);
                }
            tracef2("[calling wimp_poll(&%X, &%p)]\n", mask, result);
            err = wimp_poll(mask, result);
            if(err)
                tracef0("--- ERROR returned from wimp_poll()!!! (Ta Neil)\n");
            wimpt_complain(err);
            }
        while(err);

        if(wimpt__atentryproc)          /* pragmatic fix - make general in due course */
            wimpt__atentryproc();

        tracef1("[wimpt_poll returns real event %s]\n", trace_wimp_event(result));
        }

    #if defined(TRACE_POLL)
    trace_off();
    #endif

    wimpt__last_event = *result;

    return(NULL);
}


extern void
wimpt_fake_event(const wimp_eventstr *e)
{
    tracef1("[wimpt_fake_event(%s): ", trace_wimp_event(e));

    if(wimpt__fake_waiting == 0)
        {
        tracef0("fake event stored ok]\n");
        wimpt__fake_waiting = 1;
        wimpt__fake_event   = *e; /* copy event to buffer */
        }
    #if TRACE
    else
        {
        trace_on();
        tracef1("double fake event - this event dropped because %s still buffered]\n",
                trace_wimp_xevent(wimpt__fake_event.e, &wimpt__fake_event.data));
        trace_pause();
        trace_off();
        }
    #endif
}


extern wimp_eventstr *
wimpt_last_event(void)
{
    tracef1("[wimpt_last_event() returns %s]\n", trace_wimp_xevent(wimpt__fake_event.e, &wimpt__fake_event.data));
    return(&wimpt__last_event);
}


/* -------------------- Control of graphics environment ------------------ */

/* force redraw of entire screen */

extern void
wimpt_forceredraw(void)
{
    wimp_redrawstr r;

    r.w       = (wimp_w) -1;
    r.box.x0  = 0;
    r.box.y0  = 0;
    r.box.x1  = (1 + bbc_vduvar(bbc_XWindLimit)) *
                (1 << bbc_vduvar(bbc_XEigFactor));
    r.box.y1  = (1 + bbc_vduvar(bbc_YWindLimit)) *
                (1 << bbc_vduvar(bbc_YEigFactor));

    wimpt_safe(wimp_force_redraw(&r));
}


/* ---------- Task initialisation, error handling, finalisation ---------- */

extern os_error *
wimpt_complain(os_error *e)
{
    if(e)
        (void) wimp_reporterror(e, 0, wimpt_programname());

    return(e);
}


static const char *dumpfile = NULL;

static BOOL stderr_opened = FALSE;

static void
wimpt__freopen_stderr(void)
{
    if(!stderr_opened  &&  dumpfile)
        {
        freopen(dumpfile, "w", stderr);
        stderr_opened = TRUE;       /* even if reopen failed */
        setvbuf(stderr, NULL, _IONBF, 0);
        }
}


static int abort_caused_by_me = 0;


extern void
wimpt_abort(os_error *e)
{
    abort_caused_by_me++;

    wimpt_noerr(e);
}


extern void
wimpt_noerr(os_error *e)
{
    if(e)
        {
        os_error err;

        err.errnum = e->errnum;

        sprintf(err.errmess,
                "%s has suffered a fatal internal error (%s)" \
                " and must exit immediately",
                wimpt_programname(),
                e->errmess);

        if(!abort_caused_by_me)
            wimpt_complain(&err);

        if(dumpfile)
            {
            wimpt__freopen_stderr();

            fputs(err.errmess, stderr);
            fprintf(stderr, ": &%8.8X\n", err.errnum);

            abort_caused_by_me++;

            abort();    /* get a precious stack backtrace! */
            }

        exit(EXIT_FAILURE);
        }
}


/* Wimp programs must ignore escape events: they will not happen normally,
 * but may happen while (eg) printing is happening; in this case, the
 * printer driver will handle the problem, and the wimp program simply
 * deals with the returned error.
*/

static void
wimpt__escape_handler(int sig)
{
    #if defined(NO_SURRENDER)
    if(akbd_pollctl())
        {
        if(akbd_pollsh())
            trace_on();
        else
            raise(SIGTERM);         /* goodbye cruel world */
        }
    #endif

    /* reinstall ourselves, as SIG_DFL has been restored by the system
     * as defined by the (dumb) ANSI spec!
     */
    (void) signal(sig, &wimpt__escape_handler);
}


#pragma -s1

static jmp_buf safepoint;

static void
wimpt__stacko_handler(int sig)
{
    longjmp(safepoint, sig);
}

#pragma -s0


static void
wimpt__signal_handler(int sig)
{
    os_error err;
    char causebuffer[32];
    const char *cause;

    switch(sig)
        {
        case SIGABRT:
            cause = "Abnormal termination";
            break;

        case SIGFPE:
            cause = "Arithmetic exception";
            break;

        case SIGILL:
            cause = "Illegal instruction";
            break;

        case SIGSEGV:
            cause = "Address exception";
            break;

        case SIGTERM:
            cause = "Termination request received";
            break;

        case SIGSTAK:
            cause = "Stack overflow";
            break;

        default:
            sprintf(causebuffer, "type=%d", sig);
            cause = causebuffer;
            break;
        }

    err.errnum = sig;

    sprintf(err.errmess, 
            "%s has suffered a fatal internal error (%s)" \
            " and must exit immediately",
            wimpt_programname(),
            cause);

    if(!abort_caused_by_me)         /* first level fault */
        wimpt_complain(&err);

    if(dumpfile)
        {
        if(!abort_caused_by_me)
            wimpt__freopen_stderr();

        if(abort_caused_by_me != 1) /* abort() comes here from below */
            {
            fputs(err.errmess, stderr);
            fputc('\n', stderr);
            }

        abort_caused_by_me++;       /* append subsequent faults in backtrace */

        if(sig != SIGSTAK)
            raise(sig);             /* get a precious stack backtrace! */
        }

    exit(EXIT_FAILURE);
}


static const char *wimpt__programname;

extern const char *
wimpt_programname(void)
{
    return(wimpt__programname);
}


static wimp_t wimpt__task;

extern wimp_t
wimpt_task(void)
{
    return(wimpt__task);
}


static int backtrace_on_exit = 0;

extern void
wimpt_backtrace_on_exit(BOOL t)
{
    if(t)
        {
        wimpt__freopen_stderr();
        backtrace_on_exit++;
        }
    elif(backtrace_on_exit)
        backtrace_on_exit--;
}


static void
wimpt__exit(void)
{
    if(backtrace_on_exit  &&  !abort_caused_by_me)
        /* no longer on atexit() processing list so won't recurse */
        abort();

    (void) wimp_taskclose(wimpt_task());
}


extern void
wimpt_init(const char *progname)
{
    char varname[256];
    int sig;

    wimpt__programname = progname;

    strcat(strcpy(varname, wimpt_programname()), "$DumpFile");

    dumpfile = getenv(varname);

    (void) wimp_taskinit(wimpt_programname(), &wimpt__task);

    if((sig = setjmp(safepoint)) != 0)
        wimpt__signal_handler(sig);

#if 0 /* <<< TODO - reinstate */
    (void) signal(SIGABRT,  &wimpt__signal_handler);
    (void) signal(SIGFPE,   &wimpt__signal_handler);
    (void) signal(SIGILL,   &wimpt__signal_handler);
    (void) signal(SIGINT,   &wimpt__escape_handler);
    (void) signal(SIGSEGV,  &wimpt__signal_handler);
    (void) signal(SIGTERM,  &wimpt__signal_handler);
    (void) signal(SIGSTAK,  &wimpt__stacko_handler);
#endif

    atexit(wimpt__exit);
}


/* ------------------------ standard wimp calls --------------------------- */

extern void
wimpt_send_message(wimp_etype code, wimp_msgstr *msg, wimp_t dest)
{
    wimpt_safe(wimp_sendmessage(code, msg, dest));
}


extern void
wimpt_send_wmessage(wimp_etype code, wimp_msgstr *msg, wimp_w w, wimp_i i)
{
    wimpt_safe(wimp_sendwmessage(code, msg, w, i));
}


extern void
wimpt_ack_message(wimp_msgstr *msg)
{
    wimp_t sender_id = msg->hdr.task;

    tracef0("acknowledging message: ");

    msg->hdr.your_ref = msg->hdr.my_ref;

    wimpt_send_message(wimp_EACK, msg, sender_id);
}


/* end of wimpt.c */
