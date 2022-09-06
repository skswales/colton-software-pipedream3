/*
 * Title: h.dbox
 * Purpose: System-independent dialog boxes.
 * Author: WRS
 * Requires:
 *  <stddef.h>
 * History:
 *   21 August 87 -- started
 *   13-Dec-87: WRS: converted to C.
 *   24-Feb-88: WRS: dbox_read removed.
 *   13-Jun-88: WRS: added dbox_static
*/

/* It is important to note that the structure of your dialogue templates is
an integral part of your program. Always use symbolic names for templates and
for fields and action buttons within them. */

/* See separate documentation for how to use the Arthur Template Editor
in conjunction with this interface. */

typedef struct dbox__str *dbox; /* abstract dialogue box handle */

/* -------- Initialisation. -------- */

extern void dbox_verify(const char *name);
/* Check that the program will be able to generate templates of the given
name. If it is not, the call will not return. This should be called at the
initialisation of the program, to check that the program's templates are all
present. */

/* -------- Creation, Deletion. -------- */

extern dbox dbox_new(const char *name, char **errorp /*out*/);
/* Build a dialog box with the given name. Use the name to find a template
 * for the dialog box, prepared using the template editor utility and available
 * as a resource for this program. If there is not enough space to do this then
 * return *errorp=0; error in window creation returns *errorp=errormessage and
 * return 0.
*/

extern void dbox_show(dbox);
extern void dbox_hide(dbox);
/* A call to show when already showing, or hide when already hidden, has no
effect. The show will occur on the screen close to the last menu selection or
caret setting, whichever was the more recent. */

extern void dbox_showstatic(dbox);
/* This form should be used instead of dbox_show when the dbox is not part
of a menu tree, but persists for a period of time on the screen. */

/* send a close request to a given dbox
*/
extern void dbox_sendclose(dbox d);

/* send (or do) a front request to a given dbox
*/
extern void dbox_sendfront(dbox d, BOOL immediate);

extern void dbox_dispose(dbox *);
/* The * is for compatibility with Modula-2. */


/* -------- dbox Fields. -------- */

/*
A dbox has a number of fields, labelled from 0. There are the following
distinct field types:

"action" fields. Mouse clicks here are communicated to the client. The fields
are usually labelled "go", "quit", etc. Set/GetField apply to the label on
the field, although this is usually set up in the template.

"output" fields. These display a message to the user, using SetField. Mouse
clicks etc. have no effect.

"input" fields. The user can type into these, and simple local editing is
provided. Set/GetField can be used on the textual value, or Set/GetNumeric if
the user should type in numeric values.

"on/off" fields. The user can click on these to display their on/off status.
They are always "off" when the dbox is first created. The template editor
can set up mutually exclusive sets of these at will. Set/GetField apply to
the label on this field, Set/GetNumeric set/get 1 (on) and 0 (off) values.
*/

typedef int dbox_field; /* a field number */
typedef enum {
  dbox_FACTION, dbox_FOUTPUT, dbox_FINPUT, dbox_FONOFF
} dbox_fieldtype;

extern void dbox_setfield(dbox d, dbox_field f, const char *text);
extern void dbox_getfield(dbox d, dbox_field f, char *buffer /*out*/, size_t size);
/* If the result (plus a 0-terminator) is larger than the buffer then the
result will be truncated. */

void dbox_setnumeric(dbox, dbox_field, int);
/* If the field is an input or output field then the value is converted into
a string, and displayed as the data in the field. */
int dbox_getnumeric(dbox, dbox_field);

/* When fields are updated on a dbox that is showing, the update appears
immediately on the screen without any calls to Events.ProcessEvent being
required. Thus, messages of the form "wait..." during the processing of
an action key will work fine. */


/* -------- Arrival of events from dboxes. -------- */

/* A dbox acts as an input device: a stream of characters comes from it
somewhat like a keyboard, and an up-call can be arranged when input is
waiting. See analogous facilities in Text objects. */

extern dbox_field dbox_get(dbox);
/* When an action field activated, a value is generated to indicate which
one. */

#define dbox_CLOSE  ((dbox_field) -1)
/* dboxes may have a "close" button that is separate from their action buttons,
 * usually in the header of the window. If this is pressed then CLOSE is
 * returned; this should lead to the dbox being invisible. If the dbox
 * represents a particular pending operation then the operation should be
 * cancelled
*/

