/*
 * win32_io.c - A stdio-like disk I/O implementation for low-level disk access
 *		on Win32.  Can access an NTFS volume while it is mounted.
 *		Originated from the Linux-NTFS project.
 *
 * Copyright (c) 2003-2004 Lode Leroy
 * Copyright (c) 2003-2006 Anton Altaparmakov
 * Copyright (c) 2004-2005 Yuval Fledel
 * Copyright (c) 2012-2014 Jean-Pierre Andre
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the NTFS-3G
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "Ntfs.h"
#include "device.h"

/**
 * ntfs_device_uefi_open - open a device
 * @dev:	a pointer to the NTFS_DEVICE to open
 * @flags:	unix open status flags
 *
 * @dev->d_name must hold the device name, the rest is ignored.
 * Supported flags are O_RDONLY, O_WRONLY and O_RDWR.
 *
 * If name is in format "(hd[0-9],[0-9])" then open a partition.
 * If name is in format "(hd[0-9])" then open a volume.
 * Otherwise open a file.
 */
static int ntfs_device_uefi_open(struct ntfs_device *dev, int flags)
{
//Print(L"ntfs_device_uefi_open\n");
	return 0;
}

/**
 * ntfs_device_uefi_seek - change current logical file position
 * @dev:	ntfs device obtained via ->open
 * @offset:	required offset from the whence anchor
 * @whence:	whence anchor specifying what @offset is relative to
 *
 * Return the new position on the volume on success and -1 on error with errno
 * set to the error code.
 *
 * @whence may be one of the following:
 *	SEEK_SET - Offset is relative to file start.
 *	SEEK_CUR - Offset is relative to current position.
 *	SEEK_END - Offset is relative to end of file.
 */
static s64 ntfs_device_uefi_seek(struct ntfs_device *dev, s64 offset,
		int whence)
{
	s64 abs_ofs;
	NTFS_VOLUME 		  *Volume = (NTFS_VOLUME *)dev->d_private;
	s64 part_length = Volume->BlockIo->Media->LastBlock * Volume->BlockIo->Media->BlockSize;

	ntfs_log_trace("seek offset = 0x%llx, whence = %d.\n", offset, whence);
	switch (whence) {
	case SEEK_SET:
		abs_ofs = offset;
		break;
	case SEEK_CUR:
		abs_ofs = Volume->Pos + offset;
		break;
	case SEEK_END:
		/* End of partition != end of disk. */
		if (part_length == -1) {
			ntfs_log_trace("Position relative to end of disk not "
					"implemented.\n");
			errno = EOPNOTSUPP;
			return -1;
		}
		abs_ofs = part_length + offset;
		break;
	default:
		ntfs_log_trace("Wrong mode %d.\n", whence);
		errno = EINVAL;
		return -1;
	}
	if ((abs_ofs < 0)
	    || (abs_ofs > part_length)) {
		ntfs_log_trace("Seeking outsize seekable area.\n");
		errno = EINVAL;
		return -1;
	}
	Volume->Pos = abs_ofs;
	return abs_ofs;
}


/**
 * ntfs_device_uefi_read - read bytes from an ntfs device
 * @dev:	ntfs device obtained via ->open
 * @b:		pointer to where to put the contents
 * @count:	how many bytes should be read
 *
 * On success returns the number of bytes actually read (can be < @count).
 * On error returns -1 with errno set.
 */
static s64 ntfs_device_uefi_read(struct ntfs_device *dev, void *b, s64 count)
{
	//Print(L"ntfs_device_uefi_read\n");

	return 0;
}

/**
 * ntfs_device_uefi_close - close an open ntfs deivce
 * @dev:	ntfs device obtained via ->open
 *
 * Return 0 if o.k.
 *	 -1 if not, and errno set.  Note if error fd->vol_handle is trashed.
 */
static int ntfs_device_uefi_close(struct ntfs_device *dev)
{
	//Print(L"ntfs_device_uefi_close\n");

	return 0;
}

/**
 * ntfs_device_uefi_sync - flush write buffers to disk
 * @dev:	ntfs device obtained via ->open
 *
 * Return 0 if o.k.
 *	 -1 if not, and errno set.
 *
 * Note: Volume syncing works differently in windows.
 *	 Disk cannot be synced in windows.
 */
static int ntfs_device_uefi_sync(struct ntfs_device *dev)
{
	//Print(L"ntfs_device_uefi_sync\n");

	return 0;
}

