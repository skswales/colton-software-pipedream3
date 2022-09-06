/* > c.trace */

/* Title  : trace.c
 * Purpose: centralised control for trace/debug output
 * Version: 0.5
 *
 * cc trace.c -dtubetracing
 *   allows you to direct vdu output to another machine,
 *   using the HostFS relocatable module and a Tube podule.
 *
*/


#define TRACE 1 /* Always have trace present here */

#include "include.h"

#include <stdio.h>

#include "bbc.h"
#include "wimp.h"
#include "akbd.h"
/*#include "CModules:swinumbers.h"*/

#define XOS_Confirm                     0x20059

extern int wimpt_mode(void);


static int trace__count = 0;
static const char *trace__prefix = NULL;


extern void
trace_explode(void)
{
    if(trace__count)
        {
        if(akbd_pollalt())
            {
            if(akbd_pollsh())
                trace_off();
            else
                raise(SIGTERM);
            }
        elif(akbd_pollctl())
            {
            do  {
                /* nothing - wait for release */
                }
            while(akbd_pollctl()  &&  akbd_pollsh());
            }
        }
}


#if defined(tubetracing)

#include <stdlib.h>

extern int
tracef(const char *format, ...)
{
    char array[4096];
    char *ptr = array;
    char ch;

    if(trace__count)
        {
        va_list args;

        /* NB. if no Tube attached output goes to stdout */
        (void) os_cli("%HostVdu");

        if(trace__prefix)
            {
            fputs(trace__prefix, stdout);
            fputs(": ", stdout);
            }

        va_start(args, format);
        vsprintf(array, format, args);
        #if !defined(VA_END_SUPERFLUOUS)
        va_end(args);
        #endif

        while((ch = *ptr++) != '\0')
            {
            if(ch < 0x20)
                {
                if(ch == 10)
                    {
                    fputc(ch, stdout);
                    ch = 13;
                    }
                else
                    {
                    fputc('|', stdout);
                    ch = ch + '@';
                    }
                }
            else if(ch == 0x7F)
                {
                fputc('|', stdout);
                ch = '?';
                }

            fputc(ch, stdout);
            }

        (void) os_cli("%TubeVdu");

        trace_explode();
        }

    return(0);
}

#else


#include "h.wimpt"

static int trace__msgcont = 0;
/* This is set if the last message did not end in an immediate \n.
If it's not set then the message will be preceded by a setting of the
message window. */

static void vduqq(int a, ...)
{
    va_list ap;
    int i;
    int c;

    va_start(ap, a);

    for(i = a; i > 0; i--)
        bbc_vdu(va_arg(ap, int));

    #if !defined(VA_END_SUPERFLUOUS)
    va_end(ap);
    #endif
}


static void trace__setwindow(void)
{
  if (wimpt_mode() == 20) {
    vduqq(9,
          4, 28, 0, 63, 79, 48, /* set window */
          31, 0, 15);
  } else {
    vduqq(9,
          4, 28, 0, 28, 79, 16, /* set window */
          31, 0, 15);
  }
}


extern int
tracef(const char *format, ...)
{
    va_list args;

    va_start(args, format);

    if(trace__count)
        {
        char v[1024];
        int i = 0;
        int l = strlen(format);

        if(l > 0)
            {
            if(trace__msgcont == 0) /* start of new message */
                trace__setwindow();

            trace__msgcont = format[l-1] != '\n';

            vsprintf(v, format, args);

            while(v[i])
                {
                if(v[i] == '\n')
                    {
                    (void) bbc_vdu(10);
                    (void) bbc_vdu(13);
                    }
                else
                    (void) bbc_vdu(v[i]);
                i++;
                }
            }

        trace_explode();
        }

    #if !defined(VA_END_SUPERFLUOUS)
    va_end(args);
    #endif

    return(0);
}

#endif


extern void
trace_on(void)
{
    trace__count++;

    trace_explode();
}


