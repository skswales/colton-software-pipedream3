/* spell.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/***************************************
*                                      *
* header file for the spelling checker *
* MRJC                                 *
* May 1988                             *
*                                      *
***************************************/

#if MS
// OLD_NORCROFT #pragma pack(2)
#endif

#define NAM_SIZE 100        /* maximum size of names */
#define MAX_DICT 5          /* maximum number of dictionaries */
#define BUF_SIZE 1000       /* size of buffer on stream */
#define TOKEN_OFFSET 91     /* start of ending tokens */
#define CHAR_OFFSET 32      /* start of characters allowed in dictionary */
#define END_BLOCK 0         /* token at end of block */
#define EXT_SIZE 500        /* size of disk extend */
#define MAX_TOKLEN 7        /* length of longest token */
#define MAX_TOKEN 255       /* a number higher than the max. token value */

#define KEYSTR "Integrale"
#define DICTFOFF 9
#define KEYOFF 10
#define DATAOFF (KEYOFF + 26 * 28 * sizeof(struct ixstruct))

#if ARTHUR || RISCOS

#define read_str "r"
#define update_str "r+"
#define write_str "w"

#elif MS

#define read_str "rb"
#define update_str "r+b"
#define write_str "wb"

#endif

#define SPELL_MAXITEMSIZE 12000
#define SPELL_MAXPOOLSIZE 12000
#define SPELL_MAXPOOLFREE 12000

#ifndef MS_HUGE
#define huge
#endif

typedef struct ixstruct *sixp;
typedef struct ixofdict *dixp;
typedef struct tokword *wrdp;
typedef struct endstruct *endp;
typedef struct cacheblock *cachep;

/*
cached block structure
*/

struct cacheblock
{
    word32 usecount;
    intl dict;
    intl lettix;
    word32 diskaddress;
    word32 diskspace;
    uchar data[1];
};

/*
structure of an index to a dictionary
*/

struct ixstruct
{
    union
        {
        list_itemno cacheno;
        word32 disk;
        } p;
    short blklen;
    uchar letflags;
};

/* letter flags */

#define LET_CACHED 0x80
#define LET_LOCKED 0x40
#define LET_ONE 0x20
#define LET_TWO 0x10
#define LET_WRITE 8


/*
structure of the index of dictionaries
*/

struct ixofdict
{
    FILE *  dicthandle;
    mhandle dicth;
    word32  dictsize;
    void *  dictbuf;
    uchar   dictflags;
};


/*
tokenised word structure
*/

struct tokword
{
    intl len;
    intl lettix;
    intl tail;

    char body[MAX_WORD];
    char bodyd[MAX_WORD];
    char bodydp[MAX_WORD];

    intl fail;          /* reason for failure of search */
    intl findpos;       /* offset from start of data of root */
    intl matchc;        /* characters at start of root matched */
    intl match;         /* whole root matched ? */
    intl matchcp;       /* characters at start of previous root matched */
    intl matchp;        /* whole of previous root matched ? */
};


/*
ending structure
*/

struct endstruct
{
    char *ending;
    intl len;
    intl alpha;
};

/*
insert codes
*/

#define INS_WORD 1
#define INS_TOKENCUR 2
#define INS_TOKENPREV 3
#define INS_STARTLET 4

#if MS
// OLD_NORCROFT #pragma pack()
#endif

/* end of spell.h */
