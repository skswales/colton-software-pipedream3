/*
 * Title: akbd.c
 * Purpose: Access to Archimedes keyboard under the Wimp.
 * Author: W. Stoye
 * Status: Arthur-specific
 * History:
 *   13-Oct-87: started
 *   13-Dec-87: converted to C.
*/

#include "include.h"

#include "kernel.h"

#include "akbd.h"


static int
akbd__poll(int x)
{
    int r = _kernel_osbyte(0x81, x, 255);
    return((r & 0x0000FFFF) == 0x0000FFFF);     /* X and Y both 255? */
}


extern int
akbd_pollsh(void)
{
    return(akbd__poll(-1));
}


extern int
akbd_pollctl(void)
{
    return(akbd__poll(-2));
}


extern int
akbd_pollalt(void)
{
    return(akbd__poll(-3));
}


#if !defined(SMALL)

int akbd_pollkey(int *keycode /*out*/)
{
  int x = 0;
  int y = 0;
  (void) os_byte(129, &x, &y);
  tracef2("PollKey returns %i %i.\n", x, y);
  if (y==0 && x==0) {
    /* it's a function key: 0, followed by the actual code. */
    x = 0;
    y = 0;
    (void) os_byte(129, &x, &y);
    if (y==0 && x>=128) {
      /* bona fide function key */
      *keycode = 256 + x;
    } else {
      /* he's typing ahead with a control-@: sorry boy, you loose
      the next key. */
      *keycode = 0;
    };
    return(1);
  } else {
    *keycode = x;
    return(y==0);
  };
}
#endif


/* end of akbd.c */
