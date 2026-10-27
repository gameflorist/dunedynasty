#ifndef BRUTAL_AI_H
#define BRUTAL_AI_H

#include <stdio.h>
#include "house.h"
#include "structure.h"
#include "unit.h"

extern bool AI_IsBrutalAI(enum HouseType houseID);

extern uint16 StructureAI_PickNextToBuild(const Structure *s);

extern bool UnitAI_CallCarryallToEvadeSandworm(const Unit *harvester);
extern uint16 UnitAI_GetAnyEnemyInRange(const Unit *unit);
extern bool UnitAI_ShouldDestructDevastator(const Unit *devastator);

extern void UnitAI_ClearSquads(void);
extern bool AI_IsInOpeningPeriod(void);
extern enum UnitActionType UnitAI_OpeningAction(const Unit *u, enum UnitActionType action);
extern void UnitAI_HarvesterAttacked(Unit *harvester, const Unit *attacker);
extern void UnitAI_RegisterGuardLoss(const Unit *unit);
extern void UnitAI_ClaimGuardPost(const Unit *unit);
extern void UnitAI_DetachFromSquad(Unit *unit);
extern void UnitAI_AbortMission(Unit *unit, uint16 enemy);
extern uint16 UnitAI_GetSquadDestination(Unit *unit, uint16 destination);
extern void UnitAI_SquadLoop(void);

extern bool BrutalAI_Load(FILE *fp, uint32 length);
extern bool BrutalAI_Save(FILE *fp);

#endif
