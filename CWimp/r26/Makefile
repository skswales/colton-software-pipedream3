# Makefile for CWimp (26-bit)

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright (C) 1989-1998 Colton Software Limited
# Copyright (C) 1998-2015 R W Colton

# Created by Tutu's magic CDep V0.19 at Tue,10 Jan 1989.15:48:00

PD3DEVROOT=^.^
CMODULES=$(PD3DEVROOT).CModules

CC=cc
# -DOWN_MEMCPY is useful for ancient SharedCLibary without optimised memcpy/memmove
# options for release
# Circa C Release 5 (e.g. CC 5.09 in RPCEmu Easy Start Bundle 3.71)
CCOPTIONS=-c -i^,CModules:,C: -fafh -DSMALL -throwback -apcs 3/26  -arch=2
CCOPTIONS=-c -i^,CModules:,C: -fah  -DSMALL -throwback -apcs 3/26  -arch=2

# C Release 4.05 (with kernel.h from 4.01, other headers are builtin)
CC=v405.cc
CCOPTIONS=-c -i^,$(CMODULES),^.^.Library26.v401 -fafh -DSMALL -throwback  -arch=2

# C Release 3.1B
CC=v31B.cc
CCOPTIONS=-c -i^,$(CMODULES),^.^.Library26.v31B -fafh -DSMALL -throwback  -arch=2

# options for debugging
# CCOPTIONS=...  -DWATCH_ALLOCS

# options for release
TRACE=
# options for debugging
# TRACE=-DTRACE

AS=objasm
# Circa C Release 5 (e.g. ObjAsm 3.06 in RPCEmu Easy Start Bundle 3.71)
ASOPTIONS=-apcs 3/26 -throwback -o $@

# C Release 4.01 (ObjAsm 2.00) is also too archaic...
# AS=v401.objasm
# ASOPTIONS=-stamp -quit -throwback -To $@ -From

# C Release 3.1B (ObjAsm 1.70) is just too archaic...
# AS=v31B.objasm
# ASOPTIONS=-stamp -quit -To $@ -From


LIBFILE=v401.libfile


all:            WimpLib

clean:
                Remove WimpLib
                |
                Remove o.akbd
                Remove o.bbc
                Remove o.baricon
                Remove o.cs-wimptx
                Remove o.dbox
                Remove o.event
                Remove o.fileicon
                Remove o.flex
                Remove o.font
                Remove o.menu
                Remove o.misc
                Remove o.os
                Remove o.print
                Remove o.poll
                Remove o.res
                Remove o.resspr
                Remove o.sprite
                Remove o.swi
                Remove o.trace
                Remove o.tralloc
                Remove o.werr
                Remove o.wimp
                Remove o.wimpt
                Remove o.win
                Remove o.visdelay
                Remove o.xfersend
                Remove o.xferrecv

o.akbd:         ^.c.akbd \
                ^.h.os \
                ^.h.trace \
                ^.h.akbd
                $(CC) $(CCOPTIONS) ^.c.akbd

o.bbc:          ^.c.bbc \
                C:h.stdarg \
                ^.h.os \
                ^.h.bbc
                $(CC) $(CCOPTIONS) ^.c.bbc -DVA_END_SUPERFLUOUS

o.baricon:      ^.c.baricon \
                ^.h.os \
                ^.h.wimp \
                ^.h.wimpt \
                ^.h.trace \
                ^.h.baricon
                $(CC) $(CCOPTIONS) ^.c.baricon

o.cs-wimptx:    ^.c.cs-wimptx \
                ^.h.cs-wimptx \
                ^.h.os
                $(CC) $(CCOPTIONS) ^.c.cs-wimptx

o.dbox:         ^.c.dbox \
                C:h.stdarg \
                C:h.stdlib \
                C:h.string \
                C:h.stdio \
                C:h.limits \
                ^.h.trace \
                ^.h.werr \
                ^.h.os \
                ^.h.akbd \
                ^.h.misc \
                ^.h.wimp \
                ^.h.wimpt \
                ^.h.win \
                ^.h.menu \
                ^.h.event \
                ^.h.dbox \
                ^.h.res \
                ^.h.sprite \
                ^.h.resspr
                $(CC) $(CCOPTIONS) ^.c.dbox -DPDACTION $(TRACE)

o.event:        ^.c.event \
                C:h.stdlib \
                ^.h.trace \
                ^.h.os \
                ^.h.wimp \
                ^.h.wimpt \
                ^.h.win \
                ^.h.menu \
                ^.h.event
                $(CC) $(CCOPTIONS) ^.c.event $(TRACE)

o.fileicon:     ^.c.fileicon \
                C:h.stdio \
                ^.h.os \
                ^.h.wimp \
                ^.h.wimpt \
                ^.h.sprite \
                ^.h.trace \
                ^.h.fileicon \
                ^.h.werr
                $(CC) $(CCOPTIONS) ^.c.fileicon

o.flex:         ^.c.flex \
                C:h.stdlib \
                C:h.stdarg \
                C:h.string \
                C:h.stdio \
                ^.h.os \
                ^.h.werr \
                ^.h.flex \
                ^.h.trace \
                ^.h.wimp \
                ^.h.wimpt
                $(CC) $(CCOPTIONS) ^.c.flex $(TRACE)

o.font:         ^.c.font \
                ^.h.os \
                ^.h.font
                $(CC) $(CCOPTIONS) ^.c.font

o.menu:         ^.c.menu \
                C:h.stdlib \
                C:h.string \
                C:h.stdarg \
                ^.h.os \
                ^.h.wimp \
                ^.h.werr \
                ^.h.menu \
                ^.h.sprite \
                ^.h.resspr
                $(CC) $(CCOPTIONS) ^.c.menu $(TRACE)

