/* file.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/************************************
*                                   *
* the file handling module          *
* (both stream & pathname)          *
*                                   *
* SKS                               *
* February 1990                     *
*                                   *
************************************/


/* standard include files */
#include "flags.h"


/* imports */
#include "file.h"


/* export check */
#if ARTHUR || RISCOS
#include "ext.file"
#else
#include "file.ext"
#endif


/* exported functions */

extern void    EXTERNAL file_add_cwd_to_name(char *destfilename, bytep srcfilename);
extern intl    EXTERNAL file_add_path(char *filename /*inout*/, BOOL allow_cwd);
extern intl    EXTERNAL file_buffer(file_handle f, intl bufsize);
extern intl    EXTERNAL file_close(file_handle f);
extern char *  EXTERNAL file_dirname(char *destpath, bytep srcfilename);
extern bytep   EXTERNAL file_extension(bytep filename);
extern intl    EXTERNAL file_getbyte(file_handle f);
extern intl    EXTERNAL file_flush(file_handle f);
extern BOOL    EXTERNAL file_get_cwd(char *destpath);
extern char *  EXTERNAL file_get_prefix(char *destpath, BOOL allow_cwd);
extern void    EXTERNAL file_init(char *appname, char *place);
extern intl    EXTERNAL file_isrooted(bytep filename);
extern bytep   EXTERNAL file_leafname(bytep filename);
extern word32  EXTERNAL file_length(file_handle f);
extern intl    EXTERNAL file_open(bytep filename, file_openmode openmode, file_handle f);
extern intl    EXTERNAL file_putbyte(intl c, file_handle f);
extern intl    EXTERNAL file_read(memp ptr, size_t size, size_t nmemb, file_handle f);
extern intl    EXTERNAL file_readable(bytep filename);
extern word32  EXTERNAL file_seek(file_handle f, word32 offset, intl origin);
extern char *  EXTERNAL file_separatename(char *destpath, char *destfilename, bytep srcfilename);
extern word32  EXTERNAL file_tell(file_handle f);
extern bytep   EXTERNAL file_wild(bytep filename);
extern intl    EXTERNAL file_write(memp ptr, size_t size, size_t nmemb, file_handle f);
#if RISCOS
extern char *  EXTERNAL file_get_error(void);
#endif
#if defined(Z88FS)
extern intl    EXTERNAL file_looks_like_z88(bytep name);
#endif
extern intl    EXTERNAL file_yield(void);


/* internal functions */

static intl   INTERNAL file__read(memp ptr, size_t size, size_t nmemb, file_handle f);
static intl   INTERNAL file__write(memp ptr, size_t size, size_t nmemb, file_handle f);
static intl   INTERNAL fillbuffer(file_handle f);
static intl   INTERNAL flushbuffer(file_handle f);
static char * INTERNAL get_path(void);
static char * INTERNAL make_path_element_good(char *path);
#if RISCOS
static BOOL   INTERNAL set_error(char *errorstr);
static BOOL   INTERNAL obtain_error(os_error *e);
#endif


/* ------------------------------------------------------------------------- */

/* a linked list of all open files */

static file_handle file_list = NULL;


/* error detection */

#if RISCOS

static char *errorptr         = NULL;
static char  errorbuffer[252] = "No error here kids";

static BOOL INTERNAL
set_error(char *errorstr)
{
    if(errorstr  &&  !errorptr)
        {
        errorptr = errorbuffer;
        strcpy(errorptr, errorstr);
        }

    return(errorstr != NULL);
}


static BOOL INTERNAL
obtain_error(os_error *e)
{
    return(e ? set_error(e->errmess) : FALSE);
}

#endif


#if RISCOS
/* RISC OS filenames are like fsname::discname.$.dirname.leafname */
static char path_separator[] = ".";
#elif MS || WINDOWS
/* DOS filenames are like d:\dirname\leafname.ext */
static char path_separator[] = "\\";
#endif


#if RISCOS
static char path_variable[] = "application$Path";
static char default_path[]  = "<application$Dir>.";
#elif MS
static char path_variable[] = "PATH";
static char default_path[]  = "";
#elif WINDOWS
static char path_variable[MAX_FILENAME] = "PATH";
static char default_path[]  = "";
#endif

