; > s.swi

; ****************************************************************************
; * This source file was written by Acorn Computers Limited. It is part of   *
; * the "cwimp" library for writing applications in C for RISC OS. It may be *
; * used freely in the creation of programs for Archimedes. It should be     *
; * used with Acorn's objasm assembler                                       *
; *                                                                          *
; * No support can be given to programmers using this code and, while we     *
; * believe that it is correct, no correspondence can be entered into        *
; * concerning behaviour or bugs.                                            *
; *                                                                          *
; * Upgrades of this code may or may not appear, and while every effort will *
; * be made to keep such upgrades upwards compatible, no guarantees can be   *
; * given.                                                                   *
; ****************************************************************************

; Title  : s.swi
; Purpose: provide access to RISC OS SWIs from C
; Version: 0.1     created
;          0.2 RCM bbc_vdu & bbc_vduw now return ->os_error in R0
;          0.3 SKS made os_swi, os_swix accept NULL inout regset
;                  bbc_get now returns &1xx when an ESCAPE condition is present
;                  optional with names assembly for postmortem

        GBLL    names
names   SETL    {FALSE}


;       GET     "clib:s.h_regs"
        GET     "Hdr:ListOpts"
        GET     "Hdr:Macros"

; SWI values

WriteC        * &00
WriteS        * &01
Write0        * &02
NewLine       * &03
ReadC         * &04
CLI           * &05
Byte          * &06
Word          * &07
File          * &08
Args          * &09
BGet          * &0A

BPut          * &0B
Multiple      * &0C
Open          * &0D
ReadLine      * &0E
Control       * &0F
GetEnv        * &10
Exit          * &11
SetEnv        * &12
IntOn         * &13
IntOff        * &14
CallBack      * &15
EnterOS       * &16
BreakPt       * &17
BreakCtrl     * &18
UnusedSWI     * &19
UpdateMEMC    * &1A
SetCallBack   * &1B
Mouse         * &1C

Heap          * &1D
Module        * &1E
Claim         * &1F     ; vector handling routines
Release       * &20     ; vector handling routines
ReadUnsigned  * &21     ; Read an unsigned number
GenerateEvent * &22
ReadVarVal    * &23     ; read variable value & type
SetVarVal     * &24     ; set  variable value & type
GSInit        * &25
GSRead        * &26
GSTrans       * &27
BinaryToDecimal   * &28
FSControl         * &29
ChangeDynamicArea * &2A
GenerateError     * &2B
ReadEscapeState   * &2C

WriteI        * &100

UserSWI       * &200


Initialise          *    &000400c0
CreateWindow        *    &000400c1
CreateIcon          *    &000400c2
DeleteWindow        *    &000400c3
DeleteIcon          *    &000400c4
OpenWindow          *    &000400c5
CloseWindow         *    &000400c6
Poll                *    &000400c7
RedrawWindow        *    &000400c8
UpdateWindow        *    &000400c9
GetRectangle        *    &000400ca
GetWindowState      *    &000400cb
GetWindowInfo       *    &000400cc
SetIconState        *    &000400cd
GetIconState        *    &000400ce
GetPointerInfo      *    &000400cf
DragBox             *    &000400d0
ForceRedraw         *    &000400d1
SetCaretPosition    *    &000400d2
GetCaretPosition    *    &000400d3
CreateMenu          *    &000400d4
DecodeMenu          *    &000400d5
WhichIcon           *    &000400d6
SetExtent           *    &000400d7
SetPointerShape     *    &000400d8


SWI_OP              * &EF000000 ; SWIAL opcode
XOS_MASK            * &00020000 ; mask to make a swi a RISC OS V-error SWI


; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        AREA    |C$$code|,CODE,READONLY

|x$codeseg|

        EXPORT  |bbc_get|

bbc_get SWI     XOS_MASK :OR: ReadC
        Return  ,LinkNotStacked,VS
        ORRCSS  a1, a1, #&100           ; SKS
        Return  ,LinkNotStacked


        EXPORT  |bbc_vdu|
        EXPORT  |bbc_vduw|

bbc_vduw
        SWI     XOS_MASK :OR: WriteC
        Return  ,LinkNotStacked,VS
        MOV     a1, a1, LSR #8

bbc_vdu SWI     XOS_MASK :OR: WriteC
        MOVVC   a1, #0
        Return  ,LinkNotStacked


; extern void os_swi(int swicode, os_regset* /*inout*/);

; In    a1 contains swi number, a2 points to ARM register structure

 [ names
swilabel
        DCB     "os_swi", 0
        ALIGN
swilength * . - swilabel
        DCD     &FF000000 + swilength
 ]

        EXPORT  |os_swi|

os_swi  STMDB   sp!, {v1-v6, lr}

        ORR     a1, a1, #SWI_OP         ; make into SWI operation

        ADR     v1, exit_sequence
        LDMIA   v1,      {v2,v3,v4}
        STMDB   sp!, {a1, v2,v3,v4}     ; copy SWI and exit code onto stack
        MOVS    ip, a2
        LDMNEIA a2, {r0-r9}             ; load up registers for SWI if wanted
        MOV     pc, sp                  ; and jump to the sequence

;       SWI     whatever                ; <- sp
exit_sequence
        TEQ     ip, #0
        STMNEIA ip, {r0-r9}
 [ CONFIG=26
        LDMIA   sp!, {a1-a4, v1-v6, pc}^ ; a1-a4 just to pop stack
 |
        LDMIA   sp!, {a1-a4, v1-v6, pc}  ; a1-a4 just to pop stack
 ]


; extern os_error *os_swix(int swicode, os_regset* /*inout*/);

; In    a1 contains swi number, a2 points to ARM register structure

 [ names
swixlabel
        DCB     "os_swix", 0
        ALIGN
swixlength * . - swixlabel
        DCD     &FF000000 + swixlength
 ]

        EXPORT  |os_swix|

os_swix STMDB   sp!, {v1-v6, lr}

        ORR     a1, a1, #SWI_OP         ; make into SWI operation
        ORR     a1, a1, #XOS_MASK       ; make a SWI of V-error type

        ADR     v1, xexit_sequence
        LDMIA   v1,      {v2,v3,v4,v5,v6}
        STMDB   sp!, {a1, v2,v3,v4,v5,v6} ; copy SWI and exit code onto stack
        MOVS    ip, a2
        LDMNEIA ip, {r0-r9}             ; load up registers for SWI if wanted
        MOV     pc, sp                  ; and jump to the sequence

;       SWI     Xwhatever               ; <- sp
xexit_sequence
        TEQ     ip, #0
        STMNEIA ip, {r0-r9}
        MOVVC   a1, #0
        ADD     sp, sp, #4*4            ; SWI itself, and three words of exit
        LDMIA   sp!, {a2, a3, v1-v6, pc}^
                                        ; a2, a3 are junk (ADD and LDM)
                                        ; Note: CAN NOT move stack past LDM
                                        ; before instruction executes


; extern os_error *os_swir(int swicode, os_regset *r);
;                               a1          a2

; Do given SWI with register set: if not X form, always return NULL

        EXPORT  |os_swir|

os_swir TST     a1, #XOS_MASK
        BNE     os_swix

        FunctionEntry
        BL      os_swi
        MOV     a1, #0
        Return  ,LinkNotStacked


        END
