/* actionpanel.c
 *
 * Notes about factory layout:
 *  At 320x200, widget->height is 32 >= 1 + SMALL_PRODUCTION_ICON_HEIGHT.
 *  At 320x240, widget->height is 69 >= 1 + 2 * SMALL_PRODUCTION_ICON_MIN_STRIDE.
 */

#include <assert.h>
#include <math.h>
#include "enum_string.h"
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
#include "../table/widgetinfo.h"
#include "../timer/timer.h"
#include "../unit.h"
#include "../video/video.h"

enum {
	SMALL_PRODUCTION_ICON_WIDTH         = 32,
	SMALL_PRODUCTION_ICON_HEIGHT        = 24,
	SMALL_PRODUCTION_ICON_MIN_STRIDE    = 34,
	SMALL_PRODUCTION_ICON_MAX_STRIDE    = 43,
	LARGE_PRODUCTION_ICON_WIDTH         = 52,
	LARGE_PRODUCTION_ICON_HEIGHT        = 39,
	LARGE_PRODUCTION_ICON_MIN_STRIDE    = 52,
	LARGE_PRODUCTION_ICON_MAX_STRIDE    = 58,
	SCROLL_BUTTON_WIDTH     = 24,
	SCROLL_BUTTON_HEIGHT    = 15,
	SCROLL_BUTTON_MARGIN    = 5,
	SEND_ORDER_BUTTON_MARGIN= 2,
};

enum FactoryPanelLayout {
	FACTORYPANEL_LARGE_ICON_FLAG    = 0x01,
	FACTORYPANEL_SCROLL_FLAG        = 0x02,
	FACTORYPANEL_STARPORT_FLAG      = 0x04,

	FACTORYPANEL_SMALL_ICONS_WITHOUT_SCROLL = 0x00,
};

FactoryWindowItem g_factoryWindowItems[MAX_FACTORY_WINDOW_ITEMS];
int g_factoryWindowTotal;

static enum FactoryPanelLayout s_factory_panel_layout;

/*--------------------------------------------------------------*/

static void
ActionPanel_CalculateOptimalLayout(const Widget *widget, bool is_starport)
{
	const int h = widget->height
		- 1 /* top margin. */
		- 1 /* bottom margin. */
		- (SCROLL_BUTTON_MARGIN + SCROLL_BUTTON_HEIGHT)
		- (is_starport ? SEND_ORDER_BUTTON_MARGIN : 0)
		- (is_starport ? g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].height : 0) /* send order button. */
		;

	/* Use large icons if possible. */
	if (h >= 4 * LARGE_PRODUCTION_ICON_MIN_STRIDE) {
		s_factory_panel_layout
			= (is_starport ? FACTORYPANEL_STARPORT_FLAG : 0)
			| (FACTORYPANEL_SCROLL_FLAG | FACTORYPANEL_LARGE_ICON_FLAG);
	}

	/* Use small icons with the scroll and send order buttons if they fit. */
	else if (h >= 3 * SMALL_PRODUCTION_ICON_MIN_STRIDE) {
		s_factory_panel_layout
			= (is_starport ? FACTORYPANEL_STARPORT_FLAG : 0)
			| FACTORYPANEL_SCROLL_FLAG;
	}

	/* Otherwise, use small icons, without the scroll and send order buttons. */
	else {
		s_factory_panel_layout = FACTORYPANEL_SMALL_ICONS_WITHOUT_SCROLL;
	}
}

static int
ActionPanel_ProductionListHeight(const Widget *widget)
{
	int h = widget->height - 1;

	if (s_factory_panel_layout & FACTORYPANEL_SCROLL_FLAG)
		h -= SCROLL_BUTTON_MARGIN + SCROLL_BUTTON_HEIGHT;

	if (s_factory_panel_layout & FACTORYPANEL_STARPORT_FLAG)
		h -= SEND_ORDER_BUTTON_MARGIN + g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].height;

	return h;
}

