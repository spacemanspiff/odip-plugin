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

#include "es.h"
#include "fat.h"

#define TITLEFORMAT "%s/title/%08x/%08x/content/%08x.app" 

const char *devices[] = {
	"sd:", "usb:"
};

s32 handleESMsg(ipcmessage *msg)
{
	if (msg->command == IOS_IOCTLV)
		handleESIoctlv(msg);
	return -1;
}

s32 handleESIoctlv(ipcmessage *msg)
{
	s32 ret;
	s32 emulationType = *((s32 *) ES_EMU_TYPE_ADDR);

	switch(msg->ioctlv.command) {

	case IOCTL_ES_OPENCONTENT:
		if (emulationType == ES_EMU_NONE)
			goto original_ioctlv;
		// TODO
		//
		break;

	case IOCTL_ES_READCONTENT:
		if (emulationType == ES_EMU_NONE)
			goto original_ioctlv;
		// TODO

		break;

	case IOCTL_ES_SEEKCONTENT:
		if (emulationType == ES_EMU_NONE)
			goto original_ioctlv;
		// TODO

		break;

	case IOCTL_ES_CLOSECONTENT: {
		if (emulationType == ES_EMU_NONE)
			goto original_ioctlv;

		s32 fd = *((s32 *) msg->ioctlv.vector[0].data);
		if (fd <= FD_MAGIC )
			goto original_ioctlv;
		FAT_Close(fd);
		break;
	}
	case IOCTL_ES_SETNANDEMULATION: {
		s32 nandEmu = *((s32 *) msg->ioctlv.vector[0].data);
		if (nandEmu) {
			ret = FAT_Init();
			if (ret < 0) 
				break;
		}
		*((s32 *) ES_EMU_TYPE_ADDR) = nandEmu;
		ret = 0;
		break;
	}
	default:
	original_ioctlv:
		ret = ES_OriginalIoctlv(msg);
	}
	return ret;
}

