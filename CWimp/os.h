/* > h.os */

/*
 * Title  : h.os
 * Purpose: provides general access to low-level RISC OS routines
 * Version: 0.1     created
 *          0.2 SKS added comments
 * SKS made some const char *s
 */

#ifndef __os_h
#define __os_h

/*
 * This header is provided as an alternative to Arthur.h.
 * It can be used in concert with, or instead of, that file.
 * It provides low-level access to RISC OS.
 * Higher-level facilities can be found in other library files.
 */

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
        int r[10];               /* only r0 - r9 matter for SWIs */
} os_regset;

typedef struct {
        int errnum;             /* error number */
        char errmess[252];      /* error message (zero terminated) */
} os_error;

/*
 * os_error * functions return a pointer to an error if it has occurred,
 * otherwise return NULL (0)
 */

extern void os_swi(int swicode, os_regset* /*inout*/);
/* Perform the given SWI instruction, with the given registers loaded.
   An error results in an RISC OS error being raised, which normally means that
   the C runtime library fakes a VS return.
*/
/* A NULL regset pointer means that no inout parameters are used */

#define os_X (0x00020000)

extern os_error *os_swir(int swicode, os_regset* /*inout*/);
/* as os_swix below, but always returns NULL for non X form SWIs */

extern os_error *os_swix(int swicode, os_regset* /*inout*/);
/* swicode will always be ORed with os_X. */
/* A NULL regset pointer means that no inout parameters are used */

/* Calls returning os_error* use the X form of the relevant SWI. If an error
   is returned then the os_error should be copied before further system calls
   are made. If no error occurs then NULL is returned.
*/

/* Important note on the following functions:
   if swicode does not have the X bit set, then os_swi is called and these
   functions return NULL (regardless of whether an error was raised);
   please try to use X bit set swicodes to save confusion.
*/
os_error *os_swi0(int swicode); /* zero arguments and results */
os_error *os_swi1(int swicode, int r0);
os_error *os_swi2(int swicode, int r0, int r1);
os_error *os_swi3(int swicode, int r0, int r1, int r2);
os_error *os_swi4(int swicode, int r0, int r1, int r2, int r3);
os_error *os_swi6(int swicode, int r0, int r1, int r2, int r3, int r4, int r5);

os_error *os_swi1r(int swicode, int r0in, int *r0out);
os_error *os_swi2r(int swicode, int r0in, int r1in, int *r0out, int *r1out);
os_error *os_swi3r(int swicode, int, int, int, int*, int*, int*);
os_error *os_swi4r(int swicode, int, int, int, int, int*, int*, int*, int*);
os_error *os_swi6r(int swicode,
  int r0, int r1, int r2, int r3, int r4, int r5,
  int *r0out, int *r1out, int *r2out, int *r3out, int *r4out, int *r5out);
/* SWI with varying numbers of arguments and results. NULL result pointers
mean that the result from that register is not required. The swi codes can
be of the X form if required, as specified by swicode. */


os_error *os_byte(int a, int *x /*inout*/, int *y /*inout*/);

os_error *os_word(int wordcode, void*);

typedef struct {
        int action;             /* specifies action of osgbpb */
        int file_handle;        /* file handle, but may be used as a char *
                                 * pointing to wildcarded dir-name */
        void *data_addr;        /* memory address of data */
        int number, seq_point, buf_len;
        const char *wild_fld;   /* points to wildcarded filename to match */
        int reserved[3];        /* space to allow treatment as reg_block */
} os_gbpbstr;

os_error *os_gbpb(os_gbpbstr*);

typedef struct {
        int action;             /* action or object type if output data */
        const char * name;      /* pointer to filename or pathname */
        int loadaddr, execaddr; /* load, exec addresses */
        int start, end;         /* start address/length, end add./attributes */
        int reserved[4];        /* space to allow treatment as reg_block */
} os_filestr;

os_error *os_file(os_filestr *);

os_error *os_args(os_regset *);

os_error *os_find(os_regset *);

os_error *os_cli(const char *cmd);

os_error *os_cli(const char *cmd);

#if defined(PEDANTIC)
#pragma -v1
#endif
os_error *os_clif(const char *cmd, ...);
#if defined(PEDANTIC)
#pragma -v0
#endif

void os_read_var_val(const char *name, char *buf /*out*/, int bufsize);
/* returns a null string if the variable is not present. */

#endif  /* __os_h */

/* end of h.os */
