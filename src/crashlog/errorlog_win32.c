/**
 * @file src/crashlog/errorlog_win32.c
 *
 * System dependant error messages for win32.
 */

#include <windows.h>
#include "buildcfg.h"
#include "errorlog.h"
#include "types.h"
#include "../os/strings.h"

void
Error(const char *format, ...)
{
	char message[512];
	va_list ap;

	va_start(ap, format);
	vsnprintf(message, sizeof(message), format, ap);
	vfprintf(stderr, format, ap);
	va_end(ap);

	MessageBox(NULL, message,
			"Error - " DUNE_DYNASTY_STR " " DUNE_DYNASTY_VERSION,
			MB_OK | MB_ICONERROR);
}

void
Warning(const char *format, ...)
{
	char message[512];
	va_list ap;

	va_start(ap, format);
	vsnprintf(message, sizeof(message), format, ap);
	vfprintf(stderr, format, ap);
	va_end(ap);
}
