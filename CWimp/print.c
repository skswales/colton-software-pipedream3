/* > c.print */

/* Title:   print.c
 * Purpose: provide RISC OS printer driver access
 * Author:  R.C.Manby
 * History:
 *  25-Apr-88   RCM started
 *  02-Sep-88   RCM changes for new printer driver interface
 *  06-Oct-88   RCM added missing field to print_drawpage
 *  24-Oct-88   RCM added background colour field to print_giverectangle
 *                  implemented print_canceljob & print_screendump
 *  07-Jul-89   SKS changed drawpage
*/

#include "trace.h"

#include "os.h"

#include "print.h"


/* Printer Driver SWIs */

#define Info          0x00080140
#define SetInfo       0x00080141
#define CheckFeatures 0x00080142
#define PageSize      0x00080143
#define SetPageSize   0x00080144
#define SelectJob     0x00080145
#define CurrentJob    0x00080146
/* FontSWI goes in here - only the font manager may use this */
#define EndJob        0x00080148
#define AbortJob      0x00080149
#define Reset         0x0008014A
#define GiveRectangle 0x0008014B
#define DrawPage      0x0008014C
#define GetRectangle  0x0008014D
#define CancelJob     0x0008014E
#define ScreenDump    0x0008014F


extern os_error *
print_info(print_infostr *i /*out*/)
{
    os_regset r;
    os_error *e = os_swix(Info, &r);

    i->version      = r.r[0] & 0xFFFF;
    i->identity     = r.r[0] >> 16;
    i->xres         = r.r[1];
    i->yres         = r.r[2];
    i->features     = r.r[3];
    i->description  = (char *) r.r[4];
    i->xhalf        = r.r[5];
    i->yhalf        = r.r[6];
    i->number       = r.r[7];

    return(e);
}


#if !defined(SMALL)

extern os_error *
print_setinfo(const print_infostr *i)
{
    os_regset r;

    r.r[1] = i->xres;
    r.r[2] = i->yres;
    r.r[3] = i->features;
    r.r[5] = i->xhalf;
    r.r[6] = i->yhalf;
    r.r[7] = i->number;

    return(os_swix(SetInfo, &r));
}


extern os_error *
print_checkfeatures(int mask, int value)
{
    os_regset r;

    r.r[0] = mask;
    r.r[1] = value;

    return(os_swix(CheckFeatures, &r));
}

#endif  /* SMALL */


extern os_error *
print_pagesize(print_pagesizestr *p /*out*/)
{
    os_regset r;
    os_error *e = os_swix(PageSize, &r);

    p->xsize    = r.r[1];
    p->ysize    = r.r[2];
    p->bbox.x0  = r.r[3];
    p->bbox.y0  = r.r[4];
    p->bbox.x1  = r.r[5];
    p->bbox.y1  = r.r[6];

    return(e);
}


#if !defined(SMALL)

extern os_error *
print_setpagesize(const print_pagesizestr *p)
{
    os_regset r;

    r.r[1] = p->xsize;
    r.r[2] = p->ysize;
    r.r[3] = p->bbox.x0;
    r.r[4] = p->bbox.y0;
    r.r[5] = p->bbox.x1;
    r.r[6] = p->bbox.y1;

    return(os_swix(SetPageSize, &r));
}

#endif


extern os_error *
print_selectjob(int job, const char *title, int *oldjobp /*out*/)
{
    os_regset r;
    os_error *e;

    r.r[0] = job;
    r.r[1] = (int) title;

    e = os_swix(SelectJob,&r);

    if(!e)
        *oldjobp = r.r[0];

    return(e);
} 


#if !defined(SMALL)

extern os_error *
print_currentjob(int *curjobp /*out*/)
{
    os_regset r;
    os_error *e = os_swix(CurrentJob, &r);

    if(!e)
        *curjobp = r.r[0];

    return(e);
} 

#endif


static os_error *
abort_end_cancel(int job, int swicode)
{
    os_regset r;

    r.r[0] = job;

    return(os_swix(swicode, &r));
}


extern os_error *
print_endjob(int job)
{
    return(abort_end_cancel(job, EndJob));
}


extern os_error *
print_abortjob(int job)
{
    return(abort_end_cancel(job, AbortJob));
}


#if !defined(SMALL)

extern os_error *
print_canceljob(int job)
{
  return(abort_end_cancel(job, CancelJob));
}

#endif  /* SMALL */


#if !defined(SMALL)  ||  TRACE

extern os_error *
print_reset(void)
{
    return(abort_end_cancel(0, Reset));
}

#endif


extern os_error *
print_giverectangle(int ident,
                    const print_box *area,
                    const print_transmatstr *trans,
                    const print_positionstr *posn, int bgcol)
{
    os_regset r;

    r.r[0] = ident;
    r.r[1] = (int) area;
    r.r[2] = (int) trans;
    r.r[3] = (int) posn;
    r.r[4] = bgcol;

    return(os_swix(GiveRectangle, &r));
}


extern os_error *
print_drawpage(int copies, print_box *clip /*out*/, int sequ,
               const char *page, int *more /*out*/, int *ident /*out*/)
{
    os_regset r;
    os_error *e;

    r.r[0] = copies;
    r.r[1] = (int) clip;
    r.r[2] = sequ;
    r.r[3] = (int) page;

    e = os_swix(DrawPage, &r);

    if(!e)
        {
        *more  = r.r[0];
        *ident = r.r[2];
        }

    return(e);
} 


extern os_error *
print_getrectangle(print_box *clip /*out*/, int *more /*out*/, int *ident /*out*/)
{
    os_regset r;
    os_error *e;

    r.r[1] = (int) clip;

    e = os_swix(GetRectangle, &r);

    if(!e)
        {
        *more  = r.r[0];
        *ident = r.r[2];
        }

    return(e);
} 


#if !defined(SMALL)

extern os_error *
print_screendump(int job)
{
    os_regset r;

    r.r[0] = job;

    return(os_swix(ScreenDump, &r));
}

#endif  /* SMALL */


/* end of print.c */
