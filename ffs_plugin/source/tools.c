/*
 * DIP plugin for Custom IOS.
 *
 * Copyright (C) 2008-2010 Waninkoko, WiiGator.
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

#include "types.h"

#include "tools.h"
#include "syscalls.h"

// TODO: Implementar!!!

int FFS_Strncmp(const char*str1, const char *str2, int size)
{
}

void FFS_Strcpy(char *dst, const char *src)
{
}

void FFS_Strcat(char *str1, const char *str2)
{
}

void FFS_Memcpy(void *dst, const void *src, int len)
{
	//
	//
	os_sync_after_write(dst, len);
}


void FFS_Memset(void *buf, u8 val, u32 len)
{
	//
	os_sync_after_write(buf, len);
}
