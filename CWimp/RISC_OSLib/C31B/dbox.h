/* dbox.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __dbox_h
#define __dbox_h
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef struct dbox__str *dbox;
dbox dbox_new(char *name);
void dbox_dispose(dbox*);
 
void dbox_show(dbox);
void dbox_showstatic(dbox);
void dbox_hide(dbox);
 
typedef int dbox_field; 
typedef enum {
 dbox_FACTION, dbox_FOUTPUT, dbox_FINPUT, dbox_FONOFF
}dbox_fieldtype;
void dbox_setfield(dbox, dbox_field, char*);
void dbox_getfield(dbox, dbox_field, char *buffer, int size);
void dbox_setnumeric(dbox, dbox_field, int);
int dbox_getnumeric(dbox, dbox_field);
void dbox_fadefield(dbox d, dbox_field f);
void dbox_unfadefield(dbox d, dbox_field f);
#define dbox_CLOSE ((dbox_field) -1)
 
dbox_field dbox_get(dbox d);
 
dbox_field dbox_read(dbox d);
typedef void (*dbox_handler_proc)(dbox, void *handle);
void dbox_eventhandler(dbox, dbox_handler_proc, void* handle);
 
typedef BOOL (*dbox_raw_handler_proc)(dbox, void *event, void *handle);
void dbox_raw_eventhandler(dbox, dbox_raw_handler_proc, void *handle);
dbox_field dbox_fillin(dbox d);
dbox_field dbox_fillin_fixedcaret(dbox d);
dbox_field dbox_popup(char *name, char *message);
BOOL dbox_persist(void);
int dbox_syshandle(dbox);
void dbox_init(void);
#endif
