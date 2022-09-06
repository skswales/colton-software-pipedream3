/* > c.event */

/* Title:   event.c
 * Purpose: system-independent central processing for window sytem events.
 * Author:  WRS
 * Status:  under development
 * History:
 *  24 August 87 -- started
 *  13-Dec-87   WRS converted to C.
 *  25-Feb-88   WRS removal of implicit menu descruction.
 *  11-Mar-88   WRS icon bar fix.
 *  29-Jun-88   WRS slight fix to prevent right-click on dbox OK button
 *              screwing up the menu tree. It has no special meaning, the menu tree
 *              goes.
 *  20-Jul-88   APT Added event_is_menu_being_recreated
 *  28-Feb-89   SKS corrected spelling of attach, made several static procs
*/

#define BOOL    int
#define TRUE    1
#define FALSE   0

#include <stdlib.h>

#include "h.trace"
#include "h.os"
#include "h.wimp"
#include "h.wimpt"
#include "h.win"
#include "h.menu"
#include "misc.h"
#include "werr.h"
#include "upcall.h"

#include "event.h"


/* -------- attaching menus. -------- */

/* An event_w is in fact a wimp_w. */

typedef struct
{
    menu             m;
    event_menu_maker maker;
    event_menu_proc  event;
    void *           handle;
}
mstr;


static BOOL
event__attach(event_w w, menu m,
              event_menu_maker menumaker, event_menu_proc eventproc, void *handle)
{
    mstr *p = win_getmenuh(w);

    if(!m  &&  !menumaker)
        {
        /* cancelling */
        if(p)
            {
            #if defined(WATCH_ALLOCS)
            tracef0("freeing menu processor: ");
            #endif
            free(p);
            win_setmenuh(w, NULL);
            }
        else
            tracef0("no menu processor/already freed\n");
        }
    else
        {
        if(!p)
            {
            p = malloc(sizeof(mstr));
            if(!p)
                return(FALSE);
            }
        p->m        = m;
        p->maker    = menumaker;
        p->event    = eventproc;
        p->handle   = handle;
        win_setmenuh(w, p);
        }

    return(TRUE);
}


extern BOOL
event_attachmenu(event_w w,
                 menu m, event_menu_proc eventproc, void *handle)
{
    return(event__attach(w, m, NULL, eventproc, handle));
}


extern BOOL
event_attachmenumaker(event_w w,
                      event_menu_maker menumaker,
                      event_menu_proc  eventproc, void *handle)
{
    return(event__attach(w, NULL, menumaker, eventproc, handle));
}


/* -------- Processing Events. -------- */

static menu event__current_menu = NULL;
static wimp_w event__menuw;
static int  event__menux;
static int  event__menuy;
static BOOL event__recreation;
static BOOL event__recreatepending = FALSE;

static void
event__createmenu(BOOL recreate)
{
    mstr *p = win_getmenuh(event__menuw);
    wimp_menuhdr *m;
    wimp_menuitem *mi;

    if(p)
        {
        event__recreation = recreate;
        event__current_menu = p->m;
        if(p->m)
            m = (wimp_menuhdr *) menu_sysmenu(p->m);
        elif(!TRACE  ||  p->maker)
            {
            event__current_menu = p->maker(p->handle);
            m = (wimp_menuhdr *) menu_sysmenu(event__current_menu);
            }
        else
            {
            werr_fatal("registered menu with no means of creation!!!\n");
            m = NULL;
            }

        if(!recreate  &&  (event__menuw == win_ICONBAR))
            {
            /* move icon bar menus up to standard position. */
            mi = (wimp_menuitem *) (m + 1);

            event__menuy = 96;

            tracef0("positioning icon bar menu.\n");
            do  {
                event__menuy += m->height + m->gap;
                }
            while(!((mi++)->flags & wimp_MLAST));
            }

        (void) event_create_menu((wimp_menustr *) m, event__menux, event__menuy);
        }
    else
        tracef0("no registered menu\n");
}


static BOOL
event__defaultprocess(wimp_eventstr *e)
{
    switch(e->e)
        {
        case wimp_ENULL:
            /* machine idle: say so */
            tracef0("unclaimed idle event\n");
            return(TRUE);


        case wimp_EOPEN:
            tracef0("unclaimed open request - doing open\n");
            wimpt_complain(wimp_open_wind(&e->data.o));
            break;


        case wimp_ECLOSE:
            tracef0("unclaimed close request - doing close\n");
            wimpt_complain(wimp_close_wind(e->data.o.w));
            break;


        case wimp_EREDRAW:
            {
            wimp_redrawstr r;
            BOOL more;
            tracef0("unclaimed redraw request - doing redraw\n");
            r.w = e->data.o.w;
            if(!wimpt_complain(wimp_redraw_wind(&r, &more)))
                while(more)
                    if(wimpt_complain(wimp_get_rectangle(&r, &more)))
                        more = FALSE;
            }
            break;


        default:
            tracef1("unclaimed event %s\n", trace_wimp_event(e));
            break;
        }

    return(FALSE);
}


