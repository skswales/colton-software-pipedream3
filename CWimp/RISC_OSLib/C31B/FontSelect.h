/* FontSelect.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __fontselect_h
#define __fontselect_h
typedef BOOL (*fontselect_fn)(char *font_name, double width, double height, wimp_eventstr *event );
int fontselect_init( void );
void fontselect_closedown( void );
void fontselect_closewindows( void );
int fontselect_selector( char *title, int flags, char *font_name, double width, double height, fontselect_fn unknown_icon_routine );
#define fontselect_REOPEN 0x001
#define fontselect_SETTITLE 0x002
#define fontselect_SETFONT 0x004
BOOL fontselect_attach_menu( menu mn, event_menu_proc menu_processor, void *handle );
#endif
