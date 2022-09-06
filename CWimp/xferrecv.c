/* Title: -> c.xferrecv
 * Purpose: general purpose import of data by dragging icon
 * Requires:
 *  BOOL
*/

#include <string.h>
#include <stdio.h>

#include "trace.h"

#include "h.os"
#include "h.bbc"
#include "h.wimp"
#include "h.wimpt"
#include "h.win"
#include "h.fileicon"
#include "h.werr"
#include "h.menu"
#include "h.event"
#include "h.xferrecv"
#include "upcall.h"

#include "misc.h"

#define OS_ReadVarVal 0x23

typedef enum
{
    xferrecv__state_Ask,
    xferrecv__state_Talk,
    xferrecv__state_Done,
    xferrecv__state_Broken
}
xferrecv__stateval;


static BOOL         xferrecv__fileissafe = TRUE;
static wimp_t       xferrecv__sendertask;
static wimp_msgstr  xferrecv__ack;
static int          xferrecv__msgid;
static int          scrap_ref = 0;
static int          xferrecv__state;

static char *       xferrecv__buffer;
static int          xferrecv__buffersize;
static xferrecv_buffer_processor xferrecv__processbuff;


static void
xferrecv__send_ack(wimp_etype etype)
{
    wimpt_send_message(etype, &xferrecv__ack, xferrecv__sendertask);
}


/* Is the current event (uses wimpt) an invitation to insert a file?
 * If so return the file type and write a low-lifetime pointer to
 * the filename into *filename. If not return -1.
 * Set up ack block for subsequent insertfileok()
*/

extern int
xferrecv_checkinsert(char **filename)
{
    wimp_eventstr *e = wimpt_last_event();
    wimp_msgstr *m   = &e->data.msg;

    if( ((e->e == wimp_ESENDWANTACK)  ||  (e->e == wimp_ESEND))                     &&
        ((m->hdr.action == wimp_MDATALOAD)  ||  (m->hdr.action == wimp_MDATAOPEN))  )
        {
        tracef4("xferrecv_checkinsert returning filetype %x: %d %d %d\n",
                m->data.dataload.type, scrap_ref, m->hdr.your_ref, m->hdr.my_ref);

        /* Is this DATALOAD coming from a DATASAVEOK using scrap? */
        xferrecv__fileissafe            = (m->hdr.your_ref != scrap_ref)  ||  (scrap_ref == 0);

        xferrecv__sendertask            = m->hdr.task;

        xferrecv__ack.hdr.size          = sizeof(wimp_msghdr);
        xferrecv__ack.hdr.your_ref      = m->hdr.my_ref;
        xferrecv__ack.hdr.action        = wimp_MDATALOADOK;

        *filename = m->data.dataload.name;

        return(m->data.dataload.type);
        }
    else
        return(-1);
}


/* An insert has been completed successfully.
 * This sends an acknowledge back to the original application.
*/

extern void
xferrecv_insertfileok(void)
{
    tracef0("xferrecv_insertfileok\n");

    if(!xferrecv__fileissafe)
        {
        tracef0("Must delete scrap file\n");
        os_cli("Remove <Wimp$Scrap>");
        xferrecv__fileissafe = TRUE;
        }

    scrap_ref = 0;

    xferrecv__send_ack(wimp_ESEND);
}


/* Is the current event (uses wimpt) an invitation to print a file? If so
 * return the file type and write a pointer to the filename into *filename.
 * If not return -1. The application can print the file directly, or convert
 * it into the returned filename for subsequent dumping by the printer
 * application.
*/

extern int
xferrecv_checkprint(char **filename)
{
    wimp_eventstr *e = wimpt_last_event();
    wimp_msgstr *m   = &e->data.msg;

    if(((e->e==wimp_ESENDWANTACK)  ||  (e->e == wimp_ESEND))  &&
        (m->hdr.action == wimp_MPrintTypeOdd))
        {
        tracef4("xferrecv_checkprint returning filetype %x: %d %d %d\n",
                m->data.print.type, scrap_ref, m->hdr.your_ref, m->hdr.my_ref);

        xferrecv__fileissafe            = FALSE;

        xferrecv__sendertask            = m->hdr.task;

        xferrecv__ack.hdr.size          = sizeof(wimp_msghdr)
                                        + sizeof(wimp_msgprint);
        xferrecv__ack.hdr.your_ref      = m->hdr.my_ref;
        xferrecv__ack.hdr.action        = wimp_MPrintTypeKnown;

        *filename = m->data.print.name;

        return(m->data.print.type);
        }
    else
        return(-1);
}


