/* -> c.win */

/* Title:   win.c
 * Purpose: central control of window sytem events.
 * Author:  WRS
 * Status:  under development
 * History:
 *   24 August 87 -- started
 *   02-Mar-88 WRS use of BOOL
 *   02-Mar-88 WRS drag events put in.
 *   05-Mar-88 WRS data transfer messages routed.
 *   10-Mar-88 WRS fix for icon bar events.
 *   15-Jun-88 APT added knowledge of data transfer messages
 *   29-Jul-88 WRS stuff about help request messages put in.
 *   03-Apr-89 SKS made window allocation stuff dynamic
*/

#include "include.h"

#include "wimp.h"
#include "wimpt.h"
#include "misc.h"
#include "upcall.h"

#include "win.h"


/* ---------------------------- event claiming --------------------------- */

typedef struct win__str
{
    struct win__str *   link;
    wimp_w              w;
    win_event_handler   proc;
    void *              handle;
    void *              menuh;
}
win__str;


static win__str *win__window_list = NULL;


static BOOL
win__register(wimp_w w, win_event_handler eventproc, void *handle)
{
    win__str *wp = (win__str *) &win__window_list;
    BOOL new = TRUE;

    tracef3("win__register(%d, %s, &%p): ",
                w, trace_procedure_name((trace_proc) eventproc), handle);

    /* search list to see if it's already there */
    while((wp = wp->link) != NULL)
        if(wp->w == w)
            {
            tracef0("registering\n");
            new = FALSE; /* found previous definition */
            break;
            }

    if(new)
        wp = malloc(sizeof(win__str));

    if(wp)
        {
        tracef1("adding win__str &%p\n", wp);

        if(new)
            {
            wp->link   = win__window_list; /* add to head of list */
            win__window_list = wp;
            }

        wp->w      = w;
        wp->proc   = eventproc;
        wp->handle = handle;
        wp->menuh  = NULL;
    }
    else
        tracef0("failed to register handler\n");

    return(wp != NULL);
}


static void
win__discard(wimp_w w)
{
    win__str *pp = (win__str *) &win__window_list;
    win__str *cp;

    tracef1("win__discard(%d): ", w);

    while((cp = pp->link) != NULL)
        if(cp->w == w)
            {
            tracef1("removing win__str &%p\n", cp);
            pp->link = cp->link;
            free(cp);
            return;
            }
        else
            pp = cp;

    tracef0("failed\n");
}


extern BOOL
win_register_event_handler(wimp_w w, win_event_handler eventproc, void *handle)
{
    if(!eventproc)
        {
        win__discard(w);
        return(TRUE);
        }
    else
        return(win__register(w, eventproc, handle));
}


static win__str *
win__find(wimp_w w)
{
    win__str *wp = (win__str *) &win__window_list;

    while((wp = wp->link) != NULL)
        if(wp->w == w)
            return(wp);

    return(wp /*NULL*/);
}


/* --------------------------- event previewing -------------------------- */

typedef struct unknown_previewer
{
    struct unknown_previewer *  link;
    win_unknown_event_processor proc;
    void *                      handle;
}
unknown_previewer;


static wimp_w win__unknown = -1;
static unknown_previewer *win__unknown_previewer_list = NULL;

extern BOOL
win_add_unknown_event_processor(win_unknown_event_processor proc, void *handle)
{
    unknown_previewer *up = malloc(sizeof(unknown_previewer));

    tracef2("win_add_unknown_event_processor(%s, &%p): ",
                trace_procedure_name((trace_proc) proc), handle);

    if(up)
        {
        tracef1("adding previewer &%p\n", up);

        up->link   = win__unknown_previewer_list;
        up->proc   = proc;
        up->handle = handle;

        win__unknown_previewer_list = up;
        }
    else
        tracef0("failed\n");

    return(up != NULL);
}


extern void
win_remove_unknown_event_processor(win_unknown_event_processor proc, void *handle)
{
    unknown_previewer *pp =
   (unknown_previewer *) &win__unknown_previewer_list;
    unknown_previewer *cp;

    tracef2("win_remove_unknown_event_processor(%s, &%p): ",
                trace_procedure_name((trace_proc) proc), handle);

    while((cp = pp->link) != NULL)
        if((cp->proc == proc)  &&  (cp->handle == handle))
            {
            tracef1("removing previewer &%p\n", cp);
            pp->link = cp->link;
            free(cp);
            return;
            }
        else
            pp = cp;

    tracef0("failed\n");
}


static wimp_w win__idle = win_IDLE_OFF;

#define win__unknown_flag (('U'<<24)+('N'<<16)+('K'<<8)+'N')

extern void
win_claim_idle_events(wimp_w w)
{
    tracef1("idle events -> window %d\n",w);
    win__idle = w;
}


extern wimp_w
win_idle_event_claimer(void)
{
    return(win__idle);
}


extern void
win_claim_unknown_events(wimp_w w)
{
    win__unknown = w;
}


extern wimp_w
win_unknown_event_claimer(void)
{
    return(win__unknown);
}


/* -------- Menus. -------- */

extern void
win_setmenuh(wimp_w w, void *handle)
{
    win__str *p = win__find(w);

    if(p)
        p->menuh = handle;
}


