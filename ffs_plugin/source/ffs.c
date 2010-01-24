#include "ipc.h"
#include "tools.h"
#include "ffs.h"
#include "ffs_calls.h"
#include "syscalls.h"

#include <sys/stat.h>
#include <sys/statvfs.h>

#include "fat.h"

#define FFS_EMU_NONE    0
#define FFS_EMU_SD      1
#define FFS_EMU_USB     2

s32 FFS_Open(char *filename, s32 mode)
{
	return os_open(filename, mode);
}

s32 FFS_Close(u32 fd)
{
	return os_close(fd);
}

s32   FFS_Read(s32 fd, void *d, s32 len)
{
	return os_read(fd, d, len);
}

s32   FFS_Write(s32 fd, void *s, s32 len)
{
	return os_write(fd, s, len);
}

s32   FFS_Seek(s32 fd, s32 offset, s32 mode)
{
	return os_seek(fd, offset, mode);
}


void preappend_nand_dev_name(const char *origname, char *newname)
{
	volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
	s32 type = *emulationType;

	if (type == 1) 
		FFS_Strcpy(newname, "sd:");
	else if (type == 2)
		FFS_Strcpy(newname, "usb:");
	FFS_Strcat(newname, origname);
}

s32 handleFFSOpen(ipcmessage *msg)
{
	char name[MAX_FILENAME_SIZE];
	char *origname = msg->open.device;
	volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;

	if ((*emulationType == FFS_EMU_NONE) ||
		FFS_Strncmp(origname, "/dev/",5) == 0) 
	{
		return handleFFSOpen(msg);
	}

	preappend_nand_dev_name(origname, name);
	return FFS_Open(name, IPC_OPEN_RW);
}

#define FFS_FD_MAGIC       0x10

s32 handleFFSClose(ipcmessage *msg)
{
	if (msg->fd > FFS_FD_MAGIC)
		return handleFFSClose(msg);

	return FFS_Close(msg->fd);
}

s32 handleFFSRead(ipcmessage *msg)
{
	u32 len = msg->read.length;
	void *data = msg->read.data;
	u32 fd = msg->fd;

	if (fd > FFS_FD_MAGIC)
		return handleFFSRead(msg);
	
	return FFS_Read(fd, data, len);
}

s32 handleFFSWrite(ipcmessage *msg)
{
	u32 len = msg->write.length;
	void *data = msg->write.data;
	u32 fd = msg->fd;

	if (fd > FFS_FD_MAGIC)
		return handleFFSWrite(msg);
	
	return FFS_Write(fd, data, len);
}

s32 handleFFSSeek(ipcmessage *msg)
{
	s32 offset = msg->seek.offset;
	s32 origin = msg->seek.origin;
	u32 fd = msg->fd;

	if (fd > FFS_FD_MAGIC)
		return handleFFSSeek(msg);

	return FFS_Seek(fd, offset, origin);
}

