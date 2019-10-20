/** @file
  Functions for performing directory entry io.

Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ntfs.h"

/*BOOL 
IsParentRecur(
  ntfs_inode  *par_ni, 
  CHAR8       *CurStr
  )
{
  ntfs_inode    *dir_ni;
  CHAR8         ParStr[PATH_MAX];

  utils_inode_get_name(par_ni, ParStr, PATH_MAX);

  if (strcmp(ParStr, CurStr) == 0) {
  	return TRUE;
  }

  dir_ni = ntfs_dir_parent_inode(par_ni);
  if (dir_ni == NULL) {
  	return FALSE;
  }

  return IsParentRecur(dir_ni, CurStr);
}*/

/*BOOL 
IsDuplicate(
  NTFS_IFILE  *IFile, 
  CHAR8       *CurStr
  )
{
  LIST_ENTRY     *Link;
  NTFS_IFILE     *SubFile;
  CHAR8 		 TempStr[PATH_MAX];

  for ( Link = GetFirstNode (&IFile->SubFIleHead)
    ; !IsNull (&IFile->SubFIleHead, Link)
    ; Link = GetNextNode (&IFile->SubFIleHead, Link)
    ) {
    
    SubFile = IFILE_FROM_LINK (Link);
	utils_inode_get_name(SubFile->ni, TempStr, PATH_MAX);
	if (strcmp(TempStr, CurStr) == 0) {
  	  return TRUE;
    }
  }

  return FALSE;
}*/

static int NtfsFiller(NTFS_IFILE *IFile,
		const ntfschar *name, const int name_len, const int name_type,
		const s64 pos __attribute__((unused)), const MFT_REF mref,
		const unsigned dt_type __attribute__((unused)))
{
  EFI_STATUS     Status;
  NTFS_IFILE     *SubFile;
  int            i;
  ntfs_inode     *ni;
  const CHAR16   *ESTR[] = { L"$MFT", L"$MFTMirr", L"$LogFile",
			  L"$Volume", L"$AttrDef", L"root directory", L"$Bitmap",
			  L"$Boot", L"$BadClus", L"$Secure", L"$UpCase", L"$Extend", L".." };

  for (i = 0; i < ARRAY_COUNT(ESTR); i++) {
  	if (!StrnCmp(name, ESTR[i], name_len)) {
	  return 0;
  	}
  }

  //Print(L"NtfsFiller %s %d %d %d %d\n", name, name_len, mref, dt_type, name_type);

  if (name_type == FILE_NAME_DOS) {
  	return 0;
  }

  Status = NtfsAllocateIFile (IFile->Volume, &SubFile);
  if (EFI_ERROR (Status)) {
  	return -1;
  }

  ni = ntfs_inode_open(IFile->Volume->VolInfo, mref);
  if (ni == NULL) {
  	FreePool(SubFile);
    return -1;
  }
  utils_inode_get_name(ni, SubFile->Path, PATH_MAX);
  ntfs_inode_close(ni);

  SubFile->IsDir = ((dt_type & NTFS_DT_DIR) == NTFS_DT_DIR);
  SubFile->FileInfoSize = SIZE_OF_EFI_FILE_INFO + (name_len + 1) * 2;
  InsertTailList (&IFile->SubFIleHead, &SubFile->LinkNode);
  
  IFile->FileInfo->FileSize += SubFile->FileInfoSize;

  return 0;
}

/**

  Get the directory entry's info into Buffer.

  @param  Volume                - FAT file system volume.
  @param  DirEnt                - The corresponding directory entry.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing file info.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_BUFFER_TOO_SMALL  - The buffer is too small.

**/
EFI_STATUS
NtfsGetDirEntInfo (
  IN     NTFS_VOLUME        *Volume,
  IN     NTFS_IFILE         *IFile,
  IN OUT UINTN              *BufferSize,
    OUT VOID                *Buffer,
  IN     BOOL               IsCountDirSize
  )
{
  UINTN               Size;
  UINTN               NameSize;
  UINTN               ResultSize;
  EFI_STATUS          Status;
  EFI_FILE_INFO       *Info;
  //CHAR8               AsciiStr[PATH_MAX];
  CHAR16              *UnicodeStr;
  CHAR16              *FileName;
  ntfs_inode          *ni;

  //utils_inode_get_name(IFile->ni, AsciiStr, PATH_MAX);
  UnicodeStr = (CHAR16 *) AllocateZeroPool (AsciiStrSize(IFile->Path) * 2);
  Ascii2Unicode(UnicodeStr, IFile->Path);
  //Print(L"%s\n", UnicodeStr);
  //Ascii2Unicode(UnicodeStr, AsciiStr);
  //Print(L"%s\n", UnicodeStr);
  FileName = GetFileNameFromPathW(UnicodeStr);

  ASSERT_VOLUME_LOCKED (Volume);

  Size        = SIZE_OF_EFI_FILE_INFO;
  NameSize    = StrSize (FileName);
  ResultSize  = Size + NameSize;

  Status      = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= ResultSize) {
    Status      = EFI_SUCCESS;
    Info        = Buffer;
    Info->Size  = ResultSize;

    ni = ntfs_pathname_to_inode(Volume->VolInfo, NULL, IFile->Path);
	
	NtfsTime2EfiTime(ni->last_access_time, &Info->LastAccessTime);
	NtfsTime2EfiTime(ni->creation_time, &Info->CreateTime);
	NtfsTime2EfiTime(ni->last_data_change_time, &Info->ModificationTime);
	
    Info->Attribute = ni->flags & EFI_FILE_VALID_ATTR;
	
	if (IFile->IsDir) {
	  IFile->FileInfo->FileSize = 0;
	
	  if (IsCountDirSize) {
	  	INT64 Pos = 0;
	    ntfs_readdir(ni, &Pos, IFile, (ntfs_filldir_t)NtfsFiller);
	  }
	  
	  Info->FileSize = IFile->FileInfo->FileSize;
	  Info->PhysicalSize = Info->FileSize;
	  Info->Attribute |= EFI_FILE_DIRECTORY;
	}
	else {
      Info->FileSize      = ni->data_size;
      Info->PhysicalSize  = ni->allocated_size;
	}

    ntfs_inode_close(ni);
	
    CopyMem ((CHAR8 *) Buffer + Size, FileName, NameSize);
    //Print(L"NtfsGetDirEntInfo %d %d %t %t %t 0x%x %s\n", Info->FileSize, Info->PhysicalSize, Info->LastAccessTime, Info->CreateTime, Info->ModificationTime, Info->Attribute, (Buffer + Size));
  }

  FreePool(UnicodeStr);
  *BufferSize = ResultSize;

  return Status;
}

/**

  Set the relevant directory entry into disk for the volume.

  @param  Volume              - FAT file system volume.
  @param  Name                - The new file name of the volume.

  @retval EFI_SUCCESS         - Update the Volume sucessfully.
  @retval EFI_UNSUPPORTED     - The input label is not a valid volume label.
  @return other               - An error occurred when setting volume label.

**/
EFI_STATUS
NtfsSetVolumeEntry (
  IN NTFS_VOLUME          *Volume,
  IN CHAR16               *Name
  )
{
  UINTN Ret;

  Ret = ntfs_volume_rename(Volume->VolInfo, Name, StrLen(Name));
  if (Ret) {
  	return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
