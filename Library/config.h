/*
 *
 * Copyright (c) 2007-2008 Jean-Pierre Andre
 *
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the NTFS-3G
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CONFIG_H
#define CONFIG_H

#define HAVE_STDIO_H
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_ERRNO_H
#define HAVE_SYS_STAT_H
#define HAVE_FCNTL_H
#define HAVE_SYS_TYPES_H
#define HAVE_STDDEF_H
#define HAVE_LOCALE_H
#define HAVE_TIME_H
#define HAVE_CTYPE_H
#define HAVE_LIMITS_H
#define HAVE_UNISTD_H
#define HAVE_STDARG_H
#define HAVE_STRSEP
#define HAVE_FFS

#define HAVE_DAEMON
#define __timespec_defined
#define HAVE_GETTIMEOFDAY

#define S_ISVTX       0001000
#define S_IFLNK       0120000

#undef linux

#define random rand
#define srandom srand

#endif /* CONFIG_H */