extern void
trace_off(void)
{
    if(trace__count)
        #if defined(tubetracing)
        --trace__count;
        #else
        if(!--trace__count)
            wimpt_forceredraw();
        #endif
}


extern BOOL
trace_is_on(void)
{
    return(trace__count);
}


extern void
trace_clearscreen(void)
{
    if(trace__count)
        {
        (void) os_cli("%HostVdu");
        (void) bbc_mode(0);
        (void) os_cli("%TubeVdu");
        trace_explode();
        }
}


extern void
trace_pause(void)
{
    if(trace__count)
        {
#if defined(USE_SHIFT)
        tracef0("*** pausing - press SHIFT\n");

        while(!akbd_pollsh())
            trace_explode();
#else
        int res;

        tracef0("*** pausing - press a key or click the mouse\n");

        os_swi1r(XOS_Confirm, 0, &res);

        if(toupper(res) == 'A')
            raise(SIGTERM);
#endif
        }
}


extern const char *
trace_boolstring(BOOL t)
{
    return(t ? "True " : "False");
}


extern void
trace_system(const char *format, ...)
{
    if(trace__count)
        {
        char array[256];
        va_list args;

        va_start(args, format);
        vsprintf(array, format, args);
        #if !defined(VA_END_SUPERFLUOUS)
        va_end(args);
        #endif

        os_cli("HostVdu");
        if(trace__prefix)
            {
            fputs(trace__prefix, stdout);
            fputs(": ", stdout);
            }

        puts(array);
        os_cli(array);
        os_cli("TubeVdu");

        trace_explode();
        }
}


typedef union
{
    trace_proc  proc;
    int        *value;
}
deviant_proc;


extern const char *
trace_procedure_name(trace_proc proc)
{
    const char *name = "<not found>";
    deviant_proc deviant;
    int *z;

    deviant.proc = proc;
    z            = deviant.value;

    if( ((int) z & 0xFC000003)  ||  ((int) z < 0x00008000) )
        name = "<invalid address>";
    else
        {
        int w = *--z;
        if((w & 0xFFFF0000) == 0xFF000000) /* marker? */
            name = ((const char *) z) - (w & 0xFFFF);
        }

    return(name);
}


extern const char *
trace_wimp_eventcode(const int etype)
{
    static char default_eventstring[10];

    switch(etype)
        {
        case wimp_ENULL:
            return("wimp_ENULL");
        case wimp_EREDRAW:
            return("wimp_EREDRAW");
        case wimp_EOPEN:
            return("wimp_EOPEN");
        case wimp_ECLOSE:
            return("wimp_ECLOSE");
        case wimp_EPTRLEAVE:
            return("wimp_EPTRLEAVE");
        case wimp_EPTRENTER:
            return("wimp_EPTRENTER");
        case wimp_EBUT:
            return("wimp_EBUT");
        case wimp_EUSERDRAG:
            return("wimp_EUSERDRAG");
        case wimp_EKEY:
            return("wimp_EKEY");
        case wimp_EMENU:
            return("wimp_EMENU");
        case wimp_ESCROLL:
            return("wimp_ESCROLL");
        case wimp_ELOSECARET:
            return("wimp_ELOSECARET");
        case wimp_EGAINCARET:
            return("wimp_EGAINCARET");

        case wimp_ESEND:
            return("wimp_ESEND");
           /* send message, don't worry if it doesn't arrive */
        case wimp_ESENDWANTACK:
            return("wimp_ESENDWANTACK");
            /* send message, return ack if not acknowledged */
        case wimp_EACK:
             return("wimp_EACK");
            /* acknowledge receipt of message */

        default:
            sprintf(default_eventstring, "&%X", etype);
            return(default_eventstring);
        }
}


