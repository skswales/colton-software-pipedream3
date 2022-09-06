/* > c.xfersend */

/* Title:   xfersend.c
 * Purpose: generalised data transfer to a concurrent wimp program.
*/

#include <string.h>
#include <stdio.h>

#define BOOL    int
#define TRUE    1
#define FALSE   0

#include "trace.h"

#include "os.h"
#include "bbc.h"
#include "wimp.h"
#include "wimpt.h"
#include "win.h"
#include "dbox.h"
#include "fileicon.h"
#include "werr.h"
#include "menu.h"
#include "event.h"
#include "misc.h"
#include "upcall.h"

#include "xfersend.h"


#if FALSE
/* external dboxes must provide these fields */

#define xfersend_FOK    0   /* OK action button */
#define xfersend_FName  1   /* name field */
#define xfersend_FIcon  2   /* icon to drag. */
#endif


static BOOL                     xfersend__fileissafe;
static wimp_t                   xfersend__receiver;
static wimp_msgstr              xfersend__msg;
static int                      xfersend__msgid = 0;
static int                      xfersend__rcvbufsize;

static wimp_mousestr            xfersend__mousestr;

static BOOL                     xfersend__ukproc = FALSE;
static dbox_raw_handler_proc    xfersend__oldraw_handler_proc;
static void *                   xfersend__oldhandle;


/* provided by the caller */
static int                      xfersend__filetype;
static int                      xfersend__estsize;
static xfersend_saveproc        xfersend__saveproc;
static void *                   xfersend__savehandle;
static xfersend_sendproc        xfersend__sendproc;
static xfersend_printproc       xfersend__printproc;
static dbox                     xfersend__d;
static xfersend_clickproc       xfersend__clickproc;
static void *                   xfersend__clickhandle;


#define OS_File 8


static void
xfersend__send_msg(wimp_etype etype)
{
    wimpt_send_message(etype, &xfersend__msg, xfersend__receiver);
}


/* This is the handler for the xfersend dialogue box window */

static BOOL xfersend__wimpevents(dbox d, void *ev, void *handle);


static BOOL
xfersend__unknowns(wimp_eventstr *e, void *handle)
{
    tracef1("xfersend__unknowns got event %s\n", trace_wimp_event(e));

    return(xfersend__wimpevents((dbox) handle, (void *) e, NULL));
}


