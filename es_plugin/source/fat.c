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
#include "es.h"
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

	ES_Strcpy(fatDataPtr->open.filename, filename);
	fatDataPtr->open.mode = mode;

	fatDataPtr->vector[0].data = &fatDataPtr->open.filename; 
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = &fatDataPtr->open.mode; 
	fatDataPtr->vector[1].len = sizeof(int);

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_OPEN, 2, 0, fatDataPtr->vector);
	
	return res;
}

