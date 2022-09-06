/* > c.os */

/* Title:   os.c
 * Purpose: provides general access to low-level RISC OS routines
 * Version: 0.1 WRS created
 *          0.2 SKS made os_cli and os_read_var_val use const char *
*/

#include "include.h"


#include <stdio.h>


#include "swi.h" /* asm implementation also present */

#include "CModules:swinumbers.h"


extern os_error *
os_swi0(int swicode)
{
    os_regset r;
    return(os_swir(swicode, &r));
}


extern os_error *
os_swi1(int swicode, int r0)
{
    os_regset r;
    r.r[0] = r0;
    return(os_swir(swicode, &r));
}


extern os_error *
os_swi2(int swicode, int r0, int r1)
{
    os_regset r;
    r.r[0] = r0;
    r.r[1] = r1;
    return(os_swir(swicode, &r));
}


extern os_error *
os_swi3(int swicode, int r0, int r1, int r2)
{
    os_regset r;
    r.r[0] = r0;
    r.r[1] = r1;
    r.r[2] = r2;
    return(os_swir(swicode, &r));
}


extern os_error *
os_swi4(int swicode, int r0, int r1, int r2, int r3)
{
    os_regset r;
    r.r[0] = r0;
    r.r[1] = r1;
    r.r[2] = r2;
    r.r[3] = r3;
    return(os_swir(swicode, &r));
}


extern os_error *
os_swi6(int swicode, int r0, int r1, int r2, int r3, int r4, int r5)
{
    os_regset r;
    r.r[0] = r0;
    r.r[1] = r1;
    r.r[2] = r2;
    r.r[3] = r3;
    r.r[4] = r4;
    r.r[5] = r5;
    return(os_swir(swicode, &r));
}


extern os_error *
os_swi1r(int swicode, int r0, int *r0out)
{
    return(os_swi4r(swicode, r0, NULL, NULL, NULL, r0out, NULL, NULL, NULL));
}


extern os_error *
os_swi2r(int swicode, int r0, int r1, int *r0out, int *r1out)
{
    return(os_swi4r(swicode, r0, r1, NULL, NULL, r0out, r1out, NULL, NULL));
}


extern os_error *
os_swi3r(int swicode,
    int r0,     int r1,     int r2,
    int *r0out, int *r1out, int *r2out)
{
    return(os_swi4r(swicode, r0, r1, r2, NULL, r0out, r1out, r2out, NULL));
}


extern os_error *
os_swi4r(int swicode,
    int r0,     int r1,     int r2,     int r3,
    int *r0out, int *r1out, int *r2out, int *r3out)
{
    os_regset r;
    os_error *err;

    r.r[0] = r0;
    r.r[1] = r1;
    r.r[2] = r2;
    r.r[3] = r3;

    err = os_swir(swicode, &r);

    if(r0out) *r0out = r.r[0];
    if(r1out) *r1out = r.r[1];
    if(r2out) *r2out = r.r[2];
    if(r3out) *r3out = r.r[3];

    return(err);
}


extern os_error *
os_swi6r(int swicode,
    int r0,     int r1,     int r2,     int r3,     int r4,     int r5,
    int *r0out, int *r1out, int *r2out, int *r3out, int *r4out, int *r5out)
{
    os_regset r;
    os_error *err;

    r.r[0] = r0;
    r.r[1] = r1;
    r.r[2] = r2;
    r.r[3] = r3;
    r.r[4] = r4;
    r.r[5] = r5;

    err = os_swir(swicode, &r);

    if(r0out) *r0out = r.r[0];
    if(r1out) *r1out = r.r[1];
    if(r2out) *r2out = r.r[2];
    if(r3out) *r3out = r.r[3];
    if(r4out) *r4out = r.r[4];
    if(r5out) *r5out = r.r[5];

    return(err);
}


extern os_error *
os_byte(int a, int *x /*inout*/, int *y /*inout*/)
{
    return(os_swi3r(os_X | OS_Byte, a, *x, *y, 0, x, y));
}


extern os_error *
os_word(int wordcode, void *p /*inout*/)
{
    os_regset r;
    r.r[0] = wordcode;
    r.r[1] = (int) p;
    return(os_swix(OS_Word, &r));
}


extern os_error *
os_gbpb(os_gbpbstr *p)
{
    return(os_swix(OS_GBPB, (os_regset *) p));
}


extern os_error *
os_file(os_filestr *p)
{
    return(os_swix(OS_File, (os_regset *) p));
}


extern os_error *
os_args(os_regset *p)
{
    return(os_swix(OS_Args, (os_regset *) p));
}


extern os_error *
os_find(os_regset *p)
{
    return(os_swix(OS_Find, (os_regset *) p));
}


extern os_error *
os_cli(const char *cmd)
{
    os_regset r;
    r.r[0] = (int) cmd;
    return(os_swix(OS_CLI, &r));
}


extern os_error *
os_clif(const char *cmd, ...)
{
    char buffer[256];
    va_list argp;

    va_start(argp, cmd);

    vsprintf(buffer, cmd, argp);

    #if !defined(VA_END_SUPERFLUOUS)
    va_end(argp);
    #endif

    return(os_cli(buffer));
}


#if FALSE

extern void
os_read_var_val(const char *name, char *buf /*out*/, int bufsize)
{
  os_regset r;
  os_error *err;

  r.r[0] = (int) name;
  r.r[1] = (int) buf;
  r.r[2] = bufsize;
  r.r[3] = 0;
  r.r[4] = 3;   /* expand macros */
  err = os_swix(OS_ReadVarVal, &r);
  if (err != 0) /* variable not found etc. */
    *buf = 0;
  else
    buf[r.r[2]] = 0;
}

#endif


#if FALSE

extern os_error *
os_swir(int swicode, os_regset *r /*inout*/)
{
    if((swicode & os_X) != 0)
        return(os_swix(swicode, r));

    os_swi(swicode, r);
    return(NULL);
}

#endif


/* end of os.c */
