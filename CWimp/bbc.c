/* -> c.bbc */

/* Title  : bbc.c
 * Purpose: provides bbc-style graphics and mouse/keyboard control
 * Version: 0.1
 *          0.2 RCM all bbc_xxxx routines now return ->os_error
 *          0.3 SKS tidied up. Fixed bbc_soundoff AGAIN (someone had UNfixed it)
 *                  fixed bbc_adval to be useful.
 *          0.4 SKS fixed bbc_vpos (always returned 0)
*/

#include "include.h"

#include "bbc.h"


#define  XOS_Write0           0x00000002 | os_X
#define  XOS_Plot             0x00000045 | os_X


/* Set screen mode. */
os_error *bbc_mode(int n)
{
   os_error *e = bbc_vdu(bbc_ScreenMode);
   if(!e) e = bbc_vdu(n);
   return(e);
}


/* Physical to logical colour definition. Logical colour, Physical colour, 
                                          Red level, Green level, Blue level.*/
os_error *bbc_palette(int l, int p, int r, int g, int b)
{
   os_error *e = bbc_vdu(bbc_DefLogical);
   if (!e) e = bbc_vdu(l);
   if (!e) e = bbc_vdu(p);
   if (!e) e = bbc_vdu(r);
   if (!e) e = bbc_vdu(g);
   if (!e) e = bbc_vdu(b);
   return(e);
}


/* Set white component of a colour in 256 colour modes. */
os_error *bbc_tint(int type, int value)
{
   os_error *e = bbc_vdu(bbc_MultiPurpose);
   if (!e) e = bbc_vdu(17);
   if (!e) e = bbc_vdu(type & 3);
   if (!e) e = bbc_vdu((value << 6) & 0x0C0);
   if (!e) e = bbc_vduw(0);
   if (!e) e = bbc_vduw(0);
   if (!e) e = bbc_vduw(0);
   return(e);
}


/* Find the logical colour of a pixel at indicated coordinates. x, y. */
int bbc_point(int x, int y)
{
   int block[3];

   block[0] = x;
   block[1] = y;
   block[2] = 0;             /* to zero high byte */
   x = (int)os_word(9,block);
   return block[2];
}

 
int bbc_modevar(int mode, int varno)
{
   int result;
   (void) os_swi3r(os_X | 0x35, mode, varno, 0, 0, 0, &result);
   return result;
}


int bbc_vduvar(int varno)
{
   int vars[2];
   int result;
   vars[0] = varno;
   vars[1] = -1; /* terminator. */
   (void) os_swi2(os_X | 0x31, (int) &vars[0], (int) &result);
   return result;
}


os_error *bbc_vduvars(int *vars, int *values)
{
   return(os_swi2(os_X | 0x31, (int) vars, (int) values));
}


/* Array of length of sequence for vdu codes. */ 
static char Qlen[32] =
{ 1,   /* VDU 0 */
  2,   /* next character to printer only */
  1,   /* printer on */
  1,   /* printer off */
  1,   /* print at text cursor */
  1,   /* print at graphics cursor */
  1,   /* enable VDU driver */
  1,   /* beep */
  1,   /* backspace */
  1,   /* forward space (horizontal tab) */
  1,   /* line feed */
  1,   /* up a line */
  1,   /* clear (text) screen */
  1,   /* carriage return */
  1,   /* page mode on */
  1,   /* page mode off */
  1,   /* clear graphics window */
  2,   /* define text colour */
  3,   /* define graphics colour */
  6,   /* define logical colour */
  1,   /* restore default palette */
  1,   /* disable VDU drivers */
  2,   /* Select screen mode */
  10,  /* VDU 23,.. */
  9,   /* set graphics window */
  6,   /* PLOT ... */
  1,   /* restore default windows */
  1,   /* ESCAPE char - no effect */
  5,   /* define text window */
  5,   /* define graphics origin */
  1,   /* home cursor */
  3    /* tab cursor */
/* and all the rest are 1 */
};

/*
 * Send the appropiate number of characters to vdu()
 * It is assumed that the correct number of arguments have been supplied
 */

/* Multiple character VDU call. */
extern os_error *
bbc_vduq(int c, ...)
{
    os_error *e;
    va_list ap;
    int n;

    e = bbc_vdu(c);

    if((c >= ' ')  ||  (e != 0))
        return(e);

    va_start(ap, c);
    n = Qlen[c];

    while((--n)  &&  (!e))
        e = bbc_vdu(va_arg(ap, int));

#ifndef VA_END_SUPERFLUOUS
    va_end(ap);
#endif

    return(e);
}


/* ---------- Graphics ----------- */

/* Clear graphics window. */
os_error *bbc_clg(void)
{
   return(bbc_vdu(bbc_ClearGraph));
}


/* Set up graphics window. */
extern os_error *
bbc_gwindow(int a, int b, int c, int d)
{
    os_error *e = bbc_vdu(bbc_DefGraphWindow);
    if(!e) e = bbc_vduw(a);
    if(!e) e = bbc_vduw(b);
    if(!e) e = bbc_vduw(c);
    if(!e) e = bbc_vduw(d);
    return(e);
}


/* Move the graphics origin to the given absolute coordinates. */
os_error *bbc_origin(int x, int y)
{
   os_error *e = bbc_vdu(bbc_DefGraphOrigin);
   if (!e) e = bbc_vduw(x);
   if (!e) e = bbc_vduw(y);
   return(e);
}


