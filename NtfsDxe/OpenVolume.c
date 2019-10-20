/** @file
  OpenVolume() function of Simple File System Protocol.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ntfs.h"

#define DEVICE_NAME "Ntfs%d"

static ntfs_volume *NtfsMount(const char *name __attribute__((unused)),
		ntfs_mount_flags flags __attribute__((unused)), void *priv_data)
{
	struct ntfs_device *dev;
	ntfs_volume *vol;

	/* Allocate an ntfs_device structure. */
	dev = ntfs_device_alloc(name, 0, &ntfs_device_default_io_ops, priv_data);
	if (!dev)
		return NULL;
	/* Call ntfs_device_mount() to do the actual mount. */
	vol = ntfs_device_mount(dev, flags);
	if (!vol) {
		int eo = errno;
		ntfs_device_free(dev);
		errno = eo;
	} else {
		ntfs_create_lru_caches(vol);
	}
	
	return vol;
}

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
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL                **File
  )
{
  EFI_STATUS     Status;
  NTFS_VOLUME    *Volume;
  NTFS_IFILE     *IFile;
  CHAR8          DevName[8];

  Volume = VOLUME_FROM_VOL_INTERFACE (This);
  NtfsAcquireLock ();

  sprintf(DevName, DEVICE_NAME, Volume->MediaId);
  
  Print(L"NtfsOpenVolume+\n");

  Volume->VolInfo = NtfsMount(DevName, NTFS_MNT_FORENSIC, Volume);
  if (Volume->VolInfo == NULL) {
    if (fix_mount(DevName, NTFS_MNT_FORENSIC) < 0) {
      Status = EFI_UNSUPPORTED;
	  goto Done;
    }
	
	Volume->VolInfo = NtfsMount(DevName, NTFS_MNT_FORENSIC, Volume);
    if (Volume->VolInfo == NULL) {
      Status = EFI_UNSUPPORTED;
	  goto Done;
    }
	
	if (check_alternate_boot(Volume->VolInfo)) {
      Status = EFI_UNSUPPORTED;
	  goto Done;
	}

	if (ntfs_version_is_supported(Volume->VolInfo)) {
      Status = EFI_UNSUPPORTED;
      goto Done;
	}
  }
  NVolClearCaseSensitive(Volume->VolInfo);

  //
  // Open a new instance to the root
  //
  Status = NtfsAllocateIFile (Volume, &IFile);
  if (!EFI_ERROR (Status)) {
	IFile->IsRoot = TRUE;
    IFile->IsDir = TRUE;
    AsciiStrCpy(IFile->Path, "\\");
    *File = &IFile->Handle;
  }

Done:

  NtfsCleanupVolume (Volume);
  NtfsReleaseLock ();
  return Status;
}