extern void *
win_getmenuh(wimp_w w) /* 0 if not set */
{
    win__str *p = win__find(w);

    return(p ? p->menuh : p /*NULL*/);
}


/* -------- Processing Events. -------- */

extern BOOL
win_processevent(wimp_eventstr *e)
{
    wimp_msgstr *m;
    wimp_w w;
    win__str *p;
    BOOL processed;

    tracef1("win_processevent(%s)\n", trace_wimp_event(e));

    switch(e->e)
        {
        case wimp_ENULL:
            w = win__idle;
            break;


        case wimp_EUSERDRAG:
            w = win__unknown_flag;
            break;


        case wimp_EREDRAW:
        case wimp_ECLOSE:
        case wimp_EOPEN:
        case wimp_EPTRLEAVE:
        case wimp_EPTRENTER:
        case wimp_EKEY:
        case wimp_ESCROLL:
        case wimp_EGAINCARET:
        case wimp_ELOSECARET:
            w = e->data.o.w;
            break;


        case wimp_EBUT:
            w = e->data.but.m.w;
            if(w < 0)
                w = win_ICONBAR;
            break;


        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            /* Some standard messages we understand, and give to the right guy. */
            m = &e->data.msg;

            switch(m->hdr.action)
                {
                case wimp_MCLOSEDOWN:
                    tracef0("closedown message: for the moment, just do it\n");
                    exit(EXIT_SUCCESS);


                case wimp_MDATALOAD:
                case wimp_MDATASAVE:
                    tracef1("data %s message arriving.\n",
                            (m->hdr.action == wimp_MDATASAVE ? "save" : "load"));

                    assert(offsetof(wimp_msgstr, data.dataload.w) == offsetof(wimp_msgstr, data.datasave.w));
                    w = m->data.dataload.w;
                    if(w < 0)
                        {
                        tracef0("data message to the icon bar.\n");
                        w = win_ICONBARLOAD;
                        }
                    break;


                case wimp_MHELPREQUEST:
                    tracef1("help request for window %d.\n", m->data.helprequest.m.w);
                    w = m->data.helprequest.m.w;
                    if(w < 0)
                        w = win_ICONBARLOAD;
                    break;


                default:
                    tracef1("unknown message arriving: %s\n", trace_wimp_message(m));
                    w = win__unknown_flag;
                    if(w < 0)
                        w = win_ICONBARLOAD;
                    break;
                }
            break;


        default:
            w = win__unknown_flag;
            break;
        }

    if(w == win__unknown_flag)
        {
        unknown_previewer *pr =
       (unknown_previewer *) &win__unknown_previewer_list;

        while((pr = pr->link) != NULL)
            {
            tracef2("Sending unknown event to previewer (%s %p)\n",
                    trace_procedure_name((trace_proc) pr->proc), pr->handle);

            #if defined(VISDELAY_EVENT)
            visdelay_begin();
            #endif

            processed = pr->proc(e, pr->handle);

            #if defined(VISDELAY_EVENT)
            visdelay_end();
            #endif

            if(processed)
                return(processed);
            }

        tracef0("No previewer interested\n");
        w = win__unknown;
        }

    p = ((w == -1) ? NULL : win__find(w));
    if(p)
        {
        #if defined(VISDELAY_EVENT)
        visdelay_begin();
        #endif

        processed = p->proc(e, p->handle);

        #if defined(VISDELAY_EVENT)
        visdelay_end();
        #endif

        if(processed)
            return(processed);
        }

    return(FALSE);
}


#if !defined(SMALL)

/* -------- Giving away the caret. -------- */

/* Whatever window is on top (only in our task), just "open" it at its
 * current position. This should make it grab the caret, if it is interested
 * It doesn't really matter if this routine has no effect, it just means
 * that the user has to click somewhere.
*/

extern void
win_give_away_caret(void)
{
    win__str *wp = (win__str *) &win__window_list;

    while((wp = wp->link) != NULL)
        {
        wimp_wstate s;
        wimp_eventstr e;
        tracef1("get state of window %d: ", wp->w);
        (void) wimp_get_wind_state(wp->w, &s);
        tracef2("behind = %d, flags = %8.8X\n", s.o.behind, s.flags);
        if((s.o.behind == -1)  &&  ((s.flags & wimp_WOPEN) != 0))
            {
            tracef0("Opening it.\n");
            /* the top window: if it wants the caret, it will grab it. */
            /* If we just wimp_open_wind, the wimp notices that nothing
             * changes and so it ignores our call. So, fake it. This smells
             * of bodge... But, this whole area of the wimp seems a bit
             * wrong anyway
            */
#if FALSE
            wimpt_complain(wimp_open_wind(&s.o));
#else
            e.e      = wimp_EOPEN;
            e.data.o = s.o;
            wimpt_fake_event(&e);
#endif
            break;
            }
        }
}

#endif  /* SMALL */


/* -------- Termination. -------- */

static int win__active = 0;


extern void
win_activeinc(void)
{
    win__active++;
}


extern void
win_activedec(void)
{
    win__active--;
}


extern int
win_activeno(void)
{
    return(win__active);
}


/* end of win.c */
