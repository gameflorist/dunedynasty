/* actionpanel.c */

#include <assert.h>
#include <math.h>
#include "../os/math.h"

#include "actionpanel.h"

#include "../audio/audio.h"
#include "../config.h"
#include "../enhancement.h"
#include "../gfx.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../opendune.h"
#include "../pool/house.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../string.h"
#include "../table/strings.h"
#include "../unit.h"
#include "../video/video.h"

FactoryWindowItem g_factoryWindowItems[MAX_FACTORY_WINDOW_ITEMS];
int g_factoryWindowTotal;

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

	const int x = wi->xBase + 37;
	const int y = wi->yBase + 10;
	const int w = max(1, 24 * curr / max);
	const int h = 7;

	uint8 colour = 4;
	if (curr <= max / 2) colour = 5;
	if (curr <= max / 4) colour = 8;

	Prim_DrawBorder(x - 1, y - 1, 24 + 2, h + 2, 1, false, true, 1);
	Prim_FillRect_i(x, y, x + w - 1, y + h - 1, colour);

	Shape_Draw(SHAPE_HEALTH_INDICATOR, 36, 18, WINDOWID_ACTIONPANEL_FRAME, 0x4000);
	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_DMG), wi->xBase + 40, wi->yBase + 23, 29, 0, 0x11);
}

void
ActionPanel_DrawStructureDescription(Structure *s)
{
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

				Shape_Draw(g_table_unitInfo[u->o.type].o.spriteID, 20, 49, 0, 0);
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_D_DONE), 18, 76, 29, 0, 0x11, percent);
			}
			break;

		case STRUCTURE_WINDTRAP:
			{
				uint16 powerOutput = o->hitpoints * -si->powerUsage / oi->hitpoints;
				uint16 powerAverage = (h->windtrapCount == 0) ? 0 : h->powerUsage / h->windtrapCount;
				uint8 fg = (powerOutput >= powerAverage) ? 29 : 6;

				Prim_Hline(21, 55, 72, 16);
				if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
					GUI_DrawText_Wrapper("Power Info", 46, 48, 29, 0, 0x111);
					GUI_DrawText_Wrapper("Needed:", 22, g_fontCurrent->height * 2 + 40, 29, 0, 0x11);
					GUI_DrawText_Wrapper("Output:", 22, g_fontCurrent->height * 3 + 40, 29, 0, 0x11);
					GUI_DrawText_Wrapper("%d", 71, g_fontCurrent->height * 2 + 40, 29, 0, 0x211, powerAverage);
					GUI_DrawText_Wrapper("%d", 71, g_fontCurrent->height * 3 + 40, fg, 0, 0x211, powerOutput);
				}
				else {
					GUI_DrawText_Wrapper(String_Get_ByIndex(STR_POWER_INFONEEDEDOUTPUT), 18, 48, 29, 0, 0x11);
					GUI_DrawText_Wrapper("%d", 62, g_fontCurrent->height * 2 + 40, 29, 0, 0x11, powerAverage);
					GUI_DrawText_Wrapper("%d", 62, g_fontCurrent->height * 3 + 40, fg, 0, 0x11, powerOutput);
				}
			}
			break;

		case STRUCTURE_STARPORT:
			if (h->starportLinkedID != 0xFFFF) {
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_FRIGATEARRIVAL_INTMINUS_D), 18, 48, 29, 0, 0x11, h->starportTimeLeft);
			}
			else {
				/* GUI_DrawText_Wrapper(String_Get_ByIndex(STR_FRIGATE_INORBIT_ANDAWAITINGORDER), 18, 48, 29, 0, 0x11); */
			}
			break;

		case STRUCTURE_REFINERY:
		case STRUCTURE_SILO:
			{
				uint16 creditsStored;

				creditsStored = h->credits * si->creditsStorage / h->creditsStorage;
				if (h->credits > h->creditsStorage) creditsStored = si->creditsStorage;

				Prim_Hline(21, 55, 72, 16);
				if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
					GUI_DrawText_Wrapper("Spice", 46, 48, 29, 0, 0x111);
					GUI_DrawText_Wrapper("Holds:", 22, g_fontCurrent->height * 2 + 40, 29, 0, 0x11);
					GUI_DrawText_Wrapper("Max:", 22, g_fontCurrent->height * 3 + 40, 29, 0, 0x11);
					GUI_DrawText_Wrapper("%d", 71, g_fontCurrent->height * 2 + 40, 29, 0, 0x211, creditsStored);
					GUI_DrawText_Wrapper("%d", 71, g_fontCurrent->height * 3 + 40, 29, 0, 0x211, (si->creditsStorage <= 1000) ? si->creditsStorage : 1000);
				}
				else {
					GUI_DrawText_Wrapper(String_Get_ByIndex(STR_SPICEHOLDS_4DMAX_4D), 18, 48, 29, 0, 0x11, creditsStored, (si->creditsStorage <= 1000) ? si->creditsStorage : 1000);
				}
			}
			break;

		case STRUCTURE_OUTPOST:
			Prim_Hline(21, 55, 72, 16);
			if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
				GUI_DrawText_Wrapper("Radar Scan", 46, 48, 29, 0, 0x111);
				GUI_DrawText_Wrapper("Friend:", 22, g_fontCurrent->height * 2 + 40, 29, 0, 0x11);
				GUI_DrawText_Wrapper("Enemy:", 22, g_fontCurrent->height * 3 + 40, 29, 0, 0x11);
				GUI_DrawText_Wrapper("%d", 71, g_fontCurrent->height * 2 + 40, 29, 0, 0x211, h->unitCountAllied);
				GUI_DrawText_Wrapper("%d", 71, g_fontCurrent->height * 3 + 40, 29, 0, 0x211, h->unitCountEnemy);
			}
			else {
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_RADAR_SCANFRIEND_2DENEMY_2D), 18, 48, 29, 0, 0x11, h->unitCountAllied, h->unitCountEnemy);
			}
			break;
	}
}