/* Set graphics foreground/background colour and action. */
os_error *bbc_gcol(int a, int b)
{
   os_error *e = bbc_vdu(bbc_DefGraphColour);
   if (!e) e = bbc_vdu(a);
   if (!e) e = bbc_vdu(b);
   return(e);
}


/* Perform an operating system plot operation. Plot number, x, y. */
os_error *bbc_plot(int n, int x, int y)
{
   return(os_swi3(XOS_Plot, n, x, y));
}


/* Move graphics cursor to an absolute position. */
os_error *bbc_move(int x, int y)
{ 
   return(bbc_plot(bbc_SolidBoth + bbc_BMoveAbs, x, y));
}


/* Move the graphics cursor to a position relative to its current position. */
os_error *bbc_moveby(int x, int y)
{
   return(bbc_plot(bbc_SolidBoth, x, y));
}


/* Draw a line to absolute coordinates from the current graphics position. */
os_error *bbc_draw(int x, int y)
{
   return(bbc_plot(bbc_SolidBoth + bbc_DrawAbsFore, x, y));
}


/* Draw a line to coordinates specified relative to current graphic cursor. */
os_error *bbc_drawby(int x, int y)
{
   return(bbc_plot(bbc_SolidBoth + bbc_DrawRelFore, x, y));
}


/* Plot a rectangular outline. Left X, bottom Y, Width, Height. */
os_error *bbc_rectangle(int x, int y, int w, int h)
{
   os_error *e = bbc_move(x, y);
   if (!e) e = bbc_plot(bbc_SolidExFinal + bbc_DrawRelFore,  0,  h);
   if (!e) e = bbc_plot(bbc_SolidExFinal + bbc_DrawRelFore,  w,  0);
   if (!e) e = bbc_plot(bbc_SolidExFinal + bbc_DrawRelFore,  0, -h);
   if (!e) e = bbc_plot(bbc_SolidExFinal + bbc_DrawRelFore, -w,  0);
   return(e);
}


/* Plot a solid rectangle. Left X, bottom Y, Width, Height. */
os_error *bbc_rectanglefill(int x, int y, int w, int h)
{
   os_error *e = bbc_move(x, y);
   if (!e) e = bbc_plot(bbc_RectangleFill + bbc_DrawRelFore, w, h);
   return(e);
}


/* Draw a circle outline at absolute coordinates: x, y, radius. */
os_error *bbc_circle(int x, int y, int r)
{
   os_error *e = bbc_move(x, y);
   if (!e) e = bbc_plot(bbc_Circle + bbc_DrawAbsFore, x + r, y);
   return(e);
}


/* Draw a solid circle at absolute coordinates: x, y, radius. */
os_error *bbc_circlefill(int x, int y, int r)
{
   os_error *e = bbc_move(x, y);
   if (!e) e = bbc_plot(bbc_CircleFill + bbc_DrawAbsFore, x + r, y);
   return(e);
}


/* Flood-fill an area from absolute coordinates x, y. */
os_error *bbc_fill(int x, int y)
{
   return(bbc_plot(bbc_FloodToBack + bbc_DrawAbsFore, x, y));
}


/* --------- Text ----------- */

/* Clear text window. */
os_error *bbc_cls(void)
{
   return(bbc_vdu(bbc_ClearText));
}


/* Set text foreground/background colour. */
os_error *bbc_colour(int c)
{
   os_error *e = bbc_vdu(bbc_DefTextColour);
   if (!e) e = bbc_vdu(c);
   return(e);
}


/* Return the X coordinate of the text cursor. */
int bbc_pos(void)
{ 
  int n, dmy;
  os_byte(0x86, &n, &dmy);
  return n & 0xFF;
}


/* Return Y coordinate of text cursor. */
int bbc_vpos(void)
{
   int n, dmy;
   os_byte(0x86, &dmy, &n);
   return n & 0xFF;
}


/* Alter text cursor appearance. Argument value 0 to 3. */
os_error *bbc_cursor(int c)
{
   return(bbc_vduq(bbc_MultiPurpose, 1, c, 0, 0, 0, 0, 0, 0, 0));
}


/* Position text cursor. */
os_error *bbc_tab(int x, int y)
{
   os_error *e = bbc_vdu(bbc_MoveText);
   if (!e) e = bbc_vdu(x);
   if (!e) e = bbc_vdu(y); 
   return(e);
}


/* Print a null-terminated string to the screen. */
os_error *bbc_stringprint(char *s)
{
  return(os_swi1(XOS_Write0, (int) s));
}


/* ------ Miscellaneous ------ */

/* Return a random number. */
unsigned bbc_rnd(unsigned);


/* Read data from ADC conversion or give buffer data. */
int bbc_adval(int x)
{
   if ((x & 0xFF) < 0x80)
   { /* ADC conversion info */
      int y;
      os_error *e = os_byte(0x80, &x, &y);
      if (e) return 0x80000000; /* Panic, I suppose */
      return ((y << 24) | (x << 16)) >> 16; /* SignExtend result */
   }
   else
   { /* Read buffer status */
      int dmy;
      os_byte(0x80, &x, &dmy);
      return x;
   };
}


/* Return a character code from an input stream or the keyboard. */
int bbc_inkey(int n)
{
  int x = n & 0xFF;
  int y = (n & 0xFF00) / 256;
  os_byte(0x81, &x, &y);
  if (y == 0xFF) return -1;
  return n;
}


/* end of c.bbc */
