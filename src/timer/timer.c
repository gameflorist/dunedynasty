/* timer.c */

#include "timer.h"

#include "timer_a5.h"

int64_t g_timerGame;

bool
Timer_SetTimer(enum TimerType timer, bool set)
{
	return TimerA5_SetTimer(timer, set);
}

int64_t
Timer_GetTimer(enum TimerType timer)
{
	return TimerA5_GetTicks(timer);
}

void
Timer_Sleep(int tics)
{
	TimerA5_Sleep(tics);
}
