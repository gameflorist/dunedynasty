#ifndef TIMER_TIMERA5_H
#define TIMER_TIMERA5_H

#include "timer.h"

extern bool TimerA5_Init(void);
extern void TimerA5_Uninit(void);
extern bool TimerA5_SetTimer(enum TimerType timer, bool set);
extern void TimerA5_Sleep(int tics);
extern int64_t TimerA5_GetTicks(enum TimerType timer);

#endif
