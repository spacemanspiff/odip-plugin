/*
 * FFS plugin for Custom IOS.
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

#ifndef __FFS_CALLS_H__
#define __FFS_CALLS_H__

#include "types.h"
#include "ipc.h"

s32 FFS_HandleOpen(ipcmessage *);
s32 FFS_HandleClose(ipcmessage *);
s32 FFS_HandleRead(ipcmessage *);
s32 FFS_HandleWrite(ipcmessage *);
s32 FFS_HandleSeek(ipcmessage *);
s32 FFS_HandleIoctl(ipcmessage *);
s32 FFS_HandleIoctlv(ipcmessage *);

#endif
