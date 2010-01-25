This is based on the FFS Plugin found in the Revision 15 of the Waninkoko's
cIOS.

Not complete yet.

There are some errors in the code...


------------------------------
Puntos de Enganche:

El entry point de la rutina que parsea todos los mensajes esta en: 20001ED0

EL FFS Plugin se carga a partir de la pos: 20007200

Los puntos de enganche son los que están definidos en source/ffs_calls.s

Que son los puntos de las llamadas que reciben los diferentes mensajes del
modulo. IOS_Open, IOS_Close, IOS_Read, IOS_Write, IOS_Seek, IOS_Ioctl, IOS_Ioctlv.

Otro punto de enganche son los 3 punteros que se usan:

#define FFS_FAT_HANDLE_PTR 0x2004F9EC
#define FFS_FAT_DATA_PTR   0x2004F9F0
#define FFS_EMU_TYPE_ADDR  0x2004F9F4

Donde FFS_FAT_HANDLE_PTR -> guarda la pos. 
      FFS_FAT_DATA_PTR   -> puntero donde se guardan datos privados para usar
                            con el modulo
      FFS_EMU_TYPE_ADDR   -> Aca se guarda el tipo de emulacion, 0 no emulado,
                                   1 sd, 2 usb

