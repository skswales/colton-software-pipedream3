/* Title  > h.wimp
 * Purpose: C interface to RISC OS Wimp routines
 * Version: 0.1
 * Needs h.os
 *  DAHE, 31 Oct 1988: type in wimp_ploticon corrected.
 *  SKS 09-Feb-89: added definitions for system icons in windows
 *  SKS 10-Feb-89: wimp_openstr had scx,scy wrongly defined
*/

#ifndef __wimp_h
#define __wimp_h

#ifndef __os_h
#include "os.h"
#endif

/* necessary forward ref */

typedef struct wimp_menustr *wimp_menustrp;


typedef enum
{                                       /* window flag set */

    wimp_WMOVEABLE      = 0x00000002,   /* is moveable */
    wimp_REDRAW_OK      = 0x00000010,   /* can be redrawn entirely by wimp
                                         * ie. no user graphics */
    wimp_WPANE          = 0x00000020,   /* window is stuck over tool window */
    wimp_WTRESPASS      = 0x00000040,   /* window is allowed to go outside
                                         * main area */
    wimp_WSCROLL_R1     = 0x00000100,   /* scroll request returned when
                                         * scroll button clicked- auto repeat*/
    wimp_SCROLL_R2      = 0x00000200,   /* as SCROLL_R1, debounced, no auto */
    wimp_REAL_COLOURS   = 0x000000400,  /* use real window colours. */
    wimp_BACK_WINDOW    = 0x000000800,  /* this window is a background window. */

    wimp_WOPEN          = 0x00010000,   /* window is open */
    wimp_WTOP           = 0x00020000,   /* window is on top (not covered) */
    wimp_WFULL          = 0x00040000,   /* window is full size */

    wimp_WBACK          = 0x01000000,   /* window has back button */
    wimp_WQUIT          = 0x02000000,   /* has a quit button */
    wimp_WTITLE         = 0x04000000,   /* has a title bar */
    wimp_WTOGGLE        = 0x08000000,   /* has a toggle-size button */
    wimp_WVSCR          = 0x10000000,   /* has vertical scroll bar */
    wimp_WSIZE          = 0x20000000,   /* has size box */
    wimp_WHSCR          = 0x40000000/*,*/   /* has horizontal scroll bar */
    /*wimp_WNEW         = 0x80000000*/  /* use these new flags */
                                        /* NB! always set the WNEW flag */
}
wimp_wflags;


typedef enum
{
    wimp_WCTITLEFORE,
    wimp_WCTITLEBACK,
    wimp_WCWKAREAFORE,
    wimp_WCWKAREABACK,
    wimp_WCSCROLLOUTER,
    wimp_WCSCROLLINNER,
    wimp_WCTITLEHI,
    wimp_WCRESERVED
}
wimp_wcolours;
/* If work area background is 255 then it isn't painted. */
/* If title foreground is 255 then you get no borders, title etc. at all */


typedef enum
{                                   /* icon flag set */

    wimp_ITEXT      = 0x00000001,   /* icon contains text */
    wimp_ISPRITE    = 0x00000002,   /* icon is a sprite */
    wimp_IBORDER    = 0x00000004,   /* icon has a border */
    wimp_IHCENTRE   = 0x00000008,   /* text is horizontally centred */
    wimp_IVCENTRE   = 0x00000010,   /* text is vertically centred */
    wimp_IFILLED    = 0x00000020,   /* icon has a filled background */
    wimp_IFONT      = 0x00000040,   /* text is an anti-aliased font */
    wimp_IREDRAW    = 0x00000080,   /* redraw needs application's help */
    wimp_INDIRECT   = 0x00000100,   /* icon data is 'indirected' */
    wimp_IRJUST     = 0x00000200,   /* text right justified in box */
    wimp_IESG_NOC   = 0x00000400,   /* if selected by right button, don't
                                     * cancel other icons in same ESG */
    wimp_IHALVESPRITE=0x00000800,   /* plot sprites half-size */
    wimp_IBTYPE     = 0x00001000,   /* 4-bit field: button type */
    wimp_ISELECTED  = 0x00200000,   /* icon selected by user (inverted) */
    wimp_INOSELECT  = 0x00400000,   /* icon cannot be selected (shaded) */
    wimp_IDELETED   = 0x00800000,   /* icon has been deleted */
    wimp_IFORECOL   = 0x01000000,   /* 4-bit field: foreground colour */
    wimp_IBACKCOL   = 0x10000000    /* 4-bit field: background colour */
}
wimp_iconflags;

