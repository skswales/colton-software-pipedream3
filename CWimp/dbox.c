/* > c.dbox */

/* Title:   dbox.c
 * Purpose: System-independent dialog boxes.
 *          RISC OS implementation.
 * Author:  WRS
 * History:
 *   28-Sep-87 -- started
 *   13-Dec-87 WRS: conversion to C started.
 *   22-Jan-88 SKS: changed resource usage to new system
 *   22-Jan-88 SKS: changed o.x0,x1,y0,y1 after WRS changes to h.wimp
 *   24-Feb-88 WRS: conversion to new trace facilities.
 *                   acceptance of lose/gain caret messages.
 *   10-Mar-88 WRS: bug fix re key events in fillin.
 *   09-Jun-88 APT: made poll only mask out nulls if nobody wants them.
 *   13-Jun-88 WRS: increase read-in buffer size
 *                           add dbox_showstatic
 *   20-Jun-88 WRS: fix in dbox_fillin re redraw events.
 *   21-Nov-88 WRS: by popular request, let keystrokes through
 *     as though harmless. Not true, but the wimp menu system has
 *     the same bug so...
 *   22-Nov-88 WRS: reverse last change. Caused problems in Edit
 *     e.g. in the "found" dbox. Better fix is that if there are
 *     any "action" buttons in the box then set the caret in this
 *     box: but, no time to do this for the major release.
 *   14-Dec-88 WRS: prequit messages marked as harmful.
 *     Otherwise you can crash it by hitting sh-ctl-12 repeatedly,
 *     cos dboxquery will recurse...
 *   07-Mar-89 SKS: hacked format so I can read it!
 *   22-Mar-89 SKS: added a hook into the window definition
 *   05-Apr-89 SKS: made PDACTION changes for PipeDream
*/

#include "include.h"

#include <stdio.h>

#include "werr.h"
#include "akbd.h"
#include "wimp.h"
#include "wimpt.h"
#include "win.h"
#include "menu.h"
#include "event.h"
#include "sprite.h"
#include "res.h"
#include "resspr.h"
#include "misc.h"
#include "upcall.h"

#include "dbox.h"


typedef struct dbox__str
{
    union
    {
    struct dbox__str *next; /* only used for the list of templates */
    wimp_w w;               /* only used in live dialog boxes */
    } d;

    dbox_handler_proc     eventproc;
    void *eventprochandle;

    dbox_raw_handler_proc raweventproc;
    void *raweventprochandle;

    BOOL showing;
    BOOL posatcaret;        /* appears "near" the caret when shown */

    dbox_field field;       /* button last pressed */
    BOOL fieldwaiting;      /* a button waiting to be picked up */

    int eventdepth;         /* for delaying disposal */
    BOOL disposepending;    /* death imminent */

    char name[12];
    char *workspace;
    int workspacesize;
    wimp_wind window;
    /* any icons follow directly after this */
} dbox__str;

/* Abstraction: a dbox is really a dbox__str * */


/* -------- Miscellaneous. -------- */

/* Make a copy of the given dbox template and its workspace */

static dbox
dbox__copy(dbox from)
{
    dbox to;
    int j;
    int size = sizeof(dbox__str) +
               from->window.nicons * sizeof(wimp_icon);
    int ind_change;
    wimp_wind *wto;
    wimp_icon *i;
    char *ptr;

    tracef1("dbox__copy, size = %i\n", size);
    to = malloc(size);
    if(!to)
        return(to);
    memcpy(to, from, size);

    if(to->workspacesize)
        {
        tracef1("needs workspace, size = %d\n", to->workspacesize);
        to->workspace = malloc(to->workspacesize);
        if(!to->workspace)
            {
            free(to);
            return(NULL);
            }
        tracef3("Copy workspace of %i from &%p to &%p\n",
                 to->workspacesize,
                 from->workspace,
                 to->workspace);
        memcpy(to->workspace, from->workspace, to->workspacesize);

        ind_change = to->workspace - from->workspace;

        wto = &to->window;

        /* relocate indirection pointers */
        for(j = 0; j < wto->nicons; j++)
            {
            i = ((wimp_icon *) (wto + 1)) + j;

            if(i->flags & wimp_INDIRECT)
                {
                i->data.indirecttext.buffer = i->data.indirecttext.buffer + ind_change;
                /* not needed for sprite-only icons.
                 * Note that the Wimp sometimes uses -1 as a
                 * null string, thus the > 0.
                */
                if( (i->flags & wimp_ITEXT)  &&
                    (((int) i->data.indirecttext.validstring) > 0))
                        i->data.indirecttext.validstring = i->data.indirecttext.validstring + ind_change;
                }
            }

        /* relocate title indirection pointer if was in our indirected space */
        if(wto->titleflags & wimp_INDIRECT)
            {
            ptr = wto->title.indirecttext.buffer;

            if( (ptr >= from->workspace)  &&
                (ptr < from->workspace + from->workspacesize))
                    wto->title.indirecttext.buffer = ptr + ind_change;
            }
        }
    else
        to->workspace = NULL;

    return(to);
}


static void
dbox__dispose(dbox d)
{
    if(d->workspace)
        {
        #if defined(WATCH_ALLOCS)
            tracef0("freeing dbox workspace: ");
        #endif
        free(d->workspace);
        }

    #if defined(WATCH_ALLOCS)
        tracef0("freeing dbox: ");
    #endif
    free(d);
}


