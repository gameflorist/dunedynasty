/** @file src/gui/viewport.c Viewport routines. */

#include <assert.h>
#include <stdio.h>

#include "gui.h"
#include "widget.h"
#include "../common_a5.h"
#include "../enhancement.h"
#include "../explosion.h"
#include "../map.h"
#include "../newui/viewport.h"
#include "../opendune.h"
#include "../pool/pool.h"
#include "../pool/pool_unit.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../structure.h"
#include "../table/widgetinfo.h"
#include "../tools/coord.h"
#include "../unit.h"
#include "../video/video.h"

#if 0
extern bool GUI_Widget_Viewport_Click(Widget *w);
static uint8 *GUI_Widget_Viewport_Draw_GetSprite(uint16 spriteID, uint8 houseID);
#endif

void GUI_Widget_Viewport_Draw(void)
{
	const Screen oldScreenID = GFX_Screen_SetActive(SCREEN_1);
	const uint16 oldValue_07AE_0000 = Widget_SetCurrentWidget(2);
	PoolFindStruct find;

	Viewport_DrawTiles();

	for (const Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_SANDWORM);
			u != NULL;
			u = Unit_FindNext(&find)) {
		Viewport_DrawSandworm(u);
	}

	/* Draw selected unit under units. */
	if ((g_selectionType != SELECTIONTYPE_PLACE) && !Unit_AnySelected() && (Structure_Get_ByPackedTile(g_selectionRectanglePosition) != NULL)) {
		const int x1 = TILE_SIZE * (Tile_GetPackedX(g_selectionRectanglePosition) - Tile_GetPackedX(g_viewportPosition)) - g_viewport_scrollOffsetX;
		const int y1 = TILE_SIZE * (Tile_GetPackedY(g_selectionRectanglePosition) - Tile_GetPackedY(g_viewportPosition)) - g_viewport_scrollOffsetY;
		const int x2 = x1 + (TILE_SIZE * g_selectionWidth) - 1;
		const int y2 = y1 + (TILE_SIZE * g_selectionHeight) - 1;

		Prim_Rect_i(x1, y1, x2, y2, 0xFF);
	}

	for (const Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			u != NULL;
			u = Unit_FindNext(&find)) {

		if (u->o.index < 20 || u->o.index > UnitPool_GetMaxIndex() -1 )
			continue;

		if (enhancement_invisible_saboteurs && u->o.type == UNIT_SABOTEUR)
			continue;

		Viewport_DrawUnit(u, 0, 0, false);
	}

	Explosion_Draw();
	Viewport_DrawTileFog();

	Viewport_DrawRallyPoint();
	Viewport_DrawSelectionHealthBars();
	Viewport_DrawSelectionBox();
	Viewport_DrawPanCursor();

	/* Draw placement box over fog. */
	if (g_selectionType == SELECTIONTYPE_PLACE) {
		const int x1 = TILE_SIZE * (Tile_GetPackedX(g_selectionRectanglePosition) - Tile_GetPackedX(g_viewportPosition)) - g_viewport_scrollOffsetX;
		const int y1 = TILE_SIZE * (Tile_GetPackedY(g_selectionRectanglePosition) - Tile_GetPackedY(g_viewportPosition)) - g_viewport_scrollOffsetY;

		if (g_selectionState == 0 && g_selectionType == SELECTIONTYPE_PLACE) {
			VideoA5_DrawRectCross(x1, y1, g_selectionWidth, g_selectionHeight, 0xFF);
		} else {
			const int x2 = x1 + (TILE_SIZE * g_selectionWidth) - 1;
			const int y2 = y1 + (TILE_SIZE * g_selectionHeight) - 1;

			Prim_Rect_i(x1, y1, x2, y2, 0xFF);
		}
	}

	for (const Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			u != NULL;
			u = Unit_FindNext(&find)) {
		if (u->o.index <= 15)
			Viewport_DrawAirUnit(u);
	}

	if ((g_viewportMessageCounter & 1) != 0 && g_viewportMessageText != NULL) {
		const enum ScreenDivID old_div = A5_SaveTransform();
		A5_UseTransform(SCREENDIV_MENU);

		const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
		const int xcentre = (wi->width * g_screenDiv[SCREENDIV_VIEWPORT].scalex) / (2 * g_screenDiv[SCREENDIV_MENU].scalex);
		const int ymessage = SCREEN_HEIGHT - 61;

		GUI_DrawText_Wrapper(g_viewportMessageText, xcentre, ymessage, 15, 0, 0x132);

		A5_UseTransform(old_div);
	}

	GFX_Screen_SetActive(oldScreenID);

	Widget_SetCurrentWidget(oldValue_07AE_0000);
}

#if 0
static void GUI_Widget_Viewport_DrawTile(int x, int y);
#endif

void GUI_Widget_Viewport_RedrawMap(void)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP];

	Video_DrawMinimap(wi->offsetX, wi->offsetY, g_scenario.mapScale, MINIMAP_IN_GAME);

	Map_UpdateMinimapPosition(g_viewportPosition, true);
}
