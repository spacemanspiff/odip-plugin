/*
 * FFS plugin for Custom IOS.
 *
 * Copyright (C) 2010 Spaceman Spiff.
 * Copyright (C) 2009-2010 Waninkoko.
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

#ifndef __FAT_H_
#define __FAT_H_

#include "types.h"

#include <sys/stat.h>
#include <sys/statvfs.h>

/* FAT Module IOCTL commands */
#define IOCTL_FAT_OPEN          0x01
#define IOCTL_FAT_CLOSE         0x02
#define IOCTL_FAT_READ          0x03
#define IOCTL_FAT_WRITE         0x04
#define IOCTL_FAT_SEEK          0x05
#define IOCTL_FAT_MKDIR         0x06
#define IOCTL_FAT_MKFILE        0x07
#define IOCTL_FAT_READDIR       0x08
#define IOCTL_FAT_DELETE        0x09
#define IOCTL_FAT_DELETEDIR     0x0A
#define IOCTL_FAT_RENAME        0x0B
#define IOCTL_FAT_STAT          0x0C
#define IOCTL_FAT_VFSSTATS      0x0D
#define IOCTL_FAT_FILESTATS     0x0E

#define IOCTL_FAT_MOUNTSD       0xF0
#define IOCTL_FAT_UMOUNTSD      0xF1
#define IOCTL_FAT_MOUNTUSB      0xF2
#define IOCTL_FAT_UMOUNTUSB     0xF3

/* Filestats structure */
typedef struct {
	u32 file_length;
	u32 file_pos;
} fstats;

int FAT_Init(void);
int FAT_FileStats(int fd, void *filestat);
int FAT_VFSStats(const char *path, struct statvfs *vfsstats);
int FAT_Stat(const char *filename, struct stat *statdata);
int FAT_Rename(const char *oldname, const char *newname);
int FAT_DeleteDir(const char *filename);
int FAT_Delete(const char *filename);
int FAT_ReadDir(const char *dirpath, u32 *outbuf, u32 *outlen);
int FAT_MakeFile(const char *filename);
int FAT_MakeDir(const char *dirname);

#endif

