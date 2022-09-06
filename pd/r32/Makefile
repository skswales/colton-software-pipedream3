# Makefile for PipeDream 3 (32-bit)

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright (C) 1989-1998 Colton Software Limited
# Copyright (C) 1998-2015 R W Colton

# Hand-welded together by SKS 11-Jan-1989

# The default object to create is RISC OS PipeDream 3 (32-bit)

# AMU_PD3DEVROOT=$.pipedream3.pipedrea3__branch
# ^^^ new amu can now take this setting from the environment
# ^^^ but if you are using amu from C Release 5 or so it does not,
# ^^^ so uncomment the AMU_PD3DEVROOT line and amend appropriately

WSP=$(AMU_PD3DEVROOT).CWimp
CMODULES=$(AMU_PD3DEVROOT).CModules

# For release, use ccoptions -fafh, debugging use -fah, sometimes -DTRACE
CC=cc

# options for release
CCOPTIONS=-c -i^,$(CMODULES),$(WSP)    -fafh -DRELEASED -throwback -apcs 3/32  -arch 2 -cpu 3
# Circa C Release 5 (e.g. cc 5.09 in RPCEmu Easy Start Bundle 3.71)
# see defineos.h for how __CC_NORCROFT_VERSION is sorted out, or append -D__CC_NORCROFT_VERSION=509 to force
# CCOPTIONS=-c -i^,$(CMODULES),$(WSP),C: -fafh -DRELEASED -throwback -apcs 3/32  -arch=2
STUBS=C:o.Stubs

# options for debugging
# CCOPTIONS=...  -DWATCH_ALLOCS

# options for release
TRACE=
DBG=0
# options for debugging
# TRACE=-DTRACE
# DBG=1


AS=objasm

ASOPTIONS=-apcs 3/32 -throwback -o $@


LD=link

# options for release
LDOPTIONS=-base &8000 -map -xref > LinkOutput -via LinkFile

# options for debugging
# LDOPTIONS=-base &8000 -map -xref > LinkOutput -via LinkFileDB


# Default target

all: LastMade


# > MakeFile for C modules
# include the appropriate part of this in your own MakeFile

# Hand-welded together by SKS 08-May-1989

# Requires CC, CMODULES defined

CPM=$(CC) -i, $(CCOPTIONS)


# Clean module bits

cleanmodul:
                Remove o.alloc
                Remove o.pd123
                Remove o.123pd
                Remove o.spell
                Remove o.z88com
                Remove o.handlist
                Remove o.vsload
                Remove o.copymem
                Remove o.esch
                Remove o.eventh
                Remove o.monotime


# Module bits

o.alloc:        $(CMODULES).c.alloc \
                $(CMODULES).ext.alloc
                $(CPM) $(CMODULES).alloc.c $(TRACE)

o.pd123:        $(CMODULES).c.pd123 \
                $(CMODULES).h.pd123 \
                $(CMODULES).ext.pd123
                $(CPM) $(CMODULES).pd123.c

o.123pd:        $(CMODULES).c.123pd \
                $(CMODULES).h.pd123 \
                $(CMODULES).ext.pd123
                $(CPM) $(CMODULES).123pd.c

o.spell:        $(CMODULES).c.spell \
                $(CMODULES).h.spell \
                $(CMODULES).ext.spell
                $(CPM) $(CMODULES).spell.c $(TRACE)

o.z88com:       $(CMODULES).c.z88com \
                $(CMODULES).h.z88com \
                $(CMODULES).ext.z88com
                $(CPM) $(CMODULES).z88com.c

o.handlist:     $(CMODULES).c.handlist \
                $(CMODULES).h.handlist \
                $(CMODULES).ext.handlist
                $(CPM) $(CMODULES).handlist.c $(TRACE)

o.vsload:       $(CMODULES).c.vsload \
                $(CMODULES).h.vsload \
                $(CMODULES).ext.vsload
                $(CPM) $(CMODULES).vsload.c

o.report:       $(CMODULES).c.report \
                $(CMODULES).h.report
                $(CPM) $(CMODULES).report.c $(TRACE)

# o.sbscr not made as that's for the PC

o.copymem:      $(CMODULES).s.copymem
                $(AS) $(ASOPTIONS) $(CMODULES).s.copymem

o.esch:         $(CMODULES).s.esch
                $(AS) $(ASOPTIONS) $(CMODULES).s.esch

o.eventh:       $(CMODULES).s.eventh
                $(AS) $(ASOPTIONS) $(CMODULES).s.eventh