/* This is more logically connected with dbox_dispose below, the
 * ordering is dictated by importation of process_wimp_event in dbox_new.
*/
static void
dbox__dodispose(dbox d)
{
    (void) win_register_event_handler(d->d.w, NULL, NULL);

    event_attachmenu(d->d.w, 0, 0, 0);

    if(d->showing)
        win_activedec();

    wimpt_complain(wimp_delete_wind(d->d.w));

    dbox__dispose(d);
}
/* The menu is removed just in case any client had registered one */


/* -------- The Template Table. -------- */

/* >>>> Where should "system" templates come from? In particular we
assume "err" and "rerr" within this module. This may change, for
instance if this facility is provided by the OS. Other small packages
requiring collections of templates have not appeared */

static dbox dbox__templates = NULL;

#define USE_BUFFER_ON_STACK 1

static const char Not_enough_space[] =
                 "Not enough space to load dialogue boxes";

/* returns TRUE if sprites used */
static BOOL
dbox__readintemplatefile(const char *name)
{
    wimp_wflags clearbits;
    int j;
    os_error *err;
    dbox t;
    BOOL sprites = FALSE;
    dbox buffer; /* could be on stack, but it's quite big */
    wimp_template tem;
    char workspace[1280];
    wimp_wind *w;
    #if USE_BUFFER_ON_STACK
    char buffer_on_stack[sizeof(dbox__str) + 100 * sizeof(wimp_icon)];

    /* check that it's not excessive */
    /*assert(sizeof(buffer_on_stack) < 8*1024);*/
    #endif

    tracef0("dboxes.Read\n");
    if(wimp_open_template(name))
        werr_fatal("Template file not found");

    #if USE_BUFFER_ON_STACK
    buffer = (dbox) buffer_on_stack;
    #else
    /* create a big buffer for the dbox__str's */
    buffer = malloc(sizeof(dbox__str) + 100 * sizeof(wimp_icon));
    if(!buffer)
        {
        wimp_close_template();
        werr_fatal(Not_enough_space);
        }
    #endif

    tem.index = 0;

    while(TRUE)
        {
        tracef0("reading template\n");
        tem.buf       = &buffer->window;
        tem.work_free = workspace;
        tem.work_end  = workspace + sizeof(workspace);
        tem.font      = 0;   /* >>>> no fonts for now */
        tem.name      = &buffer->name[0];
        buffer->name[0] = '*';    /* build name pattern */
        buffer->name[1] = 0;
        tracef1("index=%i\n", tem.index);
        err = wimp_load_template(&tem);
        if(err)
            {
            wimp_close_template();
            werr_fatal("Template load failed because %s", err->errmess);
            }

        tracef2("read template, index=%i nicons=%i\n",
                tem.index, buffer->window.nicons);
        if(!tem.index)
            break;

        buffer->d.w             = 0;
        buffer->workspace       = workspace;
        buffer->workspacesize   = tem.work_free - workspace;

        t = dbox__copy(buffer);
        if(!t)
            {
            wimp_close_template();
            werr_fatal(Not_enough_space);
            }

        tracef1("template &%p created\n", t);

        /* initialise some fields: all derived dboxes inherit these
         * on creation
        */
        w = &t->window;

        /* knock out back bits on certain dboxes */
        clearbits = wimp_WTRESPASS |
                    ((w->colours[wimp_WCSCROLLOUTER] == 12) ? wimp_WBACK : 0);

        t->eventproc        = NULL;
        t->raweventproc     = NULL;
        t->showing          = FALSE;
        t->posatcaret       = (w->flags & wimp_WTRESPASS);
        t->field            = 0;
        t->fieldwaiting     = FALSE;
        t->eventdepth       = 0;
        t->disposepending   = FALSE;
        w->flags            = w->flags & ~clearbits;

        t->d.next = dbox__templates;            /* add to list of templates */
        dbox__templates = t;

        /* Look for sprites */
        for(j = 0; j < w->nicons; j++)
            {
            wimp_icon *i = ((wimp_icon *) (w + 1)) + j;

            if(i->flags & wimp_ISPRITE)
                {
                if(TRACE  &&  !sprites)
                    tracef0("Sprite in use\n");
                sprites = TRUE;
                }
            }

        w->spritearea = resspr_area();
        tracef1("Binding sprites to area &%p\n", resspr_area());
    }

    wimpt_noerr(wimp_close_template());

    #if !USE_BUFFER_ON_STACK
    free(buffer);
    #endif

    tracef0("dboxes.Read exit\n");
    return(sprites);
}


static BOOL
dbox__nameeq(const char *s, const char *s12)
{
    int i;
    char c;
    char c12;
    BOOL b12;

    for(i = 0; i < 12; i++)
        {
        c   = *s++;
        c12 = *s12++;
        b12 = (c12 < 32); /* s12 finished? */

        if(c == '\0')
            return(b12); /* finished together? */

        if(b12  ||  (c != c12))
            return(FALSE); /* finished s12 before s, or failed match */
        }

    return(*s == '\0'); /* s12 full 12 chars, has s finished? */
}


static dbox
dbox__findtemplate(const char *name)
{
    dbox d = dbox__templates;

    while(d  &&  !dbox__nameeq(name, d->name))
        d = d->d.next;

    if(TRACE  &&  !d)
        werr_fatal("Template '%s' not found", name);

    return(d);
}


/* -------- Finding Icons. -------- */

