/* > h.flex */

/* Title  : flex.h
 * Purpose: provide memory allocation for interactive programs requiring
 *          large chunks of store. Such programs must respond to memory
 *          full errors, and must not suffer from fragmentation.
*/


#ifndef __flex_h
#define __flex_h


typedef void **flex_ptr;
typedef void (*flex_notifyproc)(void);


/* Allocates n bytes of store, which must always be accessed indirectly via
 * *anchor (i.e. (*anchor)[0]..(*anchor)[n-1]). The first argument should be &
 * of some char* variable. flex retains knowledge of this address, so that it
 * can move the allocated store if necessary, updating this char* variable.
 * Because of this the allocated store should always be accessed via this
 * variable. Returns 0 in *anchor, with situation unchanged, if there is not
 * enough store.
*/
extern int flex_alloc(flex_ptr anchor, int n);

/* The flexible store area is extended or truncated to have the new size.
 * Returns 0 (with situation unchanged) if there is not enough store, this can
 * only happen if extension is being requested. Otherwise, returns 1.
*/
extern int flex_extend(flex_ptr anchor, int newsize);

/* return amount of store available from wimp */
extern int flex_extrastore(void);

/* frees area and sets *anchor to 0 */
extern void flex_free(flex_ptr anchor);

/* call before any other calls to this interface. */
extern int flex_init(void);

/* If by is positive then the flexible store area is extended, and locations
 * above (at) are copied up by (by). If by is negative then the flexible store
 * area is reduced, and any bytes beyond (at) are copied down to (at+by).
 * Returns FALSE (with situation unchanged) if there is not enough store, this
 * can only happen if by is positive.
*/
extern int flex_midextend(flex_ptr anchor, int at, int by);

/* move a block of anchor pointers */
extern void flex_move_anchors(void **oldstart, int block_size, int move_amount);
                           
/* set up notification procedure */
extern void flex_set_notify(flex_notifyproc proc);

/* return the exact number of bytes in a flex store area. */
extern int flex_size(flex_ptr anchor);

/* A number to tell the user, ie. only approximate: number of bytes free. */
extern int flex_storefree(void);

/* granularity of allocation available from OS */
extern int flex_pagesize;

#if TRACE
extern void flex_display(void);

/* return the start and end of the allocated part of the flex area,
 * how much is left free in and the end of the flex area
*/
extern void flex_limits(void **start, void **end, int *free, int *size);
#endif


#endif /* __flex_h */


/* end of flex.h */