void
ActionPanel_DrawActionDescription(uint16 stringID, int x, int y, uint8 fg)
{
	GUI_DrawText_Wrapper(String_Get_ByIndex(stringID), x, y, fg, 0, 0x11);
}

void
ActionPanel_DrawMissileCountdown(uint8 fg, int count)
{
	if (count <= 0)
		count = 0;

	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_PICK_TARGETTMINUS_D), 19, 44, fg, 0, 0x11, count);
}

static int
ActionPanel_ProductionButtonStride(const Widget *widget, bool is_starport, int *ret_items_per_screen)
{
	const int h = widget->height
		- 3  /* top margin */
		- (widget->height >= 58 ? 21 : 5) /* arrows */
		- (is_starport ? 12 : 0);

	const int items_per_screen = h / 58;

	/* Try to squeeze in 1 more item. */
	int reth = h / (items_per_screen + 1);
	if (reth < 52)
		reth = 58;

	if (ret_items_per_screen != NULL)
		*ret_items_per_screen = h / reth;

	return reth;
}

static void
ActionPanel_ProductionButtonDimensions(const Widget *widget, const Structure *s,
		int item, int *x1, int *y1, int *x2, int *y2, int *w, int *h)
{
	int items_per_screen;
	const bool is_starport = (s->o.type == STRUCTURE_STARPORT);
	const int stride = ActionPanel_ProductionButtonStride(widget, is_starport, &items_per_screen);
	const int width = 52;
	const int height = 39;
	const int x = widget->offsetX + 4;
	int y = widget->offsetY + 16 + stride * item + s->factoryOffsetY;

	if (items_per_screen <= 1) {
		y = widget->offsetY + widget->height - height
			- (widget->height >= 58 ? 21 : 3)
			- (is_starport ? 12 : 0)
			+ stride * item + s->factoryOffsetY;
	}

	if (x1 != NULL) *x1 = x;
	if (y1 != NULL) *y1 = y;
	if (x2 != NULL) *x2 = x + width - 1;
	if (y2 != NULL) *y2 = y + height - 1;
	if (w != NULL) *w = width;
	if (h != NULL) *h = height;
}

static void
ActionPanel_ScrollButtonDimensions(const Widget *widget, int height, bool up,
		int *x1, int *y1, int *x2, int *y2)
{
	int x, y;

	if (widget->height < 58) {
		x = widget->offsetX + widget->width - 24 - 1;
		y = widget->offsetY + height / 2 + 1 - (up ? 15 : 0);
	}
	else {
		x = widget->offsetX + (up ? 5 : 31);
		y = widget->offsetY + height - 18;
	}

	if (x1 != NULL) *x1 = x;
	if (y1 != NULL) *y1 = y;
	if (x2 != NULL) *x2 = x + 24 - 1;
	if (y2 != NULL) *y2 = y + 15 - 1;
}