/* useful icon flag masks, for searching for specific icon types */
#define BUTTON_IFLAGS       (0x0F               * wimp_IBTYPE)
#define WRITABLE_IFLAGS     (wimp_BWRITABLE     * wimp_IBTYPE)
#define CLICK_IFLAGS        (wimp_BCLICKDEBOUNCE* wimp_IBTYPE)
#define AUTO_IFLAGS         (wimp_BCLICKAUTO    * wimp_IBTYPE)
#define RELEASE_IFLAGS      (wimp_BSELREL       * wimp_IBTYPE)
#define ONOFF_IFLAGS        (wimp_BSELDOUBLE    * wimp_IBTYPE)
#define ONOFF2_IFLAGS       (wimp_BCLICKSEL     * wimp_IBTYPE)
/* This (button type 11) is the preferable form for on/off switches now.
 * The old one is retained to make moving over easier.
*/
#define MENU_IFLAGS         (wimp_BSELNOTIFY    * wimp_IBTYPE)

/* Rather like SWI WhichIcon, but only finds the first.
 * Returns FALSE if not found.
*/

static BOOL
dbox__findicon(dbox d, wimp_iconflags mask,
                       wimp_iconflags settings, wimp_i *j /*inout*/)
{
    wimp_wind *w = &d->window;
    wimp_i icon = *j;
    wimp_icon *i;

    while(icon < w->nicons)
        {
        i = ((wimp_icon *) (w + 1)) + icon;

        if((i->flags & mask) == settings)
            {
            tracef1("Found icon %i\n", icon);
            *j = icon;
            return(TRUE);
            }

        icon++;
        }

    *j = icon;
    return(FALSE);
}


static BOOL
dbox__findiconbefore(dbox d, wimp_iconflags mask,
                             wimp_iconflags settings, wimp_i *j /*inout*/)
/* Does not look at the current icon */
{
    wimp_wind *w = &d->window;
    wimp_i icon = *j;
    wimp_icon *i;

    while(icon--)
        {
        i = ((wimp_icon *) (w + 1)) + icon;

        if((i->flags & mask) == settings)
            {
            tracef1("Found icon %i\n", icon);
            *j = icon;
            return(TRUE);
            }
        }

    *j = icon;
    return(FALSE);
}


/* -------- Icons and Fields. -------- */

/* >>>> What to do about decorations which must go behind? Icons with
"none" button type should perhaps be ignored, as they can't possibly be
fields */

#if FALSE
static dbox_field
dbox__icontofield(wimp_i i)
{
    return(i);
}
#endif

/* >>>> More complex mappings could perhaps be introduced to allow
decorations around the place. Not clear if this is useful */

#if TRUE
#define dbox__fieldtoicon(f) f
/* 330 compiler winges about static here */
extern wimp_i (dbox__fieldtoicon)(dbox_field f);
#else
static wimp_i dbox__fieldtoicon(dbox_field f)
{
    return(f);
}
#endif


static wimp_icon *
dbox__iconhandletoptr(dbox d, wimp_i i)
{
    if(TRACE  &&  (i > d->window.nicons))
        werr_fatal("dbox field %d does not exist", i);  

    return(((wimp_icon *) (&d->window + 1)) + i);
}
/* >>>> Use this in the icon searches! */


static wimp_icon *
dbox__fieldtoiconptr(dbox d, dbox_field f)
{
    return(dbox__iconhandletoptr(d, dbox__fieldtoicon(f)));
}


static wimp_iconflags
dbox__ibutflags(wimp_icon *i)
{
    return(i->flags & BUTTON_IFLAGS);
}


/* returns OutputF if not sure */

static dbox_fieldtype
dbox__iconfieldtype(wimp_icon *i)
{
    switch(dbox__ibutflags(i))
        {
        case AUTO_IFLAGS:
        case RELEASE_IFLAGS:
        case CLICK_IFLAGS:
        case MENU_IFLAGS:
            return(dbox_FACTION);

        case ONOFF_IFLAGS:
        case ONOFF2_IFLAGS:
            return(dbox_FONOFF);

        case WRITABLE_IFLAGS:
            return(dbox_FINPUT);

        default:
            return(dbox_FOUTPUT);
        }
}


extern void
dbox_setfield(dbox d, dbox_field f, const char *value)
{
    wimp_icon *i = dbox__fieldtoiconptr(d, f);
    wimp_caretstr caret;
    int newlen, oldlen, maxlen;

    tracef3("dbox_setfield(&%p, %d, \"%s\"): ", d, f, value);

    if(TRACE  &&  !(i->flags & wimp_ITEXT))
        {
        tracef0(" non-text icon - ignored");
        return;
        }

    if(TRACE  &&  !(i->flags & wimp_INDIRECT))
        werr_fatal("dbox field %d has inline buffer", f);

    maxlen = i->data.indirecttext.bufflen - 1;  /* MUST be term */
    oldlen = strlen(i->data.indirecttext.buffer);
    i->data.indirecttext.buffer[0] = '\0';
    strncat(i->data.indirecttext.buffer, value, maxlen);
    tracef1(" indirect buffer now \"%s\"\n", i->data.indirecttext.buffer);
    newlen = strlen(i->data.indirecttext.buffer);

    /* ensure that the caret moves correctly if it's in this icon */
    wimpt_safe(wimp_get_caret_pos(&caret));
    tracef2("caret in window %d, icon %d\n", caret.w, caret.i);

    if((caret.w == d->d.w)  &&  (caret.i == dbox__fieldtoicon(f)))
        {
        if(caret.index == oldlen)
            caret.index = newlen;       /* if grown and caret was at end,
                                         * move caret right */
        if(caret.index > newlen)
            caret.index = newlen;       /* if shorter, bring caret left */

        caret.height = -1;   /* calc x,y,h from icon/index */

        wimpt_complain(wimp_set_caret_pos(&caret));
        }

    /* prod it, to cause redraw */
    wimpt_complain(wimp_set_icon_state(d->d.w, dbox__fieldtoicon(f),
                                        /* EOR */   0,
                                        /* BIC */   wimp_ISELECTED));
}


