/* txt.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __txt_h
#define __txt_h
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef struct txt1_str *txt;
txt txt_new(char *title);
void txt_show(txt t);
void txt_hide(txt t);
void txt_settitle(txt, char *title);
void txt_dispose(txt *t);
int txt_bufsize(txt);
BOOL txt_setbufsize(txt, int);
typedef enum { txt_DISPLAY = 1, 
 txt_CARET = 2, 
 txt_UPDATED = 4 
 } txt_charoption;
txt_charoption txt_charoptions(txt);
void txt_setcharoptions(txt, txt_charoption affect, txt_charoption values);
void txt_setdisplayok(txt);
typedef int txt_index; 
txt_index txt_dot(txt t);
txt_index txt_size(txt t); 
void txt_setdot(txt t, txt_index i);
void txt_movedot(txt, int by);
void txt_insertchar(txt t, char c);
void txt_insertstring(txt t, char *s);
void txt_delete(txt t, int n);
void txt_replacechars(txt t, int ntodelete, char *a, int n);
char txt_charatdot(txt t);
char txt_charat(txt t, txt_index i);
void txt_charsatdot(txt, char *buffer, int *n);
void txt_replaceatend(txt, int ntodelete, char*, int);
void txt_movevertical(txt t, int by, int caretstill);
void txt_movehorizontal(txt, int by);
int txt_visiblelinecount(txt t);
int txt_visiblecolcount(txt t);
typedef struct {int a; int b;} txt_marker;
void txt_newmarker(txt, txt_marker *mark);
void txt_movemarker(txt t, txt_marker *mark, txt_index to);
void txt_movedottomarker(txt t, txt_marker *mark);
txt_index txt_indexofmarker(txt t, txt_marker *mark);
void txt_disposemarker(txt, txt_marker*);
BOOL txt_selectset(txt t);
 
txt_index txt_selectstart(txt t);
txt_index txt_selectend(txt t);
void txt_setselect(txt, txt_index start, txt_index end);
typedef int txt_eventcode;
txt_eventcode txt_get(txt t);
 
int txt_queue(txt t);
void txt_unget(txt t, txt_eventcode code);
typedef enum {
 txt_MSELECT = 0x01000000,
 txt_MEXTEND = 0x02000000,
 txt_MSELOLD = 0x04000000,
 txt_MEXTOLD = 0x08000000,
 txt_MEXACT = 0x10000000,
 txt_EXTRACODE = 0x40000000,
 txt_MOUSECODE =~0x7fffffff
}txt_mouseeventflag; 
typedef void (*txt_event_proc)(txt,void *handle);
 
void txt_eventhandler(txt, txt_event_proc, void *handle);
 
void txt_readeventhandler(txt t, txt_event_proc *func, void **handle);
void txt_arrayseg(txt t, txt_index at, char **a , int *n );
int txt_syshandle(txt t);
void txt_init(void);
#endif