static void
ActionPanel_SendOrderButtonDimensions(const Widget *widget,
		int *x1, int *y1, int *x2, int *y2, int *w, int *h)
{
	if (x1 != NULL)	*x1 = widget->offsetX;
	if (y1 != NULL)	*y1 = widget->offsetY + widget->height - 10;
	if (x2 != NULL)	*x2 = widget->offsetX + widget->width - 1;
	if (y2 != NULL)	*y2 = widget->offsetY + widget->height - 1;
	if (w != NULL) *w = widget->width;
	if (h != NULL) *h = 10;
}

static void
ActionPanel_ClampFactoryScrollOffset(const Widget *widget, Structure *s)
{
	int items_per_screen;
	const bool is_starport = (s->o.type == STRUCTURE_STARPORT);
	const int stride = ActionPanel_ProductionButtonStride(widget, is_starport, &items_per_screen);
	const int first_item = g_factoryWindowTotal - (items_per_screen <= 1 ? 1 : items_per_screen);
	int y1;

	ActionPanel_ProductionButtonDimensions(widget, s, first_item, NULL, &y1, NULL, NULL, NULL, NULL);

	if (y1 < widget->offsetY + 16)
		s->factoryOffsetY = -stride * first_item;

	if (s->factoryOffsetY > 0)
		s->factoryOffsetY = 0;
}

static bool
ActionPanel_ScrollFactory(const Widget *widget, Structure *s)
{
	const bool is_starport = (s->o.type == STRUCTURE_STARPORT);
	const int height = (is_starport ? widget->height - 12 : widget->height);
	const int stride = ActionPanel_ProductionButtonStride(widget, is_starport, NULL);

	int delta = g_mouseDZ;
	int x1, y1, x2, y2;

	ActionPanel_ScrollButtonDimensions(widget, height, true, &x1, &y1, &x2, &y2);
	if ((widget->state.s.buttonState & 0x44) && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2))
		delta = 1;

	ActionPanel_ScrollButtonDimensions(widget, height, false, &x1, &y1, &x2, &y2);
	if ((widget->state.s.buttonState & 0x44) && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2))
		delta = -1;

	if (delta == 0)
		return false;

	s->factoryOffsetY += delta * stride;
	ActionPanel_ClampFactoryScrollOffset(widget, s);

	return true;
}

void
ActionPanel_BeginPlacementMode(Structure *construction_yard)
{
	Structure *ns = Structure_Get_ByIndex(construction_yard->o.linkedID);

	g_structureActive = ns;
	g_structureActiveType = construction_yard->objectType;
	g_selectionState = Structure_IsValidBuildLocation(g_selectionRectanglePosition, g_structureActiveType);
	g_structureActivePosition = g_selectionPosition;
	construction_yard->o.linkedID = STRUCTURE_INVALID;

	GUI_ChangeSelectionType(SELECTIONTYPE_PLACE);
}

