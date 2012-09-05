#ifndef BRUTAL_AI_H
#define BRUTAL_AI_H

#include "house.h"
#include "structure.h"
#include "unit.h"

extern bool AI_IsBrutalAI(enum HouseType houseID);

extern uint32 StructureAI_FilterBuildOptions(enum StructureType s, enum HouseType houseID, uint32 buildable);
extern uint32 StructureAI_FilterBuildOptions_Original(enum StructureType s, enum HouseType houseID, uint32 buildable);

extern bool UnitAI_CallCarryallToEvadeSandworm(const Unit *harvester);

#endif