static char * INTERNAL
get_path(void)
{
    /* getenv() always returns pointer to local heap (in default data segment) */
    char *pathptr = (char *) getenv(path_variable);

    if(!pathptr)
        pathptr = default_path;

    tracef1("get_path() yields \"%s\"\n", pathptr);
    return(pathptr);
}


static char * INTERNAL
make_path_element_good(char *path)
{
    char *ptr = path + strlen(path);

    #if RISCOS
    /* RISC OS path elements without FILE_DIR_SEP_CH termination can
     * be deliberately created by the user and should not be harmed
     * But they can also have leading and trailing (harmful) spaces
    */
    while((ptr > path)  &&  (*--ptr == ' '))
        *ptr = '\0';
    #elif MS || WINDOWS
    /* DOS PATH elements not neccesarily terminated with FILE_DIR_SEP_CH */
    if(ptr != path)
        {
        #if defined(MULTIBYTE) && WINDOWS
        ptr = (char *) AnsiPrev(path, str);
        #else
        --ptr;
        #endif
        if((*ptr != FILE_DIR_SEP_CH)  &&  (*ptr != FILE_ROOT_CH))
            {
            *++ptr = FILE_DIR_SEP_CH;
            *++ptr = '\0';
            }
        }
    #endif

    return(path);
}


/*************************************
*                                    *
* if a filename is rooted, use that  *
* otherwise add on the cwd prefix    *
*                                    *
*************************************/

extern void EXTERNAL
file_add_cwd_to_name(char *destfilename, bytep srcfilename)
{
    if(file_isrooted(srcfilename))
        strcpy(destfilename, srcfilename);
    else
        {
        (void) file_get_cwd(destfilename);
        strcat(destfilename, srcfilename);
        }
}


/******************************************************
*                                                     *
* try to find a file anywhere on the current path/cwd *
*                                                     *
* --out--                                             *
*   -ve: error                                        *
*     0: not found                                    *
*   +ve: found                                        *
*                                                     *
******************************************************/

extern intl EXTERNAL
file_add_path(char *filename /*inout*/, BOOL allow_cwd)
{
    char      array[MAX_FILENAME];
    char ALIP pathelem;
    char      strtok_array[MAX_FILENAME];
    intl      count, res;

    if(file_isrooted(filename))
        return(file_readable(filename));

    if(allow_cwd  &&  file_get_cwd(array))
        {
        if(file_isrooted(filename))
            strcpy(array, filename);
        else
            {
            count = MAX_FILENAME - strlen(array);
            strncat(array, filename, count);
            }

        if((res = file_readable(array)) > 0)
            {
            strcpy(filename, array);
            tracef1("add_path() returns filename \"%s\" & TRUE\n", filename);
            return(1);
            }
        elif(res < 0)
            return(res);
        }

    strcpy(strtok_array, get_path());

    #if WINDOWS
    /* convert from DOS path to WINDOWS/ANSI path */
    (void) OemToAnsi(strtok_array, strtok_array);
    #endif

    if(!strlen(strtok_array))
        return(0);

    pathelem = strtok(strtok_array, path_separator);

    while(pathelem)
        {
        strcpy(array, pathelem);
        make_path_element_good(array);
        strcat(array, filename);

        if((res = file_readable(array)) > 0)
            {
            strcpy(filename, array);
            tracef1("add_path() returns filename \"%s\" & TRUE\n", filename);
            return(1);
            }
        elif(res < 0)
            return(res);

        pathelem = strtok(NULL, path_separator);
        }

    tracef0("add_path() returns FALSE\n");
    return(0);                                  /* no file found */
}


extern intl EXTERNAL
file_buffer(file_handle f, intl bufsize)
{
    void *buffer = list_allocptr((word32) bufsize);

    if(!buffer)
        return(FILE_ERR_STREAMUNBUFFERED);

    f->cnt     = -1;
    f->ptr     = buffer;
    f->base    = buffer;
    f->bufsize = bufsize;

    return(0);
}


