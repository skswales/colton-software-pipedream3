/* spell.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*****************************************
*                                        *
* spelling checker for PipeDream         *
* MRJC                                   *
* May 1988                               *
* March 1989 updated to use sparse lists *
* July 1989 updated for moveable indexes *
*                                        *
*****************************************/

/* standard header files */
#include "flags.h"

#if !defined(SPELL_OFF)

/* external header */
#if ARTHUR || RISCOS
#include "ext.handlist"
#include "ext.spell"
#else
#include "handlist.ext"
#include "spell.ext"
#endif

/* local header file */
#include "spell.h"

/*
function declarations
*/

intl spell_addword(intl, char *);
intl spell_checkword(intl, char *);
intl spell_close(intl);
intl spell_createdict(char *);
intl spell_deleteword(intl, char *);
intl spell_freemem(void);
#if !MS
intl spell_iswordc(char ch);
#endif
intl spell_load(intl);
intl spell_nextword(intl, char *, char *, char *, intl *);
intl spell_opendict(char *);
intl spell_pack(intl, intl);
intl spell_prevword(intl, char *, char *, char *, intl *);
intl spell_setoptions(intl, intl, intl);
void spell_stats(intl *, intl *, word32 *);
intl spell_unlock(intl);

static intl badcharsin(const char *str);
static intl chkdtable(void);
/*static intl decodeword(char *, wrdp, intl);*/
/*static void deletecache(list_itemno);*/
static intl delreins(intl, char *, wrdp);
static intl endmatch(char *, char *, intl);
static intl ensuredict(intl);
static intl fetchblock(intl, intl);
static uchar *force_buffer(FILE *stream);
static intl freecache(void);
static intl initmatch(char *, char *, char *);
static intl killcache(list_itemno);
static intl lookupword(intl, wrdp, intl);
static wrdp makeindex(wrdp, char *);
static intl matchword(char *, char *);
static intl nextword(intl, char *);
static intl prevword(intl, char *);
static void stuffcache(intl);
static void tokenise(wrdp, intl);
static intl writeblock(cachep);
static intl writeindex(intl dict, intl lettix);

/*
table of endings
*/

/* this could be on a list, and loaded
for different internationalisations */

static struct endstruct endings[] =
{
    "LESSNESS", 8, 120,
    "FULNESS",  7,  60,
    "ABILITY",  7,   1,
    "IBILITY",  7,  68,
    "LESSLY",   6, 119,
    "IOUSLY",   6,  95,
    "ISHING",   6, 103,
    "ICALLY",   6,  73,
    "ATIONS",   6,  25,
    "SOMELY",   6, 138,
    "NESSES",   6, 128,
    "IONING",   6,  92,
    "IETIES",   6,  81,
    "ISABLE",   6,  96,
    "IZABLE",   6, 113,
    "ANCES",    5,  13,
    "EABLE",    5,  31,
    "INESS",    5,  87,
    "MENTS",    5, 126,
    "INGLY",    5,  89,
    "FULLY",    5,  59,
    "ISHED",    5, 101,
    "STERS",    5, 140,
    "ERING",    5,  50,
    "ISING",    5, 104,
    "IZING",    5, 117,
    "ATION",    5,  24,
    "ATORS",    5,  27,
    "ATELY",    5,  21,
    "ENCES",    5,  40,
    "ENTLY",    5,  43,
    "ERIES",    5,  49,
    "ESSES",    5,  56,
    "IALLY",    5,  64,
    "IVELY",    5, 111,
    "OUSLY",    5, 132,
    "IFIES",    5,  84,
    "ANTLY",    5,  16,
    "ICALS",    5,  74,
    "ITIES",    5, 108,
    "IFIED",    5,  83,
    "ISHES",    5, 102,
    "SHIPS",    5, 136,
    "ATING",    5,  23,
    "URING",    5, 146,
    "ENESS",    5,  41,
    "EMENT",    5,  37,
    "ERERS",    5,  48,
    "AGING",    5,   7,
    "HOOD",     4,  62,
    "ANCE",     4,  12,
    "LIKE",     4, 121,
    "ABLE",     4,   2,
    "SHIP",     4, 135,
    "LESS",     4, 118,
    "NESS",     4, 127,
    "ERED",     4,  46,
    "MENT",     4, 125,
    "IEST",     4,  80,
    "ABLY",     4,   3,
    "IBLY",     4,  70,
    "IBLE",     4,  69,
    "EDLY",     4,  33,
    "ALLY",     4,   9,
    "IOUS",     4,  94,
    "ICAL",     4,  72,
    "FULS",     4,  61,
    "IERS",     4,  78,
    "IONS",     4,  93,
    "ISES",     4,  99,
    "IZES",     4, 116,
    "ISTS",     4, 107,
    "IANS",     4,  67,
    "ISED",     4,  98,
    "IZED",     4, 115,
    "ATOR",     4,  26,
    "AGES",     4,   6,
    "ENCE",     4,  39,
    "ANTS",     4,  17,
    "SOME",     4, 137,
    "STER",     4, 139,
    "ATED",     4,  20,
    "INGS",     4,  90,
    "YING",     4, 148,
    "AGED",     4,   5,
    "ENTS",     4,  44,
    "IALS",     4,  65,
    "URAL",     4, 142,
    "URES",     4, 145,
    "URED",     4, 144,
    "ATES",     4,  22,
    "IVES",     4, 112,
    "ERER",     4,  47,
    "IETY",     4,  82,
    "ING",      3,  88,
    "IED",      3,  76,
    "ISH",      3, 100,
    "UAL",      3, 141,
    "FUL",      3,  58,
    "DOM",      3,  29,
    "ISM",      3, 105,
    "IAN",      3,  66,
    "MAN",      3, 123,
    "MEN",      3, 124,
    "ION",      3,  91,
    "IER",      3,  77,
    "IES",      3,  79,
    "EST",      3,  57,
    "ILY",      3,  86,
    "ITY",      3, 109,
    "ERS",      3,  51,
    "ORS",      3, 130,
    "ANT",      3,  15,
    "IST",      3, 106,
    "ISE",      3,  97,
    "IZE",      3, 114,
    "ATE",      3,  19,
    "AGE",      3,   4,
    "ENT",      3,  42,
    "ARY",      3,  18,
    "EES",      3,  35,
    "ERY",      3,  52,
    "ANS",      3,  14,
    "ESE",      3,  54,
    "IFY",      3,  85,
    "ESS",      3,  55,
    "IAL",      3,  63,
    "IVE",      3, 110,
    "OUS",      3, 131,
    "URE",      3, 143,
    "ELY",      3,  36,
    "ALS",      3,  10,
    "ICS",      3,  75,
    "ED",       2,  32,
    "IC",       2,  71,
    "AL",       2,   8,
    "EN",       2,  38,
    "ER",       2,  45,
    "OR",       2, 129,
    "ES",       2,  53,
    "LY",       2, 122,
    "AN",       2,  11,
    "EE",       2,  34,
    "RY",       2, 133,
    "CY",       2,  28,
    "E",        1,  30,
    "S",        1, 134,
    "Y",        1, 147,
    "",         0,   0,
};

/*
variables
*/

static list_block *cachelp = NULL;             /* list of cached blocks */

static struct ixofdict dictlist[MAX_DICT];     /* dictionary table */

static intl spell_addword_nestf = 0;

/*
macro to get index pointer given
either dictionary number or pointer
*/

#define ixp(dict, ix) (((sixp) list_getptr((dictlist[(dict)].dicth))) + ix)
#define ixpdp(dp, ix) (((sixp) list_getptr(((dp)->dicth))) + ix)

/*
macro to determine whether character
is allowed in a word. SKS
*/

#define isatoz(ch)          (   ((ch >= 'a')  &&  (ch <= 'z'))  ||  \
                                ((ch >= 'A')  &&  (ch <= 'Z'))  )

#define iswordc(ch)         (   isatoz(ch)  ||  (ch == '\'')  ||  (ch == '-')   )

#define iswordorwildc(ch)   (   iswordc(ch)                 ||  \
                                (ch == SPELL_WILD_SINGLE)   ||  \
                                (ch == SPELL_WILD_MULTIPLE) )


