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
 * Title  : c.font
 * Purpose: access to RISC OS font facilities
 * Version: 0.1
 */

#include "h.os"
#include "h.font"


/*                          F O N T    S W I 's                          */
#define CacheAddr           0x00040080
#define FindFont            0x00040081
#define LoseFont            0x00040082
#define ReadDefn            0x00040083
#define ReadInfo            0x00040084
#define StringWidth         0x00040085
#define Paint               0x00040086
#define Caret               0x00040087
#define ConverttoOS         0x00040088
#define Converttopoints     0x00040089
#define SetFont             0x0004008A
#define CurrentFont         0x0004008B
#define FutureFont          0x0004008C
#define FindCaret           0x0004008D
#define CharBBox            0x0004008E
#define ReadScaleFactor     0x0004008F
#define SetScaleFactor      0x00040090
#define ListFonts           0x00040091
#define SetFontColours      0x00040092
#define SetPalette          0x00040093
#define ReadThresholds      0x00040094
#define SetThresholds       0x00040095
#define FindCaretJ          0x00040096
#define StringBBox          0x00040097
#define ReadColourTable     0x00040098




os_error *font_cacheaddress(version, cacheused, cachesize)

int *version, *cacheused, *cachesize;

{
os_regset r;
os_error *e;

    r.r[0] = 0;
    e = os_swix(CacheAddr, &r);

    if (!e)

      {
      *version = r.r[0];
      *cacheused = r.r[1];
      *cachesize = r.r[2];
      }

return e;

}

os_error *font_find(name, xsize, ysize, xres, yres, fontadd)

char *name;
int xsize, ysize; /* in 16ths of a point */
int xres, yres;   /* dots per inch of res: 0->use default */
font *fontadd;

{

os_regset r;
os_error *e;

r.r[1] = (int)name;
r.r[2] = xsize;
r.r[3] = ysize;
r.r[4] = xres;
r.r[5] = yres;

e = os_swix(FindFont, &r);

*fontadd = (font) r.r[0];

return e;
}

os_error *font_lose(f)

font f;

{
os_regset r;
os_error *e;

r.r[0] = f;
e = os_swix(LoseFont, &r);
return e;
}



os_error * font_readdef(f, d)

font f;
font_def *d;

{

os_regset r;
os_error *e;

r.r[0] = (int)f;
r.r[1] = (int)&d->name;

e = os_swix(ReadDefn, &r);

d->xsize = r.r[2];
d->ysize = r.r[3];
d->xres = r.r[4];
d->yres = r.r[5];
d->usage = r.r[6];
d->age = r.r[7];

return(e);

}




os_error *font_readinfo(f, i)

font f;
font_info *i;

{

os_regset r;
os_error *e;

r.r[0] = f;

e = os_swix(ReadInfo, &r);

i->minx = r.r[1];
i->miny = r.r[2];
i->maxx = r.r[3];
i->maxy = r.r[4];

return e;

}



os_error *font_strwidth(fs)

font_string *fs;

{

os_regset r;
os_error *e;

r.r[1] = (int)fs->s;
r.r[2] = fs->x;
r.r[3] = fs->y;
r.r[4] = fs->split;
r.r[5] = fs->term;

e = os_swix(StringWidth, &r);

fs->x = r.r[2];
fs->y = r.r[3];
fs->split = r.r[4];
fs->term = r.r[5];

return e;

}





os_error *font_paint(s, options, x, y)

char *s;
int options, x, y;

{

os_regset r;
os_error *e;

r.r[1] = (int)s;
r.r[2] = options;
r.r[3] = x;
r.r[4] = y;

e = os_swix(Paint, &r);

return e;

}



os_error *font_caret(colour, height, flags, x, y)

int colour, height, flags, x, y;

{

os_regset r;
os_error *e;

r.r[0] = colour;
r.r[1] = height;
r.r[2] = flags;
r.r[3] = x;
r.r[4] = y;

e = os_swix(Caret, &r);

return e;

}



os_error *font_converttoos(x_inch, y_inch, x_os, y_os)

int x_inch, y_inch;
int *x_os, *y_os;

{

os_regset r;
os_error *e;

r.r[1] = x_inch;
r.r[2] = y_inch;

e = os_swix(ConverttoOS, &r);

*x_os = r.r[1];
*y_os = r.r[2];

return e;

}



os_error *font_converttopoints(x_os, y_os, x_inch, y_inch)

int x_os, y_os;
int *x_inch, *y_inch;