extern void
dbox_getfield(dbox d, dbox_field f, char *buffer /*out*/, size_t size)
{
    wimp_icon *i = dbox__fieldtoiconptr(d, f);
    int j = 0;
    char *from;

    tracef4("dbox_getfield(&%p, %d, &%p, %d): ", d, f, buffer, size);

    if(TRACE  &&  !(i->flags & wimp_ITEXT))
        tracef0("- non-text icon: ");   /* returns "" */
    else
        {
        if(TRACE  &&  !(i->flags & wimp_INDIRECT))
            werr_fatal("dbox field %d has inline buffer", f);

        from = i->data.indirecttext.buffer;
        while(from[j] >= 32)
            j++;

        j = min(j, size);
        memcpy(buffer, from, j);
    }

    buffer[j] = '\0';
    tracef1(" returns \"%s\"\n", buffer);
}


static int
dbox__fieldlength(dbox d, dbox_field f)
{
    char a[256];

    dbox_getfield(d, f, a, sizeof(a));
    tracef1("got field %i in FieldLength\n", f);
    return(strlen(a));
}


extern void
dbox_setnumeric(dbox d, dbox_field f, int n)
{
    char a[20];
    wimp_icon *i = dbox__fieldtoiconptr(d, f);
    dbox_fieldtype ftype = dbox__iconfieldtype(i);

    switch(ftype)
        {
        case dbox_FONOFF:
        case dbox_FACTION:
            wimpt_complain(wimp_set_icon_state(d->d.w, dbox__fieldtoicon(f),
                                        /* EOR */   n ? wimp_ISELECTED : 0,
                                        /* BIC */   wimp_ISELECTED));
            break;

        default:
            sprintf(a, "%i", n);
            dbox_setfield(d, f, a);
            break;
            }
}


/* No error condition, we do the best we can */
/* >>>> Some simple expression mangling might be nice: or, we could give it
 * to Arthur's expression analyser.
*/
extern int
dbox_getnumeric(dbox d, dbox_field f)
{
    char a[20];
    int n;
    int i;
    int ch;
    BOOL neg;
    BOOL ok;
    wimp_icon icon;
    dbox_fieldtype ftype = dbox__iconfieldtype(dbox__fieldtoiconptr(d, f));

    switch(ftype)
        {
        case dbox_FONOFF:
        case dbox_FACTION:
            wimpt_safe(wimp_get_icon_info(d->d.w, dbox__fieldtoicon(f), &icon));
            n = ((icon.flags & wimp_ISELECTED) != 0);
            break;

        default:
            dbox_getfield((dbox) d, f, a, sizeof(a));
            tracef1("dbox_getnumeric on '%s'\n", a);
            n = 0;
            i = 0;
            neg = FALSE;
            ok  = TRUE;

            while(ok  &&  ((ch = a[i++]) != '\0'))
            {
                if(ch == '-')
                    {
                    if(i != 1) /* must have been first char read */
                        ok = FALSE;
                    else
                        neg = TRUE;
                    }
                elif(isdigit(ch))
                    {
                    if(n > (INT_MAX/10))
                        ok = FALSE;
                    else
                        n = n * 10 + ch - '0';
                    }
                else
                    {
                    tracef1("dbox_getnumeric fails with %d\n", ch);
                    ok = FALSE;
                    }
            }

            if(!ok)
                n = 0;
            elif(neg)
                n = -n;

            break;
        }

    return(n);
}


/* -------- Arrival of events from dboxes. -------- */

/* If he is well-behaved then all will be fine! e.g. only read this from an
EventHandler. Otherwise, I don't what conditions to place on the processing
of events */
/* >>>> Could improve on this? e.g. squish PopUp into this. Yes,
maybe a good idea */

extern dbox_field
dbox_get(dbox d)
{
    d->fieldwaiting = FALSE;
    return(d->field);
}


extern BOOL
dbox_queue(dbox d)
{
    return(d->fieldwaiting);
}


extern void
dbox_fakeaction(dbox d, dbox_field f)
{
    tracef2("dbox_fakeaction(&%p, %d)\n", d, f);
    d->field        = f;
    d->fieldwaiting = TRUE;
}


extern void
dbox_eventhandler(dbox d, dbox_handler_proc handler, void *handle)
{
    d->eventproc       = handler;
    d->eventprochandle = handle;
}


extern void
dbox_raw_eventhandler(dbox d, dbox_raw_handler_proc handler, void *handle)
{
    d->raweventproc       = handler;
    d->raweventprochandle = handle;
}


extern void
dbox_raw_eventhandlers(dbox d, dbox_raw_handler_proc *handlerp, void **handlep)
{
    if(*handlerp)
        *handlerp = d->raweventproc;

    if(*handlep)
        *handlep  = d->raweventprochandle;
}


/* -------- Processing Wimp Events. -------- */

static void
dbox__buttonclick(dbox d, dbox_field f)
{
    tracef1("Button click icon %i\n", f);
    d->field        = f;
    d->fieldwaiting = TRUE;

    if(d->eventproc)
        {
        tracef0("obeying user event proc\n");

        d->eventdepth = d->eventdepth + 1;

        d->eventproc(d, d->eventprochandle);

        d->eventdepth = d->eventdepth - 1;

        if(d->disposepending  &&  !d->eventdepth)
            {
            tracef0("delayed dispose of dbox\n");
            dbox__dodispose(d);
            }
        }
}