/*--------------------------------------------------------------*/

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

	/* Position to draw text so that it is not overlapped by the
	 * repair/upgrade button.  When the button is 10 pixels tall, as
	 * in the original game, y = 40.
	 */
	const int y
		= g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].offsetY
		+ g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].height + 2 - 8;

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

				Shape_Draw(g_table_unitInfo[u->o.type].o.spriteID, 20, y + 9, 0, 0);
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_D_DONE), 18, y + 36, 29, 0, 0x11, percent);
			}
			break;

		case STRUCTURE_WINDTRAP:
			{
				uint16 powerOutput = o->hitpoints * -si->powerUsage / oi->hitpoints;
				uint16 powerAverage = (h->windtrapCount == 0) ? 0 : h->powerUsage / h->windtrapCount;
				uint8 fg = (powerOutput >= powerAverage) ? 29 : 6;

				Prim_Hline(21, y + 15, 72, 16);
				if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
					GUI_DrawText_Wrapper("Power Info", 46, y + 8, 29, 0, 0x111);
					GUI_DrawText_Wrapper("Needed:", 21, y + 2 * g_fontCurrent->height, 29, 0, 0x11);
					GUI_DrawText_Wrapper("Output:", 21, y + 3 * g_fontCurrent->height, 29, 0, 0x11);
					GUI_DrawText_Wrapper("%d", 72, y + 2 * g_fontCurrent->height, 29, 0, 0x211, powerAverage);
					GUI_DrawText_Wrapper("%d", 72, y + 3 * g_fontCurrent->height, fg, 0, 0x211, powerOutput);
				}
				else {
					GUI_DrawText_Wrapper(String_Get_ByIndex(STR_POWER_INFONEEDEDOUTPUT), 18, y + 8, 29, 0, 0x11);
					GUI_DrawText_Wrapper("%d", 62, y + 2 * g_fontCurrent->height, 29, 0, 0x11, powerAverage);
					GUI_DrawText_Wrapper("%d", 62, y + 3 * g_fontCurrent->height, fg, 0, 0x11, powerOutput);
				}
			}
			break;

		case STRUCTURE_STARPORT:
			if (h->starportLinkedID != 0xFFFF) {
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_FRIGATEARRIVAL_INTMINUS_D), 18, y + 8, 29, 0, 0x11, h->starportTimeLeft);
			}
			else {
				/* GUI_DrawText_Wrapper(String_Get_ByIndex(STR_FRIGATE_INORBIT_ANDAWAITINGORDER), 18, y + 8, 29, 0, 0x11); */
			}
			break;

		case STRUCTURE_REFINERY:
		case STRUCTURE_SILO:
			{
				uint16 creditsStored;

				creditsStored = h->credits * si->creditsStorage / h->creditsStorage;
				if (h->credits > h->creditsStorage) creditsStored = si->creditsStorage;

				Prim_Hline(21, y + 15, 72, 16);
				if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
					GUI_DrawText_Wrapper("Spice", 46, y + 8, 29, 0, 0x111);
					GUI_DrawText_Wrapper("Holds:", 21, y + 2 * g_fontCurrent->height, 29, 0, 0x11);
					GUI_DrawText_Wrapper("Max:", 21, y + 3 * g_fontCurrent->height, 29, 0, 0x11);
					GUI_DrawText_Wrapper("%d", 72, y + 2 * g_fontCurrent->height, 29, 0, 0x211, creditsStored);
					GUI_DrawText_Wrapper("%d", 72, y + 3 * g_fontCurrent->height, 29, 0, 0x211, (si->creditsStorage / 100) * 100);
				}
				else {
					GUI_DrawText_Wrapper(String_Get_ByIndex(STR_SPICEHOLDS_4DMAX_4D), 18, y + 8, 29, 0, 0x11, creditsStored, (si->creditsStorage <= 1000) ? si->creditsStorage : 1000);
				}
			}
			break;

		case STRUCTURE_OUTPOST:
			{
				Prim_Hline(21, y + 15, 72, 16);
				if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
					GUI_DrawText_Wrapper("Radar Scan", 46, y + 8, 29, 0, 0x111);
					GUI_DrawText_Wrapper("Friend:", 21, y + 2 * g_fontCurrent->height, 29, 0, 0x11);
					GUI_DrawText_Wrapper("Enemy:", 21, y + 3 * g_fontCurrent->height, 29, 0, 0x11);
					GUI_DrawText_Wrapper("%d", 72, y + 2 * g_fontCurrent->height, 29, 0, 0x211, h->unitCountAllied);
					GUI_DrawText_Wrapper("%d", 72, y + 3 * g_fontCurrent->height, 29, 0, 0x211, h->unitCountEnemy);
				}
				else {
					GUI_DrawText_Wrapper(String_Get_ByIndex(STR_RADAR_SCANFRIEND_2DENEMY_2D), 18, y + 8, 29, 0, 0x11, h->unitCountAllied, h->unitCountEnemy);
				}
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

