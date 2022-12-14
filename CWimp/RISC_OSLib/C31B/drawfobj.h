/* drawfobj.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __drawfobj_h
#define __drawfobj_h
#ifdef DRAWFILE_INTERNAL
#include "h.DrawIntern.drawcvttyp"
#else
#ifndef __drawfdiag_h
#include "h.drawfdiag"
#endif
#ifndef __drawftypes_h
#include "h.drawftypes"
#endif
#endif
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef union
{
 draw_objhdr *object;
 draw_fileheader *fileHeader;
 draw_fontliststr *fontList;
 draw_textstr *text;
 draw_pathstr *path;
 draw_spristr *sprite;
 draw_groustr *group;
 draw_textareahdr *textarea;
 char *bytep;
 int *wordp;
}draw_objectType;
#define draw_NoObject (draw_object)-1
#define draw_FirstObject (draw_object)-1
#define draw_LastObject (draw_object)-2
extern int *draw_transTable;
 
void draw_create_diag(draw_diag *diag, char *creator, draw_box bbox);
BOOL draw_doObjects(draw_diag *diag, draw_object start, draw_object end,
 draw_redrawstr *r, double scale, draw_error *error);
void draw_setFontTable(draw_diag *diag);
BOOL draw_verifyObject(draw_diag *diag, draw_object object, int *size,
 draw_error *error);
 
BOOL draw_createObject(draw_diag *diag, draw_objectType newObject,
 draw_object after, BOOL rebind, draw_object *object,
 draw_error *error);
BOOL draw_deleteObjects(draw_diag *diag, draw_object start, draw_object end,
 BOOL rebind, draw_error *error);
BOOL draw_extractObject(draw_diag *diag, draw_object object,
 draw_objectType result, draw_error *error);
void draw_translateText(draw_diag *diag);
BOOL drawfobj_init(void);
#endif
