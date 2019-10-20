/** @file
  Miscellaneous functions.

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

  Lock the volume.

**/
VOID
NtfsAcquireLock (
  VOID
  )
{
  EfiAcquireLock (&NtfsFsLock);
}

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
  )
{
  return EfiAcquireLockOrFail (&NtfsFsLock);
}

/**

  Unlock the volume.

**/
VOID
NtfsReleaseLock (
  VOID
  )
{
  EfiReleaseLock (&NtfsFsLock);
}

/**

  Free volume structure (including the contents of directory cache and disk cache).

  @param  Volume                - The volume structure to be freed.

**/
VOID
NtfsFreeVolume (
  IN NTFS_VOLUME       *Volume
  )
{
  FreePool (Volume);
}

/**

  Translate EFI time to FAT time.

  @param  ETime                 - The time of EFI_TIME.
  @param  FTime                 - The time of FAT_DATE_TIME.

**/
/*VOID
FatEfiTimeToFatTime (
  IN  EFI_TIME        *ETime,
  OUT FAT_DATE_TIME   *FTime
  )
{
  //
  // ignores timezone info in source ETime
  //
  if (ETime->Year > 1980) {
    FTime->Date.Year = (UINT16) (ETime->Year - 1980);
  }

  if (ETime->Year >= 1980 + FAT_MAX_YEAR_FROM_1980) {
    FTime->Date.Year = FAT_MAX_YEAR_FROM_1980;
  }

  FTime->Date.Month         = ETime->Month;
  FTime->Date.Day           = ETime->Day;
  FTime->Time.Hour          = ETime->Hour;
  FTime->Time.Minute        = ETime->Minute;
  FTime->Time.DoubleSecond  = (UINT16) (ETime->Second / 2);
}*/

/**

  Translate Fat time to EFI time.

  @param  FTime                 - The time of FAT_DATE_TIME.
  @param  ETime                 - The time of EFI_TIME..

**/
VOID
NtfsNtfsTimeToEfiTime (
  IN  NTFS_DATE_TIME    *FTime,
  OUT EFI_TIME          *ETime
  )
{
  ETime->Year       = (UINT16) (FTime->Date.Year + 1980);
  ETime->Month      = (UINT8) FTime->Date.Month;
  ETime->Day        = (UINT8) FTime->Date.Day;
  ETime->Hour       = (UINT8) FTime->Time.Hour;
  ETime->Minute     = (UINT8) FTime->Time.Minute;
  ETime->Second     = (UINT8) (FTime->Time.DoubleSecond * 2);
  ETime->Nanosecond = 0;
  ETime->TimeZone   = EFI_UNSPECIFIED_TIMEZONE;
  ETime->Daylight   = 0;
}

/**

  Get Current FAT time.

  @param  FatNow                - Current FAT time.

**/
/*VOID
FatGetCurrentFatTime (
  OUT FAT_DATE_TIME   *FatNow
  )
{
  EFI_STATUS Status;
  EFI_TIME   Now;

  Status = gRT->GetTime (&Now, NULL);
  if (!EFI_ERROR (Status)) {
    FatEfiTimeToFatTime (&Now, FatNow);
  } else {
    ZeroMem (&Now, sizeof (EFI_TIME));
    Now.Year = 1980;
    Now.Month = 1;
    Now.Day = 1;
    FatEfiTimeToFatTime (&Now, FatNow);
  }
}*/

/**

  Check whether a time is valid.

  @param  Time                  - The time of EFI_TIME.

  @retval TRUE                  - The time is valid.
  @retval FALSE                 - The time is not valid.

**/
BOOLEAN
NtfsIsValidTime (
  IN EFI_TIME         *Time
  )
{
  UINTN         Day;
  BOOLEAN       ValidTime;
  UINT8         mMonthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  ValidTime = TRUE;

  //
  // Check the fields for range problems
  // Fat can only support from 1980
  //
  if (Time->Year < 1980 ||
      Time->Month < 1 ||
      Time->Month > 12 ||
      Time->Day < 1 ||
      Time->Day > 31 ||
      Time->Hour > 23 ||
      Time->Minute > 59 ||
      Time->Second > 59 ||
      Time->Nanosecond > 999999999
      ) {

    ValidTime = FALSE;

  } else {
    //
    // Perform a more specific check of the day of the month
    //
    Day = mMonthDays[Time->Month - 1];
    if (Time->Month == 2 && IS_LEAP_YEAR (Time->Year)) {
      Day += 1;
      //
      // 1 extra day this month
      //
    }
    if (Time->Day > Day) {
      ValidTime = FALSE;
    }
  }

  return ValidTime;
}

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
  )
{
  CHAR16  *Str;

  Str = UnicodeStr;

  while (TRUE) {

    *(UnicodeStr++) = (CHAR16) *AsciiStr;

    if (*(AsciiStr++) == '\0') {
      return Str;
    }
  }
}

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
  )

