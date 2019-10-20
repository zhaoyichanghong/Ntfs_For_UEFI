/** @file
  Main header file for EFI FAT file system driver.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _NTFS_H_
#define _NTFS_H_

#include <Uefi.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/UnicodeCollation.h>

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "config.h"
#include "volume.h"
#include "bootsect.h"
#include "cache.h"
#include "dir.h"
#include "NtfsFileSystem.h"
#include "utils.h"
#include "string.h"
#include "stdlib.h"
#include "wchar.h"
#include "misc.h"
#include <fcntl.h>

#define NTFS_VOLUME_SIGNATURE         SIGNATURE_32 ('n', 't', 'f', 'V')
#define NTFS_IFILE_SIGNATURE          SIGNATURE_32 ('n', 't', 'f', 'i')

#define ASSERT_VOLUME_LOCKED(a)      ASSERT_LOCKED (&NtfsFsLock)

#define IFILE_FROM_FHAND(a)          CR (a, NTFS_IFILE, Handle, NTFS_IFILE_SIGNATURE)

#define VOLUME_FROM_VOL_INTERFACE(a) CR (a, NTFS_VOLUME, VolumeInterface, NTFS_VOLUME_SIGNATURE);

#define IFILE_FROM_LINK(a)           CR (a, NTFS_IFILE, LinkNode, NTFS_IFILE_SIGNATURE)

//
// Efi Time Definition
//
#define IS_LEAP_YEAR(a)                   (((a) % 4 == 0) && (((a) % 100 != 0) || ((a) % 400 == 0)))

#define EFI_FILE_STRING_LENGTH  255

#define ARRAY_COUNT(a) (sizeof (a) / sizeof (a[0]))

//
// Used in NtfsDiskIo
//
typedef enum {
  ReadDisk     = 0,  // raw disk read
  WriteDisk    = 1,  // raw disk write
  ReadNtfs     = 2,  // read fat cache
  WriteNtfs    = 3,  // write fat cache
  ReadData     = 6,  // read data cache
  WriteData    = 7   // write data cache
} IO_MODE;

typedef struct _NTFS_VOLUME NTFS_VOLUME;

typedef struct {
  UINTN               Signature;
  EFI_FILE_PROTOCOL   Handle;
  CHAR8               Path[PATH_MAX];
  INT64               Position;
  BOOLEAN             ReadOnly;
  NTFS_VOLUME         *Volume;
  BOOLEAN             IsDir;
  BOOLEAN             IsRoot;
  UINTN               FileInfoSize;
  EFI_FILE_INFO       *FileInfo;
  LIST_ENTRY          LinkNode;                  
  LIST_ENTRY          SubFIleHead;             
} NTFS_IFILE;

struct _NTFS_VOLUME {
  UINTN                           Signature;

  EFI_HANDLE                      Handle;
  BOOLEAN                         Valid;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL VolumeInterface;
  EFI_FILE_PROTOCOL                 FileInterface;
  UINTN                             RefCount;
  INT64                             Pos;		  /* Logical current position on the volume. */

  //
  // If opened, the parent handle and BlockIo interface
  //
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_DISK_IO_PROTOCOL            *DiskIo;
  EFI_DISK_IO2_PROTOCOL           *DiskIo2;
  UINT32                          MediaId;
  BOOLEAN                         ReadOnly;

  ntfs_volume                     *VolInfo;
};

//
// Function Prototypes
//

