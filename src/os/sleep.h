/** @file src/os/sleep.h OS-independent inclusion of the delay routine. */

#ifndef OS_SLEEP_H
#define OS_SLEEP_H

#if defined(_WIN32)
	#include <windows.h>
	#define sleep(x) Sleep(x * 1000)
	#define msleep(x) Sleep(x)
#else
	#include <unistd.h>

	#define msleep(x) usleep(x * 1000)
#endif /* _WIN32 */

#define sleepIdle() msleep(1)

#endif /* OS_SLEEP_H */