/* process event: returns true if idle */

static BOOL
event__process(wimp_eventstr *e)
{
    wimp_msgstr *m = &e->data.msg;
    wimp_mousestr ms;
    char hit[20];
    int i;
    BOOL submenu_fake_hit;

    tracef1("event__process(%s)\n", trace_wimp_event(e));

    /* Look for submenu requests, and if found turn them into menu hits. */
    /* People wishing to respond can pick up the original from wimpt. */
    if((e->e == wimp_ESEND)  &&  (m->hdr.action == wimp_MMENUWARN))
        {
        /* A hit on the submenu pointer may be interpreted as slightly different
         * from actually clicking on the menu entry with the arrow. The most
         * important example of this is the standard "save" menu item, where clicking
         * on "Save" causes a save immediately while following the arrow gets
         * the standard save dbox up.
        */
        i = 0;
        tracef0("hit on submenu => pointer, fake menu hit\n");
        e->e = wimp_EMENU;
        do  {
            e->data.menu[i] = m->data.menuwarning.menu[i];
            }
        while(e->data.menu[i++] != -1);

        submenu_fake_hit = TRUE;
        }
    else
        submenu_fake_hit = FALSE;


    /* Look for events to do with menus */
    if((e->e == wimp_EBUT)  &&  (wimp_BMID & e->data.but.m.bbits))
        {
        /* Menu creation */
        mstr *p;
        wimp_w w = e->data.but.m.w;

        if(w <= -1)
            w = win_ICONBAR;

        p = win_getmenuh(w);

        if(p)
            {
            event__menuw = w;
            event__menux = e->data.but.m.x - 64;
            event__menuy = e->data.but.m.y;
            event__createmenu(FALSE);
            tracef0("menu created\n");
            return(FALSE);
            }
        else
            tracef0("menu creation not wanted\n");
        }
    elif((e->e == wimp_EMENU)  &&  event__current_menu)
        {
        /* Menu hit */
        mstr *p = win_getmenuh(event__menuw);

        tracef0("menu hit ");

        if(p)
            {
            if(!submenu_fake_hit)
                {
                wimpt_safe(wimp_get_point_info(&ms));
                event__recreatepending = ((ms.bbits & wimp_BRIGHT) == wimp_BRIGHT);
                }
            else
                event__recreatepending = FALSE;

            /* form array of menu hits ending in 0 */
            i = 0;
            do  {
                hit[i] = e->data.menu[i] + 1;
                tracef1("[%d]", hit[i]);
                }
            while(e->data.menu[i++] != -1);

            tracef1(", ADJUST = %s\n", trace_boolstring(event__recreatepending));

            if(!p->event(p->handle, hit, submenu_fake_hit)  &&  submenu_fake_hit)
                {
                /* handle unprocessed submenu open events */
                wimp_menustr *submenu;
                int x, y;
                event_read_submenudata((event_submenu *) &submenu, &x, &y);
                wimpt_complain(wimp_create_submenu(submenu, x, y));
                }

            if(event__recreatepending)
                {
                /* Twas an ADJ-hit on a menu item.
                 * The menu should be recreated.
                */
                tracef0("menu hit caused by ADJUST - recreating menu\n");
                event__createmenu(TRUE);
                }
            #if TRACE
            elif(submenu_fake_hit)
                tracef0("menu hit was faked\n");
            else
                tracef0("menu hit caused by SELECT - let tree collapse\n");
            #endif

            return(FALSE);
            }
        else
            tracef0("- no registered handler\n");
        }

    /* now process the event */
      if(win_processevent(e))
        return(FALSE); /* all is well, it was claimed */
    else
        return(event__defaultprocess(e));
}

    
extern void
event_process(void)
{
    wimp_eventstr e;

    tracef0("event_process()\n");

    if(!win_activeno())
        exit(EXIT_SUCCESS);     /* stop program - no-one alive */

    /* don't normally accept null events */
    wimpt_poll(wimp_EMNULL, &e);

    (void) event__process(&e);
}


/* for dbox to zap pendingrecreate */

extern BOOL
event_create_menu(wimp_menustr *m, int x, int y)
{
    event__recreatepending = FALSE;

    return(!wimpt_complain(wimp_create_menu(m, x, y)));
}


extern void
event_clear_current_menu(void)
{
    (void) event_create_menu((wimp_menustr *) -1, 0, 0);
}


extern BOOL
event_is_menu_being_recreated(void)
{
    return(event__recreation);
}


extern BOOL
event_is_menu_recreate_pending(void)
{
    return(event__recreatepending);
}


extern void
event_read_submenudata(event_submenu *smp, int *xp, int *yp)
{
    wimp_msgstr *m = &wimpt_last_event()->data.msg;

    if(smp)
        * ((wimp_menustrp *) smp) = m->data.menuwarning.submenu.m;

    if(xp)
        *xp = m->data.menuwarning.x;

    if(yp)
        *yp = m->data.menuwarning.y;
}


/* end of event.c */