/*****************************
*                            *
* add a word to a dictionary *
*                            *
* --out--                    *
* >0 word added              *
*  0 word exists             *
* <0 error                   *
*                            *
*****************************/

intl
spell_addword(intl dict, char *word)
{
    struct tokword newword;
    intl res, wordsize, err, rootlen;
    uchar *newpos, *ci, *temppos;
    cachep cp;
    list_itemp it;
    sixp lett;

    if(!makeindex(&newword, word))
        return(SPELL_ERR_BADWORD);

    /* check if word exists and get position */
    if((res = lookupword(dict, &newword, TRUE)) < 0)
        return(res);

    /* check if dictionary is read only */
    if(dictlist[dict].dictflags & DICT_READONLY)
        return(SPELL_ERR_READONLY);

    if(res)
        return(0);

    switch(newword.len)
        {
        /* single letter word */
        case 1:
            ixp(dict, newword.lettix)->letflags |= LET_ONE;
            break;

        /* two letter word */
        case 2:
            ixp(dict, newword.lettix)->letflags |= LET_TWO;
            break;

        /* all other words */
        default:
            /* tokenise the word to be inserted */
            rootlen = (newword.fail == INS_STARTLET)
                      ? max(newword.matchc, 1)
                      : max(newword.matchcp, 1);
            tokenise(&newword, rootlen);

            /* check if the root matches the current or previous words */
            rootlen = strlen(newword.body);
            if(newword.fail == INS_WORD)
                {
                if(newword.match)
                    {
                    if(rootlen == newword.matchc)
                        newword.fail = INS_TOKENCUR;
                    }
                else if(newword.matchp)
                    {
                    if(rootlen == newword.matchcp)
                        newword.fail = INS_TOKENPREV;
                    else if(!spell_addword_nestf && (rootlen > newword.matchcp))
                        {
                        uchar *pos;
                        struct tokword delword;

                        delword = newword;
                        strcpy(delword.bodyd, delword.bodydp);

                        cp = (cachep) list_gotoitem(cachelp,
                                                    ixp(dict, delword.lettix)->
                                                    p.cacheno)->i.inside;

                        pos = cp->data + delword.findpos;

                        /* skip back to start of unit */
                        while(delword.findpos && (*--pos >= TOKEN_OFFSET))
                            --delword.findpos;

                        spell_addword_nestf = 1;
                        if((res = delreins(dict, word, &delword)) < 0)
                            return(res);
                        spell_addword_nestf = 0;
                        if(res)
                            break;
                        }
                    }
                }

            /* calculate space needed for word */
            switch(newword.fail)
                {
                /* 1 byte for root count,
                   n bytes for body,
                   1 byte for token */
                case INS_STARTLET:
                    wordsize = 1 + rootlen - newword.matchc + 1;
                    break;

                case INS_WORD:
                    wordsize = 1 + rootlen - newword.matchcp + 1;
                    break;

                /* 1 byte for token */
                case INS_TOKENPREV:
                case INS_TOKENCUR:
                    wordsize = 1;
                    break;
                }

            /* check we have a cache block */
            if((err = fetchblock(dict, newword.lettix)) != 0)
                return(err);

            /* add word to cache block */
            do  {
                /* loop to get some memory */
                lett = ixp(dict, newword.lettix);

                if((it = list_createitem(cachelp,
                                         lett->p.cacheno,
                                         lett->blklen +
                                         wordsize +
                                         sizeof(struct cacheblock),
                                         FALSE)) != NULL)
                    {
                    cp = (cachep) it->i.inside;
                    break;
                    }

                if((err = freecache()) != 0)
                    return(err);
                }
            while(TRUE);

            /* find place to insert new word */
            newpos = cp->data + newword.findpos;
            switch(newword.fail)
                {
                case INS_TOKENCUR:
                    /* skip to tokens of current word */
                    while(*newpos < TOKEN_OFFSET)
                        {
                        ++newpos;
                        ++newword.findpos;
                        }
                    break;

                case INS_TOKENPREV:
                case INS_WORD:
                case INS_STARTLET:
                    break;
                }

            /* make space for new word */
            temppos = newpos;
            lett = ixp(dict, newword.lettix);
            err = lett->blklen - newword.findpos;
            memmove(newpos + wordsize,
                    newpos,
                    lett->blklen - newword.findpos);

            lett->blklen += wordsize;

            /* move in word */
            switch(newword.fail)
                {
                case INS_STARTLET:
                    *newpos++ = (uchar) 0;
                    ci = (uchar *) newword.body;
                    while(*ci)
                        *newpos++ = *ci++;
                    *newpos++ = (uchar) newword.tail;
                    *newpos++ = (uchar) newword.matchc;
                    break;

                /* note fall thru */
                case INS_WORD:
                    *newpos++ = (uchar) newword.matchcp;
                    ci = (uchar *) (newword.body + newword.matchcp);
                    while(*ci)
                        *newpos++ = *ci++;

                /* note fall thru */
                case INS_TOKENPREV:
                case INS_TOKENCUR:
                    *newpos++ = (uchar) newword.tail;
                    break;
                }

            break;
        }

    /* mark that it needs a write */
    ixp(dict, newword.lettix)->letflags |= LET_WRITE;

    return(1);
}

/*****************************************
*                                        *
* check if the word is in the dictionary *
*                                        *
*****************************************/

intl
spell_checkword(intl dict, char *word)
{
    struct tokword curword;
    intl err;

    if(!makeindex(&curword, word))
        return(SPELL_ERR_BADWORD);

    /* check if word exists and get position */
    if((err = lookupword(dict, &curword, FALSE)) < 0)
        return(err);

    return(err);
}

/*********************
*                    *
* close a dictionary *
*                    *
*********************/

intl
spell_close(intl dict)
{
    FILE *dicthand;
    intl err;

    tracef0("[spell_close]\n");

    if((dicthand = dictlist[dict].dicthandle) == 0)
        {
        trace_on();
        tracef1("[spell_close called to close non-open dictionary: %d]\n",
                dict);
        return(SPELL_ERR_CANTCLOSE);
        }

    /* write out any modified part */
    err = ensuredict(dict);

    /* make sure no cache blocks left */
    stuffcache(dict);

    /* free memory used by index */
    list_deallochandle(dictlist[dict].dicth);
    dictlist[dict].dicthandle = NULL;

    /* close file on media */
    if(fclose(dicthand) == EOF)
        err = err ? err : SPELL_ERR_CANTCLOSE;

    /* free buffer if there is one */
    list_disposeptr(&dictlist[dict].dictbuf);

    return(err);
}

/************************
*                       *
* create new dictionary *
*                       *
************************/

intl
spell_createdict(char *name)
{
    intl i, err;
    FILE *newdict;
    struct ixstruct wix;
    void *fbuf;

    /* check to see if it exists */
    if((newdict = fopen(name, read_str)) != NULL)
        {
        fclose(newdict);
        return(SPELL_ERR_EXISTS);
        }

    /* create a blank file */
    if((newdict = fopen(name, write_str)) == NULL)
        return(SPELL_ERR_CANTOPEN);

    fbuf = force_buffer(newdict);

    /* get a blank structure */
    wix.p.disk = 0;
    wix.blklen = NULL;
    wix.letflags = 0;

    /* dummy loop for efficiency */
    do  {
        err = 0;
        /* write out file identifier */
        if(fwrite(KEYSTR, sizeof(char), strlen(KEYSTR), newdict) <
                  strlen(KEYSTR))
            {
            err = 1;
            break;
            }

        /* write out dictionary flag byte */
        if(fputc(0, newdict) == EOF)
            {
            err = 1;
            break;
            }

        /* write out blank structures */
        for(i = 0; i < 26 * 28; ++i)
            {
            if((fwrite((char *) &wix,
                       sizeof(struct ixstruct),
                       1,
                       newdict)) < 1)
                {
                err = 1;
                break;
                }
            }
        }
    while(FALSE);

    if(fflush(newdict) != 0)
        err = 1;

    list_disposeptr(&fbuf);

    if(err)
        {
        fclose(newdict);
        remove(name);
        return(SPELL_ERR_CANTWRITE);
        }

    if(fclose(newdict) == EOF)
        return(SPELL_ERR_CANTCLOSE);

    return(0);
}

