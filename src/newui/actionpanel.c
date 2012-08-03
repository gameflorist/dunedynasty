/* actionpanel.c */

#include <assert.h>
#include "../os/math.h"

#include "actionpanel.h"

#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../input/mouse.h"
#include "../pool/house.h"
#include "../pool/structure.h"
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

static void
ActionPanel_ProductionButtonDimensions(const Widget *widget, int item,
		int *x1, int *y1, int *x2, int *y2, int *w, int *h)
{
	const int height = 35;
	const int y = widget->offsetY + (height + 1) * item;

	if (x1 != NULL) *x1 = widget->offsetX;
	if (y1 != NULL) *y1 = y;
	if (x2 != NULL) *x2 = widget->offsetX + widget->width - 1;
	if (y2 != NULL) *y2 = y + height - 1;
	if (w != NULL) *w = widget->width;
	if (h != NULL) *h = height;
}

bool
ActionPanel_ClickFactory(const Widget *widget, Structure *s)
{
	int x1, y1, x2, h;
	int item;

	if (s->o.flags.s.upgrading)
		return false;

	ActionPanel_ProductionButtonDimensions(widget, 0, &x1, &y1, &x2, NULL, NULL, &h);
	if (!(x1 <= g_mouseX && g_mouseX <= x2))
		return false;

	Structure_GetBuildable(s);
	GUI_FactoryWindow_InitItems(s->o.type);

	item = (g_mouseY - y1) / (h + 1);
	if (!(0 <= item && item < g_factoryWindowTotal))
		return false;

	if (g_factoryWindowItems[item].objectInfo->available <= 0)
		return false;

	const bool lmb = (widget->state.s.buttonState & 0x04);
	const bool rmb = (widget->state.s.buttonState & 0x40);

	switch (g_productionStringID) {
		case STR_PLACE_IT:
			if (s->objectType == g_factoryWindowItems[item].objectType) {
				if (lmb) {
					Structure *ns;

					ns = Structure_Get_ByIndex(s->o.linkedID);
					g_structureActive = ns;
					g_structureActiveType = s->objectType;
					g_selectionState = Structure_IsValidBuildLocation(g_selectionRectanglePosition, g_structureActiveType);
					g_structureActivePosition = g_selectionPosition;
					s->o.linkedID = STRUCTURE_INVALID;

					GUI_ChangeSelectionType(SELECTIONTYPE_PLACE);
				}
				else if (rmb) {
					Structure_CancelBuild(s);
				}
			}
			break;

		case STR_ON_HOLD:
			if (s->objectType == g_factoryWindowItems[item].objectType) {
				if (lmb) {
					s->o.flags.s.repairing = false;
					s->o.flags.s.onHold    = false;
					s->o.flags.s.upgrading = false;
				}
				else if (rmb) {
					Structure_CancelBuild(s);
				}
			}
			break;

		case STR_BUILD_IT:
			if (lmb) {
				s->objectType = g_factoryWindowItems[item].objectType;
				Structure_BuildObject(s, s->objectType);
			}
			break;

		case STR_D_DONE:
			if (s->objectType == g_factoryWindowItems[item].objectType) {
				if (rmb) {
					s->o.flags.s.onHold = true;
				}
			}
			break;

		default:
			break;
	}

	return false;
}

static void
ActionPanel_DrawStructureLayout(uint16 type, int x1, int y1)
{
	const StructureInfo *si = &g_table_structureInfo[type];
	const int lw = g_table_structure_layoutSize[si->layout].width;
	const int lh = g_table_structure_layoutSize[si->layout].height;
	const int size = Shape_Width(SHAPE_STRUCTURE_LAYOUT_BLOCK) + 1;

	/* XXX: when button down, need to invert colours? */
	Shape_Draw(SHAPE_STRUCTURE_LAYOUT_OUTLINE, x1 + 37, y1 + 5, 0, 0);

	for (int ly = 0; ly < lh; ly++) {
		for (int lx = 0; lx < lw; lx++) {
			Shape_Draw(SHAPE_STRUCTURE_LAYOUT_BLOCK, x1 + size*lx + 38, y1 + size*ly + 6, 0, 0);
		}
	}
}

void
ActionPanel_DrawFactory(const Widget *widget, Structure *s)
{
	if (g_productionStringID == STR_UPGRADINGD_DONE) {
		const int percentDone = 100 - s->upgradeTimeLeft;
		int x1, y1, h;

		ActionPanel_ProductionButtonDimensions(widget, 0, &x1, &y1, NULL, NULL, NULL, &h);

		GUI_DrawText_Wrapper(String_Get_ByIndex(g_productionStringID), x1, y1 + h - 19, 0xF, 0, 0x021, percentDone);
		return;
	}

	Structure_GetBuildable(s);
	GUI_FactoryWindow_InitItems(s->o.type);

	for (int item = 0; item < g_factoryWindowTotal; item++) {
		const ObjectInfo *oi = g_factoryWindowItems[item].objectInfo;
		const uint16 object_type = g_factoryWindowItems[item].objectType;
		const int icon_w = Shape_Width(oi->spriteID);
		const int icon_h = Shape_Height(oi->spriteID);

		int x1, y1, x2, y2, w, h;
		bool buttonDown = false;
		int fg = 0xF;

		ActionPanel_ProductionButtonDimensions(widget, item, &x1, &y1, &x2, &y2, &w, &h);

		if ((x1 <= g_mouseX && g_mouseX <= x2) &&
		    (y1 <= g_mouseY && g_mouseY <= y2) && (widget->state.s.hover2)) {
			buttonDown = true;
			fg = 0xE;
		}

		GUI_DrawWiredRectangle(x1 - 1, y1 - 1, x2 + 1, y2 + 1, 12);
		GUI_DrawBorder(x1, y1, w, h, buttonDown ? 0 : 1, true);

		if (oi->available < 0) {
			Shape_DrawGrey(oi->spriteID, x1 + 2, y1 + 2, 0, 0);
		}
		else {
			Shape_Draw(oi->spriteID, x1 + 2, y1 + 2, 0, 0);
		}

		/* Draw layout. */
		if (s->o.type == STRUCTURE_CONSTRUCTION_YARD)
			ActionPanel_DrawStructureLayout(object_type, x1, y1);

		/* Production Status. */
		if (s->objectType == object_type) {
			if (g_productionStringID == STR_D_DONE) {
				int buildTime;

				if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
					const StructureInfo *si = &g_table_structureInfo[s->objectType];

					buildTime = si->o.buildTime;
				}
				else {
					const UnitInfo *ui = &g_table_unitInfo[s->objectType];

					buildTime = ui->o.buildTime;
				}

				const int timeLeft = buildTime - (s->countDown + 255) / 256;
				const int percentDone = 100 * timeLeft / buildTime;

				GUI_DrawText_Wrapper("%d%%", x1 + 2 + icon_w/2, y1 + icon_h/2, fg, 0, 0x121, percentDone);
			}
			else if (g_productionStringID != STR_BUILD_IT) {
				GUI_DrawText_Wrapper(String_Get_ByIndex(g_productionStringID), x1 + w/2, y1 + icon_h/2, fg, 0, 0x121);
			}
		}

		GUI_DrawText_Wrapper("%d", x1 + 2 + icon_w/2, y1 + h - 8, 29, 0, 0x111, oi->buildCredits);
		GUI_DrawText_Wrapper("%d", x1 + w - 4, y1 + h - 9, fg, 0, 0x221, 0);
	}
}
