/****************************************************************************
 * This source file was written by Acorn Computers Limited. It is part of   *
 * the RISCOS library for rendering RISCOS Draw files from applications     *
 * in C. It may be used freely in the creation of programs for Archimedes.  *
 * It should be used with Acorn's C Compiler Release 3 or later.            *
 *                                                                          *
 ***************************************************************************/

/* 
 * Title:   drawfobj.h
 * Purpose: Processing of Draw format files (object level interface)
 *
 */

/*
 *
 * This file supplements the diagram level interface with routines for
 * dealing with individual objects.
 */

#ifndef __drawfobj_h
#define __drawfobj_h

#ifdef DRAWFILE_INTERNAL
#include "h.DrawIntern.DrawCvtTyp"
#else
/* Data types for specifying object details */
#ifndef __drawfdiag_h
#include "h.drawfdiag"
#endif
#ifndef __drawftypes_h
#include "h.drawftypes"
#endif
#endif

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif



/******************************** DATA TYPES *******************************/

/* 
 * Data type for specifying the full details of any object type.
 * It is used in operations on complete objects, such as creation
 * and querying, and so does not include the types needed for
 * contained structures and types, such as path elements or text
 * columns : those are defined in h.drawftypes.
 *
 */

typedef union
{
  draw_objhdr      *object;
  draw_fileheader  *fileHeader;
  draw_fontliststr *fontList;
  draw_textstr     *text;
  draw_pathstr     *path;
  draw_spristr     *sprite;
  draw_groustr     *group;
  draw_textareahdr *textarea;
  char             *bytep;
  int              *wordp;
} draw_objectType;



/* Symbolic code for no object */

#define draw_NoObject (draw_object)-1


/* 
 * For functions that operate on a range of objects (and ONLY those 
 * functions), the following special codes may be used to mean 'from the
 * start' and 'to the end'.
 *
 */

#define draw_FirstObject (draw_object)-1
#define draw_LastObject  (draw_object)-2



/********************************* Global data *****************************/
/* An array of 256 integers, used for font translation. Immediately after 
 * a call to draw_createObject where the object was a font table, this 
 * contains the mapping between old and new font reference numbers, such 
 * that  transTable[old fontref] = new fontref.
 * draw_translateText makes use of this table; it is exported for
 * callers who may want to do something elaborate.
 */

extern int draw_transTable[];

/************************** Interface functions ****************************/


/* --------------------------- draw_create_diag ----------------------------
 * Description:   Create an empty diagram (ie. just the file header), with
 *                a given bounding box.
 *
 * Parameters:    draw_diag *diag -- pointer to store to hold diagram
 *                char *creator -- pointer to character string holding
 *                                 creator's name
 *                draw_box bbox -- the bounding box (in Draw units).
 * Returns:       void.
 * Other Info:    diag must point at sufficient memory to hold the diagram.
 *                The first 12 chars of creator are stored in the file
 *                header. diag.length is set appropriately by this function.
 *
 */
 
void draw_create_diag(draw_diag *diag, char *creator, draw_box bbox);


/* -------------------------- draw_doObjects -------------------------------
 * Description:   Render a specified range of objects from a diagram.
 *
 * Parameters:    draw_diag *diag -- the diagram
 *                draw_object start -- start of range of objects to be
 *                                     rendered
 *                draw_object end   -- end of range of objects to be rendered
 *                draw_redrawstr *r -- WIMP-style redraw rectangle
 *                double scale -- the scale factor for rendering
 *                draw_error *error -- possible error condition.
 * Returns:       TRUE if render was successful.
 * Other Info:    Parameters (except range) are used as in draw_render_diag,
 *                in diagram level module.
 *                The diagram must be verified before a call to this function
 *                If the range of objects inclues text with anti-aliasing
 *                fonts, you MUST call draw_setFontTable first.
 *                Very small (<0.00009) or negative scale factors will cause
 *                run-time errors.
 *
 */

BOOL draw_doObjects(draw_diag *diag, draw_object start, draw_object end,
                    draw_redrawstr *r, double scale, draw_error *error);


/* -------------------------- draw_setFontTable ----------------------------
 * Description:   Scans a diagram for a font table object and records it for
 *                a subsequent call of draw_doObjects.
 *
 * Parameters:    draw_diag *diag -- the diagram to be scanned.
 * Returns:       void.
 * Other Info:    This function must be called for draw_doObjects to work on 
 *                a sequence of objects that includes text objects using
 *                anti-aliasing fonts, but no font table object. The font
 *                table remains valid until either a different one is 
 *                encountered during a call to draw_doObjects, or until
 *                draw_render_diag is called, or until a different diagram
 *                is rendered.
 *
 */

