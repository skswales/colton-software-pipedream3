/****************************************************************************
 * This source file was written by Acorn Computers Limited. It is part of   *
 * the RISCOS library for rendering RISCOS Draw files from applications     *
 * in C. It may be used freely in the creation of programs for Archimedes.  *
 * It should be used with Acorn's C Compiler Release 3 or later.            *
 *                                                                          *
 ***************************************************************************/

/* 
 * Title:  drawfdiag.h
 * Purpose: Processing of Draw format files (diagram level interface)
 *
 */

/*
 *
 * This file defines the interface to the simplest version of the
 * DrawFile module. It can read in files to diagrams and render them.
 * There is no checking of whether we have overrun the end of the
 * diagram.
 *
 * To read in Draw files, it is expected that the caller will do the
 * work of the I/O themselves.
 *
 * To dispose of a diagram, the caller can just throw it away, ie,
 * the module does not keep any hidden information about what
 * diagrams it has seen.
 *
 * Note on returning errors: some calls return an offset to the bad data on
 * an error. This is not necessarily the start of an object: it may be bad
 * data part way through it. The offset is relative to the start of the
 * diagram.
 *
 * The module cannot handle rectangle or ellipse objects. Use a path instead.
 */

#ifndef __drawfdiag_h
#define __drawfdiag_h

#ifndef __os_h
#include "os.h"
#endif

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif

/********************************* Data types ******************************/

/*
 * Type for a diagram: it consists of a pointer to the data and a
 * length field. The length must be an exact number of words, and is
 * the amount of space used in the diagram, not the size of the
 * memory allocated to it.
 */

typedef struct
{ char * data;
  int  length;
} draw_diag;


/* --- Abstract handle for an object --- */
/* The object handle is an offset from the start of the diagram to the
 * object data.  You may use it to set a pointer directly to an object, 
 * when using the object level interface
 */

typedef int draw_object;

/* --- Rectangular box: bounding boxes, etc. --- */

typedef struct {int x0, y0, x1, y1;} draw_box;

/* --- Redraw structure: similar to a wimp_redrawstr --- */

typedef struct
{
  int      reserved;
  draw_box box;         /* Work area box (x0, y1 used) */
  int      scx, scy;    /* Scroll positions */
  draw_box g;           /* Graphics window box */
} draw_redrawstr;


/* Error type.
 * Where a routine can produce an error, the actual value returned is a BOOL,
 * which is TRUE if the routine succeeded. The error itself is returned in a
 * block passed by the user; if NULL, then the details of the error are not
 * passed back.
 * The error block may contain either an operating system error, or an
 * internal error. In the latter case, it consists of a code and possibly a
 * pointer to the location in the file where the error occurred (if NULL,
 * the location is not known or not specified). By convention, this should
 * be reported by the caller in the form '<message> (location &xx in file)'.
 * For a list of codes and standard errors, see h.DrawErrors. The location is
 * relative to the start of the data block in the diagram.
 *
 */

typedef struct
{
  enum { DrawOSError, DrawOwnError, None } type;
  union
  {
    os_error os;
    struct { int  code; int location; } draw;
  } err;
} draw_error;


/*----------------------------- Macros ---------------------------*/

/* Macros for unit conversion between Draw units and screen units */

#define draw_drawToScreen(i) ((i) >> 8)
#define draw_screenToDraw(i) ((i) << 8)

/****************************** Function declarations **********************/


/* ------------------------- draw_verify_diag ------------------------------
 * Description:   Verifies a diagram which has been read in from file
 *
 * Parameters:    draw_diag *diag -- the diagram to be verified
 *                draw_error *error -- the first error encountered (if any)
 * Returns:       True if diagram is correct.
 * Other Info:    Each object in the file is checked and the first error
 *                encountered causes return (with error set appropriately)
 *
 */

BOOL draw_verify_diag(draw_diag *diag, draw_error *error);