/* If the icon contains anti-aliased text then the colour fields
 * give the font handle
*/
#define wimp_IFONT 0x01000000


typedef enum
{                               /* button types */

    wimp_BIGNORE,               /* ignore all mouse ops */
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
    wimp_BCLICKSEL,             /* useful for on/off and radio buttons */
    wimp_BWRITABLE = 15
}
wimp_ibtype;


/* system icon handles (SKS) */

typedef enum
{
    wimp_SYSICON_WORKAREA       = -1,
    wimp_SYSICON_BACKBOX        = -2,
    wimp_SYSICON_QUITBOX        = -3,
    wimp_SYSICON_TITLEBAR       = -4,
    wimp_SYSICON_TOGGLEBOX      = -5,
    wimp_SYSICON_SCROLLUP       = -6,
    wimp_SYSICON_VSCROLLBAR     = -7,
    wimp_SYSICON_SCROLLDOWN     = -8,
    wimp_SYSICON_SIZEBOX        = -9,
    wimp_SYSICON_SCROLLEFT      = -10,
    wimp_SYSICON_HSCROLLBAR     = -11,
    wimp_SYSICON_SCROLLRIGHT    = -12,
    wimp_SYSICON_OUTLINE        = -13
}
wimp_systemiconhandles;


typedef enum
{                               /* button state bits */
    wimp_BRIGHT         = 0x001,
    wimp_BMID           = 0x002,
    wimp_BLEFT          = 0x004,
    wimp_BDRAGRIGHT     = 0x010,
    wimp_BDRAGLEFT      = 0x040,
    wimp_BCLICKRIGHT    = 0x100,
    wimp_BCLICKLEFT     = 0x400
}
wimp_bbits;


typedef enum
{
    wimp_MOVE_WIND      = 1,    /* change position of window */
    wimp_SIZE_WIND      = 2,    /* change size of window */
    wimp_DRAG_HBAR      = 3,    /* drag horizontal scroll bar */
    wimp_DRAG_VBAR      = 4,    /* drag vertical scroll bar */
    wimp_USER_FIXED     = 5,    /* user drag box - fixed size */
    wimp_USER_RUBBER    = 6,    /* user drag box - rubber box */
    wimp_USER_HIDDEN= 7     /* user drag box - invisible box */
}
wimp_dragtype;


/*****************************************************************************/

typedef int wimp_w; /* abstract window handle */
typedef int wimp_i; /* abstract icon handle */
typedef int wimp_t; /* abstract task handle */

typedef union
{                               /* the data field in an icon */
    char text[12];              /* up to 12 bytes of text */
    char sprite_name[12];       /* up to 12 bytes of sprite name */

    struct
    {
        const char *name;           /* pointer to sprite/sprite name */
        const void *spritearea;     /* 0 -> use the common sprite area */
                                    /* 1 -> use the wimp sprite area */
        BOOL        nameisname;     /* if FALSE, name is in fact a sprite pointer */
    }
    indirectsprite;

    struct
    {
        char *      buffer;         /* pointer to text buffer */
        const char *validstring;    /* pointer to validation string */
        int         bufflen;        /* length of text buffer */
    }
    indirecttext;
}
wimp_icondata;


typedef struct
{
    int x0, y0, x1, y1;
}
wimp_box;


typedef struct
{
    wimp_box        box;        /* screen coords of work area */
    int             scx, scy;   /* scroll bar positions */
    wimp_w          behind;     /* handle to open window behind, or -1 if top */
    wimp_wflags     flags;      /* word of flag bits defined above */
    char            colours[8]; /* colours: index using wimp_wcolours. */
    wimp_box        ex;         /* maximum extent of work area */
    wimp_iconflags  titleflags; /* icon flags for title bar */
    wimp_iconflags  workflags;  /* just button type relevant */
    void *          spritearea; /* 0->use the common sprite area */
                                /* 1->use the wimp sprite area */
    int             minsize;    /* two 16-bit OS-unit fields, (width/height)
                                 * giving min size of window. 0->use title. */
    wimp_icondata   title;      /* title icon data */
    int             nicons;     /* no. of icons in window */
}
wimp_wind;
/* If there are any icon definitions, they should follow this structure
 * immediately in memory.
*/