o.monotime:     $(CMODULES).s.monotime
                $(AS) $(ASOPTIONS) $(CMODULES).s.monotime


# end of MakeFile (CMODULES include)
# Clean

clean:          cleanmodul \
                cleanpd \
                cleanrisc


cleanpd:
                Remove o.progvars
                Remove o.windvars
                Remove o.browse
                Remove o.bufedit
                Remove o.commlin
                Remove o.handlist
                Remove o.slotconv
                Remove o.constr
                Remove o.cursmov
                Remove o.dialog
                Remove o.doprint
                Remove o.eval
                Remove o.execs
                Remove o.expcomp
                Remove o.help
                Remove o.lists
                Remove o.markers
                Remove o.mcdiff
                Remove o.numbers
                Remove o.pdmain
                Remove o.pdsearch
                Remove o.pdriver
                Remove o.reperr
                Remove o.replace
                Remove o.savload
                Remove o.scdraw
                Remove o.semantic
                Remove o.slector
                Remove o.slot
                Remove o.strings
                Remove o.version
                Remove o.viewio
                Remove o.dtpsave
                Remove o.riscos
                Remove o.riscdialog
                Remove o.riscdraw
                Remove o.riscmenu
                Remove o.report


# PipeDream specific files

o.browse:       ^.c.browse \
                $(CMODULES).ext.spell \
                $(CMODULES).ext.fastsc
                $(CC) $(CCOPTIONS) ^.c.browse

o.bufedit:      ^.c.bufedit \
                $(CMODULES).ext.spell
                $(CC) $(CCOPTIONS) ^.c.bufedit

o.commlin:      ^.c.commlin
                $(CC) $(CCOPTIONS) ^.c.commlin

o.constr:       ^.c.constr
                $(CC) $(CCOPTIONS) ^.c.constr $(TRACE)

o.cursmov:      ^.c.cursmov
                $(CC) $(CCOPTIONS) ^.c.cursmov $(TRACE)

o.dialog:       ^.c.dialog \
                ^.h.riscdialog
                $(CC) $(CCOPTIONS) ^.c.dialog

o.doprint:      ^.c.doprint \
                $(CMODULES).ext.spell \
                $(CMODULES).h.swinumbers
                $(CC) $(CCOPTIONS) ^.c.doprint

o.eval:         ^.c.eval \
                ^.h.eval \
                $(CMODULES).h.monotime
                $(CC) $(CCOPTIONS) ^.c.eval $(TRACE)

o.execs:        ^.c.execs
                $(CC) $(CCOPTIONS) ^.c.execs $(TRACE)

o.expcomp:      ^.c.expcomp \
                ^.h.eval
                $(CC) $(CCOPTIONS) ^.c.expcomp $(TRACE)

o.help:         ^.c.help
                $(CC) $(CCOPTIONS) ^.c.help

o.lists:        ^.c.lists
                $(CC) $(CCOPTIONS) ^.c.lists $(TRACE)

o.markers:      ^.c.markers \
                ^.h.markers
                $(CC) $(CCOPTIONS) ^.c.markers $(TRACE)

o.mcdiff:       ^.c.mcdiff \
                $(CMODULES).h.swinumbers
                $(CC) $(CCOPTIONS) ^.c.mcdiff

o.numbers:      ^.c.numbers
                $(CC) $(CCOPTIONS) ^.c.numbers $(TRACE)

o.pdmain:       ^.c.pdmain \
                $(CMODULES).h.esch \
                $(CMODULES).h.eventh
                $(CC) $(CCOPTIONS) ^.c.pdmain $(TRACE)

o.pdsearch:     ^.c.pdsearch
                $(CC) $(CCOPTIONS) ^.c.pdsearch $(TRACE)

o.pdriver:      ^.c.pdriver
                $(CC) $(CCOPTIONS) ^.c.pdriver

o.progvars:     ^.c.progvars \
                ^.h.progvars
                $(CC) $(CCOPTIONS) ^.c.progvars

o.reperr:       ^.c.reperr
                $(CC) $(CCOPTIONS) ^.c.reperr

o.replace:      ^.c.replace
                $(CC) $(CCOPTIONS) ^.c.replace

o.savload:      ^.c.savload
                $(CC) $(CCOPTIONS) ^.c.savload $(TRACE)

o.scdraw:       ^.c.scdraw
                $(CC) $(CCOPTIONS) ^.c.scdraw $(TRACE)