extern const char *
trace_wimp_xevent(const int etype, const void *ev)
{
    const wimp_eventdata *ed = (const wimp_eventdata *) ev;
    char tempbuffer[256];

    static char messagebuffer[512];

    strcpy(messagebuffer, trace_wimp_eventcode(etype));

    switch(etype)
        {
        case wimp_EOPEN:
            sprintf(tempbuffer, ": window %d; coords (%d, %d, %d, %d); scroll (%d, %d)",
                    ed->o.w,
                    ed->o.box.x0, ed->o.box.y0, ed->o.box.x1, ed->o.box.y1,
                    ed->o.scx,    ed->o.scy);
            break;


        case wimp_EREDRAW:
        case wimp_ECLOSE:
        case wimp_EPTRLEAVE:
        case wimp_EPTRENTER:
            sprintf(tempbuffer, ": window %d",
                    ed->o.w);
            break;


        case wimp_ESCROLL:
            sprintf(tempbuffer, ": window %d; coords (%d, %d, %d, %d); scroll (%d, %d); dir'n (%d, %d)",
                    ed->scroll.o.w,
                    ed->scroll.o.box.x0, ed->scroll.o.box.y0, ed->scroll.o.box.x1, ed->scroll.o.box.y1,
                    ed->scroll.o.scx,    ed->scroll.o.scy,
                    ed->scroll.x,        ed->scroll.y);
            break;


        case wimp_ESEND:
        case wimp_ESENDWANTACK:
        case wimp_EACK:
            sprintf(tempbuffer, ": %s",
                    trace_wimp_message(&ed->msg));
            break;


        case wimp_ENULL:
        case wimp_EBUT:
        case wimp_EUSERDRAG:
        case wimp_EKEY:
        case wimp_EMENU:
        case wimp_ELOSECARET:
        case wimp_EGAINCARET:

        default:
            *tempbuffer = '\0';
            break;
        }

    strcat(messagebuffer, tempbuffer);

    return(messagebuffer);
}


extern const char *
trace_wimp_action(const int action)
{
    static char default_actionstring[10];

    switch(action)
        {
        case wimp_MCLOSEDOWN:
            return("wimp_MCLOSEDOWN");

        case wimp_MDATASAVE:
            return("wimp_MDATASAVE");
            /* request to identify directory */

        case wimp_MDATASAVEOK:
            return("wimp_MDATASAVEOK");
            /* reply to wimp_MDATASAVE */

        case wimp_MDATALOAD:
            return("wimp_MDATALOAD");
            /* request to load/insert dragged icon */

        case wimp_MDATALOADOK:
            return("wimp_MDATALOADOK");
            /* reply that file has been loaded */

        case wimp_MDATAOPEN:
            return("wimp_MDATAOPEN");
            /* warning that an object is to be opened */

        case wimp_MRAMFETCH:
            return("wimp_MRAMFETCH");
            /* transfer data to buffer in my workspace */

        case wimp_MRAMTRANSMIT:
            return("wimp_MRAMTRANSMIT");
            /* I have transferred some data to a buffer in your workspace */

        case wimp_MPREQUIT:
            return("wimp_MPREQUIT");
        case wimp_PALETTECHANGE:
            return("wimp_PALETTECHANGE");

        case wimp_FilerOpenDir:
            return("wimp_FilerOpenDir");
        case wimp_FilerCloseDir:
            return("wimp_FilerCloseDir");

        case wimp_MHELPREQUEST:
            return("wimp_MHELPREQUEST");    /* interactive help request */
        case wimp_MHELPREPLY:
            return("wimp_MHELPREPLY");    /* interactive help message */

        case wimp_MNOTIFY:
            return("wimp_MNOTIFY");

        case wimp_MMENUWARN:
            return("wimp_MMENUWARN");      /* menu warning */
        case wimp_MMODECHANGE:
            return("wimp_MMODECHANGE");
        case wimp_MINITTASK:
            return("wimp_MINITTASK");
        case wimp_MCLOSETASK:
            return("wimp_MCLOSETASK");
        case wimp_MSLOTCHANGE:
            return("wimp_MSLOTCHANGE");  /* Slot size has altered */
        case wimp_MSETSLOT:
            return("wimp_MSETSLOT");
        case wimp_MTASKNAMERQ:
            return("wimp_MTASKNAMERQ");
        case wimp_MTASKNAMEIS:
            return("wimp_MTASKNAMEIS");

        /* Messages for dialogue with printer applications */

        case wimp_MPrintFile:
            return("wimp_MPrintFile");
        case wimp_MWillPrint:
            return("wimp_MWillPrint");
        case wimp_MPrintTypeOdd:
            return("wimp_MPrintTypeOdd");
        case wimp_MPrintTypeKnown:
            return("wimp_MPrintTypeKnown");
        case wimp_MPrinterChange:
            return("wimp_MPrinterChange");

        case wimp_MPD_DDE:
            return("wimp_MPD_DDE");

        default:
            sprintf(default_actionstring, "&%X", action);
            return(default_actionstring);
        }
}


