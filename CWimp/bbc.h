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
 * Title  : h.bbc
 * Purpose: provides bbc-style graphics and mouse/keyboard control
 * Version: 0.1
 *          0.2 RCM all bbc_xxxx routines now return os_error*
 *          0.3 SKS corrected spelling of bbc_MultiPurpose
 */

/* -------- Text output. -------- */

/* VDU commands. */

#define  bbc_CharToPrinter   1
#define  bbc_EnablePrinter   2
#define  bbc_DisablePrinter  3
#define  bbc_TextToText      4
#define  bbc_TextToGraph     5
#define  bbc_EnableVDU       6
#define  bbc_Bell            7
#define  bbc_MoveOneBack     8
#define  bbc_MoveOneOn       9
#define  bbc_MoveDownOne     10
#define  bbc_MoveUpOne       11
#define  bbc_ClearText       12
#define  bbc_MoveToStart     13
#define  bbc_PageOn          14
#define  bbc_PageOff         15
#define  bbc_ClearGraph      16
#define  bbc_DefTextColour   17
#define  bbc_DefGraphColour  18
#define  bbc_DefLogical      19
#define  bbc_RestoreLogical  20
#define  bbc_DisableVDU      21
#define  bbc_ScreenMode      22
#define  bbc_MultiPurpose    23
#define  bbc_DefGraphWindow  24
#define  bbc_PlotCommand     25
#define  bbc_DefaultWindow   26

#define  bbc_DefTextWindow   28
#define  bbc_DefGraphOrigin  29
#define  bbc_HomeText        30
#define  bbc_MoveText        31

os_error *bbc_vdu(int);
/* Single character VDU call. */

os_error *bbc_vduw(int);
/* Double character VDU call. */

os_error *bbc_vduq(int ctl,...); 
/* Multiple character VDU call. ctl is a control character, bbc_vduq knows
how many argument bytes ctl takes, and assumes that that many extra
arguments have been supplied. */

os_error *bbc_stringprint(char *);
/* Print a null-terminated string indicated on the screen. */

os_error *bbc_cls(void);
/* Clear text window. */

os_error *bbc_colour(int);
/* Set text colour. */

int bbc_pos(void);
/* Return the X coordinate of the text cursor. */

int bbc_vpos(void);
/* Return Y coordinate of text cursor. */

os_error *bbc_tab(int,int);
/* Positon text cursor. */

/* -------- Graphics output. -------- */

/* Plot codes to be used with the bbc_plot function. */

#define  bbc_SolidBoth       0x00
#define  bbc_SolidExFinal    0x08
#define  bbc_DottedBoth      0x10
#define  bbc_DottedExFinal   0x18
#define  bbc_SolidExInit     0x20
#define  bbc_SolidExBoth     0x28
#define  bbc_DottedExInit    0x30
#define  bbc_DottedExBoth    0x38
#define  bbc_Point           0x40
#define  bbc_HorizLineFillNB 0x48
#define  bbc_TriangleFill    0x50
#define  bbc_HorizLineFillB  0x58
#define  bbc_RectangleFill   0x60
#define  bbc_HorizLineFillF  0x68
#define  bbc_ParallelFill    0x70
#define  bbc_HorizLineFillNF 0x78
#define  bbc_FloodToBack     0x80
#define  bbc_FloodToFore     0x88
#define  bbc_Circle          0x90
#define  bbc_CircleFill      0x98
#define  bbc_CircleArc       0xA0
#define  bbc_Segment         0xA8
#define  bbc_Sector          0xB0
#define  bbc_Block           0xB8
#define  bbc_Ellipse         0xC0
#define  bbc_EllipseFill     0xC8
#define  bbc_GraphicsChar    0xD0

#define  bbc_SpritePlot      0xE8       


/* Within each block of eight the offset from the base number has the 
   following meaning : */