/**********************************
*                                 *
* delete a word from a dictionary *
*                                 *
**********************************/

intl
spell_deleteword(intl dict, char *word)
{
    struct tokword curword;
    intl res, delsize, err, tokcount, addroot, blockbefore, i;
    uchar *ci, *co, *sp, *dp, *ep, *endword;
    cachep cp;
    sixp lett;

    if(!makeindex(&curword, word))
        return(SPELL_ERR_BADWORD);

    /* check if word exists and get position */
    if((res = lookupword(dict, &curword, TRUE)) < 0)
        return(res);

    /* check if dictionary is read only */
    if(dictlist[dict].dictflags & DICT_READONLY)
        return(SPELL_ERR_READONLY);

    if(!res)
        return(SPELL_ERR_NOTFOUND);

    lett = ixp(dict, curword.lettix);
    lett->letflags |= LET_WRITE;

    /* deal with 1 and 2 letter words */
    if(curword.len == 1)
        {
        lett->letflags &= ~LET_ONE;
        return(0);
        }

    if(curword.len == 2)
        {
        lett->letflags &= ~LET_TWO;
        return(0);
        }

    /* check we have a cache block */
    if((err = fetchblock(dict, curword.lettix)) != 0)
        return(err);

    /* after succesful find, the pointer points
    at the token of the word found */
    lett = ixp(dict, curword.lettix);
    cp = (cachep) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
    sp = cp->data;

    dp = sp + curword.findpos;
    ep = sp + lett->blklen;

    /* count the tokens */
    while(*dp >= TOKEN_OFFSET)
        --dp;

    ++dp;
    tokcount = 0;
    while((*dp >= TOKEN_OFFSET) && (dp < ep))
        {
        ++dp;
        ++tokcount;
        }
    endword = dp;

    /* calculate bytes to delete */
    if(tokcount == 1)
        {
        /* move to beginning of word */
        --dp;
        while(*dp > MAX_WORD)
            --dp;

        /* last word in the block ? */
        if(endword == ep)
            {
            delsize = endword - dp;
            }
        else
            {
            if(*endword <= *dp)
                {
                delsize = endword - dp;
                }
            else
                {
                /* copy across the extra root required */
                addroot = ((intl) *endword - (intl) *dp) + 1;
                delsize = endword - dp - addroot + 1;
                ci = dp + addroot;
                co = endword + 1;
                for(i = 0; i < addroot; ++i)
                    *--co = *--ci;
                }
            }
        }
    else
        {
        delsize = 1;
        dp = sp + curword.findpos;
        }

    lett = ixp(dict, curword.lettix);
    blockbefore = (dp - sp) + delsize;
    memmove(dp, dp + delsize, lett->blklen - blockbefore);

    lett->letflags |= LET_WRITE;
    lett->blklen -= delsize;
    return(0);
}

/***********************************************
*                                              *
* free spelling checker cache memory if we can *
* least used is freed first                    *
*                                              *
* --out--                                      *
* >0 freed some memory                         *
*  0 no space reclaimed                        *
* <0 an error occured                          *
*                                              *
***********************************************/

intl
spell_freemem(void)
{
    intl err;

    if((err = freecache()) == SPELL_ERR_NOMEM)
        return(0);
    else
        return(err == 0 ? 1 : err);
}


/*************************************
*                                    *
* report whether a character is part *
* of a valid spellcheck word         *
*                                    *
*************************************/

#if !MS

intl
spell_iswordc(char ch)
{
    return(iswordc(ch) ? 1 : 0);
}

#endif

/***********************************
*                                  *
* load a dictionary                *
* all the dictionary is loaded and *
* locked into place                *
*                                  *
***********************************/

intl
spell_load(intl dict)
{
    intl err, i;
    dixp dp;

    dp = &dictlist[dict];

    for(i = 0; i < 26 * 28; ++i)
        {
        /* is there a block defined ? */
        if(!ixpdp(dp, i)->blklen)
            continue;

        if((err = fetchblock(dict, i)) != 0)
            return(err);

        ixpdp(dp, i)->letflags |= LET_LOCKED;
        }

    return(0);
}

/***************************************
*                                      *
* return the next word in a dictionary *
*                                      *
* --in--                               *
* word must point to a character       *
* buffer at least MAX_WORD long        *
*                                      *
* --out--                              *
* <0 error                             *
* =0 end of dictionary                 *
* >0 word returned                     *
*                                      *
***************************************/

intl
spell_nextword(intl dict, char *wordout, char *wordin, char *mask, intl *brkflg)
{
    char oldword[MAX_WORD + 1], lastword[MAX_WORD + 1];
    char *ci, *co;
    intl res, gotw;

    tracef5("[spell_nextword(%d, &%p, %s, %s, &%p)]\n",
            dict, wordout, trace_string(wordin), trace_string(mask), brkflg);

    /* check for start of dictionary */
    gotw = !initmatch(wordout, wordin, mask);
    res = 0;

    if(badcharsin(wordout))
        {
        tracef1("[spell_nextword yields %s, res = SPELL_ERR_BADWORD]\n", wordout);
        return(SPELL_ERR_BADWORD);
        }

    /* copy word to oldword */
    ci = wordout;
    co = oldword;
    do
        *co++ = tolower(*ci);
    while(*ci++);

    do  {
        if(*brkflg)
            {
            tracef1("[spell_nextword yields %s, res = SPELL_ERR_ESCAPE]\n", wordout);
            return(SPELL_ERR_ESCAPE);
            }

        if(!gotw)
            if((res = nextword(dict, wordout)) < 0)
                {
                tracef2("[spell_nextword yields %s, res = %d]\n", wordout, res);
                return(res);
                }

        gotw = 0;

        if(res)
            continue;

        if(*wordout == '\0')
            {
            res = 0;
            break;
            }

        do  {
            strcpy(lastword, wordout);

            if((res = prevword(dict, wordout)) < 0)
                {
                tracef2("[spell_nextword yields %s, res = %d]\n", wordout, res);
                return(res);
                }

            if(strcmp(oldword, wordout) >= 0)
                {
                res = 1;
                break;
                }
            }
        while(res);

        strcpy(wordout, lastword);
        }
    while(  res &&
            matchword(mask, wordout) &&
            ((res = !endmatch(wordout, mask, 1)) != 0) );

    /* return blank word at end */
    if(!res)
        *wordout = '\0';

    tracef2("[spell_nextword yields %s, res = %d]\n", wordout, res);
    return(res);
}

/********************
*                   *
* open a dictionary *
*                   *
* --out--           *
* dictionary handle *
*                   *
********************/

intl
spell_opendict(char *name)
{
    intl dict, err, i;
    char keystr[NAM_SIZE];
    dixp dp;
    sixp lett, tempix;

    if((dict = chkdtable()) < 0)
        return(dict);

    /* dummy loop for structure */
    do  {
        /* save dictionary parameters */
        dp = &dictlist[dict];

        dp->dicthandle = NULL;

        if((dp->dicth = list_allochandle(26 * 28 * sizeof(struct ixstruct))) == 0)
            {
            err = SPELL_ERR_NOMEM;
            break;
            }

        /* look for the file */
        if((dp->dicthandle = fopen(name, read_str)) == NULL)
            {
            err = SPELL_ERR_CANTOPEN;
            break;
            }

        dp->dictbuf = force_buffer(dp->dicthandle);

        err = SPELL_ERR_CANTREAD;

        /* read key string */
        if(fread(keystr, 1, strlen(KEYSTR), dp->dicthandle) < strlen(KEYSTR))
            break;

        if(strncmp(keystr, KEYSTR, strlen(KEYSTR)))
            {
            err = SPELL_ERR_BADDICT;
            break;
            }

        /* read dictionary flags */
        if((dp->dictflags = (uchar) fgetc(dp->dicthandle)) == (uchar) EOF)
            break;

        tempix = list_getptr(dp->dicth);
        if(fread(tempix, sizeof(struct ixstruct), 26 * 28, dp->dicthandle) < 26 * 28)
            break;

        /* read size of dictionary file */
        if(fseek(dp->dicthandle, 0L, SEEK_END))
            break;

        dp->dictsize = ftell(dp->dicthandle) - DATAOFF;

        /* if dictionary can be updated, re-open for update */
        if(!(dp->dictflags & DICT_READONLY))
            {
            fclose(dp->dicthandle);
            list_disposeptr(&dp->dictbuf);

            /* look for the file */
            if((dp->dicthandle = fopen(name, update_str)) == NULL)
                {
                err = SPELL_ERR_CANTOPEN;
                break;
                }

            dp->dictbuf = force_buffer(dp->dicthandle);
            }

        err = 0;
        }
    while(FALSE);

    if(err)
        {
        if(dp->dicthandle)
            {
            fclose(dp->dicthandle);
            dp->dicthandle = NULL;
            list_disposeptr(&dp->dictbuf);
            }
        list_disposehandle(&dp->dicth);
        return(err);
        }

    /* loop over index, masking off unwanted bits */
    for(i = 0, lett = ixpdp(dp, 0); i < 26 * 28; ++i, ++lett)
        lett->letflags = lett->letflags & (LET_ONE | LET_TWO);

    tracef1("[spell_open returns: %d]\n", dict);

    return(dict);
}

