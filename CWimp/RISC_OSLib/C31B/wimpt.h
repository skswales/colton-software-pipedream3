/* wimpt.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __wimpt_h
#define __wimpt_h
#ifndef __wimp_h
#include "wimp.h"
#endif
#ifndef __os_h
#include "os.h"
#endif
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
os_error * wimpt_poll(wimp_emask mask, wimp_eventstr *result);
void wimpt_fake_event(wimp_eventstr *);
wimp_eventstr *wimpt_last_event(void);
int wimpt_last_event_was_a_key(void);
void wimpt_noerr(os_error *e);
os_error *wimpt_complain(os_error *e);
BOOL wimpt_checkmode(void);
int wimpt_mode(void);
 
int wimpt_dx(void); 
int wimpt_dy(void); 
int wimpt_bpp(void); 
 
int wimpt_init(char *programname);
 
void wimpt_wimpversion(int version);
char *wimpt_programname(void);
void wimpt_reporterror(os_error*, wimp_errflags);
wimp_t wimpt_task(void);
void wimpt_forceredraw(void);
#endif
