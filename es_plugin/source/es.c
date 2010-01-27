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

#include "tools.h"
#include "external.h"

#define TITLEFORMAT "%s/title/%08x/%08x/content/%08x.app" 

static s32 emulationType = 0;

const char *devices[] = {
	"sd:", "usb:"
};

s32 handleESMsg(ipcmessage *msg)
{
	if (msg->command == IOS_IOCTLV)
		handleESIoctlv(msg);
	return -1;
}

int ES_getTitleId(u32 *titleId)
{
	ipcmessage msg;
	ioctlv vector[8];
	
	Memset(&msg, 0, sizeof(ipcmessage));
	
	vector[0].data = &titleId;
	vector[0].len = 8;

	msg.ioctlv.command = IOCTL_ES_GETTITLEID;
	msg.ioctlv.num_in = 0;
	msg.ioctlv.num_io = 1;
	msg.ioctlv.vector = vector;
	
	// fd = 0 ??? Es correcto???
	return ES_OriginalIoctlv(&msg);
}

void generateFilename(char *filename, u32 *titleId, u32 index)
{
	const char *device = (emulationType == ES_EMU_USB || emulationType == ES_EMU_SD)?devices[emulationType-1]:NULL;

	ES_snprintf(filename, MAX_FILENAME_SIZE, TITLEFORMAT, device, *titleId, *(titleId+1), index);
}

s32 handleESIoctlv(ipcmessage *msg)
{
	s32 ret = 0;

	switch(msg->ioctlv.command) {

	case IOCTL_ES_OPENCONTENT: {
		if (emulationType == ES_EMU_NONE)
			goto original_ioctlv;

		u32 index = *((s32 *) msg->ioctlv.vector[0].data);
		char filename[MAX_FILENAME_SIZE];
		u32 titleId[2];

		if (ES_getTitleId(titleId) <= 0) 
			goto original_ioctlv;

		generateFilename(filename, titleId, index);
		ret = FAT_Open(filename, IPC_OPEN_READ);
		if (ret < 0)
			goto original_ioctlv;
		break;
	}
	case IOCTL_ES_READCONTENT: {
		if (emulationType == ES_EMU_NONE)
			goto original_ioctlv;

		s32 cfd = *((s32 *) msg->ioctlv.vector[0].data);
		u8* data = (u8 *) msg->ioctlv.vector[1].data;
		u32 size = msg->ioctlv.vector[1].len;

		if (cfd <= FD_MAGIC )
			goto original_ioctlv;

		ret = FAT_Read(cfd, data, size);
		break;
	}
	case IOCTL_ES_SEEKCONTENT: {
		if (emulationType == ES_EMU_NONE)
			goto original_ioctlv;

		s32 cfd = *((s32 *) msg->ioctlv.vector[0].data);
		u32 where = *((u32 *) msg->ioctlv.vector[1].data);
		u32 whence = *((u32 *) msg->ioctlv.vector[2].data);
		if (cfd <= FD_MAGIC )
			goto original_ioctlv;

		ret = FAT_Seek(cfd, where, whence);
		break;
	}
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
		emulationType = nandEmu;
		ret = 0;
		break;
	}
	default:
	original_ioctlv:
		ret = ES_OriginalIoctlv(msg);
	}
	return ret;
}
