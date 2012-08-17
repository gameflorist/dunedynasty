#ifndef NEWUI_ACTIONPANEL_H
#define NEWUI_ACTIONPANEL_H

#include "../shape.h"
#include "../structure.h"

extern void ActionPanel_DrawPortrait(uint16 action_type, enum ShapeID shapeID);
extern void ActionPanel_DrawHealthBar(int curr, int max);
extern void ActionPanel_DrawStructureDescription(Structure *s);
extern void ActionPanel_DrawActionDescription(uint16 stringID, int x, int y, uint8 fg);
extern void ActionPanel_DrawMissileCountdown(uint8 fg, int count);

#endif