bool
ActionPanel_ClickFactory(const Widget *widget, Structure *s)
{
	const int height = widget->height;

	int x1, y1, x2, y2;
	int item;

	if (s->o.flags.s.upgrading)
		return false;

	if (widget->state.s.keySelected) {
		if (g_productionStringID == STR_PLACE_IT)
			ActionPanel_BeginPlacementMode(s);
		return true;
	}

	if (g_factoryWindowTotal < 0) {
		Structure_InitFactoryItems(s);
		ActionPanel_ClampFactoryScrollOffset(widget, s);
	}

	if (ActionPanel_ScrollFactory(widget, s))
		return true;

	int mouseY;
	Mouse_TransformToDiv(widget->div, NULL, &mouseY);

	if (mouseY >= widget->offsetY + height - (widget->height >= 58 ? 21 : 5))
		return false;

	const bool lmb = (widget->state.s.buttonState & 0x04);
	const bool rmb = (widget->state.s.buttonState & 0x40);

	for (item = 0; item < g_factoryWindowTotal; item++) {
		ActionPanel_ProductionButtonDimensions(widget, s, item, &x1, &y1, &x2, &y2, NULL, NULL);
		if (Mouse_InRegion_Div(widget->div, x1, y1, x2, y2))
			break;
	}

	/* Upgrade required. */
	if (g_factoryWindowItems[item].available == -1) {
		if (item < g_factoryWindowTotal)
			Audio_PlaySound(EFFECT_ERROR_OCCURRED);

		return false;
	}

	const uint16 clicked_type = g_factoryWindowItems[item].objectType;
	bool action_successful = true;

	if ((s->objectType == clicked_type) || (g_productionStringID == STR_BUILD_IT)) {
		switch (g_productionStringID) {
			case STR_PLACE_IT:
			case STR_ON_HOLD:
				if (lmb && (g_productionStringID == STR_PLACE_IT)) {
					ActionPanel_BeginPlacementMode(s);
				}
				else if (lmb && (g_productionStringID == STR_ON_HOLD)) {
					s->o.flags.s.repairing = false;
					s->o.flags.s.onHold    = false;
					s->o.flags.s.upgrading = false;
				}
				else if (rmb) {
					const uint16 next_type = BuildQueue_RemoveHead(&s->queue);

					if (next_type == 0xFFFF) {
						Structure_CancelBuild(s);
					}
					else if (s->objectType != next_type) {
						Structure_BuildObject(s, next_type);
						s->o.flags.s.onHold = true;
					}
				}
				break;

			case STR_BUILD_IT:
				if (lmb) {
					s->objectType = g_factoryWindowItems[item].objectType;

					if (g_factoryWindowItems[item].available > 0) {
						action_successful = Structure_BuildObject(s, s->objectType);
					}
					else {
						action_successful = false;
					}
				}
				break;

			case STR_COMPLETED:
			case STR_D_DONE:
				if (lmb) {
					if (g_factoryWindowItems[item].available > 0) {
						BuildQueue_Add(&s->queue, clicked_type, 0);
					}
					else {
						action_successful = false;
					}
				}
				else if (rmb && (g_productionStringID == STR_D_DONE)) {
					s->o.flags.s.onHold = true;
				}
				break;

			default:
				break;
		}
	}
	else {
		if (lmb) {
			if (g_factoryWindowItems[item].available > 0) {
				BuildQueue_Add(&s->queue, clicked_type, 0);
			}
			else {
				action_successful = false;
			}
		}
		else if (rmb) {
			BuildQueue_RemoveTail(&s->queue, clicked_type, NULL);
		}
	}

	if (!action_successful) {
		Audio_PlaySound(EFFECT_ERROR_OCCURRED);
	}

	return false;
}

static void
ActionPanel_ClickStarportOrder(Structure *s)
{
	uint16 objectType;
	House *h = g_playerHouse;

	while ((objectType = BuildQueue_RemoveHead(&s->queue)) != 0xFFFF) {
		Unit *u;

		g_var_38BC++;
		{
			tile32 tile;
			tile.tile = 0xFFFFFFFF;
			u = Unit_Create(UNIT_INDEX_INVALID, (uint8)objectType, s->o.houseID, tile, 0);
		}
		g_var_38BC--;

		if (u == NULL) {
			/* XXX: What is going on here? */
			h->credits += g_table_unitInfo[UNIT_CARRYALL].o.buildCredits;
			if (s->o.houseID != g_playerHouseID)
				continue;

			GUI_DisplayText(String_Get_ByIndex(STR_UNABLE_TO_CREATE_MORE), 2);
			continue;
		}

		g_structureIndex = s->o.index;

		if (h->starportTimeLeft == 0) h->starportTimeLeft = g_table_houseInfo[h->index].starportDeliveryTime;

		u->o.linkedID = h->starportLinkedID & 0xFF;
		h->starportLinkedID = u->o.index;
	}
}

static void
ActionPanel_ClickStarportPlus(Structure *s, int entry)
{
	const FactoryWindowItem *item = &g_factoryWindowItems[entry];
	const uint16 type = item->objectType;

	House *h = g_playerHouse;

	if ((g_starportAvailable[type] > 0) && (item->credits <= h->credits)) {
		BuildQueue_Add(&s->queue, item->objectType, item->credits);

		if (g_starportAvailable[type] == 1) {
			g_starportAvailable[type] = -1;
		}
		else {
			g_starportAvailable[type]--;
		}

		h->credits -= item->credits;
	}
	else {
		Audio_PlaySound(EFFECT_ERROR_OCCURRED);
	}
}

