/* Title: -> c.xfersend
 * Purpose: generalised data transfer to a concurrent wimp program.
 */

#define BOOL int
#define TRUE 1
#define FALSE 0

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trace.h"
#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "wimpt.h"
#include "win.h"
#include "dbox.h"
#include "xfersend.h"
#include "fileicon.h"
#include "werr.h"
#include "menu.h"
#include "event.h"
#include "msgs.h"

static int rcvbufsize ;
static int xfersend__msgid = 0;
static xfersend_saveproc xfersend__saveproc;
static xfersend_sendproc xfersend__sendproc;
static xfersend_printproc xfersend__printproc;
static int xfersend__filetype;
static void *xfersend__savehandle;
static int xfersend__estsize = 0;
static wimp_t xfersend__receiver ;
static BOOL xfersend__fileissafe ;
static char *xfersend__filename;  /*[256]*/

static wimp_mousestr xfersend__mousestr;
static wimp_msgstr xfersend__msg;
static BOOL xfersend__close = FALSE;
static wimp_w xfersend__w;

#define OS_File 8


void xfersend_close_on_xfer(BOOL do_we_close, wimp_w w)
{  xfersend__close = do_we_close;
   xfersend__w = w;
}

static void xfersend__winclose(void)
{  wimp_eventdata e;
   e.o.w = xfersend__w;
   wimpt_noerr(wimp_sendwmessage(wimp_ECLOSE, (wimp_msgstr*) &e, e.o.w, -1));
}


static BOOL xfersend__unknowns(wimp_eventstr *e, void *handle)

