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
#include "ipc.h"
#include "es.h"

#include "tools.h"

#include "syscalls.h"

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
			char data[STAT_DATA_SIZE];  // struct stat data;
		} stat;
		struct {
			char filename[FILENAME_SIZE];
			char data[VFSSTAT_DATA_SIZE];
		} vfsstat;
		struct {
			char data[FILESTAT_DATA_SIZE];
		} filestats;
		struct {
			s32 ATTRIBUTE_ALIGN(32) cfd;
			u32 ATTRIBUTE_ALIGN(32) where;
			u32 ATTRIBUTE_ALIGN(32) whence;
		} seek;
		struct {
			s32 ATTRIBUTE_ALIGN(32) cfd;
		} read;
		struct {
			s32 ATTRIBUTE_ALIGN(32) cfd;
		} close;
		struct {
			char filename[FILENAME_SIZE];
			u32 mode;
		} open;
	};
} ATTRIBUTE_PACKED fat_data;

int FAT_Init(void)
{
	s32 *fathandleptr = (s32 *) ES_FAT_HANDLE_PTR;
	void *fatdataptr = (void *) ES_FAT_DATA_PTR;

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


int FAT_Seek(s32 cfd, u32 where, u32 whence)
{
	int res;
	s32 *fathandle = (s32 *) ES_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) ES_FAT_DATA_PTR;

	fatdata->seek.cfd = cfd;
	fatdata->seek.where = where;
	fatdata->seek.whence = whence;

	fatdata->vector[0].data = &fatdata->seek.cfd;
	fatdata->vector[0].len = 4;

	fatdata->vector[1].data = &fatdata->seek.where;
	fatdata->vector[1].len = 4;

	fatdata->vector[2].data = &fatdata->seek.whence;
	fatdata->vector[2].len = 4;

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	res = os_ioctlv(*fathandle, IOCTL_FAT_SEEK, 3, 0, fatdata->vector);
	
	return res;
}

int FAT_Read(s32 cfd, u8 *data, u32 data_size)
{
	int res;
	s32 *fathandle = (s32 *) ES_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) ES_FAT_DATA_PTR;

	fatdata->read.cfd = cfd;
	fatdata->vector[0].data = &fatdata->read.cfd;
	fatdata->vector[0].len = sizeof(s32);

	fatdata->vector[1].data = data;
	fatdata->vector[1].len = data_size;


	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	res = os_ioctlv(*fathandle, IOCTL_FAT_READ, 1, 1, fatdata->vector);
	
	return res;
}

int FAT_Close(s32 cfd)
{
	int res;
	s32 *fathandle = (s32 *) ES_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) ES_FAT_DATA_PTR;

	fatdata->close.cfd = cfd;
	fatdata->vector[0].data = &fatdata->close.cfd; 
	fatdata->vector[0].len = sizeof(s32);

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	res = os_ioctlv(*fathandle, IOCTL_FAT_CLOSE, 1, 0, fatdata->vector);
	
	return res;
}

int FAT_Open(const char *filename, u32 mode)
{
	int res;
	s32 *fathandle = (s32 *) ES_FAT_HANDLE_PTR;
	fat_data* fatdata = (fat_data *) ES_FAT_DATA_PTR;

	ES_Strcpy(fatdata->open.filename, filename);
	fatdata->open.mode = mode;

	fatdata->vector[0].data = &fatdata->open.filename; 
	fatdata->vector[0].len = MAX_FILENAME_SIZE;

	fatdata->vector[1].data = &fatdata->open.mode; 
	fatdata->vector[1].len = sizeof(int);

	os_sync_after_write(fatdata, FAT_DATA_SIZE);

	res = os_ioctlv(*fathandle, IOCTL_FAT_OPEN, 2, 0, fatdata->vector);
	
	return res;
}