extern const char *
trace_wimp_xmessage(const void *msgp, BOOL send)
{
    const wimp_msgstr *m = (wimp_msgstr *) msgp;
    const char *format   = send
                            ? "type %s, size %d, your_ref %d "
                            : "type %s, size %d, your_ref %d from task &%8.8X, my(his)_ref %d ";
    char tempbuffer[256];

    static char messagebuffer[512];

    sprintf(messagebuffer, format,
            trace_wimp_action(m->hdr.action), m->hdr.size, m->hdr.your_ref,
            m->hdr.task, m->hdr.my_ref);

    switch(m->hdr.action)
        {
        case wimp_MINITTASK:
            sprintf(tempbuffer, "CAO &%8.8X AplSize &%8.8X TaskName \"%s\"",
                    m->data.words[0],
                    m->data.words[1],
                    &m->data.words[2]);
            break;


        case wimp_MPD_DDE:
            sprintf(tempbuffer, "id %d handle %d",
                    m->data.pd_dde.id,
                    m->data.pd_dde.type.a.handle);
            break;


        case wimp_MCLOSEDOWN:
        case wimp_MDATASAVE:
            /* request to identify directory */

        case wimp_MDATASAVEOK:
            /* reply to wimp_MDATASAVE */

        case wimp_MDATALOAD:
            /* request to load/insert dragged icon */

        case wimp_MDATALOADOK:
            /* reply that file has been loaded */

        case wimp_MDATAOPEN:
            /* warning that an object is to be opened */

        case wimp_MRAMFETCH:
            /* transfer data to buffer in my workspace */

        case wimp_MRAMTRANSMIT:
            /* I have transferred some data to a buffer in your workspace */

        case wimp_MPREQUIT:
        case wimp_PALETTECHANGE:

        case wimp_FilerOpenDir:
        case wimp_FilerCloseDir:

        case wimp_MHELPREQUEST:
        case wimp_MHELPREPLY:

        case wimp_MNOTIFY:

        case wimp_MMENUWARN:
        case wimp_MMODECHANGE:
        case wimp_MCLOSETASK:
        case wimp_MSLOTCHANGE:
        case wimp_MSETSLOT:
        case wimp_MTASKNAMERQ:
        case wimp_MTASKNAMEIS:

        /* Messages for dialogue with printer applications */

        case wimp_MPrintFile:
        case wimp_MWillPrint:
        case wimp_MPrintTypeOdd:
        case wimp_MPrintTypeKnown:
        case wimp_MPrinterChange:

        default:
            *tempbuffer = '\0';
            break;
        }

    strcat(messagebuffer, tempbuffer);

    return(messagebuffer);
}


extern const char *
trace_string(const char *str)
{
    static char stringbuffer[16];

    if(!str)
        return("<<NULL>>");

    if( (str < (const char *) 0x8000)  ||
        ((unsigned long int) str > 0xFC000000))
        {
        sprintf(stringbuffer, "<<&%p>>", str);
        return(stringbuffer);
        }

    return(str);
}


extern void
trace_set_prefix(const char *prefix)
{
    trace__prefix = prefix;
}


/* end of trace.c */
