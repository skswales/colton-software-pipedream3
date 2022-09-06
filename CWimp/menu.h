/* Title: menu.h
 * Purpose: Portable menu manipulation.
 * Author: WRS
 * Status: highly experiemental
 *         not intended as a major public interface yet
 * History:
 *   20 August 87 -- started
 *   24-Feb-88: WRS:
 *     Pending change in usage style:
 *        always create your menu structure statically, and use that,
 *        rather than recreating it every time.
 *   28-Feb-89 SKS made some const char *s
*/

#ifndef __menu_h
#define __menu_h

/* This interface supports the convenient construction of simple heirarchical
 * menu trees, from a convenient textual form.
*/

typedef struct menu__str *menu; /* abstract menu handle */

/*
A menu description string defines a sequence of entries, with the following
syntax (curly brackets mean 0 or more, square brackets mean 0 or 1):
    opt   ::= "!" or "~" or ">" or " "
    sep   ::= "," or "|"
    l1    ::= any char but opt or sep
    l2    ::= any char but sep
    name  ::= l1 {l2}
    entry ::= {opt} name
    descr ::= entry {sep entry}

Each entry defines a single entry in the menu. "|" as a separator means that
there should be a gap or line between these menu components.
    opt ! means, put a tick by it
    opt ~ means, make it non-pickable
    space has no effect as an opt.
*/

extern menu menu_new(const char *name, const char *description);
/* Creates a menu, makes some entries according to the description string.
The entries are indexed starting at 1. The name must be of the form:
  l1 {l2}
(e.g. be itself a legitimate menu item, without flags). The description must
be of the form:
  [entry {sep entry}]
*/

extern void menu_dispose(menu *mm /*, int recursive*/);
/* Destroys the menu. If recursive!=0, (recursively) destroys all submenus
first. */

/* >>>> This is menu* rather than menu for compatibility with M2's VAR
convention for allocation. This may change to a menu argument in the future.
*/

extern BOOL menu_extend(menu *mm, const char *description);
/* Add more entries to the end of the menu. The string is of the form:
  [sep] entry {sep entry}
*/

extern void menu_setflags(menu, int entry, int tick, int fade);
/* Set/change the flags on an already existing entry. */

extern BOOL menu_submenu(menu, int entry, menu submenu);
/* Attatch one menu as a submenu of another, accessed at the given place in
the parent menu. Replaces any previous submenu on that place, without harming
the previous submenu. attatch (menu) 0 to erase a submenu entry. Indexing of
menu entries starts at 1. Only a strict heirarchy is allowed, no sub-tree
sharing or cycles. Once a menu has been attatched as a submenu, it cannot
be extended or deleted. */


/* -------- Only for use by system-specific clients. -------- */

extern void menu_make_writeable(menu m, int entry, char *buffer, int bufferlength,
                                const char *validstring);
/* Arthur-specific: make an entry of a menu writable. */


extern void menu_make_sprite(menu m, int entry, const char *spritename);
/* Arthur-specific: make an entry of a menu a sprite */
/* The entry, initialy a non-indirected text entry (as created by menu_new),
   is changed to an indirected sprite, with sprite area given by resspr_area()
   and sprite name held in the buffer passed as spritename.
*/ 


extern void *menu_sysmenu(menu m);
/* The calls above construct a menu in the system's underlying form. This
call gains access to this underlying menu, and allows a system-dependent
client to "massage" to provide special effects that may be available under
one system, e.g. setting colours of entries. */
/* In Arthur terms, this is a wimp_menustr*. */


#endif  /* __menu_h */


/* end of menu.h */
