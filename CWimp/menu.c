/* Title: -> c.menu
 * Purpose: Portable menu manipulation.
 * Author: WRS
 * Status: experimental
 * History:
 *  24 August 87 -- started
 *  SKS tweaked a little 28 Feb 89
 *  big hack 15-May-89 to not explode with fatal errors
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "trace.h"

#include "os.h"
#include "wimp.h"
#include "sprite.h"
#include "resspr.h"
#include "misc.h"
#if TRACE
#include "werr.h"
#endif

#include "menu.h"


typedef struct menu__str
{
    wimp_menuhdr *m;         /* the wimp-level menu that we've built. */
    void *entryspace;        /* for sub-menus, and entries with >12 chars */
    int nitems;
    int nbytes;
    int maxentrywidth;
} menu__str;
/* concrete representation of abstract menu object */

/* The menu__str structure points to an Arthur-style menu, and to a separate
 * buffer of workspace for sub-menu pointers and for string fields that
 * are >12 characters. The format of the entryspace is:
 *  each sub-Menu pointer
 *  a Menu(NIL) word
 *  each entry buffer
*/

/* >>>> Could perhaps improve by simplifying: squash everything into one
space area, and shift things up every time. */


static wimp_menuitem *
menu__itemptr(menu m, int n)
/* Pointer to the nth item in the menu (starting at 0). */
{
    return(((wimp_menuitem *)(m->m + 1)) + n);
}


/* -------- Building Arthur Menus. -------- */

/* The menu is assembled entry by entry in temporary workspace, then copied
to more precisely allocated store. The copying of menu structures is split
into the allocation of store and then the copying of data, so that the copy
into the larger buffer can share the latter half of the operation. */

/* This situation would be improved if shifting/extendable store were used to
record menus, rather than the basic M2 allocation facilities. But,
performance is not believed particularly important for these facilities as
the delay will not be humanly discernable. */


static void
menu__disposespace(menu m)
{
    dispose((void **) &m->m);
    dispose((void **) &m->entryspace);
}


static BOOL
menu__preparecopy(menu from, menu to)
{
    /* Allocate space in the destination to suit the copy. */

    wimp_menuhdr *ptr;
    void *ptr2;

    ptr = malloc(sizeof(wimp_menuhdr)
                + from->nitems * sizeof(wimp_menuitem));

    if(ptr == NULL)
        return(FALSE);

    if(from->nbytes != 0)
        {
        ptr2 = malloc(from->nbytes);

        if(ptr2 == NULL)
            {
            free(ptr);
            return(FALSE);
            }
        }
    else
        ptr2 = NULL;

    to->m           = ptr;
    to->entryspace  = ptr2;

    return(TRUE);
}


/* Copy the data and lengths. Relocate indirection pointers. */

static void
menu__copydata(menu from, menu to)
{
    int moveoffset;
    int i;

    (void) memmove(to->m, from->m,
            sizeof(wimp_menuhdr) + from->nitems * sizeof(wimp_menuitem));

    to->nitems        = from->nitems;
    to->maxentrywidth = from->maxentrywidth;
    to->nbytes        = from->nbytes;

    moveoffset = ((char *) to->entryspace) - ((char *) from->entryspace);

    (void) memmove(to->entryspace, from->entryspace, from->nbytes);

    for(i = 0; i < to->nitems; i++)
        {
        wimp_menuitem *ptr = menu__itemptr(to, i);

        if(ptr->iconflags & wimp_INDIRECT)
            ptr->data.indirecttext.buffer =
                                ptr->data.indirecttext.buffer + moveoffset;
        }
}


/* The work area is allocated on the stack, with the following limits: */

#define WORKAREASIZE  10*1024
#define AVGINDSIZE    32         /* estimated average size of indirected entry */
#define MAXITEMS      ((WORKAREASIZE - sizeof(menu__str) - sizeof(wimp_menuhdr)) / \
                       (sizeof(wimp_menuitem) + AVGINDSIZE))
                                 /* max size of a menu: ca. 192 - surely OK. */
#define MAXENTRYSPACE (MAXITEMS * AVGINDSIZE)
                                 /* space for building entries > 12 chars */

typedef struct {
    menu__str m;
    wimp_menuhdr menuhdr;
    wimp_menuitem menuitems[MAXITEMS];
    char entryspace[MAXENTRYSPACE];
} menu__workarea;