/********************
*                   *
* pack a dictionary *
*                   *
********************/

intl
spell_pack(intl olddict, intl newdict)
{
    intl err, i;
    word32 diskpoint;
    cachep cp;
    dixp olddp, newdp;

    if((err = ensuredict(olddict)) < 0)
        return(err);

    diskpoint = DATAOFF;
    err = 0;
    olddp = &dictlist[olddict];
    newdp = &dictlist[newdict];

    for(i = 0; i < 26 * 28; ++i)
        {
        sixp lettin, lettout;

        lettin  = ixpdp(olddp, i);
        lettout = ixpdp(newdp, i);

        /* if no block, copy index entries and continue */
        if(!lettin->blklen)
            {
            *lettout = *lettin;
            lettin->letflags &= LET_ONE | LET_TWO;
            lettout->letflags |= LET_WRITE;
            continue;
            }

        if((err = fetchblock(olddict, i)) != 0)
            break;

        /* re-load index pointers */
        lettin  = ixpdp(olddp, i);
        lettout = ixpdp(newdp, i);

        /* clear input index flags */
        *lettout = *lettin;
        lettin->letflags &= LET_ONE | LET_TWO;

        cp = (cachep) list_gotoitem(cachelp, lettin->p.cacheno)->i.inside;

        /* output index takes over block read from input index */
        lettin->p.disk = cp->diskaddress;
        cp->diskaddress = diskpoint;
        diskpoint += lettout->blklen + ((word32) sizeof(word32));
        cp->diskspace = lettout->blklen;
        cp->dict = newdict;
        cp->lettix = i;
        lettout->letflags |= LET_WRITE;
        }

    newdp->dictflags = olddp->dictflags;

    return(err);
}

/***************************
*                          *
* return the previous word *
*                          *
***************************/

intl
spell_prevword(intl dict, char *wordout, char *wordin, char *mask, intl *brkflg)
{
    char oldword[MAX_WORD + 1], *ci, *co;
    intl res;

    tracef5("[spell_prevword(%d, &%p, %s, %s, &%p)]\n",
            dict, wordout, trace_string(wordin), trace_string(mask), brkflg);

    initmatch(wordout, wordin, mask);

    if(badcharsin(wordout))
        {
        tracef1("[spell_prevword yields %s, res = SPELL_ERR_BADWORD]\n", wordout);
        return(SPELL_ERR_BADWORD);
        }

    ci = wordout;
    co = oldword;
    do
        *co++ = tolower(*ci);
    while(*ci++);

    do  {
        if(*brkflg)
            {
            tracef1("[spell_prevword yields %s, res = SPELL_ERR_ESCAPE]\n", wordout);
            return(SPELL_ERR_ESCAPE);
            }

        do
            if((res = prevword(dict, wordout)) <= 0)
                {
                tracef2("[spell_prevword yields %s, res = %d]\n", wordout, res);
                return(res);
                }
        while(strcmp(oldword, wordout) <= 0);
        }
    while(  res &&
            matchword(mask, wordout) &&
            ((res = !endmatch(wordout, mask, 0)) != 0) );

    /* return blank word at start */
    if(!res)
        *wordout = '\0';

    tracef2("[spell_prevword yields %s, res = %d]\n", wordout, res);
    return(res);
}

/*************************
*                        *
* set dictionary options *
*                        *
*************************/

intl
spell_setoptions(intl dict, intl optionset, intl optionmask)
{
    FILE *dicthand;

    dictlist[dict].dictflags &= (uchar) optionmask;
    dictlist[dict].dictflags |= (uchar) optionset;

    dicthand = dictlist[dict].dicthandle;

    if(fseek(dicthand, (long) DICTFOFF, SEEK_SET) != 0)
        return(SPELL_ERR_CANTWRITE);

    if(fputc(dictlist[dict].dictflags & DICT_READONLY, dicthand) == EOF)
        return(SPELL_ERR_CANTWRITE);

    return(0);
}

/*******************************************
*                                          *
* return statistics about spelling checker *
*                                          *
*******************************************/

void
spell_stats(intl *cblocks, intl *largest, word32 *totalmem)
{
    list_itemno cacheno;
    list_itemp it;
    cachep cp;
    intl blksiz;

    *cblocks = *largest = 0;
    *totalmem = 0;

    cacheno = 0;
    if((it = list_initseq(cachelp, &cacheno)) != NULL)
        {
        do  {
            cp = (cachep) it->i.inside;

            ++(*cblocks);
            blksiz = sizeof(struct cacheblock) +
                     ixp(cp->dict, cp->lettix)->blklen;
            *largest = max(*largest, blksiz);
            *totalmem += blksiz;
            }
        while((it = list_nextseq(cachelp, &cacheno)) != NULL);
        }
}

/**********************
*                     *
* unlock a dictionary *
*                     *
**********************/

intl
spell_unlock(intl dict)
{
    intl i;
    sixp lett;

    for(i = 0, lett = ixp(dict, 0);
        i < 26 * 28;
        ++i, ++lett)
        lett->letflags &= ~LET_LOCKED;

    return(0);
}

/*********************************************************
*                                                        *
* ensure string contains only chars valid for wild match *
*                                                        *
*********************************************************/

static intl
badcharsin(const char *str)
{
    char ch;

    for(;;)
        {
        ch = *str++;

        if(!ch)
            {
            tracef0("[badcharsin: FALSE]\n");
            return(FALSE);
            }

        if(!iswordorwildc(ch))
            {
            tracef4("[badcharsin: TRUE because char %c %d, isatoz %s iswordc %s]\n",
                    ch, ch, trace_boolstring(isatoz(ch)), trace_boolstring(iswordc(ch)));
            return(TRUE);
            }
        }
}

/**********************************
*                                 *
* check space in dictionary table *
*                                 *
* --out--                         *
* <0 if no space, otherwise index *
* of free entry                   *
*                                 *
**********************************/

static intl
chkdtable(void)
{
    intl i;

    for(i = 0; i < MAX_DICT; ++i)
        if(dictlist[i].dicthandle == NULL)
            return(i);

    return(SPELL_ERR_DICTFULL);
}

/***************************************
*                                      *
* compare two strings for sort routine *
*                                      *
***************************************/

static intl
compar(const void *word1, const void *word2)
{
    return(strcmp(word1, word2));
}

/*********************************************
*                                            *
* take a tokenised and indexed word from     *
* a word structure, and return the real word *
*                                            *
*********************************************/

static intl
decodeword(char *word, wrdp wp, intl len)
{
    intl ch;
    uchar *co, *ci;

    ch = wp->lettix / 28;
    *(word + 0) = (char) (ch + 'a');

    if(len == 1)
        {
        *(word + 1) = '\0';
        return(1);
        }

    ch = wp->lettix - (ch * 28);
    switch(ch)
        {
        case 0:
            ch = '\'';
            break;

        case 1:
            ch = '-';
            break;

        default:
            ch += 'a' - 2;
            break;
        }

    *(word + 1) = (char) ch;

    if(len == 2)
        {
        *(word + 2) = '\0';
        return(2);
        }

    ci = (uchar *) wp->bodyd;
    co = (uchar *) word + 2;
    while(*ci)
        {
        *co++ = (uchar) tolower(*ci);
        ++ci;
        }

    ci = (uchar *) endings[wp->tail].ending;
    do
        *co++ = tolower(*ci);
    while(*ci++);
    return(strlen(word));
}