{
  handle = handle ;
  tracef1("xfersend raw event %i.\n", e->e);
  switch (e->e)
  { case wimp_EUSERDRAG:
    tracef0("drag event received.\n");
    {
      wimp_get_point_info(&xfersend__mousestr);
      if (xfersend__mousestr.w == -1)
      {
        tracef0("drag to no window: has no effect.\n");
        /* do nothing */
      }
      else
      {
        wimp_msgstr msg;

        tracef1("drag to window %i: offer data.\n", xfersend__mousestr.w);
        msg.hdr.size = sizeof(wimp_msghdr) + sizeof(wimp_msgdatasave);
        msg.hdr.task = xfersend__mousestr.w;
        msg.hdr.your_ref = 0;
        msg.hdr.action = wimp_MDATASAVE;
        msg.data.datasave.w = xfersend__mousestr.w;
        msg.data.datasave.i = xfersend__mousestr.i;
        msg.data.datasave.x = xfersend__mousestr.x;
        msg.data.datasave.y = xfersend__mousestr.y;
        msg.data.datasave.type = xfersend__filetype;
        msg.data.datasave.estsize = xfersend__estsize;
        {
          int i, tail;
          char name[256];
          if (xfersend__filename == 0) xfersend__filename = malloc(256);
          strncpy(name,xfersend__filename,  256);
          tail = strlen(name); /* point at the zero */
          while (tail > 0 && name[tail-1] != '.' && name[tail-1] != ':')
               tail--;

          for (i = 0; i <= 10; i++) msg.data.datasave.leaf[i] = name[tail++];
          msg.data.datasave.leaf[11] = '\0' ;     /* force termination */
          tracef1("suggest leaf '%s'.\n", (int) &msg.data.datasave.leaf[0]);
        }
        wimpt_noerr(wimp_sendwmessage(
           wimp_ESEND, &msg, xfersend__mousestr.w, xfersend__mousestr.i));
        xfersend__msgid = msg.hdr.my_ref; /* filled in by wimp. */
        /* We still get unknown events, so we'll get the reply sometime. */
      }
    }
    return TRUE;

  case wimp_ESEND:
  case wimp_ESENDWANTACK:

    tracef3 ("xfersend msg %x received: %i %i.\n",
            e->data.msg.hdr.action,e->data.msg.hdr.your_ref,xfersend__msgid);

    if (e->data.msg.hdr.your_ref == xfersend__msgid)
     switch (e->data.msg.hdr.action)
     {
      case wimp_MRAMFETCH:
       if (xfersend__sendproc != 0)
       {
        xfersend__fileissafe = FALSE ;

        /* He wants to do an in-core transfer, and we can do this. */
        /* Note that this can't be other than the first response, as others
           are grabbed by sendbuf */

        tracef0("ram transfer starting.\n");

        /* Prepare the reply record. */
        xfersend__msg = e->data.msg;
        xfersend__msg.hdr.your_ref = xfersend__msg.hdr.my_ref;
        xfersend__msg.hdr.action = wimp_MRAMTRANSMIT;
        xfersend__msg.data.ramtransmit.addr = e->data.msg.data.ramfetch.addr;
        xfersend__msg.data.ramtransmit.nbyteswritten = 0; /* so far. */
        rcvbufsize = e->data.msg.data.ramfetch.nbytes;

        xfersend__receiver = e->data.msg.hdr.task ;
        /* the copy in xfersend__msg.hdr.task is overwritten by the Wimp
           message sending */

        if (xfersend__sendproc(xfersend__savehandle, &rcvbufsize))
        {
         /* See sendbuf for all the real work for this case... */
         tracef0("The send succeeded; send final RAMTRANSMIT.\n");

         /* We may have transferred some data but not yet told the
         other end about it. xfersend__msg contains a final RAMTRANSMIT,
         which does not quite fill his buffer (or we'd have sent it already)
         thus signalling to him that the transfer is over. */

         wimpt_noerr(wimp_sendmessage
         ( wimp_ESEND,
           &xfersend__msg,
           xfersend__receiver));

        }
        else
        {
          tracef0("the send failed.\n");
        }
        if(xfersend__close) xfersend__winclose();
        return TRUE;
       }
       break ;

      case wimp_MPrintFile:       /* was dropped on a printer application */
       if (xfersend__printproc != 0)
       {
        int res ;

        tracef0("print request acceptable\n");
        xfersend__fileissafe = FALSE ;

        res = xfersend__printproc(&e->data.msg.data.print.name[0],
                                      xfersend__savehandle) ;

        xfersend__msg = e->data.msg;
        xfersend__msg.hdr.your_ref = xfersend__msg.hdr.my_ref;
        xfersend__msg.hdr.action =
           res >= 0 ? wimp_MDATALOAD : wimp_MWillPrint;
        xfersend__msg.data.print.type = res ;  /* in case it's been saved */
        wimpt_noerr(wimp_sendmessage(
           wimp_ESEND,
           &xfersend__msg,
           xfersend__receiver));
        if(xfersend__close) xfersend__winclose();
        return TRUE;
       }
       break ;

      case wimp_MDATASAVEOK:
      {
        tracef4("datasaveok %i %i %i %i.\n",
          e->data.msg.hdr.size,
          e->data.msg.hdr.task,
          e->data.msg.hdr.your_ref,
          e->data.msg.hdr.my_ref);
        tracef4("datasaveok %x %x %x %x.\n",
          e->data.msg.data.words[0],
          e->data.msg.data.words[1],
          e->data.msg.data.words[2],
          e->data.msg.data.words[3]);
        tracef1("it's the datasaveok, to file '%s'.\n",
                 (int) &e->data.msg.data.datasaveok.name[0]);

        win_remove_unknown_event_processor(xfersend__unknowns, 0);

        tracef1("save to filename '%s'.\n",
                (int) &e->data.msg.data.datasaveok.name[0]);

        xfersend__fileissafe = e->data.msg.data.datasaveok.estsize > 0 ;


        if (xfersend__saveproc != NULL &&
            xfersend__saveproc(&e->data.msg.data.datasaveok.name[0],
                             xfersend__savehandle))
        {
          tracef0("the save succeeded: send dataload\n");

          xfersend__msg = e->data.msg;
                                 /* sets hdr.size, data.w,i,x,y, size, name */
          xfersend__msg.hdr.your_ref = e->data.msg.hdr.my_ref;
          xfersend__msg.hdr.action = wimp_MDATALOAD;
          xfersend__msg.data.dataload.type = xfersend__filetype ;
          wimpt_noerr(wimp_sendmessage(
            wimp_ESENDWANTACK,
            &xfersend__msg,
            e->data.msg.hdr.task));
        }
        else
        {
          /* he has already reported the error: nothing more to do. */
          tracef0("save was not successful.\n");
        }
        if(xfersend__close) xfersend__winclose();
        return (xfersend__saveproc == NULL) ? FALSE : TRUE;
      } 
     }
    return FALSE ;      /* unknown not dealt with */

  case wimp_EACK:
    if (e->data.msg.hdr.your_ref == xfersend__msgid &&
        e->data.msg.hdr.action == wimp_MDATALOAD)
    {
      /* It looks as if he hasn't acknowledged my DATALOAD acknowledge:
      thus it may be a loose scrap file, and must be deleted. */
      char a[256];
      tracef0("he hasn't ack'd our data load of temp file, so delete the file.\n");
      werr(FALSE, msgs_lookup("xfersend1:Bad data transfer, receiver dead."));
      sprintf(a, "%%delete %s", &xfersend__msg.data.dataload.name[0]);
      os_cli(a) ;
    }
    return TRUE;
  default:
    return FALSE ;
  }
}


