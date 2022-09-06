/****************************************************************************
 * This source file was written by Acorn Computers Limited. It is part of   *
 * the "cwimp" library for writing applications in C for RISC OS. It may be *
 * used freely in the creation of programs for Archimedes. It should be     *
 * used with Acorn's C Compiler Release 2 or later.                         *
 *                                                                          *
 * No support can be given to programmers using this code and, while we     *
 * believe that it is correct, no correspondence can be entered into        *
 * concerning behaviour or bugs.                                            *
 *                                                                          *
 * Upgrades of this code may or may not appear, and while every effort will *
 * be made to keep such upgrades upwards compatible, no guarantees can be   *
 * given.                                                                   *
 ***************************************************************************/

/*
 * Title  : c.sprite
 * Purpose: provide access to RISC OS sprite facilities
 * Version: 0.1
 */

#include <stddef.h>
#include "h.os"
#include "h.sprite"


/* Basic primitive used by sprite_xxx calls */

#define OS_SpriteOp 0x2E

static os_error *sprite__op(os_regset *r)
{
  return(os_swix(OS_SpriteOp, r));
}


/******** Simple operations, use no sprite area, no name/sprite pointer ***/


os_error * sprite_screensave(const char *filename, sprite_palflag palflag)
{
  os_regset r;
  r.r[0] = 2;
/*r.r[1] unused */  
  r.r[2] = (int) filename;
  r.r[3] = palflag;
  return(sprite__op(&r));
}


os_error * sprite_screenload(const char *filename)
{
  os_regset r;
  r.r[0] = 3;
/*r.r[1] unused */  
  r.r[2] = (int) filename;
  return(sprite__op(&r));
}


/****** Operations on either system/user area, no name/sprite pointer *****/

static void sprite__setfromarea(int op, sprite_area *area, os_regset *r)
{
  if (area == sprite_mainarea)
  {
    r->r[0] = op;
/*  r->r[1] unused */
  }
  else
  {
    r->r[0] = op + 256;
    r->r[1] = (int) area;
  }
}


void sprite_area_initialise(sprite_area *area, int length)
{
  area->size    = length; /* No SpriteOp to do this ! */
  area->number  = 0;
  area->sproff  = 16;
  area->freeoff = 16;
}


os_error * sprite_area_readinfo(sprite_area *area, sprite_area *resultarea)
{
  os_regset r;
  os_error *result;
  sprite__setfromarea(8, area, &r);
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    resultarea->size    = r.r[2];
    resultarea->number  = r.r[3];
    resultarea->sproff  = r.r[4];
    resultarea->freeoff = r.r[5];
  }
  return(result);
}


os_error * sprite_area_reinit(sprite_area *area)
{
  os_regset r;
  sprite__setfromarea(9, area, &r);
  return(sprite__op(&r));
}


os_error * sprite_area_save(sprite_area *area, const char *filename)
{
  os_regset r;
  sprite__setfromarea(12, area, &r);
  r.r[2] = (int) filename;
  return(sprite__op(&r));
}


os_error * sprite_area_load(sprite_area *area, const char *filename)
{
  os_regset r;
  sprite__setfromarea(10, area, &r);
  r.r[2] = (int) filename;
  return(sprite__op(&r));
}


os_error * sprite_area_merge(sprite_area *area, const char *filename)
{
  os_regset r;
  sprite__setfromarea(11, area, &r);
  r.r[2] = (int) filename;
  return(sprite__op(&r));
}


os_error * sprite_getname(sprite_area *area, void *buffer,
                          int *length, int index)
{
  os_regset r;
  os_error *result;
  sprite__setfromarea(13, area, &r);
  r.r[2] = (int) buffer;
  r.r[3] = *length;
  r.r[4] = index;
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    *length = r.r[3];
  }
  return(result);
}


os_error * sprite_get(sprite_area *area, const char *name,
                      sprite_palflag palflag)
{
  os_regset r;
  sprite__setfromarea(14, area, &r);
  r.r[2] = (int) name;
  r.r[3] = palflag;
  return(sprite__op(&r));
}


os_error * sprite_get_rp(sprite_area *area, const char *name,
                         sprite_palflag palflag, sprite_ptr *resultaddress)
{
  os_regset r;
  os_error *result;
  sprite__setfromarea(14, area, &r);
  r.r[2] = (int) name;
  r.r[3] = palflag;
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    *resultaddress = (void *) r.r[2];
  }
  return(result);
}


os_error * sprite_get_given(sprite_area *area, const char *name,
                            sprite_palflag palflag,
                            int x0, int y0, int x1, int y1)
{
  os_regset r;
  sprite__setfromarea(16, area, &r);
  r.r[2] = (int) name;
  r.r[3] = palflag;
  r.r[4] = x0;
  r.r[5] = y0;
  r.r[6] = x1;
  r.r[7] = y1;
  return(sprite__op(&r));
}


