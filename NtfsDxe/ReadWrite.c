/** @file
  Functions that perform file read/write.

Copyright (c) 2005 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Ntfs.h"

/**

  Get the file's position of the file.


  @param  FHand                 - The handle of file.
  @param  Position              - The file's position of the file.

  @retval EFI_SUCCESS           - Get the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_UNSUPPORTED       - The open file is not a file.

**/
EFI_STATUS
EFIAPI
NtfsGetPosition (
  IN  EFI_FILE_PROTOCOL *FHand,
  OUT UINT64            *Position
  )
{
  NTFS_IFILE *IFile;
  Print(L"NtfsGetPosition+\n");

  IFile = IFILE_FROM_FHAND (FHand);

  if (IFile->IsDir) {
    return EFI_UNSUPPORTED;
  }

  *Position = IFile->Position;
  return EFI_SUCCESS;
}

/**

  Set the file's position of the file.

  @param  FHand                 - The handle of file.
  @param  Position              - The file's position of the file.

  @retval EFI_SUCCESS           - Set the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_UNSUPPORTED       - Set a directory with a not-zero position.

**/
EFI_STATUS
EFIAPI
NtfsSetPosition (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN UINT64             Position
  )
{
  NTFS_IFILE *IFile;
  Print(L"NtfsSetPosition+\n");

  IFile = IFILE_FROM_FHAND (FHand);

  //
  // If this is a directory, we can only set back to position 0
  //
  if (IFile->IsDir) {
    if (Position != 0) {
      //
      // Reset current directory cursor;
      //
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Set the position
  //
  if (Position == (UINT64)-1) {
    Position = IFile->FileInfo->FileSize;
  }
  //
  // Set the position
  //
  IFile->Position = Position;
  return EFI_SUCCESS;
}

/**

  Get the file info from the open file of the IFile into Buffer.

  @param FHand                 - The file handle to access.
  @param  IoMode                - Indicate whether the access mode is reading or writing.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing read data.
  @param  Token                 - A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @retval EFI_WRITE_PROTECTED   - The disk is write protect.
  @retval EFI_ACCESS_DENIED     - The file is read-only.
  @return other                 - An error occurred when operating on the disk.

**/
EFI_STATUS
NtfsIFileAccess (
  IN     EFI_FILE_PROTOCOL     *FHand,
  IN     IO_MODE               IoMode,
  IN OUT UINTN                 *BufferSize,
  IN OUT VOID                  *Buffer,
  IN     EFI_FILE_IO_TOKEN     *Token
  )
{
  EFI_STATUS   Status = EFI_SUCCESS;
  s64          res = 0;
  NTFS_VOLUME  *Volume;
  NTFS_IFILE   *IFile;
  ntfs_inode   *ni;

  IFile = IFILE_FROM_FHAND (FHand);
  Volume = IFile->Volume;

  if (IoMode == ReadData) {
	//
	// If position is at EOF, then return device error
	//
	if (IFile->Position > IFile->FileInfo->FileSize) {
      return EFI_DEVICE_ERROR;
	}
  } else {
	//
	// Check if the we can write data
    //
	if (Volume->ReadOnly) {
      return EFI_WRITE_PROTECTED;
	}
  
	if (IFile->ReadOnly) {
      return EFI_ACCESS_DENIED;
	}

	if (IFile->IsDir) {
	  return EFI_UNSUPPORTED;
	}
  }
  
  NtfsAcquireLock ();
  
  ni = ntfs_pathname_to_inode(Volume->VolInfo, NULL, IFile->Path);

  if (IoMode == ReadData) {
  	if (IFile->IsDir) {
	  LIST_ENTRY		   *Node;
	  NTFS_IFILE           *SubFile;
	  UINTN                Size = 0;
	  
	  Node = GetFirstNode (&IFile->SubFIleHead);
	  while (!IsNull (&IFile->SubFIleHead, Node)) {
	  	SubFile = IFILE_FROM_LINK (Node);
		if (Size == IFile->Position) {
			break;
	    }
		
	    Size += SubFile->FileInfoSize;
		Node = GetNextNode (&IFile->SubFIleHead, Node);
	  }

	  if (Size == IFile->FileInfo->FileSize) {
	  	*BufferSize = 0;
		Status = EFI_SUCCESS;
	  }
	  else {
	    Status = NtfsGetDirEntInfo (IFile->Volume, SubFile, BufferSize, Buffer, FALSE);
	  }
    }
	else {
  	  res = ntfs_attr_data_read(ni, AT_UNNAMED, 0, Buffer, *BufferSize, IFile->Position);
	  if (res < 0) {
	  	Status = EFI_DEVICE_ERROR;
	  }
	  else {
	  	*BufferSize = res;
	  }
	}
  }
  else {
  	res = ntfs_attr_data_write(ni, AT_UNNAMED, 0, Buffer, *BufferSize, IFile->Position);
	if (res < 0) {
	  Status = EFI_DEVICE_ERROR;
	}
	else {
	  *BufferSize = res;
	  IFile->FileInfo->FileSize = ni->data_size;
      IFile->FileInfo->PhysicalSize = ni->allocated_size;
	}	
  }

  ntfs_inode_close(ni);

  if (!EFI_ERROR (Status)) {
    IFile->Position += *BufferSize;
  }

  if (EFI_ERROR (Status)) {
    NtfsCleanupVolume (Volume);
  }

  NtfsReleaseLock ();
  return Status;
}

/**

  Get the file info.

  @param  FHand                 - The handle of the file.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing read data.


  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
NtfsRead (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT UINTN              *BufferSize,
     OUT VOID               *Buffer
  )
{
  return NtfsIFileAccess (FHand, ReadData, BufferSize, Buffer, NULL);
}

/**

  Write the content of buffer into files.

  @param  FHand                 - The handle of the file.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing write data.

  @retval EFI_SUCCESS           - Set the file info successfully.
  @retval EFI_WRITE_PROTECTED   - The disk is write protect.
  @retval EFI_ACCESS_DENIED     - The file is read-only.
  @retval EFI_DEVICE_ERROR      - The OFile is not valid.
  @retval EFI_UNSUPPORTED       - The open file is not a file.
                        - The writing file size is larger than 4GB.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
NtfsWrite (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
  )
{
  return NtfsIFileAccess (FHand, WriteData, BufferSize, Buffer, NULL);
}
