        TTL     SwiC.s : Routine to make a SWI call from C

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

; Copyright (C) 1987-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

; MRJC April 1987
; Modified to return PC April 1988
; SKS modified to preserve caller's condition codes 11-Jan-1989

;       GET     "clib:s.h_regs"


        AREA    |C$$Code|,CODE,READONLY

        EXPORT  |SwiC|

; errp SwiC(SWInum, *rblock /* inout */)
;   a1: int SWInum;                     /* SWI number */
;   a2: int *rblock;                    /* register passing block - 11 ints */
;                                       /* rblock[10] out is returned PC */

SwiC    STMFD   sp!, {v1-v6, lr}

        ORR     a1, a1, #&EF000000  ; OR in SWIAL opcode
        LDR     a3, RETCOD          ; load return code    
        STMFD   sp!, {a1, a3}       ; push SWI code and return code

        MOV     ip, a2              ; remember rblock address for after SWI
        LDMIA   ip, {r0-r9}         ; load registers from rblock

        MOV     lr, pc              ; form return address
        MOV     pc, sp              ; do SWI


        ADD     sp, sp, #8          ; correct stack
        STMIA   ip, {r0-r9, pc}     ; save registers in rblock

        MOVVC   a1, #0              ; return NULL if no error

 [ {CONFIG}=26
        LDMFD   sp!,{v1-v6, pc}^    ; return to caller, restoring flags
 |
        LDMFD   sp!,{v1-v6, pc}     ; return to caller
 ]


RETCOD  MOV     pc, lr              ; assemble return code for load


        END ; of SwiC.s