/*--------------------------------------------------------------*/

static int
ActionPanel_ProductionButtonStride(const Widget *widget, int *ret_items_per_screen)
{
	const int h = ActionPanel_ProductionListHeight(widget);
	const int min_item_height = (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) ? LARGE_PRODUCTION_ICON_MIN_STRIDE : SMALL_PRODUCTION_ICON_MIN_STRIDE;
	const int max_item_height = (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) ? LARGE_PRODUCTION_ICON_MAX_STRIDE : SMALL_PRODUCTION_ICON_MAX_STRIDE;
	const int items_per_screen = h / max_item_height;

	/* Try to squeeze in 1 more item. */
	int reth = h / (items_per_screen + 1);
	if (reth < min_item_height)
		reth = max_item_height;

	if (ret_items_per_screen != NULL)
		*ret_items_per_screen = h / reth;

	return reth;
}

static void
ActionPanel_ProductionButtonDimensions(const Widget *widget, const Structure *s,
		int item, int *x1, int *y1, int *x2, int *y2, int *w, int *h)
{
	int items_per_screen;
	const int stride = ActionPanel_ProductionButtonStride(widget, &items_per_screen);
	const int width  = (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) ? LARGE_PRODUCTION_ICON_WIDTH  : SMALL_PRODUCTION_ICON_WIDTH;
	const int height = (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) ? LARGE_PRODUCTION_ICON_HEIGHT : SMALL_PRODUCTION_ICON_HEIGHT;
	const int x = (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) ? (widget->offsetX + 4) : (widget->offsetX + 2);
	const int widget_padding = ActionPanel_ProductionListHeight(widget) - items_per_screen * stride;
	int y = widget->offsetY + 1 + s->factoryOffsetY + stride * item;

	if (items_per_screen <= 0) {
		y += (widget_padding - height) / 2;
	}
	else {
		const int item_padding = stride - 6 - 2 - height;
		y += (widget_padding + item_padding) / 2 + 6 + 2;
	}

	if (x1 != NULL) *x1 = x;
	if (y1 != NULL) *y1 = y;
	if (x2 != NULL) *x2 = x + width - 1;
	if (y2 != NULL) *y2 = y + height - 1;
	if (w != NULL) *w = width;
	if (h != NULL) *h = height;
}

static void
ActionPanel_ScrollButtonDimensions(const Widget *widget, bool up,
		int *x1, int *y1, int *x2, int *y2)
{
	int x, y;

	if (s_factory_panel_layout == FACTORYPANEL_SMALL_ICONS_WITHOUT_SCROLL) {
		x = widget->offsetX + widget->width - SCROLL_BUTTON_WIDTH - 1;
		y = widget->offsetY + widget->height / 2 - (up ? SCROLL_BUTTON_HEIGHT : 0);
	}
	else {
		x = widget->offsetX + (up ? 5 : 31);
		y = widget->offsetY + ActionPanel_ProductionListHeight(widget) + SCROLL_BUTTON_MARGIN / 2;
	}

	if (x1 != NULL) *x1 = x;
	if (y1 != NULL) *y1 = y;
	if (x2 != NULL) *x2 = x + SCROLL_BUTTON_WIDTH  - 1;
	if (y2 != NULL) *y2 = y + SCROLL_BUTTON_HEIGHT - 1;
}

