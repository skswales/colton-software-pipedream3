/* Title: -> h.win
 * Purpose: central management of Arthur windows
 * Author: WRS
 * Status: Arthur/Wimp specific
 * Requires:
 *   wimp.h
 * History:
 *   24 August 87 -- started
 *   03-Dec-87: WRS: converted to C.
 *   02-Mar-88: WRS: use of BOOL.
 *   02-Mar-88: WRS: drag events.
 *   10-Mar-88: WRS: fix for icon bar events.
 *   20-Mar-88: WRS: added claim_unknown_events.
 *   15-Jun-88: APT: added win_ICONBARLOAD
 */

/* This module constructs a very simple idea of "window class" within Arthur.
The type win_str shows all the data that is maintained for a specific window,
using this standard event-processing loops can be built. Arthur window class
implementations register the existence of each window with this module. */

/* This structure allows event-processing loops to be constructed that
have no knowledge of what other modules are present in the program.
For instance, the dialog box module can contain an event-processing loop
without reference to what other window types are present in the program. */

typedef BOOL (*win_event_handler)(wimp_eventstr*, void *handle);

/* -------- Claiming Events. -------- */

extern BOOL win_register_event_handler(wimp_w w, win_event_handler,
                                        void *handle);
/* Register (win_event_handler) 0 for a window in order to forget that
window. Note that this has no effect on the window itself, only on the record
of it kept here. It is permissable to do this with a window that has not
been registered (e.g. "just in case"), there will be no effect. */

/* Bug and fix: in Arthur, key events on icon bar icons appear to come from
window -1. Unfortunately the value -1 is also used to mean "no window" in
this context. In order to catch key events from the icon bar, register a
handler for window win_ICONBAR. */
#define win_ICONBAR     ((wimp_w)  -3)
#define win_ICONBARLOAD ((wimp_w) -99)
#define win_IDLE_OFF    ((wimp_w)  -1)

extern void win_claim_idle_events(wimp_w w);
/* Idle events should be sent to this window.
 * Call with win_IDLE_OFF to stop null events being requested.
*/

typedef BOOL (*win_unknown_event_processor)(wimp_eventstr *e, void *handle);
/* You can ask to vet unknown events, before they are passed to the default
   unknown event handler. These procs return TRUE if they have dealt with the
   event.
*/

extern BOOL win_add_unknown_event_processor(win_unknown_event_processor,
                                            void *handle);
extern void win_remove_unknown_event_processor(win_unknown_event_processor,
                                                void *handle);

extern wimp_w win_idle_event_claimer(void);
/* Says who is currently claiming idle events, or (wimp_w) -1 if noone.
Anyone claiming unknown events temporarily should remember the previous
owner, and reinstate it at the end of the operation. */

extern void win_claim_unknown_events(wimp_w w);
/* Any unknown or non-window-specific events should be sent to this window.
User drag events, closedown messages etc. are sent here. Call with (wimp_w)
-1 to cancel. */

extern wimp_w win_unknown_event_claimer(void);


/* -------- Menus. -------- */

extern void win_setmenuh(wimp_w w, void *handle);
extern void *win_getmenuh(wimp_w w); /* 0 if not set */
/* This is for use by higher level code to manage the automatic
 * association of menus to windows. No interpretation is placed on this
 * word within this module.
*/


/* -------- Event Processing. -------- */

extern BOOL win_processevent(wimp_eventstr *e);
/* Give this event to the relevant window. FALSE is returned if there is no
relevant window. Keyboard events are sent to the current owner of the caret,
if he is known to this module. */


/* -------- Termination. -------- */

extern void win_activeinc(void);
extern void win_activedec(void);
extern int  win_activeno(void);
/* Programs terminate if event processing is attempted with no "active"
 * windows visible. This is so that individual windows may be closed without
 * regard for whether there are "any left". Window implementations should call
 * these routines to maintain a count of the active number of windows on the
 * screen. NB. Icon bar icons should be treated as active windows.
*/

extern void win_give_away_caret(void);
/* Find the top window in this program, and open it at its current position.
 * If it is interested, it will take this as a signal to take the caret.
 * Typically called by windows when they are sent to the bottom. In order
 * for this to work, polling must be performed via the wimpt module.
*/


/* end of win.h */
