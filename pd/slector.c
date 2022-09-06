/* slector.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * module that deals with load file selector
 * RJM February 1988
*/

#if defined(__system_io)
/* MUST be before stdio.h */
typedef int FILEHANDLE;
#endif


/* standard header files */
#include "flags.h"


#if ARTHUR || RISCOS
#include "os.h"
#include "bbc.h"
#if RISCOS
#include "kernel.h"
#include <string.h>
#elif ARTHUR
#include "ext.adosdir"
#endif
#if !defined(Z88_OFF)
#include "ext.z88com"
#endif

#define _A_SUBDIR TRUE
#define _A_NORMAL 0xFF

#elif MS
#include <io.h>
#include <dos.h>
#include <direct.h>

#if !defined(Z88_OFF)
#include "z88com.ext"
#endif
#include "pd123.ext"
#else
    assert(0);
#endif


#include "datafmt.h"

#if RISCOS
#include "ext.riscos"
#endif


/* exported functions */

extern char *add_prefix_to_name(char *buffer /*out*/, const char *name, BOOL allow_cwd);
extern BOOL  add_path(char *filename /*out*/, const char *src, BOOL allow_cwd);
extern BOOL  checkoverwrite(const char *name);
extern BOOL  filereadable(const char *name);
extern BOOL  get_cwd(char *buffer /*out*/);
extern void  get_prefix(char *buffer /*out*/, BOOL allow_cwd);
extern BOOL  isrooted(const char *name);
extern FILE *myfopen(const char *name, const char *atts);
extern int   myfclose(FILE *xput);
extern void  mysetvbuf(FILE *xput, void *buffer, size_t bufsize);

#if !defined(SAVE_OFF)
extern BOOL  away_byte(intl ch, FILE *output);
extern BOOL  away_eol(FILE *output);
extern BOOL  away_string(const char *str, FILE *output);
#endif

#if !defined(Z88_OFF) || defined(Z88FS)
extern intl  looks_like_z88(const char *name);
#endif

#if ARTHUR || RISCOS
extern int   myfputc(int ch, FILE *output);
#endif

#if MS || ARTHUR
extern BOOL  getfilename(uchar *array);
#endif


/* internal functions */

#if !defined(FILE_SELECTOR_OFF)
static uchar *next_in_dir(uchar *path, BOOL directory);
#endif

#if RISCOS
/*static void       clear_kernel_error(void);*/
/*static char      *last_kernel_error(void);*/
#elif MS || ARTHUR
#define         clear_kernel_error()    /* does nothing */
#define         last_kernel_error()     ((char *) NULL)
#endif


/* ----------------------------------------------------------------------- */

#if !defined(Z88_OFF)
static intl com_port = 0;
#endif


#if !defined(FILE_SELECTOR_OFF)
struct find_t any_file;

list_block *this_dir;           /* list of drives and files */

static char DOT_STR[] = ".";

#if ARTHUR || RISCOS
static char HAT_STR[] = "^";

#define DIR_SEP         DOT
#define DIR_SEP_STR     DOT_STR
#define PREV_DIR_STR    HAT_STR

#elif MS
#define DIR_SEP         BACKSLASH
#define DIR_SEP_STR     BACKSLASH_STR
#define PREV_DIR_STR    TWO_DOT_STR

#endif
#endif  /* FILE_SELECTOR_OFF */


#if RISCOS

/****************************************************************
*                                                               *
*  bits to deal with RISC OS specific file system error cases   *
*                                                               *
****************************************************************/

static void
clear_kernel_error(void)
{
    (void) _kernel_last_oserror();
}


static char *
last_kernel_error(void)
{
    _kernel_oserror *err = _kernel_last_oserror();

    return((err == NULL) ? NULL : err->errmess);
}

#endif  /* RISCOS */


#if MS || ARTHUR

extern BOOL
isdirectory(char *pathname, char *rest)
{
    char array[LIN_BUFSIZ+1];
#if MS
    unsigned atts = 0;
#endif

    if(rest == NULL)
        return(FALSE);

#if MS
    if(!strcmp(rest, ONE_DOT_STR))
        return(TRUE);

#elif (ARTHUR || RISCOS)  &&  (!defined(Z88_OFF) || defined(Z88FS))
    /* previous directory on Z88 is .. even on ARTHUR || RISCOS */
    if(using_Z88  &&  !strcmp(rest, TWO_DOT_STR))
        return(TRUE);
#endif

    if(!strcmp(rest, PREV_DIR_STR))
        return(TRUE);

    strcpy(array, pathname);

#if ARTHUR || RISCOS
    if(!str_isblank(pathname) && !str_isblank(rest))
        strcat(array, DIR_SEP_STR);
#endif

    strcat(array, rest);

    if(looks_like_z88(array))
        return(FALSE);

#if MS
    _dos_getfileattr(array, &atts);
    return((atts & _A_SUBDIR) != 0);

#elif ARTHUR || RISCOS
    {
    intl res = arch_getfileattr(array);

    if(res < 0)
        rep_fserr(arch_geterr());

    return(res == ARCH_DIR);
    }
#endif
}

#endif  /* MS || ARTHUR */