static void
menu__initworkarea(menu__workarea *w)
{
    tracef3("menu__initworkarea(&%p): MAXITEMS %d, sizeof(menu__workarea) = %d\n",
            w, MAXITEMS, sizeof(menu__workarea));

    w->m.m             = &w->menuhdr;
    w->m.entryspace    = &w->entryspace;
    w->m.nitems        = 0;
    w->m.maxentrywidth = 0;
    /* insert a NIL in the entrySpace to distinguish sub-Menu pointers
     * from text space.
    */
    w->m.nbytes = 4;
    *((int *) w->entryspace) = 0;
}


static void
menu__copytoworkarea(menu m /*in*/, menu__workarea *w /*out*/)
{
    wimp_menuitem *p;

    menu__initworkarea(w);
    menu__copydata(m, &w->m);

    p = menu__itemptr(&w->m, w->m.nitems-1);
    p->flags = p->flags & ~wimp_MLAST;
}


static BOOL
menu__copyworkarea(menu__workarea *w /*in*/, menu m /*out*/)
{
    BOOL res;

    if(w->m.nitems > 0)
        {
        wimp_menuitem *p = menu__itemptr(&w->m, w->m.nitems-1);
        p->flags = p->flags | wimp_MLAST;
        }

    menu__disposespace(m);

    res = menu__preparecopy(&w->m, m);
    if(res)
        menu__copydata(&w->m, m);

    return(res);
}


/* -------- Creating menu descriptions. -------- */

static void
menu__initmenu(const char *name, menu m /*out*/)
{
    int i;

    for(i = 0; i < 12; i++)
        {
        /* could use strncpy but for width calculation */
        m->m->title[i] = name[i];
        if(name[i] == '\0')
            break;
        }

    m->m->tit_fcol  = 7;        /* title fore: black */
    m->m->tit_bcol  = 2;        /* title back: grey */
    m->m->work_fcol = 7;        /* entries fore */
    m->m->work_bcol = 0;        /* entries back */
    m->m->width     = i * 16;   /* minimum value */
    m->m->height    = 44;       /* per entry */
    m->m->gap       = 0;        /* gap between entries, in OS units */
}
/* >>>> titles of more than 12 characters impossible */


/* The returned pointer can be used to set flags, etc. */
/* There's no point in being tough on errors: the menu is simply
truncated. */

static wimp_menuitem *
menu__additem(menu__workarea *w /*out*/, const char *name, int length)
{
    wimp_menuitem *ptr;

    if(w->m.nitems == MAXITEMS)
        return(menu__itemptr(&w->m, MAXITEMS-1));

    ptr = menu__itemptr(&w->m, w->m.nitems++);
    ptr->flags      = 0;
    ptr->submenu.m  = (wimp_menustr *) -1;
    ptr->iconflags  = wimp_ITEXT + wimp_IFILLED + wimp_IVCENTRE + (7*wimp_IFORECOL);
    if(length > w->m.maxentrywidth)
        {
        w->m.maxentrywidth  = length;
        w->m.m->width       = max(w->m.m->width, 16 + length * 16);
        /* in OS units, 16 per char. */
        }

    if(length <= 12)
        {
        int i;
        for(i = 0; i < length; i++)
            ptr->data.text[i] = name[i];
        if(length < 12)
            ptr->data.text[length] = '\0';
        }
    elif(length + 1 + w->m.nbytes >= MAXENTRYSPACE)
        {
        /* no room for the text: unlikely */
        ptr = menu__itemptr(&w->m, w->m.nitems-1); /* fudge */
        }
    else
        {
        ptr->iconflags = ptr->iconflags | wimp_INDIRECT;
        ptr->data.indirecttext.buffer       = ((char *) w->m.entryspace) + w->m.nbytes;
        ptr->data.indirecttext.validstring  = (char *) -1;
        ptr->data.indirecttext.bufflen      = 100;
        (void) memmove(((char *) w->m.entryspace) + w->m.nbytes, name, length);
        w->m.nbytes = w->m.nbytes + length + 1;
        ((char *) w->m.entryspace)[w->m.nbytes-1] = '\0'; /* terminate the string. */
        }

    return(ptr);
}


/* -------- Parsing Description Strings. -------- */

