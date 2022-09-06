            TTL     copymem.s

            SUBT    Fast memory copier for ARM systems

; Author:       Stuart K. Swales 26-Sep-1989 adapted for PipeDream 3 use

        GET     "Hdr:ListOpts"
        GET     "Hdr:Macros"
 [ {CONFIG}=26
        GET     "Hdr:Machine.26"
        GET     "Hdr:CPU.Generic26"
 ]
        ;GET     "Hdr:System"
 [ {CONFIG}=26
        GET     "Hdr:APCS.APCS-R"
 |
        GET     "Hdr:APCS.<APCS>"
 ]

            GBLL    False
False       SETL    {FALSE}

        GET     "Hdr:Debug"
 [ {FALSE} ; host debug
            GET     "&.Hdr.ModHand"
            GET     "&.Hdr.HostFS"
 ]

            GBLL    debug
debug       SETL    {FALSE}

Host_Debug  SETL    debug :LAND: {TRUE}

            GBLS    Proc_RegList    ; Which registers to preserve
            GBLA    Proc_LocalStack ; And any ADJSP on entry/exit for local vars

            GBLL    Proc_Debug      ; Whether to dump procedure name in image
Proc_Debug  SETL    {FALSE}


        GBLS    nextreg

        MACRO
$label  MergeFirstDown $a
$label  ORR     srcLo, srcLo, $a, LSL shftL
nextreg SETS    "$a"
        MEND

        MACRO
$label  MergeWordsDown $a
 ASSERT $nextreg < $a
$label  MOV     $nextreg, $nextreg, LSR shftR
        ORR     $nextreg, $nextreg, $a, LSL shftL
nextreg SETS    "$a"
        MEND

        MACRO
$label  PrepareDown
$label  MOV     srcLo, srcHi, LSR shftR
        MEND


        MACRO
$label  MergeFirstUp $a
$label  ORR     srcHi, srcHi, $a, LSR shftR
nextreg SETS    "$a"
        MEND

        MACRO
$label  MergeWordsUp $a
 ASSERT $nextreg > $a
$label  MOV     $nextreg, $nextreg, LSL shftL
        ORR     $nextreg, $nextreg, $a, LSR shftR
nextreg SETS    "$a"
        MEND

        MACRO
$label  PrepareUp
$label  MOV     srcHi, srcLo, LSL shftL
        MEND


        AREA    |C$$Code|,PIC,CODE,READONLY

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; 
; MoveBytes(source, dest, size in bytes) - fast data copier
; =========

; ** Not yet optimised to do transfers to make most of 1N,3S feature of MEMC **

; In:   r0 = dst^ (byte address)
;       r1 = src^ (byte address)
;       r2 = count (byte count - never zero!)

; Out:  r0-r3, lr corrupt

dstptr  RN 0
srcptr  RN 1
nbytes  RN 2
srcLo   RN 3
src3    RN 4
src4    RN 5
src5    RN 6
src6    RN 7
src7    RN 8
src8    RN 9
src9    RN 10
shftL   RN 11                       ; These two go at end to save a word
shftR   RN 12                       ; and an extra Pull lr!
srcHi   RN 14                       ; Note deviancy, so care in LDM/STM

        EXPORT  copymem

        ALIGN

copymem ROUT

 [ debug
 DREG srcptr,"copymem(",cc
 DREG dstptr,", ",cc
 DREG nbytes,", ",cc
 DLINE ")"
 ]

        CMP     nbytes, #0
        CMPNE   dstptr, srcptr
        Return  ,LinkNotStacked,EQ           ; [moving to self or nothing to move]

        FunctionEntry                        ; doesn't affect PSR, just pushes lr

        ADDHI   lr, srcptr, nbytes
        CMPHI   lr, dstptr
        BHI     MovByt500                   ; [overlap]

 [ debug
 DLINE "Destination block doesn't start within the source block"
 DLINE " - start copy from low memory end"
 ]
        TST     dstptr, #3
        BNE     MovByt100                   ; [dst^ not word aligned]

MovByt20 ; dst^ now word aligned. branched back to from below

        TST     srcptr, #3
        BNE     MovByt200                   ; [src^ not word aligned]

 [ debug
 DLINE "src^ & dst^ are now both word aligned"
 ]