#if RISCOS

static const char default_path[]   = "<PipeDrea3$Dir>.";
static const char path_separator[] = ",";

#elif ARTHUR

static const char *prefix[] =
    {
    "",
    "piped.",
    "&.piped.",
    "$.piped."
    };
#define NPREFIXES (sizeof(prefix) / sizeof(char *))

#elif MS

static const char path_separator[] = ";";
#define default_path ((const char *) NULLSTR)

#endif


/* currentfilename never > MAX_FILENAME so will not overflow o/p buffer */

extern BOOL
get_cwd(char *buffer /*out*/)
{
    char *namep = currentfilename;
    char *leafp = leafname(namep);
    intl nchars = leafp - namep;

    buffer[0] = '\0';
    if(nchars)
        strncat(buffer, namep, nchars);

    tracef2("get_cwd() yields cwd = \"%s\", has cwd = %s\n",
                buffer, trace_boolstring(nchars));
    return(nchars /*!= 0*/);
}


extern char *
add_prefix_to_name(char *buffer /*out*/, const char *name, BOOL allow_cwd)
{
    if(isrooted(name))
        strcpy(buffer, name);
    else
        {
        get_prefix(buffer, allow_cwd);
        strcat(buffer, name);
        }

    return(buffer);
}


static const char *
get_pd_path(void)
{
    const char *pathptr = getenv(PD_PATH_STR);

    if(!pathptr)
        pathptr = default_path;

    tracef1("get_pd_path() yields \"%s\"\n", pathptr);
    return(pathptr);
}


/********************************************
*                                           *
* get the file anywhere on the current path *
*                                           *
********************************************/

extern BOOL
add_path(char *filename /*out*/, const char *src, BOOL allow_cwd)
{
    char array[LIN_BUFSIZ+1];

#if MS

    const char *pathvar;

    IGNOREPARM(allow_cwd);

    if(src)
        strcpy(filename, src);

    /* is it in current directory? */
    if(filereadable(filename))
        return(TRUE);

    /* get the path */
    pathvar = get_pd_path();

    while(*pathvar)
        {
        char *ptr = array;

        /* copy next directory into array */

        while(*pathvar)
            {
            if(*pathvar == ';')
                {
                pathvar++;
                break;
                }

            *ptr++ = *pathvar++;
            }

        /* add \filename to end */
        /* fixed after 2.21 */
        if(ptr > array  &&  *(ptr-1) != BACKSLASH)
            *ptr++ = BACKSLASH;

        strcpy(ptr, filename);

        if(filereadable(array))
            if(!isdirectory(array, NULLSTR))
                {
                strcpy(filename, array);
                return(TRUE);
                }
        }

#elif ARTHUR

    intl i = (first) ? 0 : 1;

    if(src)
        strcpy(filename, src);

    while(i < NPREFIXES)
        {
        strcpy(array, prefix[i]);
        strcat(array, filename);
        if(filereadable(array))
            {
            strcpy(filename, array);
            return(TRUE);
            }

        i++;
        }

#elif RISCOS

    const char *pathvar;
    const char *pathelem;
    char  strtok_array[MAX_FILENAME];
    intl  count;

    if(src)
        strcpy(filename, src);

    if(isrooted(filename))
        return(filereadable(filename));

    if(allow_cwd  &&  get_cwd(array))
        {
        if(isrooted(filename))
            strcpy(array, filename);
        else
            {
            count = MAX_FILENAME - strlen(array);
            strncat(array, filename, count);
            }

        if(filereadable(array))
            {
            strcpy(filename, array);
            tracef1("add_path() returns filename \"%s\" & TRUE\n", filename);
            return(TRUE);
            }
        }

    pathvar = get_pd_path();

    strcpy(strtok_array, pathvar);

    pathelem = strtok(strtok_array, path_separator);

    while(pathelem != NULL)
        {
        strcpy(array, pathelem);
        strcat(array, filename);

        if(filereadable(array))
            {
            strcpy(filename, array);
            tracef1("add_path() returns filename \"%s\" & TRUE\n", filename);
            return(TRUE);
            }

        pathelem = strtok(NULL, path_separator);
        }

#endif

    tracef0("add_path() returns FALSE\n");
    return(FALSE);                          /* no file found */
}


extern void
get_prefix(char *buffer /*out*/, BOOL allow_cwd)
{
    const char *pathvar;
    const char *pathelem;
    char strtok_array[MAX_FILENAME];

    /* if allowed, use current filename prefix if there is one */
    if(allow_cwd  &&  get_cwd(buffer))
        {
        tracef1("get_prefix returns %s\n", buffer);
        return;
        }

    pathvar = get_pd_path();

    strcpy(strtok_array, pathvar);

    pathelem = strtok(strtok_array, path_separator);

    strcpy(buffer, pathelem ? pathelem : default_path);

    tracef1("get_prefix returns %s\n", buffer);
}


/* is the filename rooted? (or user knows what he's doing with '^') */

