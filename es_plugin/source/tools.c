/*
 * ES plugin for Custom IOS.
 *
 * Copyright (C) 2008-2010 Waninkoko, WiiGator.
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

#include "types.h"

#include "tools.h"
#include "syscalls.h"

void ES_Strcpy(char *dst, const char *src)
{
	u32 cnt;

	/* Copy bytes */
	for (cnt = 0; src[cnt]; cnt++)
		dst[cnt] = src[cnt];

	/* End of string */
	dst[cnt] = 0;

	/* Flush cache */
	os_sync_after_write(dst, cnt);
}

void ES_Memset(void *dst, char c, int len)
{
	u8 *ptr = (u8 *)dst;
	u32 cnt;

	/* Set bytes */
	for (cnt = 0; cnt < len; cnt++)
		ptr[cnt] = c;

	/* Flush cache */
	os_sync_after_write(dst, len);
}
