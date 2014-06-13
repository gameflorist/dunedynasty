/**
 * @file src/crashlog/errorlog_win32.c
 *
 * System dependant error messages for win32.
 */

#if defined(_MSC_VER)
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif /* _MSC_VER */
#include <io.h>
#include <windows.h>
#include "buildcfg.h"
#include "errorlog.h"
#include "types.h"
#include "../os/strings.h"

void
ErrorLog_Init(const char *dir)
{
#if defined(__MINGW32__) && defined(__STRICT_ANSI__)
	extern int __cdecl __MINGW_NOTHROW _fileno(FILE *);
#endif

	char filename[1024];

	snprintf(filename, sizeof(filename), "%s/error.log", dir);
	FILE *err = fopen(filename, "w");

	if (err != NULL) {
		if (_dup2(_fileno(err), _fileno(stderr)) != 0) {
			fclose(err);
			freopen(filename, "w", stderr);
		}
	}

	snprintf(filename, sizeof(filename), "%s/output.log", dir);
	FILE *out = fopen(filename, "w");

	if (out != NULL) {
		if (_dup2(_fileno(out), _fileno(stdout)) != 0) {
			fclose(out);
			freopen(filename, "w", stdout);
		}
	}

#if defined(_MSC_VER)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	FreeConsole();
}

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