static void
ActionPanel_ClickStarportMinus(Structure *s, int entry)
{
	const FactoryWindowItem *item = &g_factoryWindowItems[entry];
	const uint16 type = item->objectType;

	House *h = g_playerHouse;
	int credits;

	if (BuildQueue_RemoveTail(&s->queue, type, &credits)) {
		if (g_starportAvailable[type] == -1) {
			g_starportAvailable[type] = 1;
		}
		else {
			g_starportAvailable[type]++;
		}

		h->credits += credits;
	}
}

bool
ActionPanel_ClickStarport(const Widget *widget, Structure *s)
{
	const bool lmb = (widget->state.s.buttonState & 0x04);
	const bool rmb = (widget->state.s.buttonState & 0x40);
	const int height = widget->height - 12;

	int x1, y1, x2, y2;
	int item;

	if (g_factoryWindowTotal < 0) {
		Structure_InitFactoryItems(s);
		ActionPanel_ClampFactoryScrollOffset(widget, s);
	}

	ActionPanel_SendOrderButtonDimensions(widget, &x1, &y1, &x2, &y2, NULL, NULL);
	if (lmb && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2)) {
		ActionPanel_ClickStarportOrder(s);
		return true;
	}

	if (ActionPanel_ScrollFactory(widget, s))
		return true;

	int mouseY;
	Mouse_TransformToDiv(widget->div, NULL, &mouseY);

	if (mouseY >= widget->offsetY + height - (widget->height >= 58 ? 21 : 5))
		return false;

	for (item = 0; item < g_factoryWindowTotal; item++) {
		ActionPanel_ProductionButtonDimensions(widget, s, item, &x1, &y1, &x2, &y2, NULL, NULL);
		if (Mouse_InRegion_Div(widget->div, x1, y1, x2, y2))
			break;
	}

	if (!(0 <= item && item < g_factoryWindowTotal))
		return false;

	if (lmb) {
		ActionPanel_ClickStarportPlus(s, item);
	}
	else if (rmb) {
		ActionPanel_ClickStarportMinus(s, item);
	}

	return false;
}

bool
ActionPanel_ClickPalace(const Widget *widget, Structure *s)
{
	const bool lmb = (widget->state.s.buttonState & 0x04);
	int x1, y1, x2, y2;

	g_factoryWindowTotal = 0;

	ActionPanel_ProductionButtonDimensions(widget, s, 0, &x1, &y1, &x2, &y2, NULL, NULL);
	if (lmb && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2)) {
		Structure_ActivateSpecial(s);
		return true;
	}

	return false;
}

static void
ActionPanel_DrawStructureLayout(enum StructureType s, int x1, int y1)
{
	const StructureInfo *si = &g_table_structureInfo[s];
	const int lw = g_table_structure_layoutSize[si->layout].width;
	const int lh = g_table_structure_layoutSize[si->layout].height;
	const int size = 6;

	x1 += 31;
	y1 += 4;

	/* Use shadow effect for transluscent outline. */
	Shape_Draw(SHAPE_STRUCTURE_LAYOUT_OUTLINE, x1 - 1, y1 - 1, 0, 0x300);

	/* Shift layout to the right. */
	for (int ly = 0; ly < lh; ly++) {
		for (int lx = 3 - lw; lx < 3; lx++) {
			Shape_Draw(SHAPE_STRUCTURE_LAYOUT_BLOCK, x1 + size * lx, y1 + size * ly, 0, 0);
		}
	}
}

static void
ActionPanel_DrawScrollButtons(const Widget *widget, bool is_starport)
{
	const int height = (is_starport ? widget->height - 12 : widget->height);
	int items_per_screen;

	ActionPanel_ProductionButtonStride(widget, is_starport, &items_per_screen);
	if (g_factoryWindowTotal <= items_per_screen)
		return;

	const bool pressed = (widget->state.s.hover1);
	int x1, y1, x2, y2;

	ActionPanel_ScrollButtonDimensions(widget, height, true, &x1, &y1, &x2, &y2);
	if (pressed && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2)) {
		Shape_Draw(SHAPE_SAVE_LOAD_SCROLL_UP_PRESSED, x1, y1, 0, 0);
	}
	else if (widget->height >= 58 || Mouse_InRegion_Div(widget->div, x1, y1, x2, y2 + 15)) {
		Shape_Draw(SHAPE_SAVE_LOAD_SCROLL_UP, x1, y1, 0, 0);
	}

	ActionPanel_ScrollButtonDimensions(widget, height, false, &x1, &y1, &x2, &y2);
	if (pressed && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2)) {
		Shape_Draw(SHAPE_SAVE_LOAD_SCROLL_DOWN_PRESSED, x1, y1, 0, 0);
	}
	else if (widget->height >= 58 || Mouse_InRegion_Div(widget->div, x1, y1 - 15, x2, y2)) {
		Shape_Draw(SHAPE_SAVE_LOAD_SCROLL_DOWN, x1, y1, 0, 0);
	}
}

