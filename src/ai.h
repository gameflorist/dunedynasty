#ifndef BRUTAL_AI_H
#define BRUTAL_AI_H

#include "house.h"
#include "structure.h"
#include "unit.h"

extern bool AI_IsBrutalAI(enum HouseType houseID);

extern uint32 StructureAI_FilterBuildOptions(enum StructureType s, enum HouseType houseID, uint32 buildable);
extern uint32 StructureAI_FilterBuildOptions_Original(enum StructureType s, enum HouseType houseID, uint32 buildable);

extern bool UnitAI_CallCarryallToEvadeSandworm(const Unit *harvester);
extern uint16 UnitAI_GetAnyEnemyInRange(const Unit *unit);
extern bool UnitAI_ShouldDestructDevastator(const Unit *devastator);

extern void UnitAI_ClearSquads(void);
extern void UnitAI_DetachFromSquad(Unit *unit);
extern void UnitAI_AbortMission(Unit *unit, uint16 enemy);
extern uint16 UnitAI_GetSquadDestination(Unit *unit, uint16 destination);
extern void UnitAI_SquadLoop(void);

#endif