static BOOL
xfersend__wimpevents(dbox d, void *ev, void *handle)
{
    wimp_eventstr  eventcopy = * (wimp_eventstr *) ev;  /* copy event */
    wimp_eventstr *e         = &eventcopy;

    IGNOREPARM(handle);

    tracef1("xfersend__wimpevents got event %s\n", trace_wimp_event(e));

    switch(e->e)
        {
        case wimp_EBUT:
            tracef1("xfersend EBUT &%p\n", e->data.but.m.bbits);

            if( (e->data.but.m.bbits == wimp_BDRAGLEFT) &&
                (e->data.but.m.i     == xfersend_FIcon) )
                {
                wimp_dragstr dr;
                wimp_wstate wstate;
                wimp_icon icon;
                wimp_w w;

                if(win_add_unknown_event_processor(xfersend__unknowns, d))
                    {
                    w = (wimp_w) dbox_syshandle(d);

                    tracef0("Initiate a drag.\n");

                    xfersend__ukproc = TRUE;

                    wimpt_safe(wimp_get_wind_state(w, &wstate));
                    wimpt_safe(wimp_get_icon_info(w, xfersend_FIcon, &icon));
                    dr.window   = w;    /* not relevant */
                    dr.type     = wimp_USER_FIXED;
                    dr.box.x0   = wstate.o.box.x0 - wstate.o.scx + icon.box.x0;
                    dr.box.y0   = wstate.o.box.y1 - wstate.o.scy + icon.box.y0;
                    dr.box.x1   = wstate.o.box.x0 - wstate.o.scx + icon.box.x1;
                    dr.box.y1   = wstate.o.box.y1 - wstate.o.scy + icon.box.y1;
                    dr.parent.x0 = 0;
                    dr.parent.y0 = 0;
                    dr.parent.x1 = bbc_vduvar(bbc_XWindLimit) << bbc_vduvar(bbc_XEigFactor);
                    dr.parent.y1 = bbc_vduvar(bbc_YWindLimit) << bbc_vduvar(bbc_YEigFactor);
                    wimpt_safe(wimp_drag_box(&dr));
                    }

                return(TRUE);
                }
            else
                tracef0("not a drag on the file icon - ignored\n");

            break;


        case wimp_EUSERDRAG:
            wimp_get_point_info(&xfersend__mousestr);

            if( (xfersend__mousestr.w != -1)                            &&
                (xfersend__mousestr.w != dbox_syshandle(xfersend__d))   )
                {
                char filename[212];
                wimp_msgstr msg;

                tracef1("dragged to window %d: offer data.\n", xfersend__mousestr.w);

                msg.hdr.size                = sizeof(wimp_msghdr) + sizeof(wimp_msgdatasave);
                msg.hdr.task                = xfersend__mousestr.w;
                msg.hdr.your_ref            = 0;
                msg.hdr.action              = wimp_MDATASAVE;

                msg.data.datasave.w         = xfersend__mousestr.w;
                msg.data.datasave.i         = xfersend__mousestr.i;
                msg.data.datasave.x         = xfersend__mousestr.x;
                msg.data.datasave.y         = xfersend__mousestr.y;
                msg.data.datasave.type      = xfersend__filetype;
                msg.data.datasave.estsize   = xfersend__estsize;

                dbox_getfield(d, xfersend_FName, filename, sizeof(filename));

                strcpy(msg.data.datasave.leaf, leafname(filename));
                tracef1("suggest leafname \"%s\"\n", msg.data.datasave.leaf);

                wimpt_send_wmessage(wimp_ESEND, &msg, xfersend__mousestr.w, xfersend__mousestr.i);

                /* note the ref. which has been filled in by the wimp. */
                xfersend__msgid = msg.hdr.my_ref;

                /* We still get unknown events, so we'll get the reply sometime. */
                }
            else
                tracef0("drag to no window/same window: has no effect\n");

            return(TRUE);


        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            {
            wimp_msgstr *m = &e->data.msg;

            tracef3("xfersend msg %s received: your_ref %i  msgid %i\n",
                    trace_wimp_message(e), m->hdr.your_ref, xfersend__msgid);

            if(m->hdr.your_ref == xfersend__msgid)
                {
                tracef0("correct msgid\n");

                switch(m->hdr.action)
                    {
                    case wimp_MRAMFETCH:
                        if(xfersend__sendproc)
                            {
                            /* He wants to do an in-core transfer, and we can do this.
                             * Note that this can't be other than the first response, as others
                             * are grabbed by sendbuf
                            */
                            tracef0("ram transfer starting.\n");

                            xfersend__fileissafe = FALSE;

                            /* Prepare the reply record. */
                            xfersend__msg               = *m;
                            xfersend__receiver          = xfersend__msg.hdr.task;
                            xfersend__msg.hdr.your_ref  = xfersend__msg.hdr.my_ref;
                            xfersend__msg.hdr.action    = wimp_MRAMTRANSMIT;

                            xfersend__msg.data.ramtransmit.addr = m->data.ramfetch.addr;
                            xfersend__msg.data.ramtransmit.nbyteswritten = 0; /* so far */

                            xfersend__rcvbufsize        = m->data.ramfetch.nbytes;

                            if(xfersend__sendproc(xfersend__savehandle, &xfersend__rcvbufsize))
                                {
                                /* get dbox closed */
                                dbox_sendclose(d);

                                /* See sendbuf for all the real work for this case... */

                                tracef0("The send succeeded; send final RAMTRANSMIT.\n");

                                /* We may have transferred some data but not yet told the
                                 * other end about it. xfersend__msg contains a final RAMTRANSMIT,
                                 * which does not quite fill his buffer (or we'd have sent it already)
                                 * thus signalling to him that the transfer is over.
                                */
                                xfersend__send_msg(wimp_ESEND);
                                }
                            else
                                tracef0("the send failed.\n");

                            return(TRUE);
                            }
                        else
                            tracef0("ram transfer request ignored\n");

                    break;


                    case wimp_MPrintFile:
                        /* file was dropped on a printer application */
                        if(xfersend__printproc)
                            {
                            int res;

                            tracef0("print request acceptable\n");

                            xfersend__fileissafe = FALSE;

                            xfersend__msg               = *m;
                            xfersend__receiver          = xfersend__msg.hdr.task;
                            xfersend__msg.hdr.your_ref  = xfersend__msg.hdr.my_ref;

                            /* get dbox closed */
                            dbox_sendclose(d);

                            res = xfersend__printproc(m->data.print.name, xfersend__savehandle);

                            xfersend__msg.hdr.action    = (res >= 0)
                                                            ? wimp_MDATALOAD
                                                            : wimp_MWillPrint;

                            xfersend__msg.data.print.type = res;
                            /* in case it's been saved */

                            xfersend__send_msg(wimp_ESEND);

                            return(TRUE);
                            }
                        else
                            tracef0("ignoring print request\n");

                        break;


                    case wimp_MDATASAVEOK:
                        {
                        tracef4("datasaveok %i %i %i %i.\n",
                                m->hdr.size, m->hdr.task, m->hdr.your_ref, m->hdr.my_ref);
                        tracef4("datasaveok %x %x %x %x.\n",
                                m->data.words[0], m->data.words[1], m->data.words[2], m->data.words[3]);
                        tracef1("it's the datasaveok, to file '%s'\n",
                                m->data.datasaveok.name);

                        /* Don't need these any more */
                        win_remove_unknown_event_processor(xfersend__unknowns, d);
                        xfersend__ukproc = FALSE;

                        tracef1("save to filename '%s'\n", m->data.datasaveok.name);

                        xfersend__fileissafe = (m->data.datasaveok.estsize > 0);

                        xfersend__msg               = *m;
                        xfersend__receiver          = xfersend__msg.hdr.task;
                        xfersend__msg.hdr.your_ref  = xfersend__msg.hdr.my_ref;
                        xfersend__msg.hdr.action    = wimp_MDATALOAD;

                        if(xfersend__saveproc(m->data.datasaveok.name, xfersend__savehandle))
                            {
                            /* get dbox closed */
                            dbox_sendclose(d);

                            tracef0("the save succeeded: send dataload\n");

                            xfersend__msg.data.dataload.type = xfersend__filetype;

                            xfersend__send_msg(wimp_ESENDWANTACK);
                            }
                        else
                            /* he has already reported the error: nothing more to do. */
                            tracef0("save was not successful.\n");

                        return(TRUE);
                        } 
                    }
                }
            else
                tracef0("strange msgid\n");
            }
            break;


        case wimp_EACK:
            {
            wimp_msgstr *m = &e->data.msg;

            if((m->hdr.your_ref == xfersend__msgid)  &&  (m->hdr.action  == wimp_MDATALOAD))
                {
                /* It looks as if he hasn't acknowledged my DATALOAD
                 * acknowledge: thus it may be a loose scrap file,
                 * and must be deleted.
                */
                char a[256];
                tracef0("he hasn't ack'd our data load of temp file, so delete the file.\n");
                werr("Bad data transfer, receiver dead");
                sprintf(a, "Remove %s", xfersend__msg.data.dataload.name);
                os_cli(a);
                return(TRUE);
                }
            else
                tracef0("strange EACK msgid\n");
            }
            break;


        default:
            break;
        }

    return(xfersend__oldraw_handler_proc ? xfersend__oldraw_handler_proc(d, ev, xfersend__oldhandle) : FALSE);
}


