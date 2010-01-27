/*
 * ES plugin for Custom IOS.
 *
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

#ifndef __ES_H__
#define __ES_H__

#include "types.h"
#include "ipc.h"

#define MAX_FILENAME_SIZE    256

/* Not needed!!!
#define ES_FAT_HANDLE_PTR 0x20110630
#define ES_FAT_DATA_PTR   0x20110634
#define ES_EMU_TYPE_ADDR  0x20110638
*/

#define ES_SNPRINTF_ADDR  0x2010979C

#define IOCTL_ES_OPENCONTENT          0x09
#define IOCTL_ES_READCONTENT          0x0A
#define IOCTL_ES_CLOSECONTENT         0x0B
#define IOCTL_ES_SEEKCONTENT          0x23
#define IOCTL_ES_SETNANDEMULATION     0x70

#define FD_MAGIC 0x10

#define ES_EMU_NONE 0x00
#define ES_EMU_SD   0x01
#define ES_EMU_USB  0x02


s32 handleESMsg(ipcmessage *msg);
s32 handleESIoctlv(ipcmessage *msg);

// Return
s32 ES_OriginalIoctlv(ipcmessage *msg);

#endif