; nbytes is a byte value (may not be a whole number of words)

; Quick sort out of what we've got left to do

        SUBS    nbytes, nbytes, #4*4        ; Four whole words to do (or more) ?
        BLT     MovByt40                    ; [no]

        SUBS    nbytes, nbytes, #8*4-4*4    ; Eight whole words to do (or more) ?
        BLT     MovByt30                    ; [no]

        Push    "src3-src8"                 ; Push some more registers
MovByt25
 [ debug
 DREG srcptr,"Copying 8 words from ",cc
 DREG dstptr," to "
 ]
        LDMIA   srcptr!, {srcLo, src3-src8, srcHi}  ; NB. Order!
        STMIA   dstptr!, {srcLo, src3-src8, srcHi}

        SUBS    nbytes, nbytes, #8*4
        BGE     MovByt25                    ; [do another 8 words]
        Pull    "src3-src8"                 ; Outside loop (silly otherwise)

        CMP     nbytes, #-8*4               ; Quick test rather than chaining down
        Return  ,,EQ ; [finished]


MovByt30
        ADDS    nbytes, nbytes, #8*4-4*4    ; Four whole words to do ?
        BLT     MovByt40

        Push    "src3-src4"                 ; Push some more registers
 [ debug
 DREG srcptr,"Copying 4 words from ",cc
 DREG dstptr," to "
 ]
        LDMIA   srcptr!, {srcLo, src3-src4, srcHi}  ; NB. Order!
        STMIA   dstptr!, {srcLo, src3-src4, srcHi}
        Pull    "src3-src4"

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #4*4


MovByt40
        ADDS    nbytes, nbytes, #4*4-2*4    ; Two whole words to do ?
        BLT     MovByt50

 [ debug
 DREG srcptr,"Copying 2 words from ",cc
 DREG dstptr," to "
 ]
        LDMIA   srcptr!, {srcLo, srcHi}
        STMIA   dstptr!, {srcLo, srcHi}

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #2*4


MovByt50
        ADDS    nbytes, nbytes, #2*4-1*4    ; One whole word to do ?
        BLT     MovByt60

 [ debug
 DREG srcptr,"Copying 1 word from ",cc
 DREG dstptr," to "
 ]
        LDMIA   srcptr!, {srcLo}
        STMIA   dstptr!, {srcLo}

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #1*4


MovByt60
        ADDS    nbytes, nbytes, #1*4-0*4    ; No more to do ?
        Return  ,,EQ ; [finished]


        LDR     srcLo, [srcptr]             ; Store remaining 1, 2 or 3 bytes
 [ debug
 DREG srcptr,"Loading 1 word from "
 ]
MovByt70
 [ debug
 DREG dstptr,"Storing 1 byte at "
 ]
        STRB    srcLo, [dstptr], #1
        MOV     srcLo, srcLo, LSR #8
        SUBS    nbytes, nbytes, #1
        BGT     MovByt70

        Return  , ; [finished]


MovByt100
 [ debug
 DLINE "Initial dst^ not word aligned. Loop doing bytes (1, 2 or 3) until it is"
 ]
MovByt101
 [ debug
 DREG srcptr,"Copying 1 byte from ",cc
 DREG dstptr," to "
 ]
        LDRB    srcLo, [srcptr], #1
        STRB    srcLo, [dstptr], #1
        SUBS    nbytes, nbytes, #1
        Return  ,,EQ ; [finished] (after 1..3 bytes)
        TST     dstptr, #3
        BNE     MovByt101

        B       MovByt20                    ; Back to mainline code



MovByt200
 [ debug
 DLINE "dst^ now word aligned, but src^ isn't"
 ]

 ASSERT "$Proc_RegList" = "" ; ie. just lr stacked at the moment
Proc_RegList SETS "shftL, shftR"

        Push    "shftL, shftR"              ; Need more registers this section

        AND     shftR, srcptr, #3           ; Offset
        BIC     srcptr, srcptr, #3          ; Align src^

        MOV     shftR, shftR, LSL #3        ; rshft = 0, 8, 16 or 24 only
        RSB     shftL, shftR, #32           ; lshft = 32, 24, 16 or 8 only

 [ debug
 DREG srcptr,"Loading first partial word from "
 DREG shftL,"Left shift  "
 DREG shftR,"Right shift "
 ]
        LDR     srcHi, [srcptr], #4
        PrepareDown

