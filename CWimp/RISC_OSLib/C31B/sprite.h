/* sprite.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __sprite_h
#define __sprite_h
#ifndef __os_h
#include "os.h"
#endif
typedef enum
{
 sprite_nopalette = 0,
 sprite_haspalette = 1
}sprite_palflag;
typedef struct
{
 int xmag,ymag,xdiv,ydiv;
}sprite_factors;
typedef char sprite_pixtrans; 
os_error * sprite_screensave(const char *filename, sprite_palflag);
 
os_error * sprite_screenload(const char *filename);
typedef struct 
{
 int size;
 int number;
 int sproff;
 int freeoff;
}sprite_area;
typedef struct 
{
 int next; 
 char name[12]; 
 int width; 
 int height; 
 int lbit; 
 int rbit; 
 int image; 
 int mask; 
 int mode; 
 
 
}sprite_header;
#define sprite_mainarea ((sprite_area *) 0)
typedef void * sprite_ptr;
void sprite_area_initialise(sprite_area *, int size);
os_error * sprite_area_readinfo(sprite_area *, sprite_area *resultarea);
os_error * sprite_area_reinit(sprite_area *);
os_error * sprite_area_load(sprite_area *, const char *filename);
 
os_error * sprite_area_merge(sprite_area *, const char *filename);
 
os_error * sprite_area_save(sprite_area *, const char *filename);
os_error * sprite_getname(sprite_area *, void *buffer, int *length, int index);
os_error * sprite_get(sprite_area *, char *name, sprite_palflag);
os_error * sprite_get_rp(sprite_area *, char *name, sprite_palflag,
 sprite_ptr *resultaddress);
os_error * sprite_get_given(sprite_area *, char *name, sprite_palflag,
 int x0, int y0, int x1, int y1);
os_error * sprite_get_given_rp(sprite_area *, char *name, sprite_palflag,
 int x0, int y0, int x1, int y1,
 sprite_ptr *resultaddress);
os_error * sprite_create(sprite_area *, char *name, sprite_palflag,
 int width, int height, int mode);
os_error * sprite_create_rp(sprite_area *, char *name, sprite_palflag,
 int width, int height, int mode,
 sprite_ptr *resultaddress);
typedef enum
{
 sprite_id_name = 0,
 sprite_id_addr = 0x74527053 
}sprite_type;
typedef struct
{
 union
 {
 char * name; 
 sprite_ptr addr;
 } s;
 sprite_type tag; 
}sprite_id;
os_error * sprite_select(sprite_area *, sprite_id *);
 
os_error * sprite_select_rp(sprite_area *, sprite_id *,
 sprite_ptr *resultaddress);
os_error * sprite_delete(sprite_area *, sprite_id *);
os_error * sprite_rename(sprite_area *, sprite_id *, char *newname);
os_error * sprite_copy(sprite_area *, sprite_id *, char *copyname);
os_error * sprite_put(sprite_area *, sprite_id *, int gcol);
os_error * sprite_put_given(sprite_area *, sprite_id *, int gcol,
 int x, int y);
os_error * sprite_put_scaled(sprite_area *, sprite_id *, int gcol,
 int x, int y,
 sprite_factors *factors,
 sprite_pixtrans pixtrans[]);
os_error * sprite_put_greyscaled(sprite_area *, sprite_id *,
 int x, int y,
 sprite_factors *factors,
 sprite_pixtrans pixtrans[]);
os_error * sprite_put_mask(sprite_area *, sprite_id *);
os_error * sprite_put_mask_given(sprite_area *, sprite_id *, int x, int y);
os_error * sprite_put_mask_scaled(sprite_area *, sprite_id *,
 int x, int y,
 sprite_factors *factors);
os_error * sprite_put_char_scaled(char ch,
 int x, int y,
 sprite_factors *factors);
os_error * sprite_create_mask(sprite_area *, sprite_id *);
os_error * sprite_remove_mask(sprite_area *, sprite_id *);
os_error * sprite_insert_row(sprite_area *, sprite_id *, int row);
os_error * sprite_delete_row(sprite_area *, sprite_id *, int row);
os_error * sprite_insert_column(sprite_area *, sprite_id *, int column);
os_error * sprite_delete_column(sprite_area *, sprite_id *, int column);
os_error * sprite_flip_x(sprite_area *, sprite_id *);
os_error * sprite_flip_y(sprite_area *, sprite_id *);
typedef struct
{
 int width;
 int height;
 int mask;
 int mode;
}sprite_info;
os_error * sprite_readsize(sprite_area *, sprite_id *,
 sprite_info *resultinfo);
typedef struct
{
 int colour;
 int tint;
}sprite_colour;
os_error * sprite_readpixel(sprite_area *, sprite_id *,
 int x, int y, sprite_colour *resultcolour);
os_error * sprite_writepixel(sprite_area *, sprite_id *,
 int x, int y, sprite_colour *colour);
typedef enum
{
 sprite_masktransparent = 0,
 sprite_masksolid = 1
}sprite_maskstate;
os_error * sprite_readmask(sprite_area *, sprite_id *,
 int x, int y, sprite_maskstate *resultmaskstate);
os_error * sprite_writemask(sprite_area *, sprite_id *,
 int x, int y, sprite_maskstate *maskstate);
typedef struct
 { int r[4];
 } sprite_state;
os_error *sprite_restorestate(sprite_state state);
os_error *sprite_outputtosprite(sprite_area *area, sprite_id *id,
 int *save_area, sprite_state *state);
os_error *sprite_outputtomask(sprite_area *area, sprite_id *id,
 int *save_area, sprite_state *state);
os_error *sprite_outputtoscreen(int *save_area, sprite_state *state);
os_error *sprite_sizeof_spritecontext(sprite_area *area, sprite_id *id,
 int *size);
os_error *sprite_sizeof_screencontext(int *size);
os_error *sprite_removewastage(sprite_area *area, sprite_id *id);
 
#endif
