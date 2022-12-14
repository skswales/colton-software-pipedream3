/* coords.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __coords_h
#define __coords_h
#ifndef __wimp_h
#include "wimp.h"
#endif
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef struct
{
 wimp_box box;
 int scx, scy;
}coords_cvtstr;
typedef struct
{
 int x, y;
}coords_pointstr;
 
int coords_x_toscreen(int x, coords_cvtstr *r);
int coords_y_toscreen(int y, coords_cvtstr *r);
int coords_x_toworkarea(int x, coords_cvtstr *r);
int coords_y_toworkarea(int y, coords_cvtstr *r);
void coords_box_toscreen(wimp_box *b, coords_cvtstr *r);
void coords_box_toworkarea(wimp_box *b, coords_cvtstr *r);
void coords_point_toscreen(coords_pointstr *point, coords_cvtstr *r);
void coords_point_toworkarea(coords_pointstr *point, coords_cvtstr *r);
BOOL coords_withinbox(coords_pointstr *point, wimp_box *box);
void coords_offsetbox(wimp_box *source, int byx, int byy, wimp_box *result);
BOOL coords_intersects(wimp_box *line, wimp_box *rect, int widen);
BOOL coords_boxesoverlap(wimp_box *box1, wimp_box *box2);
#endif
