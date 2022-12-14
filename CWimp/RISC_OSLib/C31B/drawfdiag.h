/* drawfdiag.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __drawfdiag_h
#define __drawfdiag_h
#ifndef __os_h
#include "os.h"
#endif
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef struct
{char * data;
 int length;
}draw_diag;
typedef int draw_object;
typedef struct {int x0, y0, x1, y1;} draw_box;
typedef struct
{
 int reserved;
 draw_box box; 
 int scx, scy; 
 draw_box g; 
}draw_redrawstr;
typedef struct
{
 enum { DrawOSError, DrawOwnError, None } type;
 union
 {
 os_error os;
 struct { int code; int location; } draw;
 } err;
}draw_error;
#define draw_drawToScreen(i) ((i) >> 8)
#define draw_screenToDraw(i) ((i) << 8)
BOOL draw_verify_diag(draw_diag *diag, draw_error *error);
BOOL draw_append_diag(draw_diag *diag1, draw_diag *diag2, draw_error *error);
 
BOOL draw_render_diag(draw_diag *diag, draw_redrawstr *r, double scale,
 draw_error *error);
typedef int (*draw_allocate)(void **anchor, int n);
typedef int (*draw_extend)(void **anchor, int n);
typedef void (*draw_free)(void **anchor);
void draw_registerMemoryFunctions(draw_allocate alloc,
 draw_extend extend,
 draw_free free);
void draw_shift_diag(draw_diag *diag, int xMove, int yMove);
void draw_queryBox(draw_diag *diag, draw_box *box, BOOL screenUnits);
void draw_convertBox(draw_box *from, draw_box *to, BOOL toScreen);
void draw_rebind_diag(draw_diag *diag);
typedef BOOL (*draw_unknown_object_handler)(void *object, void *handle,
 draw_error *error);
draw_unknown_object_handler draw_set_unknown_object_handler
 (draw_unknown_object_handler handler, void *handle);
BOOL drawfdiag_init(void);
#endif