/* ------------------------- draw_append_diag ------------------------------
 * Description:   Merges two diagrams into one.
 *
 * Parameters:    draw_diag *diag1 -- diagram to which to append diag2
 *                draw_diag *diag2 -- diagram to be appended to diag1
 *                draw_error *error -- possible error condition.
 * Returns:       True if merge was successful.
 * Other Info:    Both diagrams should have been processed by
 *                draw_verify_diag(). Diag1's data block must be at least
 *                diag1.length+diag2.length. Diag1.length will be updated
 *                to its new appropriate value. Diag1's bounding box will
 *                be set to the union of the bounding boxes of the two
 *                diagrams. NOTE: offsets of objects in diag1 may change
 *                due to a change in font table size (if diag2 has fonts).
 *                Errors referring to specific locations, refer to diag2.
 *
 */

BOOL draw_append_diag(draw_diag *diag1, draw_diag *diag2, draw_error *error);


/* -------------------------- draw_render_diag -----------------------------
 * Description:   Render a diagram with a given scale factor, in a given
 *                WIMP redraw rectangle
 *
 * Parameters:    draw_diag *diag -- the diagram to be rendered
 *                draw_redrawstr *r -- the WIMP redraw rectangle
 *                double scale -- scale factor
 *                draw_error *error -- possible error condition
 *
 * Returns:       True if render was successful
 * Other Info:    The diagram must have been processed by
 *                draw_verify_diag(). Note that draw_redrawstr is the same
 *                as wimp_redrawstr, which may be cast to it.
 *                Very small and negative scale factors will result in a
 *                run-time error (safe > 0.00009). The caller should do
 *                range checking on the scale factor.
 *                Note: following the normal convention for coordinate
 *                mapping, the part of the diagram rendered is found by
 *                mapping the top left of the diagram, in draw coord space
 *                onto a point:
 *                   (r->box.x0 - r->scx, r->box.y1 - r->scy)
 *                in screen coordinates.
 *
 */
  
BOOL draw_render_diag(draw_diag *diag, draw_redrawstr *r, double scale,
                      draw_error *error);


/************************** Memory allocation functions ********************/

typedef int  (*draw_allocate)(void **anchor, int n);
typedef int  (*draw_extend)(void **anchor, int n);
typedef void (*draw_free)(void **anchor);


/* ---------------------- draw_registerMemoryFunctions ---------------------
 * Description:   Register three functions to be used to allocate, extend
 *                and free memory, when rendering text objects
 *
 * Parameters:    draw_allocate alloc -- pointer to function to be used for
 *                                       memory allocation
 *                draw_extend extend  -- pointer to function to be used for
 *                                       memory extension
 *                draw_free   free    -- pointer to function to be used for
 *                                       memory freeing.
 * Returns:       void.
 * Other Info:    These three functions will be used only when rendering text
 *                area objects. Any memory allocated during rendering will
 *                be freed (using the supplied function) after rendering.  
 *                If draw_registerMemoryFunctions() is never called,
 *                or if memory allocation fails, then an attempt to render
 *                a text area will produce no effect.
 *                The three functions should operate as follows:
 *                  int alloc(void **anchor, int n) : 
 *                          allocate n bytes of store and set *anchor
 *                          to point to them.
 *                          Return 0 if store can't be allocated, otherwise
 *                          non-zero.
 *                  int extend (void **anchor, int n):
 *                          extend the block of memory which starts at 
 *                          *anchor to a total size of n bytes. Note that
 *                          n will always be positive, and the new memory
 *                          should be appended to the existing block (which
 *                          may be moved by the operation).
 *                          Return 0 if the memory can't be allocated, else
 *                          non-zero.
 *                  void free(void **anchor):
 *                          free the block of memory which starts at
 *                          *anchor, and set *anchor to 0.
 *
 *                **NOTE** : The specification for these three functions is
 *                           the same as that for flex_alloc, flex_extend
 *                           and flex_free (in the flex module), so these
 *                           can be used as the three required functions!!
 *
 */