static int sendbuf__state;

#define SENDBUF_START   0
#define SENDBUF_GOING   1
#define SENDBUF_BROKEN  2

static BOOL
sendbuf__unknowns(wimp_eventstr *e, void *handle)
{
    wimp_msgstr *m = &e->data.msg;

    IGNOREPARM(handle);

    tracef4("sendbuf__unknowns %d %d %d %d\n",
            m->hdr.my_ref, m->hdr.your_ref, xfersend__msg.hdr.your_ref, xfersend__msg.hdr.my_ref);

    if( ((e->e == wimp_ESENDWANTACK)  ||  (e->e == wimp_ESEND)) &&
        (m->hdr.your_ref == xfersend__msg.hdr.my_ref)           &&
        (m->hdr.action == wimp_MRAMFETCH)                       )
        {
        /* Prepare xfersend__msg as the next RAMTRANSMIT.
         * Most of the fields are already set up.
        */
        xfersend__msg.data.ramtransmit.addr = m->data.ramfetch.addr;
        xfersend__msg.data.ramtransmit.nbyteswritten = 0; /* so far. */
        xfersend__msg.hdr.your_ref  = m->hdr.my_ref;
        xfersend__rcvbufsize        = m->data.ramfetch.nbytes;

        tracef2("RAMFETCH received: continue with buffer at %x, size %d\n",
                xfersend__msg.data.ramtransmit.addr, xfersend__rcvbufsize);

        sendbuf__state = SENDBUF_GOING;

        return(TRUE);       /* We've had another RAMFETCH: off we go again */
        }

    elif((e->e == wimp_EACK)  &&
        (m->hdr.my_ref == xfersend__msg.hdr.my_ref))
        {
        tracef0("xfersend RAMTRANSMIT bounced; set failed state\n");

        sendbuf__state = SENDBUF_BROKEN;

        return(TRUE);       /* our message bounced back; give up */
        }

    return(FALSE);          /* we don't want it */
}