void draw_setFontTable(draw_diag *diag);


/* -------------------------- draw_verifyObject ----------------------------
 * Description:   Verify the data for an existing object in a diagram
 *
 * Parameters:    draw_diag *diag -- the diagram
 *                draw_object object -- the object to be verified
 *                int *size -- gets set to the amount of memory occupied by
 *                             the object.
 *                draw_error *error -- possible error condition.
 * Returns:       TRUE if object found and verified.
 * Other Info:    Verifying an object ensures that its bounding box is
 *                consistent with the data in it; if not, no error is
 *                reported, but the box is made consistent, On an error, the
 *                location is relative to the start of the diagram.
 *                Note: the object's size is only returned if "size" is a
 *                non-null pointer.
 *
 */

BOOL draw_verifyObject(draw_diag *diag, draw_object object, int *size,
                       draw_error *error);


/* -------------------------- draw_createObject ----------------------------
 * Description:   Creates an object after a specified object in a given
 *                diagram.
 *
 * Parameters:    draw_diag *diag -- the diagram
 *                draw_objectType newObject -- the created object
 *                draw_object after -- the object after which the new object
 *                                     should be created
 *                BOOL rebind -- if TRUE, the bounding box of the diagram is
 *                               updated to the union of its existing value
 *                               and that of the new object
 *                draw_object *object -- new object's handle
 *                draw_error *error -- possible error condition
 * Returns:       TRUE if object was created OK
 * Other Info:    All data after the insertion point is moved down. "after"
 *                may be set to draw_FirstObject/draw_LastObject for
 *                inserting at the start/end of the diagram.
 *                Diagram must be large enough for the new data; its length
 *                field is updated.
 *                On an error, the location is not meaningful.
 *                The handle of the new object is returned in "object"
 *                If this function is used to create a font table, "after" 
 *                is ignored, and the object merged with the existing one
 *                (if such exists) or inserted at the start of the diagram
 *                otherwise.  This can cause the font reference numbers to
 *                change; if a call to this function is followed by a 
 *                draw_translateText(), the font change will be applied (only
 *                needed when anti-aliased fonts are used in text objects.
 *                
 */
 
BOOL draw_createObject(draw_diag *diag, draw_objectType newObject,
                       draw_object after, BOOL rebind, draw_object *object,
                       draw_error *error);


/* ---------------------------- draw_deleteObjects -------------------------
 * Description:   Deletes the specified range of objects from a diagram.
 *
 * Parameters:    draw_diag *diag -- the diagram
 *                draw_object start -- start of range of objects to be
 *                                     deleted
 *                draw_object end   -- end of range of objects to be deleted
 *                BOOL rebind -- if set to TRUE, then the diagram's bounding
 *                               box will be set to the union of those 
 *                               remaining objects
 *                draw_error *error -- possible error condition.
 * Returns:       TRUE if objects deleted successfully.
 * Other Info:    diag.length is updated appropriately.
 *
 */

BOOL draw_deleteObjects(draw_diag *diag, draw_object start, draw_object end,
                        BOOL rebind, draw_error *error);


/* ----------------------- draw_extractObject ------------------------------
 * Description:   Extracts an object from a diagram into a supplied buffer
 *
 * Parameters:    draw_diag *diag -- the diagram
 *                draw_object object -- the object to be extracted
 *                draw_objectType result -- pointer to the buffer
 *                draw_error *error -- possible error division
 * Returns:       TRUE if the object was extracted successfully.
 * Other Info:    The buffer for the result must be large enough to hold
 *                the extracted object (an object's size can be ascertained
 *                by calling draw_verifyObject()).
 *
 */

BOOL draw_extractObject(draw_diag *diag, draw_object object,
                        draw_objectType result, draw_error *error);


/* -------------------------- draw_translateText ---------------------------
 * Description:   Updates all font reference numbers for text objects
 *                following creation of a font table.
 *
 * Parameters:    draw_diag *diag -- the diagram.
 * Returns:       void.
 * Other Info:    If the font table has not been changed then this function
 *                does nothing.
 *
 */

void draw_translateText(draw_diag *diag);

#endif

/* end drawfobj.h */

