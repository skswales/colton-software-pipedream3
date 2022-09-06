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
 * Title  : h.sprite
 * Purpose: provide access to RISC OS sprite facilities
 * Version: 0.1
 */

#ifndef __sprite_h
#define __sprite_h

/******** Simple operations, use no sprite area, no name/sprite pointer ***/

typedef enum
{
  sprite_nopalette  = 0,
  sprite_haspalette = 1
} sprite_palflag;

typedef struct
{
  int xmag,ymag,xdiv,ydiv;
} sprite_factors;

typedef struct
{
  char pixel[1];     /* length depends on number of input colours */
} sprite_pixtrans;

os_error * sprite_screensave(const char *filename, sprite_palflag);
/* Save the current graphics window as a sprite file, with optional palette.
 * Equivalent to *ScreenSave.
 */

os_error * sprite_screenload(const char *filename);
/* Load a sprite file onto the screen. Equivalent to *ScreenLoad. */


/****** Operations on either system/user area, no name/sprite pointer *****/

typedef struct /* Format of a sprite area control block */
{
  int size;
  int number;
  int sproff;
  int freeoff;
} sprite_area;

typedef struct /* Format of a sprite header */
{
  int  next;      /*  Offset to next sprite                */
  char name[12];  /*  Sprite name                          */
  int  width;     /*  Width in words-1      (0..639)       */
  int  height;    /*  Height in scanlines-1 (0..255/511)   */
  int  lbit;      /*  First bit used (left end of row)     */
  int  rbit;      /*  Last bit used (right end of row)     */
  int  image;     /*  Offset to sprite image               */
  int  mask;      /*  Offset to transparancy mask          */
  int  mode;      /*  Mode sprite was defined in           */
} sprite_header;

#define sprite_mainarea ((sprite_area *) 0)

typedef void * sprite_ptr;

void sprite_area_initialise(sprite_area *, int size);
/* Initialise an area of memory as a sprite area */

os_error * sprite_area_readinfo(sprite_area *, sprite_area *resultarea);
/* Read information from a sprite area control block */

os_error * sprite_area_reinit(sprite_area *);
/* Reinitialise a sprite area.
 * If system area, then equivalent to *SNew
 */

os_error * sprite_area_load(sprite_area *, const char *filename);
/* Load a sprite file into a sprite area.
 * If system area, then equivalent to *SLoad
 */

os_error * sprite_area_merge(sprite_area *, const char *filename);
/* Merge a sprite file with a sprite area.
 * If system area, then equivalent to *SMerge
 */

os_error * sprite_area_save(sprite_area *, const char *filename);
/* Saves a sprite area as a sprite file.
 * If system area, then equivalent to *SSave
 */

os_error * sprite_getname(sprite_area *, void *buffer, int *length, int index);
/* Return the name and length of the n'th sprite in a sprite area into a buffer
 */

os_error * sprite_get(sprite_area *, const char *name, sprite_palflag);
os_error * sprite_get_rp(sprite_area *, const char *name, sprite_palflag,
                         sprite_ptr *resultaddress);
/* Copy a rectangle of screen delimited by the last pair of graphics cursor
 * positions as a named sprite in a sprite area, optionally storing the palette
 * with the sprite.
 * sprite_get_rp: return the address of the sprite.
 * If system area, then equivalent to *SGet.
 */

os_error * sprite_get_given(sprite_area *, const char *name,
                            sprite_palflag,
                            int x0, int y0, int x1, int y1);
os_error * sprite_get_given_rp(sprite_area *, const char *name,
                               sprite_palflag,
                               int x0, int y0, int x1, int y1,
                               sprite_ptr *resultaddress);
/* Copy a rectangle of screen delimited by the given pair of graphics
 * coordinates as a named sprite in a sprite area, optionally storing the
 * palette with the sprite.
 * sprite_get_given_rp: return the address of the sprite.
 */

os_error * sprite_create(sprite_area *, const char *name, sprite_palflag,
                         int width, int height, int mode);
os_error * sprite_create_rp(sprite_area *, const char *name, sprite_palflag,
                            int width, int height, int mode,
                            sprite_ptr *resultaddress);
/* Create a named sprite in a sprite area of specified size and screen mode,
 * optionally reserving space for palette data with the sprite.
 * sprite_create_rp: return the address of the sprite.
 */


/*********** Operations on system/user area, name/sprite pointer **********/

typedef enum
{
  sprite_id_name = 0,
  sprite_id_addr = 0x74527053 /* 'Magic' number ("SpRt") to test against */
} sprite_type;