o.misc:         ^.c.misc \
                ^.h.misc
                $(CC) $(CCOPTIONS) ^.c.misc

o.os:           ^.c.os \
                ^.h.os \
                ^.h.swi
                $(CC) $(CCOPTIONS) ^.c.os

o.poll:         ^.s.poll
                $(AS) $(ASOPTIONS) ^.s.poll

o.print:        ^.c.print \
                ^.h.os \
                ^.h.print
                $(CC) $(CCOPTIONS) ^.c.print

o.res:          ^.c.res \
                C:h.stdio \
                C:h.string \
                ^.h.os \
                ^.h.trace \
                ^.h.res
                $(CC) $(CCOPTIONS) ^.c.res

o.resspr:       ^.c.resspr \
                C:h.stdlib \
                C:h.stdio \
                ^.h.os \
                ^.h.res \
                ^.h.sprite \
                ^.h.werr \
                ^.h.trace \
                ^.h.resspr
                $(CC) $(CCOPTIONS) ^.c.resspr

o.sprite:       ^.c.sprite \
                C:h.stddef \
                ^.h.os \
                ^.h.sprite
                $(CC) $(CCOPTIONS) ^.c.sprite

o.swi:          ^.s.swi
                $(AS) $(ASOPTIONS) ^.s.swi

o.trace:        ^.c.trace \
                C:h.string \
                C:h.stdarg \
                C:h.stdio \
                ^.h.os \
                ^.h.bbc \
                ^.h.wimp \
                ^.h.wimpt \
                ^.h.trace \
                C:h.stdlib
                $(CC) $(CCOPTIONS) ^.c.trace -DVA_END_SUPERFLUOUS

o.tralloc:      ^.c.tralloc \
                ^.h.tralloc
                $(CC) $(CCOPTIONS) ^.c.tralloc

o.werr:         ^.c.werr \
                C:h.stdarg \
                C:h.stdlib \
                C:h.stdio \
                C:h.string \
                ^.h.os \
                ^.h.wimp \
                ^.h.werr \
                ^.h.wimpt
                $(CC) $(CCOPTIONS) ^.c.werr -DVA_END_SUPERFLUOUS

o.wimp:         ^.c.wimp \
                ^.h.os \
                ^.h.wimp
                $(CC) $(CCOPTIONS) ^.c.wimp

o.wimpt:        ^.c.wimpt \
                C:h.stdarg \
                C:h.stdlib \
                C:h.stdio \
                C:h.signal \
                ^.h.trace \
                ^.h.os \
                ^.h.bbc \
                ^.h.wimp \
                ^.h.wimpt
                $(CC) $(CCOPTIONS) ^.c.wimpt $(TRACE)

o.win:          ^.c.win \
                C:h.stdarg \
                C:h.stdlib \
                ^.h.trace \
                ^.h.os \
                ^.h.wimp \
                ^.h.wimpt \
                ^.h.win
                $(CC) $(CCOPTIONS) ^.c.win $(TRACE)

o.visdelay:     ^.c.visdelay \
                ^.h.os
                $(CC) $(CCOPTIONS) ^.c.visdelay

o.xferrecv:     ^.c.xferrecv \
                C:h.string \
                C:h.stdio \
                ^.h.trace \
                ^.h.os \
                ^.h.bbc \
                ^.h.wimp \
                ^.h.wimpt \
                ^.h.win \
                ^.h.dbox \
                ^.h.fileicon \
                ^.h.werr \
                ^.h.menu \
                ^.h.event \
                ^.h.xferrecv
                $(CC) $(CCOPTIONS) ^.c.xferrecv $(TRACE)

o.xfersend:     ^.c.xfersend \
                C:h.string \
                C:h.stdio \
                ^.h.trace \
                ^.h.os \
                ^.h.bbc \
                ^.h.wimp \
                ^.h.wimpt \
                ^.h.win \
                ^.h.dbox \
                ^.h.xfersend \
                ^.h.fileicon \
                ^.h.werr \
                ^.h.menu \
                ^.h.event
                $(CC) $(CCOPTIONS) ^.c.xfersend $(TRACE)

o.main:         ^.c.main
                $(CC) $(CCOPTIONS) ^.c.main


LastComp:       o.akbd \
                o.baricon \
                o.cs-wimptx \
                o.dbox \
                o.event \
                o.fileicon \
                o.flex \
                o.font \
                o.menu \
                o.misc \
                o.poll \
                o.print \
                o.res \
                o.resspr \
                o.sprite \
                o.trace \
                o.tralloc \
                o.werr \
                o.wimp \
                o.wimpt \
                o.win \
                o.visdelay \
                o.xferrecv \
                o.xfersend
                Create LastComp

# i.e. don't use o.bbc, o.os, o.swi;
# take those from the DDE's RISC_OSLib
# unless you desperately want a RISC OS 2 build
# (which we do, for the 26-bit build vvv)


WimpLib:        Makefile LastComp
                $(LIBFILE) -o -c WimpLib \
 o.* \
 ^.RISC_OSLib.C31B.o.bbc \
 ^.RISC_OSLib.C31B.o.colourtran \
 ^.RISC_OSLib.C31B.o.drawmod \
 ^.RISC_OSLib.C31B.o.os \
 ^.RISC_OSLib.C31B.o.drawcheck \
 ^.RISC_OSLib.C31B.o.drawfiles \
 ^.RISC_OSLib.C31B.o.drawfdiag \
 ^.RISC_OSLib.C31B.o.drawfobj \
 ^.RISC_OSLib.C31B.o.drawtextc \
 ^.RISC_OSLib.C31B.o.swi



# end of Makefile
