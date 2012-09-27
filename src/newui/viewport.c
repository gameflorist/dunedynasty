/* viewport.c */

#include <assert.h>
#include <stdlib.h>
#include "../os/math.h"

#include "viewport.h"

#include "menubar.h"
#include "../audio/audio.h"
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
#include "../sprites.h"
#include "../string.h"
#include "../structure.h"
#include "../table/strings.h"
#include "../table/widgetinfo.h"
#include "../tile.h"
#include "../timer/timer.h"
#include "../tools.h"
#include "../unit.h"
#include "../video/video.h"

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
static bool selection_box_active = false;
static int selection_box_x1;
static int selection_box_x2;
static int selection_box_y1;
static int selection_box_y2;

static void Viewport_InterpolateMovement(const Unit *u, int *x, int *y);

/*--------------------------------------------------------------*/

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
	const int dx = selection_box_x2 - selection_box_x1;
	const int dy = selection_box_y2 - selection_box_y1;
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

		if (g_map[packed].overlaySpriteID == g_veiledSpriteID)
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
		const int x1 = selection_box_x1;
		const int x2 = selection_box_x2;
		const int y1 = selection_box_y1;
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

			if ((oi->flags.tabSelectable) && (x1 <= ux && ux <= x2) && (y1 <= uy && uy <= y2)) {
				if (!Unit_IsSelected(u))
					Unit_Select(u);
			}

			u = Unit_Find(&find);
		}

		if (Unit_AnySelected()) {
			Unit_DisplayGroupStatusText();
			return;
		}

		find.index = 0xFFFF;

		/* Try to find own structure. */
		Structure *s = Structure_Find(&find);
		while (s != NULL) {
			const StructureInfo *si = &g_table_structureInfo[s->o.type];
			const int sx = Tile_GetPosX(s->o.position);
			const int sy = Tile_GetPosY(s->o.position);
			const int xx = TILE_SIZE * (Tile_GetPosX(s->o.position) - x0);
			const int yy = TILE_SIZE * (Tile_GetPosY(s->o.position) - y0);
			const int sw = TILE_SIZE * g_table_structure_layoutSize[si->layout].width;
			const int sh = TILE_SIZE * g_table_structure_layoutSize[si->layout].height;

			if ((x1 <= xx + sw && xx <= x2) && (y1 <= yy + sh && yy <= y2)) {
				const uint16 packed = Tile_PackXY(sx, sy);
				Map_SetSelection(packed);
				return;
			}

			s = Structure_Find(&find);
		}
	}
}

