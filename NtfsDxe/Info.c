/** @file
  Routines dealing with setting/getting file/volume info

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

  Get the volume's info into Buffer.

  @param  Volume                - FAT file system volume.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing volume info.

  @retval EFI_SUCCESS           - Get the volume info successfully.
  @retval EFI_BUFFER_TOO_SMALL  - The buffer is too small.

**/
EFI_STATUS
NtfsGetVolumeInfo (
  IN NTFS_VOLUME       *Volume,
  IN OUT UINTN        *BufferSize,
  OUT VOID            *Buffer
  );

/**

  Set the volume's info.

  @param  Volume                - FAT file system volume.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing the new volume info.

  @retval EFI_SUCCESS           - Set the volume info successfully.
  @retval EFI_BAD_BUFFER_SIZE   - The buffer size is error.
  @retval EFI_WRITE_PROTECTED   - The volume is read only.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
NtfsSetVolumeInfo (
  IN NTFS_VOLUME      *Volume,
  IN UINTN            BufferSize,
  IN VOID            *Buffer
  );

/**

  Set or Get the some types info of the file into Buffer.

  @param  IsSet      - TRUE:The access is set, else is get
  @param  FHand      - The handle of file
  @param  Type       - The type of the info
  @param  BufferSize - Size of Buffer
  @param  Buffer     - Buffer containing volume info

  @retval EFI_SUCCESS       - Get the info successfully
  @retval EFI_DEVICE_ERROR  - Can not find the OFile for the file

**/
EFI_STATUS
NtfsSetOrGetInfo (
  IN BOOLEAN              IsSet,
  IN EFI_FILE_PROTOCOL    *FHand,
  IN EFI_GUID             *Type,
  IN OUT UINTN            *BufferSize,
  IN OUT VOID             *Buffer
  );

/**

  Get the open file's info into Buffer.

  @param  OFile                 - The open file.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing file info.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_BUFFER_TOO_SMALL  - The buffer is too small.

**/
EFI_STATUS
NtfsGetFileInfo (
  IN NTFS_IFILE       *IFile,
  IN OUT UINTN        *BufferSize,
  OUT VOID            *Buffer
  )
{
  EFI_STATUS          Status;
  EFI_FILE_INFO       *Info;

  ASSERT_VOLUME_LOCKED (Volume);

  Status      = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= IFile->FileInfoSize) {
    Status      = EFI_SUCCESS;
	Info        = Buffer;
	
    CopyMem ((CHAR8 *) Buffer, (CHAR8 *)IFile->FileInfo, IFile->FileInfoSize);
  }

  *BufferSize = IFile->FileInfoSize;

  return Status;
}

/**

  Get the volume's info into Buffer.

  @param  Volume                - FAT file system volume.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing volume info.

  @retval EFI_SUCCESS           - Get the volume info successfully.
  @retval EFI_BUFFER_TOO_SMALL  - The buffer is too small.

**/
EFI_STATUS
NtfsGetVolumeInfo (
  IN     NTFS_VOLUME    *Volume,
  IN OUT UINTN          *BufferSize,
     OUT VOID           *Buffer
  )
{
  UINTN                 Size;
  UINTN                 NameSize;
  UINTN                 ResultSize;
  CHAR16                *Name;
  EFI_STATUS            Status;
  EFI_FILE_SYSTEM_INFO  *Info;
  UINT8                 ClusterAlignment;

  Size              = SIZE_OF_EFI_FILE_SYSTEM_INFO;
  Name = (CHAR16 *) AllocateZeroPool (AsciiStrSize(Volume->VolInfo->vol_name) * 2);
  Ascii2Unicode(Name, Volume->VolInfo->vol_name);
  NameSize          = StrSize (Name);
  ResultSize        = Size + NameSize;
  ClusterAlignment  = Volume->VolInfo->cluster_size_bits;

  //
  // If we don't have valid info, compute it now
  //
  //FatComputeFreeInfo (Volume);

  Status = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= ResultSize) {
    Status  = EFI_SUCCESS;

    Info    = Buffer;
    ZeroMem (Info, SIZE_OF_EFI_FILE_SYSTEM_INFO);

    Info->Size        = ResultSize;
    Info->ReadOnly    = Volume->ReadOnly;
    Info->BlockSize   = (UINT32) Volume->VolInfo->cluster_size;
    Info->VolumeSize  = LShiftU64 (Volume->VolInfo->nr_clusters, ClusterAlignment);
	ntfs_volume_get_free_space(Volume->VolInfo);
    Info->FreeSpace   = LShiftU64 (Volume->VolInfo->free_clusters, ClusterAlignment);
    CopyMem ((CHAR8 *) Buffer + Size, Name, NameSize);
  }

  FreePool(Name);
  *BufferSize = ResultSize;
  return Status;
}

