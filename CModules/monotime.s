        TTL     monotime.s : Monotonic time handling

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

; Copyright (C) 1989-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

; 0.01 28-Apr-89 SKS created

        GET     "Hdr:ListOpts"
        GET     "Hdr:Macros"
 [ {CONFIG}=26
        GET     "Hdr:APCS.APCS-R"
 |
;       GET     "Hdr:APCS.<APCS>"
        GET     "Hdr:APCS.APCS-32" ; C Release 5
 ]


XOS_ReadMonotonicTime * (1 :SHL: 17) + &42


        AREA    |C$$Code|,CODE,READONLY

        EXPORT  |monotime|
        EXPORT  |monotime_diff|

; typedef unsigned long int monotime_t;

; extern monotime_t monotime(void);

monotime ROUT

        SWI     XOS_ReadMonotonicTime
        Return  ,LinkNotStacked

; extern monotime_t monotime_diff(monotime_t oldtime);

monotime_diff ROUT

        MOV     a2, a1
        SWI     XOS_ReadMonotonicTime
        SUB     a1, a1, a2
        Return  ,LinkNotStacked

        END ; of monotime.s