typedef struct
{
  union
  {
    char *     name; /* Can use either name of sprite or address (faster) */
    sprite_ptr addr;
  } s;
  sprite_type tag;   /* User must tag the use of this structure manually */
} sprite_id;


os_error * sprite_select(sprite_area *, const sprite_id *);
os_error * sprite_select_rp(sprite_area *, const sprite_id *,
                            sprite_ptr *resultaddress);
/* Select the specified sprite for plotting using plot(0xed,x,y).
 * sprite_select_rp: return the address of the sprite.
 * If system area, equivalent to *SChoose.
 */

os_error * sprite_delete(sprite_area *, const sprite_id *);
/* Delete the specified sprite.
 * If system area, equivalent to *SDelete.
 */

os_error * sprite_rename(sprite_area *, const sprite_id *,
                         const char *newname);
/* Rename the specified sprite within the same sprite area.
 * If system area, equivalent to *SRename.
 */

os_error * sprite_copy(sprite_area *, const sprite_id *,
                       const char *copyname);
/* Copy the specified sprite as another named sprite in the same sprite area.
 * If system area, equivalent to *SCopy.
 */

os_error * sprite_put(sprite_area *, const sprite_id *, int gcol);
/* Plot the specified sprite using the given GCOL action. */

os_error * sprite_put_given(sprite_area *, const sprite_id *, int gcol,
                            int x, int y);
/* Plot the specified sprite at (x,y) using the given GCOL action. */

os_error * sprite_put_scaled(sprite_area *, const sprite_id *, int gcol,
                            int x, int y,
                            sprite_factors *factors,
                            sprite_pixtrans *pixtrans);

os_error * sprite_put_greyscaled(sprite_area *, const sprite_id *,
                            int x, int y,
                            sprite_factors *factors,
                            sprite_pixtrans *pixtrans);

os_error * sprite_put_mask(sprite_area *, const sprite_id *);
/* Plot the specified sprite mask in the background colour. */

os_error * sprite_put_mask_given(sprite_area *, const sprite_id *, int x, int y);
/* Plot the specified sprite mask at (x,y) in the background colour. */

os_error * sprite_put_mask_scaled(sprite_area *, const sprite_id *,
                            int x, int y,
                            sprite_factors *factors);
/* Plot the sprite mask at (x,y) scaled, using the background colour/action */

os_error * sprite_put_char_scaled(char ch,
                                  int x, int y,
                                  sprite_factors *factors);
/* paint char scaled at (x,y) */

os_error * sprite_create_mask(sprite_area *, const sprite_id *);
/* Create a mask definition for the specified sprite. */

os_error * sprite_remove_mask(sprite_area *, const sprite_id *);
/* Remove the mask definition from the specified sprite. */

os_error * sprite_insert_row(sprite_area *, const sprite_id *, int row);
/* Insert a row into the specified sprite at the given row. */

os_error * sprite_delete_row(sprite_area *, const sprite_id *, int row);
/* Delete the given row from the specified sprite. */

os_error * sprite_insert_column(sprite_area *, const sprite_id *, int column);
/* Insert a column into the specified sprite at the given column. */

os_error * sprite_delete_column(sprite_area *, const sprite_id *, int column);
/* Delete the given column from the specified sprite. */

os_error * sprite_flip_x(sprite_area *, const sprite_id *);
/* Flip the specified sprite about the x axis */

os_error * sprite_flip_y(sprite_area *, const sprite_id *);
/* Flip the specified sprite about the y axis */

typedef struct
{
 int width;
 int height;
 int mask;
 int mode;
} sprite_info;

os_error * sprite_readsize(sprite_area *, const sprite_id *,
                           sprite_info *resultinfo);
/* Read the size information for the specified const sprite_id */

typedef struct
{
  int colour;
  int tint;
} sprite_colour;

os_error * sprite_readpixel(sprite_area *, const sprite_id *,
                            int x, int y, sprite_colour *resultcolour);
/* Read the colour of a given pixel in the specified const sprite_id */

os_error * sprite_writepixel(sprite_area *, const sprite_id *,
                             int x, int y, sprite_colour *colour);
/* Write the colour of a given pixel in the specified const sprite_id */

typedef enum
{
  sprite_masktransparent = 0,
  sprite_masksolid       = 1
} sprite_maskstate;

os_error * sprite_readmask(sprite_area *, const sprite_id *,
                           int x, int y, sprite_maskstate *resultmaskstate);
/* Read the state of a given pixel in the specified sprite mask */

os_error * sprite_writemask(sprite_area *, const sprite_id *,
                            int x, int y, sprite_maskstate *maskstate);
/* Write the state of a given pixel in the specified sprite mask */

#endif

/* end of h.sprite */