extern BOOL
isrooted(const char *name)
{
#if RISCOS
    BOOL res = (strpbrk(name, "$&%@\\:") != NULL);
#elif MS
    BOOL res = ((*name == '\\')  ||  (strchr(name, ':') != NULL));
#endif

    tracef2("isrooted(%s) yields %s\n", name, trace_boolstring(res));
    return(res);
}


extern BOOL
checkoverwrite(const char *name)
{
    if(filereadable(name))
        {
        (void) init_dialog_box(D_OVERWRITE);

        if(!dialog_box(D_OVERWRITE))
            return(FALSE);

        dialog_box_end();

        return(d_overwrite[0].option != 'N');
        }

    return(TRUE);
}


#if defined(Z88FS)

/************************************************************************
*                                                                       *
* determine if a filename indicates the z88                             *
* in Z88 filer model in PD3 all z88 filenames start z88:                *
*                                                                       *
************************************************************************/

extern intl
looks_like_z88(const char *name)
{
    return( (toupper(*(name+0)) == 'Z') && 
            (*(name+1) == '8') &&
            (*(name+2) == '8') &&
            (*(name+3) == ':'));
}

#endif  /* Z88FS */

#if !defined(Z88_OFF)

/************************************************************************
*                                                                       *
* determine if a filename indicates the z88                             *
* z88 filenames start z:, z88: or :RAM.                                 *
* in upper or lower or mixture of case                                  *
* Note that z: and z88: always needs to be stripped off before filename *
* goes to z88                                                           *
*                                                                       *
************************************************************************/

static uchar RAM_STR[] = ":RAM.";

extern intl
looks_like_z88(const char *name)
{
    char *ptr;
    char ch;
    intl i;

    /* look for z: or z88: */

    if(toupper(*name) == 'Z')
        {
        intl offset = 0;

        if(name[1] == COLON)
            offset = 2;
        elif(!memcmp(name+1, "88:", 3))
            offset = 4;

        if(offset && isdigit(name[offset]) && name[offset+1] == COLON)
            {
            if(name[offset] == '1')
                com_port = 0;
            elif(name[offset] == '2')
                com_port = 1;
            offset += 2;
            }

        /* do we need to retain the COLON ? */
        if(offset)
            {
            if(!looks_like_z88(name+offset) &&
                looks_like_z88(name+offset-1))
                offset--;

            return(offset);
            }
        }

    /* look for :RAM. */

    for(ptr = (char *) name, i = 0; (ch = RAM_STR[i]) != '\0'; ptr++, i++)
        if(ch != toupper(*ptr))
            return(0);

    return(i);
}

#endif  /* Z88_OFF */


/********************************************
*                                           *
* test for file readabilitly                *
* FALSE if failed to open or open had error *
*                                           *
********************************************/

extern BOOL
filereadable(const char *name)
{
    FILE *fin;

    #if RISCOS
        if(name == CFILE_IN_RAM_BUFFER)
            return(TRUE);
    #endif

    clear_kernel_error();

    fin = myfopen(name, read_str);

    if(fin != NULL)
        myfclose(fin);

    return((fin != NULL)  &&  (last_kernel_error() == NULL));
}


#if RISCOS
typedef struct __extradata {
  /*
   * BODGE BODGE BODGE BODGE
   * This structure is copied from (an old) c.stdio
   */
  unsigned char _a[2];
  long _b;
  unsigned char *_c;
  int _d;
  int _e;
  int _f;
} __extradata, *__extradatap;

static FILE               rambuffer_file;
#if defined(__system_io)
static struct __extradata rambuffer_extradata;
#endif
#endif


/****************************************************
*                                                   *
*  do open, dealing with not ready errors from z88  *
*                                                   *
****************************************************/

/*************************************************************************/
/* Note that this code interacts in a dubious way with the getc macro.   */
/* Also ungetc.                                                          */
/*************************************************************************/

