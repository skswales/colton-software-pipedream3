        TTL     EscH.s : Routine to handle ESCAPEs from within C

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

; Copyright (C) 1988-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

; MRJC May 1988
; SKS fixed 05-Jan-89

        GET     "Hdr:ListOpts"
        GET     "Hdr:Macros"
 [ {CONFIG}=26
        GET     "Hdr:APCS.APCS-R"
 |
;       GET     "Hdr:APCS.<APCS>"
        GET     "Hdr:APCS.APCS-32" ; C Release 5
 ]

OS_Control  *   15

        AREA    |C$$Code|,CODE,READONLY

CatchEsc
        Push    lr
        AND     r11, r11, #&40 ; set flag or clear flag on bit 6 of R11
        LDR     r14, =CtrlfP
        LDR     r14, [r14]
        STR     r11, [r14, #0]

 [ {CONFIG}=26
        Pull    pc,,^  ; 26-bit exit
 |
        TEQ     pc, pc
        Pull    pc, EQ ; 32-bit exit
        Pull    lr
        MOVS    pc, lr ; 26-bit exit
 ]

        LTORG

        EXPORT  |EscH|

; called from C:
; void EscH(int *ctrlflg);

EscH
        FunctionEntry
        LDR     r14, =CtrlfP
        STR     a1, [r14, #0] ; save address of flag

        MOV     r0, #0
        MOV     r1, #0
        ADR     r2, CatchEsc
        MOV     r3, #0
        SWI     OS_Control
        Return


        AREA    |C$$Data|
|x$dataseg|

CtrlfP
        DCD     0


        END ; of EscH.s