/* A print has been completed successfully. This sends an acknowledge back
 * to the printer application; if printing was done to <Printer$Temp>, then
 * indicate the file type of the resulting file.
*/

extern void
xferrecv_printfileok(int type)
{
    xferrecv__ack.data.print.type = type;

    xferrecv__send_ack(wimp_ESEND);
}


/* Is the current event (uses wimpt) an invitation to receive a data send
 * operation? If so return the file type, if not return -1.
 * Estimated size is also returned.
*/

extern int
xferrecv_checkimport(int *estsize)
{
    wimp_eventstr *e = wimpt_last_event();
    wimp_msgstr *m   = &e->data.msg;

    if(((e->e==wimp_ESENDWANTACK)  ||  (e->e == wimp_ESEND))  &&
        (m->hdr.action == wimp_MDATASAVE))
        {
        xferrecv__sendertask                = m->hdr.task;

        xferrecv__ack.hdr.size              = sizeof(wimp_msghdr)
                                            + sizeof(wimp_msgdatasaveok);
        xferrecv__ack.hdr.your_ref          = m->hdr.my_ref;
        xferrecv__ack.hdr.action            = wimp_MDATASAVEOK;

        xferrecv__ack.data.datasaveok.w     = m->data.datasave.w;
        xferrecv__ack.data.datasaveok.i     = m->data.datasave.i;
        xferrecv__ack.data.datasaveok.x     = m->data.datasave.x;
        xferrecv__ack.data.datasaveok.y     = m->data.datasave.y;
        xferrecv__ack.data.datasaveok.type  = m->data.datasave.type;
        *estsize = xferrecv__ack.data.datasaveok.estsize = m->data.datasave.estsize;

        tracef2("xferrecv_checkimport returning filetype %x from %d\n",
                m->data.datasave.type, xferrecv__sendertask);

        return(xferrecv__ack.data.datasaveok.type);
        }
    else
        return(-1);
}


static void
xferrecv__sendRAMFETCH(void)
{
    int action = xferrecv__ack.hdr.action;
    char *addr = xferrecv__ack.data.ramfetch.addr;
    int size   = xferrecv__ack.data.ramfetch.nbytes;

    tracef2("xferrecv__sendRAMFETCH with buffer %x, size %d: ",
            xferrecv__buffer, xferrecv__buffersize);

    xferrecv__ack.hdr.action           = wimp_MRAMFETCH;
    xferrecv__ack.data.ramfetch.addr   = xferrecv__buffer;
    xferrecv__ack.data.ramfetch.nbytes = xferrecv__buffersize;
    xferrecv__send_ack(wimp_ESENDWANTACK);
    xferrecv__msgid = xferrecv__ack.hdr.my_ref;
    tracef1("ramfetch msg id %d\n", xferrecv__msgid);

    xferrecv__ack.hdr.action           = action;
    xferrecv__ack.data.ramfetch.addr   = addr;
    xferrecv__ack.data.ramfetch.nbytes = size;
}


static void
xferrecv__sendWimpScrap(void)
{
    os_regset r;

    tracef0("asking for Wimp$Scrap transfer\n") ;

    /* first check that variable exists */
    r.r[0] = (int) "Wimp$Scrap";
    r.r[1] = NULL;
    r.r[2] = -1;
    r.r[3] = 0;
    r.r[4] = 3;
    os_swix(OS_ReadVarVal+os_X, &r);

    if(r.r[2] == 0)
        werr("Can't transfer file (use *Set Wimp$Scrap <filename>)");
    else
        {
        /* rest of ack has been set up by checkimport */
        strcpy(xferrecv__ack.data.datasaveok.name, "<Wimp$Scrap>");
        /* file will not be safe with us */ 
        xferrecv__ack.data.datasaveok.estsize = -1;
        xferrecv__send_ack(wimp_ESEND);
        scrap_ref = xferrecv__ack.hdr.my_ref;
        }
}


