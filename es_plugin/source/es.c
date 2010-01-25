#include "es.h"

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
	switch(msg->ioctlv.command) {

	case IOCTL_ES_OPENCONTENT:
		break;
	case IOCTL_ES_READCONTENT:
		break;
	case IOCTL_ES_SEEKCONTENT:
		break;
	case IOCTL_ES_CLOSECONTENT:
		break;
	case IOCTL_ES_SETNANDEMULATION:
		break;
	default:
		ret = ES_OriginalIoctlv(msg);
	}
	return ret;
}