typedef struct
{                               /* result of get_info call. */
    wimp_w      w;
    wimp_wind   info;
}
wimp_winfo;
/* Space for icons must follow. */


typedef struct
{                               /* icon description structure */
    wimp_box        box;        /* bounding box - relative to
                                 * window origin (work area top left) */
    wimp_iconflags  flags;      /* word of flag bits defined above */
    wimp_icondata   data;       /* union of bits & bobs as above */
}
wimp_icon;


typedef struct
{                               /* structure for creating icons. */
    wimp_w      w;
    wimp_icon   i;
}
wimp_icreate;


typedef struct {
    wimp_w      w;              /* window handle */
    wimp_box    box;            /* position on screen of visible work area */
    int         scx, scy;       /* scroll offsets */
    wimp_w      behind;         /* handle of window to go behind
                                 * -1 = top, -2 = bottom */
}
wimp_openstr;


typedef struct
{                               /* result for window state enquiry */
    wimp_openstr    o;
    wimp_wflags     flags;
}
wimp_wstate;


typedef enum
{                               /* event types */
    wimp_ENULL,                 /* null event */
    wimp_EREDRAW,               /* redraw event */
    wimp_EOPEN,
    wimp_ECLOSE,
    wimp_EPTRLEAVE,
    wimp_EPTRENTER,
    wimp_EBUT,                  /* mouse button change */
    wimp_EUSERDRAG,
    wimp_EKEY,
    wimp_EMENU,
    wimp_ESCROLL,
    wimp_ELOSECARET,
    wimp_EGAINCARET,
    wimp_ESEND = 17,            /* send message, don't worry if it doesn't arrive */
    wimp_ESENDWANTACK = 18,     /* send message, return ack if not acknowledged */
    wimp_EACK = 19              /* acknowledge receipt of message. */
}
wimp_etype;


typedef enum
{                               /* event type masks */
    wimp_EMNULL     = 1 << wimp_ENULL,
    wimp_EMREDRAW   = 1 << wimp_EREDRAW,
    wimp_EMOPEN     = 1 << wimp_EOPEN,
    wimp_EMCLOSE    = 1 << wimp_ECLOSE,
    wimp_EMPTRLEAVE = 1 << wimp_EPTRLEAVE,
    wimp_EMPTRENTER = 1 << wimp_EPTRENTER,
    wimp_EMBUT      = 1 << wimp_EBUT,
    wimp_EMUSERDRAG = 1 << wimp_EUSERDRAG,
    wimp_EMKEY      = 1 << wimp_EKEY,
    wimp_EMMENU     = 1 << wimp_EMENU,
    wimp_EMSCROLL   = 1 << wimp_ESCROLL
}
wimp_emask;


typedef struct
{
    wimp_w      w;
    wimp_box    box;            /* work area coordinates */
    int         scx, scy;       /* scroll bar positions */
    wimp_box    g;              /* current graphics window */
}
wimp_redrawstr;


typedef struct
{
    int         x, y;           /* mouse x and y */
    wimp_bbits  bbits;          /* button state */
    wimp_w      w;              /* window handle, or -1 if none */
    wimp_i      i;              /* icon handle, or -1 if none */
}
wimp_mousestr;


typedef struct
{
    wimp_w  w;
    wimp_i  i;
    int     x, y;               /* offset relative to window origin */
    int     height;             /* -1 if calc within icon
                                 * bit 24 -> VDU-5 type caret
                                 * bit 25 -> caret invisible
                                 * bit 26 -> bits 16..23 contain colour
                                 * bit 27 -> colour is "real" colour */
    int     index;              /* position within icon */
}
wimp_caretstr;


