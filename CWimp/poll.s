; > s.poll

; ****************************************************************************
; * This source file was written by Acorn Computers Limited. It is part of   *
; * the "cwimp" library for writing applications in C for RISC OS. It may be *
; * used freely in the creation of programs for Archimedes. It should be     *
; * used with Acorn's objasm assembler.                                      *
; *                                                                          *
; * No support can be given to programmers using this code and, while we     *
; * believe that it is correct, no correspondence can be entered into        *
; * concerning behaviour or bugs.                                            *
; *                                                                          *
; * Upgrades of this code may or may not appear, and while every effort will *
; * be made to keep such upgrades upwards compatible, no guarantees can be   *
; * given.                                                                   *
; ****************************************************************************

; Title  :  s.poll
; Purpose:  interface to wimp poll for C programs
; Version:  0.1                 created
;           0.2 SKS 22-Nov-88   made SWI Poll use X-form SWI
;           0.3 SKS 12-May-89   new APCS
;           0.4 SKS 17-Jul-89   SHORT defined

;       GET     "clib:s.h_regs"

            GET     "Hdr:ListOpts"
            GET     "Hdr:Macros"
;           GET     "Hdr:System"
 [ {CONFIG}=26
            GET     "Hdr:APCS.APCS-R"
 |
;           GET     "Hdr:APCS.<APCS>"
            GET     "Hdr:APCS.APCS-32" ; for C Release 5
 ]


                GBLL    NO_SURRENDER        ; Whether to keep ESCAPE state
NO_SURRENDER    SETL    {TRUE}

                GBLL    SHORT               ; Whether to export useless routines
SHORT           SETL    {TRUE}


        AREA    |C$$code|,CODE,READONLY

|x$codeseg|

        EXPORT  wimp_poll

; os_error *wimp_poll(wimp_emask mask, wimp_eventstr *result /*out*/);
; a1: poll mask, a2: pointer to event structure

XWimp_Poll      *   (1 :SHL: 17) :OR: &400C7
XWimp_PollIdle  *   (1 :SHL: 17) :OR: &400E1

wimp_poll ROUT

        FunctionEntry   "v1,v2",MakeFrame

        MOV     v1, a1                      ; mask
        MOV     v2, a2                      ; event^

 [ NO_SURRENDER
        BL      save_escape_state           ; NB. before fp_state
 ]

 [ {CONFIG}=26
        LDR     a1, =poll_preserve_fp
        LDR     a1, [a1]
        CMP     a1, #0
        BLNE    save_fp_state
        MOV     ip, a1
 ]

        MOV     a1, v1                      ; restore mask
        ADD     a2, v2, #4                  ; &eventstr->data
        SWI     XWimp_Poll

common_exit ; comes here from below too

        STR     a1, [v2, #0]                ; eventstr->e = rc;
        MOVVC   a1, #0                      ; no error

 [ {CONFIG}=26
        TEQ     ip, #0
        BLNE    restore_fp_state
 ]

 [ NO_SURRENDER
        BL      restore_escape_state        ; NB. after fp_state
 ]

        Return  "v1,v2",fpbased


 [ :LNOT: SHORT
; .............................................................................
; os_error *wimp_pollidle(wimp_emask mask, wimp_eventstr *result, int earliest)
; a1: poll mask, a2: eventstr^, a3: earliest

        EXPORT  wimp_pollidle

wimp_pollidle ROUT

        FunctionEntry   "v1,v2",MakeFrame

        MOV     v1, a1
        MOV     v2, a2

 [ NO_SURRENDER
        BL      save_escape_state           ; NB. before fp_state
 ]

 [ {CONFIG}=26
        LDR     a1, =poll_preserve_fp
        LDR     a1, [a1]
        CMP     a1, #0
        BLNE    save_fp_state
        MOV     ip, a1
 ]

        MOV     a1, v1                      ; restore mask
        ADD     a2, v2, #4                  ; &eventstr->data
        SWI     XWimp_PollIdle

        B       common_exit
 ]


 [ {CONFIG}=26
; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; Only f4..f7 defined preserved over procedure call
; APCS foolishly ignores FP status!

; Out:  fp state stacked, ip corrupted

save_fp_state ROUT

        RFS     ip                      ; save FP status
        STMFD   sp!, {ip}
        MOV     ip, #0                  ; no exceptions
        WFS     ip
        SUB     sp, sp, #4*12           ; emulated a lot faster than writeback
        STFE    f4, [sp, #0*12]
        STFE    f5, [sp, #1*12]
        STFE    f6, [sp, #2*12]
        STFE    f7, [sp, #3*12]
        Return  ,LinkNotStacked


; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; In:   stacked fp state

; Out:  ip corrupted

restore_fp_state ROUT

        MOV     ip, #0                  ; no exceptions
        WFS     ip
        LDFE    f4, [sp, #0*12]
        LDFE    f5, [sp, #1*12]
        LDFE    f6, [sp, #2*12]
        LDFE    f7, [sp, #3*12]
        ADD     sp, sp, #4*12           ; emulated a lot faster than writeback
        LDMFD   sp!, {ip}
        WFS     ip                      ; restore FP status
        Return  ,LinkNotStacked

 [ :LNOT: SHORT
; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern void wimp_save_fp_state_on_poll(void);

        EXPORT  wimp_save_fp_state_on_poll

        IMPORT  |_kernel_fpavailable|

wimp_save_fp_state_on_poll ROUT

        STMFD   sp!, {lr}
        BL      |_kernel_fpavailable|
        LDR     a2, =poll_preserve_fp
        STR     a1, [a2]
        LDMFD   sp!, {pc}^


; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern void wimp_corrupt_fp_state_on_poll(void);

        EXPORT  wimp_corrupt_fp_state_on_poll

wimp_corrupt_fp_state_on_poll ROUT

        MOV     a1, #0
        LDR     a2, =poll_preserve_fp
        STR     a1, [a2]
        Return

        LTORG
 ]
 ] ; {CONFIG}=26


 [ NO_SURRENDER

XOS_Byte * &20006

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; Out:  escape state stacked

save_escape_state ROUT

       Push    "a1, a2, a3, lr, pc" ; note dummy pc push

        MOV     a1, #229                    ; Disable ESCAPE
        MOV     a2, #1
        MOV     a3, #0
        SWI     XOS_Byte
        STR     a2, [sp, #4*4]              ; Poke stack with old value

        MOV     a1, #124                    ; Clear any ESCAPE condition
        SWI     XOS_Byte

        Return  "a1, a2, a3" ; note different regs - escape state left on stack

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; In:   stacked escape state

; Out:  a2 corrupted

restore_escape_state ROUT

        Pull    a2 ; restores stack too

        FunctionEntry "a1, a3"
        MOV     a1, #229                    ; Restore ESCAPE state
        MOV     a3, #0
        SWI     XOS_Byte
        Return  "a1, a3"
 ]


 [ {CONFIG}=26
; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

                    AREA    |C$$data|
|x$dataseg|

poll_preserve_fp    DCD     1               ; Save state by default
 ]


        END