{
  CHAR8 *Str;

  Str = AsciiStr;

  while (TRUE) {

    *AsciiStr = (CHAR8) *(UnicodeStr++);

    if (*(AsciiStr++) == '\0') {
      return Str;
    }
  }
}

#define BASE_YEAR                           1601
#define BASE_MONTH                          1
#define BASE_DAY                            1

#define DAYS_PER_YEAR                       365
#define HOURS_PER_DAY                       24
#define MINUTES_PER_HOUR                    60
#define SECONDS_PER_MINUTE                  60
#define SECONDS_PER_DAY                     86400
#define SECONDS_PER_HOUR                    3600

VOID
EfiTime2NtfsTime (
  IN  const EFI_TIME                   *Time,
  OUT sle64                            *sle_ntfs_clock
  )
{
  UINT16                                Year;
  UINT16                                AddedDays;
  UINT8                                 Month;
  UINT32                                DaysOfMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  //
  // Find number of leap years
  //
  AddedDays = 0;
  for (Year = BASE_YEAR; Year < Time->Year; ++Year) {
    if (IS_LEAP_YEAR (Year)) {
      ++AddedDays;
    }
  }

  //
  // Number of days of complete years (include all leap years)
  //
  *sle_ntfs_clock = (Time->Year - BASE_YEAR) * DAYS_PER_YEAR;
  *sle_ntfs_clock += AddedDays;

  for (Month = 0; Month < Time->Month - BASE_MONTH; ++Month) {
    *sle_ntfs_clock += DaysOfMonth[Month];
  }
  *sle_ntfs_clock += Time->Day - BASE_DAY;

  //
  // Check this Feb. is 28 days or 29 days
  //
  if (IS_LEAP_YEAR (Time->Year) && Time->Month > 2) {
    *sle_ntfs_clock += 1;
  }

  //
  // Convert days to seconds
  //
  *sle_ntfs_clock *= SECONDS_PER_DAY;

  //
  // Add rest seconds
  //
  *sle_ntfs_clock += (Time->Hour * SECONDS_PER_HOUR) +
                (Time->Minute * SECONDS_PER_MINUTE) +
                Time->Second;

  *sle_ntfs_clock = *sle_ntfs_clock * 10000000LL + Time->Nanosecond / 100;
}

VOID 
NtfsTime2EfiTime(
  IN  const sle64    sle_ntfs_clock, 
  OUT EFI_TIME       *EfiTime
  )
{
	long long stamp;
	u32 days;
	u32 seconds;
	u64 nanoseconds;
	unsigned int year;
	int mon;
	int cnt;

	stamp = sle64_to_cpu(sle_ntfs_clock);
	days = (stamp/(SECONDS_PER_DAY*10000000LL)) & 0x7ffff;
	seconds = ((stamp/10000000LL)%SECONDS_PER_DAY) & 0x1ffff;
	nanoseconds = stamp % 10000000 * 100;
	year = BASE_YEAR;
				/* periods of 400 years */
	cnt = days/146097;
	days -= 146097*cnt;
	year += 400*cnt;
				/* periods of 100 years */
	cnt = (3*days + 3)/109573;
	days -= 36524*cnt;
	year += 100*cnt;
				/* periods of 4 years */
	cnt = days/1461;
	days -= 1461*cnt;
	year += 4*cnt;
				/* periods of a single year */
	cnt = (3*days + 3)/1096;
	days -= DAYS_PER_YEAR*cnt;
	year += cnt;

	if ((!(year % 100) ? (year % 400) : (year % 4))
		&& (days > 58)) days++;
	if (days > 59) {
		mon = (5*days + 161)/153;
		days -= (153*mon - 162)/5;
	} else {
		mon = days/31 + 1;
		days -= 31*(mon - 1) - 1;
	}

	EfiTime->Year = year;
	EfiTime->Month = mon;
	EfiTime->Day = days;
	EfiTime->Hour = seconds / SECONDS_PER_HOUR;
	EfiTime->Minute = seconds / SECONDS_PER_MINUTE % MINUTES_PER_HOUR;
	EfiTime->Second = seconds % SECONDS_PER_MINUTE;
	EfiTime->Nanosecond = nanoseconds;
}

