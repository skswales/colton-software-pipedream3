/* report.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2022 Stuart Swales */

/* Header file for the report module */

/* SKS February 2013 */

#ifndef __report_h
#define __report_h

/*
exported type
*/

#if defined(__cplusplus)
typedef uintptr_t report_proc;
#define report_proc_cast(proc) reinterpret_cast<report_proc>(proc)
#else
typedef void (* report_proc) (void);
#define report_proc_cast(proc) (report_proc) (proc)
#endif

#define report_ptr_cast(ptr) (void*)(ptr) /* NorCroft has gone a bit wonky with %p */

/*
exported functions
*/

#if defined(__CC_NORCROFT) /* this can check parameters for matching format */
#pragma check_printf_formats
#endif

extern void /*__cdecl*/
reportf(
    /*_In_z_*/ /*_Printf_format_string_*/ const char * format,
    /**/        ...);

extern void
vreportf(
    /*_In_z_*/ /*_Printf_format_string_*/ const char * format,
    /**/        va_list args);

#if defined(__CC_NORCROFT)
#pragma no_check_printf_formats
#endif

extern void
report_enable(
  /*_InVal_*/   BOOL enable);

/*_Check_return_*/
extern BOOL
report_enabled(void);

extern void
report_output(
  /*_In_z_*/    const char * buffer);

/*_Check_return_*/
/*_Ret_z_*/
extern const char *
report_boolstring(
  /*_InVal_*/   BOOL t);

/*_Check_return_*/
/*_Ret_z_*/
extern const char *
report_procedure_name(
    report_proc proc);

/*_Check_return_*/
/*_Ret_z_*/
extern const char *
report_str(
    /*_In_opt_z_*/ const char * tstr);

#if RISCOS

/*_Check_return_*/
/*_Ret_z_*/
extern const char *
report_wimp_event_code(
  /*_InVal_*/   int event_code);

/*_Check_return_*/
/*_Ret_z_*/
extern const char *
report_wimp_event(
  /*_InVal_*/   int event_code,
  /*_In_*/      const void * const p_event_data);

/*_Check_return_*/
/*_Ret_z_*/
extern const char *
report_wimp_message_action(
  /*_InVal_*/   int message_action);

/*_Check_return_*/
/*_Ret_z_*/
extern const char *
report_wimp_message(
  /*_In_*/      const void * const p_wimp_message,
  /*_InVal_*/   BOOL sending);

#endif /* RISCOS */

/*_Check_return_*/
extern BOOL
reporting_is_enabled(void);

#endif /* __report_h */

/* end of report.h */