/* Message action codes are allocated just like SWI codes. */
typedef enum
{
    wimp_MCLOSEDOWN         = 0,    /* Reply if any dialogue with the user is required,
                                     * and the closedown sequence will be aborted. */
    wimp_MDATASAVE          = 1,    /* request to identify directory */
    wimp_MDATASAVEOK        = 2,    /* reply to message type 1 */
    wimp_MDATALOAD          = 3,    /* request to load/insert dragged icon */
    wimp_MDATALOADOK        = 4,    /* reply that file has been loaded */
    wimp_MDATAOPEN          = 5,    /* warning that an object is to be opened */
    wimp_MRAMFETCH          = 6,    /* transfer data to buffer in my workspace */
    wimp_MRAMTRANSMIT       = 7,    /* I have transferred some data to a buffer in your workspace */
    wimp_MPREQUIT           = 8,
    wimp_PALETTECHANGE      = 9,

    wimp_FilerOpenDir       = 0x0400,
    wimp_FilerCloseDir      = 0x0401,

    wimp_MHELPREQUEST       = 0x0502,       /* interactive help request */
    wimp_MHELPREPLY         = 0x0503,       /* interactive help message */

    wimp_MNOTIFY            = 0x40040,      /* Econet notify */

    wimp_MMENUWARN          = 0x400c0,
  /* menu warning. Sent if wimp_MSUBLINKMSG set. Data sent is:
         submenu field of relevant wimp_menuitem.
         screen x-coord
         screen y-coord
         list of menu selection indices (0..n-1 for each menu)
         terminating -1 word.
     Typical response is to call wimp_create_submenu.
  */
    wimp_MMODECHANGE        = 0x400c1,
    wimp_MINITTASK          = 0x400c2,
    wimp_MCLOSETASK         = 0x400c3,
    wimp_MSLOTCHANGE        = 0x400c4,  /* Slot size has altered */
    wimp_MSETSLOT           = 0x400c5,
    wimp_MTASKNAMERQ        = 0x400c6,
    wimp_MTASKNAMEIS        = 0x400c7,

    /* Messages for dialogue with printer applications */

    wimp_MPrintFile         = 0x80140,  /* Printer app's first response to a DATASAVE */
    wimp_MWillPrint         = 0x80141,  /* Acknowledgement of PrintFile */
    wimp_MPrintTypeOdd      = 0x80145,  /* Broadcast when strange files
                                         * dropped on the printer */
    wimp_MPrintTypeKnown    = 0x80146,  /* Ack to above */
    wimp_MPrinterChange     = 0x80147,  /* New printer application installed */

    wimp_MPD_DDE            = 0x0600    /* PipeDream dynamic data exchange */
}
wimp_msgaction;


typedef struct
{                                       /* message block header. */
    int             size;               /* size of the whole msgstr: 20<=size<=256, multiple of 4 */
    wimp_t          task;               /* task handle of sender (filled in by wimp) */
    int             my_ref;             /* unique ref number (filled in by wimp) */
    int             your_ref;           /* (0==>none) if non-zero, acknowledge */
    wimp_msgaction  action;             /* message action code */
}
wimp_msghdr;
#define wimp_MSGBODY_SIZE   (256 - sizeof(wimp_msghdr))


typedef struct
{
    wimp_w  w;                          /* window in which save occurs */
    wimp_i  i;                          /* icon there */
    int     x;                          /* position within that window of destination of save */
    int     y;
    int     estsize;                    /* estimated size of data, in bytes */
    int     type;                       /* file type of data to save */
    char    leaf[12];                   /* proposed leaf-name of file, 0-terminated */
}
wimp_msgdatasave;


typedef struct
{
/* w, i, x, y, type, estsize copied unaltered from DataSave message. */
    wimp_w  w;                          /* window in which save occurs */
    wimp_i  i;                          /* icon there */
    int     x;                          /* position within that window of destination of save */
    int     y;
    int     estsize;                    /* estimated size of data, in bytes */
    int     type;                       /* file type of data to save */
    char    name[wimp_MSGBODY_SIZE-24]; /* the name of the file to save */
}
wimp_msgdatasaveok;


typedef struct
{
    wimp_w  w;                          /* target window */
    wimp_i  i;                          /* target icon */
    int     x;                          /* target coords in target window work area */
    int     y;
    int     size;                       /* must be 0 */
    int     type;                       /* type of file */
    char    name[wimp_MSGBODY_SIZE-24]; /* the filename follows. */
}
wimp_msgdataload;

/* for a dataloadok, no arguments are required. */


typedef wimp_msgdataload wimp_msgdataopen;
/* The data provided when opening a file is exactly the same. the
 * window, x, y refer to the bottom lh corner of the icon that represents the
 * file being opened, or w=-1 if there is no such.
*/


typedef struct
{                                       /* transfer data in memory */
    char *  addr;                       /* address of data to transfer */
    int     nbytes;                     /* number of bytes to transfer */
}
wimp_msgramfetch;


