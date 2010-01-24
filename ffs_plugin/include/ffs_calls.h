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