{

os_regset r;
os_error *e;

r.r[1] = x_os;
r.r[2] = y_os;

e = os_swix(Converttopoints, &r);

*x_inch = r.r[1];
*y_inch = r.r[2];

return e;

}


os_error *font_setfont(f)

font f;

{

os_regset r;
os_error *e;

r.r[0] = f;

e = os_swix(SetFont, &r);

return(e);

}


os_error *font_current(f)

font_state *f;

{

os_regset r;
os_error *e;

e = os_swix(CurrentFont, &r);

f->f = r.r[0];
f->back_colour = r.r[1];
f->fore_colour = r.r[2];
f->offset = r.r[3];

return(e);

}



os_error *font_future(f)

font_state *f;

{

os_regset r;
os_error *e;

e = os_swix(FutureFont, &r);

f->f = r.r[0];
f->back_colour = r.r[1];
f->fore_colour = r.r[2];
f->offset = r.r[3];

return(e);

}



os_error *font_findcaret(fs)

font_string *fs;

{

os_regset r;
os_error *e;

r.r[1] = (int)fs->s;
r.r[2] = fs->x;
r.r[3] = fs->y;

e = os_swix(FindCaret, &r);

fs->x = r.r[2];
fs->y = r.r[3];
fs->split = r.r[4];
fs->term = r.r[5];

return e;

}


extern os_error *
font_charbbox(font f, char ch, int options, font_info *i)
{

os_regset r;
os_error *e;

r.r[0] = f;
r.r[1] = ch;
r.r[2] = options;

e = os_swix(CharBBox, &r);

i->minx = r.r[1];
i->miny = r.r[2];
i->maxx = r.r[3];
i->maxy = r.r[4];

return e;

}


os_error *font_readscalefactor(x, y)

int *x, *y;

{

os_regset r;
os_error *e;

e = os_swix(ReadScaleFactor, &r);

*x = r.r[1];
*y = r.r[2];

return e;

}


os_error *font_setscalefactor(x, y)

int x, y;

{

os_regset r;
os_error *e;

r.r[1] = x;
r.r[2] = y;

e = os_swix(SetScaleFactor, &r);

return e;

}



os_error *font_list(a, count)

char *a;
int *count;

{

os_regset r;
os_error *e;
int i;

r.r[1] = (int)a;
r.r[2] = *count;
r.r[3] = -1;

e = os_swix(ListFonts, &r);

if (!e)

  {

  *count = r.r[2];
  i = 0;
    
  while (a[i] >= 32 && i <= 99)

       ++i;

  a[i] = 0;

  }

else /* error return: probably some filing system error */

  *count = -1; /* signal end of list */

return e;

}



os_error *font_setcolour(f, background, foreground, offset)

font f;
int background, foreground, offset;

{

os_regset r;
os_error *e;

r.r[0] = f;
r.r[1] = background;
r.r[2] = foreground;
r.r[3] = offset;

e = os_swix(SetFontColours, &r);

return e;

}


os_error *font_setpalette(background, foreground, offset, 
                          physical_back, physical_fore)

int background, foreground, offset, physical_back, physical_fore;

{

os_regset r;
os_error *e;

r.r[1] = background;
r.r[2] = foreground;
r.r[3] = offset;
r.r[4] = physical_back;
r.r[5] = physical_fore;

e = os_swix(SetPalette, &r);

return e;

}



os_error *font_readthresholds(th)

font_threshold *th;

{

os_regset r;
os_error *e;

r.r[1] = (int)th;

e = os_swix(ReadThresholds, &r);

return e;

}



os_error *font_setthresholds(th)

font_threshold *th;

{

os_regset r;
os_error *e;

r.r[1] = (int)th;

e = os_swix(SetThresholds, &r);

return e;

}


os_error *font_findcaretj(fs, offset_x, offset_y)

font_string *fs;
int offset_x, offset_y;

{

os_regset r;
os_error *e;

r.r[1] = (int)fs->s;
r.r[2] = fs->x;
r.r[3] = fs->y;
r.r[4] = offset_x;
r.r[5] = offset_y;

e = os_swix(FindCaretJ, &r);

fs->x = r.r[2];
fs->y = r.r[3];
fs->split = r.r[4];
fs->term = r.r[5];

return e;

}


os_error *font_stringbbox(s, fi)

char *s;
font_info *fi;

{

os_regset r;
os_error *e;

r.r[1] = (int)s;

e = os_swix(StringBBox, &r);

fi->minx = r.r[1];
fi->miny = r.r[2];
fi->maxx = r.r[3];
fi->maxy = r.r[4];

return e;

}

