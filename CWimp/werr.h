/****************************************************************************
 * This source file was written by Acorn Computers Limited. It is part of   *
 * the "cwimp" library for writing applications in C for RISC OS. It may be *
 * used freely in the creation of programs for Archimedes. It should be     *
 * used with Acorn's C Compiler Release 2 or later.                         *
 *                                                                          *
 * No support can be given to programmers using this code and, while we     *
 * believe that it is correct, no correspondence can be entered into        *
 * concerning behaviour or bugs.                                            *
 *                                                                          *
 * Upgrades of this code may or may not appear, and while every effort will *
 * be made to keep such upgrades upwards compatible, no guarantees can be   *
 * given.                                                                   *
 ***************************************************************************/

/*
 * Title  : h.werr
 * Purpose: provide error reporting in wimp programs
 * Version: 0.1
*/

/* This module is used in windowed programs. Messages sent to it will appear
in a pop-up window with a button labelled "OK" which the user must press
before it returns. It involves quite a lot of mechanism working within the
program, so the module notices recursive invocation and calls Stop.Stop if
this occurs. If the error is marked as fatal, then Stop.Stop is called
after the display of the message. */


extern void werr(const char *format, ...);
extern void werr_fatal(const char *format, ...);


/* The message should be divided into at most three lines, each of 40
 * characters or less.
*/


/* end of werr.h */
