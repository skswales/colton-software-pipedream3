/* > h.print */

/* Title:   print.h
 * Purpose: access to Arthur printer driver facilities
 * Author:  R.C.Manby
 * History:
 *  25-Apr-88   RCM started
 *  02-Sep-88   RCM changes for new printer driver interface
 *  06-Oct-88   RCM added missing field to print_drawpage
 *  24-Oct-88   RCM added background colour field to print_giverectangle
 *                  implemented print_canceljob & print_screendump
 *  07-Jul-89   SKS modified drawpage
*/


#ifndef __wimplib__print_h
#define __wimplib__print_h


typedef enum
{
    print_PostScript     = 0,
    print_FX80compatible = 1

} print_identity;


typedef enum
{
    print_colour        = 0x0000001,    /* colour                               */
    print_limited       = 0x0000002,    /* if print_COLOUR bit set, full        */
                                        /* colour range not available           */
    print_discrete      = 0x0000004,    /* only a discrete colour set supported */

    print_NOFILL        = 0x0000100,    /* cannot handle filled shapes well     */
    print_NOTHICKNESS   = 0x0000200,    /* cannot handle thick lines well       */
    print_NOOVERWRITE   = 0x0000400,    /* cannot overwrite colours properly    */

    print_SCREENDUMP    = 0x1000000,    /* supports PDriver_ScreenDump          */
    print_TRANSFORM     = 0x2000000     /* supports arbitrary transformations   */
                                        /* (else only axis-preserving ones)     */
} print_features;


typedef struct print_infostr
{
    short int version;      /* version number * 100                     */
    short int identity;     /* driver identity (eg 0=Postscript,1=FX80) */
    int xres,yres;          /* x,y resolution (pixels/inch)             */
    int features;           /* see print_features                       */
    char *description;      /* printers supported, <=20chars + null     */
    int xhalf,yhalf;        /* halftone resolution (repeats/inch)       */
    int number;             /* configured printer number                */

} print_infostr;



typedef struct
{
    int x0, y0, x1, y1;

} print_box;

 
typedef struct print_pagesizestr
{
    int xsize,ysize;    /* size of page, including margins (1/72000 inch)   */
    print_box bbox;     /* bounding box of printable portion (1/72000 inch) */

} print_pagesizestr;


typedef struct print_transmatstr
{
    int xx,xy,yx,yy;

} print_transmatstr;


typedef struct print_positionstr
{
    int dx,dy;

} print_positionstr;


/* ------------------------- print_info -------------------------------------
 *
 *  read details of current printer driver
 *  (version, resolution, features etc)
*/

extern os_error *print_info(print_infostr *pinfo /*out*/);


/* ------------------------- print_setinfo ----------------------------------
 *
 * reconfigure printer driver
 *
 * Parameters:
 *  i->version      }
 *  i->identity     } not used
 *  i->description  }
 *  i->features     bit0 clear for set monochrome, set for colour
*/

extern os_error *print_setinfo(const print_infostr *i);


extern os_error *print_checkfeatures(int mask, int value);

extern os_error *print_pagesize(print_pagesizestr*);
extern os_error *print_setpagesize(const print_pagesizestr *p);

extern os_error *print_selectjob(int job, const char *title, int *oldjobp /*out*/);
extern os_error *print_currentjob(int *curjobp /*out*/);
extern os_error *print_endjob(int job);
extern os_error *print_abortjob(int job);
extern os_error *print_canceljob(int job);
extern os_error *print_reset(void);


/* ------------------------- print_giverectangle ----------------------------
 *
 * Parameters:
 *  ident               rectangle identification word
 *  -> (x0,y0,x1,y1)    rectangle to be plotted (OS coords)
 *  -> (xx,xy,yx,yy)    transformation matrix (fixed point, 16 binary places)
 *  -> (dx,dy)          posn. of bottom left of rectangle on page (1/72000 inch)
 *  bgcol               background colour for this rectangle, &BBGGRRXX
*/

extern os_error *print_giverectangle(int ident,
                                     const print_box *rect,
                                     const print_transmatstr *trans,
                                     const print_positionstr *botleft,
                                     int bgcol);


/* ------------------------- print_drawpage ---------------------------------
 *
 * Parameters:
 *  copies              number of copies
 *  -> (x0,y0,x1,y1)    to receive rectangle to print
 *  sequ                zero or pages sequence number within document
 *  -> page             zero or ptr to string, a textual page number (no spaces)
 *  -> more             to receive number of copies left to print
 *  -> ident            to receive rectangle identification word
*/

extern os_error *print_drawpage(int copies, print_box *clip /*out*/, int sequ,
                                const char *page,
                                int *more /*out*/, int *ident /*out*/);


/* ------------------------- print_getrectangle -----------------------------
 *
 * Parameters:
 *  -> (x0,y0,x1,y1)    to receive clip rectangle
 *  -> more             to receive number of rectangles left to print
 *  -> ident            to receive rectangle identification word
*/

extern os_error *print_getrectangle(print_box *clip /*out*/,
                                    int *more /*out*/,
                                    int *ident /*out*/);


extern os_error *print_screendump(int job);


#endif  /* __wimplib__print_h */


/* end of print.h */
