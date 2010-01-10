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

#include <ipc.h>

#include <types.h>
#include <utils.h>
#include <syscalls.h>

#define WBFS_BASE (('W'<<24)|('F'<<16)|('S'<<8))
#define USB_IOCTL_WBFS_OPEN_DISC		(WBFS_BASE+0x1)
#define USB_IOCTL_WBFS_READ_DISC		(WBFS_BASE+0x2)
#define USB_IOCTL_WBFS_READ_DIRECT_DISC		(WBFS_BASE+0x3)
#define USB_IOCTL_WBFS_STS_DISC			(WBFS_BASE+0x4)

u32 usb_is_dvd;

u32 usb_device_fd;

#define USB_DATA_SIZE 64
// Ver atributo packed
static struct _usb_data{
	struct _ioctl ioctlv[4];
	u32 values[8];
} usb_data;

static struct _usb_data* usb_data_ptr;


#define MAX_DEVICES 2
static char *usb_device[] = {"/dev/usb2", 
			     "/dev/sdio/sdhc", 
			     "/dev/usb/ehc"};

u32 usb_dvd_inserted(void)
{
	if (!usb_is_dvd)
		return 1;
	
	if (usb_device_fd <= 0)
		return 1;
	
	return os_ioctlv(usb_device_fd, USB_IOCTL_WBFS_STS_DISC, 0,0, usb_data_ptr->ioctlv);
}

u32 usb_read_device(u8 *outbuf, u32 size, u32 lba)
{
	u32 res;

	if (usb_device_fd <= 0)
		return -1;

	usb_data_ptr->values[0] = lba;
	usb_data_ptr->values[7] = size;

	usb_data_ptr->ioctlv[2].data = outbuf;
	usb_data_ptr->ioctlv[3].data = (u32 *) size;  // No es necesario, pero estaba en el codigo original

	os_sync_after_write(usb_data_ptr, USB_DATA_SIZE);
	os_sync_after_write(outbuf, size);

	if (usb_is_dvd) 
		res = os_ioctlv(usb_device_fd, USB_IOCTL_WBFS_READ_DIRECT_DISC, 2, 1, usb_data_ptr->ioctlv);
	else
		res = os_ioctlv(usb_device_fd, USB_IOCTL_WBFS_READ_DISC, 2, 1, usb_data_ptr->ioctlv);
		
	os_sync_before_read(outbuf, size);

	return res;
}


u32 usb_open_device(u32 device_nr, u8 *id, u32 partition)
{
	u32 res;
	u32 part = partition;	

	if (device_nr > MAX_DEVICES)
		return -1;
	
	if (usb_data_ptr == 0) {
		usb_data_ptr = &usb_data;
		usb_device_fd = -1;
	} else 	if (usb_device_fd)
		os_close(usb_device_fd);

	usb_is_dvd = 0;

	if (id[0] == '_' && id[1] == 'D' && id[2] == 'V' && id[3] == 'D') 
		usb_is_dvd = 1;
	
	usb_device_fd = os_open(usb_device[device_nr] , 1);
	if (usb_device_fd <= 0) {
		if (device_nr == 0)
			usb_device_fd = os_open(usb_device[0], 1);
	}

	if (usb_device_fd <= 0)
		return usb_device_fd;

	ios_memcpy((void *)(usb_data_ptr->values), id, 6);
	ios_memcpy((void *) &(usb_data_ptr->values[7]), (u8 *) &part, sizeof(part));

	usb_data_ptr->ioctlv[0].data = (void *) (usb_data_ptr->values);
	usb_data_ptr->ioctlv[0].len = 6;
	usb_data_ptr->ioctlv[1].data = (void *) &(usb_data_ptr->values[7]);
	usb_data_ptr->ioctlv[1].len = sizeof(u32);

	os_sync_after_write(usb_data_ptr, USB_DATA_SIZE);
	res = os_ioctlv(usb_device_fd, USB_IOCTL_WBFS_OPEN_DISC, 2, 0, usb_data_ptr->ioctlv);

	usb_data_ptr->ioctlv[0].data = (void *) (usb_data_ptr->values);
	usb_data_ptr->ioctlv[0].len = sizeof(u32);
	usb_data_ptr->ioctlv[1].data = (void *) &(usb_data_ptr->values[7]);
	usb_data_ptr->ioctlv[1].len = sizeof(u32);
	
	os_sync_after_write(usb_data_ptr, USB_DATA_SIZE);

	return res;
}

