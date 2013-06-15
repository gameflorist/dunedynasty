#ifndef TIMER_TIMER_H
#define TIMER_TIMER_H

#include <stdbool.h>
#include <stdint.h>
#include "types.h"

enum TimerType {
	TIMER_GUI   = 0,
	TIMER_GAME  = 1
};

#define Timer_GameTicks()   Timer_GetTimer(TIMER_GAME)
#define Timer_GetTicks()    Timer_GetTimer(TIMER_GUI)

extern int64_t g_timerGame;
extern int64_t g_tickScenarioStart;

extern int64_t g_tickHousePowerMaintenance;
extern int64_t g_tickHouseHouse;
extern int64_t g_tickHouseStarport;
extern int64_t g_tickHouseReinforcement;
extern int64_t g_tickHouseMissileCountdown;
extern int64_t g_tickHouseStarportAvailability;
extern int64_t g_tickHouseStarportRecalculatePrices;

extern int64_t g_tickStructureDegrade;
extern int64_t g_tickStructureStructure;
extern int64_t g_tickStructureScript;
extern int64_t g_tickStructurePalace;

extern int64_t g_tickTeamGameLoop;

extern int64_t g_tickUnitMovement;
extern int64_t g_tickUnitRotation;
extern int64_t g_tickUnitBlinking;
extern int64_t g_tickUnitUnknown4;
extern int64_t g_tickUnitScript;
extern int64_t g_tickUnitUnknown5;
extern int64_t g_tickUnitDeviation;

extern void Timer_ResetScriptTimers(void);
extern uint16 Tools_AdjustToGameSpeed(uint16 normal, uint16 minimum, uint16 maximum, bool inverseSpeed);
extern double Timer_GetUnitMovementFrame(void);
extern double Timer_GetUnitRotationFrame(void);

extern bool Timer_SetTimer(enum TimerType timer, bool set);
extern int64_t Timer_GetTimer(enum TimerType timer);
extern void Timer_Sleep(int tics);
extern void Timer_RegisterSource(void);
extern void Timer_UnregisterSource(void);
extern void Timer_WaitForEvent(void);
extern bool Timer_QueueIsEmpty(void);

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
