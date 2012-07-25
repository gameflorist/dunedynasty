#ifndef TIMER_TIMER_COMMON_H
#define TIMER_TIMER_COMMON_H

#include <inttypes.h>

enum TimerType {
	TIMER_GUI   = 0,
	TIMER_GAME  = 1
};

extern int64_t g_timerGame;

#endif