; Quick sort out of what we've got left to do

        SUBS    nbytes, nbytes, #4*4        ; Four whole words to do (or more) ?
        BLT     MovByt240                   ; [no]

        SUBS    nbytes, nbytes, #8*4-4*4    ; Eight whole words to do (or more) ?
        BLT     MovByt230                   ; [no]

        Push    "src3-src9"                 ; Push some more registers
MovByt225
 [ debug
 DREG srcptr,"Copying 8 words from ",cc
 DREG srcLo," merging low word with ",cc
 DREG dstptr," to "
 ]
        LDMIA   srcptr!, {src3-src9, srcHi} ; NB. Order!
        MergeFirstDown  src3
        MergeWordsDown  src4
        MergeWordsDown  src5
        MergeWordsDown  src6
        MergeWordsDown  src7
        MergeWordsDown  src8
        MergeWordsDown  src9
        MergeWordsDown  srcHi
        STMIA   dstptr!, {srcLo, src3-src9}
        PrepareDown

        SUBS    nbytes, nbytes, #8*4
        BGE     MovByt225                   ; [do another 8 words]
        Pull    "src3-src9"

        CMP     nbytes, #-8*4               ; Quick test rather than chaining down
        Return  ,,EQ ; [finished]


MovByt230
        ADDS    nbytes, nbytes, #8*4-4*4    ; Four whole words to do ?
        BLT     MovByt240

        Push    "src3-src5"                 ; Push some more registers
 [ debug
 DREG srcptr,"Copying 4 words from ",cc
 DREG srcLo," merging low word with ",cc
 DREG dstptr," to "
 ]
        LDMIA   srcptr!, {src3-src5, srcHi} ; NB. Order!
        MergeFirstDown  src3
        MergeWordsDown  src4
        MergeWordsDown  src5
        MergeWordsDown  srcHi
        STMIA   dstptr!, {srcLo, src3-src5}
        Pull    "src3-src5"

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #4*4
        PrepareDown


MovByt240
        ADDS    nbytes, nbytes, #2*4        ; Two whole words to do ?
        BLT     MovByt250

        Push    "src3"
 [ debug
 DREG srcptr,"Copying 2 words from ",cc
 DREG srcLo," merging low word with ",cc
 DREG dstptr," to "
 ]
        LDMIA   srcptr!, {src3, srcHi}      ; NB. Order!
        MergeFirstDown  src3
        MergeWordsDown  srcHi
        STMIA   dstptr!, {srcLo, src3}
        Pull    "src3"

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #2*4
        PrepareDown


MovByt250
        ADDS    nbytes, nbytes, #2*4-1*4    ; One whole word to do ?
        BLT     MovByt260

 [ debug
 DREG srcptr,"Copying 1 word from ",cc
 DREG srcLo," merging with ",cc
 DREG dstptr," to "
 ]
        LDMIA   srcptr!, {srcHi}
        MergeFirstDown  srcHi
        STMIA   dstptr!, {srcLo}

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #1*4
        PrepareDown


MovByt260
        ADDS    nbytes, nbytes, #1*4-0*4
        Return  ,,EQ ; [finished]


 [ debug
 DREG srcptr,"Loading 1 word from ",cc
 DREG srcLo," merging with "
 ]
        LDR     srcHi, [srcptr]             ; Store remaining 1, 2 or 3 bytes
        MergeFirstDown  srcHi
MovByt270
 [ debug
 DREG dstptr,"Storing 1 byte at "
 ]
        STRB    srcLo, [dstptr], #1
        MOV     srcLo, srcLo, LSR #8
        SUBS    nbytes, nbytes, #1
        BGT     MovByt270

        Return  , ; [finished]


Proc_RegList SETS ""

MovByt500
 [ debug
 DLINE "Destination block starts within the source block"
 DLINE "- start copy at high memory end"
 ]
        ADD     srcptr, srcptr, nbytes
        ADD     dstptr, dstptr, nbytes

        TST     dstptr, #3
        BNE     MovByt600                   ; [dst^ not word aligned]

MovByt520 ; dst^ now word aligned. branched back to from below

        TST     srcptr, #3
        BNE     MovByt700                   ; [src^ not word aligned]

 [ debug
 DLINE "src^ & dst^ are now both word aligned"
 ]
