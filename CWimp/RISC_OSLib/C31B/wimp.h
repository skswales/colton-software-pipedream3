/* wimp.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __wimp_h
#define __wimp_h
#ifndef __os_h
#include "os.h"
#endif
#ifndef __sprite_h
#include "sprite.h"
#endif
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef enum { 
 wimp_WMOVEABLE = 0x00000002, 
 wimp_REDRAW_OK = 0x00000010, 
 wimp_WPANE = 0x00000020, 
 wimp_WTRESPASS = 0x00000040, 
 wimp_WSCROLL_R1= 0x00000100, 
 wimp_SCROLL_R2 = 0x00000200, 
 wimp_REAL_COLOURS = 0x000000400, 
 wimp_BACK_WINDOW = 0x000000800, 
 wimp_HOT_KEYS = 0x000001000, 
 wimp_WOPEN = 0x00010000, 
 wimp_WTOP = 0x00020000, 
 wimp_WFULL = 0x00040000, 
 wimp_WCLICK_TOGGLE = 0x00080000, 
 wimp_WFOCUS = 0x00100000, 
 wimp_WBACK = 0x01000000, 
 wimp_WQUIT = 0x02000000, 
 wimp_WTITLE = 0x04000000, 
 wimp_WTOGGLE= 0x08000000, 
 wimp_WVSCR = 0x10000000, 
 wimp_WSIZE = 0x20000000, 
 wimp_WHSCR = 0x40000000, 
 wimp_WNEW =~0x7fffffff 
 
}wimp_wflags;
typedef enum {
 wimp_WCTITLEFORE,
 wimp_WCTITLEBACK,
 wimp_WCWKAREAFORE,
 wimp_WCWKAREABACK,
 wimp_WCSCROLLOUTER,
 wimp_WCSCROLLINNER,
 wimp_WCTITLEHI,
 wimp_WCRESERVED
}wimp_wcolours;
typedef enum { 
 wimp_ITEXT = 0x00000001, 
 wimp_ISPRITE = 0x00000002, 
 wimp_IBORDER = 0x00000004, 
 wimp_IHCENTRE = 0x00000008, 
 wimp_IVCENTRE = 0x00000010, 
 wimp_IFILLED = 0x00000020, 
 wimp_IFONT = 0x00000040, 
 wimp_IREDRAW = 0x00000080, 
 wimp_INDIRECT = 0x00000100, 
 wimp_IRJUST = 0x00000200, 
 wimp_IESG_NOC = 0x00000400, 
 wimp_IHALVESPRITE=0x00000800, 
 wimp_IBTYPE = 0x00001000, 
 wimp_ISELECTED = 0x00200000, 
 wimp_INOSELECT = 0x00400000, 
 wimp_IDELETED = 0x00800000, 
 wimp_IFORECOL = 0x01000000, 
 wimp_IBACKCOL = 0x10000000 
}wimp_iconflags;
#define wimp_IFONTH 0x01000000
typedef enum { 
 wimp_BIGNORE, 
 wimp_BNOTIFY,
 wimp_BCLICKAUTO,
 wimp_BCLICKDEBOUNCE,
 wimp_BSELREL,
 wimp_BSELDOUBLE,
 wimp_BDEBOUNCEDRAG,
 wimp_BRELEASEDRAG,
 wimp_BDOUBLEDRAG,
 wimp_BSELNOTIFY,
 wimp_BCLICKDRAGDOUBLE,
 wimp_BCLICKSEL, 
 wimp_BWRITABLE = 15
}wimp_ibtype;
typedef enum { 
 wimp_BRIGHT = 0x001,
 wimp_BMID = 0x002,
 wimp_BLEFT = 0x004,
 wimp_BDRAGRIGHT = 0x010,
 wimp_BDRAGLEFT = 0x040,
 wimp_BCLICKRIGHT = 0x100,
 wimp_BCLICKLEFT = 0x400
}wimp_bbits;
typedef enum {
 wimp_MOVE_WIND = 1, 
 wimp_SIZE_WIND = 2, 
 wimp_DRAG_HBAR = 3, 
 wimp_DRAG_VBAR = 4, 
 wimp_USER_FIXED = 5, 
 wimp_USER_RUBBER = 6, 
 wimp_USER_HIDDEN = 7 
}wimp_dragtype;
typedef int wimp_w; 
typedef int wimp_i; 
typedef int wimp_t; 
typedef union { 
 char text[12]; 
 char sprite_name[12]; 
 struct {
 char *name;
 void *spritearea; 
 
 BOOL nameisname; 
 } indirectsprite;
 struct { 
 char *buffer; 
 char *validstring; 
 int bufflen; 
 } indirecttext;
}wimp_icondata;
typedef struct {
 int x0, y0, x1, y1;
}wimp_box;
typedef struct {
 wimp_box box; 
 int scx, scy; 
 wimp_w behind; 
 wimp_wflags flags; 
 char colours[8]; 
 wimp_box ex; 
 wimp_iconflags titleflags; 
 wimp_iconflags workflags; 
 void *spritearea; 
 
 int minsize; 
 wimp_icondata title; 
 int nicons; 
}wimp_wind;
typedef struct { 
 wimp_w w;
 wimp_wind info;
}wimp_winfo;
typedef struct { 
 wimp_box box; 
 wimp_iconflags flags; 
 wimp_icondata data; 
}wimp_icon;
typedef struct { 
 wimp_w w;
 wimp_icon i;
}wimp_icreate;
typedef struct {
 wimp_w w; 
 wimp_box box; 
 int x, y; 
 wimp_w behind; 
}wimp_openstr;
typedef struct { 
 wimp_openstr o;
 wimp_wflags flags;
}wimp_wstate;
typedef enum { 
 wimp_ENULL, 
 wimp_EREDRAW, 
 wimp_EOPEN,
 wimp_ECLOSE,
 wimp_EPTRLEAVE,
 wimp_EPTRENTER,
 wimp_EBUT, 
 wimp_EUSERDRAG,
 wimp_EKEY,
 wimp_EMENU,
 wimp_ESCROLL,
 wimp_ELOSECARET,
 wimp_EGAINCARET,
 wimp_ESEND = 17, 
 wimp_ESENDWANTACK = 18, 
 wimp_EACK = 19 
}wimp_etype;
typedef enum { 
 wimp_EMNULL = 1 << wimp_ENULL,
 wimp_EMREDRAW = 1 << wimp_EREDRAW,
 wimp_EMOPEN = 1 << wimp_EOPEN,
 wimp_EMCLOSE = 1 << wimp_ECLOSE,
 wimp_EMPTRLEAVE = 1 << wimp_EPTRLEAVE,
 wimp_EMPTRENTER = 1 << wimp_EPTRENTER,
 wimp_EMBUT = 1 << wimp_EBUT,
 wimp_EMUSERDRAG = 1 << wimp_EUSERDRAG,
 wimp_EMKEY = 1 << wimp_EKEY,
 wimp_EMMENU = 1 << wimp_EMENU,
 wimp_EMSCROLL = 1 << wimp_ESCROLL
}wimp_emask;
typedef struct {
 wimp_w w;
 wimp_box box; 
 int scx, scy; 
 wimp_box g; 
}wimp_redrawstr;
typedef struct {
 int x, y; 
 wimp_bbits bbits; 
 wimp_w w; 
 wimp_i i; 
}wimp_mousestr;
typedef struct {
 wimp_w w;
 wimp_i i;
 int x, y; 
 int height; 
 int index; 
}wimp_caretstr;
typedef enum {
 wimp_MCLOSEDOWN = 0, 
 wimp_MDATASAVE = 1, 
 wimp_MDATASAVEOK = 2, 
 wimp_MDATALOAD = 3, 
 wimp_MDATALOADOK = 4, 
 wimp_MDATAOPEN = 5, 
 wimp_MRAMFETCH = 6, 
 wimp_MRAMTRANSMIT = 7, 
 wimp_MPREQUIT = 8,
 wimp_PALETTECHANGE = 9,
 wimp_FilerOpenDir = 0x0400,
 wimp_FilerCloseDir = 0x0401,
 wimp_Notify = 0x40040, 
 wimp_MMENUWARN = 0x400c0,
 
 wimp_MMODECHANGE = 0x400c1,
 wimp_MINITTASK = 0x400c2,
 wimp_MCLOSETASK = 0x400c3,
 wimp_MSLOTCHANGE = 0x400c4, 
 wimp_MSETSLOT = 0x400c5, 
 wimp_MTASKNAMERQ = 0x400c6, 
 wimp_MTASKNAMEIS = 0x400c7, 
 wimp_MHELPREQUEST = 0x502, 
 wimp_MHELPREPLY = 0x503, 
 
 wimp_MPrintFile = 0x80140, 
 
 wimp_MWillPrint = 0x80141, 
 wimp_MPrintTypeOdd = 0x80145, 
 
 wimp_MPrintTypeKnown = 0x80146, 
 wimp_MPrinterChange = 0x80147 
}wimp_msgaction;
typedef struct { 
 int size; 
 wimp_t task; 
 int my_ref; 
 int your_ref; 
 wimp_msgaction action; 
}wimp_msghdr;
typedef struct {
 wimp_w w; 
 wimp_i i; 
 int x; 
 int y;
 int estsize; 
 int type; 
 char leaf[12]; 
}wimp_msgdatasave;
typedef struct {
 
 wimp_w w; 
 wimp_i i; 
 int x; 
 int y;
 int estsize; 
 int type; 
 char name[212]; 
}wimp_msgdatasaveok;
typedef struct {
 wimp_w w; 
 wimp_i i; 
 int x; 
 int y;
 int size; 
 int type; 
 char name[212]; 
}wimp_msgdataload;
typedef wimp_msgdataload wimp_msgdataopen;
typedef struct { 
 char *addr; 
 int nbytes; 
}wimp_msgramfetch;
typedef struct { 
 char *addr; 
 int nbyteswritten; 
}wimp_msgramtransmit;
typedef struct {
 wimp_mousestr m; 
}wimp_msghelprequest;
typedef struct {
 char text[200]; 
}wimp_msghelpreply;
typedef struct { 
 int filler[5] ;
 int type ; 
 char name[256-44] ; 
}wimp_msgprint ;
typedef struct { 
 wimp_msghdr hdr;
 union {
 char chars[236];
 int words[59]; 
 wimp_msgdatasave datasave;
 wimp_msgdatasaveok datasaveok;
 wimp_msgdataload dataload;
 wimp_msgdataopen dataopen;
 wimp_msgramfetch ramfetch;
 wimp_msgramtransmit ramtransmit;
 wimp_msghelprequest helprequest;
 wimp_msghelpreply helpreply;
 wimp_msgprint print;
 } data;
}wimp_msgstr;
typedef union {
 wimp_openstr o; 
 struct {
 wimp_mousestr m;
 wimp_bbits b;} but; 
 wimp_box dragbox; 
 struct {wimp_caretstr c; int chcode;} key; 
 int menu[10]; 
 struct {wimp_openstr o; int x, y;} scroll; 
 
 
 
 wimp_caretstr c; 
 wimp_msgstr msg; 
}wimp_eventdata;
typedef struct { 
 wimp_etype e; 
 wimp_eventdata data;
}wimp_eventstr;
typedef struct {
 char title[12]; 
 char tit_fcol, tit_bcol, work_fcol, work_bcol; 
 int width, height; 
 int gap; 
}wimp_menuhdr;
typedef enum { 
 wimp_MTICK = 1,
 wimp_MSEPARATE = 2,
 wimp_MWRITABLE = 4,
 wimp_MSUBLINKMSG = 8, 
 wimp_MLAST = 0x80 
}wimp_menuflags;
typedef struct wimp_menustr *wimp_menuptr;
typedef struct {
 wimp_menuflags flags; 
 wimp_menuptr submenu; 
 wimp_iconflags iconflags; 
 wimp_icondata data; 
}wimp_menuitem;
typedef struct wimp_menustr {
 wimp_menuhdr hdr;
 
}wimp_menustr;
typedef struct {
 wimp_w window;
 wimp_dragtype type;
 wimp_box box; 
 wimp_box parent; 
}wimp_dragstr;
typedef struct {
 wimp_w window; 
 int bit_mask; 
 int bit_set; 
}wimp_which_block;
typedef struct {
 int shape_num; 
 char * shape_data; 
 int width, height; 
 int activex, activey; 
}wimp_pshapestr;
typedef struct {
 char f[256]; 
}wimp_font_array;
typedef struct { 
 int reserved; 
 wimp_wind *buf; 
 char *work_free; 
 char *work_end; 
 wimp_font_array *font; 
 char *name; 
 int index; 
}wimp_template;
typedef union {
 struct {char gcol; char red; char green; char blue;} bytes;
 int word;
}wimp_paletteword;
typedef struct {
 wimp_paletteword c[16]; 
 wimp_paletteword screenborder, mouse1, mouse2, mouse3;
}wimp_palettestr;
os_error *wimp_initialise(int * v);
os_error *wimp_taskinit(char *name, int *version, wimp_t *t );
os_error *wimp_create_wind(wimp_wind *, wimp_w *);
os_error *wimp_create_icon(wimp_icreate *, wimp_i *result);
os_error *wimp_delete_wind(wimp_w);
os_error *wimp_delete_icon(wimp_w, wimp_i);
os_error *wimp_open_wind(wimp_openstr *);
os_error *wimp_close_wind(wimp_w);
os_error *wimp_poll(wimp_emask mask, wimp_eventstr *result);
void wimp_save_fp_state_on_poll(void) ;
void wimp_corrupt_fp_state_on_poll(void) ;
os_error *wimp_redraw_wind(wimp_redrawstr*, BOOL* );
os_error *wimp_update_wind(wimp_redrawstr*, BOOL* );
os_error *wimp_get_rectangle(wimp_redrawstr*, BOOL*);
os_error *wimp_get_wind_state(wimp_w, wimp_wstate * result);
os_error *wimp_get_wind_info(wimp_winfo * result);
os_error *wimp_set_icon_state(wimp_w, wimp_i,
 wimp_iconflags value, wimp_iconflags mask);
os_error *wimp_get_icon_info(wimp_w, wimp_i, wimp_icon * result);
os_error *wimp_get_point_info(wimp_mousestr * result);
os_error *wimp_drag_box(wimp_dragstr *);
os_error *wimp_force_redraw(wimp_redrawstr * r);
os_error *wimp_set_caret_pos(wimp_caretstr *);
os_error *wimp_get_caret_pos(wimp_caretstr *);
os_error *wimp_create_menu(wimp_menustr *m, int x, int y);
os_error *wimp_decode_menu(wimp_menustr *, void *, void *);
os_error *wimp_which_icon(wimp_which_block *, wimp_i * results);
os_error *wimp_set_extent(wimp_redrawstr *);
os_error *wimp_set_point_shape(wimp_pshapestr *);
os_error *wimp_open_template(char * name);
os_error *wimp_close_template(void);
os_error *wimp_load_template(wimp_template *);
os_error *wimp_processkey(int chcode);
os_error *wimp_closedown(void);
os_error *wimp_taskclose(wimp_t);
os_error *wimp_starttask(char *clicmd);
os_error *wimp_getwindowoutline(wimp_redrawstr *r);
os_error *wimp_pollidle(wimp_emask mask, wimp_eventstr *result, int earliest);
os_error *wimp_ploticon(wimp_icon*);
os_error *wimp_setmode(int mode);
os_error *wimp_readpalette(wimp_palettestr*);
os_error *wimp_setpalette(wimp_palettestr*);
os_error *wimp_setcolour(int colour);
os_error *wimp_spriteop(int reason_code, char *name) ;
os_error *wimp_spriteop_full(os_regset *) ;
void *wimp_baseofsprites(void);
os_error *wimp_blockcopy(wimp_w, wimp_box *source, int x, int y);
typedef enum {
 wimp_EOK = 1, 
 wimp_ECANCEL = 2, 
 wimp_EHICANCEL = 4 
}wimp_errflags;
os_error *wimp_reporterror(os_error*, wimp_errflags, char *name);
os_error *wimp_sendmessage(wimp_etype code, wimp_msgstr* msg, wimp_t dest);
os_error *wimp_sendwmessage(
 wimp_etype code, wimp_msgstr *msg, wimp_w w, wimp_i i);
os_error *wimp_create_submenu(wimp_menustr *sub, int x, int y);
os_error *wimp_slotsize(int *currentslot ,
 int *nextslot ,
 int *freepool );
os_error *wimp_transferblock(
 wimp_t sourcetask,
 char *sourcebuf,
 wimp_t desttask,
 char *destbuf,
 int buflen);
os_error *wimp_setfontcolours(int foreground, int background);
os_error *wimp_readpixtrans(sprite_area *area, sprite_id *id, 
 sprite_factors *factors, sprite_pixtrans *pixtrans);
typedef enum
 { wimp_command_TITLE = 0,
 wimp_command_ACTIVE = 1,
 wimp_command_CLOSE_PROMPT = 2,
 wimp_command_CLOSE_NOPROMPT = 3
 } wimp_command_tag;
typedef struct
 {
 wimp_command_tag tag;
 char *title;
 } wimp_commandwind;
os_error *wimp_commandwindow(wimp_commandwind commandwindow);
#endif
