/* viewport.c */

#include <assert.h>
#include <stdlib.h>
#include "enum_string.h"
#include "../os/math.h"

#include "viewport.h"

#include "menubar.h"
#include "../audio/audio.h"
#include "../common_a5.h"
#include "../config.h"
#include "../enhancement.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../map.h"
#include "../opendune.h"
#include "../pool/house.h"
#include "../pool/pool.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../string.h"
#include "../structure.h"
#include "../table/widgetinfo.h"
#include "../tile.h"
#include "../timer/timer.h"
#include "../tools/coord.h"
#include "../tools/encoded_index.h"
#include "../tools/orientation.h"
#include "../tools/random_general.h"
#include "../unit.h"
#include "../video/video.h"

enum ViewportClickAction {
	VIEWPORT_CLICK_NONE,
	VIEWPORT_LMB,           /* LMB pressed, drag undecided. */
	VIEWPORT_SELECTION_BOX,
	VIEWPORT_FAST_SCROLL,
	VIEWPORT_PAN_MINIMAP,
	VIEWPORT_RMB,           /* RMB pressed, drag undecided. */
	VIEWPORT_PAN
};

enum SelectionMode {
	SELECTION_MODE_NONE,
	SELECTION_MODE_CONTROLLABLE_UNIT,
	SELECTION_MODE_UNCONTROLLABLE_UNIT,
	SELECTION_MODE_STRUCTURE
};

/* Sprite index offset and flag pairs for the 8 orientations. */
/* For sprites drawn at {0, 45, 90, 135, 180} degrees (e.g. tanks). */
static const uint16 values_32A4[8][2] = {
	{0, 0}, {1, 0}, {2, 0}, {3, 0},
	{4, 0}, {3, 1}, {2, 1}, {1, 1}
};

/* Selection box is in screen coordinates. */
static enum ViewportClickAction viewport_click_action = VIEWPORT_CLICK_NONE;
static int64_t viewport_click_time;
static int viewport_click_x;
static int viewport_click_y;
static bool selection_box_add_to_selection;
static int selection_box_x2;
static int selection_box_y2;
static int viewport_pan_dx;
static int viewport_pan_dy;

static bool Viewport_InterpolateMovement(const Unit *u, int *x, int *y);

/*--------------------------------------------------------------*/

void
Viewport_Init(void)
{
	viewport_click_action = VIEWPORT_CLICK_NONE;
	g_viewport_desiredDX = 0.0f;
	g_viewport_desiredDY = 0.0f;
	g_mousePanning = false;
}

static bool
Map_InRange(int xy)
{
	return (0 <= xy && xy < MAP_SIZE_MAX);
}

static int
Viewport_ClampSelectionBoxX(int x)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];

	return clamp(0, x, wi->width - 1);
}

static int
Viewport_ClampSelectionBoxY(int y)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];

	return clamp(0, y, wi->height - 1);
}

static enum SelectionMode
Viewport_GetSelectionMode(void)
{
	if (Unit_AnySelected()) {
		const Unit *u = Unit_FirstSelected(NULL);

		if (Unit_GetHouseID(u) == g_playerHouseID) {
			return SELECTION_MODE_CONTROLLABLE_UNIT;
		}
		else {
			return SELECTION_MODE_UNCONTROLLABLE_UNIT;
		}
	}
	else {
		if (Structure_Get_ByPackedTile(g_selectionPosition) != NULL) {
			return SELECTION_MODE_STRUCTURE;
		}
		else {
			return SELECTION_MODE_NONE;
		}
	}
}

static void
Viewport_SelectRegion(void)
{
	const int radius = 5;
	const int dx = selection_box_x2 - viewport_click_x;
	const int dy = selection_box_y2 - viewport_click_y;
	const int x0 = Tile_GetPackedX(g_viewportPosition);
	const int y0 = Tile_GetPackedY(g_viewportPosition);
	const enum SelectionMode mode = Viewport_GetSelectionMode();

	/* Select individual unit or structure. */
	if (dx*dx + dy*dy < radius*radius) {
		const int tilex = x0 + (selection_box_x2 + g_viewport_scrollOffsetX) / TILE_SIZE;
		const int tiley = y0 + (selection_box_y2 + g_viewport_scrollOffsetY) / TILE_SIZE;

		if (!(Map_InRange(tilex) && Map_InRange(tiley)))
			return;

		const uint16 packed = Tile_PackXY(tilex, tiley);

		if (g_mapVisible[packed].fogOverlayBits == 0xF)
			return;

		if (mode == SELECTION_MODE_NONE) {
			Map_SetSelection(packed);
		}
		else if (mode == SELECTION_MODE_CONTROLLABLE_UNIT || mode == SELECTION_MODE_UNCONTROLLABLE_UNIT) {
			Unit *u = Unit_Get_ByPackedTile(packed);

			if (u == NULL) {
			}
			else if (Unit_IsSelected(u)) {
				Unit_Unselect(u);
			}
			else if ((mode == SELECTION_MODE_CONTROLLABLE_UNIT) && (Unit_GetHouseID(u) == g_playerHouseID)) {
				Unit_Select(u);
				Unit_DisplayStatusText(u);
			}
		}
		else if (mode == SELECTION_MODE_STRUCTURE) {
			if (Structure_Get_ByPackedTile(packed) == Structure_Get_ByPackedTile(g_selectionPosition))
				Map_SetSelection(0xFFFF);
		}
	}

	/* Box selection. */
	else if (mode == SELECTION_MODE_NONE || mode == SELECTION_MODE_CONTROLLABLE_UNIT) {
		const int x1 = viewport_click_x;
		const int y1 = viewport_click_y;
		const int x2 = selection_box_x2;
		const int y2 = selection_box_y2;

		PoolFindStruct find;

		find.houseID = g_playerHouseID;
		find.type = 0xFFFF;
		find.index = 0xFFFF;

		/* Try to find own units. */
		Unit *u = Unit_Find(&find);
		while (u != NULL) {
			const ObjectInfo *oi = &g_table_unitInfo[u->o.type].o;

			int ux, uy;
			Map_IsPositionInViewport(u->o.position, &ux, &uy);

			if ((oi->flags.tabSelectable) &&
			    (x1 < ux + TILE_SIZE / 4 && ux - TILE_SIZE / 4 < x2) &&
			    (y1 < uy + TILE_SIZE / 4 && uy - TILE_SIZE / 4 < y2)) {
				if (!Unit_IsSelected(u))
					Unit_Select(u);
			}

			u = Unit_Find(&find);
		}

		if (Unit_AnySelected()) {
			Unit_DisplayGroupStatusText();
			return;
		}

#if 0
		find.index = 0xFFFF;

		/* Try to find own structure. */
		Structure *s;
		while ((s = Structure_Find(&find)) != NULL) {
			const StructureInfo *si = &g_table_structureInfo[s->o.type];
			const int sx = Tile_GetPosX(s->o.position);
			const int sy = Tile_GetPosY(s->o.position);
			const int xx = TILE_SIZE * (Tile_GetPosX(s->o.position) - x0) - g_viewport_scrollOffsetX;
			const int yy = TILE_SIZE * (Tile_GetPosY(s->o.position) - y0) - g_viewport_scrollOffsetY;
			const int sw = TILE_SIZE * g_table_structure_layoutSize[si->layout].width;
			const int sh = TILE_SIZE * g_table_structure_layoutSize[si->layout].height;

			if ((x1 <= xx + sw && xx <= x2) && (y1 <= yy + sh && yy <= y2)) {
				const uint16 packed = Tile_PackXY(sx, sy);
				Map_SetSelection(packed);
				return;
			}
		}
#endif
	}
}