static void
ActionPanel_SendOrderButtonDimensions(const Widget *widget,
		int *x1, int *y1, int *x2, int *y2, int *w, int *h)
{
	if (x1 != NULL) *x1 = widget->offsetX;
	if (y1 != NULL) *y1 = widget->offsetY + widget->height - g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].height;
	if (x2 != NULL) *x2 = widget->offsetX + widget->width  - 1;
	if (y2 != NULL) *y2 = widget->offsetY + widget->height - 1;
	if (w  != NULL) *w = widget->width;
	if (h  != NULL) *h = g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].height;
}

static void
ActionPanel_ClampFactoryScrollOffset(const Widget *widget, Structure *s)
{
	int items_per_screen;
	const int stride = ActionPanel_ProductionButtonStride(widget, &items_per_screen);
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
	int delta = g_mouseDZ;
	int x1, y1, x2, y2;

	ActionPanel_ScrollButtonDimensions(widget, true, &x1, &y1, &x2, &y2);
	if ((widget->state.buttonState & 0x44) && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2))
		delta = 1;

	ActionPanel_ScrollButtonDimensions(widget, false, &x1, &y1, &x2, &y2);
	if ((widget->state.buttonState & 0x44) && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2))
		delta = -1;

	if (delta == 0)
		return false;

	const int stride = ActionPanel_ProductionButtonStride(widget, NULL);

	s->factoryOffsetY = stride * (s->factoryOffsetY / stride + delta);
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
	if (s->o.flags.s.upgrading)
		return false;

	if (widget->state.keySelected) {
		if (g_productionStringID == STR_PLACE_IT)
			ActionPanel_BeginPlacementMode(s);
		return true;
	}

	if (g_factoryWindowTotal < 0) {
		Structure_InitFactoryItems(s);
		ActionPanel_CalculateOptimalLayout(widget, false);
		ActionPanel_ClampFactoryScrollOffset(widget, s);
	}

	if (ActionPanel_ScrollFactory(widget, s))
		return true;

	int mouseY;
	Mouse_TransformToDiv(widget->div, NULL, &mouseY);

	if (mouseY >= widget->offsetY + ActionPanel_ProductionListHeight(widget))
		return false;

	const bool lmb = (widget->state.buttonState & 0x04);
	const bool rmb = (widget->state.buttonState & 0x40);
	int item;

	for (item = 0; item < g_factoryWindowTotal; item++) {
		int x1, y1, x2, y2;

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
						s->state = STRUCTURE_STATE_IDLE;

						/* Only update high-tech factory to avoid resetting animations. */
						if (s->o.type == STRUCTURE_HIGH_TECH)
							Structure_UpdateMap(s);
					}
					else if (s->objectType != next_type) {
						Structure_BuildObject(s, next_type);
						s->o.flags.s.onHold = false;
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

void
ActionPanel_ClickStarportOrder(Structure *s)
{
	House *h = g_playerHouse;

	while (!BuildQueue_IsEmpty(&s->queue)) {
		int credits = s->queue.first->credits;
		uint16 objectType = BuildQueue_RemoveHead(&s->queue);
		Unit *u;

		g_validateStrictIfZero++;
		{
			tile32 tile;
			tile.x = 0xFFFF;
			tile.y = 0xFFFF;
			u = Unit_Create(UNIT_INDEX_INVALID, (uint8)objectType, s->o.houseID, tile, 0);
		}
		g_validateStrictIfZero--;

		if (u == NULL) {
			/* Originally the starport only allowed you to purchase a
			 * unit if there was an empty index.  However, that only
			 * really worked with the factory window interface.
			 */
			h->credits += credits;
			Structure_Starport_Restock(objectType);

			if (s->o.houseID == g_playerHouseID)
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
		Structure_Starport_Restock(type);
		h->credits += credits;
	}
}

bool
ActionPanel_ClickStarport(const Widget *widget, Structure *s)
{
	const bool lmb = (widget->state.buttonState & 0x04);
	const bool rmb = (widget->state.buttonState & 0x40);

	int x1, y1, x2, y2;
	int item;

	if (g_factoryWindowTotal < 0) {
		Structure_InitFactoryItems(s);
		ActionPanel_CalculateOptimalLayout(widget, true);
		ActionPanel_ClampFactoryScrollOffset(widget, s);
	}

	if (s_factory_panel_layout != FACTORYPANEL_SMALL_ICONS_WITHOUT_SCROLL) {
		ActionPanel_SendOrderButtonDimensions(widget, &x1, &y1, &x2, &y2, NULL, NULL);
		if (lmb && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2)) {
			ActionPanel_ClickStarportOrder(s);
			return true;
		}
	}

	if (ActionPanel_ScrollFactory(widget, s))
		return true;

	int mouseY;
	Mouse_TransformToDiv(widget->div, NULL, &mouseY);

	if (mouseY >= widget->offsetY + ActionPanel_ProductionListHeight(widget))
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
	const bool lmb = (widget->state.buttonState & 0x04);
	const int xcentre = widget->offsetX + widget->width / 2;
	int y1, y2, w;

	g_factoryWindowTotal = 0;

	ActionPanel_ProductionButtonDimensions(widget, s, 0, NULL, &y1, NULL, &y2, &w, NULL);
	if (lmb && Mouse_InRegion_Div(widget->div, xcentre - w/2, y1, xcentre + w/2, y2)) {
		Structure_ActivateSpecial(s);
		return true;
	}

	return false;
}

static void
ActionPanel_DrawStructureLayout(enum StructureType s, int x1, int y1)
{
	if (!(s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG))
		return;

	const StructureInfo *si = &g_table_structureInfo[s];
	const int lw = g_table_structure_layoutSize[si->layout].width;
	const int lh = g_table_structure_layoutSize[si->layout].height;
	const int size = 6;

	x1 += 31;
	y1 += 4;

	/* Use shadow effect for transluscent outline. */
	Shape_Draw(SHAPE_STRUCTURE_LAYOUT_OUTLINE, x1 - 1, y1 - 1, 0, 0x340);

	/* Shift layout to the right. */
	for (int ly = 0; ly < lh; ly++) {
		for (int lx = 3 - lw; lx < 3; lx++) {
			Shape_Draw(SHAPE_STRUCTURE_LAYOUT_BLOCK, x1 + size * lx, y1 + size * ly, 0, 0);
		}
	}
}

static void
ActionPanel_DrawScrollButtons(const Widget *widget)
{
	int items_per_screen;

	ActionPanel_ProductionButtonStride(widget, &items_per_screen);
	if (g_factoryWindowTotal <= items_per_screen)
		return;

	const bool pressed = (widget->state.hover1);
	int x1, y1, x2, y2;

	ActionPanel_ScrollButtonDimensions(widget, true, &x1, &y1, &x2, &y2);
	if (pressed && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2)) {
		Shape_Draw(SHAPE_SAVE_LOAD_SCROLL_UP_PRESSED, x1, y1, 0, 0);
	}
	else if ((s_factory_panel_layout != FACTORYPANEL_SMALL_ICONS_WITHOUT_SCROLL) ||
			Mouse_InRegion_Div(widget->div, x1, y1, x2, y2 + 15)) {
		Shape_Draw(SHAPE_SAVE_LOAD_SCROLL_UP, x1, y1, 0, 0);
	}

	ActionPanel_ScrollButtonDimensions(widget, false, &x1, &y1, &x2, &y2);
	if (pressed && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2)) {
		Shape_Draw(SHAPE_SAVE_LOAD_SCROLL_DOWN_PRESSED, x1, y1, 0, 0);
	}
	else if ((s_factory_panel_layout != FACTORYPANEL_SMALL_ICONS_WITHOUT_SCROLL) ||
			Mouse_InRegion_Div(widget->div, x1, y1 - 15, x2, y2)) {
		Shape_Draw(SHAPE_SAVE_LOAD_SCROLL_DOWN, x1, y1, 0, 0);
	}
}