o.semantic:     ^.c.semantic \
                ^.h.eval
                $(CC) $(CCOPTIONS) ^.c.semantic

o.slector:      ^.c.slector
                $(CC) $(CCOPTIONS) ^.c.slector $(TRACE)

o.slot:         ^.c.slot \
                $(CMODULES).ext.spell
                $(CC) $(CCOPTIONS) ^.c.slot $(TRACE)

o.slotconv:     ^.c.slotconv
                $(CC) $(CCOPTIONS) ^.c.slotconv $(TRACE)

o.strings:      ^.c.strings \
                ^.h.strings
                $(CC) $(CCOPTIONS) ^.c.strings

o.version:      ^.c.version \
                ^.ext.version
                $(CC) $(CCOPTIONS) ^.c.version

o.viewio:       ^.c.viewio \
                ^.h.viewio
                $(CC) $(CCOPTIONS) ^.c.viewio

o.dtpsave:      ^.c.dtpsave \
                ^.h.dtpsave
                $(CC) $(CCOPTIONS) ^.c.dtpsave

o.windvars:     ^.c.windvars \
                ^.h.windvars
                $(CC) $(CCOPTIONS) ^.c.windvars


# RISC OS specific files

o.riscos:       ^.c.riscos \
                ^.h.riscos \
                ^.ext.riscos
                $(CC) $(CCOPTIONS) ^.c.riscos $(TRACE)

o.riscdialog:   ^.c.riscdialog \
                ^.h.riscdialog
                $(CC) $(CCOPTIONS) ^.c.riscdialog $(TRACE)

o.riscdraw:     ^.c.riscdraw \
                ^.h.riscdraw
                $(CC) $(CCOPTIONS) ^.c.riscdraw $(TRACE)

o.riscmenu:     ^.c.riscmenu \
                ^.h.riscmenu
                $(CC) $(CCOPTIONS) ^.c.riscmenu $(TRACE)


# RISC OS version

!PipeDrea3.!RunImage: \
                o.progvars \
                o.windvars \
                o.browse \
                o.bufedit \
                o.commlin \
                o.constr \
                o.cursmov \
                o.dialog \
                o.doprint \
                o.eval \
                o.execs \
                o.expcomp \
                o.help \
                o.lists \
                o.markers \
                o.mcdiff \
                o.numbers \
                o.pdmain \
                o.pdsearch \
                o.pdriver \
                o.reperr \
                o.replace \
                o.riscos \
                o.riscdialog \
                o.riscdraw \
                o.riscmenu \
                o.savload \
                o.scdraw \
                o.semantic \
                o.slotconv \
                o.slector \
                o.slot \
                o.strings \
                o.version \
                o.viewio \
                o.dtpsave \
                o.alloc \
                o.handlist \
                o.spell \
                o.vsload \
                o.report \
                o.esch \
                o.eventh \
                o.monotime \
                $(WSP).r32.WimpLib \
                $(STUBS) \
                LinkFile \
                Makefile
                Remove !PipeDrea3.!RunImage
                $(LD) -output !PipeDrea3.!RunImage $(LDOPTIONS) $(STUBS)
                SetType !PipeDrea3.!RunImage Absolute
                Info !PipeDrea3.!RunImage
                If $(DBG)=0 Then squeeze -v !PipeDrea3.!RunImage
                If $(DBG)=0 Then Info !PipeDrea3.!RunImage


!PipeDrea3.Templates: \
                $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.tem_out
                Copy $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.tem_out $@ ~CFV


$(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.tem_out: \
                $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_main \
                $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_bc \
                $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_el \
                $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_f \
                $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_p \
                $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_s
               @IfThere Run:ccres Then Else Error 0 Put a copy of ccres (built with DDE) in your library
               @| Error 0 Run Make in $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New
                Remove              txt_out
                Print $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_main {  > txt_out }
                Print $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_bc   { >> txt_out }
                Print $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_el   { >> txt_out }
                Print $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_f    { >> txt_out }
                Print $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_p    { >> txt_out }
                Print $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.txt_s    { >> txt_out }
                SetType             txt_out Text
               @Echo If the following step hangs, please replace ccres with one built with DDE
                ccres txt_out $(AMU_PD3DEVROOT).pd.Resources.UK.RISC_OS.WTempl_New.tem_out


LastMade:       !PipeDrea3.Templates \
                !PipeDrea3.!RunImage
                Create LastMade


cleanrisc:
                Remove !PipeDrea3.!RunImage


# end of Makefile
