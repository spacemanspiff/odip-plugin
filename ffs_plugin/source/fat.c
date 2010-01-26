/*
 * FFS plugin for Custom IOS.
 *
 * Copyright (C) 2010 Spaceman Spiff
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
#include "fat.h"
#include "ipc.h"
#include "ffs.h"

#include "tools.h"

#include "syscalls.h"

#include <sys/stat.h>
#include <sys/statvfs.h>

#define FAT_DATA_ALIGN	0x20
#define FAT_DATA_SIZE 	(sizeof(fat_data)) 

#define FILENAME_SIZE 		0x60
#define STAT_DATA_SIZE		(sizeof(struct stat))
#define VFSSTAT_DATA_SIZE	(sizeof(struct statvfs))
#define FILESTAT_DATA_SIZE	0x08

// Make Aligned...
typedef struct {
	ioctlv vector[8];
	union {
		struct {
			char filename[FILENAME_SIZE];
		} basic;
		struct {
			char filename[FILENAME_SIZE];
			int outlen;
		} readdir;
		struct {
			char oldfilename[FILENAME_SIZE];
			char newfilename[FILENAME_SIZE];
		} rename;
		struct {
			char filename[FILENAME_SIZE];
			struct stat data_stat;
		} stat;
		struct {
			char filename[FILENAME_SIZE];
			struct statvfs stat_vfs;
		} statvfs;
		struct {
			char data[FILESTAT_DATA_SIZE];
		} filestats;
	};
} ATTRIBUTE_PACKED fat_data;

static fat_data *fatDataPtr = NULL;

s32 fatHandle = 0;

int FAT_Init(void)
{
	if (!fatHandle) {
		if (!fatDataPtr) 
			fatDataPtr = os_heap_alloc_aligned(0, FAT_DATA_SIZE, FAT_DATA_ALIGN);

		s32 res = os_open("fat", IPC_OPEN_NONE);
		fatHandle = res;

		if (res < 0)        // ERROR ?
			return res;
	}
	return 0;
}

int FAT_FileStats(int fd, void *filestat)
{
	int res;

	fatDataPtr->vector[0].data = fatDataPtr->filestats.data;
	fatDataPtr->vector[0].len = FILESTAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_FILESTATS, 0, 1, fatDataPtr->vector);

	if (res >= 0){
		FFS_Memcpy(filestat, fatDataPtr->filestats.data, FILESTAT_DATA_SIZE);
	}
	return res;
}

int FAT_VFSStats(const char *path, struct statvfs *vfsstats)
{
	int res;

	FFS_Strcpy(fatDataPtr->statvfs.filename, path);

	fatDataPtr->vector[0].data = fatDataPtr->statvfs.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = (u32 *) &fatDataPtr->statvfs.stat_vfs;
	fatDataPtr->vector[1].len = VFSSTAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_VFSSTATS, 1, 1, fatDataPtr->vector);
	
	if (res >= 0) 
		FFS_Memcpy(vfsstats,(void *) &fatDataPtr->statvfs.stat_vfs, VFSSTAT_DATA_SIZE);

	return res;
}

int FAT_Stat(const char *filename, struct stat *statdata)
{
	int res;

	FFS_Strcpy(fatDataPtr->stat.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->stat.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = (u32 *) &fatDataPtr->stat.data_stat;
	fatDataPtr->vector[1].len = STAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_STAT, 1, 1, fatDataPtr->vector);
	
	if (res >= 0) {
		if (statdata)
			FFS_Memcpy(statdata,(void *) &fatDataPtr->stat.data_stat, STAT_DATA_SIZE);
	}
	return res;
}

int FAT_Rename(const char *oldname, const char *newname)
{
	FFS_Strcpy(fatDataPtr->rename.oldfilename, oldname);
	FFS_Strcpy(fatDataPtr->rename.newfilename, newname);

	fatDataPtr->vector[0].data = fatDataPtr->rename.oldfilename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = fatDataPtr->rename.newfilename;
	fatDataPtr->vector[1].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_RENAME, 2, 0, fatDataPtr->vector);
}

int FAT_DeleteDir(const char *filename)
{
	FFS_Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_DELETEDIR, 1, 0, fatDataPtr->vector);
}

int FAT_Delete(const char *filename)
{
	FFS_Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_DELETE, 1, 0, fatDataPtr->vector);
}

int FAT_ReadDir(const char *dirpath, u32 *outbuf, u32 *outlen)
{
	int ret;
	
	u32 io;

	FFS_Strcpy(fatDataPtr->basic.filename, dirpath);

	fatDataPtr->vector[0].data = &fatDataPtr->readdir.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = (u32 *) fatDataPtr->readdir.outlen;
	fatDataPtr->vector[1].len = sizeof(int);

	if (outbuf) {
		fatDataPtr->readdir.outlen = *outlen;

		fatDataPtr->vector[2].data = outbuf;
		fatDataPtr->vector[2].len = (*outlen) << 8;  // MAX_FILENAME_SIZE * (*outlen)

		fatDataPtr->vector[3].data = &fatDataPtr->readdir.outlen;
		fatDataPtr->vector[3].len = sizeof(int);
		io = 2;
	} else
		io = 1;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	ret = os_ioctlv(fatHandle, IOCTL_FAT_READDIR, io, io, fatDataPtr->vector);
	if (ret >= 0) {
		*outlen = fatDataPtr->readdir.outlen;
	}
	return ret;
}

int FAT_MakeFile(const char *filename)
{
	FFS_Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_MKFILE, 1, 0, fatDataPtr->vector);
}


int FAT_MakeDir(const char *dirname)
{
	FFS_Strcpy(fatDataPtr->basic.filename, dirname);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_MKDIR, 1, 0, fatDataPtr->vector);
}