os_error * sprite_get_given_rp(sprite_area *area, const char *name,
                               sprite_palflag palflag, int x0, int y0,
                               int x1, int y1, sprite_ptr *resultaddress)
{
  os_regset r;
  os_error *result;
  sprite__setfromarea(16, area, &r);
  r.r[2] = (int) name;
  r.r[3] = palflag;
  r.r[4] = x0;
  r.r[5] = y0;
  r.r[6] = x1;
  r.r[7] = y1;
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    *resultaddress = (void *) r.r[2];
  }
  return(result);
}


os_error * sprite_create(sprite_area *area, const char *name,
                         sprite_palflag palflag,
                         int width, int height, int mode)
{
  os_regset r;
  sprite__setfromarea(15, area, &r); /* NB. Not all done in numeric order !! */
  r.r[2] = (int) name;
  r.r[3] = palflag;
  r.r[4] = width;
  r.r[5] = height;
  r.r[6] = mode;
  return(sprite__op(&r));
}


os_error * sprite_create_rp(sprite_area *area, const char *name,
                            sprite_palflag palflag,
                            int width, int height, int mode,
                            sprite_ptr *resultaddress)
{
  os_regset r;
  os_error *result;
  sprite__setfromarea(15, area, &r); /* NB. Not all done in numeric order !! */
  r.r[2] = (int) name;
  r.r[3] = palflag;
  r.r[4] = width;
  r.r[5] = height;
  r.r[6] = mode;
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    *resultaddress = (void *) r.r[2];
  }
  return(result);
}


/*********** Operations on system/user area, name/sprite pointer **********/

/* Modify op if using sprite address is address, not name */
/* But only if using own sprite area */

static void sprite__setfromtag(int op, sprite_area *area,
                               const sprite_id *spr, os_regset *r)
{
  if (area == sprite_mainarea)
  {
    r->r[0] = op;
 /* r->r[1] unused */
  }
  else
  {
    r->r[1] = (int) area;
    if ((spr->tag) == sprite_id_addr)
    {
      r->r[0] = 512 + op;
      r->r[2] = (int) (spr->s.addr);
    }
    else
    {
      r->r[0] = 256 + op;
      r->r[2] = (int) (spr->s.name);
    }
  }
}


os_error * sprite_readinfo(sprite_area *area, const sprite_id *spr,
                           sprite_info *resultinfo)
{
  os_regset r;
  os_error *result;
  sprite__setfromtag(40, area, spr, &r);
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    resultinfo->width  = r.r[3];
    resultinfo->height = r.r[4];
    resultinfo->mask   = r.r[5];
    resultinfo->mode   = r.r[6];
  }
  return(result);
}


os_error * sprite_select(sprite_area *area, const sprite_id *spr)
{
  os_regset r;
  sprite__setfromtag(24, area, spr, &r);
  return(sprite__op(&r));
}


os_error * sprite_select_rp(sprite_area *area, const sprite_id *spr,
                            sprite_ptr *resultaddress)
{
  os_regset r;
  os_error *result;
  sprite__setfromtag(24, area, spr, &r);
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    *resultaddress = (void *) r.r[2];
  }
  return(result);
}


os_error * sprite_delete(sprite_area *area, const sprite_id *spr)
{
  os_regset r;
  sprite__setfromtag(25, area, spr, &r);
  return(sprite__op(&r));
}


os_error * sprite_rename(sprite_area *area, const sprite_id *spr,
                         const char *newname)
{
  os_regset r;
  sprite__setfromtag(26, area, spr, &r);
  r.r[3] = (int) newname;
  return(sprite__op(&r));
}


os_error * sprite_copy(sprite_area *area, const sprite_id *spr,
                       const char *copyname)
{
  os_regset r;
  sprite__setfromtag(27, area, spr, &r);
  r.r[3] = (int) copyname;
  return(sprite__op(&r));
}


os_error * sprite_put(sprite_area *area, const sprite_id *spr, int gcol_action)
{
  os_regset r;
  sprite__setfromtag(28, area, spr, &r);
  r.r[5] = gcol_action;
  return(sprite__op(&r));
}


