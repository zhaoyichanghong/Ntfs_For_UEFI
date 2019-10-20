/** @file
  Routines that check references and flush OFiles

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

  Flushes all data associated with the file handle.

  @param  FHand                 - Handle to file to flush.

  @retval EFI_SUCCESS           - Flushed the file successfully.
  @retval EFI_WRITE_PROTECTED   - The volume is read only.
  @retval EFI_ACCESS_DENIED     - The file is read only.
  @return Others                - Flushing of the file failed.

**/
EFI_STATUS
EFIAPI
NtfsFlush (
  IN EFI_FILE_PROTOCOL  *FHand
  )
{
  NTFS_IFILE   *IFile;
  NTFS_VOLUME  *Volume;
  EFI_STATUS   Status = EFI_SUCCESS;
  UINTN        ret;
  Print(L"NtfsFlushEx+\n");

  IFile   = IFILE_FROM_FHAND (FHand);
  Volume  = IFile->Volume;

  if (Volume->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  //
  // If read only, return error
  //
  if (IFile->ReadOnly) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Flush the OFile
  //
  NtfsAcquireLock ();
  ret = ntfs_device_sync(Volume->VolInfo->dev);
  
  if (ret) {
    Status = EFI_DEVICE_ERROR;
  }
  NtfsCleanupVolume (Volume);
  NtfsReleaseLock ();

  return Status;
}


/**

  Flushes & Closes the file handle.

  @param  FHand                 - Handle to the file to delete.

  @retval EFI_SUCCESS           - Closed the file successfully.

**/
EFI_STATUS
EFIAPI
NtfsClose (
  IN EFI_FILE_PROTOCOL  *FHand
  )
{
  NTFS_IFILE   *IFile;
  NTFS_VOLUME  *Volume;

  IFile   = IFILE_FROM_FHAND (FHand);
  Volume  = IFile->Volume;
  
  Print(L"NtfsClose %s\n", IFile->FileInfo->FileName);

  //
  // Lock the volume
  //
  NtfsAcquireLock ();

  //
  // Close the file instance handle
  //
  NtfsIFileClose (IFile);

  Volume->RefCount--;

  //
  // Done. Unlock the volume
  //
  NtfsCleanupVolume (Volume);
  NtfsReleaseLock ();

  return EFI_SUCCESS;
}

/**

  Close the open file instance.

  @param  IFile                 - Open file instance.

  @retval EFI_SUCCESS           - Closed the file successfully.

**/
EFI_STATUS
NtfsIFileClose (
  NTFS_IFILE          *IFile
  )
{
  NTFS_VOLUME    *Volume;
  LIST_ENTRY     *Link;
  NTFS_IFILE     *SubFile;

  Volume  = IFile->Volume;

  ASSERT_VOLUME_LOCKED (Volume);

  FreePool (IFile->FileInfo);

  while (!IsListEmpty (&IFile->SubFIleHead)) {
    Link = GetFirstNode (&IFile->SubFIleHead);
    RemoveEntryList (Link);
    SubFile = IFILE_FROM_LINK (Link);
    FreePool(SubFile);
  }
  
  //
  // Done. Free the open instance structure
  //
  FreePool (IFile);

  return EFI_SUCCESS;
}
  
/**

  Set error status for a specific OFile, reference checking the volume.
  If volume is already marked as invalid, and all resources are freed
  after reference checking, the file system protocol is uninstalled and
  the volume structure is freed.

  @param  Volume                - the Volume that is to be reference checked and unlocked.
  @param  OFile                 - the OFile whose permanent error code is to be set.
  @param  EfiStatus             - error code to be set.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - Clean up the volume successfully.
  @return Others                - Cleaning up of the volume is failed.

**/
EFI_STATUS
NtfsCleanupVolume (
  IN NTFS_VOLUME      *Volume
  )
{
  //
  // If the volume is cleared , remove it.
  // The only time volume be invalidated is in DriverBindingStop.
  //
  if (Volume->RefCount == 0 && !Volume->Valid) {
  	ntfs_umount(Volume->VolInfo, FALSE);
    //
    // Free the volume structure
    //
    NtfsFreeVolume (Volume);
  }
  
  return EFI_SUCCESS;
}