extern FILE *
myfopen(const char *name, const char *atts)
{
    FILE *file;
    char *err;
    #if !defined(Z88_OFF)
    intl offset;
    #endif

    tracef2("myfopen(%s, %s): ",
            (name == CFILE_IN_RAM_BUFFER) ? (const char *) "RamBuffer" : name,
            atts);

    #if RISCOS && FALSE
    if(name == CFILE_IN_RAM_BUFFER)
        {
        unsigned char *buffer;
        int count;
        riscos_rambufferinfo((char **) &buffer, &count);
        #if defined(__system_io)
            /* Bang goes ANSI-conformance! */
            memset(&rambuffer_file,      0, sizeof(FILE));
            memset(&rambuffer_extradata, 0, sizeof(struct __extradata));
            rambuffer_file.__ptr    = buffer;
            rambuffer_file.__icnt   = count;
            rambuffer_file.__flag   = _IOSTRG | _IOREAD;
            rambuffer_file.__base   = buffer;
            rambuffer_file.__extrap = &rambuffer_extradata;
        #else
        #   error   Sorry, but I need to fake an in-core stream somehow!
        #endif
        return(&rambuffer_file);
        }
    #endif

#if defined(Z88_OFF)

    #if RISCOS
    if(!strcmp(atts, write_str))
        {
        char array[10 + MAX_FILENAME];
        sprintf(array, Create_Zs_STR, name);
        /* zero initial size, default atts, datestamped, typed Data */
        err = mysystem(array);
        if(err)
            {
            rep_fserr(err);
            file = NULL;
            }
        else
            file = myfopen(name, update_str);

        return(file);
        }
    #endif

    clear_kernel_error();

    file = fopen(name, atts);

    #if RISCOS
    if((file != NULL)  &&  ((err = last_kernel_error()) != NULL))
        {
        char buffer[252];
        strcpy(buffer, err);
        myfclose(file); /* may itself set kernel error - report first one */
        tracef1("had kernel error %s\n", err);
        rep_fserr(buffer);
        return(NULL);
        }
    #endif

    tracef1("returns &%p\n", file);

#else   /* Z88_OFF */

    offset = looks_like_z88(name);

    if(offset  ||  using_Z88)
        {
        char *ptr = (char *) name;

        while(*ptr++ == SPACE)
            ;
        ptr--;

        /* ignore z88 bit at front */

        if(toupper(*ptr) == 'Z')
            name += offset;

        using_Z88 = TRUE;

        if(!Z88_on)
            {
            if(z88_open(com_port))
                {
                reperr_module(ERR_Z88, z88_geterr());
                using_Z88 = FALSE;

                return(NULL);
                }

            Z88_on = TRUE;
            }

        /* no tag files */
        ptr = (char *) name + strlen(name);

        #if MS
            if(toupper(*(ptr-1)) == 'L' && *(ptr-2) == DOT)
                return(NULL);
        #elif ARTHUR || RISCOS
            if(toupper(*name) == 'L' && name[1] == DOT)
                return(NULL);
        #endif

        if(z88_fopen((char *) name, (char *) atts))
            {
            reperr_module(ERR_Z88, z88_geterr());
            return(NULL);
            }

        file = (FILE *) 1;
        }
    else
        {
        #if MS
            _harderr(myhandler);
        #endif

        file = been_error ? NULL : fopen(name, atts);
        }
#endif  /* Z88_OFF */

    return(file);
}


extern void
mysetvbuf(FILE *xput, void *buffer, size_t bufsize)
{
    tracef3("[mysetvbuf(&%p, &%p, %d)]\n", xput, buffer, bufsize);

    (void) setvbuf(xput, buffer, buffer ? _IOFBF : _IONBF, bufsize);
}


/********************************
*                               *
*  close file, perhaps on z88   *
*                               *
********************************/

extern intl
myfclose(FILE *xput)
{
#if defined(Z88_OFF)
    intl res;
    char *err;

    #if RISCOS
    if(xput == &rambuffer_file)
        {
        rambuffer_file.__ptr    = BAD_POINTER;
        rambuffer_file.__icnt   = 0;
        return(0);
        }
    #endif

    clear_kernel_error();

    res = fclose(xput);

    #if RISCOS
    if((err = last_kernel_error()) != NULL)
        {
        tracef1("myfclose had kernel error %s\n", err);
        rep_fserr(err);
        return(EOF);
        }
    #endif

    return(res);

#else
    if(using_Z88)
        {
        using_Z88 = FALSE;
        return(z88_fclose());
        }
    else
        return(fclose(xput));
#endif
}



/***********************************
*                                  *
* get a char from the file or z88  *
*                                  *
***********************************/

#if defined(Z88_OFF)  &&  !RISCOS

/* Repeat definitions for consistency check */
#define     myfgetc(input)  fgetc(input)
extern int (mygetc)(FILE *input);

#else

extern int
myfgetc(FILE *input)
{
#if !defined(Z88_OFF)
    if(using_Z88)
        return(z88_getbyte());
    else
#endif
        {
        char *err;
        int res;


#if RISCOS && FALSE     /* done in a much less kosher fashion now */
        if(input == rambuffer_file)
            {
            fakeFILE *p = (fakeFILE *) rambuffer_file;
            /* derived from getc macro */
            res = (--((p)->__icnt) >= 0) ? *((p)->__ptr)++ : EOF;
            return(res);
            }
#endif

        /* no need for clear_kernel_error() here */

        res = fgetc(input);

#if RISCOS
        if((err = last_kernel_error()) != NULL)
            {
            tracef1("myfgetc had kernel error %s\n", err);
            rep_fserr(err);
            return(EOF);
            }
#endif

        return(res);
        }
}

#endif


#if RISCOS

extern int
myfread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    char *err;
    int res;

#if RISCOS && FALSE     /* done in a much less kosher fashion now */
    if(input == rambuffer_file)
        {
        fakeFILE *p = (fakeFILE *) rambuffer_file;
        /* derived from getc macro */
        res = (--((p)->__icnt) >= 0) ? *((p)->__ptr)++ : EOF;
        return(res);
        }
#endif

    /* no need for clear_kernel_error() here */

    res = fread(ptr, size, nmemb, stream);

    if((err = last_kernel_error()) != NULL)
        {
        tracef1("myfread had kernel error %s\n", err);
        rep_fserr(err);
        return(EOF);
        }

    return(res);
}

#endif


#if ARTHUR || RISCOS

