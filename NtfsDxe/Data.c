/** @file
  Global data in the FAT Filesystem driver.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ntfs.h"

//
// Globals
//
//
// NtfsFsLock - Global lock for synchronizing all requests.
//
EFI_LOCK NtfsFsLock   = EFI_INITIALIZE_LOCK_VARIABLE (TPL_CALLBACK);

EFI_LOCK NtfsTaskLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);

//
// Filesystem interface functions
//
EFI_FILE_PROTOCOL               NtfsFileInterface = {
  EFI_FILE_PROTOCOL_REVISION,
  NtfsOpen,
  NtfsClose,
  NtfsDelete,
  NtfsRead,
  NtfsWrite,
  NtfsGetPosition,
  NtfsSetPosition,
  NtfsGetInfo,
  NtfsSetInfo,
  NtfsFlush,
  NULL,
  NULL,
  NULL,
  NULL
};