BOOL
IsDir(
  IN ntfs_inode *ni
)
{
  ntfs_attr_search_ctx  *ctx;
  ATTR_RECORD           *rec;
  FILE_NAME_ATTR        *attr;

  ctx = ntfs_attr_get_search_ctx(ni, NULL);
  if (!ctx)
    return FALSE;
	
  while ((rec = find_attribute(AT_FILE_NAME, ctx))) {
    /* We know this will always be resident. */
    attr = (FILE_NAME_ATTR *) ((char *) rec + le16_to_cpu(rec->value_offset));
	if (attr->file_attributes & FILE_ATTR_I30_INDEX_PRESENT) {
      return TRUE;
    }
  }

  return FALSE;
}

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
  )
{
  CHAR16   *Tail;

  if (Path == NULL) {
    return NULL;
  }

  Tail = wcsrchr(Path, L'\\');
  if(Tail == NULL) {
    Tail = (CHAR16 *) Path;
  } else {
    // Move to the next character after the '\\' to get the file name.
    Tail++;
  }

  return Tail;
}

CHAR8 *
GetFileNameFromPathA (
  const CHAR8   *Path
  )
{
  CHAR8   *Tail;

  if (Path == NULL) {
    return NULL;
  }

  Tail = strrchr(Path, L'\\');
  if(Tail == NULL) {
    Tail = (CHAR8 *) Path;
  } else {
    // Move to the next character after the '\\' to get the file name.
    Tail++;
  }

  return Tail;
}

int 
ntfs_sd_add_everyone
  (
    ntfs_inode *ni
  )
{
	/* JPA SECURITY_DESCRIPTOR_ATTR *sd; */
	SECURITY_DESCRIPTOR_RELATIVE *sd;
	ACL *acl;
	ACCESS_ALLOWED_ACE *ace;
	SID *sid;
	int ret, sd_len;
	
	/* Create SECURITY_DESCRIPTOR attribute (everyone has full access). */
	/*
	 * Calculate security descriptor length. We have 2 sub-authorities in
	 * owner and group SIDs, but structure SID contain only one, so add
	 * 4 bytes to every SID.
	 */
	sd_len = sizeof(SECURITY_DESCRIPTOR_ATTR) + 2 * (sizeof(SID) + 4) +
		sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE); 
	sd = (SECURITY_DESCRIPTOR_RELATIVE*)ntfs_calloc(sd_len);
	if (!sd)
		return -1;
	
	sd->revision = SECURITY_DESCRIPTOR_REVISION;
	sd->control = SE_DACL_PRESENT | SE_SELF_RELATIVE;
	
	sid = (SID*)((u8*)sd + sizeof(SECURITY_DESCRIPTOR_ATTR));
	sid->revision = SID_REVISION;
	sid->sub_authority_count = 2;
	sid->sub_authority[0] = const_cpu_to_le32(SECURITY_BUILTIN_DOMAIN_RID);
	sid->sub_authority[1] = const_cpu_to_le32(DOMAIN_ALIAS_RID_ADMINS);
	sid->identifier_authority.value[5] = 5;
	sd->owner = cpu_to_le32((u8*)sid - (u8*)sd);
	
	sid = (SID*)((u8*)sid + sizeof(SID) + 4); 
	sid->revision = SID_REVISION;
	sid->sub_authority_count = 2;
	sid->sub_authority[0] = const_cpu_to_le32(SECURITY_BUILTIN_DOMAIN_RID);
	sid->sub_authority[1] = const_cpu_to_le32(DOMAIN_ALIAS_RID_ADMINS);
	sid->identifier_authority.value[5] = 5;
	sd->group = cpu_to_le32((u8*)sid - (u8*)sd);
	
	acl = (ACL*)((u8*)sid + sizeof(SID) + 4);
	acl->revision = ACL_REVISION;
	acl->size = const_cpu_to_le16(sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE));
	acl->ace_count = const_cpu_to_le16(1);
	sd->dacl = cpu_to_le32((u8*)acl - (u8*)sd);
	
	ace = (ACCESS_ALLOWED_ACE*)((u8*)acl + sizeof(ACL));
	ace->type = ACCESS_ALLOWED_ACE_TYPE;
	ace->flags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
	ace->size = const_cpu_to_le16(sizeof(ACCESS_ALLOWED_ACE));
	ace->mask = const_cpu_to_le32(0x1f01ff); /* FIXME */
	ace->sid.revision = SID_REVISION;
	ace->sid.sub_authority_count = 1;
	ace->sid.sub_authority[0] = const_cpu_to_le32(0);
	ace->sid.identifier_authority.value[5] = 1;

	ret = ntfs_attr_add(ni, AT_SECURITY_DESCRIPTOR, AT_UNNAMED, 0, (u8*)sd,
			    sd_len);
	if (ret)
		ntfs_log_perror("Failed to add initial SECURITY_DESCRIPTOR");
	
	free(sd);
	return ret;
}