static intl INTERNAL
closefile(file_handle f)
{
    #if RISCOS
    os_regset r;
    #endif
    intl res;

    if(!(f->flags & _FILE_CLOSED))
        {
        res = file_flush(f);

        #if RISCOS
        r.r[0] = 0;
        r.r[1] = f->_handle;

        if(obtain_error(os_find(&r))  &&  (res >= 0))
            res = FILE_ERR_CANTCLOSE;
        #elif MS || WINDOWS
        errno = 0;

        if(close(f->_handle) == -1)
            res = FILE_ERR_CANTCLOSE;
        #endif

        f->flags = f->flags | _FILE_CLOSED;
        }
    else
        /* no need to do anything; already closed on fs */
        res = 0;

    return(res);
}


extern intl EXTERNAL
file_close(file_handle f)
{
    file_handle pp, cp;
    intl res;

    /* search for file and delink from list */
    pp = file_list;

    while(pp)
        {
        cp = pp->extradatap->next;
        if(cp == f)
            break;
        pp = cp;
        }

    if(!pp)
        res = FILE_ERR_BADFILE;
    elif(f->_handle)
        {
        if(pp == file_list)
            file_list = f->extradatap->next;
        else
            pp->extradatap->next = f->extradatap->next;

        res = closefile(f);

        f->_handle = 0;

        if(f->base)
            list_disposeptr(&f->base);
        }
    else
        /* nop if already closed but good */
        res = 0;

    return(res);
}


/******************************************
*                                         *
* return the directory part of a filename *
*                                         *
******************************************/

extern char * EXTERNAL
file_dirname(char *destpath, bytep srcfilename)
{
    return(file_separatename(destpath, NULL, srcfilename));
}


/******************************************
*                                         *
* return the extension part of a filename *
*                                         *
******************************************/

extern bytep EXTERNAL
file_extension(bytep filename)
{
    bytep ptr = file_leafname(filename);

    #if defined(MULTIBYTE) && WINDOWS
    char ch;

    do  {
        ch = *ptr;
        if(ch == FILE_EXT_SEP_CH)
            return(ptr);
        ptr = AnsiNext(ptr);
        }
    while(ch);
    #else
    ptr = strchr(ptr, FILE_EXT_SEP_CH);

    if(ptr)
        return(ptr+1);
    #endif

    return(NULL);
}


extern intl EXTERNAL
file_flush(file_handle f)
{
    #if RISCOS
    os_regset r;
    #endif
    intl res = flushbuffer(f);

    #if RISCOS
    r.r[0] = OSArgs_Flush;
    r.r[1] = f->_handle;

    if(obtain_error(os_args(&r))  &&  (res >= 0))
        res = FILE_ERR_CANTWRITE;
    #endif

    return(res);
}


extern intl EXTERNAL
file_getbyte(file_handle f)
{
    return(file_getc(f));
}


static bytep LAIENTRY
defaultnameproc(void)
{
    return("Untitled");
}


/* the address of a procedure to call when the current buffer's filename
 * is required to open files relative to it
*/
static file_currentnameproc currentnameproc = defaultnameproc;


extern BOOL EXTERNAL
file_get_cwd(char *destpath)
{
    bytep namep = currentnameproc();
    bytep leafp = file_leafname(namep);
    intl nchars = leafp - namep;

    *destpath = '\0';
    if(nchars)
        strncat(destpath, namep, nchars);

    tracef2("file_get_cwd() yields cwd = \"%s\", has cwd = %s\n",
            destpath, trace_boolstring(nchars));
    return(nchars != 0);
}


#if RISCOS

/**************************************************************************
*                                                                         *
* read the current error associated with this module if any               *
* clears error if set; error message only valid until next call to module *
*                                                                         *
**************************************************************************/

extern char * EXTERNAL
file_get_error(intl errornumber)
{
    char *errorstr;
    IGNOREPARM(errornumber);
    errorstr = errorptr;
    errorptr = NULL;
    return(errorstr);
}

#endif


/***********************************************************
*                                                          *
* obtain a directory prefix from cwd or first path element *
*                                                          *
***********************************************************/

