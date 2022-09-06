/* Title  : -> h.wimpt
 * Purpose: provides low-level wimp functionality
 * Version: 0.3
 */

#ifndef __wimpt_h
#define __wimpt_h

#ifndef __wimp_h
#include "wimp.h"
#endif

extern os_error * wimpt_poll(wimp_emask mask, wimp_eventstr *result /*out*/);
/* Identical to wimp_poll, except that the extra facilities below
 * are provided. All other parts of the program should use wimpt_poll
 * rather than wimp_poll.
*/

extern void wimpt_fake_event(const wimp_eventstr *);
/* The wimp_eventstr is saved away, and will be yielded by the next call to
 * wimpt_poll, rather than calling wimp_poll. If the next call to Poll will not
 * allow it because of the mask then the fake is discarded. Multiple calls
 * without wimpt_poll calls will be ignored.
*/

extern wimp_eventstr *wimpt_last_event(void);
/* The last value yielded by wimpt_poll. */


/* obtain callback just after/before calling wimp_poll */
typedef void (*wimpt_atentry_t)(void);
typedef void (*wimpt_atexit_t) (void);

extern wimpt_atentry_t wimpt_atentry(wimpt_atentry_t pfnNewProc);
extern wimpt_atexit_t  wimpt_atexit (wimpt_atexit_t  pfnNewProc);


extern void wimpt_abort(os_error *e);
extern void wimpt_noerr(os_error *e);
/* If e != 0 then wimp_reporterror(e); stop. This is useful for wrapping up
 * calls to system calls which are not expected to fail. The error message
 * has "Unexpected system error:" added. This should only arise from logical
 * errors in the program.
*/

extern os_error *wimpt_complain(os_error *e);
/* If e != 0 then wimp_reporterror(e); return e. This is useful for wrapping
 * up calls to system functions.
*/

#ifdef PARANOID
#define wimpt_safe(a) wimpt_complain(a)
#else
#define wimpt_safe(a) a
#endif
extern os_error *(wimpt_safe)(os_error *e);


extern void wimpt_init(const char *programname);
/* sets up the screen and calls wimp_inittask. Program closedown code
is set up to tidy things up on exit. The program name will appear in
error messages and on the task control panel. */

extern const char *wimpt_programname(void);
/* Yields the program name */

extern void wimpt_reporterror(os_error*, wimp_errflags);
/* Like wimp_reporterror, but fills in the program name. */

extern wimp_t wimpt_task(void);
/* Gives our task handle. */

extern void wimpt_forceredraw(void);
/* Invalidate the whole screen. */

extern void wimpt_backtrace_on_exit(BOOL t);
/* raise/lower backtrace flag */


extern void wimpt_send_message(wimp_etype code, wimp_msgstr *msg, wimp_t dest);

extern void wimpt_send_wmessage(wimp_etype code, wimp_msgstr *msg, wimp_w w, wimp_i i);

extern void wimpt_ack_message(wimp_msgstr *msg);


#endif  /* __wimpt_h */


/* end of wimpt.h */
