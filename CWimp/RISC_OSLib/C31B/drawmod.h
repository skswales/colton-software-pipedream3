/* drawmod.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __drawmod_h
#define __drawmod_h
#ifndef __os_h
#include "os.h"
#endif
typedef enum
 {
 path_term = 0, 
 path_ptr = 1, 
 path_move_2 = 2, 
 
 path_move_3 = 3, 
 
 
 path_closegap = 4, 
 
 path_closeline = 5, 
 
 path_bezier = 6, 
 
 
 path_gapto = 7, 
 
 path_lineto = 8 
 } drawmod_path_tagtype;
typedef struct 
 {
 drawmod_path_tagtype tag;
 unsigned int bytes_free;
 } drawmod_path_endstr;
typedef struct 
 {
 drawmod_path_tagtype tag;
 void *ptr;
 } drawmod_path_ptrstr;
typedef struct 
 {
 drawmod_path_tagtype tag;
 int x, y;
 } drawmod_path_movestr;
typedef struct 
 {
 drawmod_path_tagtype tag;
 } drawmod_path_closegapstr;
typedef struct 
 {
 drawmod_path_tagtype tag;
 } drawmod_path_closelinestr;
typedef struct 
 {
 drawmod_path_tagtype tag;
 int x1, 
 y1,
 x2, 
 y2,
 x3, 
 y3;
 } drawmod_path_bezierstr;
typedef struct 
 {
 drawmod_path_tagtype tag;
 int x,y;
 } drawmod_path_gaptostr;
typedef struct 
 {
 drawmod_path_tagtype tag;
 int x,y;
 } drawmod_path_linetostr;
typedef union 
 {
 drawmod_path_endstr *end;
 drawmod_path_ptrstr *ptr;
 drawmod_path_movestr *move2;
 drawmod_path_movestr *move3;
 drawmod_path_closegapstr *closegap;
 drawmod_path_closelinestr *closeline;
 drawmod_path_bezierstr *bezier;
 drawmod_path_gaptostr *gapto;
 drawmod_path_linetostr *lineto;
 char *bytep;
 int *wordp;
 } drawmod_pathelemptr;
typedef enum
 {
 fill_Default = 0x00000000, 
 fill_WNonzero = 0x00000000, 
 fill_WNegative = 0x00000001, 
 fill_WEvenodd = 0x00000002, 
 fill_WPositive = 0x00000003, 
 fill_FNonbext = 0x00000004, 
 fill_FBext = 0x00000008, 
 fill_FNonbint = 0x00000010, 
 fill_FBint = 0x00000020, 
 
 fill_PClose = 0x08000000, 
 fill_PFlatten = 0x10000000, 
 fill_PThicken = 0x20000000, 
 fill_PReflatten = 0x40000000 
 
 
 } drawmod_filltype;
#define join_mitred ((unsigned char)0x00) 
#define join_round ((unsigned char)0x01) 
#define join_bevelled ((unsigned char)0x02) 
#define cap_butt ((unsigned char)0x00) 
#define cap_round ((unsigned char)0x01) 
#define cap_square ((unsigned char)0x02) 
#define cap_triang ((unsigned char)0x03) 
typedef struct
 {
 unsigned char join; 
 unsigned char leadcap; 
 unsigned char trailcap; 
 unsigned char reserved8; 
 int mitrelimit; 
 unsigned short lead_tricap_w; 
 unsigned short lead_tricap_h; 
 unsigned short trail_tricap_w; 
 unsigned short trail_tricap_h; 
 } drawmod_capjoinspec;
typedef struct
 {
 int dashstart; 
 int dashcount; 
 } drawmod_dashhdr;
typedef struct
 {
 int flatness;
 int thickness;
 drawmod_capjoinspec spec;
 drawmod_dashhdr *dash_pattern;
 } drawmod_line;
 
 
typedef int drawmod_transmat[6];
typedef struct 
 {
 int zeroword;
 int sizeword;
 } drawmod_buffer;
 
typedef enum
 {
 tag_fill = 1,
 tag_box = 2,
 tag_buf = 3
 } drawmod_tagtype;
typedef enum
 { option_insitu = 0, 
 option_normalfill = 1, 
 option_subpathfill = 2, 
 option_countsize = 3 
 } drawmod_filling_options;
typedef struct
 {
 int lowx;
 int lowY;
 int highX;
 int highY;
 } drawmod_box;
typedef struct
 {
 drawmod_tagtype tag;
 union
 {
 drawmod_filling_options opts;
 drawmod_box *box; 
 drawmod_buffer *buffer;
 } data;
 } drawmod_options;
 
os_error *drawmod_fill(drawmod_pathelemptr path_seq,
 drawmod_filltype fill_style,
 drawmod_transmat *matrix,
 int flatness);
os_error *drawmod_stroke(drawmod_pathelemptr path_seq, 
 drawmod_filltype fill_style,
 drawmod_transmat *matrix,
 drawmod_line *line_style);
os_error *drawmod_do_strokepath(drawmod_pathelemptr path_seq,
 drawmod_transmat *matrix,
 drawmod_line *line_style,
 drawmod_buffer *buffer);
os_error *drawmod_ask_strokepath(drawmod_pathelemptr path_seq,
 drawmod_transmat *matrix,
 drawmod_line *line_style,
 int *buflen);
os_error *drawmod_do_flattenpath(drawmod_pathelemptr path_seq,
 drawmod_buffer *buffer,
 int flatness);
os_error *drawmod_ask_flattenpath(drawmod_pathelemptr path_seq,
 int flatness,
 int *buflen);
os_error *drawmod_buf_transformpath(drawmod_pathelemptr path_seq,
 drawmod_buffer *buffer,
 drawmod_transmat *matrix);
 
os_error *drawmod_insitu_transformpath(drawmod_pathelemptr path_seq,
 drawmod_transmat *matrix); 
os_error *drawmod_processpath(drawmod_pathelemptr path_seq,
 drawmod_filltype fill_style,
 drawmod_transmat *matrix,
 drawmod_line *line_style,
 drawmod_options *options,
 int *buflen);
#endif