void draw_registerMemoryFunctions(draw_allocate alloc,
                                  draw_extend   extend,
                                  draw_free     free);


/* -------------------------- draw_shift_diag ------------------------------
 * Description:   Shift a diagram by a given distance.
 *
 * Parameters:    draw_diag *diag -- the diagram to be shifted
 *                int xMove -- distance to shift in x direction
 *                int yMove -- distance to shift in y direction.
 * Returns:       void.
 * Other Info:    All coordinates in the diagram are moved by the given
 *                distance.
 *
 */

void draw_shift_diag(draw_diag *diag, int xMove, int yMove);


/* ---------------------------- draw_querybox ------------------------------
 * Description:   Find the bounding box of a diagram.
 *
 * Parameters:    draw_diag *diag -- the diagram
 *                draw_box *box  -- the returned bounding box
 *                BOOL screenUnits -- indication whether the box is to be
 *                                    specified in draw or screen units.
 * Returns:       void.
 * Other Info:    The bounding box of diag is returned in box. If 
 *                screenUnits is true, box is in screen units, else in draw
 *                units.
 *
 */

void draw_queryBox(draw_diag *diag, draw_box *box, BOOL screenUnits);


/* ----------------------------- draw_convertBox ---------------------------
 * Description:   Convert a box to/from screen coordinates.
 *
 * Parameters:    draw_box *from -- box to be converted
 *                draw_box *to   -- converted box.
 *                BOOL toScreen  -- should set to TRUE if conversion is to
 *                                  be from draw coords to screen coords
 *                                  FALSE means from screen coords to draw
 *                                  coords.
 * Returns:       void.
 * Other Info:    from and to may point to the same box.
 *
 */

void draw_convertBox(draw_box *from, draw_box *to, BOOL toScreen);


/* --------------------------- draw_rebind_diag ----------------------------
 * Description:   Force the header of a diagram's bounding box to be exactly
 *                the union of the objects in it.
 *
 * Parameters:    draw_diag *diag -- the diagram.
 * Returns:       void.
 * Other Info:    The diagram should have been processed by
 *                draw_verify_diag() first.
 *
 */

void draw_rebind_diag(draw_diag *diag);



/************************ Unknown object handling **************************/

/* 
 * New types of object can be added by registering an unknown object handler.
 * The handler is called whenever an attempt is made to render an object 
 * whose tag is not one of the standard ones known to DrawFile. It is passed
 * a pointer to the object to be rendered (cast to a void *), and a pointer
 * to a block into which to write any error status. The object pointer may
 * be cast to one of the standard Draw types (defined in the object level
 * interface), or to a client-defined type. If an error occurs, the handler
 * must return FALSE and set up the error block; otherwise it must return
 * TRUE. Unknown objects must conform to the standard convention for object
 * headers, i.e. 1-word object tag; 1-word object size; 4-word bounding box.
 * The unknown object handler is only called if the object is visible, i.e.
 * if there is an overlap between its bounding box and the region of the
 * diagram being rendered. The object size field must be correct, otherwise
 * catastrophes will probably result. 
 *
 */

/* Type for unknown object handler */

typedef BOOL (*draw_unknown_object_handler)(void *object, void *handle,
                                            draw_error *error);


/* ------------------------ draw_set_unknown_object_handler ----------------
 * Description:   Registers a function to be called when an attempt is made
 *                to render an object with an object tag which is not known
 *                
 * Parameters:    draw_unknown_object_handler handler -- the handler function
 *                void *handle -- arbitrary handle to pass to function
 * Returns:       The previous handler.
 * Other Info:    The handler can be removed by calling with 0 as a 
 *                parameter.
 *
 */

draw_unknown_object_handler draw_set_unknown_object_handler
                           (draw_unknown_object_handler handler, void *handle);

#endif

/* end drawfdiag.h */

