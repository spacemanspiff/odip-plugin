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

void Strcpy(char *dst, const char *src)
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

int Strncmp(const char*str1, const char *str2, int size)
{
	u32 cnt;

	/* Compare bytes */
	for (cnt = 0; cnt < size; cnt++) {
		if (str1[cnt] < str2[cnt])
			return -1;
		if (str1[cnt] > str2[cnt])
			return  1;

		/* End of string */
		if (!str1[cnt] || !str2[cnt])
			break;
	}

	return 0;
}

void Strcat(char *str1, const char *str2)
{
	u32 cnt;

	/* Find end of string character */
	for (cnt = 0; str1[cnt]; cnt++);

	/* Copy string */
	Strcpy(str1 + cnt, str2);
}

void Memcpy(void *dst, const void *src, int len)
{
	u8 *s = (u8 *)src;
	u8 *d = (u8 *)dst;

	u32 cnt;

	/* Copy bytes */
	for (cnt = 0; cnt < len; cnt++)
		d[cnt] = s[cnt];

	/* Flush cache */
	os_sync_after_write(dst, len);
}


void Memset(void *buf, u8 val, u32 len)
{
	u8 *ptr = (u8 *)buf;
	u32 cnt;

	/* Set bytes */
	for (cnt = 0; cnt < len; cnt++)
		ptr[cnt] = val;

	/* Flush cache */
	os_sync_after_write(buf, len);
}

#define nible2hex(x)       (((x) < 10)?('0'+(x)):('A'+(x)-10))
#define high(x)   (((x) & 0xf0) >> 4)
#define low(x)   ((x) & 0xf)

char  *Int2hex(char *str, u32 n)
{
	char *buf = str;
	u8 *c = (u8 *) &n;

	if (!buf)
		return NULL;

	*(buf++) = nible2hex(high(c[0]));
	*(buf++) = nible2hex(low(c[0]));
	*(buf++) = nible2hex(high(c[1]));
	*(buf++) = nible2hex(low(c[1]));
	*(buf++) = nible2hex(high(c[2]));
	*(buf++) = nible2hex(low(c[2]));
	*(buf++) = nible2hex(high(c[3]));
	*(buf++) = nible2hex(low(c[3]));
	*(buf++) = 0;

	return buf;
}

