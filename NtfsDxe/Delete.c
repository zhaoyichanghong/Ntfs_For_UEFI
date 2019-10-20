/** @file
  Function that deletes a file.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Ntfs.h"

/**

  Deletes the file & Closes the file handle.

  @param  FHand                    - Handle to the file to delete.

  @retval EFI_SUCCESS              - Delete the file successfully.
  @retval EFI_WARN_DELETE_FAILURE  - Fail to delete the file.

**/
EFI_STATUS
EFIAPI
NtfsDelete (
  IN EFI_FILE_PROTOCOL  *FHand
  )
{
  EFI_STATUS     Status = EFI_SUCCESS;
  NTFS_IFILE     *IFile;
  NTFS_VOLUME    *Volume;
  UINTN          ret;
  ntfs_inode     *dir_ni;
  ntfs_inode     *ni;
  
  Print(L"NtfsDelete+\n");
  IFile = IFILE_FROM_FHAND (FHand);
  Volume = IFile->Volume;

  //
  // Lock the volume
  //
  NtfsAcquireLock ();

  //
  // If the file is read-only, then don't delete it
  //
  if (IFile->ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Done;
  }

  //
  // If the file is the root dir, then don't delete it
  //
  if (IFile->IsRoot) {
    Status = EFI_ACCESS_DENIED;
    goto Done;
  }

  ni = ntfs_pathname_to_inode(Volume->VolInfo, NULL, IFile->Path);
  
  dir_ni = ntfs_dir_parent_inode(ni);

  ret = ntfs_delete(Volume->VolInfo, IFile->Path, ni, dir_ni, IFile->FileInfo->FileName, StrLen(IFile->FileInfo->FileName));
  if (ret) {
  	Status = EFI_DEVICE_ERROR;
  }
Done:
  //
  // Always close the handle
  //
  NtfsIFileClose (IFile);
  //
  // Done
  //
  NtfsCleanupVolume (Volume);
  NtfsReleaseLock ();
  
  return Status;
}
