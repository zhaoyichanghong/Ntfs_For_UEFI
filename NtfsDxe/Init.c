/** @file
  Initialization routines.

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
  IN  EFI_HANDLE                Handle,
  IN  EFI_DISK_IO_PROTOCOL      *DiskIo,
  IN  EFI_BLOCK_IO_PROTOCOL     *BlockIo
  )
{
  EFI_STATUS  Status;
  NTFS_VOLUME  *Volume;

  //
  // Allocate a volume structure
  //
  Volume = AllocateZeroPool (sizeof (NTFS_VOLUME));
  if (Volume == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the structure
  //
  Volume->Signature                   = NTFS_VOLUME_SIGNATURE;
  Volume->Handle                      = Handle;
  Volume->DiskIo                      = DiskIo;
  Volume->BlockIo                     = BlockIo;
  Volume->MediaId                     = BlockIo->Media->MediaId;
  Volume->ReadOnly                    = BlockIo->Media->ReadOnly;
  Volume->VolumeInterface.Revision    = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  Volume->VolumeInterface.OpenVolume  = NtfsOpenVolume;
  Volume->RefCount                    = 0;

  //
  // Check to see if there's a file system on the volume
  //
  Status = NtfsOpenDevice (Volume);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Install our protocol interfaces on the device's handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Volume->Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Volume->VolumeInterface,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Volume installed
  //
  DEBUG ((EFI_D_INIT, "Installed Ntfs filesystem on %p\n", Handle));
  Volume->Valid = TRUE;

Done:
  if (EFI_ERROR (Status)) {
    NtfsFreeVolume (Volume);
  }

  return Status;
}

/**

  Called by FatDriverBindingStop(), Abandon the volume.

  @param  Volume                - The volume to be abandoned.

  @retval EFI_SUCCESS           - Abandoned the volume successfully.
  @return Others                - Can not uninstall the protocol interfaces.

**/
EFI_STATUS
NtfsAbandonVolume (
  IN NTFS_VOLUME *Volume
  )
{
  EFI_STATUS  Status;
  BOOLEAN     LockedByMe;

  //
  // Uninstall the protocol interface.
  //
  if (Volume->Handle != NULL) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Volume->Handle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    &Volume->VolumeInterface,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  LockedByMe = FALSE;

  //
  // Acquire the lock.
  // If the caller has already acquired the lock (which
  // means we are in the process of some Fat operation),
  // we can not acquire again.
  //
  Status = NtfsAcquireLockOrFail ();
  if (!EFI_ERROR (Status)) {
    LockedByMe = TRUE;
  }

  Volume->Valid = FALSE;

  //
  // Release the lock.
  // If locked by me, this means DriverBindingStop is NOT
  // called within an on-going Fat operation, so we should
  // take responsibility to cleanup and free the volume.
  // Otherwise, the DriverBindingStop is called within an on-going
  // Fat operation, we shouldn't check reference, so just let outer
  // FatCleanupVolume do the task.
  //
  if (LockedByMe) {
    NtfsCleanupVolume (Volume);
  }

  return EFI_SUCCESS;
}

/**

  Detects FAT file system on Disk and set relevant fields of Volume.

  @param Volume                - The volume structure.

  @retval EFI_SUCCESS           - The Fat File System is detected successfully
  @retval EFI_UNSUPPORTED       - The volume is not FAT file system.
  @retval EFI_VOLUME_CORRUPTED  - The volume is corrupted.

**/
EFI_STATUS
NtfsOpenDevice (
  IN OUT NTFS_VOLUME           *Volume
  )
{
  EFI_STATUS            Status;
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  NTFS_BOOT_SECTOR		NtfsBs;

  //
  // Read the FAT_BOOT_SECTOR BPB info
  // This is the only part of FAT code that uses parent DiskIo,
  // Others use FatDiskIo which utilizes a Cache.
  //
  DiskIo  = Volume->DiskIo;
  Status  = DiskIo->ReadDisk (DiskIo, Volume->MediaId, 0, sizeof (NTFS_BOOT_SECTOR), &NtfsBs);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INIT, "FatOpenDevice: read of part_lba failed %r\n", Status));
    return Status;
  }

  if (!ntfs_boot_sector_is_ntfs(&NtfsBs)) {
	return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
