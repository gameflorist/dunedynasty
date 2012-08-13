#ifndef NEWUI_ACTIONPANEL_H
#define NEWUI_ACTIONPANEL_H

#include "../gui/widget.h"
#include "../shape.h"
#include "../structure.h"

extern void ActionPanel_DrawPortrait(uint16 action_type, enum ShapeID shapeID);
extern void ActionPanel_DrawHealthBar(int curr, int max);
extern void ActionPanel_DrawStructureDescription(Structure *s);
extern void ActionPanel_DrawActionDescription(uint16 stringID, int x, int y, uint8 fg);
extern void ActionPanel_DrawMissileCountdown(uint8 fg, int count);
extern void ActionPanel_DrawFactory(const Widget *widget, Structure *s);
extern void ActionPanel_DrawPalace(const Widget *w, Structure *s);
extern bool ActionPanel_ClickFactory(const Widget *widget, Structure *s);
extern bool ActionPanel_ClickStarport(const Widget *widget, Structure *s);

#endif
