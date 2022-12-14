/* event.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __event_h
#define __event_h
#ifndef __menu_h
#include "menu.h"
#endif
#ifndef __wimp_h
#include "wimp.h"
#endif
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
void event_process(void);
BOOL event_anywindows(void);
typedef int event_w;
typedef void (*event_menu_proc)(void *handle, char* hit);
typedef menu (*event_menu_maker)(void *handle);
 
BOOL event_attachmenu(event_w, menu, event_menu_proc, void *handle);
BOOL event_attachmenumaker(
 event_w, event_menu_maker, event_menu_proc, void *handle);
void event_clear_current_menu(void);
BOOL event_is_menu_being_recreated(void) ;
void event_setmask(wimp_emask mask);
wimp_emask event_getmask(void);
#endif
