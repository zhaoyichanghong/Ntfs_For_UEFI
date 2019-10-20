/** @file
  Functions for manipulating file names.

Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ntfs.h"

/**

  Check whether the IFileName is valid long file name. If the IFileName is a valid
  long file name, then we trim the possible leading blanks and leading/trailing dots.
  the trimmed filename is stored in OutputFileName

  @param  InputFileName         - The input file name.
  @param  OutputFileName        - The output file name.

  @retval TRUE                  - The InputFileName is a valid long file name.
  @retval FALSE                 - The InputFileName is not a valid long file name.

**/
BOOLEAN
NtfsFileNameIsValid (
  IN  CHAR16  *InputFileName,
  OUT CHAR16  *OutputFileName
  )
{
  CHAR16  *TempNamePointer;
  CHAR16  TempChar;
  //
  // Trim Leading blanks
  //
  while (*InputFileName == L' ') {
    InputFileName++;
  }

  TempNamePointer = OutputFileName;
  while (*InputFileName != 0) {
    *TempNamePointer++ = *InputFileName++;
  }
  
  //
  // Trim Trailing blanks and dots
  //
  while (TempNamePointer > OutputFileName) {
    TempChar = *(TempNamePointer - 1);
    if (TempChar != L' ') {
      break;
    }

    TempNamePointer--;
  }

  *TempNamePointer = 0;

  //
  // Per FAT Spec the file name should meet the following criteria:
  //   C1. Length (FileLongName) <= 255
  //   C2. Length (X:FileFullPath<NUL>) <= 260
  // Here we check C1.
  //
  if (TempNamePointer - OutputFileName > EFI_FILE_STRING_LENGTH) {
    return FALSE;
  }
  //
  // See if there is any illegal characters within the name
  //
  do {
    if (*OutputFileName < 0x20 ||
        *OutputFileName == '\"' ||
        *OutputFileName == '*' ||
        *OutputFileName == '/' ||
        *OutputFileName == ':' ||
        *OutputFileName == '<' ||
        *OutputFileName == '>' ||
        *OutputFileName == '?' ||
        //*OutputFileName == '\\' ||
        *OutputFileName == '|'
        ) {
      return FALSE;
    }

    OutputFileName++;
  } while (*OutputFileName != 0);
  return TRUE;
}
