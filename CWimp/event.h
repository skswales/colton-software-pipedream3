/* > h.event */

/* Title:   event.h
 * Purpose: system-independent central processing for window sytem events.
 * Requires:
 *   "h.menu"
 * History:
 *  20 August 87: WRS: started
 *  13-Dec-87: WRS: converted to C.
 *  02-Mar-88: WRS: use of BOOL.
 *  10-Mar-88: WRS: icon bar menu events.
*/

#ifndef __event_h
#define __event_h

#ifndef __menu_h
#include "menu.h"
#endif

#ifndef __wimp_h
#include "wimp.h"
#endif

/* -------- Processing Events. -------- */

extern void event_process(void);
/* Process one event, then return. Usually used in a loop until some
 * condition is satisfied. If the machine is idle, event_process will wait
 * until something has happened before returning, in the hope that this will
 * help to satisfy the condition. If the event is a menu activation keyclick
 * then the menu relevant to the indicated window will be activated, as
 * set up using the facilities below.
*/


/* -------- Attaching menus. -------- */

typedef int event_w;
typedef struct event__submenu *event_submenu;
/* A system-dependent representation of a user interface object. Interfaces
 * for user interface objects that contain a "syshandle" system hook function
 * returning an int, in fact return one of these. This is implicit to reduce
 * interdependencies between the definition modules.
*/

typedef BOOL (*event_menu_proc)(void *handle, char* hit, BOOL submenurequest);
typedef menu (*event_menu_maker)(void *handle);

BOOL event_attachmenu(event_w, menu, event_menu_proc, void *handle);
/* The given menu will be activated when the user invokes a menu from that
 * window. attach (menu) 0 to clear. If a menu entry is selected the
 * menu event proc will be called. The hit string passed to the menu event proc
 * when a menu item is selected consists of a character for each
 * level of nested of the selected item within the heirarchy, terminated by
 * a 0 character.
*/


extern BOOL event_attachmenumaker(event_w, event_menu_maker, event_menu_proc, void *handle);
/* Rather than attaching a menu to a window, a procedure can be attached
 * that contructs a menu. The procedure will be called whenever the menu is
 * invoked by the user, allowing setting of flags etc. at the last minute. The
 * menu will be destroyed at the end of the operation. The menumaker and the
 * menu event proc share the same data handle.
*/

/* The above return TRUE if the attachment succeeded, or FALSE
if it failed due to space allocation failing. */

/* Bug and fix: in order to receive menu hits on the icon bar, register
a handler for window -10. This is because -1 is used within this module
to mean "no window". */


extern BOOL event_create_menu(wimp_menustr *m, int x, int y);
/* for dbox */

extern void event_clear_current_menu(void);
/* It's not possible for the client of this interface to tell if a
 * registered menu tree is still active, or currently visible. This call
 * definitely clears away the current menu tree.
 * In Arthur terms: a null menu tree is registered with the Wimp.
*/


extern BOOL event_is_menu_recreate_pending(void);
/* Callable inside your menu processors, it indicates whether the menu will be recreated
*/

extern BOOL event_is_menu_being_recreated(void);
/* Callable inside your menu makers, it indicates whether information
 * should be cached on the basis of the current pointer position
*/


extern void event_read_submenudata(event_submenu *smp, int *xp, int *yp);
/* Read the submenu pointer and x, y position to open a submenu at */


#endif  /* __event_h */


/* end of event.h */
