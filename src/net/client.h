#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "types.h"

struct Object;

extern void Client_Send_RepairUpgradeStructure(const struct Object *o);
extern void Client_Send_SetRallyPoint(const struct Object *o, uint16 packed);
extern void Client_Send_EnterPlacementMode(const struct Object *o);
extern void Client_Send_LeavePlacementMode(const struct Object *o);
extern void Client_Send_PlaceStructure(uint16 packed);
extern void Client_Send_IssueUnitAction(uint8 actionID, uint16 encoded, const struct Object *o);

extern void Client_ChangeSelectionMode(void);

#endif
