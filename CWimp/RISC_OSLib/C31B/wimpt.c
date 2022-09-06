/* Title  > c.wimpt
 * Purpose: provides low-level Wimp functionality
 * Version: 0.1
 */

#define BOOL int
#define TRUE 1
#define FALSE 0

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "wimpt.h"
#include "trace.h"
#include "werr.h"
#include "alarm.h"
#include "event.h"
#include "msgs.h"
#include "win.h"

static int wimpt__fake_waiting = 0;
static wimp_eventstr wimpt__fake_event;
static wimp_eventstr wimpt__last_event;

os_error * wimpt_poll(wimp_emask mask, wimp_eventstr * result)
{
  if (wimpt__fake_waiting != 0) {
    *result = wimpt__fake_event;
    wimpt__fake_waiting = 0;
    wimpt__last_event = wimpt__fake_event;
    return(0);
  } else {
    os_error *r;
    int next_alarm_time;
    if (alarm_next(&next_alarm_time) != 0 && ((event_getmask() & wimp_EMNULL))!=0)
    {
      tracef0("Polling idle\n");
      tracef2("Mask = %d   %d\n", mask & ~wimp_EMNULL, event_getmask());
      r = wimp_pollidle(mask & ~wimp_EMNULL, result, next_alarm_time);
    }
    else
    {
      tracef0("Polling busy\n");
      tracef2("Mask = %d   %d\n", mask, event_getmask());
      r = wimp_poll(mask, result);
    }
    wimpt__last_event = *result;
    return(r);
  }
}

void wimpt_fake_event(wimp_eventstr * e)
{
  if (wimpt__fake_waiting == 0) {
    wimpt__fake_waiting = 1;
    wimpt__fake_event = *e;
  } else {
    tracef1("double fake event, event of type %i dropped.\n", e->e);
  }
}

wimp_eventstr *wimpt_last_event(void)
{
  return &wimpt__last_event;
}

int wimpt_last_event_was_a_key(void)
{
  return(wimpt__last_event.e == wimp_EKEY);
}

/* -------- Control of graphics environment -------- */

static int wimpt__mode = 12;
static int wimpt__dx;
static int wimpt__dy;
static int wimpt__bpp;

static int wimpt__read_screen_mode(void)
{
  int x, y;
  (void) os_byte(135, &x, &y);
  return y;
}

BOOL wimpt_checkmode(void) {
  int old = wimpt__mode;
  wimpt__mode = wimpt__read_screen_mode();
  wimpt__dx = 1 << bbc_vduvar(bbc_XEigFactor);
  wimpt__dy = 1 << bbc_vduvar(bbc_YEigFactor);
  wimpt__bpp = 1 << bbc_vduvar(bbc_Log2BPP);
  return old != wimpt__mode;
}

void wimpt_forceredraw()
{
  wimp_redrawstr r;
  r.w = (wimp_w) -1;
  r.box.x0 = 0;
  r.box.y0 = 0;
  r.box.x1 = (1 + bbc_vduvar(bbc_XWindLimit)) *
             (1 << bbc_vduvar(bbc_XEigFactor));
  r.box.y1 = (1 + bbc_vduvar(bbc_YWindLimit)) *
             (1 << bbc_vduvar(bbc_YEigFactor));
  (void) wimp_force_redraw(&r);
}

int wimpt_mode(void)
{
  return(wimpt__mode);
}

int wimpt_dx(void)
{
  return(wimpt__dx);
}

int wimpt_dy(void)
{
  return(wimpt__dy);
}

int wimpt_bpp(void)
{
  return(wimpt__bpp);
}

static wimp_t wimpt__task = 0;

static void wimpt__exit(void)
{
  wimpt_complain(wimp_taskclose(wimpt__task));
}

static char *programname = "";

char *wimpt_programname(void)
  {return programname;}
void wimpt_reporterror(os_error *e, wimp_errflags f)
  {wimp_reporterror(e,f,programname);}

os_error *wimpt_complain(os_error *e) {
  if (e != 0) {
    wimp_reporterror(e, 0, programname);
  };
  return e;
}

typedef void SignalHandler(int);
static SignalHandler *oldhandler;

static void escape_handler(int sig)
{
  sig = sig; /* avoid compiler warning */
  (void) signal(SIGINT, &escape_handler);
}

static void handler(int signal) {
  os_error er;
  er.errnum = 0;
  sprintf(
      er.errmess,
      msgs_lookup("wimpt1:%s has suffered a fatal internal error (type=%i) and must exit immediately"),
      programname,
      signal);
  wimp_reporterror(&er, 0, programname);
  exit(0);
}

static int wimpversion = 0;


void wimpt_wimpversion(int version)
{
  wimpversion = version;
}

int wimpt_init(char *progname)
{
  oldhandler = signal(SIGABRT, &handler);
  oldhandler = signal(SIGFPE, &handler);
  oldhandler = signal(SIGILL, &handler);
  oldhandler = signal(SIGINT, &escape_handler);
  oldhandler = signal(SIGSEGV, &handler);
  oldhandler = signal(SIGTERM, &handler);

  programname = progname;
  if (wimpversion == 0) wimpversion = 200;
  if (wimpt_complain(wimp_taskinit(programname, &wimpversion, &wimpt__task)) != 0) exit(0);
  wimpt_checkmode();
  atexit(wimpt__exit);
  if (!win_init()) werr(TRUE, msgs_lookup("wimpt3:Not enough memory for active windows -- increase wimpslot")); 
  return wimpversion;
}

wimp_t wimpt_task(void) {return wimpt__task;}

void wimpt_noerr(os_error *e) {
  if (e != 0) {
    os_error er;
    er.errnum = e->errnum;
    sprintf(
      er.errmess,
      msgs_lookup("wimpt2:%s has suffered a fatal internal error (%s) and must exit immediately"),
      programname,
      e->errmess);
    wimp_reporterror(&er, 0, programname);
    exit(0);
  };
}

/* end */
