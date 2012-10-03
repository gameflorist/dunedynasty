#ifndef TIMER_TIMER_H
#define TIMER_TIMER_H

#include <stdbool.h>
#include <stdint.h>

enum TimerType {
	TIMER_GUI   = 0,
	TIMER_GAME  = 1
};

#define Timer_Wait()        Timer_Sleep(1)
#define Timer_GameTicks()   Timer_GetTimer(TIMER_GAME)
#define Timer_GetTicks()    Timer_GetTimer(TIMER_GUI)

extern int64_t g_timerGame;

extern bool Timer_SetTimer(enum TimerType timer, bool set);
extern int64_t Timer_GetTimer(enum TimerType timer);
extern void Timer_Sleep(int tics);

#if 0
#include "timer_opendune.h"

#define Timer_Init          TimerOpenDune_Init
#define Timer_Uninit        TimerOpenDune_Uninit
#define Timer_SetTimer      TimerOpenDune_SetTimer
#define Timer_GameTicks()   g_timerGame
#define Timer_GetTicks()    g_timerGUI
#define Timer_Wait()        TimerOpenDune_Sleep(1)
#define Timer_Sleep         TimerOpenDune_Sleep
#endif

#endif