extern char * EXTERNAL
file_get_prefix(char *destpath, BOOL allow_cwd)
{
    char ALIP pathelem;
    char      strtok_array[MAX_FILENAME];

    /* if allowed, use current filename prefix if there is one */
    if(!allow_cwd  ||  !file_get_cwd(destpath))
        {
        strcpy(strtok_array, get_path());

        #if WINDOWS
        /* convert from DOS path to WINDOWS/ANSI path */
        (void) OemToAnsi(strtok_array, strtok_array);
        #endif

        pathelem = strtok(strtok_array, path_separator);

        if(!pathelem)
            pathelem = default_path;

        strcpy(destpath, pathelem);
        make_path_element_good(destpath);
        }

    tracef1("get_prefix returns %s\n", destpath);

    return(destpath);
}


extern void EXTERNAL
file_init(char *appname, char *place)
{
#if RISCOS
    /* Create xxx$Path and <xxx$Dir>. */
    strcpy(path_variable, appname);
    strcat(path_variable, "$Path");

    strcpy(default_path, "<");
    strcat(default_path, appname);
    strcat(default_path, "$Dir>.");
#else
    IGNOREPARM(appname);
    IGNOREPARM(place);
#endif
}


/********************************************************
*                                                       *
* determine whether a filename is 'rooted' sufficiently *
* to open not relative to cwd or path                   *
*                                                       *
********************************************************/

extern intl EXTERNAL
file_isrooted(bytep filename)
{
    BOOL res;

    #if RISCOS
    res = (strpbrk(filename, ":$&%@\\") != NULL);
    #elif MS || WINDOWS
    bytep ptr;

    /* rooted DOS filenames are either '\dir\dir\leaf' or 'c:dir\dir\leaf'
     * ie. drive name always one a-zA-Z
    */
    if(*filename == FILE_DIR_SEP_CH)
        res = TRUE;
    else
        {
        ptr = filename;
        if(isalpha(*ptr))
            res = (*++ptr == FILE_ROOT_CH);
        else
            res = FALSE;
        }
    #endif

    tracef2("file_isrooted(%s) yields %s\n", filename, trace_boolstring(res));
    return(res);
}


/******************************************
*                                         *
* return the leafname part of a filename  *
*                                         *
******************************************/

extern bytep EXTERNAL
file_leafname(bytep filename)
{
    bytep leaf = filename + strlen(filename);   /* point to null */
    char ch;

    while(leaf > filename)
        {
        #if defined(MULTIBYTE) && WINDOWS
        leaf = (char *) AnsiPrev(filename, leaf);
        ch = *leaf;
        }
        #else
        ch = *--leaf;
        #endif
        if((ch == FILE_DIR_SEP_CH)  ||  (ch == FILE_ROOT_CH))
            /* FILE_DIR_SEP_CH & ROOT_CH are not multibyte, so AnsiNext unneccessary */
            return(leaf+1);
        }

    return(leaf);
}


/******************************
*                             *
* return the length of a file *
*                             *
******************************/

extern word32 EXTERNAL
file_length(file_handle f)
{
    #if RISCOS
    os_regset r;
    #endif
    word32 length;
    intl res;

    if((res = flushbuffer(f)) < 0)
        return(res);

    #if RISCOS
    r.r[0] = OSArgs_ReadExtent;
    r.r[1] = f->_handle;

    if(obtain_error(os_args(&r)))
        return(FILE_ERR_CANTREAD);

    length = r.r[2];
    #elif MS || WINDOWS
    errno = 0;

    length = filelength(f->_handle);

    if(length == -1)
        return(FILE_ERR_CANTREAD);
    #endif

    return(length);
}


/************************************
*                                   *
* open a file:                      *
*                                   *
* --out--                           *
*   -ve:   error in open            *
*     0:   file could not be opened *
*   +ve:   file opened              *
*                                   *
************************************/