/*******************************************
*                                          *
* delete a cache block from the list,      *
* adjusting cache numbers for the deletion *
*                                          *
*******************************************/

static void
deletecache(list_itemno cacheno)
{
    list_itemno i;
    cachep cp;

    /* remove cache block */
    tracef2("[deleting cache block: %d, %d items on list]\n",
            cacheno, list_numitem(cachelp));
    list_deleteitems(cachelp, cacheno, (list_itemno) 1);

    /* adjust cache numbers below */
    for(i = list_atitem(cachelp); i < list_numitem(cachelp); ++i)
        {
        cp = (cachep) list_gotoitem(cachelp, i)->i.inside;
        tracef2("[cp: %x, ixp: %x]\n", cp, ixp(cp->dict, cp->lettix));
        ixp(cp->dict, cp->lettix)->p.cacheno = i;

        tracef1("[deletecache adjusted: %d]\n", i);
        }
}

/*********************************************
*                                            *
* delete a word unit from the dictionary -   *
* the root and all the endings, then insert  *
* the word we were trying to insert but      *
* couldn't because it was alphabetically in  *
* the middle of the unit, then re-insert all *
* the words in the deleted unit              *
*                                            *
*********************************************/

static intl
delreins(intl dict, char *word, wrdp wp)
{
    char *deadwords[sizeof(endings)/sizeof(struct endstruct) + 1];
    char realword[MAX_WORD + 1];
    intl wordc = 0, len, i, err;
    uchar *dp, *ep;
    cachep cp, ocp;
    sixp lett;

    lett = ixp(dict, wp->lettix);
    cp = (cachep) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
    dp = ep = cp->data;

    dp += wp->findpos;
    ep += lett->blklen;

    /* extract each word */
    while((dp < ep) && *dp >= TOKEN_OFFSET)
        {
        wp->tail = *dp - TOKEN_OFFSET;
        len = decodeword(realword, wp, 0);
        if((deadwords[wordc] = list_allocptr((word32) len + 1)) == NULL)
            return(SPELL_ERR_NOMEM);
        strcpy(deadwords[wordc++], realword);
        ocp = cp;
        cp = (cachep) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
        /* SKS: list_allocptr can move core */
        dp += (bytep) cp - (bytep) ocp;
        ep += (bytep) cp - (bytep) ocp;
        ++dp;
        }

    /* check if any words are out of range */
    for(i = 0; i < wordc; ++i)
        if(strcmp(deadwords[i], word) > 0)
            break;

    /* if none out of range, free memory and exit */
    if(i == wordc)
        {
        for(i = 0; i < wordc; ++i)
            list_deallocptr(deadwords[i]);
        return(0);
        }

    /* delete the words */
    for(i = 0; i < wordc; ++i)
        if((err = spell_deleteword(dict, deadwords[i])) < 0)
            return(err);

    /* add the new word */
    if((deadwords[wordc] = list_allocptr((word32) strlen(word) + 1)) == NULL)
            return(SPELL_ERR_NOMEM);
    strcpy(deadwords[wordc++], word);

    /* and put back all the words we deleted */
    qsort((void *) deadwords, wordc, sizeof(char *), compar);
    for(i = 0; i < wordc; ++i)
        {
        if((err = spell_addword(dict, deadwords[i])) < 0)
            return(err);
        list_deallocptr(deadwords[i]);
        }

    return(1);
}

/*************************************
*                                    *
* detect the end of possible matches *
*                                    *
*************************************/

static intl
endmatch(char *word, char *mask, intl updown)
{
    intl len, res;
    char *ci;

    if(!mask || !*mask)
        return(0);

    len = 0;
    ci = mask;
    while(iswordc((intl) *ci))
        {
        ++ci;
        ++len;
        }

    if(!len)
        return(0);

    res = strncmp(mask, word, len);

    return(updown ? (res >= 0) ? 0 : 1
                  : (res <= 0) ? 0 : 1);
}

/*****************************************
*                                        *
* ensure that any modified parts of a    *
* dictionary are written out to the disk *
*                                        *
*****************************************/

static intl
ensuredict(intl dict)
{
    intl err, allerr, i;
    dixp dp;

    tracef0("[ensuredict]\n");

    /* work down the index and write out anything altered */
    for(i = 0, err = allerr = 0, dp = &dictlist[dict];
        i < 26 * 28;
        ++i)
        {
        sixp lett;

        lett = ixpdp(dp, i);

        tracef1("[ensure letter: %d]\n", i);
        if(lett->letflags & LET_WRITE)
            {
            if(lett->letflags & LET_CACHED)
                err = killcache(lett->p.cacheno);
            else
                {
                /* mask flags to be written to disk */
                lett->letflags &= LET_ONE | LET_TWO;
                err = writeindex(dict, i);
                }

            if(err)
                allerr = allerr ? allerr : err;
            else
                ixpdp(dp, i)->letflags &= ~LET_WRITE;
            }
        }

    return(allerr);
}

/**********************************
*                                 *
* fetch a block of the dictionary *
*                                 *
**********************************/

static intl
fetchblock(intl dict, intl lettix)
{
    cachep newblock;
    list_itemp it;
    FILE *dicthand;
    intl err;
    dixp dp;
    sixp lett;

    tracef3("[fetchblock dict: %d, letter: %d, cachelp: %x]\n",
            dict, lettix, cachelp);

    dp = &dictlist[dict];

    /* check if it's already cached */
    if(ixpdp(dp, lettix)->letflags & LET_CACHED)
        return(0);

    /* allocate a list block if we don't have one */
    if(!cachelp)
        {
        if((cachelp = list_allocptr((word32) sizeof(list_block))) == NULL)
            return(SPELL_ERR_NOMEM);
        list_init(cachelp,
                  SPELL_MAXITEMSIZE,
                  SPELL_MAXPOOLSIZE);
        list_register(cachelp);
        tracef0("[fetchblock has allocated cache list block]\n");
        }

    /* get a space to receive the block */
    do  {
        tracef0("[fetchblock doing createitem]\n");
        if((it = list_createitem(cachelp,
                                 list_numitem(cachelp),
                                 sizeof(struct cacheblock) +
                                    ixpdp(dp, lettix)->blklen,
                                 FALSE)) != NULL)
            break;
        tracef0("[fetchblock doing freecache]\n");
        if((err = freecache()) != 0)
            return(err);
        tracef0("[fetchblock done freecache]\n");
        }
    while(TRUE);

    newblock = (cachep) it->i.inside;
    lett = ixpdp(dp, lettix);

    /* read the data if there is any */
    if(lett->p.disk)
        {
        /* position for the read */
        tracef0("[fetchblock doing seek]\n");
        dicthand = dictlist[dict].dicthandle;
        if(fseek(dicthand, lett->p.disk, SEEK_SET) != 0)
            {
            deletecache(list_atitem(cachelp));
            return(SPELL_ERR_CANTREAD);
            }

        /* read in the block */
        tracef0("[fetchblock doing read]\n");
        if(fread(&newblock->diskspace,
                 sizeof(uchar),
                 lett->blklen + sizeof(word32),
                 dicthand) < lett->blklen + sizeof(word32))
            {
            deletecache(list_atitem(cachelp));
            return(SPELL_ERR_CANTREAD);
            }
        }
    else
        newblock->diskspace = 0;

    /* save parameters in cacheblock */
    newblock->usecount = 0;
    newblock->lettix = lettix;
    newblock->dict = dict;
    newblock->diskaddress = lett->p.disk;

    /* move index pointer */
    lett->p.cacheno = list_atitem(cachelp);
    lett->letflags |= LET_CACHED;

    return(0);
}

/***************************
*                          *
* force a buffer on a file *
*                          *
***************************/

static uchar *
force_buffer(FILE *stream)
{
    void *buf;

    buf = list_allocptr(BUF_SIZE);

    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUF_SIZE);

    return(buf);
}

