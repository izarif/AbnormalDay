/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef SE_INCL_ERRORREPORTING_H
#define SE_INCL_ERRORREPORTING_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#if !SE1_EXF_VERIFY_VA_IN_PRINTF

/* Throw an exception of formatted string. */
ENGINE_API extern void ThrowF_t(const char *strFormat, ...) SE1_FORMAT_FUNC(1, 2); // throws char *
/* Report error and terminate program. */
ENGINE_API extern void FatalError(const char *strFormat, ...) SE1_FORMAT_FUNC(1, 2);

// [Cecil] Report error without terminating the program
ENGINE_API extern void ErrorMessage(const char *strFormat, ...) SE1_FORMAT_FUNC(1, 2);

/* Report warning without terminating program (stops program until user responds). */
ENGINE_API extern void WarningMessage(const char *strFormat, ...) SE1_FORMAT_FUNC(1, 2);
/* Report information message to user (stops program until user responds). */
ENGINE_API extern void InfoMessage(const char *strFormat, ...) SE1_FORMAT_FUNC(1, 2);
/* Ask user for yes/no answer(stops program until user responds). */
ENGINE_API extern BOOL YesNoMessage(const char *strFormat, ...) SE1_FORMAT_FUNC(1, 2);

#else

// [Cecil] See 'SE1_EXF_VERIFY_VA_IN_PRINTF' definition
EXF_VERIFY_VA_FUNC(ThrowF_t);
EXF_VERIFY_VA_FUNC(FatalError);
EXF_VERIFY_VA_FUNC(ErrorMessage);
EXF_VERIFY_VA_FUNC(WarningMessage);
EXF_VERIFY_VA_FUNC(InfoMessage);
EXF_VERIFY_VA_FUNC(YesNoMessage);

#endif // SE1_EXF_VERIFY_VA_IN_PRINTF

/* Get the description string for windows error code. */
ENGINE_API extern const CTString GetWindowsError(DWORD dwWindowsErrorCode);


#endif  /* include-once check. */