void
Viewport_Target(Unit *u, enum ActionType action, uint16 packed)
{
	uint16 encoded;

	if (Unit_GetHouseID(u) != g_playerHouseID)
		return;

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
	}
	else if (action == ACTION_HARVEST) {
		u->targetMove = encoded;
	}
	else {
		Unit *target;

		Unit_SetTarget(u, encoded);
		target = Tools_Index_GetUnit(u->targetAttack);
		if (target != NULL)
			target->blinkCounter = 8;
	}

	if (!g_enable_sounds) {
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

void
Viewport_Place(void)
{
	const StructureInfo *si = &g_table_structureInfo[g_structureActiveType];

	Structure *s = g_structureActive;
	House *h = g_playerHouse;

	/* Since there concrete slabs and walls are shared, we need to set the creator again. */
	g_structureActive->creatorHouseID = g_playerHouseID;
	g_structureActive->o.houseID = g_playerHouseID;

	if (Structure_Place(s, g_selectionPosition)) {
		Audio_PlaySound(SOUND_PLACEMENT);

		if (s->o.type == STRUCTURE_PALACE)
			House_Get_ByIndex(s->o.houseID)->palacePosition = s->o.position;

		if (g_structureActiveType == STRUCTURE_REFINERY && g_var_38BC == 0) {
			Unit *u;

			g_var_38BC++;
			u = Unit_CreateWrapper(g_playerHouseID, UNIT_HARVESTER, Tools_Index_Encode(s->o.index, IT_STRUCTURE));
			g_var_38BC--;

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
			if ((Structure_GetBuildable(s) & (1 << s->objectType)) == 0)
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
		GUI_DisplayHint(STR_STRUCTURES_MUST_BE_PLACED_ON_CLEAR_ROCK_OR_CONCRETE_AND_ADJACENT_TO_ANOTHER_FRIENDLY_STRUCTURE, 0xFFFF);
		GUI_DisplayText(String_Get_ByIndex(STR_CAN_NOT_PLACE_S_HERE), 2, String_Get_ByIndex(si->o.stringID_abbrev));
	}
}

bool
Viewport_Click(Widget *w)
{
	int mouseX, mouseY;
	Mouse_TransformToDiv(SCREENDIV_VIEWPORT, &mouseX, &mouseY);

	const int x0 = Tile_GetPackedX(g_viewportPosition);
	const int y0 = Tile_GetPackedY(g_viewportPosition);
	const int tilex = Map_Clamp(x0 + (mouseX + g_viewport_scrollOffsetX) / TILE_SIZE);
	const int tiley = Map_Clamp(y0 + (mouseY + g_viewport_scrollOffsetY) / TILE_SIZE);
	const uint16 packed = Tile_PackXY(tilex, tiley);

	if ((w->state.s.buttonState & 0x04) != 0) {
		if (!selection_box_active)
			return true;

		selection_box_active = false;
		selection_box_x2 = Viewport_ClampSelectionBoxX(mouseX);
		selection_box_y2 = Viewport_ClampSelectionBoxY(mouseY);

		if (selection_box_x1 > selection_box_x2) {
			const int swap = selection_box_x1;
			selection_box_x1 = selection_box_x2;
			selection_box_x2 = swap;
		}

		if (selection_box_y1 > selection_box_y2) {
			const int swap = selection_box_y1;
			selection_box_y1 = selection_box_y2;
			selection_box_y2 = swap;
		}

		MenuBar_ShowMentatAndOptions();
		Viewport_SelectRegion();
		return true;
	}

	const enum ShapeID cursorID = (g_selectionType == SELECTIONTYPE_TARGET) ? SHAPE_CURSOR_TARGET : SHAPE_CURSOR_NORMAL;
	if (cursorID != g_cursorSpriteID)
		Video_SetCursor(cursorID);

	if (w->index == 45)
		return true;

	/* 0x01, 0x02, 0x04, 0x08: lmb clicked, held, released, not held. */
	if ((w->state.s.buttonState & 0x01) != 0) {
		if (g_selectionType == SELECTIONTYPE_TARGET) {
			GUI_DisplayText(NULL, -1);

			if (g_unitHouseMissile != NULL) {
				Unit_LaunchHouseMissile(packed);
				return true;
			}

			int iter;
			for (Unit *u = Unit_FirstSelected(&iter); u; u = Unit_NextSelected(&iter)) {
				Viewport_Target(u, g_activeAction, packed);
			}

			g_activeAction = 0xFFFF;
			GUI_ChangeSelectionType(SELECTIONTYPE_UNIT);
		}
		else if (g_selectionType == SELECTIONTYPE_PLACE) {
			Viewport_Place();
		}
		else {
			selection_box_active = true;
			selection_box_x1 = mouseX;
			selection_box_y1 = mouseY;

			if ((Input_Test(SCANCODE_LSHIFT) == false) && (Input_Test(SCANCODE_RSHIFT) == false)) {
				Map_SetSelection(0xFFFF);
				Unit_UnselectAll();
			}

			/* Disable the mentat and options buttons when dragging. */
			MenuBar_HideMentatAndOptions();
		}

		return true;
	}
	else if ((w->state.s.buttonState & 0x02) != 0) {
		/* RMB cancels selection box. */
		if ((w->state.s.buttonState & 0x80) == 0) {
			selection_box_active = false;
			MenuBar_ShowMentatAndOptions();
		}

		return true;
	}
	else if ((w->state.s.buttonState & 0x04) != 0) {
	}
	else {
		selection_box_active = false;
		MenuBar_ShowMentatAndOptions();
	}

	/* 0x10, 0x20, 0x40, 0x80: rmb clicked, held, released, not held. */
	if (((w->state.s.buttonState & 0x10) != 0) && (g_selectionType == SELECTIONTYPE_UNIT || g_selectionType == SELECTIONTYPE_STRUCTURE)) {
		const Unit *target_u = Unit_Get_ByPackedTile(packed);
		const Structure *target_s = Structure_Get_ByPackedTile(packed);
		const enum LandscapeType lst = Map_GetLandscapeType(packed);

		bool attack = false;

		if (target_u != NULL) {
			if (!House_AreAllied(g_playerHouseID, Unit_GetHouseID(target_u)))
				attack = true;
		}
		else if (target_s != NULL) {
			if (!House_AreAllied(g_playerHouseID, target_s->o.houseID))
				attack = true;
		}

		int iter;
		for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
			const ObjectInfo *oi = &g_table_unitInfo[u->o.type].o;
			enum ActionType action = ACTION_INVALID;

			if (Unit_GetHouseID(u) != g_playerHouseID)
				continue;

			for (int i = 0; (i < 4) && (action == ACTION_INVALID); i++) {
				if ((oi->actionsPlayer[i] == ACTION_ATTACK) && attack) {
					action = ACTION_ATTACK;
				}
				else if (oi->actionsPlayer[i] == ACTION_MOVE) {
					action = ACTION_MOVE;
				}

				/* Harvesters return to work if ordered to crush a
				 * soldier on sand.  Use move command instead if
				 * ordered back to rock, e.g. to evade a worm attack.
				 */
				else if ((oi->actionsPlayer[i] == ACTION_HARVEST) && (g_table_landscapeInfo[lst].isSand)) {
					action = (u->amount < 100) ? ACTION_HARVEST : ACTION_MOVE;
				}
			}

			if (action != ACTION_INVALID) {
				Viewport_Target(u, action, packed);
			}
		}

		Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);
		if (s != NULL) {
			Structure_SetRallyPoint(s, packed);
		}
	}
	else if ((w->state.s.buttonState & 0x40) != 0) {
		if (g_selectionType == SELECTIONTYPE_TARGET || g_selectionType == SELECTIONTYPE_PLACE) {
			GUI_Widget_Cancel_Click(NULL);
		}
	}

	if (g_selectionType == SELECTIONTYPE_TARGET) {
		Map_SetSelection(Unit_FindTargetAround(packed));
	}
	else if (g_selectionType == SELECTIONTYPE_PLACE) {
		Map_SetSelection(packed);
	}

	return false;
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

				cx += (u->o.position.s.x >> 4);
				cy += (u->o.position.s.y >> 4);
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

		Structure *s = Structure_Find(&find);
		while (s != NULL) {
			if (s == st) {
				s->squadID = squad;
			}
			else if (s->squadID == squad) {
				s->squadID = SQUADID_INVALID;
			}

			s = Structure_Find(&find);
		}
	}
	else {
		const bool key_shift = (Input_Test(SCANCODE_LSHIFT) || Input_Test(SCANCODE_RSHIFT));

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
				cx += (u->o.position.s.x >> 4);
				cy += (u->o.position.s.y >> 4);
				count++;
			}

			u = Unit_Find(&find);
		}

		if (!Unit_AnySelected()) {
			find.houseID = g_playerHouseID;
			find.type = 0xFFFF;
			find.index = 0xFFFF;

			Structure *s = Structure_Find(&find);
			while (s != NULL) {
				if (s->squadID == squad) {
					const StructureInfo *si = &g_table_structureInfo[s->o.type];
					const XYSize *layout = &g_table_structure_layoutSize[si->layout];

					if (Tile_PackTile(s->o.position) != g_selectionPosition)
						modified_selection = true;

					Map_SetSelection(Tile_PackTile(s->o.position));
					cx = (s->o.position.s.x >> 4) + TILE_SIZE * layout->width / 2;
					cy = (s->o.position.s.y >> 4) + TILE_SIZE * layout->height / 2;
					count = 1;
					break;
				}

				s = Structure_Find(&find);
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
			Map_CentreViewport((s->o.position.s.x >> 4) + TILE_SIZE, (s->o.position.s.y >> 4) + TILE_SIZE);
		}
	}
}