/******************************
*                             *
* free least used cache block *
*                             *
******************************/

static intl
freecache(void)
{
    list_itemno cacheno, minno;
    list_itemp it;
    cachep cp;
    word32 mincount = 0x7FFFFFFF;
    intl err;

    if(!cachelp)
        return(SPELL_ERR_NOMEM);

    minno = -1;
    cacheno = 0;
    if((it = list_initseq(cachelp, &cacheno)) != NULL)
        {
        do  {
            cp = (cachep) it->i.inside;
            /* check if block is locked */
            if(!(ixp(cp->dict, cp->lettix)->letflags & LET_LOCKED))
                {
                if(cp->usecount < mincount)
                    {
                    mincount = cp->usecount;
                    minno = cacheno;
                    }
                }
            }
        while((it = list_nextseq(cachelp, &cacheno)) != NULL);
        }

    if(minno < 0)
        return(SPELL_ERR_NOMEM);

    #if TRACE
    {
    intl blocks, largest;
    word32 totalmem;
    cachep cp;

    cp = (cachep) list_gotoitem(cachelp, minno)->i.inside;
    tracef1("[spell freecache has freed a block of: %d bytes]\n",
            ixp(cp->dict, cp->lettix)->blklen);
    spell_stats(&blocks, &largest, &totalmem);
    tracef3("[spell stats: blocks: %d, largest: %d, totalmem: %ld]\n",
            blocks, largest, totalmem);
    }
    #endif

    tracef1("[freecache freeing: %d]\n", minno);

    if((err = killcache(minno)) < 0)
        return(err);

    list_unlockpools();
    list_freepoolspace(-1);

    return(0);
}

/*********************************
*                                *
* initialise variables for match *
*                                *
*********************************/

static intl
initmatch(char *wordout, char *wordin, char *mask)
{
    char *ci, *co;

    tracef3("[initmatch(wordout &%p, wordin %s, mask %s) ",
            wordout, trace_string(wordin), trace_string(mask));

    /* if no wordin, initialise from mask */
    if(!*wordin)
        {
        ci = mask;
        co = wordout;
        do  {
            if(!ci || !*ci || (*ci == SPELL_WILD_MULTIPLE) || (*ci == SPELL_WILD_SINGLE))
                {
                if(!ci || (ci == mask))
                    {
                    *co++ = 'a';
                    *co = '\0';
                    tracef1("returns 0; wordout = %s]\n", wordout);
                    return(0);
                    }
                *co = '\0';
                break;
                }
            *co++ = *ci;
            }
        while(*ci++);
        }
    else
        strcpy(wordout, wordin);

    tracef1("returns 1; wordout = %s]\n", wordout);
    return(1);
}

/*******************************************
*                                          *
* remove a block from the cache chain,     *
* writing out to the dictionary if changed *
*                                          *
*******************************************/

static intl
killcache(list_itemno cacheno)
{
    intl err = 0, write;
    cachep cp;
    FILE *dicthand;
    sixp lett;

    cp = (cachep) list_gotoitem(cachelp, cacheno)->i.inside;

    tracef1("[killcache cacheno: %d]\n", cacheno);

    /* write out block if altered */
    if((write = (ixp(cp->dict, cp->lettix)->letflags & LET_WRITE)) != 0)
        if((err = writeblock(cp)) != 0)
            return(err);

    /* clear flags in index */
    lett = ixp(cp->dict, cp->lettix);
    lett->letflags &= LET_ONE | LET_TWO;

    /* save disk address in index */
    lett->p.disk = cp->diskaddress;

    /* write out index entry */
    dicthand = dictlist[cp->dict].dicthandle;
    if(write && (err = writeindex(cp->dict, cp->lettix)) != 0)
        {
        /* make the dictionary useless
        if not properly updated */
        tracef1("[write of index block: %d failed]\n", cp->lettix);
        clearerr(dicthand);
        fseek(dicthand, 0l, SEEK_SET);
        fputc(0, dicthand);
        }

    /* throw away the cache block */
    deletecache(cacheno);

    /* flush buffer */
    tracef0("[flushing buffer]\n");
    if(fflush(dicthand))
        err = err ? err : SPELL_ERR_CANTWRITE;

    tracef0("[buffer flushed]\n");
    return(err);
}

/***********************************
*                                  *
* look up a word in the dictionary *
*                                  *
* --out--                          *
* <0 error                         *
*  0 not found                     *
* >0 found                         *
*                                  *
***********************************/

static intl
lookupword(intl dict, wrdp wp, intl needpos)
{
    uchar *datap, *co;
    intl i;
    uchar *startlim, *endlim, *ci, *midpoint;
    intl ch, found, bodylen, updown, err;
    cachep cp;
    sixp lett;

    if(needpos)
        {
        wp->fail = INS_WORD;
        wp->findpos = wp->matchc = wp->match = wp->matchcp = wp->matchp = 0;
        }

    lett = ixp(dict, wp->lettix);

    /* check one/two letter words */
    switch(wp->len)
        {
        case 1:
            return(lett->letflags & LET_ONE);

        case 2:
            return(lett->letflags & LET_TWO);

        default:
            break;
        }

    /* is there a block defined ? */
    if(!lett->blklen)
        return(0);

    /* fetch the block if not in memory */
    if((err = fetchblock(dict, wp->lettix)) != 0)
        return(err);

    /* search the block for the word */
    lett = ixp(dict, wp->lettix);
    cp = (cachep) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
    ++cp->usecount;

    /*
    do a binary search on the third letter
    */

    startlim = cp->data;
    endlim = cp->data + lett->blklen;
    found = updown = 0;

    while(endlim - startlim > 1)
        {
        midpoint = datap = startlim + (endlim - startlim) / 2;

        /* step back to first in block */
        while(*datap)
            --datap;

        ch = (intl) *(datap + 1);
        if(ch == (intl) *wp->body)
            {
            found = 1;
            break;
            }

        if(ch < (intl) *wp->body)
            {
            updown = 1;
            startlim = midpoint;
            }
        else
            {
            updown = 0;
            endlim = midpoint;
            }
        }

    if(!found)
        {
        /* set insert position after this letter
        if we are inserting a higher letter */
        if(needpos)
            {
            if(updown)
                {
                endlim = cp->data + lett->blklen;
                while(datap < endlim)
                    {
                    ++datap;
                    if(!*datap)
                        break;
                    }
                }

            wp->fail = INS_WORD;
            wp->findpos = datap - cp->data;
            }
        return(0);
        }

    /* search forward for word */
    endlim = cp->data + lett->blklen;

    *wp->bodyd = '\0';
    while((datap + 1) < endlim)
        {
        /* save previous body */
        if(needpos)
            strcpy(wp->bodydp, wp->bodyd);

        /* take prefix count */
        co = (uchar *) (wp->bodyd + (intl) *datap++);

        /* build body */
        while(*datap < TOKEN_OFFSET)
            *co++ = *datap++;

        /* mark end of body */
        bodylen = co - (uchar *) wp->bodyd;
        *co = '\0';

        /* compare bodies and stop search if
        we are past the place */
        for(i = 0, ci = (uchar *) wp->bodyd, co = (uchar *) wp->body, found = 0;
            i < bodylen;
            ++i, ++ci, ++co)
            {
            if(*ci != *co)
                {
                if(*ci > *co)
                    found = 1;
                else
                    found = -1;

                break;
                }
            }

        if(needpos)
            {
            wp->matchcp = wp->matchc;
            wp->matchp = wp->match;
            wp->matchc = i;
            wp->match = (i == bodylen && wp->matchc >= wp->matchcp) ? 1 : 0;
            }

        if(!found)
            {
            /* compare tokens */
            while(((ch = (intl) *datap) >= TOKEN_OFFSET) && (datap < endlim))
                {
                ++datap;
                if(!strcmp(endings[ch - TOKEN_OFFSET].ending, wp->body + bodylen))
                    {
                    if(needpos)
                        {
                        wp->fail = 0;
                        wp->findpos = datap - cp->data - 1;
                        }
                    return(1);
                    }
                }
            }

        /* if bodies didn't compare */
        if(found || (needpos && (wp->matchc < wp->matchcp)))
            {
            if(found >= 0)
                {
                /* step back to start of word */
                if(needpos)
                    {
                    while(*--datap > MAX_WORD)
                        ;

                    if(*datap)
                        wp->fail = INS_WORD;
                    else
                        wp->fail = (wp->matchc == 0) ? INS_WORD : INS_STARTLET;

                    wp->findpos = datap - cp->data;
                    }
                return(0);
                }
            else
                {
                /* skip tokens, then move to next body */
                while((*datap >= TOKEN_OFFSET) && (datap < endlim))
                    ++datap;

                continue;
                }
            }

        }

    /* at end of block */
    if(needpos)
        {
        wp->matchcp = wp->matchc;
        wp->matchp = wp->match;
        wp->matchc = wp->match = 0;
        strcpy(wp->bodydp, wp->bodyd);

        wp->fail = INS_WORD;
        wp->findpos = datap - cp->data;
        }

    return(0);
}

