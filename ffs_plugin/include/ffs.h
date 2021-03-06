/*
 * FFS plugin for Custom IOS.
 *
 * Copyright (C) 2010 Spaceman Spiff.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FFS_H__
#define __FFS_H__

#define FFS_IOCTL_FORMAT	0x01
#define FFS_IOCTL_GETSTATS	0x02
#define FFS_IOCTL_CREATEDIR	0x03
#define FFS_IOCTL_READDIR	0x04
#define FFS_IOCTL_SETATTR	0x05
#define FFS_IOCTL_GETATTR	0x06
#define FFS_IOCTL_DELETE	0x07
#define FFS_IOCTL_RENAME	0x08
#define FFS_IOCTL_CREATEFILE	0x09
#define FFS_IOCTL_UNKNOWN_0A	0x0A
#define FFS_IOCTL_GETFILESTATS	0x0B
#define FFS_IOCTL_GETUSAGE	0x0C
#define FFS_IOCTL_SHUTDOWN	0x0D

#define FFS_IOCTL_SETNANDEMULATION 0x64

#endif
