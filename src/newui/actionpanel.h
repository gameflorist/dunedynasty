#ifndef NEWUI_ACTIONPANEL_H
#define NEWUI_ACTIONPANEL_H

#include "../shape.h"

extern void ActionPanel_DrawPortrait(uint16 action_type, enum ShapeID shapeID);
extern void ActionPanel_DrawHealthBar(int curr, int max);

#endif