extern int
myfputc(int ch, FILE *output)
{
    char buffer[256];
    char *err;
    int res;

    if(output == stdout)    /* I/O redirection never useful with PD */
        {
        os_error *os_err = bbc_vdu(ch);

        if(!os_err)
            return(ch);

        strcpy(buffer, os_err->errmess);

        err = buffer;
        }
    else
        {
        /* no need for clear_kernel_error() here */

        res = fputc(ch, output);

        err = (res == EOF) ? last_kernel_error() : NULL;
        }

    #if RISCOS
    if(err)
        {
        rep_fserr(err);
        return(EOF);
        }
    #endif

    return(res);
}

#endif


#if RISCOS

extern uword32
filelength(FILE *file)
{
    fpos_t res, pos;
    fgetpos(file, &pos);
    fseek(file, 0L, SEEK_END);
    fgetpos(file, &res);
    fsetpos(file, &pos);
#if (__CC_NORCROFT_VERSION < 536) /* guess! */
    return((uword32) res.__lo);
#else /* FIX 06.10.2021 */
    return(pos);
#endif
}

#endif  /* RISCOS */


/****************************
*                           *
* send byte to file or z88  *
*                           *
****************************/

static BOOL
go_away_byte(intl ch, FILE *output)
{
    #if !defined(Z88_OFF)
    if(using_Z88)
        {
        if(z88_putbyte(ch) < 0)
            return(reperr_module(ERR_Z88, z88_geterr()));
        }
    else
    #endif
        {
        if(myfputc(ch, output) == EOF)
            return(FALSE);
        }

    return(TRUE);
}


/************************************************************
*                                                           *
* send byte to file or z88, worrying about carriage returns *
*                                                           *
************************************************************/

extern BOOL
away_eol(FILE *output)
{
    switch(d_save[SAV_LINESEP].option)
        {
        case SAV_LSEP_LFCR:
            if(!go_away_byte(LF, output))
                return(FALSE);

            /* drop thru */

        case SAV_LSEP_CR:
            return(go_away_byte(CR, output));


        case SAV_LSEP_CRLF:
            if(!go_away_byte(CR, output))
                return(FALSE);

            /* drop thru */

    /*  case SAV_LSEP_LF:   */
        default:
            return(go_away_byte(LF, output));
        }
}


extern BOOL
away_byte(intl ch, FILE *output)
{
    return((ch == CR) ? away_eol(output) : go_away_byte(ch, output));
}


/********************************
*                               *
*  send string to z88 or file   *
*                               *
********************************/

extern BOOL
away_string(const char *str, FILE *output)
{
    intl ch;

    while((ch = *str++) != '\0')
        if(!away_byte(ch, output))
            return(FALSE);

    return(TRUE);
}


#if !defined(FILE_SELECTOR_OFF)

/* ----------------------------------------------------------------------- */

/* length of leafname displayed in box */
#define FILELENGTH 15


/*********************************************************
*                                                        *
* draw the name in the box.                              *
* directories in special colour, current name in inverse *
*                                                        *
*********************************************************/

static void
drawname(uchar *ptr, BOOL thisone)
{
    coord len = 0;
    intl fore, back;

    /* find whether ptr is file or directory */

    fore = (ptr[strlen((char *) ptr)-1] == SPACE)
                                ? MENU_HOT
                                : MENU_FORE;
    back = MENU_BACK;

    if(thisone)
        setcolour(back, fore);
    else
        setcolour(fore, back);

    if(ptr)
        len = stringout(ptr);

    ospca(FILELENGTH-len);
}


/*******************************************************
*                                                      *
* get filenames from either Z88 or specified directory *
* NULL means no file                                   *
*                                                      *
*******************************************************/

static uchar *
getfirst_in_dir(uchar *path, uchar *filespec, unsigned atts, BOOL directory)
{
#if ARTHUR || RISCOS
    IGNOREPARM(atts);
#endif

#ifndef Z88_OFF
    if(using_Z88)
        {
        char array[LIN_BUFSIZ];
        char *ptr;

        strcpy(array, (char *) filespec);

        ptr = strchr(array, '*');
        if(ptr)
            *ptr = '\0';

/* at(0,30);printf("array=%s", array);eraeol();rdch(1,1);  */
        if(strchr(array, COLON))
            return((uchar *) z88_findfirst(array, (char) (directory ? Z88_DIRS : Z88_FILES)));
        elif(directory)
            return((uchar *) z88_findfirst("*", Z88_DEVS));
        else
            return(NULL);
        }
    else
#endif
        {
        uchar *ptr;
#if MS
        unsigned newatts;

        if(_dos_findfirst(filespec, atts, &any_file))
            return(NULL);
#elif ARTHUR || RISCOS
        {
        intl offset = (*path != '\0');

        if((any_file.ptr = (uchar *) arch_findfirst((char *) path,
                        (char *) filespec+offset+strlen((char *) path))) == NULL)
            {
            char *err_str;

            if((err_str = arch_geterr()) != NULL)
                rep_fserr(err_str);
            return(NULL);
            }
        strcpy((char *) any_file.name, (char *) any_file.ptr);
        }
#endif

        ptr = any_file.name;

        /* check file received is what we asked for */
        for( ;; )
            {
            if((directory == isdirectory(path, ptr)) && !been_error)
                return(ptr);

            if(been_error)
                return(NULL);

            ptr = next_in_dir(path, directory);
            if(!ptr)
                return(NULL);
            }
        }
}


