#ifndef TIMER_TIMER_OPENDUNE_H
#define TIMER_TIMER_OPENDUNE_H

#include "timer_common.h"

extern uint32 g_timerGUI;
extern uint32 g_timerInput;
extern uint32 g_timerSleep;
extern uint32 g_timerTimeout;

extern void TimerOpenDune_Init(void);
extern void TimerOpenDune_Uninit(void);

extern void TimerOpenDune_Add(void (*callback)(void), uint32 usec_delay);
extern void TimerOpenDune_Change(void (*callback)(void), uint32 usec_delay);
extern void TimerOpenDune_Remove(void (*callback)(void));

extern bool TimerOpenDune_SetTimer(enum TimerType timer, bool set);
extern void TimerOpenDune_Sleep(uint16 ticks);

#endif
