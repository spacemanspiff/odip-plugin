#ifndef _IPC_H_
#define _IPC_H_

#include "types.h"

/* IPC open modes */
#define IPC_OPEN_NONE                0
#define IPC_OPEN_READ                1
#define IPC_OPEN_WRITE               2
#define IPC_OPEN_RW             (IPC_OPEN_READ|IPC_OPEN_WRITE)


/* IPC error codes */
#define IPC_ENOENT		-6
#define IPC_ENOMEM		-22
#define IPC_EINVAL		-101
#define IPC_EACCESS		-102
#define IPC_EEXIST		-105
#define IPC_NOENT		-106

/* IOS calls */
#define IOS_OPEN		0x01
#define IOS_CLOSE		0x02
#define IOS_READ		0x03
#define IOS_WRITE		0x04
#define IOS_SEEK		0x05
#define IOS_IOCTL		0x06
#define IOS_IOCTLV		0x07

/* IOCTL vector */
typedef struct iovec {
	void *data;
	u32   len;
} ioctlv;

/* IPC message */
typedef struct ipcmessage {
	u32 command; // 0x00
	u32 result;  // 0x04
	u32 fd;      // 0x08

	union 
	{
		struct
		{
			char *device;
			u32   mode;
			u32   resultfd;
		} open;
	
		struct 
		{
			void *data;
			u32   length;
		} read, write;
		
		struct 
		{
			s32 offset;
			s32 origin;
		} seek;
		
		struct 
		{
			u32 command;     // 0x0C

			u32 *buffer_in;  // 0x10
			u32  length_in;  // 0x14
			u32 *buffer_io;  // 0x18
			u32  length_io;  // 0x1C
		} ioctl;
		struct 
		{
			u32 command;     // 0x0C

			u32 num_in;      // 0x10
			u32 num_io;      // 0x14
			ioctlv *vector;  // 0x18
		} ioctlv;
	};
} ATTRIBUTE_PACKED ipcmessage;

#endif