static void
Viewport_Target(Unit *u, enum UnitActionType action, bool command_button, uint16 packed)
{
	uint16 encoded;
	Unit *target = NULL;

	if (Unit_GetHouseID(u) != g_playerHouseID)
		return;

	/* Action might not be available to the unit due to multiple selection
	 * (e.g. no attack for saboteurs).
	 */
	const UnitInfo *ui = &g_table_unitInfo[u->o.type];
	if ((action != ui->o.actionsPlayer[0]) &&
	    (action != ui->o.actionsPlayer[1]) &&
	    (action != ui->o.actionsPlayer[2]) &&
	    (action != ui->o.actionsPlayer[3])) {
		return;
	}

	if (u->deviated != 0 && !u->deviationDecremented) {
		Unit_Deviation_Decrease(u, 5);
		if (u->deviated == 0)
			return;
	}
	else {
		u->deviationDecremented = false;
	}

	if (action == ACTION_SABOTAGE) {
		u->detonateAtTarget = true;
		action = ACTION_MOVE;
	}
	else {
		u->detonateAtTarget = false;
	}

	Object_Script_Variable4_Clear(&u->o);
	u->targetAttack = 0;
	u->targetMove = 0;
	u->route[0] = 0xFF;

	if (action != ACTION_MOVE && action != ACTION_HARVEST) {
		encoded = Tools_Index_Encode(Unit_FindTargetAround(packed), IT_TILE);
	}
	else {
		encoded = Tools_Index_Encode(packed, IT_TILE);
	}

	Unit_SetAction(u, action);

	if (action == ACTION_MOVE) {
		Unit_SetDestination(u, encoded);

		if (u->detonateAtTarget) {
			target = Tools_Index_GetUnit(u->targetMove);
		}
		else if (enhancement_permanent_follow_mode) {
			u->permanentFollow = command_button;
		}
	}
	else if (action == ACTION_HARVEST) {
		u->targetMove = encoded;
	}
	else {
		Unit_SetTarget(u, encoded);
		target = Tools_Index_GetUnit(u->targetAttack);
	}

	if (target != NULL)
		target->blinkCounter = 8;

	if (g_enable_sound_effects == SOUNDEFFECTS_SYNTH_ONLY) {
		Audio_PlayEffect(EFFECT_SET_TARGET);
	}
	else if (g_table_unitInfo[u->o.type].movementType == MOVEMENT_FOOT) {
		Audio_PlaySample(g_table_actionInfo[action].soundID, 255, 0.0);
	}
	else {
		const enum SampleID sampleID = (((Tools_Random_256() & 0x1) == 0) ? SAMPLE_ACKNOWLEDGED : SAMPLE_AFFIRMATIVE);

		Audio_PlaySample(sampleID, 255, 0.0);
	}
}

static void
Viewport_Place(void)
{
	const StructureInfo *si = &g_table_structureInfo[g_structureActiveType];

	Structure *s = g_structureActive;
	House *h = g_playerHouse;

	if (Structure_Place(s, g_selectionPosition, g_playerHouseID)) {
		Audio_PlaySound(SOUND_PLACEMENT);

		if (s->o.type == STRUCTURE_PALACE)
			House_Get_ByIndex(s->o.houseID)->palacePosition = s->o.position;

		if (g_structureActiveType == STRUCTURE_REFINERY && g_validateStrictIfZero == 0) {
			Unit *u;

			g_validateStrictIfZero++;
			u = Unit_CreateWrapper(g_playerHouseID, UNIT_HARVESTER, Tools_Index_Encode(s->o.index, IT_STRUCTURE));
			g_validateStrictIfZero--;

			if (u == NULL) {
				h->harvestersIncoming++;
			}
			else {
				u->originEncoded = Tools_Index_Encode(s->o.index, IT_STRUCTURE);
			}
		}

		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);

		s = Structure_Get_ByPackedTile(g_structureActivePosition);
		if (s != NULL) {
			if (Structure_GetAvailable(s, s->objectType) <= 0)
				Structure_BuildObject(s, 0xFFFE);
		}

		g_structureActiveType = 0xFFFF;
		g_structureActive = NULL;
		g_selectionState = 0; /* Invalid. */

		GUI_DisplayHint(si->o.hintStringID, si->o.spriteID);

		House_UpdateRadarState(h);

		if (h->powerProduction < h->powerUsage) {
			if ((h->structuresBuilt & FLAG_STRUCTURE_OUTPOST) != 0) {
				GUI_DisplayText(String_Get_ByIndex(STR_NOT_ENOUGH_POWER_FOR_RADAR_BUILD_WINDTRAPS), 3);
			}
		}

		return;
	}

	Audio_PlaySound(EFFECT_ERROR_OCCURRED);

	if (g_structureActiveType == STRUCTURE_SLAB_1x1 || g_structureActiveType == STRUCTURE_SLAB_2x2) {
		GUI_DisplayText(String_Get_ByIndex(STR_CAN_NOT_PLACE_FOUNDATION_HERE), 2);
	}
	else {
		GUI_DisplayHint(STR_HINT_STRUCTURES_MUST_BE_PLACED_ON_CLEAR_ROCK_OR_CONCRETE_AND_ADJACENT_TO_ANOTHER_FRIENDLY_STRUCTURE, 0xFFFF);
		GUI_DisplayText(String_Get_ByIndex(STR_CAN_NOT_PLACE_S_HERE), 2, String_Get_ByIndex(si->o.stringID_abbrev));
	}
}

static bool
Viewport_MouseInScrollWidget(void)
{
	for (enum GameWidgetType w = GAME_WIDGET_SCROLL_UP; w <= GAME_WIDGET_SCROLL_DOWN; w++) {
		const WidgetInfo *wi = &g_table_gameWidgetInfo[w];

		if (Mouse_InRegion(wi->offsetX, wi->offsetY, wi->offsetX + wi->width - 1, wi->offsetY + wi->height - 1))
			return true;
	}

	return false;
}

static void
Viewport_ScrollMap(enum ShapeID *cursorID)
{
	const WidgetInfo *wi;
	int dx = 0, dy = 0;

	if (viewport_click_action == VIEWPORT_PAN_MINIMAP)
		return;

	wi = &g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP];
	if (Mouse_InRegion(wi->offsetX, wi->offsetY, wi->offsetX + wi->width - 1, wi->offsetY + wi->height - 1)) dy--;

	wi = &g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT];
	if (Mouse_InRegion(wi->offsetX, wi->offsetY, wi->offsetX + wi->width - 1, wi->offsetY + wi->height - 1)) dx++;

	wi = &g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN];
	if (Mouse_InRegion(wi->offsetX, wi->offsetY, wi->offsetX + wi->width - 1, wi->offsetY + wi->height - 1)) dy++;

	wi = &g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT];
	if (Mouse_InRegion(wi->offsetX, wi->offsetY, wi->offsetX + wi->width - 1, wi->offsetY + wi->height - 1)) dx--;

	if (viewport_click_action == VIEWPORT_FAST_SCROLL) {
		const int speed = max(1, 2 * g_gameConfig.scrollSpeed);
		Map_MoveDirection(speed * dx, speed * dy);
	}
	else if (g_gameConfig.autoScroll) {
		const int speed = max(1, g_gameConfig.scrollSpeed);
		Map_MoveDirection(speed * dx, speed * dy);
	}
	else if ((!g_gameConfig.autoScroll) && (viewport_click_action == VIEWPORT_SELECTION_BOX)) {
		dx = 0;
		dy = 0;
	}

	if (dx < 0) *cursorID = SHAPE_CURSOR_LEFT;
	else if (dx > 0) *cursorID = SHAPE_CURSOR_RIGHT;

	if (dy < 0) *cursorID = SHAPE_CURSOR_UP;
	else if (dy > 0) *cursorID = SHAPE_CURSOR_DOWN;
}

static void
Viewport_StopMousePanning(void)
{
	viewport_click_action = VIEWPORT_CLICK_NONE;

	if (g_mousePanning) {
		g_mousePanning = false;
		Video_WarpCursor(viewport_click_x, viewport_click_y);
	}

	Video_ShowCursor();
}

static bool
Viewport_GenericCommandCanAttack(uint16 packed, enum LandscapeType lst, bool visible, bool scouted)
{
	if (!scouted)
		return false;

	if (lst == LST_BLOOM_FIELD)
		return true;

	if ((lst == LST_WALL || lst == LST_STRUCTURE) && (!House_AreAllied(g_playerHouseID, g_mapVisible[packed].houseID)))
		return true;

	if (visible) {
		const Unit *target_u = Unit_Get_ByPackedTile(packed);
		if ((target_u != NULL) && (!House_AreAllied(g_playerHouseID, Unit_GetHouseID(target_u))))
			return true;
	}

	return false;
}

static bool
Viewport_GenericCommandCanSabotageAlly(uint16 packed)
{
	int iter;

	/* Allow sabotaging allied and deviated units if only a single saboteur is selected. */
	const Unit *u = Unit_FirstSelected(&iter);
	if ((u != NULL) && (u->o.type == UNIT_SABOTEUR) && (Unit_NextSelected(&iter) == NULL)) {
		return (g_map[packed].hasUnit && Unit_Get_ByPackedTile(packed) != u);
	}

	return false;
}

static bool
Viewport_LMBAttemptsGenericCommand(uint16 packed)
{
	/* If hold shift, always add to selection. */
	if (Input_Test(SCANCODE_LSHIFT))
		return false;

	/* If only non-player units selected, false. */
	bool found_player = false;
	int iter;
	for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
		if (Unit_GetHouseID(u) == g_playerHouseID) {
			found_player = true;
			break;
		}
	}

	if (!found_player)
		return false;

	const bool sabotage = Viewport_GenericCommandCanSabotageAlly(packed);
	if (sabotage)
		return true;

	enum HouseType houseID = HOUSE_INVALID;

	Structure *s = Structure_Get_ByPackedTile(packed);
	if (s != NULL)
		houseID = s->o.houseID;

	if (houseID == HOUSE_INVALID) {
		Unit *u = Unit_Get_ByPackedTile(packed);
		if (u != NULL)
			houseID = Unit_GetHouseID(u);
	}

	return !House_AreAllied(houseID, g_playerHouseID);
}

