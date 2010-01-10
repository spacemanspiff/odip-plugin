/*
 *  Copyright (C) 2010 Spaceman Spiff
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <types.h>

#include <module.h>
#include <syscalls.h>

#define DVDIO_SIZE 32

#define DVDIO_ADDR 0xD006000
#define SECTORSIZE 2048
#define MAX_SIZE_DI   (0xFF << 15)

static u32 DIP_adjust_read_size(u8 *buf, u32 size, u32 align);

int DIP_StopMotor(void)
{
	u32 inbuf[8];
	inbuf[0] = IOCTL_DI_STOPMOTOR << 24;
	inbuf[1] = 0;
	inbuf[2] = 0;
	return handleDiCommand(inbuf,NULL,0);
}


int DIP_CustomCommand(u8 *cmd, u8 *ans)
{
	volatile unsigned long *dvdio = (volatile unsigned long *) DVDIO_ADDR;

	ios_memcpy((u8 *) dvdio, cmd, DVDIO_SIZE);
	os_sync_after_write((u8 *) dvdio, DVDIO_SIZE);

	// DI Control Register, wait until ready
	while (dvdio[7] & 1)      // 0xD00601C
		;

	ios_memcpy(ans, (u8 *) dvdio, DVDIO_SIZE);
	os_sync_after_write(ans, DVDIO_SIZE);

	return dvdio[8];          // 0xD006020
}

int DIP_ReadDVDVideo(void *dst, u32 len, u32 lba)
{
	int res;
	int tries = 0;
	u32 inbuf[8];
	do {
		inbuf[0] = IOCTL_DI_READDVD << 24;
		inbuf[1] = 0;
		inbuf[2] = 0;
		inbuf[3] = len >> 11; // Convert to 2048 blocks (dvd sector)
		inbuf[4] = lba;
		os_sync_before_read(dst, len);
		res = handleDiCommand(inbuf, dst, len);
		tries++;
	} while (res != 0 || tries < 15);
	return res;
}


int DIP_ReadDVD(void *dst, u32 len, u32 sector)
{
	u32 inbuf[8];

	inbuf[0] = IOCTL_DI_LOWREAD << 24; // IOCTL_DI_LOWREAD A8
	inbuf[1] = len;
	inbuf[2] = sector;

	os_sync_before_read(dst, len);
	return handleDiCommand(inbuf, dst, len);
}


int DIP_ReadDVDRom(u8 *outbuf, u32 len, u32 offset)
{
	u32 cnt = 0;
	int res;

	if (len == 0)
		return 0;

	u32 lba = offset >> 9; // offset / 512 
	u32 size = len;
	
	while (cnt < size) { 
		u32 skip;
		size = len - cnt;

		skip = (offset > (lba << 9)) ? (offset - (lba << 9)) << 2 : 0;

		outbuf = outbuf + cnt;
		res = DIP_adjust_read_size(outbuf, size, SECTOR_SIZE);

		if (skip || !res) {
			if ((skip + size) > SECTORSIZE) 
				size = SECTORSIZE - skip;
			u8 *mem = ios_alloc_aligned(SECTORSIZE, 0x20);
			if (!mem)
				return -1;

			res = DIP_ReadDVDVideo(mem, SECTORSIZE, lba);
			if (res == 0) {
				ios_memcpy(outbuf, mem + skip, size);
				os_sync_after_write(outbuf, size);
			}
			ios_free(mem);
		} else {
			size = res;
			if (size >= MAX_SIZE_DI)
				size = MAX_SIZE_DI;
			res = DIP_ReadDVDVideo(outbuf, size, lba); // inlined function
			/*
			u32 tries = 0;
			do {
				u32 inbuf[8];

				inbuf[0] = IOCTL_DI_READDVD << 24;
				inbuf[1] = 0;
				inbuf[2] = 0;
				inbuf[3] = size >> 11;
				inbuf[4] = lba;
				os_sync_before_read(outbuf, size);
				res = handleDiCommand(inbuf, (u32 *) outbuf, size);
				tries = tries + 1;
			} while (res != 0 && tries < 15);
			*/
		}

		if (res) 
			return res;

		cnt += size;
		lba += (size + skip) >> 11;
	} 
	return res;
}

/* si retorna cero, el buffer, no esta alineado. */
static u32 DIP_adjust_read_size(u8 *bufptr, u32 size, u32 align)
{
	u32 buf = (u32) bufptr;

	//from assembler: if (buf << 27)
	if (buf & 0x1F)  // if not aligned to 32 bytes
		return 0;
	
	u32 newSize = (buf < 0x17FFFFF)? 0x1800000 - buf :  0;

	if ((0xF0000000 + buf) < 0x3617FFF)
		newSize = 0x13618000 - buf;
	
	if (newSize < align)
		return 0;

	if (newSize < size) 
		newSize = size;

	newSize &= ~(align - 1); // multiplo de align
	return newSize;
}