/* A button is an action button or an on/off switch.
 * "button" counts only such interesting buttons,
 * button == 0 -> the first one in the dbox.
 * Find the right icon.
 * If an action, do it.
 * If on/off, flip it.
 * If button is too big, do nothing.
*/

static void
dbox__hitbutton(dbox d, int button)
{
    wimp_icon *i;
    int j = 0; /* counts icons */
    dbox_fieldtype f;
    wimp_icon icon;

    for(j = 0; j < d->window.nicons; j++)
        {
        i = dbox__iconhandletoptr(d, j);
        f = dbox__iconfieldtype(i);
        if((f == dbox_FACTION)  ||  (f == dbox_FONOFF))
            {
            if(!button)
                {
                /* this is the right one */
                if(f == dbox_FACTION)
                    {
                    tracef1("buttonclick %i\n", j);
                    dbox__buttonclick(d, j);
                    }
                else
                    {
                    /* on/off button */
                    tracef1("Flip on/off %i\n", j);
                    /* >>>> doesn't seem to work?
                     * wimpt_noerr(wimp_set_icon_state(d->d.w, j, 0, wimp_ISELECTED));
                    */
                    wimpt_safe(wimp_get_icon_info(d->d.w, j, &icon));
                    wimpt_complain(wimp_set_icon_state(d->d.w, j,
                                /* EOR */   wimp_ISELECTED,
                                /* BIC */   (icon.flags & wimp_ISELECTED)
                                                 ? 0 : wimp_ISELECTED));
                    /* inverted the select bit */
                    }
                break;
                }
            else
                /* right sort, but not this one. keep going */
                button--;
            }
        else
            {
            /* not the right sort of icon: keep going */
            }
        }
}


static BOOL
dbox__event_handler(wimp_eventstr *e, void *handle)
{
    dbox d = (dbox) handle;
    wimp_icon *i;
    wimp_i j;
    char target;
    BOOL setcaretpos = FALSE;
    wimp_caretstr caret;
    BOOL done;

    tracef2("[dbox__event_handler got event %s for dbox &%p]\n", trace_wimp_event(e), d);

    if(d->raweventproc)
        {
        tracef2("calling client-installed raw event handler(%s, &%p)\n",
                trace_procedure_name((trace_proc) d->raweventproc),
                d->raweventprochandle);

        d->eventdepth = d->eventdepth + 1;

        done = (d->raweventproc)(d, (void *) e, d->raweventprochandle);

        d->eventdepth = d->eventdepth - 1;

        if(d->disposepending  &&  !d->eventdepth)
            {
            tracef0("delayed dispose of dbox (can't send event to body)\n");
            dbox__dodispose(d);
            return(done);
            }

        if(done)
            return(done);
        }


    /* process some events */
    done = TRUE;

    switch(e->e)
        {
        case wimp_ECLOSE:
            dbox__buttonclick(d, dbox_CLOSE); /* special button code */
            break;


        case wimp_EBUT:
            if(wimp_BMID & e->data.but.m.bbits)
                tracef1("ignoring MENU click on icon %d\n", e->data.but.m.i);
                /* ignore it */
                /* It will already have been intercepted (by Events) if there's
                 * a menu, otherwise we're not interested anyway
                */
            elif(e->data.but.m.i != (wimp_i) -1)
                {
                /* ignore clicks not on icons */
                i = dbox__iconhandletoptr(d, e->data.but.m.i);
                if(dbox__iconfieldtype(i) == dbox_FACTION)
                    /* avoid spurious double-click from on/off button! */
                    dbox__buttonclick(d, e->data.but.m.i);
                }
            break;


        case wimp_EKEY:
            {
            int ch = e->data.key.chcode;

            wimpt_safe(wimp_get_caret_pos(&caret));

            switch(ch)
                {
                case akbd_Fn+1:
                case akbd_Fn+2:
                case akbd_Fn+3:
                case akbd_Fn+4:
                case akbd_Fn+5:
                case akbd_Fn+6:
                case akbd_Fn+7:
                case akbd_Fn+8:
                case akbd_Fn+9:
                    dbox__hitbutton(d, ch - (akbd_Fn+1));
                    break;

                case akbd_Fn10:
/* ***          case akbd_Fn11:     keep off F11 too!!!     *** */
/* ***          case akbd_Fn12:     keep off F12 !!!        *** */
                    dbox__hitbutton(d, ch - akbd_Fn10 + 9);
                    break;

                case 13: /* return key */
                    tracef1("Caret is in icon %i\n", caret.i);

#ifdef PDACTION
                    /* should be first action button! */
                    dbox__buttonclick(d, 0);
#else
                    if(caret.i != (wimp_i) -1)
                        {
                        caret.i++;
                        if( (caret.i >= d->window.nicons)  ||
                            !dbox__findicon(d,  WRITABLE_IFLAGS,
                                                WRITABLE_IFLAGS, &caret.i)
                            /* find a writeable button */
                          )
                            /* should be first action button! */
                            dbox__buttonclick(d, 0);
                        else
                            {
                            caret.index = dbox__fieldlength(d, caret.i);
                            setcaretpos = TRUE;
                            }
                        }
                    else
                        /* should be first action button! */
                        dbox_buttonclick(d, 0);
#endif
                    break;

                case 27: /* Escape key */
                    dbox__buttonclick(d, dbox_CLOSE);
                    break;

#ifdef PDACTION
                case akbd_TabK:
#endif
                case akbd_DownK:
                    tracef1("Caret is in icon %i\n", caret.i);

                    if(caret.i != (wimp_i) -1)
                        {
                        caret.i++;
                        if( (caret.i >= d->window.nicons)  ||
                            !dbox__findicon(d,  WRITABLE_IFLAGS,
                                                WRITABLE_IFLAGS, &caret.i))
                            {
                            caret.i = 0;
                            (void) dbox__findicon(d, WRITABLE_IFLAGS,
                                                     WRITABLE_IFLAGS, &caret.i);
                            /* bound to find at least the one you started on.*/
                            }

                        caret.index = dbox__fieldlength(d, caret.i);
                        setcaretpos = TRUE;
                        }
                    break;

#ifdef PDACTION
                case akbd_TabK + akbd_Sh:
#endif
                case akbd_UpK:
                    tracef1("Caret is in icon %i\n", caret.i);

                    if(caret.i != (wimp_i) -1)
                        {
                        if(!dbox__findiconbefore(d, WRITABLE_IFLAGS,
                                                    WRITABLE_IFLAGS, &caret.i))
                            {
                            caret.i = d->window.nicons;
                            (void) dbox__findiconbefore(d,  WRITABLE_IFLAGS,
                                                            WRITABLE_IFLAGS, &caret.i);
                            /* bound to find at least the one you started on */
                            }

                        caret.index = dbox__fieldlength(d, caret.i);
                        setcaretpos = TRUE;
                        }
                    break;

                default:
                    /* If not to a field and this is a letter, try matching it
                     * with the first chars of action buttons in this dbox.
                    */
                    if(!(ch & ~0xFF)  &&  isalpha(ch))
                        {
                        BOOL found = FALSE;

                        ch = toupper(ch);       /* now buggered */

                        for(j = 0; (j < d->window.nicons)  &&  !found; j++)
                            {
                            tracef1("trying icon %i\n", j);
                            i = dbox__iconhandletoptr(d, j);
                            if( (i->flags & wimp_ITEXT)  &&
                                (dbox__iconfieldtype(i) == dbox_FACTION))
                                {
                                char *targetptr = (i->flags & wimp_INDIRECT)
                                                        ? i->data.indirecttext.buffer
                                                        : i->data.text;

                                do  {
                                    target = *targetptr++;
                                    if(target == ch)
                                        {
                                        tracef2("clicking on %i, %i\n", j, target);
                                        dbox__buttonclick(d, j);
                                        found = TRUE;
                                        break;
                                        }
                                    /* end if we didn't match the capital letter */
                                    }
                                while(target  &&  !isupper(target));
                                }
                            }
                        }
                    else
                        {
                        tracef1("Key code %i ignored\n", ch);
                        wimp_processkey(ch);
                        }

                break;
                }

            /* end of all EKEY type events */
            if(setcaretpos)
                {
                tracef2("Setting caret in icon %i, index = %i\n",
                        caret.i, caret.index);
                caret.height = -1; /* calc x,y,h from icon/index */
                wimpt_complain(wimp_set_caret_pos(&caret));
                }
            }
            break;


    /*  case wimp_EOPEN:    */
    /*  case wimp_EREDRAW:  */
        default:
            /* do nothing */
            tracef1("[dbox__event_handler: ignored event %s]\n", trace_wimp_event(e));
            done = FALSE;
            break;
        }

    return(done);
}