typedef struct
{                                       /* I have transferred some data to a buffer in your workspace */
    char *  addr;                       /* copy of value sent in RAMfetch */
    int     nbyteswritten;              /* number of bytes written */
}
wimp_msgramtransmit;


typedef struct
{
    int flags;                          /* where the help is required */
}
wimp_msgprequitrequest;


typedef enum
{
    wimp_MPREQUIT_flags_killthistask    = 0x01
}
wimp_MPREQUIT_flags;


typedef struct
{
    wimp_mousestr m;                    /* where the help is required */
}
wimp_msghelprequest;


typedef struct
{
    char text[wimp_MSGBODY_SIZE];       /* the helpful string */
}
wimp_msghelpreply;


typedef struct
{
    union
    {
        wimp_menustrp   m;
        wimp_w          w;
    }
    submenu;
    
    int             x;                  /* where to open the submenu */
    int             y;

    int             menu[10];           /* list of menu selection indices, -1 at end */
}
wimp_msgmenuwarning;


typedef struct
{                                       /* structure used in all print messages */
    int     filler[5];
    int     type;                       /* filetype */
    char    name[wimp_MSGBODY_SIZE-24]; /* filename */
}
wimp_msgprint;


typedef enum
{                                           /* ddetype */
/* PD tramsmits */
    wimp_MPD_DDE_SendSlotContents = 1,  /* C, no reply expected */
    wimp_MPD_DDE_SheetClosed,               /* B, no reply expected  */
    wimp_MPD_DDE_ReturnHandleAndBlock,  /* A, no reply expected  */

/* PD receives */
    wimp_MPD_DDE_IdentifyMarkedBlock,       /* E; replies with ReturnHandleAndBlock */
    wimp_MPD_DDE_EstablishHandle,           /* A; replies with ReturnHandleAndBlock */
    wimp_MPD_DDE_RequestUpdates,            /* B; no immediate reply, updates do SendSlotContents */
    wimp_MPD_DDE_RequestContents,           /* B; replies with 0 or more SendSlotContents */
    wimp_MPD_DDE_GraphClosed,               /* B; no reply */
    wimp_MPD_DDE_DrawFileChanged,           /* D; no reply */
    wimp_MPD_DDE_StopRequestUpdates         /* B; no immediate reply, updates don't SendSlotContents */
}
wimp_msgpd_dde_id;


typedef char wimp_msgpd_ddetypeA_text[wimp_MSGBODY_SIZE-4-12];

typedef struct
{
    int                         handle;
    int                         xsize;
    int                         ysize;
    wimp_msgpd_ddetypeA_text    text;       /* leafname & tag, both 0-terminated */
}
wimp_msgpd_ddetypeA;


typedef struct
{
    int     handle;
}
wimp_msgpd_ddetypeB;


typedef enum
{
    wimp_MPD_DDE_typeC_type_Text,
    wimp_MPD_DDE_typeC_type_Number,
    wimp_MPD_DDE_typeC_type_End
}
wimp_msgpd_ddetypeC_type;


typedef char wimp_msgpd_ddetypeC_text[wimp_MSGBODY_SIZE-4-16];

typedef struct
{
    int                         handle;
    int                         xoff;
    int                         yoff;
    wimp_msgpd_ddetypeC_type    type;
    union
    {
        wimp_msgpd_ddetypeC_text    text;       /* textual content 0-terminated */
        double                      number;
    }
    content;
}
wimp_msgpd_ddetypeC;


typedef struct
{
    char    leafname[wimp_MSGBODY_SIZE-4];      /* leafname of DrawFile, 0-terminated */
}
wimp_msgpd_ddetypeD;


typedef struct
{                                       /* structure used in all PD DDE messages */
    wimp_msgpd_dde_id   id;

    union
    {
        wimp_msgpd_ddetypeA a;
        wimp_msgpd_ddetypeB b;
        wimp_msgpd_ddetypeC c;
        wimp_msgpd_ddetypeD d;
    /*  wimp_msgpd_ddetypeE e;  (no body) */
    }
    type;
}
wimp_msgpd_dde;


