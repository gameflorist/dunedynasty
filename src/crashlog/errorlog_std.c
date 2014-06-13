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