static bool
Viewport_PerformContextSensitiveAction(uint16 packed, bool dry_run)
{
	const enum LandscapeType lst = Map_GetLandscapeTypeVisible(packed);
	const bool visible = (g_mapVisible[packed].fogOverlayBits != 0xF);
	const bool scouted = g_map[packed].isUnveiled;
	const bool attack = Viewport_GenericCommandCanAttack(packed, lst, visible, scouted);
	const bool sabotage = Viewport_GenericCommandCanSabotageAlly(packed);

	if (dry_run && !(attack || sabotage) && !(lst == LST_SPICE || lst == LST_THICK_SPICE))
		return false;

	int iter;
	for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
		const ObjectInfo *oi = &g_table_unitInfo[u->o.type].o;
		enum UnitActionType action = ACTION_INVALID;

		if (Unit_GetHouseID(u) != g_playerHouseID)
			continue;

		for (int i = 0; (i < 4) && (action == ACTION_INVALID); i++) {
			if ((oi->actionsPlayer[i] == ACTION_ATTACK) && attack) {
				action = ACTION_ATTACK;
			}
			else if (oi->actionsPlayer[i] == ACTION_MOVE) {
				action = ACTION_MOVE;
			}

			/* Harvesters should only harvest if ordered to a spice field. */
			else if (oi->actionsPlayer[i] == ACTION_HARVEST && (lst == LST_SPICE || lst == LST_THICK_SPICE) && scouted) {
				action = (u->amount < 100) ? ACTION_HARVEST : ACTION_MOVE;
			}

			/* Saboteurs should show target cursor for detonating on structures, walls, and units. */
			else if ((oi->actionsPlayer[i] == ACTION_SABOTAGE) && (lst != LST_BLOOM_FIELD) && (dry_run || enhancement_targetted_sabotage)) {
				if (attack || sabotage)
					action = ACTION_SABOTAGE;
			}
		}

		if (action != ACTION_INVALID) {
			if (dry_run) {
				if (action != ACTION_MOVE)
					return true;
			}
			else {
				Viewport_Target(u, action, false, packed);
			}
		}
	}

	return false;
}

bool
Viewport_Click(Widget *w)
{
	static int64_t l_tickCursor;

	enum ShapeID cursorID
		= (viewport_click_action == VIEWPORT_SELECTION_BOX) ? SHAPE_CURSOR_NORMAL
		: (g_selectionType == SELECTIONTYPE_TARGET) ? SHAPE_CURSOR_TARGET : SHAPE_CURSOR_NORMAL;

	if (w->index == 45) {
		if ((cursorID != g_cursorSpriteID) && (g_timerGame - l_tickCursor > 10)) {
			l_tickCursor = g_timerGame;
			Video_SetCursor(cursorID);
		}

		return true;
	}

	Viewport_ScrollMap(&cursorID);

	int mouseX, mouseY;
	uint16 packed;
	bool perform_context_sensitive_action = false;

	/* Minimap. */
	if (w->index == 44) {
		Mouse_TransformToDiv(SCREENDIV_SIDEBAR, &mouseX, &mouseY);

		const int mapScale = g_scenario.mapScale;
		const MapInfo *mapInfo = &g_mapInfos[mapScale];
		const int tilex = Map_Clamp(mapInfo->minX + (mouseX - w->offsetX) / (mapScale + 1));
		const int tiley = Map_Clamp(mapInfo->minY + (mouseY - w->offsetY) / (mapScale + 1));
		packed = Tile_PackXY(tilex, tiley);
	}
	else {
		Mouse_TransformToDiv(SCREENDIV_VIEWPORT, &mouseX, &mouseY);

		const int x0 = Tile_GetPackedX(g_viewportPosition);
		const int y0 = Tile_GetPackedY(g_viewportPosition);
		const int tilex = Map_Clamp(x0 + (mouseX + g_viewport_scrollOffsetX) / TILE_SIZE);
		const int tiley = Map_Clamp(y0 + (mouseY + g_viewport_scrollOffsetY) / TILE_SIZE);
		packed = Tile_PackXY(tilex, tiley);
	}

	/* Context-sensitive mouse cursor. */
	do {
		if (w->index != 43) break;
		if ((cursorID == SHAPE_CURSOR_TARGET) || (g_timerGame - l_tickCursor <= 10)) break;

		if ((viewport_click_action == VIEWPORT_CLICK_NONE) ||
		    (viewport_click_action == VIEWPORT_LMB && g_gameConfig.leftClickOrders) ||
		    (viewport_click_action == VIEWPORT_RMB && !g_gameConfig.leftClickOrders)) {
			if (Viewport_PerformContextSensitiveAction(packed, true))
				cursorID = SHAPE_CURSOR_TARGET;
		}
	} while (false);

	if ((cursorID != g_cursorSpriteID) && (g_timerGame - l_tickCursor > 10)) {
		l_tickCursor = g_timerGame;
		Video_SetCursor(cursorID);
	}

	if (w->state.buttonState & 0x01) {
		const bool mouse_in_scroll_widget = Viewport_MouseInScrollWidget();

		/* Clicking LMB performs target. */
		if ((g_selectionType == SELECTIONTYPE_TARGET) && !mouse_in_scroll_widget) {
			GUI_DisplayText(NULL, -1);

			if (g_unitHouseMissile != NULL) {
				/* ENHANCEMENT -- In Dune 2 you only hear "Missile launched" if you let the timer run out.
				 * Note: you actually get one second to choose a target after the narrator says "Missile launched".
				 */
				if (enhancement_play_additional_voices && g_houseMissileCountdown > 1) {
					Audio_PlayVoice(VOICE_MISSILE_LAUNCHED);
				}

				Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);
				Unit_LaunchHouseMissile(s, packed);
				return true;
			}

			int iter;
			for (Unit *u = Unit_FirstSelected(&iter); u; u = Unit_NextSelected(&iter)) {
				Viewport_Target(u, g_activeAction, true, packed);
			}

			g_activeAction = 0xFFFF;
			GUI_ChangeSelectionType(SELECTIONTYPE_UNIT);
			return true;
		}

		/* Clicking LMB places structure. */
		else if ((g_selectionType == SELECTIONTYPE_PLACE) && !mouse_in_scroll_widget) {
			Viewport_Place();
			return true;
		}

		/* Clicking LMB begins minimap panning. */
		else if (w->index == 44) {
			viewport_click_action = VIEWPORT_PAN_MINIMAP;
		}

		/* Clicking LMB begins selection box or fast scroll. */
		else if (viewport_click_action == VIEWPORT_CLICK_NONE) {
			viewport_click_time = Timer_GetTicks();
			viewport_click_x = mouseX;
			viewport_click_y = mouseY;

			if (Input_Test(SCANCODE_LSHIFT)) {
				selection_box_add_to_selection = true;
				viewport_click_action = VIEWPORT_SELECTION_BOX;
			}
			else {
				viewport_click_action = VIEWPORT_LMB;
				selection_box_add_to_selection = false;

				/* For non-auto-scroll, decide fast scroll. */
				if ((!g_gameConfig.autoScroll) && Viewport_MouseInScrollWidget()) {
					viewport_click_action = VIEWPORT_FAST_SCROLL;
				}
			}
		}

		/* Clicking LMB cancels pan. */
		else {
			Viewport_StopMousePanning();
		}
	}

	if (w->state.buttonState & 0x10) {
		/* Clicking RMB begins panning. */
		if (viewport_click_action == VIEWPORT_CLICK_NONE) {
			viewport_click_action = VIEWPORT_RMB;
			viewport_click_time = Timer_GetTicks();
			viewport_click_x = g_mouseClickX;
			viewport_click_y = g_mouseClickY;
			viewport_pan_dx = 0;
			viewport_pan_dy = 0;
		}

		/* Clicking RMB cancels selection box. */
		else {
			viewport_click_action = VIEWPORT_CLICK_NONE;
		}
	}

	if (w->state.buttonState & 0x22) {
		if (viewport_click_action == VIEWPORT_LMB) {
			const int dx = viewport_click_x - mouseX;
			const int dy = viewport_click_y - mouseY;

			/* Dragging LMB begins selection box. */
			if (dx*dx + dy*dy >= 5*5) {
				viewport_click_action = VIEWPORT_SELECTION_BOX;
			}

			/* Holding LMB begins fast scroll. */
			else if (Viewport_MouseInScrollWidget() && (Timer_GetTicks() - viewport_click_time >= 10)) {
				viewport_click_action = VIEWPORT_FAST_SCROLL;
			}
		}

		else if (viewport_click_action == VIEWPORT_PAN_MINIMAP) {
			/* High-resolution panning. */
			const ScreenDiv *div = &g_screenDiv[SCREENDIV_SIDEBAR];
			const uint16 mapScale = g_scenario.mapScale;
			const MapInfo *mapInfo = &g_mapInfos[mapScale];

			float x, y;

			/* Minimap is (div->scale * 64) * (div->scale * 64).
			 * Each pixel represents 1 / (mapScale + 1) tiles.
			 */
			x = g_mouseX - div->x - div->scalex * g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].offsetX;
			y = g_mouseY - div->y - div->scaley * g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].offsetY;
			x = TILE_SIZE * mapInfo->minX + TILE_SIZE * x / (div->scalex * (mapScale + 1));
			y = TILE_SIZE * mapInfo->minY + TILE_SIZE * y / (div->scaley * (mapScale + 1));

			Map_CentreViewport(x, y);
		}

		else if (viewport_click_action == VIEWPORT_RMB) {
			const int dx = viewport_click_x - g_mouseX;
			const int dy = viewport_click_y - g_mouseY;

			if (dx*dx + dy*dy >= 15*15) {
				viewport_click_action = VIEWPORT_PAN;
				viewport_click_x = g_mouseX;
				viewport_click_y = g_mouseY;
				Video_HideCursor();
				g_mousePanning = true;
				g_mouseDX = 0;
				g_mouseDY = 0;
			}
		}

		/* Dragging RMB pans viewport. */
		else if (viewport_click_action == VIEWPORT_PAN) {
			const int dx = g_mouseDX;
			const int dy = g_mouseDY;

			g_viewport_desiredDX += dx * g_gameConfig.panSensitivity;
			g_viewport_desiredDY += dy * g_gameConfig.panSensitivity;
			g_mouseDX = 0;
			g_mouseDY = 0;
		}
	}

	if (w->state.buttonState & 0x04) {
		bool perform_selection_box = false;

		/* Releasing LMB performs selection box. */
		if (viewport_click_action == VIEWPORT_SELECTION_BOX) {
			perform_selection_box = true;
		}

		/* Releasing LMB performs selection box (alt: context_sensitive_action). */
		else if (viewport_click_action == VIEWPORT_LMB) {
			if (g_gameConfig.leftClickOrders) {
				if (Unit_AnySelected()) {
					if (Viewport_LMBAttemptsGenericCommand(packed))
						perform_context_sensitive_action = true;
				}
				else {
					const Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);

					/* Set rally point if we left click on an empty tile. */
					if ((s != NULL) && !g_map[packed].hasUnit && !g_map[packed].hasStructure &&
							Structure_SupportsRallyPoints(s->o.type)) {
						perform_context_sensitive_action = true;
					}
				}
			}

			if (!perform_context_sensitive_action)
				perform_selection_box = true;

			/* If click released over a scroll widget, don't select. */
			if (perform_selection_box && Viewport_MouseInScrollWidget())
				perform_selection_box = false;
		}

		if (perform_selection_box) {
			selection_box_x2 = Viewport_ClampSelectionBoxX(mouseX);
			selection_box_y2 = Viewport_ClampSelectionBoxY(mouseY);

			if (viewport_click_x > selection_box_x2) {
				const int swap = viewport_click_x;
				viewport_click_x = selection_box_x2;
				selection_box_x2 = swap;
			}

			if (viewport_click_y > selection_box_y2) {
				const int swap = viewport_click_y;
				viewport_click_y = selection_box_y2;
				selection_box_y2 = swap;
			}

			if (!selection_box_add_to_selection) {
				Map_SetSelection(0xFFFF);
				Unit_UnselectAll();
			}

			Viewport_SelectRegion();
		}

		viewport_click_action = VIEWPORT_CLICK_NONE;
	}

	if (w->state.buttonState & 0x40) {
		/* Releasing RMB cancels pan, but not target, placement. */
		if (viewport_click_action == VIEWPORT_PAN) {
		}

		/* Releasing RMB cancels target, placement. */
		else if (g_selectionType == SELECTIONTYPE_TARGET || g_selectionType == SELECTIONTYPE_PLACE) {
			GUI_Widget_Cancel_Click(NULL);
		}

		/* Releasing RMB performs context sensitive action (alt: deselect). */
		else if (viewport_click_action == VIEWPORT_RMB) {
			if (g_gameConfig.leftClickOrders) {
				const Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);

				/* Right-click on the selected structure clears the rally point, like RA3 airfields. */
				if ((s != NULL) && (s == Structure_Get_ByPackedTile(packed))) {
					perform_context_sensitive_action = true;
				}
				else {
					Map_SetSelection(0xFFFF);
					Unit_UnselectAll();
				}
			}
			else {
				perform_context_sensitive_action = true;
			}
		}

		/* Releasing RMB stops panning. */
		Viewport_StopMousePanning();
	}

	if (perform_context_sensitive_action) {
		Viewport_PerformContextSensitiveAction(packed, false);

		Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);
		if (s != NULL) {
			Structure_SetRallyPoint(s, packed);
		}
	}

	if (g_selectionType == SELECTIONTYPE_TARGET) {
		Map_SetSelection(Unit_FindTargetAround(packed));
	}
	else if (g_selectionType == SELECTIONTYPE_PLACE) {
		Map_SetSelection(packed);
	}

	return true;
}