typedef struct
{                                       /* message block */
    wimp_msghdr hdr;

    union
    {
        char                    chars[wimp_MSGBODY_SIZE];
        int                     words[wimp_MSGBODY_SIZE / 4];   /* max data size. */
        wimp_msgdatasave        datasave;
        wimp_msgdatasaveok      datasaveok;
        wimp_msgdataload        dataload;
        wimp_msgdataopen        dataopen;
        wimp_msgramfetch        ramfetch;
        wimp_msgramtransmit     ramtransmit;
        wimp_msgprequitrequest  prequitrequest;
        wimp_msghelprequest     helprequest;
        wimp_msghelpreply       helpreply;
        wimp_msgmenuwarning     menuwarning;
        wimp_msgprint           print;
        wimp_msgpd_dde          pd_dde;
    }
    data;
}
wimp_msgstr;


typedef union
{
    wimp_openstr    o;                  /* for redraw, close, enter, leave events */

    struct
    {                                   /* for button change event */
        wimp_mousestr m;
        wimp_bbits b;
    }
    but;

    wimp_box        dragbox;            /* for user drag box event */

    struct
    {                                   /* for key events */
        wimp_caretstr   c;
        int             chcode;
    }
    key;

    int             menu[10];           /* for menu event: terminated by -1 */

    struct
    {                                   /* for scroll request */
        wimp_openstr    o;
        int             x, y;           /* x=-1 for left, +1 for right */
                                        /* y=-1 for down, +1 for up */
                                        /* scroll by +/-2 -> page scroll request */
    }
    scroll;

    wimp_caretstr       c;              /* for caret gain/lose. */
    wimp_msgstr         msg;            /* for messages. */
}
wimp_eventdata;


typedef struct
{                                       /* wimp event description */
    wimp_etype      e;                  /* event type */
    wimp_eventdata  data;
}
wimp_eventstr;


typedef struct
{
    char    title[12];                  /* menu title (optional) */
    char    tit_fcol, tit_bcol, work_fcol, work_bcol; /* colours */
    int     width, height;              /* size of following menu items */
    int     gap;                        /* vertical gap between items */
}
wimp_menuhdr;


typedef enum
{                                       /* menu item flag set */
    wimp_MTICK          = 1,
    wimp_MSEPARATE      = 2,
    wimp_MWRITABLE      = 4,
    wimp_MSUBLINKMSG    = 8,            /* show a => flag, and inform program when it
                                         * is activated. */
    wimp_MLAST          = 0x80          /* signal last item in the menu */
}
wimp_menuflags;
/* use wimp_INOSELECT to shade the item as unselectable,
 * and the button type to mark it as writable.
*/


typedef struct
{
    wimp_menuflags  flags;          /* menu entry flags */

    union
    {
        wimp_menustrp   m;          /* wimp_menustr * pointer to submenu,
        wimp_w          w;           * or wimp_w dialogue box,
                                     * or -1 if no submenu */
    }
    submenu;

    wimp_iconflags  iconflags;      /* icon flags for the entry */
    wimp_icondata   data;           /* icon data for the entry */
}
wimp_menuitem;
/* submenu can also be a wimp_w, in which case the window is opened as a
 * dialogue box within the menu tree.
*/


typedef struct
wimp_menustr
{
    wimp_menuhdr    hdr;
/*  wimp_menuitem   item[]; */
}
wimp_menustr;


typedef struct
{
    wimp_w          window;
    wimp_dragtype   type;
    wimp_box        box;            /* initial position for drag box */
    wimp_box        parent;         /* parent box for drag box */
}
wimp_dragstr;


typedef struct
{
    wimp_w  window;                 /* handle */
    int     bit_mask;               /* bit set => consider this bit */
    int     bit_set;                /* desired bit setting */
}
wimp_which_block;


typedef struct
{
    int         shape_num;          /* pointer shape number (0 -> turn off pointer) */
    const char *shape_data;         /* shape data, NULL pointer -> existing shape */
    int         width, height;      /* Width and height in pixels. Width = 4 * n,
                                     * where n is an integer. */
    int         activex, activey;   /* active point (pixels from top left) */
}
wimp_pshapestr;


typedef struct
{
    char f[256];                    /* initialise all to zero before using for
                                     * first load_template, then just use
                                     * repeatedly without altering */
} wimp_font_array;


typedef struct
{                                   /* template reading structure */
    int                 reserved;   /* ignore - implementation detail */
    wimp_wind *         buf;        /* pointer to space for putting template in */
    char *              work_free;  /* pointer to start of free wimp workspace -
                                     * you have to provide the wimp system with
                                     * workspace to store its redirected icons in*/
    char *              work_end;   /* end of workspace you are offerring to wimp*/
    wimp_font_array *   font;       /* points to font reference count array, 0
                                     * pointer implies fonts not allowed */
    char *              name;       /* name to match with (can be wildcarded) */
    int                 index;      /* pos. in index to search from (0 = start) */
}
wimp_template;