/**

  Implements Open() of Simple File System Protocol.

  @param  FHand                 - File handle of the file serves as a starting reference point.
  @param  NewHandle             - Handle of the file that is newly opened.
  @param  FileName              - File name relative to FHand.
  @param  OpenMode              - Open mode.
  @param  Attributes            - Attributes to set if the file is created.


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
  IN  EFI_FILE_PROTOCOL *FHand,
  OUT EFI_FILE_PROTOCOL **NewHandle,
  IN  CHAR16            *FileName,
  IN  UINT64            OpenMode,
  IN  UINT64            Attributes
  );

/**

  Get the file's position of the file

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
  );

/**

  Get the some types info of the file into Buffer

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
  IN     EFI_FILE_PROTOCOL      *FHand,
  IN     EFI_GUID               *Type,
  IN OUT UINTN                  *BufferSize,
     OUT VOID                   *Buffer
  );

/**

  Set the some types info of the file into Buffer.

  @param  FHand                 - The handle of file.
  @param  Type                  - The type of the info.
  @param  BufferSize            - Size of Buffer.
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
  );

/**

  Flushes all data associated with the file handle.

  @param  FHand                 - Handle to file to flush

  @retval EFI_SUCCESS           - Flushed the file successfully
  @retval EFI_WRITE_PROTECTED   - The volume is read only
  @retval EFI_ACCESS_DENIED     - The volume is not read only
                          but the file is read only
  @return Others                - Flushing of the file is failed

**/
EFI_STATUS
EFIAPI
NtfsFlush (
  IN EFI_FILE_PROTOCOL  *FHand
  );

/**

  Flushes & Closes the file handle.

  @param  FHand                 - Handle to the file to delete.

  @retval EFI_SUCCESS           - Closed the file successfully.

**/
EFI_STATUS
EFIAPI
NtfsClose (
  IN EFI_FILE_PROTOCOL  *FHand
  );

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
  );

/**

  Set the file's position of the file.

  @param  FHand                 - The handle of file
  @param  Position              - The file's position of the file

  @retval EFI_SUCCESS           - Set the info successfully
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file
  @retval EFI_UNSUPPORTED       - Set a directory with a not-zero position

**/
EFI_STATUS
EFIAPI
NtfsSetPosition (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN UINT64             Position
  );

/**

  Get the file info.

  @param FHand                 - The handle of the file.
  @param BufferSize            - Size of Buffer.
  @param Buffer                - Buffer containing read data.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
NtfsRead (
  IN     EFI_FILE_PROTOCOL    *FHand,
  IN OUT UINTN                *BufferSize,
     OUT VOID                 *Buffer
  );

/**

  Set the file info.

  @param  FHand                 - The handle of the file.
  @param  BufferSize            - Size of Buffer.
  @param Buffer                - Buffer containing write data.

  @retval EFI_SUCCESS           - Set the file info successfully.
  @retval EFI_WRITE_PROTECTED   - The disk is write protected.
  @retval EFI_ACCESS_DENIED     - The file is read-only.
  @retval EFI_DEVICE_ERROR      - The OFile is not valid.
  @retval EFI_UNSUPPORTED       - The open file is not a file.
                        - The writing file size is larger than 4GB.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
NtfsWrite (
  IN     EFI_FILE_PROTOCOL      *FHand,
  IN OUT UINTN                  *BufferSize,
  IN     VOID                   *Buffer
  );

//
// FileName.c
//

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
  );

//
// Flush.c
//

/**

  Close the open file instance.

  @param  IFile                 - Open file instance.

  @retval EFI_SUCCESS           - Closed the file successfully.

**/
EFI_STATUS
NtfsIFileClose (
  NTFS_IFILE            *IFile
  );

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
  IN NTFS_VOLUME        *Volume
  );

//
// Init.c
//
/**

  Allocates volume structure, detects FAT file system, installs protocol,
  and initialize cache.

  @param  Handle                - The handle of parent device.
  @param  DiskIo                - The DiskIo of parent device.
  @param  DiskIo2               - The DiskIo2 of parent device.
  @param  BlockIo               - The BlockIo of parent devicel

  @retval EFI_SUCCESS           - Allocate a new volume successfully.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.
  @return Others                - Allocating a new volume failed.

**/
EFI_STATUS
NtfsAllocateVolume (
  IN  EFI_HANDLE                     Handle,
  IN  EFI_DISK_IO_PROTOCOL           *DiskIo,
  IN  EFI_BLOCK_IO_PROTOCOL          *BlockIo
  );

