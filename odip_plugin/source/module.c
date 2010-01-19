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
#include <utils.h>
#include <module.h>
#include <syscalls.h>

#include <wbfs.h>
#include <di.h>

#ifdef DEBUG
#include <debug.h>
#endif
dipstruct dip;

#define WII_DVD_SIGNATURE 0x5D1C9EA3

#define READINFO_SIZE_DATA 32

#define BCADATA_SIZE 64
const u8 bca_bytes[BCADATA_SIZE] ATTRIBUTE_ALIGN(32) = { 
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


void dummy_function(u32 *outbuf, u32 outbuf_size)
{
}

int read_from_device(u32 *outbuf, u32 size, u32 lba)
{
	lba += dip.base + dip.offset;

	if (dip.has_id) 
		return usb_read_device((u8 *)outbuf, size, lba);

	if (dip.dvdrom_mode)
		return DIP_ReadDVDRom((u8 *) outbuf, size, lba);
	
	return DIP_ReadDVD((u8 *) outbuf, size, lba);
}

int read_id_from_image(u32 *outbuf, u32 outbuf_size)
{
	u32 res;
	u32 lba = dip.offset + dip.base;

	res = read_from_device(outbuf, READINFO_SIZE_DATA, lba);
	/* inlined read_from_device...
	if (dip.has_id)
		res = usb_read_device((u8 *)outbuf, READINFO_SIZE_DATA, lba);
	else if (dip.dvdrom_mode)
		res = DIP_ReadDVDRom((u8 *) outbuf, READINFO_SIZE_DATA, lba);
	else
		res = DIP_ReadDVD(outbuf, READINFO_SIZE_DATA, lba);
	*/

	if (res == 0 && outbuf[6] == WII_DVD_SIGNATURE) {
		volatile u8 *dvd_cdata = (volatile u8 *) addr_dvd_read_controlling_data;  

		*dvd_cdata = 1;
		if (dvd_cdata[1] == 0)
			ios_doReadHashEncryptedState();
	}
	return res;
}

/*
Ioctl 0x13 -> usada por la función Disc_USB_DVD_Wait(), checkea si hay disco montado desde la unidad DVD. Solo se usa en uLoader

Ioctl 0x14 -> equivale a Ioctl 0x88, pero exceptuando el uso de DVD, devuelve un estado fake para WBFS y DVD USB

Ioctl 0x15 -> equivale a Ioctl 0x7a y devuelve el registro tal cual, exceptuando el bit 0 (para indicar la presencia del disco siempre)
 */

int DI_EmulateCmd(u32 *inbuf, u32 *outbuf, u32 outbuf_size)
{
	int res = 0;

	u8 cmd = ((u8 *) inbuf)[0];

#ifdef DEBUG
	s_printf("DIP::DI_EmulateCmd(%x, %x, %x)", cmd, outbuf, outbuf_size);
#endif

	if (cmd == IOCTL_DI_REQERROR)
		goto handleReqError;
	
	dip.currentError = 0;

	switch (cmd) {
			case IOCTL_DI_REQERROR:
handleReqError:
				if (dip.currentError != 0) {
					if (!dip.has_id)
						goto call_original_di;

					dip_memset((u8 *) outbuf, 0, outbuf_size);
					outbuf[0] = dip.currentError;
					os_sync_after_write(outbuf, outbuf_size);
					res = 0;
				}
				break;

			case IOCTL_DI_SEEK:
				if (!dip.dvdrom_mode) { 
					res = 0;
					break;
				}
			case IOCTL_DI_WAITCOVERCLOSE:
				if (!dip.has_id)
					goto call_original_di;
				res = 0;
				break;

			case IOCTL_DI_STREAMING:
				if (dip.dvdrom_mode ||
					!dip.has_id) 
					goto call_original_di;
				dip_memset((u8*) outbuf, 0, outbuf_size);
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;

			case IOCTL_DI_SETBASE:
				dip.base = inbuf[1];
				res = 0;
				break;

			case IOCTL_DI_GETBASE:
				outbuf[0] = dip.base;
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;

			case IOCTL_DI_SETFORCEREAD:
				dip.low_read_from_device = inbuf[1];
				res = 0;
				break;

			case IOCTL_DI_GETFORCEREAD:
				outbuf[0] = dip.low_read_from_device;
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;

			case IOCTL_DI_SETUSBMODE: {  
				dip.has_id = inbuf[1];
				// Copy id
				if (dip.has_id) {
					ios_memcpy(dip.id, (u8 *)&(inbuf[2]), 6);
				}
				dip.partition = inbuf[5];
				res = 0;
				break;
			}

			case IOCTL_DI_GETUSBMODE:
				outbuf[0] = dip.has_id;
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;

			case IOCTL_DI_DISABLERESET:
				dip.disableReset = *((u8 *) (&inbuf[1]));
				res = 0;
				break;

			case IOCTL_DI_13:
			case IOCTL_DI_14: {
				volatile unsigned long *dvdio = (volatile unsigned long *) 0xD006000;	
				if (outbuf == NULL) {
					res = 0;
					break;
				}
				if (cmd == 0x13) {
					if (!dip.has_id) {
						outbuf[0] = (dvdio[1] << 31)?0:2; 
					} else {
						if (usb_is_dvd) {
							outbuf[0] = (usb_dvd_inserted() == 0)?0:2;		
						} else {
							if (usb_device_fd > 0)
								outbuf[0] = 2;
							else {
								outbuf[0] = (dvdio[1] << 31)?0:2; 
							}
						}
					}
				} else {
					// ioctl 0x14
					if (!dip.has_id) {
						outbuf[0] = (dvdio[1] << 31)?0:2; 
					} else {
						if (usb_device_fd > 0)
							outbuf[0] = 2;
						else {
							outbuf[0] = (dvdio[1] << 31)?0:2; 
						}
					}
				}
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;
			}
			case IOCTL_DI_15: {
				volatile unsigned long *dvdio = (volatile unsigned long *) 0xD006000;	
				outbuf[0] = dvdio[1] & (~0x00000001); 
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;
			}

			case IOCTL_DI_CUSTOMCMD:
				res = DIP_CustomCommand((u8 *)((u32) inbuf[1] & 0x7FFFFFFF), (u8 *) outbuf);				
				break;

			case IOCTL_DI_OFFSET: {
				if (dip.dvdrom_mode || dip.has_id) {
					dip.offset =  ((inbuf[1] << 30 ) | inbuf[2]) & 0xFFFF8000; // ~(SECTOR_SIZE >> 2)
					res = 0;
				} else 
					goto call_original_di;
				break;
			}
			case IOCTL_DI_REPORTKEY:
				if (!dip.dvdrom_mode && !dip.has_id) 
					goto call_original_di;
				res = 0xA0 << 8;
				dip.currentError = 0x53100;
				break;

			case IOCTL_DI_DISC_BCA: {
				u32 cont = 0;
				os_sync_before_read((u8 *) bca_bytes, BCADATA_SIZE);
				while (bca_bytes[cont] == 0) {
					cont++;
					if (cont == 64)
						goto call_original_di;
				}
				ios_memcpy((u8 *) outbuf, bca_bytes, BCADATA_SIZE);
				os_sync_after_write(outbuf,BCADATA_SIZE);
				res = 0;
				break;
			}

			case IOCTL_DI_READ: {
				dip.reading = 1;
				if (!dip.low_read_from_device) 
					res = handleDiCommand(inbuf, outbuf, outbuf_size);
				else
					res = read_from_device(outbuf, inbuf[1], inbuf[2]);
				dip.reading = 0;
				if (res == 0) 
					dummy_function(outbuf, outbuf_size);
				break;
			}
			case IOCTL_DI_READID: {
				u32 cmdRes;
				u32 dvdRomModeNeeded;
				if (dip.has_id) {
					cmdRes = 0;
					dvdRomModeNeeded = 0;
				} else {
					cmdRes = handleDiCommand(inbuf, outbuf, outbuf_size);
					dvdRomModeNeeded = (cmdRes==0)?0:1;
				}
				if (!(dip.base || dip.offset) && cmdRes == 0 &&
					!dip.has_id) {
						dip.dvdrom_mode = dvdRomModeNeeded;	
						res = 0;
						break;
				}
				dip.dvdrom_mode = dvdRomModeNeeded;
				res = read_id_from_image(outbuf, outbuf_size);
				break;
			}
			case IOCTL_DI_RESET: {
				if (dip.disableReset) {
					res = 0;
					break;
				}
				dip.reading = 0;
				dip.low_read_from_device = 0;
				dip.dvdrom_mode = 0;
				dip.base = 0;
				dip.offset = 0;
				dip.currentError = 0;

				if (!dip.has_id)  
					goto call_original_di;

				DIP_StopMotor();
				res = usb_open_device(dip.has_id - 1, &(dip.id[0]), dip.partition);
				break;
			}
			case IOCTL_DI_UNENCREAD:
			case IOCTL_DI_LOWREAD:
			case IOCTL_DI_READDVD: {
				u32 offset = inbuf[2];
				u32 len = inbuf[1];
				if (cmd == IOCTL_DI_READDVD) {
					offset = offset << 9;
					len = len << 11;
				}
				res = read_from_device(outbuf, len, offset);
				if (res == 0 && dip.reading == 0)
						dummy_function(outbuf, outbuf_size);
				break;
			}

			default:
		call_original_di:
				res = handleDiCommand(inbuf, outbuf, outbuf_size);
				break;
	}
	return res;
}