/**

  Get the volume's label info into Buffer.

  @param  Volume                - FAT file system volume.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing volume's label info.

  @retval EFI_SUCCESS           - Get the volume's label info successfully.
  @retval EFI_BUFFER_TOO_SMALL  - The buffer is too small.

**/
EFI_STATUS
NtfsGetVolumeLabelInfo (
  IN NTFS_VOLUME      *Volume,
  IN OUT UINTN        *BufferSize,
  OUT VOID            *Buffer
  )
{
  UINTN                             Size;
  UINTN                             NameSize;
  UINTN                             ResultSize;
  CHAR16                            *Name;
  EFI_STATUS                        Status;

  Size        = SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL;
  Name = (CHAR16 *) AllocateZeroPool (AsciiStrSize(Volume->VolInfo->vol_name) * 2);
  Ascii2Unicode(Name, Volume->VolInfo->vol_name);
  NameSize    = StrSize (Name);
  ResultSize  = Size + NameSize;

  Status      = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= ResultSize) {
    Status  = EFI_SUCCESS;
    CopyMem ((CHAR8 *) Buffer + Size, Name, NameSize);
  }

  FreePool(Name);
  *BufferSize = ResultSize;
  return Status;
}

/**

  Set the volume's info.

  @param  Volume                - FAT file system volume.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing the new volume info.

  @retval EFI_SUCCESS           - Set the volume info successfully.
  @retval EFI_BAD_BUFFER_SIZE   - The buffer size is error.
  @retval EFI_WRITE_PROTECTED   - The volume is read only.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
NtfsSetVolumeInfo (
  IN NTFS_VOLUME       *Volume,
  IN UINTN            BufferSize,
  IN VOID             *Buffer
  )
{
  EFI_FILE_SYSTEM_INFO  *Info;

  Info = (EFI_FILE_SYSTEM_INFO *) Buffer;

  if (BufferSize < SIZE_OF_EFI_FILE_SYSTEM_INFO + 2 || Info->Size > BufferSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return NtfsSetVolumeEntry (Volume, Info->VolumeLabel);
}

/**

  Set the volume's label info.

  @param  Volume                - FAT file system volume.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing the new volume label info.

  @retval EFI_SUCCESS           - Set the volume label info successfully.
  @retval EFI_WRITE_PROTECTED   - The disk is write protected.
  @retval EFI_BAD_BUFFER_SIZE   - The buffer size is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
NtfsSetVolumeLabelInfo (
  IN NTFS_VOLUME      *Volume,
  IN UINTN            BufferSize,
  IN VOID             *Buffer
  )
{
  EFI_FILE_SYSTEM_VOLUME_LABEL *Info;

  Info = (EFI_FILE_SYSTEM_VOLUME_LABEL *) Buffer;

  if (BufferSize < SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL + 2) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return NtfsSetVolumeEntry (Volume, Info->VolumeLabel);
}

/**

  Set the file info.

  @param  Volume                - FAT file system volume.
  @param  IFile                 - The instance of the open file.
  @param  OFile                 - The open file.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing the new file info.

  @retval EFI_SUCCESS           - Set the file info successfully.
  @retval EFI_ACCESS_DENIED     - It is the root directory
                          or the directory attribute bit can not change
                          or try to change a directory size
                          or something else.
  @retval EFI_UNSUPPORTED       - The new file size is larger than 4GB.
  @retval EFI_WRITE_PROTECTED   - The disk is write protected.
  @retval EFI_BAD_BUFFER_SIZE   - The buffer size is error.
  @retval EFI_INVALID_PARAMETER - The time info or attributes info is error.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate new memory.
  @retval EFI_VOLUME_CORRUPTED  - The volume is corrupted.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
NtfsSetFileInfo (
  IN NTFS_VOLUME      *Volume,
  IN NTFS_IFILE       *IFile,
  IN UINTN            BufferSize,
  IN VOID             *Buffer
  )
{
  EFI_STATUS     Status = EFI_SUCCESS;
  EFI_FILE_INFO  *NewInfo;
  EFI_TIME       ZeroTime;
  UINT8          NewAttribute;
  BOOLEAN        ReadOnly;
  ntfs_inode     *ni;

  ZeroMem (&ZeroTime, sizeof (EFI_TIME));
  //
  // If this is the root directory, we can't make any updates
  //
  if (IFile->IsRoot) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Make sure there's a valid input buffer
  //
  NewInfo = Buffer;
  if (BufferSize < SIZE_OF_EFI_FILE_INFO + 2 || NewInfo->Size > BufferSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  ReadOnly = IFile->ReadOnly;

  ni =ntfs_pathname_to_inode(Volume->VolInfo, NULL, IFile->Path);
  
  //
  // if a zero time is specified, then the original time is preserved
  //
  if (CompareMem (&ZeroTime, &NewInfo->CreateTime, sizeof (EFI_TIME)) != 0) {
    if (!NtfsIsValidTime (&NewInfo->CreateTime)) {
      Status = EFI_INVALID_PARAMETER;
	  goto Done;
    }

    if (!ReadOnly) {
      EfiTime2NtfsTime(&NewInfo->CreateTime, &ni->creation_time);
    }
  }

  if (CompareMem (&ZeroTime, &NewInfo->ModificationTime, sizeof (EFI_TIME)) != 0) {
    if (!NtfsIsValidTime (&NewInfo->ModificationTime)) {
      Status = EFI_INVALID_PARAMETER;
	  goto Done;
    }

    if (!ReadOnly) {
      EfiTime2NtfsTime(&NewInfo->ModificationTime, &ni->last_data_change_time);
    }
  }

  if (NewInfo->Attribute & (~EFI_FILE_VALID_ATTR)) {
    Status = EFI_INVALID_PARAMETER;
	goto Done;
  }

  NewAttribute = (UINT8) NewInfo->Attribute;
  //
  // Can not change the directory attribute bit
  //
  if ((NewAttribute ^ ni->flags) & EFI_FILE_DIRECTORY) {
    Status = EFI_ACCESS_DENIED;
	goto Done;
  }
  
  //
  // Set the current attributes even if the IFile->ReadOnly is TRUE
  //
  ni->flags = (UINT8) ((ni->flags &~EFI_FILE_VALID_ATTR) | NewAttribute);

Done:
  ntfs_inode_close(ni);
  return Status;
}

/**

  Set or Get the some types info of the file into Buffer.

  @param  IsSet      - TRUE:The access is set, else is get
  @param  FHand      - The handle of file
  @param  Type       - The type of the info
  @param  BufferSize - Size of Buffer
  @param  Buffer     - Buffer containing volume info

  @retval EFI_SUCCESS       - Get the info successfully
  @retval EFI_DEVICE_ERROR  - Can not find the OFile for the file

**/
EFI_STATUS
NtfsSetOrGetInfo (
  IN     BOOLEAN            IsSet,
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN     EFI_GUID           *Type,
  IN OUT UINTN              *BufferSize,
  IN OUT VOID               *Buffer
  )
{
  NTFS_IFILE   *IFile;
  NTFS_VOLUME  *Volume;
  EFI_STATUS  Status;

  IFile   = IFILE_FROM_FHAND (FHand);
  Volume  = IFile->Volume;

  Status = EFI_SUCCESS;

  NtfsAcquireLock ();

  //
  // Verify the file handle isn't in an error state
  //
  if (!EFI_ERROR (Status)) {
    //
    // Get the proper information based on the request
    //
    Status = EFI_UNSUPPORTED;
    if (IsSet) {
      if (CompareGuid (Type, &gEfiFileInfoGuid)) {
        Status = Volume->ReadOnly ? EFI_WRITE_PROTECTED : NtfsSetFileInfo (Volume, IFile, *BufferSize, Buffer);
      }

      if (CompareGuid (Type, &gEfiFileSystemInfoGuid)) {
        Status = Volume->ReadOnly ? EFI_WRITE_PROTECTED : NtfsSetVolumeInfo (Volume, *BufferSize, Buffer);
      }

      if (CompareGuid (Type, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
        Status = Volume->ReadOnly ? EFI_WRITE_PROTECTED : NtfsSetVolumeLabelInfo (Volume, *BufferSize, Buffer);
      }
    } else {
      if (CompareGuid (Type, &gEfiFileInfoGuid)) {
        Status = NtfsGetFileInfo (IFile, BufferSize, Buffer);
      }

      if (CompareGuid (Type, &gEfiFileSystemInfoGuid)) {
        Status = NtfsGetVolumeInfo (Volume, BufferSize, Buffer);
      }

      if (CompareGuid (Type, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
        Status = NtfsGetVolumeLabelInfo (Volume, BufferSize, Buffer);
      }
    }
  }

  NtfsCleanupVolume (Volume);

  NtfsReleaseLock ();
  
  return Status;
}

/**

  Get the some types info of the file into Buffer.

  @param  FHand                 - The handle of file.
  @param  Type                  - The type of the info.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing volume info.

  @retval EFI_SUCCESS           - Get the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.

**/
EFI_STATUS
EFIAPI
NtfsGetInfo (
  IN     EFI_FILE_PROTOCOL   *FHand,
  IN     EFI_GUID            *Type,
  IN OUT UINTN               *BufferSize,
     OUT VOID                *Buffer
  )
{
  Print(L"NtfsGetInfo+\n");

  return NtfsSetOrGetInfo (FALSE, FHand, Type, BufferSize, Buffer);
}

/**

  Set the some types info of the file into Buffer.

  @param  FHand                 - The handle of file.
  @param  Type                  - The type of the info.
  @param  BufferSize            - Size of Buffer
  @param  Buffer                - Buffer containing volume info.

  @retval EFI_SUCCESS           - Set the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.

**/
EFI_STATUS
EFIAPI
NtfsSetInfo (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN EFI_GUID           *Type,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
{
  Print(L"NtfsSetInfo+\n");

  return NtfsSetOrGetInfo (TRUE, FHand, Type, &BufferSize, Buffer);
}