/* -------- New and Dispose. -------- */

extern void
dbox_verify(const char *name)
{
    (void) dbox__findtemplate(name);
}


extern dbox
dbox_new(const char *name, char **errorp)
{
    os_error *err;
    dbox d;

    tracef1("dbox_new(%s)\n", name);

    d = dbox__copy(dbox__findtemplate(name));

    if(!d)
        *errorp = NULL;
    else
        {
        err = wimp_create_wind(&d->window, &d->d.w);

        if(err  ||  !win_register_event_handler(d->d.w, dbox__event_handler, d))
            {
            *errorp = err ? err->errmess : NULL;
            dbox__dispose(d);
            d = NULL;
            }
        }

    tracef1("dbox_new yields &%p\n", d);
    return(d);
}


/* This is complicated by the following case: if the show is as a result
 * of a submenu message (e.g. that was the last message received) then we
 * open the dbox as a submenu rather than as a standalone window.
*/

static dbox dbox__submenu = NULL;

static void
dbox__doshow(dbox d, BOOL isstatic)
{
    wimp_eventstr *e;
    wimp_mousestr m;
    wimp_caretstr c;
    wimp_openstr o;
    wimp_wstate s;

    if(d->showing)
        return;

    d->showing = TRUE;
    win_activeinc();

    e = wimpt_last_event();
    if((e->e == wimp_ESEND)  &&  (e->data.msg.hdr.action == wimp_MMENUWARN))
        {
        /* this is a dbox that is actually part of the menu tree */
        tracef0("opening dbox as submenu for menu tree");
        if(dbox__submenu  &&  (dbox__submenu != d))
            /* a dialog box is still up that needs closing */
            dbox_sendclose(dbox__submenu);

        if(!wimpt_complain(wimp_create_submenu((wimp_menustr *) d->d.w,
                                                e->data.msg.data.words[1],
                                                e->data.msg.data.words[2])))
            dbox__submenu = d; /* there's only ever one */
        else
            dbox__submenu = 0;
        }
    else
        {
        o.w         = d->d.w;
        o.box       = d->window.box;
        o.scx       = d->window.scx;
        o.scy       = d->window.scy;
        o.behind    = (wimp_w) -1;

        if(d->posatcaret)
            {
            int xsize = o.box.x1 - o.box.x0;
            int ysize = o.box.y1 - o.box.y0;

            /* move to near the caret/pointer */
            if(e->e == wimp_EKEY)
                {
                tracef0("Move dbox to near caret\n");
                wimpt_safe(wimp_get_caret_pos(&c));

                if(c.w != (wimp_w) -1)
                    {
                    wimpt_safe(wimp_get_wind_state(c.w, &s));
                    c.x = c.x + (s.o.box.x0 - s.o.scx);
                    c.y = c.y + (s.o.box.y1 - s.o.scy);
                    }

                m.x = c.x + 100; /* a little to the right */
                m.y = c.y - 120; /* a little down */
                }
            else
                {
                tracef0("Move dbox to near pointer\n");
                wimpt_safe(wimp_get_point_info(&m));
                m.x -= 32; /* try to be a bit into it */
                m.y += 32;
                }

            tracef2("Put dbox at (%i,%i)\n", m.x, m.y);
            o.box.x0 = m.x;
            o.box.y0 = m.y - ysize;
            o.box.x1 = m.x + xsize;
            o.box.y1 = m.y;
            }

        if(isstatic)
            {
            tracef0("opening static dbox");
            wimpt_complain(wimp_open_wind(&o));
            }
        else
            {
            tracef0("opening dbox as top-level menu");
            if(dbox__submenu  &&  (dbox__submenu != d))
                /* a dialog box is still up that needs closing */
                dbox_sendclose(dbox__submenu);

            if(event_create_menu((wimp_menustr *) d->d.w, o.box.x0, o.box.y1))
                dbox__submenu = d; /* there's only ever one */
            else
                dbox__submenu = 0;
            }
        }

    tracef0(" - dbox shown\n");
}