; nbytes is a byte value (may not be a whole number of words)

; Quick sort out of what we've got left to do

        SUBS    nbytes, nbytes, #4*4        ; Four whole words to do (or more) ?
        BLT     MovByt540                   ; [no]

        SUBS    nbytes, nbytes, #8*4-4*4    ; Eight whole words to do (or more) ?
        BLT     MovByt530                   ; [no]

        Push    "src3-src8"                 ; Push some more registers
MovByt525
 [ debug
 DREG srcptr,"Copying 8 words from below ",cc
 DREG dstptr," to below "
 ]
        LDMDB   srcptr!, {srcLo, src3-src8, srcHi}  ; NB. Order!
        STMDB   dstptr!, {srcLo, src3-src8, srcHi}

        SUBS    nbytes, nbytes, #8*4
        BGE     MovByt525                   ; [do another 8 words]
        Pull    "src3-src8"                 ; Outside loop (silly otherwise)

        CMP     nbytes, #-8*4               ; Quick test rather than chaining down
        Return  ,,EQ ; [finished]


MovByt530
        ADDS    nbytes, nbytes, #8*4-4*4    ; Four whole words to do ?
        BLT     MovByt540

        Push    "src3-src4"                 ; Push some more registers
 [ debug
 DREG srcptr,"Copying 4 words from below ",cc
 DREG dstptr," to below "
 ]
        LDMDB   srcptr!, {srcLo, src3-src4, srcHi}  ; NB. Order!
        STMDB   dstptr!, {srcLo, src3-src4, srcHi}
        Pull    "src3-src4"

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #4*4


MovByt540
        ADDS    nbytes, nbytes, #4*4-2*4    ; Two whole words to do ?
        BLT     MovByt550

 [ debug
 DREG srcptr,"Copying 2 words from below ",cc
 DREG dstptr," to below "
 ]
        LDMDB   srcptr!, {srcLo, srcHi}
        STMDB   dstptr!, {srcLo, srcHi}

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #2*4


MovByt550
        ADDS    nbytes, nbytes, #2*4-1*4    ; One whole word to do ?
        BLT     MovByt560

 [ debug
 DREG srcptr,"Copying 1 word from below ",cc
 DREG dstptr," to below "
 ]
        LDMDB   srcptr!, {srcLo}
        STMDB   dstptr!, {srcLo}

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #1*4


