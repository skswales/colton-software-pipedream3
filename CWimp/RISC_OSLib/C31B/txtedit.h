/* txtedit.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __txtedit_h
#define __txtedit_h
#ifndef __txt_h
#include "txt.h"
#endif
#ifndef __typdat_h
#include "typdat.h"
#endif
#ifndef __menu_h
#include "menu.h"
#endif
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef enum
 {
 txtedit_CHARSEL = 1, txtedit_WORDSEL = 2, txtedit_LINESEL = 4
 } txtedit_seltype;
typedef struct txtedit_state
 {
 txt t;
 txt_marker selpivot; 
 txtedit_seltype seltype; 
 int selectrecent; 
 int selectctl; 
 char filename[256];
 typdat ty;
 int deletepending;
 struct txtedit_state *next; 
 BOOL overwrite;
 BOOL wordtab;
 BOOL wordwrap;
 } txtedit_state;
typedef BOOL (*txtedit_update_handler)(char *, txtedit_state *, void *);
typedef void (*txtedit_close_handler)(char *, txtedit_state *, void *);
typedef BOOL (*txtedit_save_handler)(char *, txtedit_state *, void *);
typedef void (*txtedit_shutdown_handler)(void *);
typedef void (*txtedit_undofail_handler)(char *, txtedit_state *, void *);
typedef void (*txtedit_open_handler)(char *, txtedit_state *, void *);
txtedit_state *txtedit_install(txt t);
txtedit_state *txtedit_new(char *filename);
 
void txtedit_dispose(txtedit_state *s);
BOOL txtedit_mayquit(void);
void txtedit_prequit(void);
menu txtedit_menu(txtedit_state *s);
 
void txtedit_menuevent(txtedit_state *s, char *hit);
BOOL txtedit_doimport(txtedit_state *s, int filetype, int estsize);
void txtedit_doinsertfile(txtedit_state *s, char *filename, BOOL replaceifwasnull);
txtedit_update_handler txtedit_register_update_handler(txtedit_update_handler h, void *handle);
txtedit_save_handler txtedit_register_save_handler(txtedit_save_handler h, 
 void *handle);
txtedit_close_handler txtedit_register_close_handler(txtedit_close_handler h, void *handle);
txtedit_shutdown_handler txtedit_register_shutdown_handler(txtedit_shutdown_handler h, void *handle);
txtedit_undofail_handler txtedit_register_undofail_handler(txtedit_undofail_handler h, void *handle);
txtedit_open_handler txtedit_register_open_handler(txtedit_open_handler h, void *handle);
txtedit_state *txtedit_getstates(void);
void txtedit_init(void);
#endif
