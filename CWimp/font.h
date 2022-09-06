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
 * Title  : h.font
 * Purpose: access to RISC OS font facilities
 * Version: 0.1
 */


typedef int font; /* abstract font handle */

os_error * font_cacheaddress(int *version, int *cacheused, int *cachesize);
/* The cache metrics are returned in bytes. */

os_error * font_find(
  char* name,
  int xsize, int ysize,    /* in 16ths of a point */
  int xres, int yres,      /* dots per inch of resolution: 0->use default */
  font*);                  /* result */

os_error * font_lose(font f);

typedef struct font_def {
    char name[16];
    int xsize, ysize, xres, yres; /* as above */
    int usage, age;
} font_def;

os_error * font_readdef(font, font_def*);

typedef struct font_info {
    int minx, miny, maxx, maxy;
} font_info;

os_error * font_readinfo(font, font_info*);
/* bounding box, in pixels, that would surround any char. */

typedef struct font_string {

    char* s;
    int x;                /* inout, in 1/72000 inch */
    int y;                /* inout, in 1/72000 inch */
    int split;            /* inout: space char, or -1 */
                          /* on exit, = count of space or printable */
    int term;             /* inout, index into s */

    } font_string;

os_error * font_strwidth(font_string *fs);

/* The initial values of x, y, term are the maximum allowable ones. */

/* paint options */
#define font_JUSTIFY 0x01
#define font_RUBOUT  0x02
#define font_ABS     0x04
        /* 8 not used */
#define font_OSCOORDS 0x10

os_error * font_paint(char*, int options, int x, int y);
/* To rub out or justify, use two previous MOVE commands to set things up. */

os_error *font_caret(int colour, int height, int flags, int x, int y);
/* Set the colour, size and position of the caret. */                                                   
os_error *font_converttoos(int x_inch, int y_inch, int *x_os, int *y_os);
/* Converts coords in 1/72000th inch to OS units. */

os_error *font_converttopoints(int x_os, int y_os, int *x_inch, int *y_inch);
/* Converts coords in OS units to 1/72000th inch. */

os_error * font_setfont(font);


typedef struct font_state {

       font f;
       int back_colour;
       int fore_colour;
       int offset;

       } font_state;


os_error *font_current(font_state *f);
/* Returns the current state of the painter's internal variables. */


os_error *font_future(font_state *f);
/* Used to find out what the effect of a Font_Paint would have on the
   current font handle and colours. */

os_error *font_findcaret(font_string *fs);
/* Work out the best place for the caret to go. */

os_error * font_charbbox(font f, char ch, int options, font_info *i);
/* The only relevant option is font_OSCOORDS */
/* There may be some extra whitespace for the OS coord case. */


os_error *font_readscalefactor(int *x, int *y);
/* Returns the x, y scale factors. */


os_error *font_setscalefactor(int x, int y);
/* Sets up the x, y scale factors. */        

os_error * font_list(
  char*,                 /* result buffer for font name, >= 40 chars */
  int*);                 /* 0 on entry to first call, */
                         /* -1 on exit when finished. */

os_error * font_setcolour(font, int background, int foreground, int offset);

#define font_BlueGun  0x01000000 /* 8-bit field: phsical colour blue gun.  */
#define font_GreenGun 0x00010000 /* 8-bit field: phsical colour green gun. */
#define font_RedGun   0x00000100 /* 8-bit field: phsical colour red gun.   */

os_error *font_setpalette(int background, int foreground, int offset, 
                          int physical_back, int physical_fore);

typedef struct font_threshold {

     char offset;
     char thresholds[15];

     } font_threshold;

os_error *font_readthresholds(font_threshold *th);
/* Reads the current threshold values. */

os_error *font_setthresholds(font_threshold *th);
/* Sets up threshold values for a given number of output colours. */

os_error *font_findcaretj(font_string *fs, int offset_x, int offset_y);
/* Returns nearest point where caret can go. */

os_error *font_stringbbox(char *s, font_info *fi);
/* Used to find out how much of the page a given string would take up. */


/* end h.font */
