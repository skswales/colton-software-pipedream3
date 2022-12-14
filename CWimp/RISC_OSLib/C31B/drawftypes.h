/* drawftypes.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __drawftypes_h
#define __drawftypes_h
#ifndef __os_h
#include "os.h"
#endif
#ifndef __sprite_h
#include "sprite.h"
#endif
#ifndef __drawfdiag_h
#include "drawfdiag.h"
#endif
typedef int draw_sizetyp;
typedef int draw_coltyp;
typedef int draw_pathwidth; 
typedef draw_box draw_bboxtyp;
typedef enum
{join_mitred = 0,
 join_round = 1,
 join_bevelled = 2
}draw_jointyp;
typedef enum
{cap_butt = 0,
 cap_round = 1,
 cap_square = 2,
 cap_triangle = 3
}draw_captyp;
typedef enum
{wind_nonzero = 0,
 wind_evenodd = 1
}draw_windtyp;
typedef enum
{dash_absent = 0,
 dash_present = 1
}draw_dashtyp;
#define packmask_join 0x03
#define packmask_endcap 0x0C
#define packmask_startcap 0x30
#define packmask_windrule 0x40
#define packmask_dashed 0x80
#define packshft_join 0
#define packshft_endcap 2
#define packshft_startcap 4
#define packshft_windrule 6
#define packshft_dashed 7
typedef struct
{unsigned char joincapwind; 
 
 
 
 
 unsigned char reserved8; 
 unsigned char tricapwid; 
 unsigned char tricaphei; 
}draw_pathstyle;
typedef char draw_fontref; 
typedef struct
{draw_fontref fontref; 
 char reserved8; 
 short reserved16; 
}draw_textstyle; 
typedef unsigned int draw_fontsize; 
typedef struct
{int typeface; 
 int typesizex;
 int typesizey;
 draw_coltyp textcolour; 
 draw_coltyp background; 
}fontrec;
typedef enum
{draw_OBJFONTLIST = 0,
 draw_OBJTEXT = 1,
 draw_OBJPATH = 2,
 draw_OBJSPRITE = 5,
 draw_OBJGROUP = 6,
 draw_OBJTEXTAREA = 9,
 draw_OBJTEXTCOL = 10
}draw_tagtyp;
typedef struct { int x,y; } draw_objcoord;
 
typedef struct
{char title[4];
 int majorstamp;
 int minorstamp;
 char progident[12];
 draw_bboxtyp bbox; 
}draw_fileheader;
 
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
}draw_objhdr;
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
}draw_fontliststrhdr;
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_fontref fontref; 
 char fontname[1]; 
}draw_fontliststr;
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
 draw_coltyp textcolour; 
 draw_coltyp background; 
 draw_textstyle textstyle; 
 draw_fontsize fsizex; 
 draw_fontsize fsizey; 
 draw_objcoord coord; 
}draw_textstrhdr;
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
 draw_coltyp textcolour; 
 draw_coltyp background; 
 draw_textstyle textstyle; 
 draw_fontsize fsizex; 
 draw_fontsize fsizey; 
 draw_objcoord coord; 
 char text[1]; 
}draw_textstr;
typedef enum
 { draw_PathTERM = 0, 
 draw_PathMOVE = 2, 
 draw_PathLINE = 8, 
 draw_PathCURVE = 6, 
 draw_PathCLOSE = 5 
}draw_path_tagtype;
typedef struct { draw_path_tagtype tag; int x,y; } Path_movestr;
typedef struct { draw_path_tagtype tag; int x,y; } Path_linestr;
typedef struct { draw_path_tagtype tag; int x1,y1;
 int x2,y2; int x3,y3; } Path_curvestr;
typedef struct { draw_path_tagtype tag; } Path_closestr;
typedef struct { draw_path_tagtype tag; } Path_termstr;
typedef union 
{Path_movestr a;Path_linestr b;Path_curvestr c;Path_closestr d;Path_termstr e;
}Largest_path_str;
typedef union
{Path_movestr *move;
 Path_linestr *line;
 Path_curvestr *curve;
 Path_closestr *close;
 Path_termstr *term;
 char *bytep;
 int *wordp;
}Path_eleptr;
typedef struct
{int dashstart; 
 int dashcount; 
}draw_dashstrhdr;
typedef struct
{int dashstart; 
 int dashcount; 
 int dashelements[6]; 
}draw_dashstr;
typedef struct
{char join;
 char endcap;
 char startcap;
 char reserved; 
 int mitrelimit;
 short endtricapwid;
 short endtricaphei;
 short starttricapwid;
 short starttricaphei;
}draw_jointspec;
 
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
 draw_coltyp fillcolour; 
 draw_coltyp pathcolour; 
 draw_pathwidth pathwidth; 
 draw_pathstyle pathstyle; 
}draw_pathstrhdr;
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
 draw_coltyp fillcolour; 
 draw_coltyp pathcolour; 
 draw_pathwidth pathwidth; 
 draw_pathstyle pathstyle; 
 draw_dashstr data; 
 int PATH;
}draw_pathstr;
 
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
}draw_spristrhdr;
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
 sprite_header sprite;
 int palette[1]; 
}draw_spristr;
typedef struct { char ch[12]; } draw_groupnametyp; 
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
 draw_groupnametyp name; 
}draw_groustr; 
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
}draw_textcolhdr;
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
 draw_textcolhdr column; 
}draw_textareastrhdr;
typedef struct
{draw_tagtyp tag; 
 draw_sizetyp size; 
 draw_bboxtyp bbox; 
 
}draw_textareahdr;
typedef struct 
{
 int endmark; 
 int blank1; 
 int blank2; 
 draw_coltyp textcolour; 
 draw_coltyp backcolour; 
 
}draw_textareastrend;
typedef struct
{
 int endmark; 
 int blank1; 
 int blank2; 
 draw_coltyp textcolour; 
 draw_coltyp backcolour; 
 char text[1]; 
}draw_textareaend;
#endif