static void
ActionPanel_DrawStarportOrder(const Widget *widget, const Structure *s)
{
	int x1, y1, x2, y2, w, h;
	int fg;

	ActionPanel_SendOrderButtonDimensions(widget, &x1, &y1, &x2, &y2, &w, &h);

	if (BuildQueue_IsEmpty(&s->queue)) {
		Prim_FillRect_RGBA(x1, y1, x2, y2, 0x9C, 0x9C, 0xB8, 0XFF);
		Prim_DrawBorder(x1, y1, w, h, 1, true, false, 1);
		fg = 0xE;
	}
	else {
		const bool buttonDown = (widget->state.s.hover1 && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2));

		Prim_DrawBorder(x1, y1, w, h, 1, true, true, buttonDown ? 0 : 1);
		fg = 0xF;
	}

	GUI_DrawText_Wrapper("Send Order", x1 + w/2, y1 + 1, fg, 0, 0x121);
}

void
ActionPanel_DrawFactory(const Widget *widget, Structure *s)
{
	const ScreenDiv *div = &g_screenDiv[SCREENDIV_SIDEBAR];
	const int height = (s->o.type == STRUCTURE_STARPORT) ? widget->height - 12 : widget->height;
	const int itemlist_height = height - (widget->height >= 58 ? 21 : 5);

	if (g_productionStringID == STR_UPGRADINGD_DONE) {
		const int percentDone = 100 - s->upgradeTimeLeft;

		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_UPGRADINGD_DONE),
				widget->offsetX, widget->offsetY + 35 - 19, 0xF, 0, 0x021, percentDone);
		return;
	}

	if (g_factoryWindowTotal < 0) {
		Structure_InitFactoryItems(s);
		ActionPanel_ClampFactoryScrollOffset(widget, s);
	}

	Prim_DrawBorder(widget->offsetX, widget->offsetY + 2, widget->width, height - 3, 1, false, true, 0);
	Video_SetClippingArea(0, div->scale * (widget->offsetY + 3), TRUE_DISPLAY_WIDTH, div->scale * itemlist_height);

	for (int item = 0; item < g_factoryWindowTotal; item++) {
		const uint16 object_type = g_factoryWindowItems[item].objectType;
		const enum ShapeID shapeID = g_factoryWindowItems[item].shapeID;

		const char *name;
		int x1, y1, w, h;
		int fg = 0xF;

		ActionPanel_ProductionButtonDimensions(widget, s, item, &x1, &y1, NULL, NULL, &w, &h);
		if (y1 > widget->offsetY + itemlist_height)
			break;

		if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
			name = String_Get_ByIndex(g_table_structureInfo[object_type].o.stringID_abbrev);
		}
		else {
			name = String_Get_ByIndex(g_table_unitInfo[object_type].o.stringID_abbrev);
		}

		if ((s->o.type == STRUCTURE_STARPORT) && (g_starportAvailable[object_type] < 0)) {
			Shape_DrawGreyScale(shapeID, x1, y1, w, h, 0, 0);
		}
		else if (g_factoryWindowItems[item].available < 0) {
			Shape_DrawGreyScale(shapeID, x1, y1, w, h, 0, 0);

			/* Draw layout. */
			if (s->o.type == STRUCTURE_CONSTRUCTION_YARD)
				ActionPanel_DrawStructureLayout(object_type, x1, y1);
		}
		else {
			Shape_DrawScale(shapeID, x1, y1, w, h, 0, 0);

			/* Draw layout. */
			if (s->o.type == STRUCTURE_CONSTRUCTION_YARD)
				ActionPanel_DrawStructureLayout(object_type, x1, y1);
		}

		/* Draw abbreviated name. */
		GUI_DrawText_Wrapper(name, x1 + w / 2, y1 - 9, 5, 0, 0x121);

		/* Draw production status. */
		if ((s->o.type == STRUCTURE_STARPORT) && (g_starportAvailable[object_type] < 0)) {
			GUI_DrawText_Wrapper("OUT OF", x1 + w / 2, y1 +  7, 6, 0, 0x132);
			GUI_DrawText_Wrapper("STOCK",  x1 + w / 2, y1 + 18, 6, 0, 0x132);
		}
		else if (s->objectType == object_type) {
			/* Production icon is 32x24, stretched up to 52x39. */
			if (g_productionStringID != STR_BUILD_IT) {
				const float x1f = x1 + 52.0f/32.0f;
				const float x2f = x1 + w - 1.0f - 52.0f/32.0f;

				Prim_FillRect_RGBA(x1f, y1 + 12.0f, x2f, y1 + 12.0f + g_fontCurrent->height + 3.0f, 0x00, 0x00, 0x00, 0x80);
			}

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

				GUI_DrawText_Wrapper("%d%%", x1 + w / 2, y1 + 14, fg, 0, 0x121, percentDone);
			}
			else if (g_productionStringID != STR_BUILD_IT) {
				GUI_DrawText_Wrapper(String_Get_ByIndex(g_productionStringID), x1 + w / 2, y1 + 14, fg, 0, 0x121);
			}
		}
		else if (g_factoryWindowItems[item].available < 0) {
			GUI_DrawText_Wrapper("UPGRADE", x1 + w / 2, y1 +  7, 6, 0, 0x132);

			if (s->upgradeTimeLeft >= 100) {
				GUI_DrawText_Wrapper("NEEDED", x1 + w / 2, y1 + 18, 6, 0, 0x132);
			}
			else {
				GUI_DrawText_Wrapper("%d%%", x1 + w / 2, y1 + 18, 6, 0, 0x132, 100 - s->upgradeTimeLeft);
			}
		}

		/* Draw credits. */
		GUI_DrawText_Wrapper("%d", x1 + 1, y1 + h - 8, fg, 0, 0x31, g_factoryWindowItems[item].credits);

		/* Draw build queue count. */
		int count = BuildQueue_Count(&s->queue, object_type);
		if (count > 0) {
			if (s->objectType == object_type)
				count++;

			GUI_DrawText_Wrapper("x%d", x1 + w - 1, y1 + h - 10, 15, 0, 0x232, count);
		}
	}

	Video_SetClippingArea(0, div->scale * (widget->offsetY + 3), TRUE_DISPLAY_WIDTH, div->scale * height);
	ActionPanel_DrawScrollButtons(widget, (s->o.type == STRUCTURE_STARPORT));
	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);

	if (s->o.type == STRUCTURE_STARPORT)
		ActionPanel_DrawStarportOrder(widget, s);
}