typedef union
{
    struct
    {
        char gcol;
        char red;
        char green;
        char blue;
    }
    bytes;

    int word;
}
wimp_paletteword;
/* The gcol char (least significant) is a gcol colour except in 8-bpp
 * modes, when bits 0..2 are the tint and bits 3..7 are the gcol colour.
*/


typedef struct
{
    wimp_paletteword c[16];           /* wimp colours 0..15 */
    wimp_paletteword screenborder, mouse1, mouse2, mouse3;
}
wimp_palettestr;


/*****************************************************************************/

os_error *wimp_initialise(int * v);
/* Close & delete all windows, return wimp version number. */

os_error *wimp_taskinit(const char *name, wimp_t *t /*out*/);
/* Name is the name of the program. */
/* Used instead of wimp_initialise. Returns your task handle. */

os_error *wimp_create_wind(wimp_wind *, wimp_w *);
/* define (but not display) window, return window handle */

os_error *wimp_create_icon(wimp_icreate *, wimp_i *result);
/* add icon definition to that of window, return icon handle */

os_error *wimp_delete_wind(wimp_w);

os_error *wimp_delete_icon(wimp_w, wimp_i);

os_error *wimp_open_wind(wimp_openstr *);
/* make window appear on screen */

os_error *wimp_close_wind(wimp_w);
/* Remove from active list the window with handle in integer argument. */

os_error *wimp_poll(wimp_emask mask, wimp_eventstr *result);
void wimp_save_fp_state_on_poll(void) ;
/* Activates saving of floating point state on calls to wimp_poll
 * and wimp_pollidle; this is needed if you do any floating point at
 * all, as other programs may corrupt the FP status word, which is
 * effectively a global in your program
 */
void wimp_corrupt_fp_state_on_poll(void) ;
/* Disables saving of floating point state on calls to wimp_poll
 * and wimp_pollidle; use only if you never use FP at all
 */

os_error *wimp_redraw_wind(wimp_redrawstr*, BOOL* /*out*/);
/* Draw window outline and icons. Return FALSE if there's nothing to draw. */

os_error *wimp_update_wind(wimp_redrawstr*, BOOL* /*out*/);
/* Return visible portion of window. Return FALSE if nothing to redraw. */

os_error *wimp_get_rectangle(wimp_redrawstr*, BOOL*);
/* return next rectangle in list, or FALSE if done. */

os_error *wimp_get_wind_state(wimp_w, wimp_wstate * result);
/* read current window state */

os_error *wimp_get_wind_info(wimp_winfo * result);
/* On entry result->w gives the window in question. Space for any
icons must follow *result. */

os_error *wimp_set_icon_state(wimp_w, wimp_i, wimp_iconflags value, wimp_iconflags mask);

os_error *wimp_get_icon_info(wimp_w, wimp_i, wimp_icon * result);

os_error *wimp_get_point_info(wimp_mousestr * result);
/* Some early versions of the wimp had a bug whereby a superfluous
work was returned after the mousestr. This library takes action
to remove this fault. */

os_error *wimp_drag_box(wimp_dragstr *);
/* start the wimp dragging a box */

os_error *wimp_force_redraw(wimp_redrawstr * r);
/* Mark an area of the screen as invalid.
If r->wimp_w == -1 then use screen coordinates. Only the first
five fields of r are valid. */

os_error *wimp_set_caret_pos(wimp_caretstr *);
/* set pos./size of text caret */

os_error *wimp_get_caret_pos(wimp_caretstr *);
/* get pos./size of text caret */

os_error *wimp_create_menu(wimp_menustr *m, int x, int y);
/* 'pop up' menu structure. Set m==(wimp_menustr*)-1 to clear the
menu tree. */

os_error *wimp_decode_menu(wimp_menustr *, void *, void *);

os_error *wimp_which_icon(wimp_which_block *, wimp_i * results);
/* The results appear in an array, terminated by a (wimp_i) -1. */

os_error *wimp_set_extent(wimp_redrawstr *);
/* Alter extent of a window's work area - only handle and 1st set of
4 coordinates are looked at. */

