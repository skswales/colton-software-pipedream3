/* version.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* version information for PipeDream */

/* standard header files */
#include "flags.h"


/* external header */
#if ARTHUR || RISCOS
#include "ext.version"
#else
#include "version.ext"
#endif


/* local header file */
/* #include "version.h" */


/* exported variables */

extern const char applicationname[];
extern const char applicationversion[];
extern const char nameandversionstring[];


/* ----------------------------------------------------------------------- */

#define Version         "3.10a"
#define CurrentDate     "(06-Sep-22)"

#define Name            "PipeDrea3"

#if RELEASED
#   define DevVer   ""
#else
#   define DevVer   " Test"
#endif

#if RISCOS
#   define VerLet   "" /* SKS 06sep22 was R */
#elif MS || ARTHUR
#   define VerLet   ""
#endif


extern const char applicationname[]      = Name;
extern const char applicationversion[]   =          Version VerLet DevVer " " CurrentDate;
extern const char nameandversionstring[] = Name " " Version VerLet DevVer;

/*
* 11 Oct 1990 - 3.14 - the last PipeDream 3
* 14 Sep 1990 - 3.13
* 22 May 1990 - 3.11
* 03 May 1990 - 3.10 - *** on which this is based ***
* 15 Jan 1990 - 3.07
* 16 Oct 1989 - 3.06
* 22 Sep 1989 - 3.05
* 26 Jul 1989 - 3.00 - the first PipeDream 3
* 21 Jul 1989 - 3.00 (prerelease)
*
* 04 Jun 1992 - 2.21 - the last PipeDream 2
* 18 Nov 1988 - 2.2
*/

/*
 * Changes between 2.20 and 2.21
 *
 * updref does not search sheet if no slot references have existed
 * if spell not installed initialisation file will not cause auto-check
 * calls to ack_esc() refined to stop ESCAPEs being lost (on Archimedes)
 * on returning from star prompt, ctrl-O is done to clear paged mode (on Arc)
 * exec file no longer passes strings to reperr as it may contain garbage
 * function key expansions may now call exec files
 * dialog box composite fields now handled correctly in exec files/fn keys
 * lotus files > 64K can now be loaded (Archimedes only problem)
 * MODE 16 etc. menus now work correctly (on Archimedes)
 * in fixing the bug whereby printing a zero width column that had a non-zero
 * wrap width for 2.20, a bug was introduced that resulted in the printing
 * of a zero width last column caused the machine to hang. This is fixed
 * made case of 'Shift' and 'F' same throughout
*/

/* #define logostring "Dreamin of View" */  /*  Ah, how sweet*/
/* #define logostring "Dun View in" */  /* Gawd*/
/* #define logostring "Rom with a PipeDream"   Ho, ho */
/* #define logostring "What a pd" */ /* that Rob thinks this is funny*/
/* #define logostring "Black pd" */  /* does anyone get this? */
/* #define logostring "Borland chain" */   /* anything you Kahn do, I Kahn do worse */
/* #define logostring "PipeDream Professional" */
/* #define logostring "Dtype" */
/* #define logostring "Insomnia" */
/* #define logostring "Disability" */
/* #define logostring "Lent" - what you give up Excel for - Gedditt?? (Yes, Rob - Excellent) */
/* #define logostring "Quirklette" */
/* #define logostring "Flex" */
/* #define logostring "Pope John" */

/* #define logostring "PipeDream" */
/* #define logolength ((coord) (sizeof(logostring)-1)) */