static int sendbuf__state ;

static BOOL sendbuf__unknowns(wimp_eventstr *e, void *h)
{
 h = h ;

 tracef4("sendbuf__unknowns %d %d %d %d\n",
          e->data.msg.hdr.my_ref, e->data.msg.hdr.your_ref,
          xfersend__msg.hdr.your_ref, xfersend__msg.hdr.my_ref) ;

 if ((e->e == wimp_ESENDWANTACK || e->e == wimp_ESEND) &&
     e->data.msg.hdr.your_ref == xfersend__msg.hdr.my_ref &&
     e->data.msg.hdr.action == wimp_MRAMFETCH)
 {
  /* Prepare xfersend__msg as the next RAMTRANSMIT. Most of
  the fields are already set up. */

  xfersend__msg.data.ramtransmit.addr = e->data.msg.data.ramfetch.addr;
  xfersend__msg.data.ramtransmit.nbyteswritten = 0;
  xfersend__msg.hdr.your_ref = e->data.msg.hdr.my_ref ;
  rcvbufsize = e->data.msg.data.ramfetch.nbytes;

  tracef2("RAMFETCH received: continue with buffer at %x, size %d\n",
           (int) xfersend__msg.data.ramtransmit.addr, rcvbufsize) ;

  sendbuf__state = 1 ;
  return TRUE ;      /* We've had another RAMFETCH: off we go again */
 }

 if (e->e == wimp_EACK &&
    e->data.msg.hdr.my_ref == xfersend__msg.hdr.my_ref)
 {
  sendbuf__state = 2 ;
  tracef0("xfersend RAMTRANSMIT bounced; set failed state\n") ;
  return TRUE ;/* our message bounced back; give up */
 }

 return FALSE ;    /* we don't want it */
}


BOOL xfersend_sendbuf(char *buffer, int size)
{

/* Called by his sendproc when sending things in memory. The
reply record is in xfersend__msg. */

 tracef2("xfersend_sendbuf %i %i\n", (int) buffer, size);

 /* Make the data transfer */
 tracef3("transfer block of %d from %x to %x\n", size, (int) buffer,
           (int) (xfersend__msg.data.ramtransmit.addr +
            xfersend__msg.data.ramtransmit.nbyteswritten)) ;

 wimpt_noerr(wimp_transferblock(
      wimpt_task(),
      buffer,
      xfersend__receiver,
      xfersend__msg.data.ramtransmit.addr +
        xfersend__msg.data.ramtransmit.nbyteswritten,
      size));

 /* record bytes to be sent to the other end */
 xfersend__msg.data.ramtransmit.nbyteswritten += size;
 rcvbufsize -= size ;

 /* if size != 0, there are still bytes to send. */

  if (rcvbufsize > 0) return TRUE;

 tracef1("xfersend message has put %d into buffer\n",size) ;
 /* Tell him that you've done it */
 wimpt_noerr(wimp_sendmessage(
      wimp_ESENDWANTACK,
      &xfersend__msg,
      xfersend__receiver));

  /* Get his reply. Poll and despatch events until get nack or message */

 sendbuf__state = 0 ;

 win_add_unknown_event_processor(sendbuf__unknowns, 0) ;
 do { event_process() ; } while (sendbuf__state == 0) ;
 win_remove_unknown_event_processor(sendbuf__unknowns, 0) ;

  /* This exit happens in the cases where the buffers at each end
      are of identical size. So, return for another call to sendbuf, or
      so that the sendbuf procedure can return. */

 return sendbuf__state != 2 ;  /* OK unless state = broken */
}

