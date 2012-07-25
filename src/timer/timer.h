#ifndef TIMER_TIMER_H
#define TIMER_TIMER_H

#include "timer_common.h"

#include "timer_a5.h"

#define Timer_SetTimer      TimerA5_SetTimer
#define Timer_Wait()        TimerA5_Sleep(1)
#define Timer_Sleep         TimerA5_Sleep
#define Timer_GameTicks()   TimerA5_GetTicks(TIMER_GAME)
#define Timer_GetTicks()    TimerA5_GetTicks(TIMER_GUI)

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