#define OFFSET_NEW_NAME 0x40
s32 handleFFSIoctl(ipcmessage *msg)
{
	s32 ret;

	u32 cmd = msg->ioctl.command;
	u32 length_io = msg->ioctl.length_io;
	u32 *buffer_io = msg->ioctl.buffer_io;
	u32 *buffer_in = msg->ioctl.buffer_in;

	switch(cmd) {
	case FFS_IOCTL_CREATEDIR: {
		volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
		if (*emulationType == FFS_EMU_NONE)
			goto originalIoctl;

		char dirname[MAX_FILENAME_SIZE];
		preappend_nand_dev_name(buffer_in, dirname);
		ret = FAT_MakeDir(dirname);
		break;
	}
	case FFS_IOCTL_CREATEFILE: {
		volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
		if (*emulationType == FFS_EMU_NONE)
			goto originalIoctl;

		char filename[MAX_FILENAME_SIZE];
		preappend_nand_dev_name(buffer_in, filename);
		ret = FAT_MakeFile(filename);
		break;
	}
	case FFS_IOCTL_DELETE: {
		volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
		if (*emulationType == FFS_EMU_NONE)
			goto originalIoctl;

		char filename[MAX_FILENAME_SIZE];
		preappend_nand_dev_name(buffer_in, filename);
		ret = FAT_Delete(filename);
		break;
	}
	case FFS_IOCTL_RENAME: {
		u8 *names = (u8 *) buffer_in;
		volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
		if (*emulationType == FFS_EMU_NONE)
			goto originalIoctl;

		char newname[MAX_FILENAME_SIZE];
		char oldname[MAX_FILENAME_SIZE];
		struct stat filestat;

		preappend_nand_dev_name(names, oldname);
		preappend_nand_dev_name(names+OFFSET_NEW_NAME, newname);

		// Check if newname exists
		if (FAT_Stat(newname, &filestat) >=0) {
			if (S_ISDIR(filestat.st_mode))
				FAT_DeleteDir(newname);
			else
				FAT_Delete(newname);
		}

		ret = FAT_Rename(oldname, newname);
		break;
	}
	case FFS_IOCTL_GETSTATS: {
		char drive[MAX_FILENAME_SIZE];
		struct vfsstats vfsstat;
		volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
		if (*emulationType == FFS_EMU_NONE)
			goto originalIoctl;

		preappend_nand_dev_name("/", drive);
		ret = FAT_GetVFSStats(drive, vfsstat) >= 0;
		if (ret >= 0) {
			FFS_Memset(buffer_io, 0, length_io);
			// TODO!!! FILL the args, need to know the structures
		}

		break;
	}
	case FFS_IOCTL_GETFILESTATS:
		break;
	case FFS_IOCTL_GETATTR:
		break;
	case FFS_IOCTL_SETATTR:
		break;
	case FFS_IOCTL_FORMAT: {
		volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
		if (*emulationType == FFS_EMU_NONE)
			goto originalIoctl;
		ret = 0;
		break;
	}
	case FFS_IOCTL_SETNANDEMULATION: {
		char tmpdir[MAX_FILENAME_SIZE];
		u32 state = buffer_in[0];
		if (state) {
			preappend_nand_dev_name("/tmp", tmpdir);
			if (FAT_Init() >= 0) {
				FAT_DeleteDir(tmpdir);
			}
		}
		volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
		*emulationType = state;
		break;
	}
	originalIoctl:
	default:
		ret = FFS_HandleIoctl(msg);
		break;
	}
	return ret;
}

#define FFS_IOCTLV_READDIR    0x04
#define FFS_IOCTLV_0C         0x0C

s32 handleFFSIoctlv(ipcmessage *msg)
{
	int ret; 
	ioctlv *vector = msg->ioctlv.vector;
	u32 num_io = msg->ioctlv.num_io;
	volatile s32 *emulationType = (volatile s32 *) FFS_EMU_TYPE_ADDR;
	
	switch(msg->ioctlv.command) {
	case FFS_IOCTLV_READDIR: {
		if (*emulationType == 0)
			goto handleOriginal;

		char newpath[MAX_FILENAME_SIZE];
		u32 len;

		char *dirpath = (char *)vector[0].data;
		u32 *outlen =  (u32 *)vector[1].data;
		void *outbuf;

		if (num_io > 1) {
			outbuf = (char *)vector[2].data;
			outlen =  (u32 *)vector[3].data;
		} else
			outbuf = NULL;

		preappend_nand_dev_name(dirpath, newpath);
		ret = FAT_ReadDir(newpath, outbuf, &len);
		if (ret >= 0) 
			*outlen = len;
		break;
	}
	case FFS_IOCTLV_0C:
		if (*emulationType == 0)
			goto handleOriginal;
		u32 *ptr1 = (u32*) vector[1].data;
		u32 *ptr2 = (u32*) vector[2].data;
		*ptr1 = 1;
		*ptr2 = 1;
		os_sync_after_write(ptr1, 4);
		os_sync_after_write(ptr2, 4);
		ret = 0;
		break;
	handleOriginal:
	default:
		ret = handleFFSIoctlv(msg);
	}
	return ret;
}

s32 handleFFSMessage(ipcmessage *msg)
{
	s32 ret;
	switch (msg->command) {
		case IOS_OPEN:
			ret = handleFFSOpen(msg);
			break;
		case IOS_CLOSE:
			ret = handleFFSClose(msg);
			break;
		case IOS_READ:	
			ret = handleFFSRead(msg);
			break;
		case IOS_WRITE:		
			ret = handleFFSWrite(msg);
			break;
		case IOS_SEEK:
			ret = handleFFSSeek(msg);
			break;
		case IOS_IOCTL:
			ret = handleFFSIoctl(msg);
			break;
		case IOS_IOCTLV:
			ret = handleFFSIoctlv(msg);
			break;
		default:
			ret = -1;
	}
	return ret;
}