static void
ActionPanel_DrawStarportOrder(const Widget *widget, const Structure *s)
{
	if (s_factory_panel_layout == FACTORYPANEL_SMALL_ICONS_WITHOUT_SCROLL)
		return;

	int x1, y1, x2, y2, w, h;
	int fg;

	ActionPanel_SendOrderButtonDimensions(widget, &x1, &y1, &x2, &y2, &w, &h);

	if (BuildQueue_IsEmpty(&s->queue)) {
		Prim_FillRect_RGBA(x1, y1, x2, y2, 0x9C, 0x9C, 0xB8, 0XFF);
		Prim_DrawBorder(x1, y1, w, h, 1, true, false, 1);
		fg = 0xE;
	}
	else {
		const bool buttonDown = (widget->state.hover1 && Mouse_InRegion_Div(widget->div, x1, y1, x2, y2));

		Prim_DrawBorder(x1, y1, w, h, 1, true, true, buttonDown ? 0 : 1);
		fg = 0xF;
	}

	y1 += (h == 10) ? 1 : 2;
	GUI_DrawText_Wrapper("Send Order", x1 + w/2, y1, fg, 0, 0x121);
}

void
ActionPanel_HighlightIcon(enum HouseType houseID, int x1, int y1, bool large_icon)
{
	static int64_t paletteChangeTimer;
	static int paletteColour;
	static int paletteChange = 8;

	unsigned char r, g, b;

	if (paletteChangeTimer <= Timer_GetTicks()) {
		paletteChangeTimer = Timer_GetTicks() + 3;
		paletteColour += paletteChange;
	}

	if (paletteColour < 0 || paletteColour > 63) {
		paletteChange = -paletteChange;
		paletteColour += paletteChange;
	}

	switch (houseID) {
		case HOUSE_HARKONNEN:
			r = 4 * 63;
			g = 4 * paletteColour;
			b = 4 * paletteColour;
			break;

		case HOUSE_ATREIDES:
			r = 4 * paletteColour;
			g = 4 * paletteColour;
			b = 4 * 63;
			break;

		case HOUSE_ORDOS:
			r = 4 * paletteColour;
			g = 4 * 63;
			b = 4 * paletteColour;
			break;

		case HOUSE_FREMEN:
			r = 4 * 63;
			g = 4 * 63 - 2 * paletteColour;
			b = 4 * 63 - 4 * paletteColour;
			break;

		case HOUSE_SARDAUKAR:
			r = 4 * 63;
			g = 4 * 63 - 4 * paletteColour;
			b = 4 * 63 - 1 * paletteColour;
			break;

		case HOUSE_MERCENARY:
			r = 4 * 63;
			g = 4 * 63 - 1 * paletteColour;
			b = 4 * 63 - 4 * paletteColour;
			break;

		default:
			return;
	}

	if (large_icon) {
		Prim_Rect_RGBA(x1 + 1.0f, y1 + 0.5f, x1 + 52.0f, y1 + 38.0f, r, g, b, 0xFF, 3.0f);
	}
	else {
		Prim_Rect_RGBA(x1 + 1.0f, y1 + 1.0f, x1 + 31.0f, y1 + 23.0f, r, g, b, 0xFF, 2.0f);
	}
}

