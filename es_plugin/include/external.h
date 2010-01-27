#ifndef __EXTERNAL_H_
#define __EXTERNAL_H_

#include "types.h"

/* Not needed!!!
#define ES_FAT_HANDLE_PTR 0x20110630
#define ES_FAT_DATA_PTR   0x20110634
#define ES_EMU_TYPE_ADDR  0x20110638
*/

#define ES_SNPRINTF_ADDR  0x2010979C

void ES_snprintf(char *str, u32 size, const char *format, const char *arg1, u32 arg2, u32 arg3, u32 arg4);

#endif