/**

  Detects FAT file system on Disk and set relevant fields of Volume.

  @param Volume                - The volume structure.

  @retval EFI_SUCCESS           - The Ntfs File System is detected successfully
  @retval EFI_UNSUPPORTED       - The volume is not FAT file system.
  @retval EFI_VOLUME_CORRUPTED  - The volume is corrupted.

**/
EFI_STATUS
NtfsOpenDevice (
  IN OUT NTFS_VOLUME    *Volume
  );

/**

  Called by NtfsDriverBindingStop(), Abandon the volume.

  @param  Volume                - The volume to be abandoned.

  @retval EFI_SUCCESS           - Abandoned the volume successfully.
  @return Others                - Can not uninstall the protocol interfaces.

**/
EFI_STATUS
NtfsAbandonVolume (
  IN NTFS_VOLUME        *Volume
  );

//
// DirectoryManage.c
//

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
  IN    NTFS_VOLUME        *Volume,
  IN    NTFS_IFILE         *IFile,
  IN OUT UINTN             *BufferSize,
    OUT VOID               *Buffer,
  IN    BOOL               IsCountDirSize
  );

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
  IN NTFS_VOLUME         *Volume,
  IN CHAR16             *Name
  );

//
// Misc.c
//

/**

  Lock the volume.

**/
VOID
NtfsAcquireLock (
  VOID
  );

/**

  Unlock the volume.

**/
VOID
NtfsReleaseLock (
  VOID
  );

/**

  Lock the volume.
  If the lock is already in the acquired state, then EFI_ACCESS_DENIED is returned.
  Otherwise, EFI_SUCCESS is returned.

  @retval EFI_SUCCESS           - The volume is locked.
  @retval EFI_ACCESS_DENIED     - The volume could not be locked because it is already locked.

**/
EFI_STATUS
NtfsAcquireLockOrFail (
  VOID
  );

/**

  Free volume structure (including the contents of directory cache and disk cache).

  @param  Volume                - The volume structure to be freed.

**/
VOID
NtfsFreeVolume (
  IN NTFS_VOLUME        *Volume
  );

/**

  Check whether a time is valid.

  @param  Time                  - The time of EFI_TIME.

  @retval TRUE                  - The time is valid.
  @retval FALSE                 - The time is not valid.

**/
BOOLEAN
NtfsIsValidTime (
  IN EFI_TIME           *Time
  );

/**

  Translate Fat time to EFI time.

  @param  FTime                 - The time of FAT_DATE_TIME.
  @param  ETime                 - The time of EFI_TIME..

**/
VOID
NtfsNtfsTimeToEfiTime (
  IN NTFS_DATE_TIME     *FTime,
  OUT EFI_TIME          *ETime
  );

/*++

Routine Description:
  Converts ASCII characters to Unicode.

Arguments:
  UnicodeStr - the Unicode string to be written to. The buffer must be large enough.
  AsciiStr   - The ASCII string to be converted.

Returns:
  The address to the Unicode string - same as UnicodeStr.

--*/
CHAR16 *
Ascii2Unicode (
  OUT CHAR16         *UnicodeStr,
  IN  CHAR8          *AsciiStr
  );

/*++

Routine Description:
  Converts ASCII characters to Unicode.
  Assumes that the Unicode characters are only these defined in the ASCII set.

Arguments:
  AsciiStr   - The ASCII string to be written to. The buffer must be large enough.
  UnicodeStr - the Unicode string to be converted.

Returns:
  The address to the ASCII string - same as AsciiStr.

--*/
CHAR8 *
Unicode2Ascii (
  OUT CHAR8          *AsciiStr,
  IN  CHAR16         *UnicodeStr
  );

VOID
EfiTime2NtfsTime (
  IN  const EFI_TIME                   *Time,
  OUT sle64                            *sle_ntfs_clock
  );

VOID 
NtfsTime2EfiTime(
  IN  const sle64    sle_ntfs_clock, 
  OUT EFI_TIME       *EfiTime
  );

BOOL
IsDir(
  IN ntfs_inode *ni
);

