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

#ifndef SE_INCL_SHELL_H
#define SE_INCL_SHELL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Synchronization.h>

#include <Engine/Templates/DynamicArray.h>
#include <Engine/Base/Shell_internal.h>

#define NEXTARGUMENT(type) ( *((type*&)pArgs)++ )

// Object that takes care of shell functions, variables, macros etc.
class ENGINE_API CShell {
public:
// implementation:
  CTCriticalSection sh_csShell; // critical section for access to shell data
  CDynamicArray<CShellSymbol> sh_assSymbols;  // all defined symbols

  // Get a shell symbol by its name.
  CShellSymbol *GetSymbol(const CTString &strName, BOOL bDeclaredOnly);

#if !SE1_EXF_VERIFY_VA_IN_PRINTF
  // Report error in shell script processing.
  void ErrorF(const char *strFormat, ...) SE1_FORMAT_FUNC(2, 3);
#else
  EXF_VERIFY_VA_FUNC(ErrorF); // [Cecil] See 'SE1_EXF_VERIFY_VA_IN_PRINTF' definition
#endif

// interface:

  // Constructor.
  CShell(void);
  ~CShell(void);

  // Initialize the shell.
  void Initialize(void);

  // Declare a symbol in the shell.
  void DeclareSymbol(const CTString &strDeclaration, void *pvValue);

#if !SE1_OLD_COMPILER
  // [Cecil] Declare symbol of any type
  template<typename Type> __forceinline
  void DeclareSymbol(const CTString &strDeclaration, Type *pValue) {
    DeclareSymbol(strDeclaration, (void *)pValue);
  };
#endif

  // Execute command(s).
  void Execute(const CTString &strCommands);
  // Save shell commands to restore persistent symbols to a script file
  void StorePersistentSymbols(const CTFileName &fnScript);

  // get/set symbols
  FLOAT GetFLOAT(const CTString &strName);
  void SetFLOAT(const CTString &strName, FLOAT fValue);
  INDEX GetINDEX(const CTString &strName);
  void SetINDEX(const CTString &strName, INDEX iValue);
  CTString GetString(const CTString &strName);
  void SetString(const CTString &strName, const CTString &strValue);

  CTString GetValue(const CTString &strName);
  void SetValue(const CTString &strName, const CTString &strValue);
};

// pointer to global shell object
ENGINE_API extern CShell *_pShell;


#endif  /* include-once check. */

