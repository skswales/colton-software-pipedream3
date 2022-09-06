/*  Title: -> h.poll
 * Dummy header for s.poll
 */

os_error *wimp_poll(wimp_emask mask, wimp_eventstr *result);
void wimp_save_fp_state_on_poll(void) ;
/* Activates saving of floating point state on calls to wimp_poll
 * and wimp_pollidle; this is needed if you do any floating point at
 * all, as other programs may corrupt the FP status word, which is
 * effectively a global in your program
 */
void wimp_corrupt_fp_state_on_poll(void) ;
/* Disables saving of floating point state on calls to wimp_poll
 * and wimp_pollidle; use only if you never use FP at all
 */

os_error *wimp_pollidle(wimp_emask mask, wimp_eventstr *result, int earliest);
/* Like wimp_poll, but do not return before earliest return time.
This is a value produced by OS_ReadMonotonicTime. */

/* end */
