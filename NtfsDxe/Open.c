/** @file
  Routines dealing with file open.

Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ntfs.h"

/**

  Create an Open instance for the existing OFile.
  The IFile of the newly opened file is passed out.

  @param  OFile                 - The file that serves as a starting reference point.
  @param  PtrIFile              - The newly generated IFile instance.

  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory for the IFile
  @retval EFI_SUCCESS           - Create the new IFile for the OFile successfully

**/
EFI_STATUS
NtfsAllocateIFile (
  IN NTFS_VOLUME  *Volume,
  OUT NTFS_IFILE  **PtrIFile
  )
{
  NTFS_IFILE *IFile;

  ASSERT_VOLUME_LOCKED (Volume);

  //
  // Allocate a new open instance
  //
  IFile = AllocateZeroPool (sizeof (NTFS_IFILE));
  if (IFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IFile->Signature = NTFS_IFILE_SIGNATURE;

  CopyMem (&(IFile->Handle), &NtfsFileInterface, sizeof (EFI_FILE_PROTOCOL));

  IFile->Handle.Revision = EFI_FILE_PROTOCOL_REVISION;

  IFile->Volume = Volume;

  InitializeListHead (&IFile->LinkNode);
  InitializeListHead (&IFile->SubFIleHead);

  *PtrIFile = IFile;
  return EFI_SUCCESS;
}

/**
 * create_pathname - Create a path/file from some components
 * @dir:      Directory in which to create the file (optional)
 * @name:     Filename to give the file (optional)
 * @stream:   Name of the stream (optional)
 * @buffer:   Store the result here
 * @bufsize:  Size of buffer
 *
 * Create a filename from various pieces.  The output will be of the form:
 *	dir/file
 *	dir/file:stream
 *	file
 *	file:stream
 *
 * All the components are optional.  If the name is missing, "unknown" will be
 * used.  If the directory is missing the file will be created in the current
 * directory.  If the stream name is present it will be appended to the
 * filename, delimited by a colon.
 *
 * N.B. If the buffer isn't large enough the name will be truncated.
 *
 * Return:  n  Length of the allocated name
 */
static int create_pathname(const char *dir, const char *name,
	char *buffer, int bufsize)
{
	if (dir)
		snprintf(buffer, bufsize, "%s%c%s", dir, PATH_SEP, name);
	else
		snprintf(buffer, bufsize, "%s", name);

	return strlen(buffer);
}

/**

  Open a file for a file name relative to an existing OFile.
  The IFile of the newly opened file is passed out.

  @param  OFile                 - The file that serves as a starting reference point.
  @param  NewIFile              - The newly generated IFile instance.
  @param  FileName              - The file name relative to the OFile.
  @param  OpenMode              - Open mode.
  @param  Attributes            - Attributes to set if the file is created.


  @retval EFI_SUCCESS           - Open the file successfully.
  @retval EFI_INVALID_PARAMETER - The open mode is conflict with the attributes
                          or the file name is not valid.
  @retval EFI_NOT_FOUND         - Conficts between dir intention and attribute.
  @retval EFI_WRITE_PROTECTED   - Can't open for write if the volume is read only.
  @retval EFI_ACCESS_DENIED     - If the file's attribute is read only, and the
                          open is for read-write fail it.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.

**/
EFI_STATUS
NtfsOFileOpen (
  IN  NTFS_IFILE           *IFile,
  OUT NTFS_IFILE           **NewIFile,
  IN  CHAR16               *FileName,
  IN  UINT64               OpenMode,
  IN  UINT8                Attributes
  )
{
  EFI_STATUS   Status;
  NTFS_VOLUME  *Volume;
  BOOLEAN      WriteMode;
  ntfs_inode   *ni;
  ntfs_inode   *cur_ni;
  CHAR8        *AsciiFileName;
  CHAR8        AsciiPath[PATH_MAX];
  CHAR16       *uname;
  mode_t       type = 0;
  CHAR16       *NewFileName;

  Volume = IFile->Volume;
  
  ASSERT_VOLUME_LOCKED (Volume);
  
  Print(L"NtfsOFileOpen0 %s 0x%x 0x%llx\n", FileName, Attributes, OpenMode);
  WriteMode = (BOOLEAN) (OpenMode & EFI_FILE_MODE_WRITE);
  if (Volume->ReadOnly && WriteMode) {
    return EFI_WRITE_PROTECTED;
  }

  NewFileName = (CHAR16 *) AllocateZeroPool (StrSize(FileName));
  if (!NtfsFileNameIsValid (FileName, NewFileName)) {
    return EFI_INVALID_PARAMETER;
  }

  if (StrCmp(NewFileName, L".") == 0) {
	ni = ntfs_pathname_to_inode(Volume->VolInfo, NULL, IFile->Path);
	AsciiStrCpy(AsciiPath, IFile->Path);
  }
  else if (StrCmp(NewFileName, L"..") == 0) {
    cur_ni = ntfs_pathname_to_inode(Volume->VolInfo, NULL, IFile->Path);
    ni = ntfs_dir_parent_inode(cur_ni);
    ntfs_inode_close(cur_ni);
	utils_inode_get_name(ni, AsciiPath, PATH_MAX);
  }
  else {
    AsciiFileName = (CHAR8 *) AllocateZeroPool (StrLen(NewFileName) + 2);
    Unicode2Ascii(AsciiFileName, NewFileName);
  
    create_pathname(IFile->Path, AsciiFileName, AsciiPath, PATH_MAX);
	FreePool(AsciiFileName);
  
    ni = ntfs_pathname_to_inode(Volume->VolInfo, NULL, AsciiPath);
    if (!ni) {
      if ((OpenMode & EFI_FILE_MODE_CREATE) == 0) {
	  	FreePool(NewFileName);
	    return EFI_NOT_FOUND;
      }
	
	  uname = (ntfschar *)GetFileNameFromPathW(NewFileName);
	
      if ((Attributes & EFI_FILE_DIRECTORY) != 0) {
        type |= S_IFDIR;
      }

      if ((Attributes & EFI_FILE_SYSTEM) == 0 && (Attributes & EFI_FILE_DIRECTORY) == 0) {
        type |= S_IFREG;
      }

	  if ((Attributes & EFI_FILE_READ_ONLY) != 0) {
        type |= S_IREADONLY;
      }

	  if ((Attributes & EFI_FILE_HIDDEN) != 0) {
        type |= S_IHIDDEN;
      }
	  
	  cur_ni = ntfs_pathname_to_inode(Volume->VolInfo, NULL, IFile->Path);
      ni = ntfs_create(cur_ni, const_cpu_to_le32(0), uname, StrLen(uname), type);
	  ntfs_inode_close(cur_ni);
    }
  }
  FreePool(NewFileName);
  
  if (!ni) {
	return EFI_DEVICE_ERROR;
  }

  if ((ni->flags & EFI_FILE_READ_ONLY) != 0 && (ni->flags & FILE_ATTR_DIRECTORY) == 0 && WriteMode) {
  	ntfs_inode_close(ni);
	return EFI_ACCESS_DENIED;
  }

  //
  // Create an open instance of the OFile
  //
  Status = NtfsAllocateIFile (Volume, NewIFile);
  if (EFI_ERROR (Status)) {
  	ntfs_inode_close(ni);
    return Status;
  }
  (*NewIFile)->IsDir = IsDir(ni);
  (*NewIFile)->ReadOnly = (BOOLEAN)!WriteMode;
  AsciiStrCpy((*NewIFile)->Path, AsciiPath);

  ntfs_inode_close(ni);

  (*NewIFile)->FileInfoSize = 0;
  Status = NtfsGetDirEntInfo(Volume, *NewIFile, &(*NewIFile)->FileInfoSize, NULL, TRUE);
  if (Status == EFI_BUFFER_TOO_SMALL) {
  	(*NewIFile)->FileInfo = AllocateZeroPool((*NewIFile)->FileInfoSize);
	if ((*NewIFile)->FileInfo == NULL) {
      FreePool (*NewIFile);
	  return EFI_OUT_OF_RESOURCES;
	}

	Status = NtfsGetDirEntInfo(Volume, *NewIFile, &(*NewIFile)->FileInfoSize, (*NewIFile)->FileInfo, TRUE);
	if (EFI_ERROR(Status)) {
	  FreePool((*NewIFile)->FileInfo);
	  FreePool (*NewIFile);
	  return EFI_DEVICE_ERROR;
	}
  }

  Volume->RefCount++;

  DEBUG ((EFI_D_INFO, "FSOpen: Open '%S' %r\n", FileName, Status));
  return EFI_SUCCESS;
}

/**

  Implements Open() of Simple File System Protocol.


  @param   FHand                 - File handle of the file serves as a starting reference point.
  @param   NewHandle             - Handle of the file that is newly opened.
  @param   FileName              - File name relative to FHand.
  @param   OpenMode              - Open mode.
  @param   Attributes            - Attributes to set if the file is created.

  @retval EFI_INVALID_PARAMETER - The FileName is NULL or the file string is empty.
                          The OpenMode is not supported.
                          The Attributes is not the valid attributes.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory for file string.
  @retval EFI_SUCCESS           - Open the file successfully.
  @return Others                - The status of open file.

**/
EFI_STATUS
EFIAPI
NtfsOpen (
  IN  EFI_FILE_PROTOCOL   *FHand,
  OUT EFI_FILE_PROTOCOL   **NewHandle,
  IN  CHAR16              *FileName,
  IN  UINT64              OpenMode,
  IN  UINT64              Attributes
  )
{
  NTFS_IFILE  *IFile;
  NTFS_IFILE  *NewIFile;
  EFI_STATUS  Status;
  Print(L"NtfsOpen+\n");

  //
  // Perform some parameter checking
  //
  if (FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check for a valid mode
  //
  switch (OpenMode) {
  case EFI_FILE_MODE_READ:
  case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE:
  case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE:
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for valid Attributes for file creation case. 
  //
  //if (((OpenMode & EFI_FILE_MODE_CREATE) != 0) && (Attributes & (EFI_FILE_READ_ONLY | (~EFI_FILE_VALID_ATTR))) != 0) {
  //  return EFI_INVALID_PARAMETER;
  //}

  IFile = IFILE_FROM_FHAND (FHand);
  //Print(L"NtfsOpenEx %s\n", IFile->FileInfo->FileName);

  //
  // Lock
  //
  NtfsAcquireLock ();

  //
  // Open the file
  //
  Status = NtfsOFileOpen (IFile, &NewIFile, FileName, OpenMode, (UINT8) Attributes);

  //
  // If the file was opened, return the handle to the caller
  //
  if (!EFI_ERROR (Status)) {
    *NewHandle = &NewIFile->Handle;
  }
  //
  // Unlock
  //
  NtfsCleanupVolume (IFile->Volume);
  NtfsReleaseLock ();

  return Status;
}