MovByt560
        ADDS    nbytes, nbytes, #1*4-0*4    ; No more to do ?
        Return  ,,EQ ; [finished]


        LDR     srcLo, [srcptr, #-4]        ; Store remaining 1, 2 or 3 bytes
 [ debug
 DREG srcptr,"Loading 1 word from below "
 ]
MovByt570
 [ debug
 DREG dstptr,"Storing 1 byte to below "
 ]
        MOV     srcLo, srcLo, ROR #24       ; ROL #8
        STRB    srcLo, [dstptr, #-1]!
        SUBS    nbytes, nbytes, #1
        BGT     MovByt570

        Return  , ; [finished]


MovByt600
 [ debug
 DLINE "Initial dst^ not word aligned. Loop doing bytes (1, 2 or 3) until it is"
 ]
MovByt601
 [ debug
 DREG srcptr,"Copying 1 byte from below ",cc
 DREG dstptr," to below "
 ]
        LDRB    srcLo, [srcptr, #-1]!
        STRB    srcLo, [dstptr, #-1]!
        SUBS    nbytes, nbytes, #1
        Return  ,,EQ ; [finished] (after 1..3 bytes)
        TST     dstptr, #3
        BNE     MovByt601

        B       MovByt520                   ; Back to mainline code



MovByt700
 [ debug
 DLINE "dst^ now word aligned, but src^ isn't"
 ]

 ASSERT "$Proc_RegList" = "" ; ie. just lr stacked at the moment
Proc_RegList SETS "shftL, shftR"

        Push    "shftL, shftR"              ; Need more registers this section

        AND     shftR, srcptr, #3           ; Offset
        BIC     srcptr, srcptr, #3          ; Align src^

        MOV     shftR, shftR, LSL #3        ; rshft = 0, 8, 16 or 24 only
        RSB     shftL, shftR, #32           ; lshft = 32, 24, 16 or 8 only

 [ debug
 DREG srcptr,"Loading first partial word from "
 DREG shftL,"Left shift  "
 DREG shftR,"Right shift "
 ]
        LDR     srcLo, [srcptr]
        PrepareUp

; Quick sort out of what we've got left to do

        SUBS    nbytes, nbytes, #4*4        ; Four whole words to do (or more) ?
        BLT     MovByt740                   ; [no]

        SUBS    nbytes, nbytes, #8*4-4*4    ; Eight whole words to do (or more) ?
        BLT     MovByt730                   ; [no]

        Push    "src3-src9"                 ; Push some more registers
MovByt725
 [ debug
 DREG srcptr,"Copying 8 words from below ",cc
 DREG srcHi," merging high word with ",cc
 DREG dstptr," to below "
 ]
        LDMDB   srcptr!, {srcLo, src3-src9} ; NB. Order!
        MergeFirstUp    src9
        MergeWordsUp    src8
        MergeWordsUp    src7
        MergeWordsUp    src6
        MergeWordsUp    src5
        MergeWordsUp    src4
        MergeWordsUp    src3
        MergeWordsUp    srcLo
        STMDB   dstptr!, {src3-src9, srcHi} ; NB. Order!
        PrepareUp

        SUBS    nbytes, nbytes, #8*4
        BGE     MovByt725                   ; [do another 8 words]
        Pull    "src3-src9"

        CMP     nbytes, #-8*4               ; Quick test rather than chaining down
        Return  ,,EQ ; [finished]


MovByt730
        ADDS    nbytes, nbytes, #8*4-4*4    ; Four whole words to do ?
        BLT     MovByt740

        Push    "src3-src5"                 ; Push some more registers
 [ debug
 DREG srcptr,"Copying 4 words from below ",cc
 DREG srcHi," merging high word with ",cc
 DREG dstptr," to below "
 ]
        LDMDB   srcptr!, {srcLo, src3-src5} ; NB. Order!
        MergeFirstUp    src5
        MergeWordsUp    src4
        MergeWordsUp    src3
        MergeWordsUp    srcLo
        STMDB   dstptr!, {src3-src5, srcHi} ; NB. Order!
        Pull    "src3-src5"

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #4*4
        PrepareUp


MovByt740
        ADDS    nbytes, nbytes, #2*4        ; Two whole words to do ?
        BLT     MovByt750

        Push    "src3"
 [ debug
 DREG srcptr,"Copying 2 words from below ",cc
 DREG srcHi," merging high word with ",cc
 DREG dstptr," to below "
 ]
        LDMDB   srcptr!, {srcLo, src3}      ; NB. Order!
        MergeFirstUp    src3
        MergeWordsUp    srcLo
        STMDB   dstptr!, {src3, srcHi}      ; NB. Order!
        Pull    "src3"

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #2*4
        PrepareUp


MovByt750
        ADDS    nbytes, nbytes, #2*4-1*4    ; One whole word to do ?
        BLT     MovByt760

 [ debug
 DREG srcptr,"Copying 1 word from below ",cc
 DREG srcHi," merging with ",cc
 DREG dstptr," to below "
 ]
        LDMDB   srcptr!, {srcLo}
        MergeFirstUp    srcLo
        STMDB   dstptr!, {srcHi}

        Return  ,,EQ ; [finished]
        SUB     nbytes, nbytes, #1*4
        MOV     srcHi, srcLo, LSL shftL     ; Keep srcHi prepared


MovByt760
        ADDS    nbytes, nbytes, #1*4-0*4
        Return  ,,EQ ; [finished]


 [ debug
 DREG srcptr,"Loading 1 word from below ",cc
 DREG srcHi," merging with "
 ]
        LDR     srcLo, [srcptr, #-4]        ; Store remaining 1, 2 or 3 bytes
        MergeFirstUp    srcLo
MovByt770
 [ debug
 DREG dstptr,"Storing 1 byte to below "
 ]
        MOV     srcHi, srcHi, ROR #24       ; ROL #8
        STRB    srcHi, [dstptr, #-1]!
        SUBS    nbytes, nbytes, #1
        BGT     MovByt770

        Return  , ; [finished]

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 [ debug
        InsertDebugRoutines
 ]

        END     ; of copymem.s