/* Called by his sendproc when sending things in memory.
 * The reply record is in xfersend__msg.
*/

extern BOOL
xfersend_sendbuf(char *buffer, int size)
{
    tracef2("xfersend_sendbuf &%p &%p\n", buffer, size);

#if TRACE
    /* check size of transfer */
    if(xfersend__rcvbufsize < size)
        {
        werr("Sendbuf called with too much data\n");
        return(FALSE);
        }
#endif

    /* Make the data transfer */
    tracef3("transfer block of &%p from &%p to &%p\n", size, buffer,
            xfersend__msg.data.ramtransmit.addr +
            xfersend__msg.data.ramtransmit.nbyteswritten);

    wimpt_noerr(wimp_transferblock(
                            wimpt_task(),
                            buffer,
                            xfersend__receiver,
                            xfersend__msg.data.ramtransmit.addr +
                            xfersend__msg.data.ramtransmit.nbyteswritten,
                            size));

    /* record bytes to be sent to the other end */
    xfersend__msg.data.ramtransmit.nbyteswritten += size;
    xfersend__rcvbufsize -= size;

    /* if size != 0, there are still bytes to send. */

    if(xfersend__rcvbufsize > 0)
        return(TRUE);

    /* Our client at this end has not given us enough data to
     * satisfy the guy at the other end. So, increment the buffer
     * pointers for the other end and return at this end.
    */

    /* Send a message saying that we've filled his buffer.
     * Get his reply, ensure that it gives us another buffer.
     * If it does not then return FALSE to our client at this end,
     * the receiver is full/sick or something.
    */
    tracef1("xferSend message has put %d into buffer\n", size);

    /* Tell him that you've done it */
    xfersend__send_msg(wimp_ESENDWANTACK);


    /* Get his reply. Poll and despatch events until get nack or message */
    sendbuf__state = SENDBUF_START;

    /* Now add another node on the chain to get our reply before the other
     * xfersend unknown handler eats it!
    */
    (void) win_add_unknown_event_processor(sendbuf__unknowns, NULL);

    /* ^^^ !!!! SKS */


    do { event_process(); } while(sendbuf__state == SENDBUF_START);


    win_remove_unknown_event_processor(sendbuf__unknowns, NULL);

    /* This exit happens in the cases where the buffers at each end
     * are of identical size. So, return for another call to sendbuf,
     * or so that the sendbuf procedure can return.
    */

    return(sendbuf__state != SENDBUF_BROKEN);   /* OK unless state = broken */
}