void
Viewport_Hotkey(enum SquadID squad)
{
	if (!(SQUADID_1 <= squad && squad <= SQUADID_MAX))
		return;

	if (g_selectionType == SELECTIONTYPE_TARGET || g_selectionType == SELECTIONTYPE_PLACE) {
		GUI_Widget_Cancel_Click(NULL);
	}

	const bool key_ctrl = Input_Test(SCANCODE_LCTRL); /* same as SCANCODE_RCTRL. */

	bool centre_on_selection = Input_Test(SCANCODE_LALT); /* same as SCANCODE_RALT. */
	int cx, cy, count;
	PoolFindStruct find;

	cx = 0;
	cy = 0;
	count = 0;

	if (key_ctrl) {
		find.houseID = HOUSE_INVALID;
		find.type = 0xFFFF;
		find.index = 0xFFFF;

		Unit *u = Unit_Find(&find);
		while (u != NULL) {
			if ((Unit_GetHouseID(u) == g_playerHouseID) && Unit_IsSelected(u)) {
				u->squadID = squad;

				cx += (u->o.position.x >> 4);
				cy += (u->o.position.y >> 4);
				count++;
			}
			else if (u->squadID == squad) {
				u->squadID = SQUADID_INVALID;
			}

			u = Unit_Find(&find);
		}

		find.houseID = g_playerHouseID;
		find.type = 0xFFFF;
		find.index = 0xFFFF;

		const Structure *st = Structure_Get_ByPackedTile(g_selectionPosition);
		if (st && (st->o.houseID != g_playerHouseID))
			st = NULL;

		Structure *s;
		while ((s = Structure_Find(&find)) != NULL) {
			if (s == st) {
				s->squadID = squad;
			}
			else if (s->squadID == squad) {
				s->squadID = SQUADID_INVALID;
			}
		}

		Audio_PlaySample(SAMPLE_BUTTON, 128, 0.0f);
	}
	else {
		const bool key_shift = Input_Test(SCANCODE_LSHIFT);

		bool modified_selection = false;

		find.houseID = HOUSE_INVALID;
		find.type = 0xFFFF;
		find.index = 0xFFFF;

		Unit *u = Unit_Find(&find);
		while (u != NULL) {
			const bool is_controllable = (Unit_GetHouseID(u) == g_playerHouseID);

			if ((u->squadID == squad) && is_controllable) {
				if (!Unit_IsSelected(u)) {
					Unit_Select(u);
					modified_selection = true;
				}
			}
			else if (!key_shift || !is_controllable) {
				if (Unit_IsSelected(u)) {
					Unit_Unselect(u);
					modified_selection = true;
				}
			}

			if (Unit_IsSelected(u)) {
				cx += (u->o.position.x >> 4);
				cy += (u->o.position.y >> 4);
				count++;
			}

			u = Unit_Find(&find);
		}

		if (!Unit_AnySelected()) {
			find.houseID = g_playerHouseID;
			find.type = 0xFFFF;
			find.index = 0xFFFF;

			Structure *s;
			while ((s = Structure_Find(&find)) != NULL) {
				if (s->squadID == squad) {
					const StructureInfo *si = &g_table_structureInfo[s->o.type];
					const XYSize *layout = &g_table_structure_layoutSize[si->layout];

					if (Tile_PackTile(s->o.position) != g_selectionPosition)
						modified_selection = true;

					Map_SetSelection(Tile_PackTile(s->o.position));
					cx = (s->o.position.x >> 4) + TILE_SIZE * layout->width / 2;
					cy = (s->o.position.y >> 4) + TILE_SIZE * layout->height / 2;
					count = 1;
					break;
				}
			}
		}

		if (!modified_selection)
			centre_on_selection = true;
	}

	if (centre_on_selection && (count > 0)) {
		Map_CentreViewport(cx / count, cy / count);
	}
}