#define  bbc_MoveCursorRel   0
#define  bbc_DrawRelFore     1
#define  bbc_DrawRelInverse  2
#define  bbc_DrawRelBack     3
#define  bbc_MoveCursorAbs   4
#define  bbc_DrawAbsFore     5
#define  bbc_DrawAbsInverse  6
#define  bbc_DrawAbsBack     7


/* The above applies except for bbc_Block where the codes are as follows : */

#define  bbc_BMoveRel        0
#define  bbc_BMoveRectRel    1
#define  bbc_BCopyRectRel    2
 
#define  bbc_BMoveAbs        4
#define  bbc_BMoveRectAbs    5
#define  bbc_BCopyRectAbs    6


os_error *bbc_plot(int plotnumber, int x, int y);
/* Perform an operating system plot operation. */

os_error *bbc_mode(int);
/* Set screen mode. */

os_error *bbc_move(int, int);
/* Move graphics cursor to an absolute position. */

os_error *bbc_moveby(int, int);
/* Move the graphics cursor to a position relative to its current position. */

os_error *bbc_draw(int, int);
/* Draw a line to absolute coordinates. */

os_error *bbc_drawby(int, int);
/* Draw a line to coordinates specified relative to current graphic cursor. */

os_error *bbc_rectangle(int,int,int,int);
/* Plot a rectangle. Left X, bottom Y, Width, Height. */

os_error *bbc_rectanglefill(int,int,int,int);
/* Plot a solid rectangle. Left X, bottom Y, Width, Height. */

os_error *bbc_circle(int, int, int);
/* Draw a circle x, y, radius. */

os_error *bbc_circlefill(int, int, int);
/* Draw a solid circle x, y, radius. */

os_error *bbc_origin(int,int);
/* Move the graphics origin to absolute coordinates given. */

os_error *bbc_gwindow(int, int, int, int);
/* Set up graphics window. */

os_error *bbc_clg(void);
/* Clear graphic window. */

os_error *bbc_fill(int, int);
/* Flood-fill an area x, y. */
 
os_error *bbc_gcol(int, int);
/* Set graphics colour. */

os_error *bbc_tint(int,int);
/* Set grey level of a colour: use tint 0-3, as it gets shifted for you. */

os_error *bbc_palette(int,int,int,int,int);
/* Physical to logical colour definition. Logical colour, Physical colour, 
                                          Red level, Green level, Blue level.*/

int bbc_point(int,int);
/* Find the logical colour of a pixel at indicated coordinates. x, y. */

/* -------- VDU and Mode Variables. -------- */

/* VDU variables. */

typedef enum {
  bbc_GWLCol          = 128,     /* graphics window (ic) */
  bbc_GWBRow          = 129,     /* (left, bottom, right, top) */
  bbc_GWRCol          = 130,
  bbc_GWTRow          = 131,
  bbc_TWLCol          = 132,     /* text window */
  bbc_TWBRow          = 133,     /* (left, bottom, right, top) */
  bbc_TWRCol          = 134,
  bbc_TWTRow          = 135,
  bbc_OrgX            = 136,     /* graphics origin (ec) */
  bbc_OrgY            = 137,
  bbc_GCsX            = 138,     /* graphics cursor (ec) */
  bbc_GCsY            = 139,
  bbc_OlderCsX        = 140,     /* oldest graphics cursor (ic) */
  bbc_OlderCsY        = 141,
  bbc_OldCsX          = 142,     /* previous graphics cursor (ic) */
  bbc_OldCsY          = 143,
  bbc_GCsIX           = 144,     /* graphics cursor (ic) */
  bbc_GCsIY           = 145,
  bbc_NewPtX          = 146,     /* new point (ic) */
  bbc_NewPtY          = 147,
  bbc_ScreenStart     = 148,     /* start of screen memory */
  bbc_DisplayStart    = 149,     /* start of display screen memory */
  bbc_TotalScreenSize = 150,     /* size of configured screen memory */
  bbc_GPLFMD          = 151,     /* GCOL action for foreground colour */
  bbc_CPLBMD          = 152,     /* GCOL action for background colour */
  bbc_GFCOL           = 153,     /* foreground/background colours */
  bbc_GBCOL           = 154,
  bbc_TForeCol        = 155,     /* text foreground/background colours */
  bbc_TBackCol        = 156,
  bbc_GFTint          = 157,     /* graphics tints */
  bbc_GBTint          = 158,
  bbc_TFTint          = 159,     /* text tints */
  bbc_TBTint          = 160,
  bbc_MaxMode         = 161,     /* highest mode number available */
  bbc_GCharSizeX      = 162,     /* size of VDU-5 system font in pixels */
  bbc_GCharSizeY      = 163,
  bbc_GCharSpaceX     = 164,     /* spacing of VDU-5 system font */
  bbc_GCharSpaceY     = 165,
  bbc_HLineAddr       = 166,
  bbc_TCharSizeX      = 167,     /* text chars (in pixels) */
  bbc_TCharSizeY      = 168,
  bbc_TCharSpaceX     = 169,
  bbc_TCharSpaceY     = 170
} bbc_vduvariable;