/*
Unimplemented backsplash commands:

-    highlight
-    highlight block/remove highlight
-    delete word
-    insert/delete row/column
-    copy
-    previous/next word
-    split/join line
-    new
-    load options
-    save
-    first/previous/next/last file,
-    name
-    define key
-    sort
-    format paragraph
-    insert page
-    insert reference
-    decimal places, leading/trailing char,sign minus/brackets
-    \ent - exchange numeric slots for text slots
-    count
-    delete block
-    push,pop,swap

-    create file
-    save/print row selection
-    copy/move/
-    search/replace/next match
-    spool
-    print,
-    printer driver,
-    microspace

plus

-    command option special action
-    page-breaks
-    widen eval bandwidth
-    options page
-    slot references in text slots
-    word wrap
-    numeric slot display
-    lcr
-    left/right justification
?    highlights
-    bad slot references
-    absolute slot references, $A1, A$1, $A$1
-    choose, index
-    finish time functions in expression evaluator
-    floating-point exception handling

-    read, write,
-    page information for multi-file documents
-    printer driver definitions
-    lookup, vlookup, hlookup
-    ddb, irr
-    formatting @ fields
-    sort/compare to equate different slot types
x    pd *file should exec file;   looks for pd.key instead
-    pd.ini should be found anywhere on path
-    DOS prompt should print expanded prompt
-    squash error messages


issues:

-    size of rowt=32 bit, colt=intl
-    plain text mode for multi-file documents
-    commarated files,
x    DIF, SYLK

-    ignore unrecognized constructs
     highlights in numeric slots
-    snapshot operation - \ss converts formula to its current value
-    menus
-    dialog boxes
-    FAST SCREEN UPDATE
-    merican dates
-    mail merge, @0-@32000, print using file, \PP to fill in blanks
-    1-2-3 financial functions
-    split/join to use insert-on-wrap
x    character grade marking for copy,move,delete when block noexist
-    extend \W,\WW to take range of columns
x    suggested values in option page \NO (next option)
-    initialization file pd.ini, pd remembers defaults
-    read(),write(),\CF for each machine
-    function synonyms for 1-2-3, Excel, Supercalc ...
-    key redefinition for function keys
-    make install program user-friendly
-    save options for colours
-    save option for printer driver
-    merge spool and print menus
-    do highlights get switched off at eos or eol?

spike suggestions:
     append option cos insert at slot is not obvious
-    CTRL FD to be single keypress - ESCAPE
-    highlights to be left on at end of slot and repeated at beginning of lines
-    allow ctrl chars on prompt line (ARCHIE)
     unselectable menu entries
-    help

possibles
-    recover deleted slots
-    demonstration program
-    unformattable space - documentation perhaps
-    proportional spacing
-    soft hyphen

look at:
    hard page-breaks near soft breaks
-    reference updating for move, copy
-    load buffering - sensible, bomb-proof,  memory allocation

bugs:

    number of lines not integral, first page display wrong
-    draw screen when last column on screen fits exactly
-    if current column not totally on screen, cannot move right
-    createslot called on startup
-    should start (new) with cols a-f
x    where marked line redraw is optimized out, old, new and changed slots need redrawing for inverse block
-    inverse block starts on blank slot preceded by text slot, it isn't inverted (scrslt)
-    blank slot evaluation, comparison of different classes of string
    insert text on hard break line
-    ->length gets length of inverse numeric slots wrong
-    reset page offsets after option page
-    scroll up over soft break on top line of screen, adjpud
-    overtype in option page is screwy
-    insert delete must update absolute references
-    insert delete must update block markers
-    pd portf, insert line, quit  gives null pointer assignment
-    push,pop,swap don't work with multi-files
-    on archie, generate fp error with replicate, propagated results are 0
-    CTRL C handling on PC


differences to 6502:

-    date() function returns todays date
-    american dates
-    multi-file plain text mode, commarated
-    menus, dialog boxes
-    no option page
-    colours
-    \quit to exit
-    push,pop,swap
-    \ent, \bss, \psh, \pop, \i

-    insert any ASCII character
-    different format link files
-    macros
-    install
-    load file selector
-    dsum, dcount, dmax, dmin, var, dvar, std, dstd, avg, d_avg
-    '" in internal strings
-    help
-    hlookup, vlookup

Archie version

-    bigger spreadsheet size - 500,000,000 x 500,000,000 cols, rows
-    on screen italics

PC version

-    8000 columns, 500,000,000 rows
-    43 row screen (EGA), 50 row (VGA)
-    no on-screen underlining
-    dates limited to range 1.1.42->31.12.99
-    auto repeat using SHIFT


bugs in 6502:

1    right margin funnies, display of arrow when cursor moves between slots
     on screen
2    \w should call mergebuf() for \w 0 case
3    moving to cols/rows numbered less than fixed cols/rows
4    very long edit line
5    scrolled text slot, moving cursor backwards past start
6    highlights in overtype mode should be inserted
7    split line should call mergebuf at start so travel() must succeed at
     eocol
8    if joinlines fails thru combination is too long, it should call filbuf
     to clear end of buffer
9    in option page, if unfound command (eg \FP) comes after error message,
     cursor left in wrong place
10   \dk nCR should redefine key n to original definition, not null string
11   when processing a list file, ignore % constructs
12   interpretation of %constructs in plain text load
13   \l file \r 5  ==>> Colton Sfotware
14   save with plain text is a mess
15   save with row selection saves wrong slots
16   load with row selection loads at vertical offset # rows ignored at bof
17   should fault   \l file \r 2 1
18   \ip 0, move cursor onto pb, \ex|m 123 - edits number on top of pb
19   should complain about \ip rubbish etc
20   display routine screws up when conditional page break after soft break
21   when calculating lines to shift up or down, pd assumes pbs on screen
     when rows fixed
22   does not do anything sensible when loading file with > 255 columns
23   the SAVING indicator does not appear for implicit saves in multi-file
     documents
24   \IRF overtypes in overtype mode.
25   referencing a page-break displays inverse page offset
26   @a1@ where a1 is string value adds CTRL T to displayed string value
27   slot references in title strings expanded into text slots get ignored
28   a thirty character name in 40 column mode gets wrapped in option page
29   LCR throws away second or third parameters when right margin too far
     left
30   LCR gets confused if delimiter appears in contents of slot reference
31   Highlight block highlights lone highlights
32   \hb 3 on slot containing   abc3def  leaves  3abcdef3
33   line starting highlight space should break formatting
34   ROM version default-language entry on CTRL-BREAK.  Soft characters not
     defined, bad filename
35   ROM version lookup no work
36   ROM - many ways to crash connected with editing formulae. eg enter
     highlights
37   ROM - tries to format formula longer than wrap width
38   Tube version exp(n) wrong for n<.5  Appears to calculate exp(n*2).
     (Affects power)
39   Manual implies TC and LC fix slot format
40   Manual ambiguous about whether existing TCs and LCs change when
     option page changed
41   If word wrap causes the current line to move more than one line down,
     the cursor is misplaced
42   replicate doesn't check if editing expression & does some
     confusing things
43   in replicate a bad row ref gets set to 0, but col ref is random
     impossible column
44   sort should generate error message if key column is not in block
45   sort updates references for every slot swap, not every row swap. Slooow.
46   string values are not in the same class as internal strings or
     text in sort
47   \ent does not redraw whole screen so bits of long text slots stay
     on screen
48   option page error message (eg Memory full) gets cleared off on
     immediate return to text display
49   \co treats spaces at beginning of slots as a word
50   "1 words" is a bug
51   dc on last col which is also first col on screen. Cursor moves back to A
52   if(1,2) should generate "Too few arguments"
53   "" is not treated as blank slot by overlap
54   "" is not treated as blank slot by arithmetic routines, but is
     by conditionals
55   insert reference should bleep if not editing an expression
56   new sheet, enter 1234 and -1234 centred with sign brackets. Misaligned
57   changing start page to blank does not update screen display
58   setting fixed column to zero width amuses
59   if current column is only non-zero column, deleting it crashes VP
60   wild string replace converts to upper case


6502 optimizations

*    scrolling with rows fixed: define window, soft scroll
*    don't draw numbers when moving cursor around
*    save 6 bytes by routine to do CURPNM=PAGNUM (9 bytes) + 3 calls
     (total 18 bytes) rather than 3 * 8byte explicit moves


Compiler bugs:

MSC version V

*   passing NULL to realloc can result in program hanging (especially when
    running under CodeView). Contrary to ANSI standard and p482 of
    Library Reference.

*   scanf(array, "%d%n" , &decimal, &count) generates incorrect count when
    decimal is succeeded by null.  It counts the null also.

*   Compiler should (morally, if not legally) generate a warning for a
    statement such as   lvar == rvar;
    Norcroft says warning: no side effect in... which is exactly that desired
    cf: if(cond1 = cond2) ....
*/

/* end of version.c */