#if TRUE
#define menu__syntax()  /* nothing */
#else
static void
menu__syntax(void)
{
/* General policy: be lenient on syntax errors, so do nothing */
}
#endif


typedef enum {
    TICK    = 0x01,
    FADED   = 0x02,
    DBOX    = 0x04
} opt;


typedef enum {
    OPT,
    SEP,
    NAME,
    END
} toktype;


typedef struct {
    char *s;
    toktype t;
    char ch;        /* last separator char encountered */
    opt opts;       /* last opts encountered */
    char *start;
    char *end;      /* last name encountered */
} parser;


static void
menu__initparser(parser *p, const char *s)
{
    p->s    = (char *) s;
    p->ch   = ',';
}


static void
menu__getopt(parser *p)
{
    p->opts = 0;

    while(p->ch == '!'  ||  p->ch == '~'  ||  p->ch == '>'  ||  p->ch == ' ')
        {
        int orflags;

        switch(p->ch)
            {
            case '!':
                orflags = TICK;
                break;

            case '~':
                orflags = FADED;
                break;

            case '>':
                orflags = DBOX;
                break;

            default:
                orflags = 0;
                break;
            }

        p->opts = p->opts | orflags;
        p->ch   = *p->s++;
        }

    p->s--;
}


static void
menu__getname(parser *p)
{
    p->start = p->s - 1;

    while(p->ch != '\0'  &&  p->ch != ','  &&  p->ch != '|')
        p->ch = *p->s++;

    p->end = --p->s;
}


static toktype
menu__gettoken(parser *p)
{
    p->ch = ' ';

    while(p->ch == ' ')
        p->ch = *p->s++;

    switch(p->ch)
        {
        case '\0':
            p->t = END;
            break;

        case '!':
        case '~':
        case '>':
            p->t = OPT;
            menu__getopt(p);
            break;

        case ',':
        case '|':
            p->t = SEP;
            break;

        default:
            p->t = NAME;
            menu__getname(p);
            break;
        }

    return(p->t);
}


#if FALSE
/* SKS not used */
static int
menu__checktok(parser *p, toktype type)
{
    return(menu__gettoken(p) == type);
}
#endif


/* -------- Parsing and Extension. -------- */

static void
menu__doextend(menu__workarea *w, const char *descr)
{
    parser p;
    toktype tok;
    wimp_menuitem *ptr;

    menu__initparser(&p, descr);

    tok = menu__gettoken(&p);
    if(tok == END)
        return;

    if(tok == SEP)
        {
        if(w->m.nitems == 0)
            menu__syntax();
        else
            {
            if(p.ch == '|')
                {
                ptr = menu__itemptr(&w->m, w->m.nitems-1);
                ptr->flags = ptr->flags | wimp_MSEPARATE;
                }

            tok = menu__gettoken(&p);
            }
        }

    while(TRUE)
        {
        if(tok == OPT)
            tok = menu__gettoken(&p); /* must be NAME, check below */
        else
            p.opts = 0;

        if(p.t != NAME)
            menu__syntax();
        else
            {
            ptr = menu__additem(w, p.start, p.end - p.start);

            if(TICK & p.opts)
                ptr->flags = ptr->flags | wimp_MTICK;

            if(FADED & p.opts)
                ptr->iconflags = ptr->iconflags | wimp_INOSELECT;

            if(DBOX & p.opts)
                {
                ptr->flags      = ptr->flags | wimp_MSUBLINKMSG;
                ptr->submenu.m  = (wimp_menustr *) 1;
                }

            tok = menu__gettoken(&p);
            if(tok == END)
                break;

            if(tok != SEP)
                menu__syntax();
            else
                if(p.ch == '|')
                    ptr->flags = ptr->flags | wimp_MSEPARATE;
            }

        tok = menu__gettoken(&p);
        }
}


/* -------- Entrypoints. -------- */

extern menu
menu_new(const char *name, const char *descr)
{
    menu m;
    menu__workarea menu__w;

    tracef2("menu_new(%s, %s)\n", name, descr);

    menu__initworkarea(&menu__w);
    menu__initmenu(name, &menu__w.m);
    menu__doextend(&menu__w, descr);

    m = malloc(sizeof(menu__str));
    if(m)
        {
        m->m            = NULL;
        m->entryspace   = NULL;

        if(!menu__copyworkarea(&menu__w, m))
            dispose((void **) &m);
        }

    tracef1("menu_new() returns &%p\n", m);

    return(m);
}