extern void
dbox_show(dbox d)
{
    dbox__doshow(d, FALSE);
}


extern void
dbox_showstatic(dbox d)
{
    dbox__doshow(d, TRUE);
}


/* >>>> even if there are no fill-in fields, this object should still be
selected. Set the caret in a non-icon. This means that the function key event
stuff will work. It also means that keystroke-processing has to handle it
too */

extern void
dbox_hide(dbox d)
{
    tracef1("dbox_hide(&%p): ", d);

    if(d->showing)
        {
        d->showing = FALSE;
        win_activedec();

        if(d == dbox__submenu)
            {
            wimp_wstate ws;
            tracef0("hiding submenu dbox ");
            if(!wimpt_complain(wimp_get_wind_state(d->d.w, &ws)))
                {
                if(ws.flags & wimp_WOPEN)
                    {
                    tracef0("- explicit closure of open window\n");
                    /* dbox being closed without the menu tree knowing about it */
                    /* cause the menu system to close the dbox */
                    event_clear_current_menu();
                    }
                else
                    tracef0("- already been closed by Wimp\n");
                }

            dbox__submenu = NULL;
            }
        else
            {
            tracef0("hiding non-submenu dbox\n");
            wimpt_complain(wimp_close_wind(d->d.w));
            }
        }
    else
        tracef0("dbox not showing\n");
}


extern void
dbox_dispose(dbox *dd)
{
    dbox d = *dd;

    tracef2("dbox_dispose(&%p -> &%p): ", dd, d);

    if(d)
        {
        if(d->eventdepth)
            {
            tracef0("setting pending dispose as in event processor\n");
            d->disposepending = TRUE;       /* don't kill me yet */
            }
        else
            {
            tracef0("doing dispose\n");

            dbox_hide(d);

            dbox__dodispose(d);
            }

        *dd = NULL;
        }
}


/* -------- Event processing. -------- */

/* We cheerfully allow the caret to go elsewhere, but we intercept any
keystroke events and divert them to the dbox. This allows e.g. find commands
to see where in the text they've got to so far. dboxes with no fill-in fields
do not even try to get the caret */

