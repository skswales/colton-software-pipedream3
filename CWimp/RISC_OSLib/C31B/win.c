/* Title: -> c.win
 * Purpose: central control of window sytem events.
 * Status: under development
 */

#define BOOL int
#define TRUE 1
#define FALSE 0

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "h.trace"
#include "h.os"
#include "h.wimp"
#include "h.werr"
#include "h.wimpt"
#include "h.event"
#include "h.msgs"

#include "h.win"

/* -------- Keeping Track of All Windows. -------- */

/* At the moment, a simple linear array of all windows. */

typedef struct {
  wimp_w w;
  win_event_handler proc;
  void *handle;
  void *menuh;
} win__str;

#define MAXWINDOWS (256 + 1)
#define DUD (-1)

static win__str *win__allwindows; /*[MAXWINDOWS]*/
static int win__lim = 0; /* first unused index of win__allwindows */

static void win__register(wimp_w w, win_event_handler proc, void *handle)
{
  int i;
  for (i=0;; i++) {
    if (i == MAXWINDOWS) {
      werr(1, msgs_lookup("win1:Too many windows.\n"));
    };
    if (i == win__lim) {win__lim++; break;};
    if (win__allwindows[i].w == w) {break;}; /* found prev definition */
    if (win__allwindows[i].w == DUD) {break;}; /* found hole */
  }
  win__allwindows[i].w = w;
  win__allwindows[i].proc = proc;
  win__allwindows[i].handle = handle;
  win__allwindows[i].menuh = 0;
}

static void win__discard(wimp_w w)
{
  int i;
  for (i=0; i<win__lim; i++) {
    if (win__allwindows[i].w == w) {
      win__allwindows[i].w = DUD;
      break;
    };
  }
  while (win__lim > 0 && win__allwindows[win__lim-1].w == DUD) {
    /* decrease the limit if you can. */
    win__lim--;
  };
}

static win__str *win__find(wimp_w w)
{
  int i;
  for (i=0; i<win__lim; i++) {
    if (win__allwindows[i].w == w) {
      return(&win__allwindows[i]);
    };
  };
  return(0);
}

/* -------- Claiming Events. -------- */

void win_register_event_handler(
  wimp_w w, win_event_handler proc, void *handle)
{
  if (proc == 0) {
    win__discard(w);
  } else {
    win__register(w, proc, handle);
  };
}

typedef struct unknown_previewer
{
 struct unknown_previewer *link ;
 win_unknown_event_processor proc ;
 void *handle ;
} unknown_previewer ;

static wimp_w win__idle = DUD;
#define win__unknown_flag (('U'<<24)+('N'<<16)+('K'<<8)+'N')

static wimp_w win__unknown = DUD;
static unknown_previewer *win__unknown_previewer_list = 0 ;

void win_claim_idle_events(wimp_w w)
{
  tracef1("idle events -> %d\n",w) ;
  win__idle = w; 
#if FALSE
  event_setmask(event_getmask() & ~wimp_EMNULL);
#endif
}

wimp_w win_idle_event_claimer(void)
{
  return(win__idle);
}

void win_claim_unknown_events(wimp_w w)
{
  win__unknown = w;
}

wimp_w win_unknown_event_claimer(void)
{
  return win__unknown;
}

void win_add_unknown_event_processor(win_unknown_event_processor p,
                                     void *h)
{
 unknown_previewer *block = malloc(sizeof(unknown_previewer)) ;
 if (block != 0)
 {
  block->link = win__unknown_previewer_list ;
  block->proc = p ;
  block->handle = h ;
  win__unknown_previewer_list = block ;
 }
}

void win_remove_unknown_event_processor(win_unknown_event_processor p,
                                        void *h)
{
 unknown_previewer *block ;
 for (block = (unknown_previewer *) &win__unknown_previewer_list ;
      block != 0 && (block->link->proc != p || block->link->handle != h) ;
      block = block->link) ;
 if (block != 0)
 {
  unknown_previewer *b2 = block->link ;
  block->link = b2->link ;
  free(b2) ;
 }
/*
 else tracef2("win_remove_unknown_event_processor failed: %x, %x\n",
              (int) p,(int) h) ;
*/
}

/* -------- Menus. -------- */

void win_setmenuh(wimp_w w, void *handle)
{
  win__str *p = win__find(w);
  if (p != 0) {p->menuh = handle;};
}

void *win_getmenuh(wimp_w w) /* 0 if not set */
{
  win__str *p = win__find(w);
  return(p==0 ? 0 : p->menuh);
}

/* -------- Processing Events. -------- */

