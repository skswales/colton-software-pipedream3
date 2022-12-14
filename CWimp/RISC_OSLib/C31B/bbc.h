/* bbc.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __bbc_h
#define __bbc_h
 
#ifndef __os_h
#include "os.h"
#endif
#define bbc_CharToPrinter 1
#define bbc_EnablePrinter 2
#define bbc_DisablePrinter 3
#define bbc_TextToText 4
#define bbc_TextToGraph 5
#define bbc_EnableVDU 6
#define bbc_Bell 7
#define bbc_MoveOneBack 8
#define bbc_MoveOneOn 9
#define bbc_MoveDownOne 10
#define bbc_MoveUpOne 11
#define bbc_ClearText 12
#define bbc_MoveToStart 13
#define bbc_PageOn 14
#define bbc_PageOff 15
#define bbc_ClearGraph 16
#define bbc_DefTextColour 17
#define bbc_DefGraphColour 18
#define bbc_DefLogical 19
#define bbc_RestoreLogical 20
#define bbc_DisableVDU 21
#define bbc_ScreenMode 22
#define bbc_MultiPurpose 23
#define bbc_DefGraphWindow 24
#define bbc_PlotCommand 25
#define bbc_DefaultWindow 26
#define bbc_DefTextWindow 28
#define bbc_DefGraphOrigin 29
#define bbc_HomeText 30
#define bbc_MoveText 31
os_error *bbc_vdu(int);
os_error *bbc_vduw(int);
os_error *bbc_vduq(int ctl,...);
 
os_error *bbc_stringprint(char *);
os_error *bbc_cls(void);
os_error *bbc_colour(int);
int bbc_pos(void);
int bbc_vpos(void);
os_error *bbc_tab(int,int);
#define bbc_SolidBoth 0x00
#define bbc_SolidExFinal 0x08
#define bbc_DottedBoth 0x10
#define bbc_DottedExFinal 0x18
#define bbc_SolidExInit 0x20
#define bbc_SolidExBoth 0x28
#define bbc_DottedExInit 0x30
#define bbc_DottedExBoth 0x38
#define bbc_Point 0x40
#define bbc_HorizLineFillNB 0x48
#define bbc_TriangleFill 0x50
#define bbc_HorizLineFillB 0x58
#define bbc_RectangleFill 0x60
#define bbc_HorizLineFillF 0x68
#define bbc_ParallelFill 0x70
#define bbc_HorizLineFillNF 0x78
#define bbc_FloodToBack 0x80
#define bbc_FloodToFore 0x88
#define bbc_Circle 0x90
#define bbc_CircleFill 0x98
#define bbc_CircleArc 0xA0
#define bbc_Segment 0xA8
#define bbc_Sector 0xB0
#define bbc_Block 0xB8
#define bbc_Ellipse 0xC0
#define bbc_EllipseFill 0xC8
#define bbc_GraphicsChar 0xD0
#define bbc_SpritePlot 0xE8 
#define bbc_MoveCursorRel 0
#define bbc_DrawRelFore 1
#define bbc_DrawRelInverse 2
#define bbc_DrawRelBack 3
#define bbc_MoveCursorAbs 4
#define bbc_DrawAbsFore 5
#define bbc_DrawAbsInverse 6
#define bbc_DrawAbsBack 7
#define bbc_BMoveRel 0
#define bbc_BMoveRectRel 1
#define bbc_BCopyRectRel 2
 
#define bbc_BMoveAbs 4
#define bbc_BMoveRectAbs 5
#define bbc_BCopyRectAbs 6
os_error *bbc_plot(int plotnumber, int x, int y);
os_error *bbc_mode(int);
os_error *bbc_move(int, int);
os_error *bbc_moveby(int, int);
os_error *bbc_draw(int, int);
os_error *bbc_drawby(int, int);
os_error *bbc_rectangle(int,int,int,int);
os_error *bbc_rectanglefill(int,int,int,int);
os_error *bbc_circle(int, int, int);
os_error *bbc_circlefill(int, int, int);
os_error *bbc_origin(int,int);
os_error *bbc_gwindow(int, int, int, int);
os_error *bbc_clg(void);
os_error *bbc_fill(int, int);
os_error *bbc_gcol(int, int);
os_error *bbc_tint(int,int);
os_error *bbc_palette(int,int,int,int,int);
 
int bbc_point(int,int);
typedef enum {
 bbc_GWLCol = 128, 
 bbc_GWBRow = 129, 
 bbc_GWRCol = 130,
 bbc_GWTRow = 131,
 bbc_TWLCol = 132, 
 bbc_TWBRow = 133, 
 bbc_TWRCol = 134,
 bbc_TWTRow = 135,
 bbc_OrgX = 136, 
 bbc_OrgY = 137,
 bbc_GCsX = 138, 
 bbc_GCsY = 139,
 bbc_OlderCsX = 140, 
 bbc_OlderCsY = 141,
 bbc_OldCsX = 142, 
 bbc_OldCsY = 143,
 bbc_GCsIX = 144, 
 bbc_GCsIY = 145,
 bbc_NewPtX = 146, 
 bbc_NewPtY = 147,
 bbc_ScreenStart = 148, 
 bbc_DisplayStart = 149, 
 bbc_TotalScreenSize = 150, 
 bbc_GPLFMD = 151, 
 bbc_CPLBMD = 152, 
 bbc_GFCOL = 153, 
 bbc_GBCOL = 154,
 bbc_TForeCol = 155, 
 bbc_TBackCol = 156,
 bbc_GFTint = 157, 
 bbc_GBTint = 158,
 bbc_TFTint = 159, 
 bbc_TBTint = 160,
 bbc_MaxMode = 161, 
 bbc_GCharSizeX = 162, 
 bbc_GCharSizeY = 163,
 bbc_GCharSpaceX = 164, 
 bbc_GCharSpaceY = 165,
 bbc_HLineAddr = 166,
 bbc_TCharSizeX = 167, 
 bbc_TCharSizeY = 168,
 bbc_TCharSpaceX = 169,
 bbc_TCharSpaceY = 170
}bbc_vduvariable;
typedef enum {
 bbc_ModeFlags, 
 bbc_ScrRCol, 
 bbc_ScrBCol, 
 bbc_NColour, 
 bbc_XEigFactor, 
 bbc_YEigFactor,
 bbc_LineLength, 
 bbc_ScreenSize, 
 bbc_YShftFactor, 
 bbc_Log2BPP, 
 bbc_Log2BPC, 
 bbc_XWindLimit, 
 bbc_YWindLimit 
}bbc_modevariable;
int bbc_vduvar(int varno);
os_error *bbc_vduvars(int *vars , int *values );
int bbc_modevar(int mode, int varno);
int bbc_get(void);
os_error *bbc_cursor(int);
int bbc_adval(int);
int bbc_getbeat(void);
int bbc_getbeats(void);
int bbc_gettempo(void);
int bbc_inkey(int);
unsigned bbc_rnd(unsigned);
os_error *bbc_setbeats(int);
os_error *bbc_settempo(int);
os_error *bbc_sound(int, int, int, int, int);
os_error *bbc_soundoff(void);
os_error *bbc_soundon(void);
os_error *bbc_stereo(int, int);
os_error *bbc_voices(int);
#endif
