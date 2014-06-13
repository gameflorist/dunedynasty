/**
 * @file src/crashlog/errorlog_std.c
 *
 * System dependant error messages.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "errorlog.h"
#include "types.h"
#include "../strings.h"

void
ErrorLog_Init(const char *dir)
{
	(void)dir;
}

void
Error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

void
Warning(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}