typedef enum {
  bbc_ModeFlags,                 /* bit0->non-graphic,
                                    bit1->teletext,
                                    bit2->gap */
  bbc_ScrRCol,                   /* max text col number */
  bbc_ScrBCol,                   /* max text row number */
  bbc_NColour,                   /* max logical colour: 1, 3, 15 or 63 */
  bbc_XEigFactor,                /* OS-unit->pixel shift factor.
                                    0 -> OS-units = pixels,
                                    1 -> 2 OS-units per pixel,
                                    2 -> 4 OS-units per pixel, etc. */
  bbc_YEigFactor,
  bbc_LineLength,                /* bytes per pixel row. */
  bbc_ScreenSize,                /* bytes per screen. */
  bbc_YShftFactor,               /* DEPRECATED; Log(2) of LineLength/5. */
  bbc_Log2BPP,                   /* log base 2 of bits per pixel. */
  bbc_Log2BPC,                   /* log base 2 of bytes per character. */
  bbc_XWindLimit,                /* pixels across - 1 */
  bbc_YWindLimit                 /* pixels up - 1 */
} bbc_modevariable;

int bbc_vduvar(int varno);
/* Read a single VDU or mode variable value, for the current screen mode. */

os_error *bbc_vduvars(int *vars /*in*/, int *values /*out*/);
/* Read several VDU or mode variable values. vars points to a sequence
of ints in memory, terminated by a -1 int. Each is a VDU or mode variable
number, and the corresponding int in values will be replaced by the value of
that variable. */

int bbc_modevar(int mode, int varno);
/* Read a single mode variable, for the given screen mode. */

/* -------- Other calls. -------- */

int bbc_get(void);
/* Return a character code from the input stream;
   &1xx is returned if an ESCAPE condition exists.
 */

os_error *bbc_cursor(int);
/* Alter cursor appearance. Argument takes values 0 to 3. */

int bbc_adval(int);
/* Read data from analogue ports or gives buffer data. */

int bbc_getbeat(void);
/* Return current beat value. */

int bbc_getbeats(void);
/* Read beat counter cycle length. */

int bbc_gettempo(void);
/* Read rate at which beat counter counts. */

int bbc_inkey(int);
/* Return a character code from an input stream or the keyboard. */

unsigned bbc_rnd(unsigned);
/* Return a random number. */

os_error *bbc_beats(int);
/* Set beat counter cycle length. */

os_error *bbc_settempo(int);
/* Set rate at which beat counter counts. */

os_error *bbc_sound(int, int, int, int, int);
/*Make or schedule a sound. Channel, Amplitude, Pitch, Duration, future time.*/

os_error *bbc_soundoff(void);
/* Deactivate sound system. */

os_error *bbc_soundon(void);
/* Activate sound system. */

os_error *bbc_stereo(int, int);
/* Set stereo position for specified channel. Channel, Position. */

os_error *bbc_voices(int);
/* Set number of sound channels. 1, 2, 4 or 8. */

/* end of h.bbc */