/********************************
*                               *
* set up index for a word       *
* this routine tries to be fast *
*                               *
* --in--                        *
* pointer to a word block       *
*                               *
* --out--                       *
* index set in block,           *
* pointer to block returned     *
*                               *
********************************/

static wrdp
makeindex(wrdp wp, char *word)
{
    intl ch, len;
    char *co;

    len = strlen(word);
    if((len == 0)  ||  (len > MAX_WORD))
        return(NULL);

    /* process first letter */
    wp->len = len;
    ch = (intl) *word++;
    if(!isatoz(ch))
        return(NULL);

    wp->lettix = (toupper(ch) - 'A') * 28;

    /* process second letter */
    if(len > 1)
        {
        ch = (intl) *word++;
        if(!isatoz(ch))
            {
            switch(ch)
                {
                case '\'':
                    wp->lettix += 0;
                    break;

                case '-':
                    wp->lettix += 1;
                    break;

                /* duff characters */
                default:
                    return(NULL);
                }
            }
        else
            {
            wp->lettix += (toupper(ch) - 'A') + 2;
            }
        }

    /* copy across body */
    co = (uchar *) wp->body;
    do  {
        ch = (intl) *word++;

        if(ch  &&  !iswordc(ch))    /* SKS */
            return(NULL);

        *co++ = (uchar) toupper(ch);
        }
    while(ch);

    /* clear out tail index */
    wp->tail = 0;

    return(wp);
}

/******************************************
*                                         *
* match two words with possible wildcards *
* word1 must contain any wildcards        *
*                                         *
* --out--                                 *
* -1 word1 < word2                        *
*  0 word1 = word2                        *
* +1 word1 > word2                        *
*                                         *
******************************************/

static intl
matchword(char *mask, char *word)
{
    char *maskp, *wordp, *star, *nextpos;

    if(!mask || (*mask == '\0'))
        return(0);

    maskp = star = mask;
    wordp = nextpos = word;

    do  {
        /* loop1 */
        ++nextpos;

        /* loop3 */
        do  {
            if(*maskp == SPELL_WILD_MULTIPLE)
                {
                nextpos = wordp;
                star = ++maskp;
                break;
                }

            if(tolower(*maskp) != tolower(*wordp))
                {
                if(*wordp == '\0')
                    return(1);

                if(*maskp != SPELL_WILD_SINGLE)
                    {
                    ++maskp;
                    wordp = nextpos;
                    if(star != mask)
                        {
                        maskp = star;
                        break;
                        }
                    else
                        {
                        return(tolower(*maskp) < tolower(*wordp) ? -1 : 1);
                        }
                    }
                }

            if(*maskp++ == '\0')
                return(0);
            ++wordp;
            }
        while(TRUE);
        }
    while(TRUE);
}

/***************************************
*                                      *
* return the next word in a dictionary *
*                                      *
* --out--                              *
* >0 supplied word found, therefore    *
* result is correct                    *
* =0 supplied word not found, thus     *
* step back to ensure we are at the    *
* correct point                        *
*                                      *
***************************************/

static intl
nextword(intl dict, char *word)
{
    struct tokword curword;
    intl err, tokabval, nexthigher, token, curabval, tail, res;
    uchar *dp, *co, *ep;
    cachep cp;
    sixp lett;

    if(!makeindex(&curword, word))
        return(SPELL_ERR_BADWORD);

    /* check if word exists and get position */
    if((res = lookupword(dict, &curword, TRUE)) < 0)
        return(res);

    if((curword.len == 1) && (ixp(dict, curword.lettix)->letflags & LET_TWO))
        {
        decodeword(word, &curword, 2);
        return(res);
        }

    do  {
        if(ixp(dict, curword.lettix)->blklen)
            {
            /* check we have a cache block */
            if((err = fetchblock(dict, curword.lettix)) != 0)
                return(err);

            lett = ixp(dict, curword.lettix);
            cp = (cachep) list_gotoitem(cachelp,
                                        lett->p.cacheno)->i.inside;
            dp = cp->data + curword.findpos;
            ep = cp->data + lett->blklen;

            /* build a body at the start of a block */
            if(dp < ep)
                {
                if(*dp == '\0')
                    {
                    /* skip null starter */
                    ++dp;
                    co = (uchar *) curword.bodyd;
                    while(*dp < TOKEN_OFFSET)
                        *co++ = *dp++;
                    *co++ = '\0';
                    }

                tokenise(&curword, max(curword.matchc, 1));
                if(!curword.match)
                    curabval = -1;
                else
                    curabval = endings[curword.tail - TOKEN_OFFSET].alpha;

                /* ensure we are on the tokens */
                while(*dp >= TOKEN_OFFSET)
                    --dp;

                while((*dp < TOKEN_OFFSET) && (dp < ep))
                    ++dp;
                }

            while(dp < ep)
                {
                /* find the next higher token */
                tail = -1;
                nexthigher = MAX_TOKEN + 1;
                while(((token = *dp) >= TOKEN_OFFSET) && (dp < ep))
                    {
                    ++dp;
                    tokabval = endings[token - TOKEN_OFFSET].alpha;
                    if(tokabval > curabval)
                        {
                        if(tokabval < nexthigher)
                            {
                            tail = token - TOKEN_OFFSET;
                            nexthigher = tokabval;
                            }
                        }
                    }

                if((tail >= 0) && (nexthigher > curabval))
                    {
                    /* work out the real word from the decoded stuff */
                    curword.tail = tail;
                    decodeword(word, &curword, 0);
                    return(res);
                    }

                /* if there were no tokens higher, go onto next root */
                if(++dp >= ep)
                    break;

                co = (uchar *) (curword.bodyd + token);

                while((*dp < TOKEN_OFFSET) && (dp < ep))
                    *co++ = *dp++;

                *co = '\0';
                curabval = -1;
                }
            }

        /* move onto the next letter */
        *curword.bodyd = *curword.bodydp = '\0';
        curword.tail = curword.matchc =
        curword.match = curword.findpos = 0;

        /* skip down the index till we find
        an entry with some words in it */
        if(++curword.lettix < 26 * 28)
            {
            lett = ixp(dict, curword.lettix);
            if(lett->letflags & LET_ONE)
                {
                decodeword(word, &curword, 1);
                return(res);
                }
            if(lett->letflags & LET_TWO)
                {
                decodeword(word, &curword, 2);
                return(res);
                }
            }
        }
    while(curword.lettix < 26 * 28);

    *word = '\0';
    return(0);
}

/***************************
*                          *
* return the previous word *
*                          *
***************************/

