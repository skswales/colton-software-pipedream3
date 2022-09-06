/* font.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __font_h
#define __font_h
#ifndef __os_h
#include "os.h"
#endif
typedef int font; 
os_error * font_cacheaddress(int *version, int *cacheused, int *cachesize);
os_error * font_find(
 char* name,
 int xsize, int ysize, 
 int xres, int yres, 
 font*); 
os_error * font_lose(font f);
typedef struct font_def {
 char name[16];
 int xsize, ysize, xres, yres; 
 int usage, age;
}font_def;
os_error * font_readdef(font, font_def*);
typedef struct font_info {
 int minx, miny, maxx, maxy;
}font_info;
os_error * font_readinfo(font, font_info*);
typedef struct font_string {
 char* s;
 int x; 
 int y; 
 int split; 
 
 int term; 
 } font_string;
os_error * font_strwidth(font_string *fs);
#define font_JUSTIFY 0x01 
#define font_RUBOUT 0x02 
#define font_ABS 0x04 
 
#define font_OSCOORDS 0x10 
 
os_error * font_paint(char*, int options, int x, int y);
os_error *font_caret(int colour, int height, int flags, int x, int y);
os_error *font_converttoos(int x_inch, int y_inch, int *x_os, int *y_os);
os_error *font_converttopoints(int x_os, int y_os, int *x_inch, int *y_inch);
os_error * font_setfont(font);
typedef struct font_state {
 font f;
 int back_colour;
 int fore_colour;
 int offset;
 } font_state;
os_error *font_current(font_state *f);
os_error *font_future(font_state *f);
os_error *font_findcaret(font_string *fs);
os_error * font_charbbox(font, char, int options, font_info*);
os_error *font_readscalefactor(int *x, int *y);
os_error *font_setscalefactor(int x, int y);
os_error * font_list(char*, int*);
 
os_error * font_setcolour(font, int background, int foreground, int offset);
#define font_BlueGun 0x01000000 
#define font_GreenGun 0x00010000 
#define font_RedGun 0x00000100 
os_error *font_setpalette(int background, int foreground, int offset, 
 int physical_back, int physical_fore);
typedef struct font_threshold {
 char offset;
 char thresholds[15];
 } font_threshold;
 
os_error *font_readthresholds(font_threshold *th);
os_error *font_setthresholds(font_threshold *th);
os_error *font_findcaretj(font_string *fs, int offset_x, int offset_y);
 
os_error *font_stringbbox(char *s, font_info *fi);
#endif
