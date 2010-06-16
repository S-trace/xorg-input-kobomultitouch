#ifndef XBYPASS_H
#define XBYPASS_H

#include <stdarg.h>
void xf86Msg(int type, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

#endif
