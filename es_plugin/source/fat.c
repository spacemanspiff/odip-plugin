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


int FAT_Seek()
{
	return 0;
}

int FAT_Read()
{
	return 0;
}

int FAT_Close()
{
	return 0;
}

int FAT_Open()
{
	return 0;
}