static void
Viewport_DrawTilesInRange(int x0, int y0,
		int viewportX1, int viewportY1, int viewportX2, int viewportY2,
		bool draw_tile, bool draw_fog)
{
	int top = viewportY1;
	for (int y = y0; (y < MAP_SIZE_MAX) && (top <= viewportY2); y++, top += TILE_SIZE) {
		int left = viewportX1;

		for (int x = x0; (x < MAP_SIZE_MAX) && (left <= viewportX2); x++, left += TILE_SIZE) {
			const int curPos = Tile_PackXY(x, y);
			const Tile *t = &g_map[curPos];
			const bool overlay_is_fog = (g_veiledSpriteID - 16 <= t->overlaySpriteID && t->overlaySpriteID <= g_veiledSpriteID);

			if (draw_tile && (t->overlaySpriteID != g_veiledSpriteID)) {
				Video_DrawIcon(t->groundSpriteID, t->houseID, left, top);

				if ((t->overlaySpriteID != 0) && !overlay_is_fog)
					Video_DrawIcon(t->overlaySpriteID, t->houseID, left, top);
			}

			if (draw_fog && overlay_is_fog) {
				uint16 iconID = t->overlaySpriteID;

				if (t->overlaySpriteID == g_veiledSpriteID)
					iconID = g_veiledSpriteID - 1;

				Video_DrawIcon(iconID, t->houseID, left, top);
			}
		}
	}

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
	const int viewportX2 = wi->width - 1;
	const int viewportY2 = wi->height - 1;
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
	const int viewportX2 = wi->width - 1;
	const int viewportY2 = wi->height - 1;
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

	if ((s->o.type == STRUCTURE_LIGHT_VEHICLE) ||
	    (s->o.type == STRUCTURE_HEAVY_VEHICLE) ||
	    (s->o.type == STRUCTURE_WOR_TROOPER) ||
	    (s->o.type == STRUCTURE_BARRACKS) ||
	    (s->o.type == STRUCTURE_STARPORT) ||
	    (s->o.type == STRUCTURE_REPAIR)) {
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
	const int w = max(1, width * curr / max);

	/* From ActionPanel_DrawHealthBar. */
	uint8 colour = 4;
	if (curr <= max / 2) colour = 5;
	if (curr <= max / 4) colour = 8;

	Prim_Rect(x - 0.33f, y - 0.33f, x + width + 1.33f, y + 1.33f, 12, 0.0f);

	if (w < width)
		Prim_Hline(x + w + 1, y, x + width, 12);

	Prim_Hline(x, y, x + w, colour);
}

void
Viewport_DrawSelectionHealthBars(void)
{
	if (!enhancement_draw_health_bars)
		return;

	int iter;

	Unit *u = Unit_FirstSelected(&iter);

	const bool unit_selected = (u != NULL);

	while (u != NULL) {
		const uint16 packed = Tile_PackTile(u->o.position);

		if (Map_IsValidPosition(packed) && Map_IsPositionUnveiled(packed)) {
			const UnitInfo *ui = &g_table_unitInfo[u->o.type];
			int x, y;

			Map_IsPositionInViewport(u->o.position, &x, &y);
			Viewport_DrawHealthBar(x - 7, y - TILE_SIZE / 2 - 3, 13, u->o.hitpoints, ui->o.hitpoints);
		}

		u = Unit_NextSelected(&iter);
	}

	if (unit_selected || (g_selectionType == SELECTIONTYPE_PLACE))
		return;

	Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);
	if (s != NULL) {
		if (Map_IsPositionUnveiled(g_selectionPosition)) {
			const StructureInfo *si = &g_table_structureInfo[s->o.type];
			const int x = TILE_SIZE * (Tile_GetPackedX(g_selectionPosition) - Tile_GetPackedX(g_viewportPosition)) - g_viewport_scrollOffsetX;
			const int y = TILE_SIZE * (Tile_GetPackedY(g_selectionPosition) - Tile_GetPackedY(g_viewportPosition)) - g_viewport_scrollOffsetY;

			Viewport_DrawHealthBar(x + 1, y - 2, TILE_SIZE * g_selectionWidth - 3, s->o.hitpoints, si->o.hitpoints);
		}
	}
}

