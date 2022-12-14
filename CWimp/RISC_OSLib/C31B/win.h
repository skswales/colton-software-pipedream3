/* win.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __win_h
#define __win_h
#ifndef __wimp_h
#include "wimp.h"
#endif
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef void (*win_event_handler)(wimp_eventstr*, void *handle);
#define win_ICONBAR (-3)
#define win_ICONBARLOAD (-99)
void win_register_event_handler(wimp_w, win_event_handler, void *handle);
BOOL win_read_eventhandler(wimp_w w, win_event_handler *p, void **handle);
void win_claim_idle_events(wimp_w);
typedef BOOL (*win_unknown_event_processor)(wimp_eventstr*, void *handle);
void win_add_unknown_event_processor(win_unknown_event_processor,
 void *handle) ;
void win_remove_unknown_event_processor(win_unknown_event_processor,
 void *handle) ;
wimp_w win_idle_event_claimer(void);
void win_claim_unknown_events(wimp_w);
wimp_w win_unknown_event_claimer(void);
void win_setmenuh(wimp_w, void *handle);
void *win_getmenuh(wimp_w); 
BOOL win_processevent(wimp_eventstr*);
void win_activeinc(void);
void win_activedec(void);
int win_activeno(void);
void win_give_away_caret(void);
void win_settitle(wimp_w w, char *newtitle);
BOOL win_init(void);
#endif
