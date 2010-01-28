This is based on the FFS Plugin found in the Revision 17 of the Waninkoko's cIOS.

------------------------------
Puntos de Enganche:

El entry point de la rutina que parsea todos los mensajes esta en:  (revisar
esten en el orden correcto)
	20001808	IOS_Open
	20001FB8	IOS_Close
	200018D0	IOS_Read
	20001A2C	IOS_Write
	20001C10	IOS_Seek
	20001C6C	IOS_Ioctl
	20001ED0	IOS_Ioctlv

EL FFS Plugin se carga a partir de la pos: 20007200

Los puntos de vuelta al enganche son los que están definidos en source/ffs_calls.s
Que son los puntos de las llamadas que reciben los diferentes mensajes del
modulo. IOS_Open, IOS_Close, IOS_Read, IOS_Write, IOS_Seek, IOS_Ioctl, IOS_Ioctlv.

Para volver ejecutan el código que sobreescribieron para engancharse y vuelven
"normalmente"
