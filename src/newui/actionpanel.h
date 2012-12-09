#ifndef NEWUI_ACTIONPANEL_H
#define NEWUI_ACTIONPANEL_H

#include "../gui/widget.h"
#include "../shape.h"
#include "../structure.h"

enum {
	/* 18 units or 20 structures. */
	MAX_FACTORY_WINDOW_ITEMS = 25
};

typedef struct FactoryWindowItem {
	int objectType;

	/* -2 = prerequisites lost.
	 * -1 = upgrade required.
	 *  0 = not available.
	 * 1+ = available.
	 */
	int available;

	int credits;
	int sortPriority;
	enum ShapeID shapeID;
} FactoryWindowItem;

extern FactoryWindowItem g_factoryWindowItems[MAX_FACTORY_WINDOW_ITEMS];
extern int g_factoryWindowTotal;

extern void ActionPanel_HighlightIcon(enum HouseType houseID, int x1, int y1, bool large_icon);
extern void ActionPanel_DrawPortrait(uint16 action_type, enum ShapeID shapeID);
extern void ActionPanel_DrawHealthBar(int curr, int max);
extern void ActionPanel_DrawStructureDescription(Structure *s);
extern void ActionPanel_DrawActionDescription(uint16 stringID, int x, int y, uint8 fg);
extern void ActionPanel_DrawMissileCountdown(uint8 fg, int count);
extern void ActionPanel_DrawFactory(const Widget *widget, Structure *s);
extern void ActionPanel_DrawPalace(const Widget *w, Structure *s);
extern void ActionPanel_BeginPlacementMode(Structure *construction_yard);
extern bool ActionPanel_ClickFactory(const Widget *widget, Structure *s);
extern void ActionPanel_ClickStarportOrder(Structure *s);
extern bool ActionPanel_ClickStarport(const Widget *widget, Structure *s);
extern bool ActionPanel_ClickPalace(const Widget *widget, Structure *s);

#endif
