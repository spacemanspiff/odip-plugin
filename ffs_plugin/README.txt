This is based on the FFS Plugin found in the Revision 17 of the Waninkoko's cIOS.

------------------------------
Puntos de Enganche:

El entry point de la rutina que parsea todos los mensajes esta en: 20001ED0

EL FFS Plugin se carga a partir de la pos: 20007200

Los puntos de enganche son los que están definidos en source/ffs_calls.s

Que son los puntos de las llamadas que reciben los diferentes mensajes del
modulo. IOS_Open, IOS_Close, IOS_Read, IOS_Write, IOS_Seek, IOS_Ioctl, IOS_Ioctlv.