BOOL win_processevent(wimp_eventstr *e)
{
  wimp_w w;
  win__str *p;
/*  tracef1("win_processevent %i.\n", e->e);  */
  switch (e->e) {
  case wimp_ENULL:
    w = win__idle;
    break;
  case wimp_EUSERDRAG:
    w = win__unknown_flag ;
    break;
  case wimp_EREDRAW: case wimp_ECLOSE: case wimp_EOPEN:
  case wimp_EPTRLEAVE: case wimp_EPTRENTER: case wimp_EKEY:
  case wimp_ESCROLL:
    w = e->data.o.w;
    break;
  case wimp_EBUT:
    w = e->data.but.m.w;
    if (w <= (wimp_w) -1) w = win_ICONBAR;
    break;
  case wimp_ESEND:
  case wimp_ESENDWANTACK:
    /* Some standard messages we understand, and give to the right guy. */
    switch (e->data.msg.hdr.action) {
    case wimp_MCLOSEDOWN:
      tracef0("closedown message: for the moment, just do it.\n");
      exit(0);
    case wimp_MDATALOAD:
    case wimp_MDATASAVE:
      tracef1("data %s message arriving.\n",
             (int) (e->data.msg.hdr.action==wimp_MDATASAVE ? "save" : "load"));

      /* Note that we're assuming the window handle's the same position in
         both messages */
      if (e->data.msg.data.dataload.w < 0)
      {
        tracef0("data message to the icon bar.\n");
        w = win_ICONBARLOAD ;
      } else {
        w = e->data.msg.data.dataload.w;
      };
      break;
    case wimp_MHELPREQUEST:
      tracef1("help request for window %i.\n", e->data.msg.data.helprequest.m.w);
      w = e->data.msg.data.helprequest.m.w;
      if (w < 0) w = win_ICONBARLOAD;
      break;
    default:
      tracef1("unknown message type %i arriving.\n", e->data.msg.hdr.action);
      w = win__unknown_flag;
      if (w < 0) w = win_ICONBARLOAD;
    };
    break;
  default:
    w = win__unknown_flag;
  };

  if (w==win__unknown_flag || win__find(w) == 0)
  {
   unknown_previewer *pr ;
   for (pr = win__unknown_previewer_list; pr != 0; pr = pr->link)
   {
/*
    tracef2("Unknown event to previewer %x %x\n",
            (int)pr->proc,(int)pr->handle) ;
*/
    if (pr->proc(e, pr->handle)) return TRUE ;
   }
/*
   tracef0("No previewer interested\n") ;
*/
   w = win__unknown ;
  }

  p = (w == DUD ? 0 : win__find(w));
  if (p != 0) {
    p->proc(e, p->handle);
    return TRUE;
  } else {
    return FALSE;
  };
}

/* -------- Termination. -------- */

static int win__active = 0;

void win_activeinc(void)
{
  win__active++;
}

void win_activedec(void)
{
  win__active--;
}

int win_activeno(void)
{
  return win__active;
}

/* -------- Giving away the caret. -------- */

void win_give_away_caret(void)
/* Whatever window is on top, just "open" it at its current position.
This should make it grab the caret, if it is interested. It doesn't really
matter if this routine has no effect, it just means that the user has to
click somewhere. */
{
  int i;
  for (i=0; i<win__lim; i++) {
    if (win__allwindows[i].w != DUD) { /* found a window */
      wimp_wstate s;
      wimp_eventstr e;
      tracef1("get state of window %i.", win__allwindows[i].w);
      (void) wimp_get_wind_state(win__allwindows[i].w, &s);
      tracef2("behind=%i flags=%i.\n", s.o.behind, s.flags);
      if (s.o.behind == DUD && (s.flags & wimp_WOPEN) != 0) {
        /* w is the top window */
        /* if it wants the caret, it will grab it. */
        tracef0("Opening it.\n");
        /* If we just wimp_open_wind, the wimp notices that nothing
        changes and so it ignores our call. So, fake it. This smells
        of bodge... But, this whole area of the wimp seems a bit wrong
        anyway. */
        /* (void) wimp_open_wind(&s.o); */
        e.e = wimp_EOPEN;
        e.data.o = s.o;
        wimpt_fake_event(&e);
        break;
      };
    };
  };
}

/* ----------- reading event handler for window ----------- */
/* IDJ 6-Feb-90 - useful for intercepting events to txt windows */

BOOL win_read_eventhandler(wimp_w w, win_event_handler *p, void **handle)
{
  int i;
  for (i=0; i<win__lim; i++) {
    if (win__allwindows[i].w == w) {
      *p = win__allwindows[i].proc;
      *handle = win__allwindows[i].handle;
      return(TRUE);
    };
  };
  return(FALSE);
}

/* ----------- setting a window title ------------ */

void win_settitle(wimp_w w, char *newtitle)
{
  wimp_redrawstr r;
  wimp_winfo *winfo;

  if((winfo = malloc(sizeof(wimp_wind) + 200*sizeof(wimp_icon))) == 0)
    werr(TRUE, msgs_lookup("win2:Not enough memory to change window title -- increase wimpslot"));

tracef1("New title is %s\n", (int)newtitle);
  /* --- get the window's details --- */
    winfo->w = w;
    wimpt_noerr(wimp_get_wind_info(winfo));

  /* --- put the new title string in the title icon's buffer --- */
tracef1("Lib buffer is at %d\n", (int)winfo->info.title.indirecttext.buffer);
    strcpy(winfo->info.title.indirecttext.buffer, newtitle);

  /* --- invalidate the title bar in absolute coords --- */
    r.w = (wimp_w) -1;    /* absolute screen coords */
    r.box = winfo->info.box;
    r.box.y1 += 36;            /* yuk - tweaky */
    r.box.y0 = r.box.y1 - 36;
    wimpt_noerr(wimp_force_redraw(&r));

  /* --- free space used to window info --- */
    free(winfo);
}

BOOL win_init(void)
{
    if (win__allwindows == 0) 
    {
        return((win__allwindows = malloc(MAXWINDOWS*sizeof(win__str)))!=0);
    }
    else
        return TRUE;
}

/* end */