extern intl EXTERNAL
file_open(bytep filename, file_openmode openmode, file_handle f)
{
    #if RISCOS
    os_regset r;
    static intl openatts[] = { OSFind_OpenRead,
                               OSFind_CreateUpdate,
                               OSFind_OpenUpdate };
    #elif MS
    static intl openatts[] = { O_RDONLY | O_BINARY,
                               O_RDWR   | O_BINARY | O_CREAT | O_TRUNC,
                               O_RDWR   | O_BINARY };
    #elif WINDOWS
    char szBuffer[MAX_FILENAME];
    static intl openatts[] = { OF_READ,
                               OF_CREATE,
                               OF_READWRITE };
    #endif
    #endif

    /* always initialise file to look silly */
    f->_handle = 0;
    f->cnt    = -1;             /* no buffer; forces getc/putc to fns */
    f->ptr    = NULL;
    f->base   = NULL;
    f->flags  = (openmode == file_open_read)
                            ? _FILE_READ
                            : (_FILE_READ | _FILE_WRITE);
    f->extradata = list_allocptr(sizeof(_file_extradata));
    if(!f->extradata)
        return(FILE_ERR_NOROOM);

    #if RISCOS
    r.r[0] = openatts[openmode];    
    r.r[1] = (int) filename;

    if(obtain_error(os_find(&r)))
        return(FILE_ERR_CANTOPEN);

    f->_handle = r.r[0];
    #elif MS || WINDOWS
    errno = 0;

    #if WINDOWS
    f->extradatap->openmode = openatts[openmode];
    f->_handle = OpenFile(szBuffer, &f->extradatap->of, f->extradatap->openmode);
    #else
    f->_handle = open(szBuffer, openatts[openmode], S_IREAD | S_IWRITE);
    #endif

    if(f->_handle == -1)
        {
        f->_handle = 0;

        switch(errno)
            {
            case EACCES:
                return(FILE_ERR_ACCESSDENIED);

            case EMFILE:
                return(FILE_ERR_TOOMANYFILES);
                
            case ENOENT:
                return(0);

            default:
                return(FILE_ERR_CANTOPEN);
            }
        }
    #endif

    /* link file onto head of list */
    f->extradatap->next = file_list;
    file_list = f;

    return(f->_handle);
}


/* With buffered getc/putc we too make the restriction (a la ANSI)
 * that getc and putc on the same stream are separated by a flush
 * or a seek operation (use seek(f, 0, SEEK_CUR))
*/

extern intl EXTERNAL
file_putbyte(intl c, file_handle f)
{
    return(file_putc(c, f));
}


/********************************
*                               *
* file_read much like fread()   *
* returns <0  error condition   *
*         >=0 nmemb read        *
*                               *
********************************/

static intl INTERNAL
file__read(memp ptr, size_t size, size_t nmemb, file_handle f)
{
    #if RISCOS
    os_gbpbstr blk;
    #endif
    size_t bytestoread, bytesread;

    bytestoread = size * nmemb;

    #if RISCOS
    blk.action      = OSGBPB_ReadFromPtr;
    blk.file_handle = f->_handle;
    blk.data_addr   = ptr;
    blk.number      = bytestoread;

    if(obtain_error(os_gbpb(&blk)))
        return(FILE_ERR_CANTREAD);

    bytesread = bytestoread - blk.number;
    #elif MS || WINDOWS
    errno = 0;

    bytesread = read(f->_handle, ptr, bytestoread);

    if(bytesread == -1)
        return(FILE_ERR_CANTREAD);
    #endif

    return(bytesread / size);   /* number of members read */
}


extern intl EXTERNAL
file_read(memp ptr, size_t size, size_t nmemb, file_handle f)
{
    intl res;

    if(!size  ||  !nmemb)
        return(0);

    /* limit on number of bytes to read as we're casting to intl result */
    assert(((long) size * (long) nmemb) <= INT_MAX);

    res = flushbuffer(f);

    return((res < 0) ? res : file__read(ptr, size, nmemb, f));
}


/************************
*                       *
* --out--               *
*   -ve: error          *
*     0: not readable   *
*   +ve: ok             *
*                       *
************************/

extern intl EXTERNAL
file_readable(bytep filename)
{
    intl res;
    file_handle f;

    if((res = file_open(filename, file_open_read, f)) > 0)
        if((res = file_close(f)) == 0)
            res = 1;

    return(res);
}


