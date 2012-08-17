/* actionpanel.c */

#include <assert.h>
#include "../os/math.h"

#include "actionpanel.h"

#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../pool/house.h"
#include "../string.h"
#include "../table/strings.h"
#include "../unit.h"
#include "../video/video.h"

void
ActionPanel_DrawPortrait(uint16 action_type, enum ShapeID shapeID)
{
	switch (action_type) {
		case 2: /* Unit */
		case 3: /* Structure */
		case 7: /* Placement. */
			break;

		case 4: /* Attack */
			shapeID = SHAPE_ATTACK;
			break;

		case 5: /* Movement */
		case 6: /* Harvest */
			shapeID = SHAPE_MOVE;
			break;

		case 8: /* House Missile */
			shapeID = SHAPE_DEATH_HAND;
			break;

		default:
			return;
	}

	Shape_Draw(shapeID, 2, 9, WINDOWID_ACTIONPANEL_FRAME, 0x4000);
}

void
ActionPanel_DrawHealthBar(int curr, int max)
{
	const WidgetProperties *wi = &g_widgetProperties[WINDOWID_ACTIONPANEL_FRAME];

	if (curr > max)
		curr = max;

	if (max < 1)
		max = 1;

	const int x = wi->xBase*8 + 37;
	const int y = wi->yBase + 10;
	const int w = max(1, 24 * curr / max);
	const int h = 7;

	uint8 colour = 4;
	if (curr <= max / 2) colour = 5;
	if (curr <= max / 4) colour = 8;

	GUI_DrawBorder(x - 1, y - 1, 24 + 2, h + 2, 1, true);
	GUI_DrawFilledRectangle(x, y, x + w - 1, y + h - 1, colour);

	Shape_Draw(SHAPE_HEALTH_INDICATOR, 36, 18, WINDOWID_ACTIONPANEL_FRAME, 0x4000);
	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_DMG), wi->xBase*8 + 40, wi->yBase + 23, 29, 0, 0x11);
}

void
ActionPanel_DrawStructureDescription(Structure *s)
{
	const WidgetProperties *wi = &g_widgetProperties[WINDOWID_ACTIONPANEL_FRAME];
	const int x0 = (wi->xBase - 32)*8;
	const int y0 = (wi->yBase - 42);

	const StructureInfo *si = &g_table_structureInfo[s->o.type];
	const Object *o = &s->o;
	const ObjectInfo *oi = &si->o;
	const House *h = House_Get_ByIndex(s->o.houseID);

	switch (o->type) {
		case STRUCTURE_SLAB_1x1:
		case STRUCTURE_SLAB_2x2:
		case STRUCTURE_PALACE:
		case STRUCTURE_LIGHT_VEHICLE:
		case STRUCTURE_HEAVY_VEHICLE:
		case STRUCTURE_HIGH_TECH:
		case STRUCTURE_HOUSE_OF_IX:
		case STRUCTURE_WOR_TROOPER:
		case STRUCTURE_CONSTRUCTION_YARD:
		case STRUCTURE_BARRACKS:
		case STRUCTURE_WALL:
		case STRUCTURE_TURRET:
		case STRUCTURE_ROCKET_TURRET:
			return;

		case STRUCTURE_REPAIR:
			{
				uint16 percent;
				uint16 steps;
				Unit *u;

				u = Structure_GetLinkedUnit(s);
				if (u == NULL) break;

				steps = g_table_unitInfo[u->o.type].o.buildTime / 4;
				percent = (steps - (s->countDown >> 8)) * 100 / steps;

				Shape_Draw(g_table_unitInfo[u->o.type].o.spriteID, x0 + 260, y0 + 89, 0, 0);
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_D_DONE), x0 + 258, y0 + 116, 29, 0, 0x11, percent);
			}
			break;

		case STRUCTURE_WINDTRAP:
			{
				uint16 powerOutput = o->hitpoints * -si->powerUsage / oi->hitpoints;
				uint16 powerAverage = (h->windtrapCount == 0) ? 0 : h->powerUsage / h->windtrapCount;

				GUI_DrawLine(x0 + 261, y0 + 95, x0 + 312, y0 + 95, 16);
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_POWER_INFONEEDEDOUTPUT), x0 + 258, y0 + 88, 29, 0, 0x11);
				GUI_DrawText_Wrapper("%d", x0 + 302, y0 + g_fontCurrent->height * 2 + 80, 29, 0, 0x11, powerAverage);
				GUI_DrawText_Wrapper("%d", x0 + 302, y0 + g_fontCurrent->height * 3 + 80, (powerOutput >= powerAverage) ? 29 : 6, 0, 0x11, powerOutput);
			}
			break;

		case STRUCTURE_STARPORT:
			if (h->starportLinkedID != 0xFFFF) {
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_FRIGATEARRIVAL_INTMINUS_D), x0 + 258, y0 + 88, 29, 0, 0x11, h->starportTimeLeft);
			}
			else {
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_FRIGATE_INORBIT_ANDAWAITINGORDER), x0 + 258, y0 + 88, 29, 0, 0x11);
			}
			break;

		case STRUCTURE_REFINERY:
		case STRUCTURE_SILO:
			{
				uint16 creditsStored;

				creditsStored = h->credits * si->creditsStorage / h->creditsStorage;
				if (h->credits > h->creditsStorage) creditsStored = si->creditsStorage;

				GUI_DrawLine(x0 + 261, y0 + 95, x0 + 312, y0 + 95, 16);
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_SPICEHOLDS_4DMAX_4D), x0 + 258, y0 + 88, 29, 0, 0x11, creditsStored, (si->creditsStorage <= 1000) ? si->creditsStorage : 1000);
			}
			break;

		case STRUCTURE_OUTPOST:
			GUI_DrawLine(x0 + 261, y0 + 95, x0 + 312, y0 + 95, 16);
			GUI_DrawText_Wrapper(String_Get_ByIndex(STR_RADAR_SCANFRIEND_2DENEMY_2D), x0 + 258, y0 + 88, 29, 0, 0x11, h->unitCountAllied, h->unitCountEnemy);
			break;
	}
}

void
ActionPanel_DrawActionDescription(uint16 stringID, int x, int y, uint8 fg)
{
	const WidgetProperties *wi = &g_widgetProperties[WINDOWID_ACTIONPANEL_FRAME];
	const int x0 = (wi->xBase - 32)*8;
	const int y0 = (wi->yBase - 42);

	GUI_DrawText_Wrapper(String_Get_ByIndex(stringID), x0 + x, y0 + y, fg, 0, 0x11);
}

void
ActionPanel_DrawMissileCountdown(uint8 fg, int count)
{
	const WidgetProperties *wi = &g_widgetProperties[WINDOWID_ACTIONPANEL_FRAME];
	const int x0 = (wi->xBase - 32)*8;
	const int y0 = (wi->yBase - 42);

	if (count <= 0)
		count = 0;

	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_PICK_TARGETTMINUS_D), x0 + 259, y0 + 84, fg, 0, 0x11, count);
}