/**
  Parses a normalized wide character path and returns a pointer to the entry
  following the last \.  If a \ is not found in the path the return value will
  be the same as the input value.  All error conditions return NULL.

  The behavior when passing in a path that has not been normalized is undefined.

  @param  Path - A pointer to a wide character string containing a path to a
                 directory or a file.

  @return Pointer to the file name or terminal directory.  NULL if an error is
          detected.
**/
CHAR16 *
GetFileNameFromPathW (
  const CHAR16   *Path
  );

CHAR8 *
GetFileNameFromPathA (
  const CHAR8   *Path
  );

int 
ntfs_sd_add_everyone (
    ntfs_inode *ni
  );

//
// UnicodeCollation.c
//
/**
  Initialize Unicode Collation support.

  It tries to locate Unicode Collation 2 protocol and matches it with current
  platform language code. If for any reason the first attempt fails, it then tries to
  use Unicode Collation Protocol.

  @param  AgentHandle          The handle used to open Unicode Collation (2) protocol.

  @retval EFI_SUCCESS          The Unicode Collation (2) protocol has been successfully located.
  @retval Others               The Unicode Collation (2) protocol has not been located.
  
**/
EFI_STATUS
InitializeUnicodeCollationSupport (
  IN EFI_HANDLE    AgentHandle
  );

/**
  Convert FAT string to unicode string.

  @param  FatSize               The size of FAT string.
  @param  Fat                   The FAT string.
  @param  String                The unicode string.

  @return None.

**/
VOID
NtfsNtfsToStr (
  IN UINTN              FatSize,
  IN CHAR8              *Fat,
  OUT CHAR16            *String
  );

/**
  Convert unicode string to Fat string.

  @param  String                The unicode string.
  @param  FatSize               The size of the FAT string.
  @param  Fat                   The FAT string.

  @retval TRUE                  Convert successfully.
  @retval FALSE                 Convert error.

**/
BOOLEAN
NtfsStrToNtfs (
  IN  CHAR16            *String,
  IN  UINTN             FatSize,
  OUT CHAR8             *Fat
  );

/**
  Lowercase a string

  @param  Str                   The string which will be lower-cased.

**/
VOID
NtfsStrLwr (
  IN CHAR16             *Str
  );

/**
  Uppercase a string.

  @param  Str                   The string which will be upper-cased.

**/
VOID
NtfsStrUpr (
  IN CHAR16             *Str
  );

/**
  Performs a case-insensitive comparison of two Null-terminated Unicode strings.

  @param  Str1                   A pointer to a Null-terminated Unicode string.
  @param  Str2                   A pointer to a Null-terminated Unicode string.

  @retval 0                    S1 is equivalent to S2.
  @retval >0                   S1 is lexically greater than S2.
  @retval <0                   S1 is lexically less than S2.
**/
INTN
NtfsStriCmp (
  IN CHAR16             *Str1,
  IN CHAR16             *Str2
  );

//
// Open.c
//

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
  IN NTFS_VOLUME        *Volume,
  OUT NTFS_IFILE        **PtrIFile
  );

//
// ntfsfix.c
//

int fix_mount(
  IN const char *name __attribute__((unused)), 
  IN ntfs_mount_flags flags __attribute__((unused))
  );

int check_alternate_boot(
  IN ntfs_volume *vol
  );

//
// OpenVolume.c
//

/**

  Implements Simple File System Protocol interface function OpenVolume().

  @param  This                  - Calling context.
  @param  File                  - the Root Directory of the volume.

  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.
  @retval EFI_VOLUME_CORRUPTED  - The FAT type is error.
  @retval EFI_SUCCESS           - Open the volume successfully.

**/
EFI_STATUS
EFIAPI
NtfsOpenVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE_PROTOCOL               **File
  );

extern EFI_DRIVER_BINDING_PROTOCOL     gNtfsDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL     gNtfsComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL    gNtfsComponentName2;
extern EFI_LOCK                        NtfsFsLock;
extern EFI_LOCK                        NtfsTaskLock;
extern EFI_FILE_PROTOCOL               NtfsFileInterface;

#endif