void
Viewport_Homekey(void)
{
	Structure *s;
	bool centre_on_selection = false;

	if (g_selectionType == SELECTIONTYPE_TARGET) {
		GUI_Widget_Cancel_Click(NULL);
	}
	else if (g_selectionType == SELECTIONTYPE_PLACE) {
		centre_on_selection = true;
	}

	PoolFindStruct find;

	find.houseID = g_playerHouseID;
	find.type = STRUCTURE_CONSTRUCTION_YARD;
	find.index = 0xFFFF;

	s = Structure_Find(&find);
	if (s != NULL) {
		if (g_selectionType != SELECTIONTYPE_PLACE) {
			if (s == Structure_Get_ByPackedTile(g_selectionPosition)) {
				centre_on_selection = true;
			}
			else {
				Map_SetSelection(Tile_PackTile(s->o.position));
			}
		}

		if (centre_on_selection) {
			Map_CentreViewport((s->o.position.x >> 4) + TILE_SIZE, (s->o.position.y >> 4) + TILE_SIZE);
		}
	}
}

static bool
Viewport_TileIsDebris(uint16 iconID)
{
	/* 3x2, 3x3. */
	if ((g_iconMap[g_iconMap[ICM_ICONGROUP_HOUSE_PALACE] +  9] <= iconID && iconID <= g_iconMap[g_iconMap[ICM_ICONGROUP_HOUSE_PALACE] + 11]) ||
	    (g_iconMap[g_iconMap[ICM_ICONGROUP_HOUSE_PALACE] + 12] <= iconID && iconID <= g_iconMap[g_iconMap[ICM_ICONGROUP_HOUSE_PALACE] + 14]) ||
	    (g_iconMap[g_iconMap[ICM_ICONGROUP_HOUSE_PALACE] + 15] <= iconID && iconID <= g_iconMap[g_iconMap[ICM_ICONGROUP_HOUSE_PALACE] + 17]))
		return true;

	/* 2x2. */
	if (iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY] + 4] ||
	    iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY] + 5] ||
	    iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY] + 6] ||
	    iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY] + 7])
		return true;

	/* 1x1. */
	if (iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_BASE_DEFENSE_TURRET] + 1])
		return true;

	/* Centre of starport is special. */
	if (iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_STARPORT_FACILITY] + 13])
		return true;

	/* Outpost is special. */
	if (iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_RADAR_OUTPOST] + 4] ||
	    iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_RADAR_OUTPOST] + 5] ||
	    iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_RADAR_OUTPOST] + 6] ||
	    iconID == g_iconMap[g_iconMap[ICM_ICONGROUP_RADAR_OUTPOST] + 7])
		return true;

	return false;
}

static void
Viewport_DrawTilesInRange(int x0, int y0,
		int viewportX1, int viewportY1, int viewportX2, int viewportY2,
		bool draw_tile, bool draw_fog)
{
	const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
	int left, top;
	int x, y;

	/* Find top and left boundaries. */
	for (; x0 < mapInfo->minX; x0++, viewportX1 += TILE_SIZE) {}
	for (; y0 < mapInfo->minY; y0++, viewportY1 += TILE_SIZE) {}

	/* Find bottom and right boundaries. */
	left = viewportX1;
	top = viewportY1;
	for (x = x0; (x < mapInfo->minX + mapInfo->sizeX) && (left < viewportX2); x++, left += TILE_SIZE) {}
	for (y = y0; (y < mapInfo->minY + mapInfo->sizeY) && (top < viewportY2); y++, top += TILE_SIZE) {}
	viewportX2 = left;
	viewportY2 = top;

	Video_HoldBitmapDrawing(true);

	y = y0;
	for (top = viewportY1; top < viewportY2; top += TILE_SIZE, y++) {
		int curPos = Tile_PackXY(x0, y);
		const Tile *t = &g_map[curPos];
		const FogOfWarTile *f = &g_mapVisible[curPos];

		for (left = viewportX1; left < viewportX2; left += TILE_SIZE, curPos++, t++, f++) {
			if (draw_tile && (t->overlaySpriteID != g_veiledSpriteID)) {
				if (Viewport_TileIsDebris(f->groundSpriteID)) {
					const uint16 iconID = g_mapSpriteID[curPos] & ~0x8000;

					Video_DrawIcon(iconID, HOUSE_HARKONNEN, left, top);
				}

				if (f->groundSpriteID)
					Video_DrawIcon(f->groundSpriteID, f->houseID, left, top);

				if (f->overlaySpriteID != 0)
					Video_DrawIcon(f->overlaySpriteID, f->houseID, left, top);

				/* Draw the transparent fog UNDER units, which doesn't
				 * really conceal units anyway.  This prevents it from
				 * darkening the blur effect's rendering again.
				 */
				if (enhancement_fog_of_war && f->fogOverlayBits) {
					uint16 iconID = g_veiledSpriteID - 16 + f->fogOverlayBits;
					Video_DrawIconAlpha(iconID, left, top, 0x80);
				}
			}

			if (draw_fog) {
				const bool overlay_is_fog = (g_veiledSpriteID - 16 <= t->overlaySpriteID && t->overlaySpriteID <= g_veiledSpriteID);

				if (overlay_is_fog) {
					uint16 iconID = t->overlaySpriteID;

					if (t->overlaySpriteID == g_veiledSpriteID)
						iconID = g_veiledSpriteID - 1;

					Video_DrawIcon(iconID, t->houseID, left, top);
				}
			}
		}
	}

	Video_HoldBitmapDrawing(false);

#if 0
	/* Debugging. */
	for (int x = x0, left = viewportX1; (x < MAP_SIZE_MAX) && (left <= viewportX2); x++, left += TILE_SIZE)
		GUI_DrawText_Wrapper("%d", left, viewportY1, 15, 0, 0x21, x);

	for (int y = y0, top = viewportY1; (y < MAP_SIZE_MAX) && (top <= viewportY2); y++, top += TILE_SIZE)
		GUI_DrawText_Wrapper("%d", viewportX1, top, 6, 0, 0x21, y);
#endif
}

void
Viewport_DrawTiles(void)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
	const int viewportX1 = -g_viewport_scrollOffsetX;
	const int viewportY1 = -g_viewport_scrollOffsetY;
	const int viewportX2 = wi->width;
	const int viewportY2 = wi->height;
	const int x0 = Tile_GetPackedX(g_viewportPosition);
	const int y0 = Tile_GetPackedY(g_viewportPosition);

	/* ENHANCEMENT -- Draw fog over the top of units. */
	const bool draw_fog = enhancement_fog_covers_units ? false : true;

	Viewport_DrawTilesInRange(x0, y0, viewportX1, viewportY1, viewportX2, viewportY2, true, draw_fog);
}

void
Viewport_DrawTileFog(void)
{
	if (!enhancement_fog_covers_units)
		return;

	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
	const int viewportX1 = -g_viewport_scrollOffsetX;
	const int viewportY1 = -g_viewport_scrollOffsetY;
	const int viewportX2 = wi->width;
	const int viewportY2 = wi->height;
	const int x0 = Tile_GetPackedX(g_viewportPosition);
	const int y0 = Tile_GetPackedY(g_viewportPosition);

	Viewport_DrawTilesInRange(x0, y0, viewportX1, viewportY1, viewportX2, viewportY2, false, true);
}

void
Viewport_DrawRallyPoint(void)
{
	if (g_selectionType != SELECTIONTYPE_STRUCTURE)
		return;

	const Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);

	if ((s == NULL) || (s->rallyPoint == 0xFFFF))
		return;

	if (Structure_SupportsRallyPoints(s->o.type)) {
		const int tx = Tile_GetPackedX(g_viewportPosition);
		const int ty = Tile_GetPackedY(g_viewportPosition);
		const int x1 = -g_viewport_scrollOffsetX + (TILE_SIZE * (Tile_GetPackedX(g_selectionRectanglePosition) - tx)) + (TILE_SIZE * g_selectionWidth)/2;
		const int y1 = -g_viewport_scrollOffsetY + (TILE_SIZE * (Tile_GetPackedY(g_selectionRectanglePosition) - ty)) + (TILE_SIZE * g_selectionHeight)/2;
		const int x2 = -g_viewport_scrollOffsetX + (TILE_SIZE * (Tile_GetPackedX(s->rallyPoint) - tx)) + TILE_SIZE/2;
		const int y2 = -g_viewport_scrollOffsetY + (TILE_SIZE * (Tile_GetPackedY(s->rallyPoint) - ty)) + TILE_SIZE/2;

		Prim_Line(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, 14, 1.0f);
		Shape_DrawTint(SHAPE_CURSOR_TARGET, x2, y2, 14, 0, 0x8000);
	}
}