void
ActionPanel_DrawFactory(const Widget *widget, Structure *s)
{
	if (g_productionStringID == STR_UPGRADINGD_DONE) {
		const int percentDone = 100 - s->upgradeTimeLeft;

		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_UPGRADINGD_DONE),
				widget->offsetX, widget->offsetY + 35 - 19, 0xF, 0, 0x021, percentDone);
		return;
	}

	if (g_factoryWindowTotal < 0) {
		Structure_InitFactoryItems(s);
		ActionPanel_CalculateOptimalLayout(widget, (s->o.type == STRUCTURE_STARPORT));
		ActionPanel_ClampFactoryScrollOffset(widget, s);
	}

	const ScreenDiv *div = &g_screenDiv[SCREENDIV_SIDEBAR];
	int height = widget->height;

	if (s_factory_panel_layout & FACTORYPANEL_STARPORT_FLAG)
		height -= SEND_ORDER_BUTTON_MARGIN + g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].height;

	const int itemlist_height = ActionPanel_ProductionListHeight(widget);

	Prim_DrawBorder(widget->offsetX, widget->offsetY, widget->width, height, 1, false, true, 0);
	Video_SetClippingArea(0, div->scaley * (widget->offsetY + 1), TRUE_DISPLAY_WIDTH, div->scaley * itemlist_height);

	const int xcentre = widget->offsetX + widget->width / 2;

	for (int item = 0; item < g_factoryWindowTotal; item++) {
		const uint16 object_type = g_factoryWindowItems[item].objectType;
		const enum ShapeID shapeID = g_factoryWindowItems[item].shapeID;

		const char *name;
		int x1, y1, w, h;
		int fg = 0xF;

		ActionPanel_ProductionButtonDimensions(widget, s, item, &x1, &y1, NULL, NULL, &w, &h);
		if (y1 > widget->offsetY + itemlist_height)
			break;

		if (y1 < widget->offsetY)
			continue;

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

		/* Draw production status. */
		if (s->o.type == STRUCTURE_STARPORT) {
			if ((g_starportAvailable[object_type] < 0) && (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG)) {
				GUI_DrawText_Wrapper("OUT OF", xcentre, y1 +  7, 6, 0, 0x132);
				GUI_DrawText_Wrapper("STOCK",  xcentre, y1 + 18, 6, 0, 0x132);
			}
		}
		else if (s->objectType == object_type) {
			/* Production icon is 32x24, or stretched up to 52x39. */
			if (g_productionStringID != STR_BUILD_IT) {
				const bool large_icon = (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG);
				float x1f = x1 + (float)w/32.0f;
				float x2f = x1 + w - (float)w/32.0f;
				float y1f, y2f;

				if (g_productionStringID == STR_PLACE_IT)
					ActionPanel_HighlightIcon(g_playerHouseID, x1, y1, large_icon);

				if (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) {
					/* On hold greys out the entire icon, which has 2px borders. */
					if (g_productionStringID == STR_ON_HOLD) {
						y1f = y1 + 2.0f;
						y2f = y1 + 37.0f;
					}
					else {
						y1f = y1 + 12.0f;
						y2f = y1f + g_fontCurrent->height + 4.0f;
					}
				}
				else {
					/* On hold greys out the entire icon, which has 1px borders. */
					if (g_productionStringID == STR_ON_HOLD) {
						y1f = y1 + 1.0f;
						y2f = y1 + 23.0f;
					}
					else {
						/* For long strings, grey strip spans the widget width. */
						if ((g_productionStringID == STR_COMPLETED) || (g_productionStringID == STR_PLACE_IT)) {
							x1f = widget->offsetX + 1.0f;
							x2f = widget->offsetX + widget->width - 1.0f;
						}

						y1f = y1 + 5.0f;
						y2f = y1f + g_fontCurrent->height + 4.0f;
					}
				}

				Prim_FillRect_RGBA(x1f, y1f, x2f, y2f, 0x00, 0x00, 0x00, 0x80);
			}

			if (g_productionStringID == STR_D_DONE || g_productionStringID == STR_ON_HOLD) {
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

				if ((g_productionStringID == STR_D_DONE) || !(s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG)) {
					GUI_DrawText_Wrapper("%d%%", x1 + w / 2, y1 + (h - 10) / 2, fg, 0, 0x121, percentDone);
				}
				else {
					GUI_DrawText_Wrapper("%d%%", xcentre, y1 + 10, fg, 0, 0x121, percentDone);
					GUI_DrawText_Wrapper(String_Get_ByIndex(STR_ON_HOLD), xcentre, y1 + 18, fg, 0, 0x121);
				}

				Prim_Rect(x1 + 1.0f, y1 + 1.0f, x1 + w - 1.0f, y1 + h - 1.0f, 0xFF, 2.0f);
			}
			else if (g_productionStringID != STR_BUILD_IT) {
				GUI_DrawText_Wrapper(String_Get_ByIndex(g_productionStringID), widget->offsetX + widget->width / 2, y1 + (h - 10) / 2, fg, 0, 0x121);
			}
		}
		else if (g_factoryWindowItems[item].available < 0) {
			if (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) {
				const int yupper = y1 + 7;
				const int ylower = y1 + 18;

				GUI_DrawText_Wrapper("UPGRADE", xcentre, yupper, 6, 0, 0x132);

				if (s->upgradeTimeLeft >= 100) {
					GUI_DrawText_Wrapper("NEEDED", xcentre, ylower, 6, 0, 0x132);
				}
				else {
					GUI_DrawText_Wrapper("%d%%", xcentre, ylower, 6, 0, 0x132, 100 - s->upgradeTimeLeft);
				}
			}

			/* Make upgrade cost yellow to distinguish it from unit cost. */
			fg = 5;
		}

		/* Draw abbreviated name. */
		if (y1 - 8 >= widget->offsetY + 2)
			GUI_DrawText_Wrapper(name, xcentre, y1 - 9, 5, 0, 0x161);

		/* Draw credits. */
		if (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) {
			GUI_DrawText_Wrapper("%d", x1 + 1, y1 + h - 8, fg, 0, 0x31, g_factoryWindowItems[item].credits);
		}
		else if ((s->objectType != object_type) || (g_productionStringID != STR_PLACE_IT)) {
			GUI_DrawText_Wrapper("%d", widget->offsetX + widget->width - 3, y1 + 2, fg, 0, 0x231, g_factoryWindowItems[item].credits);
		}

		/* Draw build queue count. */
		int count = BuildQueue_Count(&s->queue, object_type);
		if (count > 0) {
			if (s->objectType == object_type)
				count++;

			if (s_factory_panel_layout & FACTORYPANEL_LARGE_ICON_FLAG) {
				GUI_DrawText_Wrapper("x%d", widget->offsetX + widget->width - 5, y1 + h - 10, 15, 0, 0x232, count);
			}
			else {
				GUI_DrawText_Wrapper("x%d", widget->offsetX + widget->width - 3, y1 + h - 10, 15, 0, 0x232, count);
			}
		}
	}

	Video_SetClippingArea(0, div->scaley * (widget->offsetY + 1), TRUE_DISPLAY_WIDTH, div->scaley * (widget->height - 2));
	ActionPanel_DrawScrollButtons(widget);
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
	uint16 type;
	int y, w, h;
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
			type = g_table_houseInfo[g_playerHouseID].superWeapon.saboteur.unit;
			shapeID = g_table_unitInfo[type].o.spriteID;
			name = String_Get_ByIndex(g_productionStringID);
			deploy = String_Get_ByIndex(STR_DEPLOY);
			break;

		default:
			return;
	}

	if (g_factoryWindowTotal != 0) {
		g_factoryWindowTotal = 0;
		ActionPanel_CalculateOptimalLayout(widget, false);
	}

	ActionPanel_ProductionButtonDimensions(widget, s, 0, NULL, &y, NULL, NULL, &w, &h);
	Prim_DrawBorder(widget->offsetX, widget->offsetY, widget->width, widget->height, 1, false, true, 0);
	Video_SetClippingArea(0, div->scaley * widget->offsetY, TRUE_DISPLAY_WIDTH, div->scaley * (widget->height - 2));

	const int xcentre = widget->offsetX + widget->width / 2;

	Shape_DrawScale(shapeID, xcentre - w/2, y, w, h, 0, 0);

	/* Draw abbreviated name. */
	if (y - 8 >= widget->offsetY + 2)
		GUI_DrawText_Wrapper(name, xcentre, y - 9, 5, 0, 0x161);

	if (y + h + 1 + 6 < widget->offsetY + widget->height)
		GUI_DrawText_Wrapper(deploy, xcentre, y + h + 1, 0xF, 0, 0x161);

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}