void
ActionPanel_DrawPalace(const Widget *widget, Structure *s)
{
	const ScreenDiv *div = &g_screenDiv[SCREENDIV_SIDEBAR];
	enum ShapeID shapeID;
	const char *name;
	const char *deploy;
	int x, y, w, h;
	VARIABLE_NOT_USED(s);

	switch (g_productionStringID) {
		case STR_LAUNCH:
			shapeID = SHAPE_DEATH_HAND;
			name = g_table_unitInfo[UNIT_MISSILE_HOUSE].o.name;
			deploy = String_Get_ByIndex(STR_LAUNCH);
			break;

		case STR_FREMEN:
			shapeID = SHAPE_FREMEN_SQUAD;
			name = String_Get_ByIndex(g_productionStringID);
			deploy = String_Get_ByIndex(STR_DEPLOY);
			break;

		case STR_SABOTEUR:
			shapeID = SHAPE_SABOTEUR;
			name = String_Get_ByIndex(g_productionStringID);
			deploy = String_Get_ByIndex(STR_DEPLOY);
			break;

		default:
			return;
	}

	g_factoryWindowTotal = 0;

	ActionPanel_ProductionButtonDimensions(widget, s, 0, &x, &y, NULL, NULL, &w, &h);
	Prim_DrawBorder(widget->offsetX, widget->offsetY + 2, widget->width, widget->height - 3, 1, false, true, 0);
	Video_SetClippingArea(0, div->scale * (widget->offsetY + 3), TRUE_DISPLAY_WIDTH, div->scale * (widget->height - 5));

	Shape_DrawScale(shapeID, x, y, w, h, 0, 0);
	GUI_DrawText_Wrapper(name, x + w / 2, y - 9, 5, 0, 0x121);
	GUI_DrawText_Wrapper(deploy, x + w / 2, y + h + 1, 0xF, 0, 0x121);

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}