/*******************************
*                              *
* get next file from directory *
*                              *
*******************************/

static uchar *
next_in_dir(uchar *path, BOOL directory)
{
#ifndef Z88_OFF
    if(using_Z88)
        return((uchar *) z88_findnext());
    else
#endif
        for(;;)
            {
#if MS
            unsigned newatts;

            if(_dos_findnext(&any_file))
                return(NULL);
/* at(0,30);printf("next=%s", any_file.name);eraeol();rdch(1,1); */
            if(directory && !isdirectory(path, any_file.name))
                continue;
#elif ARTHUR || RISCOS
            if(been_error)
                return(NULL);
            if((any_file.ptr = (uchar *) arch_findnext()) == NULL)
                {
                char *err_str;

                if((err_str = arch_geterr()) != NULL)
                    rep_fserr(err_str);
                return(NULL);
                }
            else
                strcpy((char *) any_file.name, (char *) any_file.ptr);
            if(directory != isdirectory(path, any_file.name))
                continue;
#endif
            if(been_error)
                return(NULL);

            return(any_file.name);
            }
}


/*********************************************
*                                            *
* display the files in the current directory *
* let user scroll around and select one      *
*                                            *
*********************************************/

extern BOOL
getfilename(uchar *array)
{
    BOOL retval = TRUE;
    uchar pathname[LIN_BUFSIZ];
    uchar entryselected[LIN_BUFSIZ];
    BOOL entry_is_directory;
    uchar *path;
    uchar *filename = (array == NULL) ? UNULLSTR : array;
#ifndef Z88_OFF
    intl z88_offset;

    using_Z88 = FALSE;
#endif


    while(*filename == SPACE)
        filename++;


    /* if the name is a directory name, add *.* */

    path = filename + strlen((char *) filename) -1;
    for( ; *path == SPACE; path--)
        *path-- = '\0';


#ifndef Z88_OFF
    if((z88_offset = looks_like_z88(filename)) != 0)
        {
        /* get rid of z88 bit */

        *pathname = '\0';
        using_Z88 = TRUE;
        if(!Z88_on)
            {
            if(z88_open(com_port))
                return(reperr_module(ERR_Z88, z88_geterr()));

            Z88_on = TRUE;
            }
        }
    else
#endif
        {
        /* is it a drive, or directory */

#if MS
        if(*path == COLON || *path == BACKSLASH)
            strcat((char *) path, WILD_STR);
        elif(isdirectory(filename, NULLSTR))
            strcat((char *) path, "\\*.*");

        /* if the name supplied starts with \ or contains a colon, don't add a path */
        if(*filename == BACKSLASH || strchr((char *) filename, COLON))
#elif ARTHUR || RISCOS
        if(strchr((char *) filename, '*') == NULL && isdirectory(filename, UNULLSTR))
            strcat((char *) path, ".*");

        if(been_error)
            return(FALSE);

        if(strchr("-$&^:@%}", *filename) || strchr((char *) filename, COLON))
#endif
            {
            uchar *ptr = pathname;

            while(strchr((char *) filename, DIR_SEP) ||
#if ARTHUR || RISCOS
                        strchr((char *) filename, '-') ||
#endif
                        strchr((char *) filename, COLON))
                *ptr++ = *filename++;

#if ARTHUR || RISCOS
            /* strip off trailing dot */
            if(ptr > pathname && *(ptr-1) == DOT)
                ptr--;
#endif

            *ptr = '\0';
            }
        else
            {
            /* find the current path and remember it */

#if MS
            if(been_error || (path = getcwd(pathname, LIN_BUFSIZ-1)) == NULL)
                return(reperr_null(ERR_NOTFOUND));

            free(path);
#elif ARTHUR || RISCOS
            if(been_error)
                return(FALSE);

            if((path = (uchar *) arch_getcwd()) == NULL)
                {
                char *err_str;

                if((err_str = arch_geterr()) != NULL)
                    rep_fserr(err_str);

                return(FALSE);
                }

            strcpy((char *) pathname, (char *) path);
#endif

            /* if there is a \ in it, the filename contains abit of path */

            if(strchr((char *) filename, DIR_SEP))
                {
                uchar *ptr = pathname + strlen((char *) pathname);

                *ptr++ = DIR_SEP;
                while(strchr((char *) filename, DIR_SEP))
                    *ptr++ = *filename++;

                *ptr = '\0';
                }
#if MS
            else
                {
                if(pathname[strlen((char *) pathname)-1] != DIR_SEP)
                    strcat((char *) pathname, DIR_SEP_STR);
                }
#endif
#if ARTHUR || RISCOS
                /* strip off trailing dot if there is one */
                {
                uchar *ptr = pathname + strlen((char *) pathname);

                if(ptr > pathname  &&  *(ptr-1) == '.')
                    *(ptr-1) = '\0';
                }
#endif
            }
        }


    /* draw the current directory and let user select a file */

    for(;;)
        {
        word32 filenumber = 0;
        coord xdim = 4, ydim;
        coord currentx=0, currenty=0;
        coord yoffset = 0, lastyoffset = -1;
        coord topx, topy;
        word32 lastone = (word32) 0, thisone = (word32) 0;
        uchar thismask[LIN_BUFSIZ];
        uchar *next_file;


        this_dir = NULL;
        strcpy((char *) thismask, (char *) pathname);
#if ARTHUR || RISCOS
        if(!using_Z88 && !str_isblank(thismask) && thismask[strlen((char *) thismask)-1] != COLON)
            strcat((char *) thismask, DIR_SEP_STR);
#endif
#ifndef Z88_OFF
        if(!using_Z88)
#endif
            strcat((char *) thismask, "*");

        /* get all the directories, but not "." */

        if((next_file = getfirst_in_dir(pathname, thismask, _A_SUBDIR, TRUE)) != NULL)
            if(strcmp((char *) next_file, ONE_DOT_STR))
                {
                char array[LIN_BUFSIZ];

                strcpy(array, (char *) next_file);
                strcat(array, SPACE_STR);

                add_to_list(&this_dir, filenumber++, array);
                }

        while(!been_error)
            if((next_file = next_in_dir(pathname, TRUE)) != NULL)
                {
                char array[LIN_BUFSIZ];

                strcpy(array, (char *) next_file);
                strcat(array, SPACE_STR);

                add_to_list(&this_dir, filenumber++, array);
                }
            else
                break;

        if(been_error)
            goto breakingout;

        /* get the files matching the string */

        strcpy((char *) thismask, (char *) pathname);

#ifndef Z88_OFF
        if(!using_Z88)
#endif
            {
#if ARTHUR || RISCOS
            if(!str_isblank(thismask))
                strcat((char *) thismask, DIR_SEP_STR);
#endif
            strcat((char *) thismask, (char *) filename);
            }

        if((next_file = getfirst_in_dir(pathname, thismask, _A_NORMAL, FALSE)) != NULL)
            add_to_list(&this_dir, filenumber++, next_file);

        for(;!been_error;)
            if((next_file = next_in_dir(pathname, FALSE)) != NULL)
                add_to_list(&this_dir, filenumber++, next_file);
            else
                break;

        if(been_error)
            goto breakingout;

        /* if no files, go home */

        if(filenumber == (word32) 0)
            return(reperr(ERR_NOTFOUND, array));

        /* now we have a list of filenumber files */

        ydim = (coord) (filenumber-1) / 4 +1;
        if(ydim > 15)
            ydim = 15;

        /* work out how many lines of files */

        topx = (pagwid - 4*FILELENGTH) / 2;
        topy = (paghyt - ydim)         / 2;

        setcolour(MENU_HOT, MENU_BACK);
        save_screen(topx - 2, topy - 1, 4*FILELENGTH + 4, ydim + 2);

        my_rectangle(topx - 2, topy - 1, 4*FILELENGTH + 4, ydim + 2);
#if MS
        if(!fastdraw)
            clip_menu(topx - 2, topy - 1, 4*FILELENGTH + 4, ydim + 2);
#endif

        setcolour(MENU_HOT, MENU_BACK);
        at(topx-1, topy-1);
        wrch_funny(DROP_RIGHT);

        setcolour(MENU_FORE, MENU_BACK);
#ifndef Z88_OFF
        if(using_Z88)
            stringout((uchar *) Z88_STR);
#endif
        stringout(pathname);

        setcolour(MENU_HOT, MENU_BACK);
        wrch_funny(DROP_LEFT);


        /* draw the files perhaps and let user play with keys */

        for(;;)
            {
            uchar *ptr;
            intl c;
            LIST *listptr;

            if(yoffset != lastyoffset)
                {
                coord y;

                for(y = 0; y < ydim; y++)
                    {
                    coord x;

                    at(topx-1, topy+y);
                    setcolour(MENU_FORE, MENU_BACK);
                    wrch(SPACE);

                    for(x = 0; x < xdim; x++)
                        {
                        word32 key = (((word32) y) + ((word32) yoffset)) * 4 + ((word32) x);
                        LIST *lptr = search_list(&this_dir, key);

                        ptr = (lptr == NULL) ? NULL : lptr->value;
                        at(topx + x*FILELENGTH, topy + y);
                        drawname(ptr, (key == thisone));
                        }
                    setcolour(MENU_FORE, MENU_BACK);
                    wrch(SPACE);
                    }
                }
            else
                {
                /* draw last one */
                LIST *lptr = search_list(&this_dir, (word32) (lastone));

                ptr = (lptr == NULL) ? NULL : lptr->value;
                at(topx + ((coord) lastone % 4) * FILELENGTH,
                    topy + ((coord) lastone / 4) - lastyoffset);
                drawname(ptr, (lastone == thisone));
                }

            /* draw this one */
            listptr = search_list(&this_dir, (word32) (thisone));
            ptr = (listptr == NULL) ? NULL : listptr->value;
            at(topx + (coord) currentx*FILELENGTH, topy+(coord) currenty);
            drawname(ptr, TRUE);

            lastyoffset = yoffset;

            switch(c = rdch(FALSE, TRUE))
                {
                case CR:
                    if(thisone < filenumber)
                        {
                        LIST *lptr;
                        intl len;

                        lptr= search_list(&this_dir, thisone);
                        strcpy((char *) entryselected, (char *) lptr->value);
                        len = strlen((char *) entryselected);
                        if(entryselected[len-1] == SPACE)
                            {
                            entryselected[len-1] = '\0';
                            entry_is_directory = TRUE;
                            }
                        else
                            entry_is_directory = FALSE;

                        goto breakingout;
                        }
                    /* deliberate fall thru */

                case ESCAPE:
                    *filename = '\0';
                    retval = FALSE;
/* 2.21 */
                    ack_esc();
/* 2.21 */
                    goto breakingout;

                case UPCURSOR:
                    if(currenty > 0)
                        currenty--;
                    elif(yoffset > 0)
                        yoffset--;
                    else
                        {
                        currenty = ydim-1;
                        yoffset = (((coord) filenumber-1)/4) - currenty;
                        }
                    break;

                case DOWNCURSOR:
                    if(currenty < ydim-1)
                        currenty++;
                    elif(thisone / 4 < (filenumber-1) / 4)
                        yoffset++;
                    else
                        {
                        currenty = 0;
                        yoffset = 0;
                        }
                    break;

                case LEFTCURSOR:
                    if(currentx == 0)
                        currentx = xdim-1;
                    else
                        currentx--;
                    break;

                case RIGHTCURSOR:
                    if(currentx == xdim-1)
                        currentx = 0;
                    else
                        currentx++;
                    break;

                default:
                    bleep();
                    break;
                }
            clearkeyboardbuffer();
            lastone = thisone;
            thisone = (((word32) currenty) + yoffset) * 4 + (word32) currentx;
            }

        breakingout:

        clip_menu(0, 0, 0, 0);

        /* delete the list */

        delete_list(&this_dir);

        /* if retval is FALSE, we've had ESCAPE. */

        if(!retval || been_error)
            return(FALSE);

        /* has parent directory been chosen? */

        if(!strcmp((char *) entryselected, PREV_DIR_STR)
#if ARTHUR || RISCOS
            || (using_Z88 && !strcmp((char *) entryselected, TWO_DOT_STR))
#endif
            )
            {
#ifndef Z88_OFF
            if(using_Z88)
                {
                /* go back to previous / */

                uchar *ptr = pathname + strlen((char *) pathname) -2;

                for( ; *ptr != FORESLASH && *ptr != COLON && ptr >= pathname; ptr--)
                    *ptr = '\0';

                }
            else
#endif
                {
#if MS
                /* go back to previous \ */

                uchar *ptr = pathname + strlen((char *) pathname) -2;

                for( ; *ptr != BACKSLASH && *ptr != COLON && ptr >= pathname; ptr--)
                    *ptr = '\0';
#elif ARTHUR || RISCOS
                uchar *ptr = pathname + strlen((char *) pathname) -1;

                for( ; *ptr != DOT && ptr >= pathname; ptr--)
                    *ptr = '\0';
                if(ptr >= pathname && *ptr == DOT)
                    *ptr = '\0';
#endif
                }
            }
        else
            {
            /* maybe go down a directory */

            if(!entry_is_directory)
                {
                uchar tarray[LIN_BUFSIZ];

                /* durrun durrah, file found */

                *array = '\0';
#ifndef Z88_OFF
                if(using_Z88)
                    strcpy((char *) array, Z88_STR);
#endif
                strcat((char *) array, (char *) pathname);
#if ARTHUR || RISCOS
                if(!using_Z88 && !str_isblank(array))
                    strcat((char *) array, DIR_SEP_STR);
#endif
                strcat((char *) array, (char *) entryselected);

                /* remember directory for next time */

                strcpy((char *) tarray, (char *) pathname);
#if ARTHUR || RISCOS
                if(!using_Z88 && !str_isblank(tarray))
                    strcat((char *) tarray, DIR_SEP_STR);
#endif
#ifndef Z88_OFF
                if(using_Z88)
                    strcpy((char *) tarray, Z88COLON_STR);
                else
#endif
                    strcat((char *) tarray, WILD_STR);
                return(str_set(&currentdirectory, tarray));
                }
            else
                {
#if ARTHUR || RISCOS
                if(!using_Z88 && !str_isblank(pathname))
                    strcat((char *) pathname, DIR_SEP_STR);
#endif

                strcat((char *) pathname, (char *) entryselected);
#if MS
#ifdef Z88_OFF
                strcat((char *) pathname, DIR_SEP_STR);
#else
                strcat((char *) pathname, using_Z88 ? FORESLASH_STR : DIR_SEP_STR);
#endif
#elif ARTHUR || RISCOS
#ifndef Z88_OFF
                if(using_Z88)
                    strcat((char *) pathname, FORESLASH_STR);
#endif
#endif
                }
            }
        }
}

#endif /* FILE_SELECTOR_OFF */

/* end of slector.c */