static intl
prevword(intl dict, char *word)
{
    struct tokword curword;
    intl err, tokabval, nextlower, token, curabval, tail, i, onroot;
    uchar *dp, *co, *sp, *ep;
    cachep cp;
    sixp lett;

    if(!makeindex(&curword, word))
        return(SPELL_ERR_BADWORD);

    /* check if word exists and get position */
    if((err = lookupword(dict, &curword, TRUE)) < 0)
        return(err);

    if((curword.len == 2) && (ixp(dict, curword.lettix)->letflags & LET_ONE))
        return(decodeword(word, &curword, 1));

    do  {
        if(ixp(dict, curword.lettix)->blklen)
            {
            /* check we have a cache block */
            if((err = fetchblock(dict, curword.lettix)) != 0)
                return(err);

            lett = ixp(dict, curword.lettix);
            cp = (cachep) list_gotoitem(cachelp,
                                        lett->p.cacheno)->i.inside;
            sp = cp->data;

            dp = sp + curword.findpos;
            ep = sp + lett->blklen;

            onroot = 0;
            if((dp > sp) && (curword.matchc || curword.matchcp))
                {
                /* if we didn't match this root,
                start from the one before */
                if(!curword.match)
                    {
                    if(dp != ep)
                        while(*dp > MAX_WORD)
                            --dp;
                    if(dp > sp)
                        {
                        --dp;
                        curword.matchc = curword.matchcp;
                        curword.match = curword.matchcp = 0;
                        strcpy(curword.bodyd, curword.bodydp);
                        }
                    }

                tokenise(&curword, max(curword.matchc, 1));
                if(!curword.match)
                    curabval = MAX_TOKEN + 1;
                else
                    curabval = endings[curword.tail - TOKEN_OFFSET].alpha;

                /* move to the start of the root */
                while(*dp > MAX_WORD)
                    --dp;

                /* build a body at the start of a block */
                if(*dp == '\0')
                    {
                    /* skip null starter */
                    ++dp;
                    co = (uchar *) curword.bodyd;
                    while(*dp < TOKEN_OFFSET)
                        *co++ = *dp++;
                    *co++ = '\0';
                    curword.matchcp = 0;
                    }

                /* move on to the tokens */
                while((*dp < TOKEN_OFFSET) && (dp < ep))
                    ++dp;

                onroot = 1;
                }

            /* move back down the block */
            while((dp > sp) && onroot)
                {
                /* find the next lower token */
                tail = nextlower = -1;
                while(((token = *dp) >= TOKEN_OFFSET) && (dp < ep))
                    {
                    ++dp;
                    tokabval = endings[token - TOKEN_OFFSET].alpha;
                    if(tokabval < curabval)
                        {
                        if(tokabval > nextlower)
                            {
                            tail = token - TOKEN_OFFSET;
                            nextlower = tokabval;
                            }
                        }
                    }

                /* did we find a suitable token ? */
                if((tail >= 0) && (nextlower < curabval))
                    {
                    /* work out the real word from the decoded stuff */
                    curword.tail = tail;
                    return(decodeword(word, &curword, 0));
                    }

                /* if there were no tokens lower,
                go onto previous root */
                --dp;
                while(*dp > MAX_WORD)
                    --dp;

                /* check for beginning of block,
                or no previous root */
                if((dp == sp) || (!curword.matchcp))
                    break;
                --dp;
                while(*dp >= TOKEN_OFFSET)
                    --dp;
                ++dp;

                curword.match = 0;
                strcpy(curword.bodyd, curword.bodydp);
                curabval = MAX_TOKEN + 1;
                }
            }

        lett = ixp(dict, curword.lettix);
        if(dp == sp || !lett->blklen)
            {
            /* if we are at the start of the block */
            do  {
                /* return the small words if there are some */
                if((curword.len > 2) && (lett->letflags & LET_TWO))
                    return(decodeword(word, &curword, 2));
                if((curword.len > 1) && (lett->letflags & LET_ONE))
                    return(decodeword(word, &curword, 1));

                if(--curword.lettix < 0)
                    break;

                lett = ixp(dict, curword.lettix);
                }
            while(!lett->blklen);

            /* quit if at beginning */
            if(curword.lettix < 0)
                break;

            /* build previous letter ready for search */
            decodeword(word, &curword, 2);
            for(i = 2, co = (uchar *) (word + 2); i < MAX_WORD; ++i)
                *co++ = 'Z';
            *co = '\0';

            /* set position to end of block */
            makeindex(&curword, word);
            }
        else
            {
            /* if we are at the start of
            a letter in a block */
            co = (uchar *) (word + 2);
            switch(*co)
                {
                case '-':
                    *co++ = '\'';
                    break;
                case 'a':
                    *co++ = '-';
                    break;
                default:
                    --(*co++);
                    break;
                }
            for(i = 3; i < MAX_WORD; ++i)
                *co++ = 'Z';
            *co = '\0';

            makeindex(&curword, word);
            }

        if((err = lookupword(dict, &curword, TRUE)) < 0)
            return(err);
        }
    while(curword.lettix >= 0);

    *word = '\0';
    return(0);
}

/*************************************
*                                    *
* make sure that there are no cache  *
* blocks left for a given dictionary *
*                                    *
*************************************/

static void
stuffcache(intl dict)
{
    list_itemno i;
    cachep cp;

    tracef0("[stuffcache]\n");

    /* write out/remove any blocks
    from this dictionary */
    for(i = 0; i < list_numitem(cachelp); ++i)
        {
        cp = (cachep) list_gotoitem(cachelp, i)->i.inside;

        if(cp->dict == dict)
            {
            deletecache(i);
            --i;
            }
        }
}

/********************************
*                               *
* tokenise a word               *
* this routine tries to be fast *
*                               *
* --in--                        *
* pointer to a word block       *
*                               *
* --out--                       *
* tokenised word in block,      *
* pointer to block returned     *
*                               *
********************************/

static void
tokenise(wrdp wp, intl rootlen)
{
    uchar *endbody;
    intl i, maxtail;
    endp curend;

    /* calculate maximum ending length */
    if(wp->len > rootlen + 2)
        maxtail = wp->len - rootlen - 2;
    else
        maxtail = 0;

    /* find a suitable ending */
    endbody = (uchar *) (wp->body + wp->len - 2);
    for(i = 0, curend = endings;
        i < sizeof(endings)/sizeof(struct endstruct);
        ++i, ++curend)
        {
        if(maxtail < curend->len)
            continue;

        if(!strcmp(curend->ending, (char *) endbody - curend->len))
            {
            wp->tail = i + TOKEN_OFFSET;
            *(endbody - curend->len) = '\0';
            break;
            }
        }
}

/*********************************************
*                                            *
* write out an altered block to a dictionary *
*                                            *
*********************************************/

static intl
writeblock(cachep cp)
{
    FILE *dicthand;
    dixp dp;
    sixp lett;

    tracef2("[writeblock dict: %d, letter: %d]\n", cp->dict, cp->lettix);

    lett = ixp(cp->dict, cp->lettix);

    if(lett->blklen)
        {
        dp = &dictlist[cp->dict];
        dicthand = dp->dicthandle;

        /* do we need to extend file ? */
        if((word32) lett->blklen > cp->diskspace)
            {
            if(fseek(dicthand,
                     DATAOFF + dp->dictsize + lett->blklen + EXT_SIZE,
                     SEEK_SET))
                return(SPELL_ERR_CANTENLARGE);

            if(fputc(0, dicthand) == EOF)
                return(SPELL_ERR_CANTENLARGE);

            cp->diskaddress = DATAOFF + dp->dictsize;
            dp->dictsize += lett->blklen + ((word32) EXT_SIZE);
            cp->diskspace += EXT_SIZE;
            }

        /* write out block */
        if(fseek(dicthand, cp->diskaddress, SEEK_SET))
            return(SPELL_ERR_CANTWRITE);

        if(fwrite(&cp->diskspace,
                  lett->blklen + sizeof(word32),
                  1,
                  dicthand) < 1)
            return(SPELL_ERR_CANTWRITE);
        }

    lett->letflags &= ~LET_WRITE;

    return(0);
}

/*****************************************
*                                        *
* write out the index entry for a letter *
*                                        *
*****************************************/

static intl
writeindex(intl dict, intl lettix)
{
    dixp dp;

    dp = &dictlist[dict];

    /* update index entry for letter */
    if(fseek(dp->dicthandle,
             (long) (KEYOFF + sizeof(struct ixstruct) * lettix),
             SEEK_SET))
        return(SPELL_ERR_CANTWRITE);

    if(fwrite(ixpdp(dp, lettix),
              sizeof(struct ixstruct),
              1,
              dp->dicthandle) < 1)
        return(SPELL_ERR_CANTWRITE);

    return(0);
}

#endif

/* end of spell.c */