extern void
menu_dispose(menu *mm /*, BOOL recursive */)
{
    menu m = *mm;

    tracef2("menu_dispose(&%p -> &%p)\n",
                mm, m /*, trace_boolstring(recursive)*/);

    if(m)
        {
        menu__disposespace(m);

        #if defined(WATCH_ALLOCS)
        tracef0("freeing menu: ");
        #endif
        free(m);

        *mm = NULL;
        }
}


extern BOOL
menu_extend(menu *mm, const char *descr)
{
    menu m = *mm;
    menu__workarea menu__w;
    BOOL res;

    tracef2("menu_extend(&%p, %s)\n", m, descr);

    if(m)
        {
        menu__copytoworkarea(m, &menu__w);

        menu__doextend(&menu__w, descr);

        res = menu__copyworkarea(&menu__w, m);

        if(!res)
            m = NULL;
        }
    else
        res = FALSE;

    *mm = m;

    return(res);
}


extern void
menu_setflags(menu m, int entry, BOOL tick, BOOL fade)
{
    wimp_menuitem *p;

    tracef4("menu_setflags(&%p, %d, tick = %s, fade = %s)\n",
                m, entry, trace_boolstring(tick), trace_boolstring(fade));

    if(!m)
        return;

    #if TRACE
    if((entry == 0)  ||  (entry > m->nitems)) 
        {
        werr("duff entry in menu_setflags - ignored");
        return;
        }
    #endif

    p = menu__itemptr(m, entry-1);

    p->flags     = (tick)   ? p->flags |  wimp_MTICK
                            : p->flags & ~wimp_MTICK;

    p->iconflags = (fade)   ? p->iconflags |  wimp_INOSELECT
                            : p->iconflags & ~wimp_INOSELECT;
}


extern void
menu_make_writeable(menu m, int entry, char *buffer, int bufferlength,
                    const char *validstring)
{
    wimp_menuitem *p;

    tracef5("menu_make_writeable(&%p, %d, &%p, %d, %s)\n",
                m, entry, buffer, bufferlength,
                (validstring == NULL)
                        ? (const char *) "<NULL>"
                        : (validstring == (const char *) -1)
                                    ? (const char *) "<-1>"
                                    : validstring);

    if(!m)
        return;

    #if TRACE
    if((entry == 0)  ||  (entry > m->nitems)) 
        {
        werr("duff entry in menu_make_writeable - ignored");
        return;
        }
    #endif

    p = menu__itemptr(m, entry-1);

    p->flags     = p->flags | wimp_MWRITABLE;

    p->iconflags = p->iconflags | (wimp_BWRITABLE * wimp_IBTYPE
                                    + wimp_ITEXT + wimp_INDIRECT
                                    /*+ wimp_IHCENTRE + wimp_IVCENTRE*/);

    p->data.indirecttext.buffer      = buffer;
    p->data.indirecttext.bufflen     = bufferlength;
    p->data.indirecttext.validstring = validstring;
}


#if !defined(SMALL)

extern void
menu_make_sprite(menu m, int entry, const char *spritename)
{
    wimp_menuitem *p;

    tracef3("menu_make_sprite(&%p, %d, %s)\n", m, entry, spritename);

    if(!m  ||  (entry == 0)  ||  (entry > m->nitems)) 
        return;

    p = menu__itemptr(m, entry-1);

    p->iconflags =  (p->iconflags & ~wimp_ITEXT) |
                    (wimp_INDIRECT + wimp_IVCENTRE + wimp_ISPRITE);

    p->data.indirectsprite.name       = spritename;
    p->data.indirectsprite.spritearea = resspr_area();
    p->data.indirectsprite.nameisname = TRUE;
}

#endif


extern BOOL
menu_submenu(menu m, int place, menu submenu)
{
    wimp_menuitem *p;

    tracef3("menu_submenu(&%p, %d, &%p)\n", m, place, submenu);

    if(!m)
        return(FALSE);

    p = menu__itemptr(m, place-1);

    p->submenu.m = submenu ? (wimp_menustr *) submenu->m : (wimp_menustr *) submenu;

    return(TRUE);
}


extern void *
menu_sysmenu(menu m)
{
    return(m ? (void *) m->m : (void *) m);
}


/* end of menu.c */