static void
Viewport_DrawHealthBar(int x, int y, int width, int curr, int max)
{
	const float deltax = 1.0f / g_screenDiv[SCREENDIV_VIEWPORT].scalex;
	const float deltay = 1.0f / g_screenDiv[SCREENDIV_VIEWPORT].scaley;
	const int w = max(1, min(width, width * curr / max));

	/* From ActionPanel_DrawHealthBar. */
	uint8 colour = 4;
	if (curr <= max / 2) colour = 5;
	if (curr <= max / 4) colour = 8;

	if (enhancement_high_res_overlays) {
		Prim_FillRect(x - deltax, y - deltay, x + width + 1.0f + deltax, y + 1.0f + deltay, 12);
	}
	else {
		Prim_FillRect_i(x - 1, y - 1, x + width + 1, y + 1, 12);
	}

	if (w < width)
		Prim_Hline(x + w + 1, y, x + width, 13);

	Prim_Hline(x, y, x + w, colour);
}

static void
Viewport_DrawSpiceBricks(int x, int y, int num_bricks, int curr, int max)
{
	if (max < num_bricks)
		return;

	/* A spice bricks stores (max/num_bricks) credits.  However, to
	 * avoid round-off errors, we have multiplied curr and max by
	 * num_bricks in the following calcuations.
	 */
	curr = min(curr, max);

	const float deltax = 1.0f / g_screenDiv[SCREENDIV_VIEWPORT].scalex;
	const float deltay = 1.0f / g_screenDiv[SCREENDIV_VIEWPORT].scaley;
	const float width = 2.0f * num_bricks;
	const int w = 2 * (num_bricks * curr / max);

	/* Black outline. */
	if (enhancement_high_res_overlays) {
		Prim_FillRect(x - deltax, y - deltay, x + width + deltax, y + 1.0f + deltay, 12);
	}
	else {
		Prim_FillRect_i(x - 1, y - 1, x + width, y + 1, 12);
	}

	if (curr < max) {
		/* Interpolate from 0x545454 to 0xFC4400. */
		const int rem = (num_bricks * curr) % max;
		unsigned char r = 0x54 + (0xFC - 0x54) * rem / max;
		unsigned char g = 0x54 + (0x44 - 0x54) * rem / max;
		unsigned char b = 0x54 + (0x00 - 0x54) * rem / max;

		if (w > deltax) Prim_FillRect(x, y, x + w - deltax, y + 1.0f, 83);
		Prim_FillRect(x + w, y, x + width, y + 1.0f, 13);
		Prim_FillRect_RGBA(x + w, y, x + w + 2.0f, y + 1.0f, r, g, b, 0xFF);
	}
	else {
		Prim_FillRect(x, y, x + width, y + 1.0f, 83);
	}

	/* Divide into bricks. */
	x += 2;
	float thickness = enhancement_high_res_overlays ? 0.0f : 1.0f;
	for (int i = 0; i < num_bricks - 1; i++, x += 2) {
		Prim_Line(x, y, x, y + 1.0f + deltay, 12, thickness);
	}
}

void
Viewport_DrawSelectionHealthBars(void)
{
	if (enhancement_draw_health_bars == HEALTH_BAR_DISABLE)
		return;

	PoolFindStruct find;
	int iter;
	Structure *s;
	Unit *u = Unit_FirstSelected(&iter);
	const bool unit_selected = (u != NULL);

	/* Draw a structure's spice bricks under a unit's health bar. */
	if (!unit_selected &&
			(g_selectionType != SELECTIONTYPE_PLACE) &&
			(g_selectionPosition != 0xFFFF) &&
			(g_mapVisible[g_selectionPosition].fogOverlayBits != 0xF) &&
			(s = Structure_Get_ByPackedTile(g_selectionPosition)) != NULL) {
		const StructureInfo *si = &g_table_structureInfo[s->o.type];
		const int x = TILE_SIZE * (Tile_GetPackedX(g_selectionPosition) - Tile_GetPackedX(g_viewportPosition)) - g_viewport_scrollOffsetX;
		const int ty = Tile_GetPackedY(g_selectionPosition);

		int y = TILE_SIZE * (ty - Tile_GetPackedY(g_viewportPosition)) - g_viewport_scrollOffsetY;

		if ((s->o.type == STRUCTURE_REFINERY || s->o.type == STRUCTURE_SILO) && (s->o.houseID == g_playerHouseID)) {
			const House *h = g_playerHouse;
			int creditsStored = h->credits * si->creditsStorage / h->creditsStorage;
			if (h->credits > h->creditsStorage) creditsStored = si->creditsStorage;

			Viewport_DrawSpiceBricks(x + 2, y + TILE_SIZE * g_selectionHeight - 3, 10, creditsStored, si->creditsStorage);
		}

		y += (ty == g_mapInfos[g_scenario.mapScale].minY ? 1 : -2);
		Viewport_DrawHealthBar(x + 1, y, TILE_SIZE * g_selectionWidth - 3, s->o.hitpoints, si->o.hitpoints);
	}

	if (enhancement_draw_health_bars == HEALTH_BAR_ALL_UNITS) {
		find.houseID = g_playerHouseID;
		find.type = 0xFFFF;
		find.index = 0xFFFF;

		u = Unit_Find(&find);
	}

	while (u != NULL) {
		const uint16 packed = Tile_PackTile(u->o.position);
		const UnitInfo *ui = &g_table_unitInfo[u->o.type];

		if (ui->o.flags.tabSelectable && Map_IsValidPosition(packed) && (g_mapVisible[packed].fogOverlayBits != 0xF)) {
			int x, y;

			Map_IsPositionInViewport(u->o.position, &x, &y);

			y = y - TILE_SIZE / 2 - 3;

			/* Shift the meter down if off the top of the screen. */
			if ((u->o.position.y >> 4) - TILE_SIZE / 2 - 3 <= TILE_SIZE * g_mapInfos[g_scenario.mapScale].minY) {
				y += TILE_SIZE * g_mapInfos[g_scenario.mapScale].minY - ((u->o.position.y >> 4) - TILE_SIZE / 2 - 3);
			}

			Viewport_DrawHealthBar(x - 7, y, 13, u->o.hitpoints, ui->o.hitpoints);

			if ((u->o.type == UNIT_HARVESTER) && (Unit_GetHouseID(u) == g_playerHouseID))
				Viewport_DrawSpiceBricks(x - 7, y + 2, 7, u->amount, 100);
		}

		if (enhancement_draw_health_bars == HEALTH_BAR_ALL_UNITS) {
			u = Unit_Find(&find);
		}
		else {
			u = Unit_NextSelected(&iter);
		}
	}
}

static void
Viewport_DrawSelectedUnit(int x, int y)
{
	if (enhancement_high_res_overlays) {
		const int x1 = x - TILE_SIZE/2 + 1;
		const int y1 = y - TILE_SIZE/2 + 1;
		const int x2 = x1 + TILE_SIZE - 2;
		const int y2 = y1 + TILE_SIZE - 2;

		Prim_Line(x1 + 0.33f, y1 + 3.66f, x1 + 3.66f, y1 + 0.33f, 0xFF, 0.75f);
		Prim_Line(x2 - 3.66f, y1 + 0.33f, x2 - 0.33f, y1 + 3.66f, 0xFF, 0.75f);
		Prim_Line(x1 + 0.33f, y2 - 3.66f, x1 + 3.66f, y2 - 0.33f, 0xFF, 0.75f);
		Prim_Line(x2 - 3.66f, y2 - 0.33f, x2 - 0.33f, y2 - 3.66f, 0xFF, 0.75f);
	}
	else {
		Shape_DrawTint(SHAPE_SELECTED_UNIT, x, y, 0xFF, 0, 0x8000);
	}
}

void
Viewport_DrawSandworm(const Unit *u)
{
	/* if (!g_map[Tile_PackTile(u->o.position)].isUnveiled && !g_debugScenario) return; */
	if (g_mapVisible[Tile_PackTile(u->o.position)].fogOverlayBits == 0xF)
		return;

	const enum ShapeID shapeID = g_table_unitInfo[UNIT_SANDWORM].groundSpriteID;
	int x, y;

	if (Map_IsPositionInViewport(u->o.position, &x, &y))
		Shape_Draw(shapeID, x, y, WINDOWID_VIEWPORT, 0xC200 | ((u->wobbleIndex & 0x7) << 4));

	if (Map_IsPositionInViewport(u->targetLast, &x, &y))
		Shape_Draw(shapeID, x, y, WINDOWID_VIEWPORT, 0xC200 | (((u->wobbleIndex + 1) & 0x7) << 4));

	if (Map_IsPositionInViewport(u->targetPreLast, &x, &y))
		Shape_Draw(shapeID, x, y, WINDOWID_VIEWPORT, 0xC200 | (((u->wobbleIndex + 2) & 0x7) << 4));

	if (Unit_IsSelected(u)) {
		if (Map_IsPositionInViewport(u->o.position, &x, &y))
			Viewport_DrawSelectedUnit(x, y);
	}
}

