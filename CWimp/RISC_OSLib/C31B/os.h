/* os.h
 * Copyright (C) Acorn Computers Ltd., 1990
 * Copyright (C) Codemist Ltd., 1990
 */
#ifndef __os_h
#define __os_h
#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
typedef struct {
 int r[10]; 
}os_regset;
typedef struct {
 int errnum; 
 char errmess[252]; 
}os_error;
 
void os_swi(int swicode, os_regset *regs);
 
#define os_X (0x00020000)
os_error *os_swix(int swicode, os_regset *regs);
os_error *os_swi0(int swicode); 
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
os_error *os_byte(int a, int *x , int *y );
os_error *os_word(int wordcode, void *p);
typedef struct {
 int action; 
 int file_handle; 
 void *data_addr; 
 int number, seq_point, buf_len;
 char *wild_fld; 
 int reserved[3]; 
}os_gbpbstr;
os_error *os_gbpb(os_gbpbstr*);
typedef struct {
 int action; 
 char * name; 
 int loadaddr, execaddr; 
 int start, end; 
 int reserved[4]; 
}os_filestr;
os_error *os_file(os_filestr*);
os_error *os_args(os_regset*);
os_error *os_find(os_regset*);
os_error *os_cli(char *cmd);
void os_read_var_val(char *name, char *buf , int bufsize);
#endif