/* (re)make a file icon of the appropriate type */

static void
xfersend__fileicon(dbox d)
{
    wimp_w w = dbox_syshandle(d);

    fileicon(w, xfersend_FIcon, xfersend__filetype);

    wimpt_safe(wimp_set_icon_state(w, xfersend_FIcon, 0, 0));
}


extern BOOL
xfersend(
    int filetype,
    const char *name,
    int estsize,
    xfersend_saveproc  saveproc,
    void *savehandle,
    xfersend_sendproc  sendproc,
    xfersend_printproc printproc,
    dbox d,
    xfersend_clickproc clickproc,
    void *clickhandle)
{
    dbox_field f;
    char filename[216];

    xfersend__filetype      = filetype;
    xfersend__estsize       = estsize;
    xfersend__saveproc      = saveproc;
    xfersend__savehandle    = savehandle;
    xfersend__sendproc      = sendproc;
    xfersend__printproc     = printproc;
    xfersend__d             = d;
    xfersend__clickproc     = clickproc;
    xfersend__clickhandle   = clickhandle;

    /* set up icons in dbox */
    dbox_setfield(d, xfersend_FName, name);
    xfersend__fileicon(d);

    dbox_raw_eventhandlers(d, &xfersend__oldraw_handler_proc, &xfersend__oldhandle);
    dbox_raw_eventhandler(d, xfersend__wimpevents, NULL);

    while((f = dbox_fillin(d)) != dbox_CLOSE)
        {
        if(f == dbox_OK)
            {
            dbox_getfield(d, xfersend_FName, filename, sizeof(filename));

            /* complain if just leafname */
            if(leafname(filename) == filename)
                {
                werr("To save, drag the icon to a directory viewer");
                continue;
                }

            xfersend__fileissafe = TRUE;        /* bog standard save to real file */

            #if 1
            tracef0("ok clicked - return to save file\n");
            break;
            #else
            if( xfersend__saveproc(filename, xfersend__savehandle)  &&
                !dbox_persist())
                {
                tracef0("file saved ok but not persisting - terminate xfersend loop\n");
                break;
                }
            #endif
            }
        elif(xfersend__clickproc)
            {
            filetype = xfersend__filetype;
            xfersend__clickproc(d, f, &xfersend__filetype, xfersend__clickhandle);
            if(filetype != xfersend__filetype)
                xfersend__fileicon(d);
            }
        else
            {
            tracef0("strange click terminates xfersend loop\n");
            break;
            }
        }

    tracef0("xfersend loop done\n");

    /* Might have escaped the loop above still owning unknown events. */
    if(xfersend__ukproc)
        {
        xfersend__ukproc = FALSE;
        win_remove_unknown_event_processor(xfersend__unknowns, d);
        }

    return(f == dbox_OK);
}


/****************************************************
*                                                   *
*  Return indication as to whether to reset title   *
*                                                   *
****************************************************/

extern BOOL
xfersend_file_is_safe(void)
{
    return(xfersend__fileissafe);
}


#if !defined(SMALL)

/****************************************************
*                                                   *
* Read the typed-in filename during a ram transfer  *
*                                                   *
****************************************************/

extern void
xfersend_read_leafname_during_send(char *name /*out*/, int length)
{
    char filename[256];

    dbox_getfield(xfersend__d, xfersend_FName, filename, sizeof(filename));

    *name = '\0';
    strncat(name, leafname(filename), length);
}

#endif


/* end of xfersend.c */