static void
Viewport_DrawSelectedUnit(int x, int y)
{
	if (enhancement_new_selection_cursor) {
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
	if (!g_map[Tile_PackTile(u->o.position)].isUnveiled && !g_debugScenario)
		return;

	const enum ShapeID shapeID = g_table_unitInfo[UNIT_SANDWORM].groundSpriteID;
	int x, y;

	if (Map_IsPositionInViewport(u->o.position, &x, &y))
		Shape_Draw(shapeID, x, y, WINDOWID_VIEWPORT, 0xC200);

	if (Map_IsPositionInViewport(u->targetLast, &x, &y))
		Shape_Draw(shapeID, x, y, WINDOWID_VIEWPORT, 0xC200);

	if (Map_IsPositionInViewport(u->targetPreLast, &x, &y))
		Shape_Draw(shapeID, x, y, WINDOWID_VIEWPORT, 0xC200);

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

	uint8 orientation = Orientation_Orientation256ToOrientation8(u->orientation[ui->o.flags.hasTurret ? 1 : 0].current);
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
	if (!g_map[packed].isUnveiled && !g_debugScenario)
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

		x += windowX;
		y += windowY;
	}

	x += (int16)g_table_tilediff[0][u->wobbleIndex].s.x;
	y += (int16)g_table_tilediff[0][u->wobbleIndex].s.y;

	uint16 s_spriteFlags = 0;
	uint16 index;

	const UnitInfo *ui = &g_table_unitInfo[u->o.type];
	uint8 orientation = Orientation_Orientation256ToOrientation8(u->orientation[0].current);

	if (u->spriteOffset >= 0 || ui->destroyedSpriteID == 0) {
		index = ui->groundSpriteID;

		switch (ui->displayMode) {
			case 1:
			case 2:
				if (ui->movementType == MOVEMENT_SLITHER) break;
				index += values_32A4[orientation][0];
				s_spriteFlags = values_32A4[orientation][1];
				break;

			case 3:
				index += values_32C4[orientation][0] * 3;
				index += values_334A[u->spriteOffset & 3];
				s_spriteFlags = values_32C4[orientation][1];
				break;

			case 4:
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
	if (ui->o.flags.blurTile) s_spriteFlags |= 0x200;

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

	if (Unit_IsSelected(u))
		Viewport_DrawSelectedUnit(x, y);

#if 0
	/* Debugging. */
	GUI_DrawText_Wrapper("id:%x", x - TILE_SIZE / 2, y, 15, 0, 0x11, u->o.index);
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

	if (!g_map[curPos].isUnveiled && !g_debugScenario)
		return;

	int x, y;
	if (!Map_IsPositionInViewport(u->o.position, &x, &y))
		return;

	if (enhancement_smooth_unit_animation != SMOOTH_UNIT_ANIMATION_DISABLE)
		Viewport_InterpolateMovement(u, &x, &y);

	const UnitInfo *ui = &g_table_unitInfo[u->o.type];
	const bool smooth_rotation =
		(enhancement_smooth_unit_animation == SMOOTH_UNIT_ANIMATION_ENABLE) &&
		(u->o.type == UNIT_CARRYALL || u->o.type == UNIT_ORNITHOPTER || u->o.type == UNIT_FRIGATE);

	uint16 s_spriteFlags = 0xC000;
	uint16 index = ui->groundSpriteID;
	uint8 orientation = u->orientation[0].current;

	switch (ui->displayMode) {
		case 0:
			if (u->o.flags.s.bulletIsBig) index++;
			break;

		case 1: /* carryall, frigate */
			if (!smooth_rotation) {
				orientation = Orientation_Orientation256ToOrientation8(orientation);

				index += values_32E4[orientation][0];
				s_spriteFlags |= values_32E4[orientation][1];
			}
			break;

		case 2: /* rockets */
			orientation = Orientation_Orientation256ToOrientation16(orientation);

			index += values_3304[orientation][0];
			s_spriteFlags |= values_3304[orientation][1];
			break;

		case 5: /* ornithopter */
			if (!smooth_rotation) {
				orientation = Orientation_Orientation256ToOrientation8(orientation);

				index += (values_32E4[orientation][0] * 3);
				s_spriteFlags |= values_32E4[orientation][1];
			}

			index += values_33AE[u->spriteOffset & 3];
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
		if (smooth_rotation) {
			Shape_DrawRemapRotate(index, Unit_GetHouseID(u),
					x + 1, y + 3, &u->orientation[0], WINDOWID_VIEWPORT, (s_spriteFlags & 0x5FFF) | 0x300);
		}
		else {
			Shape_Draw(index, x + 1, y + 3, WINDOWID_VIEWPORT, (s_spriteFlags & 0xDFFF) | 0x300);
		}
	}

	if (ui->o.flags.blurTile)
		s_spriteFlags |= 0x200;

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
	if (!selection_box_active)
		return;

	int mouseX, mouseY;
	Mouse_TransformToDiv(SCREENDIV_VIEWPORT, &mouseX, &mouseY);

	int x1 = selection_box_x1;
	int y1 = selection_box_y1;
	int x2 = Viewport_ClampSelectionBoxX(mouseX);
	int y2 = Viewport_ClampSelectionBoxY(mouseY);

	/* Make x1 <= x2, y1 <= y2 so that rectangles are not rounded off. */
	if (selection_box_x1 > x2) {
		x1 = x2;
		x2 = selection_box_x1;
	}

	if (selection_box_y1 > y2) {
		y1 = y2;
		y2 = selection_box_y1;
	}

	Prim_Rect_i(x1, y1, x2, y2, 0xFF);
}

static void
Viewport_DrawInterface(enum HouseType houseID, int blurx, int blury)
{
	const ScreenDiv *sidebar = &g_screenDiv[SCREENDIV_SIDEBAR];
	const ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];

	const int x = g_screenDiv[SCREENDIV_VIEWPORT].width - blurx;

	if (x >= 3 * TILE_SIZE)
		return;

	/* Scaling factor between sidebar -> viewport->scale. */
	float scale = sidebar->scale / viewport->scale;
	float screen_blury = viewport->y + viewport->scale * blury;

	for (int y = sidebar->height - 85 - 52; y + 52 - 1 >= 40 + 17; y -= 52) {
		/* Translate y in sidebar's coordinates -> screen -> viewport coordinates. */
		float screen_y = sidebar->y + sidebar->scale * y;
		float desty = (screen_y - screen_blury) / viewport->scale;

		Video_DrawCPSSpecialScale(CPS_SIDEBAR_MIDDLE, houseID, x, desty, scale);
	}

	{
		int y = 0;
		float screen_y = sidebar->y + sidebar->scale * y;
		float desty = (screen_y - screen_blury) / viewport->scale;

		Video_DrawCPSSpecialScale(CPS_SIDEBAR_TOP, houseID, x, desty, scale);
	}

	{
		int y = sidebar->height - 85;
		float screen_y = sidebar->y + sidebar->scale * y;
		float desty = (screen_y - screen_blury) / viewport->scale;

		Video_DrawCPSSpecialScale(CPS_SIDEBAR_BOTTOM, houseID, x, desty, scale);
	}
}

/* Viewport_RenderBrush:
 *
 * A mini rendering routine used for the sandworm and sonic wave
 * effects.  Renders the 3x3 tiles (the largest brush is 24x24)
 * containing the wave with top-left corner (x, y) into the active
 * buffer.
 */
void
Viewport_RenderBrush(int x, int y)
{
	/* Top-left tile. */
	const int tile_left = Tile_GetPackedX(g_viewportPosition);
	const int tile_top  = Tile_GetPackedY(g_viewportPosition);

	/* Make brush coordinates relative to viewport. */
	x -= g_widgetProperties[WINDOWID_VIEWPORT].xBase;
	y -= g_widgetProperties[WINDOWID_VIEWPORT].yBase;

	/* Tile containing top-left corner of brush. */
	const int tile_dx = x / TILE_SIZE;
	const int tile_dy = y / TILE_SIZE;

	/* Where the brush should be sourced. */
	const int sx = x - tile_dx * TILE_SIZE;
	const int sy = y - tile_dy * TILE_SIZE;

	const int viewportX1 = -sx;
	const int viewportY1 = -sy;
	const int viewportX2 = viewportX1 + 3 * TILE_SIZE;
	const int viewportY2 = viewportY1 + 3 * TILE_SIZE;

	Prim_FillRect_i(viewportX1, viewportY1, viewportX2, viewportY2, 0);

	/* Draw tiles. */
	Viewport_DrawTilesInRange(tile_left + tile_dx, tile_top + tile_dy,
			viewportX1, viewportY1, viewportX2, viewportY2, true, false);

	/* Draw ground units (not sandworms, projectiles, etc.). */
	for (int dy = 0; dy < 3; dy++) {
		for (int dx = 0; dx < 3; dx++) {
			const int tilex = tile_left + tile_dx + dx;
			const int tiley = tile_top + tile_dy + dy;

			if (!(Map_InRange(tilex) && Map_InRange(tiley)))
				continue;

			const uint16 packed = Tile_PackXY(tilex, tiley);
			const Tile *t = &g_map[packed];

			if (!t->hasUnit || (t->index == 0))
				continue;

			const int index = t->index - 1;
			if (!(20 <= index && index <= 101))
				continue;

			const Unit *u = Unit_Get_ByIndex(index);
			const UnitInfo *ui = &g_table_unitInfo[u->o.type];

			if (ui->o.flags.blurTile)
				continue;

			Viewport_DrawUnit(u, x, y, true);
		}
	}

	/* Draw fog. */
	Viewport_DrawTilesInRange(tile_left + tile_dx, tile_top + tile_dy,
			viewportX1, viewportY1, viewportX2, viewportY2, false, true);

	/* Render interface. */
	Viewport_DrawInterface(g_playerHouseID, x, y);
}

static void
Viewport_InterpolateMovement(const Unit *u, int *x, int *y)
{
	const int frame = clamp(0, (3 + g_timerGame - g_tickUnitUnknown1), 2);

	tile32 origin;
	origin.s.x = *x;
	origin.s.y = *y;

	float speed = u->speedRemainder;
	speed += Tools_AdjustToGameSpeed(u->speedPerTick, 1, 255, false) * frame / 3.0f;

	int destx, desty;
	Map_IsPositionInViewport(u->currentDestination, &destx, &desty);

	const int dx = abs(destx - *x);
	const int dy = abs(desty - *y);

	int dist = max(dx, dy) + min(dx, dy) / 2;
	dist = min(u->speed * speed / 256.0f, dist);

	const tile32 pos = Tile_MoveByDirection(origin, u->orientation[0].current, dist);

	*x = (int16)pos.s.x;
	*y = (int16)pos.s.y;
}
