/*
 * ES plugin for Custom IOS.
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
#include "tools.h"
#include "syscalls.h"

static fat_data *fatDataPtr = NULL;
static s32 fatHandle = 0;

int FAT_Init(void)
{
	if (!fatHandle) {
		if (!fatDataPtr) 
			fatDataPtr = os_heap_alloc_aligned(0, FAT_DATA_SIZE, FAT_DATA_ALIGN);
		s32 res = os_open("fat", IPC_OPEN_NONE);

		if (res <= 0)        // ERROR ?
			return res;

		fatHandle = res;
	}
	return 0;
}

int FAT_Seek(s32 cfd, u32 where, u32 whence)
{
	int res;

	fatDataPtr->seek.cfd = cfd;
	fatDataPtr->seek.where = where;
	fatDataPtr->seek.whence = whence;

	fatDataPtr->vector[0].data = &fatDataPtr->seek.cfd;
	fatDataPtr->vector[0].len = 4;

	fatDataPtr->vector[1].data = &fatDataPtr->seek.where;
	fatDataPtr->vector[1].len = 4;

	fatDataPtr->vector[2].data = &fatDataPtr->seek.whence;
	fatDataPtr->vector[2].len = 4;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_SEEK, 3, 0, fatDataPtr->vector);
	
	return res;
}

int FAT_Read(s32 cfd, u8 *data, u32 data_size)
{
	int res;

	fatDataPtr->read.cfd = cfd;
	fatDataPtr->vector[0].data = &fatDataPtr->read.cfd;
	fatDataPtr->vector[0].len = sizeof(s32);

	fatDataPtr->vector[1].data = data;
	fatDataPtr->vector[1].len = data_size;


	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_READ, 1, 1, fatDataPtr->vector);
	
	return res;
}

int FAT_Close(s32 cfd)
{
	int res;

	fatDataPtr->close.cfd = cfd;
	fatDataPtr->vector[0].data = &fatDataPtr->close.cfd; 
	fatDataPtr->vector[0].len = sizeof(s32);

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_CLOSE, 1, 0, fatDataPtr->vector);
	
	return res;
}

int FAT_Open(const char *filename, u32 mode)
{
	int res;

	Strcpy(fatDataPtr->open.filename, filename);
	fatDataPtr->open.mode = mode;

	fatDataPtr->vector[0].data = &fatDataPtr->open.filename; 
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = &fatDataPtr->open.mode; 
	fatDataPtr->vector[1].len = sizeof(int);

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_OPEN, 2, 0, fatDataPtr->vector);
	
	return res;
}

int FAT_FileStats(int fd, void *filestat)
{
	int res;

	fatDataPtr->vector[0].data = (u32 *) &fatDataPtr->filestats.data;
	fatDataPtr->vector[0].len = FILESTAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_FILESTATS, 0, 1, fatDataPtr->vector);

	if (res >= 0){
		Memcpy(filestat, &fatDataPtr->filestats.data, FILESTAT_DATA_SIZE);
	}
	return res;
}

int FAT_VFSStats(const char *path, struct statvfs *vfsstats)
{
	int res;

	Strcpy(fatDataPtr->vfsstat.filename, path);

	fatDataPtr->vector[0].data = fatDataPtr->vfsstat.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = (u32 *) &fatDataPtr->vfsstat.stat_vfs;
	fatDataPtr->vector[1].len = VFSSTAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_VFSSTATS, 1, 1, fatDataPtr->vector);
	
	if (res >= 0) 
		Memcpy(vfsstats,(void *) &fatDataPtr->vfsstat.stat_vfs, VFSSTAT_DATA_SIZE);

	return res;
}

int FAT_Stat(const char *filename, struct stat *statdata)
{
	int res;

	Strcpy(fatDataPtr->stat.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->stat.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = (u32 *) &fatDataPtr->stat.data_stat;
	fatDataPtr->vector[1].len = STAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_STAT, 1, 1, fatDataPtr->vector);
	
	if (res >= 0) {
		if (statdata)
			Memcpy(statdata,(void *) &fatDataPtr->stat.data_stat, STAT_DATA_SIZE);
	}
	return res;
}

int FAT_Rename(const char *oldname, const char *newname)
{
	Strcpy(fatDataPtr->rename.oldfilename, oldname);
	Strcpy(fatDataPtr->rename.newfilename, newname);

	fatDataPtr->vector[0].data = fatDataPtr->rename.oldfilename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = fatDataPtr->rename.newfilename;
	fatDataPtr->vector[1].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_RENAME, 2, 0, fatDataPtr->vector);
}

int FAT_DeleteDir(const char *filename)
{
	Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_DELETEDIR, 1, 0, fatDataPtr->vector);
}

int FAT_Delete(const char *filename)
{
	Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_DELETE, 1, 0, fatDataPtr->vector);
}

int FAT_ReadDir(const char *dirpath, u32 *outbuf, u32 *outlen)
{
	int ret;
	
	u32 io;

	Strcpy(fatDataPtr->basic.filename, dirpath);

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
	Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_MKFILE, 1, 0, fatDataPtr->vector);
}


int FAT_MakeDir(const char *dirname)
{
	Strcpy(fatDataPtr->basic.filename, dirname);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_MKDIR, 1, 0, fatDataPtr->vector);
}