os_error * sprite_put_given(sprite_area *area, const sprite_id *spr, int gcol_action,
                            int x, int y)
{
  os_regset r;
  sprite__setfromtag(34, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  r.r[5] = gcol_action;
  return(sprite__op(&r));
}


os_error * sprite_put_scaled(sprite_area *area, const sprite_id *spr,
                             int gcol_action,
                             int x, int y,
                             sprite_factors *factors,
                             sprite_pixtrans *pixtrans)
{
  os_regset r;
  sprite__setfromtag(52, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  r.r[5] = gcol_action;
  r.r[6] = (int) factors;
  r.r[7] = (int) pixtrans;
  return(sprite__op(&r));
}


os_error * sprite_put_greyscaled(sprite_area *area, const sprite_id *spr,
                                 int x, int y,
                                 sprite_factors *factors,
                                 sprite_pixtrans *pixtrans)
{
  os_regset r;
  sprite__setfromtag(53, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  r.r[5] = 0;                   /* doesn't support mask or gcol action */
  r.r[6] = (int) factors;
  r.r[7] = (int) pixtrans;
  return(sprite__op(&r));
}


os_error * sprite_put_mask(sprite_area *area, const sprite_id *spr)
{
  os_regset r;
  sprite__setfromtag(48, area, spr, &r);
  return(sprite__op(&r));
}


os_error * sprite_put_mask_given(sprite_area *area, const sprite_id *spr,
                                 int x, int y)
{
  os_regset r;
  sprite__setfromtag(49, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  return(sprite__op(&r));
}


os_error * sprite_put_mask_scaled(sprite_area *area, const sprite_id *spr,
                                  int x, int y,
                                  sprite_factors *factors)
{
  os_regset r;
  sprite__setfromtag(50, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  r.r[6] = (int) factors;
  return(sprite__op(&r));
}


os_error * sprite_put_char_scaled(char ch,
                                  int x, int y,
                                  sprite_factors *factors)
{
  os_regset r;
  r.r[0] = 51;
  r.r[1] = ch;
  r.r[3] = x;
  r.r[4] = y;
  r.r[6] = (int) factors;
  return(sprite__op(&r));
}


os_error * sprite_create_mask(sprite_area *area, const sprite_id *spr)
{
  os_regset r;
  sprite__setfromtag(29, area, spr, &r);
  return(sprite__op(&r));
}


os_error * sprite_remove_mask(sprite_area *area, const sprite_id *spr)
{
  os_regset r;
  sprite__setfromtag(30, area, spr, &r);
  return(sprite__op(&r));
}


os_error * sprite_insert_row(sprite_area *area, const sprite_id *spr, int row)
{
  os_regset r;
  sprite__setfromtag(31, area, spr, &r);
  r.r[3] = row;
  return(sprite__op(&r));
}


os_error * sprite_delete_row(sprite_area *area, const sprite_id *spr, int row)
{
  os_regset r;
  sprite__setfromtag(32, area, spr, &r);
  r.r[3] = row;
  return(sprite__op(&r));
}


os_error * sprite_insert_column(sprite_area *area, const sprite_id *spr, int column)
{
  os_regset r;
  sprite__setfromtag(45, area, spr, &r);
  r.r[3] = column;
  return(sprite__op(&r));
}


os_error * sprite_delete_column(sprite_area *area, const sprite_id *spr, int column)
{
  os_regset r;
  sprite__setfromtag(46, area, spr, &r);
  r.r[3] = column;
  return(sprite__op(&r));
}


os_error * sprite_flip_x(sprite_area *area, const sprite_id *spr)
{
  os_regset r;
  sprite__setfromtag(33, area, spr, &r);
  return(sprite__op(&r));
}


os_error * sprite_flip_y(sprite_area *area, const sprite_id *spr)
{
  os_regset r;
  sprite__setfromtag(47, area, spr, &r);
  return(sprite__op(&r));
}

os_error * sprite_readsize(sprite_area *area, const sprite_id *spr,
                           sprite_info *resultinfo)
{
  os_regset r;
  os_error *result;
  sprite__setfromtag(40, area, spr, &r);
  result = sprite__op(&r);
  if(result == NULL)
    {
/* now copy returned data */
    resultinfo->width = r.r[3] ;
    resultinfo->height = r.r[4] ;
    resultinfo->mask = r.r[5] ;
    resultinfo->mode = r.r[6] ;
    }
  return(result);
}

os_error * sprite_readpixel(sprite_area *area, const sprite_id *spr, int x, int y,
                            sprite_colour *resultcolour)
{
  os_regset r;
  os_error *result;
  sprite__setfromtag(41, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    resultcolour->colour = r.r[5];
    resultcolour->tint   = r.r[6];
  }
  return(result);
}


os_error * sprite_writepixel(sprite_area *area, const sprite_id *spr, int x, int y,
                            sprite_colour *colour)
{
  os_regset r;
  sprite__setfromtag(42, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  r.r[5] = colour->colour;
  r.r[6] = colour->tint;
  return(sprite__op(&r));
}


os_error * sprite_readmask(sprite_area *area, const sprite_id *spr, int x, int y,
                           sprite_maskstate *resultmaskstate)
{
  os_regset r;
  os_error *result;
  sprite__setfromtag(43, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  result = sprite__op(&r);
  if (result == NULL) /* Only return result if no error */
  {
    *resultmaskstate = r.r[5];
  }
  return(result);
}


os_error * sprite_writemask(sprite_area *area, const sprite_id *spr, int x, int y,
                            sprite_maskstate *maskstate)
{
  os_regset r;
  sprite__setfromtag(44, area, spr, &r);
  r.r[3] = x;
  r.r[4] = y;
  r.r[5] = (int) (*maskstate); /* Use pointer here for consistent interface */
  return(sprite__op(&r));
}


/* end of c.sprite */
