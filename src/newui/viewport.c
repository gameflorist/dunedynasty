/* viewport.c */

#include <assert.h>
#include <stdlib.h>
#include "../os/math.h"

#include "viewport.h"

#include "../audio/driver.h"
#include "../audio/sound.h"
#include "../config.h"
#include "../enhancement.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../house.h"
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

/* Selection box is in screen coordinates. */
static bool selection_box_active = false;
static int selection_box_x1;
static int selection_box_x2;
static int selection_box_y1;
static int selection_box_y2;

static int
Map_Clamp(int x)
{
	if (x <= 0)
		return 0;

	if (x >= MAP_SIZE_MAX - 1)
		return MAP_SIZE_MAX - 1;

	return x;
}

static int
Viewport_ClampSelectionBoxX(int x)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];

	if (x <= wi->offsetX)
		return wi->offsetX;

	if (x >= wi->offsetX + wi->width - 1)
		return wi->offsetX + wi->width - 1;

	return x;
}

static int
Viewport_ClampSelectionBoxY(int y)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];

	if (y <= wi->offsetY)
		return wi->offsetY;

	if (y >= wi->offsetY + wi->height - 1)
		return wi->offsetY + wi->height - 1;

	return y;
}

static void
Viewport_SelectRegion(Widget *w)
{
	const int radius = 5;
	const int dx = selection_box_x2 - selection_box_x1;
	const int dy = selection_box_y2 - selection_box_y1;
	const int x0 = Tile_GetPackedX(g_viewportPosition);
	const int y0 = Tile_GetPackedY(g_viewportPosition);

	/* Select individual unit or structure. */
	if (dx*dx + dy*dy < radius*radius) {
		const int tilex = x0 + (selection_box_x2 - w->offsetX) / TILE_SIZE;
		const int tiley = y0 + (selection_box_y2 - w->offsetY) / TILE_SIZE;

		if (!(0 <= tilex && tilex < MAP_SIZE_MAX) &&
		     (0 <= tiley && tiley < MAP_SIZE_MAX))
			return;

		const uint16 packed = Tile_PackXY(tilex, tiley);

		Map_SetSelection(packed);
	}

	/* Box selection. */
	else {
		const int x1 = Map_Clamp(x0 + (selection_box_x1 - w->offsetX) / TILE_SIZE);
		const int x2 = Map_Clamp(x0 + (selection_box_x2 - w->offsetX) / TILE_SIZE);
		const int y1 = Map_Clamp(y0 + (selection_box_y1 - w->offsetY) / TILE_SIZE);
		const int y2 = Map_Clamp(y0 + (selection_box_y2 - w->offsetY) / TILE_SIZE);

		PoolFindStruct find;
		bool added_units = false;

		find.houseID = g_playerHouseID;
		find.type = 0xFFFF;
		find.index = 0xFFFF;

		/* Try to find own units. */
		Unit *u = Unit_Find(&find);
		while (u != NULL) {
			const int ux = Tile_GetPosX(u->o.position);
			const int uy = Tile_GetPosY(u->o.position);

			if ((x1 <= ux && ux <= x2) && (y1 <= uy && uy <= y2)) {
				if (!Unit_IsSelected(u)) {
					Unit_Select(u);
					added_units = true;
				}
			}

			u = Unit_Find(&find);
		}

		if (added_units)
			return;

		find.index = 0xFFFF;

		/* Try to find own structure. */
		Structure *s = Structure_Find(&find);
		while (s != NULL) {
			const StructureInfo *si = &g_table_structureInfo[s->o.type];
			const int sx = Tile_GetPosX(s->o.position);
			const int sy = Tile_GetPosY(s->o.position);
			const int sw = g_table_structure_layoutSize[si->layout].width;
			const int sh = g_table_structure_layoutSize[si->layout].height;

			if ((x1 <= sx + sw && sx <= x2) && (y1 <= sy + sh && sy <= y2)) {
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

	if (g_enableVoices == 0) {
		Driver_Sound_Play(36, 0xFF);
	}
	else if (g_table_unitInfo[u->o.type].movementType == MOVEMENT_FOOT) {
		Sound_StartSound(g_table_actionInfo[action].soundID);
	}
	else {
		Sound_StartSound(((Tools_Random_256() & 0x1) == 0) ? 20 : 17);
	}
}

void
Viewport_Place(void)
{
	const StructureInfo *si = &g_table_structureInfo[g_structureActiveType];

	Structure *s = g_structureActive;
	House *h = g_playerHouse;

	if (Structure_Place(s, g_selectionPosition)) {
		Voice_Play(20);

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
			if ((h->structuresBuilt & (1 << STRUCTURE_OUTPOST)) != 0) {
				GUI_DisplayText(String_Get_ByIndex(STR_NOT_ENOUGH_POWER_FOR_RADAR_BUILD_WINDTRAPS), 3);
			}
		}

		return;
	}

	Voice_Play(47);

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
	const int x0 = Tile_GetPackedX(g_viewportPosition);
	const int y0 = Tile_GetPackedY(g_viewportPosition);
	const int tilex = Map_Clamp(x0 + (g_mouseX - w->offsetX) / TILE_SIZE);
	const int tiley = Map_Clamp(y0 + (g_mouseY - w->offsetY) / TILE_SIZE);
	const uint16 packed = Tile_PackXY(tilex, tiley);

	/* 0x01, 0x02, 0x04, 0x08: lmb clicked, held, released, not held. */
	if ((w->state.s.buttonState & 0x01) != 0) {
		if (g_selectionType == SELECTIONTYPE_TARGET) {
			GUI_DisplayText(NULL, -1);

			if (g_unitHouseMissile != NULL) {
				Unit_LaunchHouseMissile(packed);
				return true;
			}

			for (Unit *u = Unit_FirstSelected(); u; u = Unit_NextSelected(u)) {
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
			selection_box_x1 = g_mouseX;
			selection_box_y1 = g_mouseY;

			Unit_UnselectAll();
		}

		return true;
	}
	else if ((w->state.s.buttonState & 0x02) != 0) {
		/* RMB cancels selection box. */
		if ((w->state.s.buttonState & 0x80) == 0)
			selection_box_active = false;

		return true;
	}
	else if ((w->state.s.buttonState & 0x04) != 0) {
		if (!selection_box_active)
			return true;

		selection_box_active = false;
		selection_box_x2 = Viewport_ClampSelectionBoxX(g_mouseX);
		selection_box_y2 = Viewport_ClampSelectionBoxY(g_mouseY);

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

		Viewport_SelectRegion(w);
		return true;
	}
	else {
		selection_box_active = false;
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
Viewport_DrawTiles(void)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
	const int viewportX1 = wi->offsetX;
	const int viewportY1 = wi->offsetY;
	const int viewportX2 = wi->offsetX + wi->width - 1;
	const int viewportY2 = wi->offsetY + wi->height - 1;
	const int x0 = Tile_GetPackedX(g_viewportPosition);
	const int y0 = Tile_GetPackedY(g_viewportPosition);

	int top = viewportY1;
	for (int y = y0; (y < MAP_SIZE_MAX) && (top <= viewportY2); y++, top += TILE_SIZE) {
		int left = viewportX1;

		for (int x = x0; (x < MAP_SIZE_MAX) && (left <= viewportX2); x++, left += TILE_SIZE) {
			const int curPos = Tile_PackXY(x, y);
			const Tile *t = &g_map[curPos];

			if ((t->overlaySpriteID == g_veiledSpriteID) && !g_debugScenario)
				continue;

			Video_DrawIcon(t->groundSpriteID, t->houseID, left, top);

			if ((t->overlaySpriteID == 0) || g_debugScenario)
				continue;

			Video_DrawIcon(t->overlaySpriteID, t->houseID, left, top);
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
Viewport_DrawSelectedUnit(int x, int y)
{
	if (enhancement_new_selection_cursor) {
		const int x1 = x - TILE_SIZE/2 + 1 + g_widgetProperties[WINDOWID_VIEWPORT].xBase*8;
		const int y1 = y - TILE_SIZE/2 + 1 + g_widgetProperties[WINDOWID_VIEWPORT].yBase;
		const int x2 = x1 + TILE_SIZE - 3;
		const int y2 = y1 + TILE_SIZE - 3;

#if 0
		/* 3 pixels long. */
		GUI_DrawLine(x1, y1 + 2, x1 + 2 + 1, y1 - 1, 0xFF);
		GUI_DrawLine(x2 - 2, y1, x2 + 1, y1 + 2 + 1, 0xFF);
		GUI_DrawLine(x2 - 2, y2, x2 + 1, y2 - 2 - 1, 0xFF);
		GUI_DrawLine(x1, y2 - 2, x1 + 2 + 1, y2 + 1, 0xFF);
#else
		/* 4 pixels long. */
		GUI_DrawLine(x1, y1 + 3, x1 + 3 + 1, y1 - 1, 0xFF);
		GUI_DrawLine(x2 - 3, y1, x2 + 1, y1 + 3 + 1, 0xFF);
		GUI_DrawLine(x2 - 3, y2, x2 + 1, y2 - 3 - 1, 0xFF);
		GUI_DrawLine(x1, y2 - 3, x1 + 3 + 1, y2 + 1, 0xFF);
#endif
	}
	else {
		Shape_DrawTint(SHAPE_SELECTED_UNIT, x, y, 0xFF, WINDOWID_VIEWPORT, 0xC000);
	}
}

void
Viewport_DrawSelectionBox(void)
{
	if (!selection_box_active)
		return;

	const int x2 = Viewport_ClampSelectionBoxX(g_mouseX);
	const int y2 = Viewport_ClampSelectionBoxY(g_mouseY);

	GUI_DrawWiredRectangle(selection_box_x1, selection_box_y1, x2, y2, 0xFF);
}

void
Viewport_InterpolateMovement(const Unit *u, uint16 *x, uint16 *y)
{
	const int frame = clamp(0, (3 + g_timerGame - g_tickUnitUnknown1), 2);

	tile32 origin;
	origin.s.x = *x;
	origin.s.y = *y;

	float speed = u->speedRemainder;
	speed += Tools_AdjustToGameSpeed(u->speedPerTick, 1, 255, false) * frame / 3.0f;

	uint16 destx;
	uint16 desty;
	Map_IsPositionInViewport(u->currentDestination, &destx, &desty);

	const int dx = abs(destx - *x);
	const int dy = abs(desty - *y);

	int dist = max(dx, dy) + min(dx, dy) / 2;
	dist = min(u->speed * speed / 256.0f, dist);

	const tile32 pos = Tile_MoveByDirection(origin, u->orientation[0].current, dist);

	*x = pos.s.x;
	*y = pos.s.y;
}