BOOL
   xfersend
   (  int filetype,
      char *filename,
      int estsize,
      xfersend_saveproc saver,
      xfersend_sendproc sender,
      xfersend_printproc printer,
      wimp_eventstr *e,
      void *handle
   )
{  wimp_dragstr dr;
   wimp_wstate wstate;
   wimp_icon icon;
   wimp_w w = e->data.but.m.w;
   wimp_mousestr mouse_str;
   int
      x_limit = bbc_vduvar (bbc_XWindLimit) << bbc_vduvar (bbc_XEigFactor),
      y_limit = bbc_vduvar (bbc_YWindLimit) << bbc_vduvar (bbc_YEigFactor),
      screen_x0, screen_y0,
      mouse_x, mouse_y,
      x0, y0, x1, y1;

   xfersend__saveproc = saver;
   xfersend__sendproc = sender;
   xfersend__printproc = printer;
   xfersend__filetype = filetype;
   xfersend__estsize = estsize;
   xfersend__savehandle = handle;
   if (xfersend__filename == 0) xfersend__filename = malloc(256);
   if(filename == 0)
      strcpy(xfersend__filename, msgs_lookup("xfersend2:Selection"));
   else
      strncpy(xfersend__filename,filename,256);
   tracef0("Initiate a drag.\n");

   /*Get pointer position to allow icon to be dragged
      partially off-screen. JRC 9 Nov '89*/
   wimp_get_point_info (&mouse_str);
   mouse_x = mouse_str.x;
   mouse_y = mouse_str.y;

   /*Find screen origin*/
   wimp_get_wind_state (w, &wstate);
   screen_x0 = wstate.o.box.x0 - wstate.o.x;
   screen_y0 = wstate.o.box.y1 - wstate.o.y;

   /*Get initial icon position*/
   wimp_get_icon_info (w, e->data.but.m.i, &icon);
   x0 = screen_x0 + icon.box.x0;
   y0 = screen_y0 + icon.box.y0;
   x1 = screen_x0 + icon.box.x1;
   y1 = screen_y0 + icon.box.y1;

   /*Set up drag*/
   dr.window    = w; /*not relevant*/
   dr.type      = wimp_USER_FIXED;
   dr.box.x0    = x0;
   dr.box.y0    = y0;
   dr.box.x1    = x1;
   dr.box.y1    = y1;
   dr.parent.x0 = x0 - mouse_x; /*Expanded parent by box overlap*/
   dr.parent.y0 = y0 - mouse_y;
   dr.parent.x1 = x1 - mouse_x + x_limit;
   dr.parent.y1 = y1 - mouse_y + y_limit;
   wimp_drag_box (&dr);

   win_add_unknown_event_processor (xfersend__unknowns, NULL);
   return TRUE;
}


BOOL xfersend_pipe(int filetype, char *filename, int estsize,
                     xfersend_saveproc saver,
                     xfersend_sendproc sender,
                     xfersend_printproc printer,
                     void *handle, wimp_t task)
{

      xfersend__saveproc = saver;
      xfersend__sendproc = sender;
      xfersend__printproc = printer;
      xfersend__filetype = filetype;
      xfersend__estsize = estsize;
      xfersend__savehandle = handle;
      if (xfersend__filename == 0) xfersend__filename = malloc(256);
      if(filename == 0)
        strcpy(xfersend__filename, msgs_lookup("xfersend2:Selection"));
      else
        strncpy(xfersend__filename,filename,256);
      
        {
        wimp_msgstr msg;

        msg.hdr.size = sizeof(wimp_msghdr) + sizeof(wimp_msgdatasave);
        msg.hdr.task = task;
        msg.hdr.your_ref = 0;
        msg.hdr.action = wimp_MDATASAVE;
        msg.data.datasave.w = 0; /* kludge, it's not being sent to a
                                        window!! Hope the wimp doesn't
                                        check this */
        msg.data.datasave.i = 0;
        msg.data.datasave.x = 0;
        msg.data.datasave.y = 0;
        msg.data.datasave.type = xfersend__filetype;
        msg.data.datasave.estsize = xfersend__estsize;
        {
          int i, tail;
          char name[256];
          if (xfersend__filename == 0) xfersend__filename = malloc(256);
          strncpy(name,xfersend__filename,  256);
          tail = strlen(name); /* point at the zero */
          while (tail > 0 && name[tail-1] != '.' && name[tail-1] != ':')
               tail--;

          for (i = 0; i <= 10; i++) msg.data.datasave.leaf[i] = name[tail++];
          msg.data.datasave.leaf[11] = '\0' ;     /* force termination */
          tracef1("suggest leaf '%s'.\n", (int) &msg.data.datasave.leaf[0]);
        };
        wimpt_noerr(wimp_sendmessage(wimp_ESEND, &msg, task));
        xfersend__msgid = msg.hdr.my_ref; /* filled in by wimp. */
      }
  
      win_add_unknown_event_processor(xfersend__unknowns, 0);
      return TRUE;
}

                     
BOOL xfersend_file_is_safe()
{  return xfersend__fileissafe;
}

void xfersend_set_fileissafe(BOOL value)
{  xfersend__fileissafe = value;
}

void xfersend_clear_unknowns(void)
{
   win_remove_unknown_event_processor(sendbuf__unknowns, 0);
   win_remove_unknown_event_processor(xfersend__unknowns, 0);
}

/* end xfersend.c */