os_error *wimp_set_point_shape(wimp_pshapestr *);
/* set pointer shape on screen */

os_error *wimp_open_template(const char * name);
/* opens named file to allow load_template to
read a template from the file. */

os_error *wimp_close_template(void);
/* close currently open template file */

os_error *wimp_load_template(wimp_template *);
/* load a window template from open file into buffer pointed to by
 * temp_blockname.buf. */

os_error *wimp_processkey(int chcode);
/* Hand back to the wimp a key that you do not understand. */

os_error *wimp_closedown(void);
os_error *wimp_taskclose(wimp_t);

os_error *wimp_taskclose(wimp_t);
/* Calls closedown in the multi-tasking form. */

os_error *wimp_starttask(const char *clicmd);
/* Start a new wimp task, with the given CLI command. */

os_error *wimp_getwindowoutline(wimp_redrawstr *r /*out*/);
/* set r->w on entry. On exit, r->box will be the screen coordinates
of the window, including border, title, scroll bars. */

os_error *wimp_pollidle(wimp_emask mask, wimp_eventstr *result /*out*/, int earliest);
/* Like wimp_poll, but do not return before earliest return time.
This is a value produced by OS_ReadMonotonicTime. */

os_error *wimp_ploticon(const wimp_icon *);
/* Called only within update or redraw loop. Just does the plotting,
this need not be a real icon attatched to a window. */

os_error *wimp_setmode(int mode);
/* Set the screen mode. Palette colours are maintained, if possible. */

os_error *wimp_readpalette(wimp_palettestr* /*out*/);

os_error *wimp_setpalette(wimp_palettestr*);
/* The bytes.gcol values of each field of the palettestr are ignored,
 * only the absolute colours are taken into account.
*/

os_error *wimp_setcolour(int colour);
/* bits 0..3 = wimp colour (translate for current mode)
 *      4..6 = gcol action
 *      7    = foreground/background.
*/
os_error *wimp_textcolour(int colour);

os_error *wimp_spriteop(int reason_code, const char *name);
/* call SWI Wimp_SpriteOp */
os_error *wimp_spriteop_full(os_regset *);
/* call SWI Wimp_SpriteOp allowing full information to be passed */

void *wimp_baseofsprites(void);
/* Returns a sprite_area*. This may be moved about by mergespritefile. */

os_error *wimp_blockcopy(wimp_w, const wimp_box *source, int x, int y);
/* Copy the source box (defined in window coords) to the given destination
 * (in window coords). Invalidate any portions of the destination that cannot be
 * updated using on-screen copy.
*/

typedef enum
{
    wimp_EOK        = 1,        /* put in "OK" box */
    wimp_ECANCEL    = 2,        /* put in "CANCEL" box */
    wimp_EHICANCEL  = 4     /* highlight CANCEL rather than OK. */
}
wimp_errflags;
/* If OK and CANCEL are both 0 you get an OK. */

os_error *wimp_reporterror(os_error *, wimp_errflags, const char *name);
/* Produces an error window. Uses sprite called "error" in the wimp sprite
 * pool. name should be the program name, appearing after "error in " at the
 * head of the dialogue box.
*/

os_error *wimp_sendmessage(wimp_etype code, wimp_msgstr* msg, wimp_t dest);
/* dest can also be 0, in which case the message is sent to every task in
 * turn, including the sender. msg can also be any other wimp_eventdata* value.
*/

os_error *wimp_sendwmessage(wimp_etype code, wimp_msgstr *msg, wimp_w w, wimp_i i);
/* Send a message to the owner of a specific window/icon. msg can also be
 * any other wimp_eventdata* value.
*/

os_error *wimp_create_submenu(const wimp_menustr *sub, int x, int y);
/* sub can also be a wimp_w, in which case it is opened by the wimp
 * as a dialogue box.
*/

os_error *wimp_slotsize(int *currentslot /*inout*/,
                        int *nextslot /*inout*/,
                        int *freepool /*out*/);
/* currentslot/nextslot==0 -> just read setting. */

os_error *wimp_transferblock(
    wimp_t sourcetask,
    const char *sourcebuf,
    wimp_t desttask,
    char *destbuf,
    int buflen);
/* Transfer memory between domains. */

os_error *wimp_setfontcolours(int foreground, int background);
/* Set font manager colours. The wimp handles how many shades etc. to use. */

#endif  /* __wimp_h */


/* end of wimp.h */
