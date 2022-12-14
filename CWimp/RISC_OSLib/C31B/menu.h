/* menu.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __menu_h
#define __menu_h
typedef struct menu__str *menu; 
menu menu_new(char *name, char *description);
void menu_dispose(menu*, int recursive);
void menu_extend(menu, char *description);
void menu_setflags(menu, int entry, int tick, int fade);
void menu_submenu(menu, int entry, menu submenu);
 
void menu_make_writeable(menu m, int entry, char *buffer, int bufferlength,
 char *validstring);
void menu_make_sprite(menu m, int entry, char *spritename);
void *menu_syshandle(menu);
#endif