extern dbox_field
dbox_fillin(dbox d)
{
    wimp_eventstr e;
    int harmless;
    wimp_i j = 0;
    dbox_field result;
    wimp_caretstr caret;
    wimp_wstate ws;

    tracef3("dbox_fillin(&%p (%d)): sp ~= &%p\n", d, d->d.w, &ws);

    wimpt_safe(wimp_get_caret_pos(&caret));

    if(caret.w != d->d.w) /* SKS: only set caret pos if not in here */
        {
        if(dbox__findicon(d, WRITABLE_IFLAGS, WRITABLE_IFLAGS, &j))
            {
            tracef1("setting caret in icon %i\n", j);

            caret.w      = d->d.w;
            caret.i      = j;
            caret.x      = 0;
            caret.y      = 0;
            caret.height = -1;      /* calc x,y,h from icon/index */
            caret.index  = dbox__fieldlength(d, j);

            wimpt_complain(wimp_set_caret_pos(&caret));
            }
        else
            tracef0("no writeable icons in dbox\n");
        }
    else
        tracef0("caret already in dbox\n");

    do  {
        #if TRACE
        int i;
        tracef3("[dbox_fillin(&%p (%d)) (sp ~= &%p) doing wimpt_poll]\n", d, d->d.w, &i);
        #endif

        wimpt_poll(wimp_EMNULL, &e);

        tracef3("[dbox_fillin got event %s, dbox__submenu = &%p, %d]\n", trace_wimp_xevent(e.e, &e.data), dbox__submenu, dbox__submenu->d.w);

        if(d == dbox__submenu)
            {
            /* Check to see if the window has been closed.
             * If it has then we are in a menu tree, and the wimp
             * is doing things behind our backs.
            */
            if(wimpt_complain(wimp_get_wind_state(d->d.w, &ws)))
                {
                tracef0("--- unable to ascertain menu dbox state!\n");

                dbox__submenu = NULL;
                return(dbox_CLOSE);
                }

            if(!(ws.flags & wimp_WOPEN))
                {
                tracef0("--- menu dbox has been closed for us!\n");

                dbox__submenu = NULL;

                wimpt_fake_event(&e); /* stuff event back in the queue */

                /* On a redraw event you may not perform operations such
                 * as the deletion of windows before actually servicing the
                 * redraw. This caused the mysterious bug in ArcEdit such
                 * that certain dboxes of the menu tree caused a repaint
                 * of the entire window.
                */
                if(e.e == wimp_EREDRAW)
                    event_process();

                return(dbox_CLOSE);
                }
            else
                {
                tracef0("menu dbox is still open\n");

                if(!(ws.flags & wimp_WTOP))
                    {
                    trace_on();
                    tracef0("menu dbox not at front!!!\n");
                    trace_off();
                    dbox_sendfront(d, FALSE);
                    }
                }
            }

        switch(e.e)
            {
#if 0
            case wimp_ECLOSE:
                if(d != dbox__submenu)
                    harmless = TRUE;
                else
                    harmless = (e.data.o.w == d->d.w);
                break;
#endif

            case wimp_EKEY:
                /* Pass key events to submenu dbox */
                if((e.data.key.c.w != d->d.w)  &&  (d == dbox__submenu))
                    {
                    e.data.key.c.w = d->d.w;
                    e.data.key.c.i = (wimp_i) -1;
                    }
                harmless = TRUE;
                break;

            case wimp_EBUT:
                if(d != dbox__submenu)
                    harmless = TRUE;
                else
                    harmless = (e.data.but.m.w == d->d.w);
                break;

            case wimp_ESEND:
            case wimp_ESENDWANTACK:
                /* prequit message must be marked as harmful! */
                harmless = (e.data.msg.hdr.action != wimp_MPREQUIT);
                /* >>>> Hum: potentially not true, but this is what the
                 * menu mechanism does...
                */
                break;

        /*  case wimp_ENULL:        */
        /*  case wimp_EOPEN:        */
        /*  case wimp_ECLOSE:       */
        /*  case wimp_EREDRAW:      */
        /*  case wimp_EPTRENTER:    */
        /*  case wimp_EPTRLEAVE:    */
        /*  case wimp_ESCROLL:      */
        /*  case wimp_EMENU:        */
        /*  case wimp_ELOSECARET:   */
        /*  case wimp_EGAINCARET:   */
        /*  case wimp_EUSERDRAG:    */
            default:
                harmless = TRUE;
                break;
            }

        wimpt_fake_event(&e); /* stuff it back in the queue in any case */

        if(harmless)
            {
            tracef0("process harmless event\n");
            event_process();
            }
        else
            {
            tracef0("harmful event causes dbox_fillin to give up\n");
            result = dbox_CLOSE;
            break;
            }

        /* keep going round until he presses a button */
        if(dbox_queue(d))
            {
            result = dbox_get(d);
            break;
            }
        }
    while(TRUE);

    return(result);
}


#if !defined(SMALL)

extern dbox_field
dbox_popup(const char *name, const char *message)
{
    dbox_field result = 0;
    dbox d = dbox_new(name);

    if(d)
        {
        dbox_setfield(d, 1, message);
        dbox_show(d);
        result = dbox_fillin(d);
        dbox_dispose(&d);
        }

    return(result);
}

#endif


extern BOOL
dbox_persist(void)
{
    wimp_mousestr m;

    wimpt_safe(wimp_get_point_info(&m));

    return((m.bbits & wimp_BRIGHT) != 0);
}


/* -------- System Hooks. -------- */

extern int
dbox_syshandle(dbox d)
{
    return((int) d->d.w);
}


extern void *
dbox_syswindowptr(const char *dname)
{
    dbox d = dbox__findtemplate(dname);

    return((void *) &d->window);
}


/* force the initial title of a dialog box to a given value */

extern void
dbox_setinittitle(const char *dname, const char *title)
{
    wimp_wind *window = dbox_syswindowptr(dname);
    char *ptr = window->title.indirecttext.buffer;

    *ptr = '\0';
    strncat(ptr, title, window->title.indirecttext.bufflen);
}


/* send a close request to this dbox */

extern void
dbox_sendclose(dbox d)
{
    wimp_w w = dbox_syshandle(d);
    wimp_eventdata msg;

    msg.o.w = w;
    wimpt_send_wmessage(wimp_ECLOSE, (wimp_msgstr *) &msg, w, -1);
}


/* send a front request to this dbox */

extern void
dbox_sendfront(dbox d, BOOL immediate)
{
    wimp_w w = dbox_syshandle(d);
    wimp_eventstr e;

    tracef2("[dbox_sendfront(&%p, immediate = %s)]\n", d, trace_boolstring(immediate));

    wimpt_safe(wimp_get_wind_state(w, (wimp_wstate *) &e.data.o));
    e.data.o.w = w;
    e.data.o.behind = -1;           /* force to the front */
    if(immediate)
        {
        e.e = wimp_EOPEN;
        wimpt_fake_event(&e);
        event_process();
        }
    else
        wimpt_send_wmessage(wimp_EOPEN, (wimp_msgstr *) &e.data.o, w, -1);
}


/* -------- Initialisation. -------- */

extern void
dbox_init(void)
{
    char s[256];

    /* read this program's templates */
    if(res_findname("Templates", s))
        dbox__readintemplatefile(s);
}


/* end of dbox.c */
