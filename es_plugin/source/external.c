#include "external.h"

void ES_snprintf(char *str, u32 size, const char *format, const char *arg1, u32 arg2, u32 arg3, u32 arg4)
{
	void (*f)(char *, u32, const char *, const char *, u32, u32, u32) = (void *) ES_SNPRINTF_ADDR;

	(*f)(str, size, format, arg1, arg2, arg3, arg4);
}


