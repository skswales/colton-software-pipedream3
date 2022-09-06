        TTL     EventH.s : Routine to block events from within C

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

; Copyright (C) 1989-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

; SKS 29-Jun-89

        GET     "Hdr:ListOpts"
        GET     "Hdr:Macros"
 [ {CONFIG}=26
        GET     "Hdr:APCS.APCS-R"
 |
;       GET     "Hdr:APCS.<APCS>"
        GET     "Hdr:APCS.APCS-32" ; C Release 5
 ]

XOS_Control *   (1 :SHL: 17) + 15

        AREA    |C$$Code|,CODE,READONLY

EventHandler
 [ {CONFIG}=32
        TEQ     pc, pc
        MOVEQ   pc, lr ; 32-bit exit
 |
        MOVS    pc, lr ; 26-bit exit
 ]

        EXPORT  |EventH|

; called from C:
; extern void *EventH(void);

EventH
        FunctionEntry
        MOV     r0, #0
        MOV     r1, #0
        MOV     r2, #0
        ADR     r3, EventHandler
        SWI     XOS_Control
        ADR     r0, EventHandler
        CMP     r0, r3
        MOVNE   r0, r3
        MOVEQ   r0, #0
        Return

        END ; of EventH.s