extern word32 EXTERNAL
file_seek(file_handle f, word32 offset, intl origin)
{
    #if RISCOS
    os_regset r;
    #endif
    intl res;
    word32 newptr;

    if((res = flushbuffer(f)) < 0)
        return(res);

    #if RISCOS
    switch(origin)
        {
        case SEEK_SET:
            newptr = 0;
            break;

        case SEEK_CUR:
            newptr = file_tell(f);
            break;

        case SEEK_END:
            newptr = file_length(f);
            break;
        }

    if(newptr >= 0)
        newptr += offset;

    if(newptr < 0)
        return(FILE_ERR_INVALIDPOSITION);

    r.r[0] = OSArgs_WritePointer;
    r.r[1] = f->_handle;
    r.r[2] = (int) newptr;

    if(obtain_error(os_args(&r)))
        return(FILE_ERR_CANTREAD);
    #elif MS || WINDOWS
    errno = 0;

    newptr = lseek(f->_handle, offset, origin);

    if(newptr == -1)
        switch(errno)
            {
            case EINVAL:
                return(FILE_ERR_INVALIDPOSITION);

            default:
                return(FILE_ERR_CANTREAD);
            }
    #endif

    return(newptr);
}


extern char * EXTERNAL
file_separatename(char *destpath, char *destfilename, bytep srcfilename)
{
    bytep leaf = file_leafname(srcfilename);
    char ch;

    if(destfilename)
        strcpy(destfilename, leaf);

    if(destpath)
        {
        *destpath = '\0';
        if(leaf != srcfilename)
            {
            #if defined(MULTIBYTE) && WINDOWS
            leaf = (bytep) AnsiPrev(srcfilename, leaf)
            ch = *leaf;
            #else
            ch = *--leaf;
            #endif
            /* normally FILE_DIR_SEP_CH but may get FILE_ROOT_CH */
            if(ch != FILE_DIR_SEP_CH)
                ++leaf;

            strncat(destpath, srcfilename, leaf - srcfilename);
            }
        }

    return(destpath);
}


extern word32 EXTERNAL
file_tell(file_handle f)
{
    #if RISCOS
    os_regset r;
    #endif
    intl res;
    word32 curptr;

    if((res = flushbuffer(f)) < 0)
        return(res);

    #if RISCOS
    r.r[0] = OSArgs_ReadPointer;
    r.r[1] = f->_handle;

    if(obtain_error(os_args(&r)))
        return(FILE_ERR_CANTREAD);

    curptr = r.r[2];
    #elif MS || WINDOWS
    errno = 0;

    curptr = tell(f->_handle);

    if(curptr == -1)
        return(FILE_ERR_CANTREAD);
    #endif

    return(curptr);
}


static intl INTERNAL
file__write(memp ptr, size_t size, size_t nmemb, file_handle f)
{
    #if RISCOS
    os_gbpbstr blk;
    #endif
    size_t bytestowrite, byteswritten;

    bytestowrite = size * nmemb;

    #if RISCOS
    blk.action      = OSGBPB_WriteAtPtr;
    blk.file_handle = f->_handle;
    blk.data_addr   = ptr;
    blk.number      = bytestowrite;

    if(obtain_error(os_gbpb(&blk)))
        return(FILE_ERR_CANTWRITE);

    byteswritten = bytestowrite - blk.number;
    #elif MS || WINDOWS
    errno = 0;

    byteswritten = write(f->_handle, ptr, bytestowrite);

    if(byteswritten == -1)
        switch(errno)
            {
            case ENOSPC:
                return(FILE_ERR_DEVICEFULL);

            default:
                return(FILE_ERR_CANTWRITE);
            }

    if(byteswritten != bytestowrite)
        return(FILE_ERR_DEVICEFULL);
    #endif

    return(nmemb);  /* number of members written */
}


/******************************************
*                                         *
* return a pointer to the first wild part *
* of a filename, if it is wild, or NULL   *
*                                         *
******************************************/

extern bytep EXTERNAL
file_wild(bytep filename)
{
    bytep ptr = filename-1;
    char ch;
    
    do  {
        #if defined(MULTIBYTE) && WINDOWS
        ptr = AnsiNext(ptr);
        ch = *ptr;
        #else
        ch = *++ptr;
        #endif
        if((ch == FILE_WILD_MULTIPLE)  ||  (ch == FILE_WILD_SINGLE))
            return(ptr);
        }
    while(ch);

    return(NULL);
}