/**********************************************************
* The handler that doimport does most of its work through *
**********************************************************/

static BOOL
xferrecv__unknown_events(wimp_eventstr *e, void *handle)
{
    wimp_msgstr *m = &e->data.msg;

    IGNOREPARM(handle);

    tracef5("xferrecv__unknown_events %d %d %d %d %d\n",
            e->e, m->hdr.action, m->hdr.my_ref, m->hdr.your_ref, xferrecv__msgid);

    if( ((e->e == wimp_ESENDWANTACK)  ||  (e->e == wimp_ESEND)) &&
        (m->hdr.your_ref == xferrecv__msgid)                    &&
        (m->hdr.action == wimp_MRAMTRANSMIT)                    )
        {
        tracef2("xferrecv got ramtransmit of %d into %d\n",
                m->data.ramtransmit.nbyteswritten,
                xferrecv__buffersize);

        if(m->data.ramtransmit.nbyteswritten == xferrecv__buffersize)
            {
            /* other end has filled our buffer; better try and allocate
             * some more space
            */
            if(xferrecv__processbuff(&xferrecv__buffer, &xferrecv__buffersize))
                {
                /* can go on */
                tracef2("user's buffer processor set buffer %x, size %d\n",
                        (int) xferrecv__buffer, xferrecv__buffersize);

                xferrecv__ack.hdr.your_ref = m->hdr.my_ref;
                xferrecv__sendRAMFETCH();
                xferrecv__state = xferrecv__state_Talk;
                }
            else
                {
                tracef0("users buffer processor failed: break down\n");
                xferrecv__state = xferrecv__state_Broken;
                }
            }
        else
            {
            tracef0("xferrecv had final ramtransmit; set done state\n");
            xferrecv__buffersize = m->data.ramtransmit.nbyteswritten;
            xferrecv__state = xferrecv__state_Done;
            }
        }
    elif((e->e == wimp_EACK)  &&
        (m->hdr.my_ref == xferrecv__msgid))
        {
        /* got our message bounced back */

        tracef0("xferrecv ramfetch bounced: ");
        if(xferrecv__state == xferrecv__state_Ask)
            xferrecv__sendWimpScrap();
        else
            {
            tracef0("tell the user the protocol fell down\n");
            werr("Data transfer failed.");
            }

        xferrecv__state = xferrecv__state_Broken;
        }
    else
        return(FALSE);

    return(TRUE);
}


/* Receives data into the buffer; calls the buffer processor if the buffer
 * given becomes full. Returns TRUE if the transaction completed sucessfully.
 * A buffer or size of zero means the data will be transferred via a file,
 * as it will if the sender can't cope with buffering.
*/

extern int
xferrecv_doimport(char *buffer, int size, xferrecv_buffer_processor p)
{
    tracef0("xferrecv_doimport() entered\n");

    xferrecv__fileissafe = FALSE;

    if(!buffer)
        {
        /* reciever wants a file */
        xferrecv__sendWimpScrap();
        return(-1);
        }

    if(!win_add_unknown_event_processor(xferrecv__unknown_events, NULL))
        return(FALSE);

    xferrecv__buffer        = buffer;
    xferrecv__buffersize    = size;
    xferrecv__processbuff   = p;

    xferrecv__state         = xferrecv__state_Ask;

    /* send me some data */
    xferrecv__sendRAMFETCH();

    do { event_process(); } while(xferrecv__state < xferrecv__state_Done);

    win_remove_unknown_event_processor(xferrecv__unknown_events, NULL);

    return((xferrecv__state == xferrecv__state_Done) ? xferrecv__buffersize : -1);
}


extern BOOL
xferrecv_file_is_safe(void)
{
    return(xferrecv__fileissafe);
}


/* end of xfersend.c */