static void
Viewport_DrawUnitHarvesting(const Unit *u, uint8 orientation, int x, int y)
{
	const int16 values_334E[8][2] = {
		{0, 7},  {-7,  6}, {-14, 1}, {-9, -6},
		{0, -9}, { 9, -6}, { 14, 1}, { 7,  6}
	};

	enum ShapeID shapeID = (u->spriteOffset % 3) + 0xDF + (values_32A4[orientation][0] * 3);
	uint16 spriteFlags = values_32A4[orientation][1];

	x += values_334E[orientation][0];
	y += values_334E[orientation][1];

	Shape_Draw(shapeID, x, y, 0, spriteFlags | 0x8000);
}

static void
Viewport_DrawUnitTurret(const Unit *u, const UnitInfo *ui, int x, int y)
{
	const int16 values_336E[8][2] = { /* siege tank */
		{ 0, -5}, { 0, -5}, { 2, -3}, { 2, -1},
		{-1, -3}, {-2, -1}, {-2, -3}, {-1, -5}
	};

	const int16 values_338E[8][2] = { /* devastator */
		{ 0, -4}, {-1, -3}, { 2, -4}, {0, -3},
		{-1, -3}, { 0, -3}, {-2, -4}, {1, -3}
	};

	uint8 orientation = Orientation_256To8(u->orientation[ui->o.flags.hasTurret ? 1 : 0].current);
	enum ShapeID shapeID = ui->turretSpriteID + values_32A4[orientation][0];
	uint16 spriteFlags = values_32A4[orientation][1];

	switch (ui->turretSpriteID) {
		case 0x8D: /* sonic tank */
			y += -2;
			break;

		case 0x92: /* rocket launcher */
			y += -3;
			break;

		case 0x7E: /* siege tank */
			x += values_336E[orientation][0];
			y += values_336E[orientation][1];
			break;

		case 0x88: /* devastator */
			x += values_338E[orientation][0];
			y += values_338E[orientation][1];
			break;

		default:
			break;
	}

	Shape_DrawRemap(shapeID, Unit_GetHouseID(u), x, y, 0, spriteFlags | 0xA000);
}

void
Viewport_DrawUnit(const Unit *u, int windowX, int windowY, bool render_for_blur_effect)
{
	const uint16 values_32C4[8][2] = {
		{0, 0}, {1, 0}, {1, 0}, {1, 0},
		{2, 0}, {1, 1}, {1, 1}, {1, 1}
	};

	const uint16 values_334A[4] = {0, 1, 0, 2};

	uint16 packed = Tile_PackTile(u->o.position);

	/* if (!g_map[packed].isUnveiled && !g_debugScenario) return; */
	if (g_mapVisible[packed].fogOverlayBits == 0xF)
		return;

	int x, y;

	if (render_for_blur_effect) {
		Map_IsPositionInViewport(u->o.position, &x, &y);

		x -= windowX;
		y -= windowY;
	}
	else {
		if (!Map_IsPositionInViewport(u->o.position, &x, &y))
			return;
	}

	x += (int16)g_table_tilediff[0][u->wobbleIndex].x;
	y += (int16)g_table_tilediff[0][u->wobbleIndex].y;

	uint16 s_spriteFlags = 0;
	uint16 index;

	const UnitInfo *ui = &g_table_unitInfo[u->o.type];
	uint8 orientation = Orientation_256To8(u->orientation[0].current);

	if (u->spriteOffset >= 0 || ui->destroyedSpriteID == 0) {
		index = ui->groundSpriteID;

		switch (ui->displayMode) {
			case DISPLAYMODE_UNIT:
			case DISPLAYMODE_ROCKET:
				if (ui->movementType == MOVEMENT_SLITHER) break;
				index += values_32A4[orientation][0];
				s_spriteFlags = values_32A4[orientation][1];
				break;

			case DISPLAYMODE_INFANTRY_3_FRAMES:
				index += values_32C4[orientation][0] * 3;
				index += values_334A[u->spriteOffset & 3];
				s_spriteFlags = values_32C4[orientation][1];
				break;

			case DISPLAYMODE_INFANTRY_4_FRAMES:
				index += values_32C4[orientation][0] * 4;
				index += u->spriteOffset & 3;
				s_spriteFlags = values_32C4[orientation][1];
				break;

			default:
				s_spriteFlags = 0;
				break;
		}
	} else {
		index = ui->destroyedSpriteID - u->spriteOffset - 1;
		s_spriteFlags = 0;
	}

	if (u->o.type != UNIT_SANDWORM && u->o.flags.s.isHighlighted) s_spriteFlags |= 0x100;
	if (ui->o.flags.blurTile)
		s_spriteFlags |= 0x200 | ((u->wobbleIndex & 0x7) << 4);

	Shape_DrawRemap(index, Unit_GetHouseID(u), x, y, 0, s_spriteFlags | 0xA000);

	/* XXX: Is it just ACTION_HARVEST, or ACTION_MOVE too? */
	if (u->o.type == UNIT_HARVESTER && u->actionID == ACTION_HARVEST && u->spriteOffset >= 0 && (u->actionID == ACTION_HARVEST || u->actionID == ACTION_MOVE)) {
		uint16 type = Map_GetLandscapeType(packed);

		if (type == LST_SPICE || type == LST_THICK_SPICE)
			Viewport_DrawUnitHarvesting(u, orientation, x, y);
	}

	if (u->spriteOffset >= 0 && ui->turretSpriteID != 0xFFFF) {
		Viewport_DrawUnitTurret(u, ui, x, y);
	}

	if (u->o.flags.s.isSmoking) {
		enum ShapeID shapeID = 180 + (u->spriteOffset & 3);
		if (shapeID == 183) shapeID = 181;

		Shape_Draw(shapeID, x, y - 14, 0, 0x8000);
	}

	if (Unit_IsSelected(u) && !render_for_blur_effect)
		Viewport_DrawSelectedUnit(x, y);

#if 0
	/* Debugging. */
	GUI_DrawText_Wrapper("id:%x", x - TILE_SIZE / 2, y, 15, 0, 0x11, u->o.index);

	/* Path finding. */
	Map_IsPositionInViewport(u->currentDestination, &x, &y);
	for (int i = 0; (i < 14) && (u->route[i] != 0xFF); i++) {
		if (u->route[i] == 0 || u->route[i] == 1 || u->route[i] == 7) y -= TILE_SIZE;
		if (u->route[i] == 1 || u->route[i] == 2 || u->route[i] == 3) x += TILE_SIZE;
		if (u->route[i] == 3 || u->route[i] == 4 || u->route[i] == 5) y += TILE_SIZE;
		if (u->route[i] == 5 || u->route[i] == 6 || u->route[i] == 7) x -= TILE_SIZE;

		GUI_DrawText_Wrapper("%x", x, y - 4, 15, 0, 0x111, u->route[i]);
	}

	if (Map_IsPositionInViewport(Tools_Index_GetTile(u->targetMove), &x, &y)) {
		Shape_Draw(SHAPE_CURSOR_TARGET, x, y, 0, 0x8000);
	}
#endif
}