extern intl EXTERNAL
file_write(memp ptr, size_t size, size_t nmemb, file_handle f)
{
    intl res;

    if(!size  ||  !nmemb)
        return(0);

    /* limit on number of bytes to write as we're casting to intl result */
    assert(((long) size * (long) nmemb) <= INT_MAX);

    res = flushbuffer(f);

    return((res < 0) ? res : file__write(ptr, size, nmemb, f));
}


#if defined(Z88FS)

extern intl EXTERNAL
file_looks_like_z88(bytep name)
{
    return( (toupper(*name) == 'Z')  &&
            (*(name+1) == '8')       &&
            (*(name+2) == '8')       &&
            (*(name+3) == ':')       );
}

#endif /* Z88FS */


/**************************************************
*                                                 *
* prepare to relinquish control:                  *
* Windows uses DOS so we have to close files etc  *
*                                                 *
**************************************************/

extern intl EXTERNAL
file_yield(file_handle f)
{
    intl res;
    
    if(f == NULL)
        {
        /* close all files; remember first error */
        f = file_list;
        while(f)
            {
            if(res >= 0)
                res = closefile(f);
            else
                (void) closefile(f);
            }
        }
    else
        res = closefile(f);

    return(res);
}


static intl INTERNAL
ensureopen(file_handle f)
{
    intl res;

    if(!(f->flags & _FILE_CLOSED))
        return(0);

    #if WINDOWS
    res = OpenFile((LPSTR) NULL, &f->extradatap->of, OF_PROMPT | OF_REOPEN | f->extradatap->openmode);
    #endif

    return(0);
}


/***************************************
*                                      *
* if a file is buffered then fill      *
* it from the filing system            *
*                                      *
***************************************/

static intl INTERNAL
fillbuffer(file_handle f)
{
    intl bytesread;
    
    bytesread = file__read(f->base, f->bufsize, 1, f);

    if(bytesread < 0)
        return(bytesread);

    if(bytesread == 0)
        return(EOF);

    f->cnt = bytesread;
    f->ptr = f->base;

    return(0);
}


/***************************************
*                                      *
* if a file is buffered and bytes have *
* been written to it then flush them   *
* to the filing system                 *
*                                      *
***************************************/

static intl INTERNAL
flushbuffer(file_handle f)
{
    size_t bytestowrite;
    intl res = 0;

    if(f->cnt != -1)
        {
        bytestowrite = f->bufsize - f->cnt;

        if(bytestowrite  &&  (f->flags & _FILE_BUFDIRTY))
            {
            /* write buffer to filing system */
            res = file__write(f->base, 1, bytestowrite, f);
            f->flags = f->flags & ~_FILE_BUFDIRTY;
            }

        /* zap buffer */
        f->cnt = -1;
        }

    return(res);
}


/* procedures that are exported for getc and putc macros */

/* getc failure */

extern intl EXTERNAL
file__filbuf(file_handle f)
{
    intl res;

    /* ++ as cnt got decremented to -1 */
    ++f->cnt;

    /* ensure buffered */
    if(!f->base  &&  ((res = file_buffer(f, FILE_DEFBUFSIZ)) < 0))
        return(res);

    res = ensureopen(f);

    if(res >= 0)
        res = fillbuffer(f);

    return((res < 0) ? res : file_getc(f));
}


/* putc failure */

extern intl EXTERNAL
file__flsbuf(intl c, file_handle f)
{
    intl res;

    /* ++ as cnt got decremented to -1 */
    if(++f->cnt == -1)
        {
        /* writeable file? */
        if(!(f->flags & _FILE_WRITE))
            return(FILE_ERR_ACCESSDENIED);

        /* ensure buffered */
        if(!f->base  &&  ((res = file_buffer(f, FILE_DEFBUFSIZ)) < 0))
            return(res);

        res = ensureopen(f);

        if(res >= 0)
            {
            /* buffer is empty, make it so we can write to it */
            f->cnt   = f->bufsize;
            f->ptr   = f->base;
            f->flags = f->flags | _FILE_BUFDIRTY; /* it will be when we do this putc */
            }
        }
    else
        /* file has filled output buffer, clear out and reset */
        res = flushbuffer(f);

    return((res < 0) ? res : file_putc(c, f));
}


/* end of file.c */