#define dbox_OK     ((dbox_field) 0)
/* 'OK' buttons are conventionally icon number 0 (RETURN 'hits' this field) */


extern int dbox_queue(dbox);
/* The number of times dbox_get will return immediately. */
/* Current implementation restriction: maximum queue size = 1. */

extern void dbox_fakeaction(dbox, dbox_field);
/* Insert this field at the front of the get queue. */

typedef void (*dbox_handler_proc)(dbox, void *handle);

extern void dbox_eventhandler(dbox, dbox_handler_proc, void* handle);
/* The dbox will call the registered event handler when there is a field
waiting that get can pick up. If handler=0 then no procedure is
registered. The event proc may make use of any of the procedures in this
interface (i.e. no internal locks are held during this call). If the dbox is
disposed then the disposal will be delayed until return from this call. */


/* -------- Intercepting Raw Events. -------- */

typedef BOOL (*dbox_raw_handler_proc)(dbox, void *event, void *handle);
/* Return TRUE -> the event has been handled, else pass to default. */
/* In Arthur terms: event is a wimp_eventstr* */

extern void dbox_raw_eventhandler(dbox d, dbox_raw_handler_proc handler, void *handle);
/* Register a procedure that is given raw window system events before
being processed in the normal way. This allows system-specific extensions
and specialisations of a dbox. */

/* yield the current raw_eventhandling values */
extern void dbox_raw_eventhandlers(dbox d, dbox_raw_handler_proc *handlerp, void **handlep);


/* -------- Event processing. -------- */

/* dboxes are often used to fill in the details of a pending operation. In
this case a down-call driven interface to the entire interaction is often
convenient. The following facilties aid this form of use. */

dbox_field dbox_popup(const char *name, const char *message);
/* Build a "name" dbox, assign the message to field 1, wait until there's
some input, destroy the dbox, and return with the field from which the input
came. This can be used for error boxes, yes/no boxes, etc. If insistent it
will block harmful messages to any other window, otherwise the arrival of such
messages to other windows will cause a CLOSE result. */

/* A "harmless" event is one that is unlikely to affect the correct workings
of a dialog. These include redraw, move, grow and scroll operations.
Keystrokes and mouse clicks, on the other hand, are classed as "harmful". */

/* Note that popup is simply a new, show, fillin, dispose sequence. */

dbox_field dbox_fillin(dbox);
/* Events are processed until a field is activated, the value of that field
is then returned. Processing of events to other windows is as for popup.
*/

extern BOOL dbox_persist(void);
/* Call this when fillin has returned an action event. If TRUE then the
user intends that the action should be performed but that the menu tree
and dialogue box should stay. So, continue round the fillin loop rather than
destroying the dbox. */

/* Typical use of fillin:
    d = dbox_new("foo");
    |* ... set up initial values *|
    dbox_show(d);
    while () {
      switch (dbox_fillin(d, 0)) {
      case |* each button *|:
          |* appropriate action *|
      case dbox_CLOSE:
          EXIT;
      default:
        |* do nothing, loop. *|
      };
    };
    dbox_dispose(d);
Alternatively d can be a static variable, which will maintain field values
between use. A menu can be attatched to the dbox using syshandle, allowing
a selection of reasonable values to be offered for some fields.
*/


/* -------- System hook. -------- */

extern int dbox_syshandle(dbox);
/* Obtain a system-dependent handle on the object underlying a dbox. This can
be used to hang a menu on a dbox, e.g. to provide help in filling in a field
of the dbox. dbox_dispose destroys any menu attatched to the dbox, in order
to help this. */
/* In Arthur terms, this is a wimp_w. */


extern void *dbox_syswindowptr(const char *dname);
/* Obtain a system-dependent handle on the dbox definition */

extern void dbox_setinittitle(const char *dname, const char *title);
/* Set the initial dbox title */


/* -------- Initialisation. -------- */

extern void dbox_init(void);
/* Must be called exactly once, before any other calls to this interface. */
/* Reads in the templates for this program. */


#define dbox_field_to_icon(d, f) f


/* end of dbox.h */
