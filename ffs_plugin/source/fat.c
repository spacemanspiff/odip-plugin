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
#define FAT_DATA_SIZE 	(sizeof(fat_data)) //0x140

#define FILENAME_SIZE 		0x60
#define STAT_DATA_SIZE		0x44       //(sizeof(struct stat))
#define VFSSTAT_DATA_SIZE	0x2C
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
			char data[STAT_DATA_SIZE];  // struct stat data_stat;
		} stat;
		struct {
			char filename[FILENAME_SIZE];
			char data[VFSSTAT_DATA_SIZE]; // struct vfsstat vfs_stat;
		} vfsstat;
		struct {
			char data[FILESTAT_DATA_SIZE];
		} filestats;
	};
} ATTRIBUTE_PACKED fat_data;

int FAT_Init(void)
{
	s32 *fathandleptr = (s32 *) FFS_FAT_HANDLE_PTR;
	void *fatdataptr = (void *) FFS_FAT_DATA_PTR;

	if (!fathandleptr) {
		if (!fatdataptr) 
			fatdataptr = os_heap_alloc_aligned(0, FAT_DATA_SIZE, FAT_DATA_ALIGN);
		s32 res = os_open("fat", IPC_OPEN_NONE);
		*fathandleptr = res;

		if (res <= 0)        // ERROR ?
			return res;
	}
	return 0;
}

int FAT_FileStats(int fd, void *filestat)
{
	int res;

	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;
	

	fatdata->vector[0].data = fatdata->filestats.data;
	fatdata->vector[0].len = FILESTAT_DATA_SIZE;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	res = os_ioctlv(fd, IOCTL_FAT_FILESTATS, 0, 1, fatdata->vector);

	if (res >= 0){
		FFS_Memcpy(filestat, fatdata->filestats.data, FILESTAT_DATA_SIZE);
	}
	return res;
}

int FAT_VFSStats(const char *path, struct statvfs *vfsstats)
{
	int res;
	s32 *fathandle = (s32 *) FFS_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;

	FFS_Strcpy(fatdata->vfsstat.filename, path);

	fatdata->vector[0].data = fatdata->vfsstat.filename;
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	fatdata->vector[1].data = fatdata->vfsstat.data;
	fatdata->vector[1].len = VFSSTAT_DATA_SIZE;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	res = os_ioctlv(*fathandle, IOCTL_FAT_VFSSTATS, 1, 1, fatdata->vector);
	
	if (res >= 0) 
		FFS_Memcpy(vfsstats,fatdata->vfsstat.data, VFSSTAT_DATA_SIZE);

	return res;
}

int FAT_Stat(const char *filename, struct stat *statdata)
{
	int res;
	s32 *fathandle = (s32 *) FFS_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;

	FFS_Strcpy(fatdata->stat.filename, filename);

	fatdata->vector[0].data = fatdata->stat.filename;
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	fatdata->vector[1].data = fatdata->stat.data;
	fatdata->vector[1].len = STAT_DATA_SIZE;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	res = os_ioctlv(*fathandle, IOCTL_FAT_STAT, 1, 1, fatdata->vector);
	
	if (res >= 0) {
		if (statdata)
			FFS_Memcpy(statdata,fatdata->stat.data, STAT_DATA_SIZE);
	}
	return res;
}

int FAT_Rename(const char *oldname, const char *newname)
{
	s32 *fathandle = (s32 *) FFS_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;

	FFS_Strcpy(fatdata->rename.oldfilename, oldname);
	FFS_Strcpy(fatdata->rename.newfilename, newname);

	fatdata->vector[0].data = fatdata->rename.oldfilename;
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	fatdata->vector[1].data = fatdata->rename.newfilename;
	fatdata->vector[1].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	return os_ioctlv(*fathandle, IOCTL_FAT_RENAME, 2, 0, fatdata->vector);
}

int FAT_DeleteDir(const char *filename)
{
	s32 *fathandle = (s32 *) FFS_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;

	FFS_Strcpy(fatdata->basic.filename, filename);

	fatdata->vector[0].data = fatdata->basic.filename;
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	return os_ioctlv(*fathandle, IOCTL_FAT_DELETEDIR, 1, 0, fatdata->vector);
}

int FAT_Delete(const char *filename)
{
	s32 *fathandle = (s32 *) FFS_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;

	FFS_Strcpy(fatdata->basic.filename, filename);

	fatdata->vector[0].data = fatdata->basic.filename;
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	return os_ioctlv(*fathandle, IOCTL_FAT_DELETE, 1, 0, fatdata->vector);
}

int FAT_ReadDir(const char *dirpath, u32 *outbuf, u32 *outlen)
{
	int ret;
	
	u32 param;

	s32 *fathandle = (s32 *) FFS_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;

	FFS_Strcpy(fatdata->basic.filename, dirpath);

	fatdata->vector[0].data = &fatdata->basic.filename;
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	fatdata->vector[1].data = (u32 *) fatdata->readdir.outlen;
	fatdata->vector[1].len = 4; // sizeof(int) ?

	if (outbuf) {
		fatdata->readdir.outlen = *outlen;

		fatdata->vector[2].data = outbuf;
		fatdata->vector[2].len = (*outlen) << 8; 

		fatdata->vector[3].data = &fatdata->readdir.outlen;
		fatdata->vector[3].len = 4; // sizeof(int) ?
		param = 2;
	} else
		param = 1;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	ret = os_ioctlv(*fathandle, IOCTL_FAT_READDIR, param, param, fatdata->vector);
	if (ret >= 0) {
		*outlen = fatdata->readdir.outlen;
	}
	return ret;
}

int FAT_MakeFile(const char *filename)
{
	s32 *fathandle = (s32 *) FFS_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;

	FFS_Strcpy(fatdata->basic.filename, filename);

	fatdata->vector[0].data = fatdata->basic.filename;
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	return os_ioctlv(*fathandle, IOCTL_FAT_MKFILE, 1, 0, fatdata->vector);
}


int FAT_MakeDir(const char *dirname)
{
	s32 *fathandle = (s32 *) FFS_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) FFS_FAT_DATA_PTR;

	FFS_Strcpy(fatdata->basic.filename, dirname);

	fatdata->vector[0].data = fatdata->basic.filename;
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	return os_ioctlv(*fathandle, IOCTL_FAT_MKDIR, 1, 0, fatdata->vector);
}