void
Viewport_DrawAirUnit(const Unit *u)
{
	/* For sprites drawn at {0, 45, 90} degrees (e.g. carryalls). */
	const uint16 values_32E4[8][2] = {
		{0, 0}, {1, 0}, {2, 0}, {1, 2},
		{0, 2}, {1, 3}, {2, 1}, {1, 1}
	};

	/* For sprites drawn at {0, 30, 45, 60, 90} degrees. */
	const uint16 values_3304[16][2] = {
		{0, 0}, {1, 0}, {2, 0}, {3, 0},
		{4, 0}, {3, 2}, {2, 2}, {1, 2},
		{0, 2}, {3, 3}, {2, 3}, {3, 3},
		{4, 1}, {3, 1}, {2, 1}, {1, 1}
	};

	const uint16 values_33AE[4] = {2, 1, 0, 1}; /* ornithopter */

	uint16 curPos = Tile_PackTile(u->o.position);

	/* Allied air units don't get concealed by fog of war. */
	/* if (!g_map[curPos].isUnveiled && !g_debugScenario) return; */
	if ((!g_map[curPos].isUnveiled) ||
	    ((g_mapVisible[curPos].fogOverlayBits == 0xF) && !House_AreAllied(u->o.houseID, g_playerHouseID)))
		return;

	int x, y;
	if (!Viewport_InterpolateMovement(u, &x, &y))
		return;

	const UnitInfo *ui = &g_table_unitInfo[u->o.type];
	const bool smooth_rotation =
		(enhancement_smooth_unit_animation == SMOOTH_UNIT_ANIMATION_ENABLE) &&
		(ui->displayMode == DISPLAYMODE_UNIT ||
		 ui->displayMode == DISPLAYMODE_ROCKET ||
		 ui->displayMode == DISPLAYMODE_ORNITHOPTER);

	uint16 s_spriteFlags = 0xC000;
	uint16 index = ui->groundSpriteID;

	switch (ui->displayMode) {
		case DISPLAYMODE_SINGLE_FRAME:
			if (u->o.flags.s.bulletIsBig) index++;
			break;

		case DISPLAYMODE_ORNITHOPTER: /* ornithopter */
			index += values_33AE[u->spriteOffset & 3];
			/* Fall through. */
		case DISPLAYMODE_UNIT: /* carryall, frigate */
			if (!smooth_rotation) {
				const uint8 orient8 = Orientation_256To8(u->orientation[0].current);

				index += values_32E4[orient8][0];
				s_spriteFlags |= values_32E4[orient8][1];
			}
			break;

		case DISPLAYMODE_ROCKET: /* rockets */
			if (!smooth_rotation) {
				const uint8 orient16 = Orientation_256To16(u->orientation[0].current);

				index += values_3304[orient16][0];
				s_spriteFlags |= values_3304[orient16][1];
			}
			break;

		default:
			s_spriteFlags = 0x0;
			break;
	}

	if (ui->flags.hasAnimationSet && u->o.flags.s.animationFlip)
		index += 5;

	if (u->o.type == UNIT_CARRYALL && u->o.flags.s.inTransport)
		index += 3;

	if (ui->o.flags.hasShadow) {
		/* ENHANCEMENT -- Mimic the bug that caused carryall shadows to flicker. */
		const int alpha = ((u->distanceToDestination & 0x1F) <= 0x1) ? 0x390 : 0x360;

		if (smooth_rotation) {
			Shape_DrawRemapRotate(index, Unit_GetHouseID(u),
					x + 1, y + 3, &u->orientation[0], WINDOWID_VIEWPORT, (s_spriteFlags & 0x5FFF) | alpha);
		}
		else {
			Shape_Draw(index, x + 1, y + 3, WINDOWID_VIEWPORT, (s_spriteFlags & 0xDFFF) | alpha);
		}
	}

	if (ui->o.flags.blurTile)
		s_spriteFlags |= 0x200 | ((u->wobbleIndex & 0x7) << 4);

	if (smooth_rotation) {
		Shape_DrawRemapRotate(index, Unit_GetHouseID(u), x, y, &u->orientation[0], WINDOWID_VIEWPORT, (s_spriteFlags & 0x7FFF) | 0x2000);
	}
	else {
		Shape_DrawRemap(index, Unit_GetHouseID(u), x, y, WINDOWID_VIEWPORT, s_spriteFlags | 0x2000);
	}
}

void
Viewport_DrawSelectionBox(void)
{
	if (viewport_click_action != VIEWPORT_SELECTION_BOX)
		return;

	int mouseX, mouseY;
	Mouse_TransformToDiv(SCREENDIV_VIEWPORT, &mouseX, &mouseY);

	int x1 = viewport_click_x;
	int y1 = viewport_click_y;
	int x2 = Viewport_ClampSelectionBoxX(mouseX);
	int y2 = Viewport_ClampSelectionBoxY(mouseY);

	/* Make x1 <= x2, y1 <= y2 so that rectangles are not rounded off. */
	if (x1 > x2) {
		x1 = x2;
		x2 = viewport_click_x;
	}

	if (y1 > y2) {
		y1 = y2;
		y2 = viewport_click_y;
	}

	Prim_Rect_i(x1, y1, x2, y2, 0xFF);
}

void
Viewport_DrawPanCursor(void)
{
	if (viewport_click_action != VIEWPORT_PAN)
		return;

	const int x = viewport_click_x;
	const int y = viewport_click_y;

	/* Note: cursor shapes are 16x16 sprites, aligned to top-left corner, so manually centre. */
	A5_UseTransform(SCREENDIV_MAIN);
	Shape_Draw(SHAPE_CURSOR_UP,    x -  5, y - 13, 0, 0);
	Shape_Draw(SHAPE_CURSOR_RIGHT, x +  5, y -  5, 0, 0);
	Shape_Draw(SHAPE_CURSOR_DOWN,  x -  5, y +  5, 0, 0);
	Shape_Draw(SHAPE_CURSOR_LEFT,  x - 13, y -  5, 0, 0);
	A5_UseTransform(SCREENDIV_VIEWPORT);
}

static void
Viewport_DrawInterface(enum HouseType houseID, int x, int blurx)
{
	const ScreenDiv *sidebar = &g_screenDiv[SCREENDIV_SIDEBAR];
	const ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];

	if (x + 3 * TILE_SIZE < g_screenDiv[SCREENDIV_VIEWPORT].width)
		return;

	x = g_screenDiv[SCREENDIV_VIEWPORT].width - blurx;

	/* Scaling factor between sidebar -> viewport->scale. */
	float scale = sidebar->scaley / viewport->scaley;

	for (int y = sidebar->height - 85 - 52; y + 52 - 1 >= 17; y -= 52) {
#if 0
		/* Translate y in sidebar's coordinates -> screen -> viewport coordinates. */
		float screen_y = sidebar->y + sidebar->scale * y;
		float desty = (screen_y - viewport->y) / viewport->scale;
#endif

		Video_DrawCPSSpecialScale(CPS_SIDEBAR_MIDDLE, houseID, x, y * scale, scale);
	}

	Video_DrawCPSSpecialScale(CPS_SIDEBAR_TOP, houseID, x, 0, scale);
	Video_DrawCPSSpecialScale(CPS_SIDEBAR_BOTTOM, houseID, x, (sidebar->height - 85) * scale, scale);
}

/* Viewport_RenderBrush:
 *
 * A mini rendering routine used for the sandworm and sonic wave
 * effects.  Renders the 3x3 tiles (the largest brush is 24x24)
 * containing the wave with top-left corner (x, y) into the active
 * buffer.
 */
void
Viewport_RenderBrush(int x, int y, int blurx)
{
	/* Top-left tile in viewport. */
	const int tile_left = Tile_GetPackedX(g_viewportPosition);
	const int tile_top  = Tile_GetPackedY(g_viewportPosition);

	/* Translate x, y to absolute coordinates. */
	const int sx = TILE_SIZE * tile_left + g_viewport_scrollOffsetX + x;
	const int sy = TILE_SIZE * tile_top  + g_viewport_scrollOffsetY + y;

	/* Tile containing top-left corner of brush. */
	const int tile_x0 = sx / TILE_SIZE;
	const int tile_y0 = sy / TILE_SIZE;

	const int viewportX1 = TILE_SIZE * (tile_x0 - tile_left) - g_viewport_scrollOffsetX - blurx;
	const int viewportY1 = TILE_SIZE * (tile_y0 - tile_top)  - g_viewport_scrollOffsetY;
	const int viewportX2 = viewportX1 + 3 * TILE_SIZE;
	const int viewportY2 = viewportY1 + 3 * TILE_SIZE;
	const bool draw_fog = enhancement_fog_covers_units ? false : true;

	/* Draw tiles. */
	Viewport_DrawTilesInRange(tile_x0, tile_y0,
			viewportX1, viewportY1, viewportX2, viewportY2, true, draw_fog);

	/* Draw ground units (not sandworms, projectiles, etc.). */
	for (int dy = 0; dy < 3; dy++) {
		for (int dx = 0; dx < 3; dx++) {
			const int tilex = tile_x0 + dx;
			const int tiley = tile_y0 + dy;

			if (!(Map_InRange(tilex) && Map_InRange(tiley)))
				continue;

			const uint16 packed = Tile_PackXY(tilex, tiley);
			const Tile *t = &g_map[packed];

			if (!t->hasUnit || (t->index == 0))
				continue;

			const int index = t->index - 1;
			if ((19 <= index && index <= 21 && !enhancement_invisible_saboteurs) ||
			    (22 <= index && index <= 101)) {
			}
			else {
				continue;
			}

			const Unit *u = Unit_Get_ByIndex(index);
			const UnitInfo *ui = &g_table_unitInfo[u->o.type];

			if (ui->o.flags.blurTile)
				continue;

			Viewport_DrawUnit(u, blurx, 0, true);
		}
	}

	/* Draw fog. */
	if (!draw_fog) {
		Viewport_DrawTilesInRange(tile_x0, tile_y0,
				viewportX1, viewportY1, viewportX2, viewportY2, false, true);
	}

	/* Render interface. */
	Viewport_DrawInterface(g_playerHouseID, x, blurx);
}

static bool
Viewport_InterpolateMovement(const Unit *u, int *x, int *y)
{
	if (enhancement_smooth_unit_animation == SMOOTH_UNIT_ANIMATION_DISABLE) {
		return Map_IsPositionInViewport(u->o.position, x, y);
	}
	else {
		const double frame = Timer_GetUnitMovementFrame();
		tile32 pos = Unit_GetNextDestination(u);

		pos.x = u->lastPosition.x + ((int)pos.x - u->lastPosition.x) * frame;
		pos.y = u->lastPosition.y + ((int)pos.y - u->lastPosition.y) * frame;

		return Map_IsPositionInViewport(pos, x, y);
	}
}