/**
 * ntfs_device_uefi_write - write bytes to an ntfs device
 * @dev:	ntfs device obtained via ->open
 * @b:		pointer to the data to write
 * @count:	how many bytes should be written
 *
 * On success returns the number of bytes actually written.
 * On error returns -1 with errno set.
 */
static s64 ntfs_device_uefi_write(struct ntfs_device *dev, const void *b,
		s64 count)
{
	//Print(L"ntfs_device_uefi_write\n");

	return 0;
}

/**
 * ntfs_device_uefi_stat - get a unix-like stat structure for an ntfs device
 * @dev:	ntfs device obtained via ->open
 * @buf:	pointer to the stat structure to fill
 *
 * Note: Only st_mode, st_size, and st_blocks are filled.
 *
 * Return 0 if o.k.
 *	 -1 if not and errno set. in this case handle is trashed.
 */
static int ntfs_device_uefi_stat(struct ntfs_device *dev, struct stat *buf)
{
	//Print(L"ntfs_device_uefi_stat\n");

	return 0;
}

static int ntfs_device_uefi_ioctl(struct ntfs_device *dev, int request,
		void *argp)
{
#if defined(BLKGETSIZE) | defined(BLKGETSIZE64)
	NTFS_VOLUME 		  *Volume = (NTFS_VOLUME *)dev->d_private;
#endif
	s64 part_length = Volume->BlockIo->Media->LastBlock * Volume->BlockIo->Media->BlockSize;

	ntfs_log_trace("win32_ioctl(%d) called.\n", request);
	switch (request) {
#if defined(BLKGETSIZE)
	case BLKGETSIZE:
		ntfs_log_debug("BLKGETSIZE detected.\n");
		if (part_length >= 0) {
			*(int *)argp = (int)(part_length / 512);
			return 0;
		}
		errno = EOPNOTSUPP;
		return -1;
#endif
#if defined(BLKGETSIZE64)
	case BLKGETSIZE64:
		ntfs_log_debug("BLKGETSIZE64 detected.\n");
		if (part_length >= 0) {
			*(s64 *)argp = part_length;
			return 0;
		}
		errno = EOPNOTSUPP;
		return -1;
#endif
#ifdef HDIO_GETGEO
	case HDIO_GETGEO:
		ntfs_log_debug("HDIO_GETGEO detected.\n");
		/* Nothing to do on UEFI. */
		return 0;
#endif
#ifdef BLKSSZGET
	case BLKSSZGET:
		ntfs_log_debug("BLKSSZGET detected.\n");
		if (Volume && Volume->VolInfo) {
			*(int*)argp = Volume->BlockIo->Media->BlockSize;
			return (0);
		} 
#endif
#ifdef BLKBSZSET
	case BLKBSZSET:
		ntfs_log_debug("BLKBSZSET detected.\n");
		/* Nothing to do on UEFI. */
		return 0;
#endif
	default:
		ntfs_log_debug("unimplemented ioctl %d.\n", request);
		errno = EOPNOTSUPP;
		return -1;
	}
}


static s64 ntfs_device_uefi_pread(struct ntfs_device *dev, void *b,
		s64 count, s64 offset)
{
	EFI_STATUS			  Status;
	NTFS_VOLUME           *Volume = (NTFS_VOLUME *)dev->d_private;

	Status	  = Volume->DiskIo->ReadDisk (Volume->DiskIo, Volume->MediaId, offset, count, b);
	if (EFI_ERROR (Status)) {
	  return -1;
	}
	
	return count;
}

static s64 ntfs_device_uefi_pwrite(struct ntfs_device *dev, const void *b,
		s64 count, s64 offset)
{
	EFI_STATUS			  Status;
	NTFS_VOLUME           *Volume = (NTFS_VOLUME *)dev->d_private;

	Status	  = Volume->DiskIo->WriteDisk (Volume->DiskIo, Volume->MediaId, offset, count, (VOID *)b);
	if (EFI_ERROR (Status)) {
	  return -1;
	}
	
	return count;
}

struct ntfs_device_operations ntfs_device_uefi_io_ops = {
	.open		= ntfs_device_uefi_open,
	.close		= ntfs_device_uefi_close,
	.seek		= ntfs_device_uefi_seek,
	.read		= ntfs_device_uefi_read,
	.write		= ntfs_device_uefi_write,
	.pread		= ntfs_device_uefi_pread,
	.pwrite		= ntfs_device_uefi_pwrite,
	.sync		= ntfs_device_uefi_sync,
	.stat		= ntfs_device_uefi_stat,
	.ioctl		= ntfs_device_uefi_ioctl
};
