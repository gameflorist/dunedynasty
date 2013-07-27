/** @file src/gui/viewport.c Viewport routines. */

#include <stdio.h>
#include <string.h>
#include "enum_string.h"
#include "types.h"
#include "../os/common.h"
#include "../os/math.h"

#include "gui.h"
#include "widget.h"
#include "../common_a5.h"
#include "../config.h"
#include "../enhancement.h"
#include "../explosion.h"
#include "../gfx.h"
#include "../house.h"
#include "../input/mouse.h"
#include "../map.h"
#include "../newui/viewport.h"
#include "../opendune.h"
#include "../pool/house.h"
#include "../pool/pool.h"
#include "../pool/unit.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../string.h"
#include "../structure.h"
#include "../table/widgetinfo.h"
#include "../timer/timer.h"
#include "../tools/coord.h"
#include "../unit.h"
#include "../video/video.h"

#if 0
static uint32 s_tickCursor;                                 /*!< Stores last time Viewport changed the cursor spriteID. */
static uint32 s_tickMapScroll;                              /*!< Stores last time Viewport ran MapScroll function. */
static uint32 s_tickClick;                                  /*!< Stores last time Viewport handled a click. */

/**
 * Handles the Click events for the Viewport widget.
 *
 * @param w The widget.
 */
bool GUI_Widget_Viewport_Click(Widget *w)
{
	int x, y;
	uint16 packed;
	bool click, drag;

	if ((w->state.buttonState & 0x44) != 0) {
		/* This variable prevents a target order from initiating minimap scrolling. */
		g_var_37B8 = true;
		return false;
	}

	enum ShapeID spriteID = g_cursorSpriteID;
	switch (w->index) {
		default: break;
		case 39: spriteID = SHAPE_CURSOR_UP; break;
		case 40: spriteID = SHAPE_CURSOR_RIGHT; break;
		case 41: spriteID = SHAPE_CURSOR_LEFT; break;
		case 42: spriteID = SHAPE_CURSOR_DOWN; break;
		case 43: case 44: case 45:
			if (g_selectionType == SELECTIONTYPE_TARGET) {
				spriteID = SHAPE_CURSOR_TARGET;
			}
			else {
				spriteID = SHAPE_CURSOR_NORMAL;
			}
			break;
	}

	if (spriteID != g_cursorSpriteID) {
		s_tickCursor = g_timerGame;
		Video_SetCursor(spriteID);
	}

	if (w->index == 45) return false;

	click = false;
	drag = false;

	if ((w->state.buttonState & 0x11) != 0) {
		click = true;
		g_var_37B8 = false;
	} else if ((w->state.buttonState & 0x22) != 0 && !g_var_37B8) {
		drag = true;
	}

	/* ENHANCEMENT -- Dune2 depends on slow CPUs to limit the rate mouse clicks are handled. */
	if (g_dune2_enhanced && (click || drag)) {
		if (s_tickClick + 2 >= g_timerGame) return true;
		s_tickClick = g_timerGame;
	}

	int scroll_dx = 0, scroll_dy = 0;
	switch (w->index) {
		default: break;
		case 39: scroll_dy = -1; break;
		case 40: scroll_dx =  1; break;
		case 41: scroll_dx = -1; break;
		case 42: scroll_dy =  1; break;
	}

	if (scroll_dx != 0 || scroll_dy != 0) {
		/* Always scroll if we have a click or a drag */
		if (!click && !drag) {
			/* Wait for either one of the timers */
			if (s_tickMapScroll + 10 >= g_timerGame || s_tickCursor + 20 >= g_timerGame) return true;
			/* Don't scroll if we have a structure/unit selected and don't want to autoscroll */
			if (g_gameConfig.autoScroll == 0 && (g_selectionType == SELECTIONTYPE_STRUCTURE || g_selectionType == SELECTIONTYPE_UNIT)) return true;
		}

		return true;
	}

	if (click) {
		x = ((float)g_mouseClickX - g_screenDiv[w->div].x) / g_screenDiv[w->div].scale;
		y = ((float)g_mouseClickY - g_screenDiv[w->div].y) / g_screenDiv[w->div].scale;
	} else {
		x = ((float)g_mouseX - g_screenDiv[w->div].x) / g_screenDiv[w->div].scale;
		y = ((float)g_mouseY - g_screenDiv[w->div].y) / g_screenDiv[w->div].scale;
	}

	if (w->index == 43) {
		x =  x / 16 + Tile_GetPackedX(g_minimapPosition);
		y = (y - 40) / 16 + Tile_GetPackedY(g_minimapPosition);
	}

	if (w->index == 44) {
		const int x0 = w->offsetX;
		const int y0 = w->offsetY;

		uint16 mapScale;
		const MapInfo *mapInfo;

		mapScale = g_scenario.mapScale;
		mapInfo = &g_mapInfos[mapScale];

		x = min((max(x, x0) - x0) / (mapScale + 1), mapInfo->sizeX - 1) + mapInfo->minX;
		y = min((max(y, y0) - y0) / (mapScale + 1), mapInfo->sizeY - 1) + mapInfo->minY;
	}

	packed = Tile_PackXY(x, y);

	if (click && g_selectionType == SELECTIONTYPE_TARGET) {
		GUI_DisplayText(NULL, -1);

		if (g_unitHouseMissile != NULL) {
			Unit_LaunchHouseMissile(packed);
			return true;
		}

		int iter;
		for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
			Viewport_Target(u, g_activeAction, packed);
		}

		g_unitActive   = NULL;
		g_activeAction = 0xFFFF;

		GUI_ChangeSelectionType(SELECTIONTYPE_UNIT);
		return true;
	}

	if (click && g_selectionType == SELECTIONTYPE_PLACE) {
		Viewport_Place();
		return true;
	}

	if (click && w->index == 43) {
		uint16 position;

		if (g_debugScenario) {
			position = packed;
		} else {
			position = Unit_FindTargetAround(packed);
		}

		if (g_map[position].overlaySpriteID != g_veiledSpriteID || g_debugScenario) {
			if (Object_GetByPackedTile(position) != NULL || g_debugScenario) {
				Map_SetSelection(position);
				/* Unit_DisplayStatusText(g_unitSelected); */
			}
		}

		if ((w->state.buttonState & 0x10) != 0) Map_SetViewportPosition(packed);

		return true;
	}

	if ((click || drag) && w->index == 44) {
		/* High-resolution panning. */
		const ScreenDiv *div = &g_screenDiv[SCREENDIV_SIDEBAR];
		const uint16 mapScale = g_scenario.mapScale;
		const MapInfo *mapInfo = &g_mapInfos[mapScale];

		/* Minimap is (div->scale * 64) * (div->scale * 64).
		 * Each pixel represents 1 / (mapScale + 1) tiles.
		 */
		x = g_mouseX - div->x - div->scale * g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].offsetX;
		y = g_mouseY - div->y - div->scale * g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].offsetY;
		x = TILE_SIZE * mapInfo->minX + TILE_SIZE * x / (div->scale * (mapScale + 1));
		y = TILE_SIZE * mapInfo->minY + TILE_SIZE * y / (div->scale * (mapScale + 1));

		Map_CentreViewport(x, y);
		return true;
	}

	if (g_selectionType == SELECTIONTYPE_TARGET) {
		Map_SetSelection(Unit_FindTargetAround(packed));
	} else if (g_selectionType == SELECTIONTYPE_PLACE) {
		Map_SetSelection(packed);
	}

	return true;
}
#endif

#if 0
static uint8 *GUI_Widget_Viewport_Draw_GetSprite(uint16 spriteID, uint8 houseID);
#endif

/**
 * Redraw parts of the viewport that require redrawing.
 *
 * @param forceRedraw If true, dirty flags are ignored, and everything is drawn.
 * @param arg08 ??
 * @param drawToMainScreen True if and only if we are drawing to the main screen and not some buffer screen.
 */
void GUI_Widget_Viewport_Draw(bool forceRedraw, bool arg08, bool drawToMainScreen)
{
	uint16 x;
	uint16 y;
	uint16 i;
	bool updateDisplay;
	Screen oldScreenID;
	uint16 oldValue_07AE_0000;
	int16 minX[10];
	int16 maxX[10];

	PoolFindStruct find;

	updateDisplay = forceRedraw;

	memset(minX, 0xF, sizeof(minX));
	memset(maxX, 0,   sizeof(minX));

	oldScreenID = GFX_Screen_SetActive(SCREEN_1);

	oldValue_07AE_0000 = Widget_SetCurrentWidget(2);

#if 0
	if (g_dirtyViewportCount != 0 || forceRedraw) {
		for (y = 0; y < 10; y++) {
			uint16 top = (y << 4) + 0x28;
			for (x = 0; x < (drawToMainScreen ? 15 : 16); x++) {
				Tile *t;
				uint16 left;

				curPos = g_viewportPosition + Tile_PackXY(x, y);

				if (x < 15 && !forceRedraw && BitArray_Test(g_dirtyViewport, curPos)) {
					if (maxX[y] < x) maxX[y] = x;
					if (minX[y] > x) minX[y] = x;
					updateDisplay = true;
				}

				if (!BitArray_Test(g_dirtyMinimap, curPos) && !forceRedraw) continue;

				BitArray_Set(g_dirtyViewport, curPos);

				if (x < 15) {
					updateDisplay = true;
					if (maxX[y] < x) maxX[y] = x;
					if (minX[y] > x) minX[y] = x;
				}

				t = &g_map[curPos];
				left = x << 4;

				if (!g_debugScenario && g_veiledSpriteID == t->overlaySpriteID) {
					GUI_DrawFilledRectangle(left, top, left + 15, top + 15, 12);
					continue;
				}

				GFX_DrawSprite(t->groundSpriteID, left, top, t->houseID);

				if (t->overlaySpriteID == 0 || g_debugScenario) continue;

				GFX_DrawSprite(t->overlaySpriteID, left, top, t->houseID);
			}
		}
		g_dirtyViewportCount = 0;
	}
#else
	Viewport_DrawTiles();
#endif

	{
		find.type    = UNIT_SANDWORM;
		find.index   = 0xFFFF;
		find.houseID = HOUSE_INVALID;

		Unit *u = Unit_Find(&find);
		while (u != NULL) {
			Viewport_DrawSandworm(u);
			u = Unit_Find(&find);
		}
	}

	/* Draw selected unit under units. */
	if ((g_selectionType != SELECTIONTYPE_PLACE) && !Unit_AnySelected() && (Structure_Get_ByPackedTile(g_selectionRectanglePosition) != NULL)) {
		const int x1 = TILE_SIZE * (Tile_GetPackedX(g_selectionRectanglePosition) - Tile_GetPackedX(g_viewportPosition)) - g_viewport_scrollOffsetX;
		const int y1 = TILE_SIZE * (Tile_GetPackedY(g_selectionRectanglePosition) - Tile_GetPackedY(g_viewportPosition)) - g_viewport_scrollOffsetY;
		const int x2 = x1 + (TILE_SIZE * g_selectionWidth) - 1;
		const int y2 = y1 + (TILE_SIZE * g_selectionHeight) - 1;

		Prim_Rect_i(x1, y1, x2, y2, 0xFF);
	}

	if (true || forceRedraw || updateDisplay) {
		find.type    = 0xFFFF;
		find.index   = 0xFFFF;
		find.houseID = HOUSE_INVALID;

		Unit *u = Unit_Find(&find);
		while (u != NULL) {
			if ((19 <= u->o.index && u->o.index <= 21 && !enhancement_invisible_saboteurs) ||
			    (22 <= u->o.index && u->o.index <= 101))
				Viewport_DrawUnit(u, 0, 0, false);

			u = Unit_Find(&find);
		}
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
		}
		else {
			const int x2 = x1 + (TILE_SIZE * g_selectionWidth) - 1;
			const int y2 = y1 + (TILE_SIZE * g_selectionHeight) - 1;

			Prim_Rect_i(x1, y1, x2, y2, 0xFF);
		}
	}

	if (true || forceRedraw || updateDisplay) {
		find.type    = 0xFFFF;
		find.index   = 0xFFFF;
		find.houseID = HOUSE_INVALID;

		Unit *u = Unit_Find(&find);
		while (u != NULL) {
			if (u->o.index <= 15)
				Viewport_DrawAirUnit(u);

			u = Unit_Find(&find);
		}
	}

#if 0
	if (g_changedTilesCount != 0) {
		bool init = false;
		bool update = false;
		Screen oldScreenID2 = SCREEN_1;

		for (i = 0; i < g_changedTilesCount; i++) {
			curPos = g_changedTiles[i];
			BitArray_Clear(g_changedTilesMap, curPos);

			if (!init) {
				init = true;

				oldScreenID2 = GFX_Screen_SetActive(SCREEN_1);

				GUI_Mouse_Hide_InWidget(3);
			}

			GUI_Widget_Viewport_DrawTile(curPos);

			if (!update && BitArray_Test(g_displayedMinimap, curPos)) update = true;
		}

		if (update) Map_UpdateMinimapPosition(g_minimapPosition, true);

		if (init) {
			GUI_Screen_Copy(32, 136, 32, 136, 8, 64, g_screenActiveID, SCREEN_0);

			GFX_Screen_SetActive(oldScreenID2);

			GUI_Mouse_Show_InWidget();
		}

		if (g_changedTilesCount == lengthof(g_changedTiles)) {
			g_changedTilesCount = 0;

			for (i = 0; i < 4096; i++) {
				if (!BitArray_Test(g_changedTilesMap, i)) continue;
				g_changedTiles[g_changedTilesCount++] = i;
				if (g_changedTilesCount == lengthof(g_changedTiles)) break;
			}
		} else {
			g_changedTilesCount = 0;
		}
	}
#endif

	if ((g_viewportMessageCounter & 1) != 0 && g_viewportMessageText != NULL && (minX[6] <= 14 || maxX[6] >= 0 || arg08 || forceRedraw)) {
		const enum ScreenDivID old_div = A5_SaveTransform();
		A5_UseTransform(SCREENDIV_MENU);

		const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
		const int xcentre = (wi->width * g_screenDiv[SCREENDIV_VIEWPORT].scalex) / (2 * g_screenDiv[SCREENDIV_MENU].scalex);
		const int ymessage = SCREEN_HEIGHT - 61;

		GUI_DrawText_Wrapper(g_viewportMessageText, xcentre, ymessage, 15, 0, 0x132);
		minX[6] = -1;
		maxX[6] = 14;

		A5_UseTransform(old_div);
	}

	if (updateDisplay && !drawToMainScreen) {
		if (g_viewport_fadein) {
			GUI_Mouse_Hide_InWidget(g_curWidgetIndex);

			/* ENHANCEMENT -- When fading in the game on start, you don't see the fade as it is against the already drawn screen. */
			if (g_dune2_enhanced) {
				Screen oldScreenID2 = g_screenActiveID;

				GFX_Screen_SetActive(SCREEN_0);
				Prim_FillRect_i(g_curWidgetXBase, g_curWidgetYBase, g_curWidgetXBase + g_curWidgetWidth, g_curWidgetYBase + g_curWidgetHeight, 0);
				GFX_Screen_SetActive(oldScreenID2);
			}

			GUI_Screen_FadeIn(g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetWidth/8, g_curWidgetHeight, g_screenActiveID, SCREEN_0);
			GUI_Mouse_Show_InWidget();

			g_viewport_fadein = false;
		} else {
			bool init = false;

			for (i = 0; i < 10; i++) {
				uint16 width;
				uint16 height;

				if (arg08) {
					minX[i] = 0;
					maxX[i] = 14;
				}

				if (maxX[i] < minX[i]) continue;

				x = minX[i] * 2;
				y = (i << 4) + 0x28;
				width  = (maxX[i] - minX[i] + 1) * 2;
				height = 16;

				if (!init) {
					GUI_Mouse_Hide_InWidget(g_curWidgetIndex);

					init = true;
				}

				GUI_Screen_Copy(x, y, x, y, width, height, g_screenActiveID, SCREEN_0);
			}

			if (init) GUI_Mouse_Show_InWidget();
		}
	}

	GFX_Screen_SetActive(oldScreenID);

	Widget_SetCurrentWidget(oldValue_07AE_0000);
}

#if 0
/**
 * Draw a single tile on the screen.
 *
 * @param packed The tile to draw.
 */
static void
GUI_Widget_Viewport_DrawTile(int x, int y)
{
	const uint16 packed = Tile_PackXY(x,y);
	const int mapScale = g_scenario.mapScale + 1;

	uint8 colour;
	uint16 spriteID;
	Tile *t;

	colour = 12;
	spriteID = 0xFFFF;

	if (mapScale == 0) return;

	t = &g_map[packed];

	if ((t->isUnveiled && g_playerHouse->flags.radarActivated) || g_debugScenario) {
		uint16 type = Map_GetLandscapeType(packed);
		Unit *u;

		if (mapScale > 1) {
			spriteID = g_scenario.mapScale + g_table_landscapeInfo[type].spriteID - 1;
		} else {
			colour = g_table_landscapeInfo[type].radarColour;
		}

		if (g_table_landscapeInfo[type].radarColour == 0xFFFF) {
			if (mapScale > 1) {
				spriteID = mapScale + t->houseID * 2 + 29;
			} else {
				colour = g_table_houseInfo[t->houseID].minimapColor;
			}
		}

		u = Unit_Get_ByPackedTile(packed);

		if (u != NULL) {
			if (mapScale > 1) {
				if (u->o.type == UNIT_SANDWORM) {
					spriteID = mapScale + 53;
				} else {
					spriteID = mapScale + Unit_GetHouseID(u) * 2 + 29;
				}
			} else {
				if (u->o.type == UNIT_SANDWORM) {
					colour = 255;
				} else {
					colour = g_table_houseInfo[Unit_GetHouseID(u)].minimapColor;
				}
			}
		}
	} else {
		Structure *s;

		s = Structure_Get_ByPackedTile(packed);

		if (s != NULL && s->o.houseID == g_playerHouseID) {
			if (mapScale > 1) {
				spriteID = mapScale + s->o.houseID * 2 + 29;
			} else {
				colour = g_table_houseInfo[s->o.houseID].minimapColor;
			}
		} else {
			if (mapScale > 1) {
				spriteID = g_scenario.mapScale + g_table_landscapeInfo[LST_ENTIRELY_MOUNTAIN].spriteID - 1;
			} else {
				colour = 12;
			}
		}
	}

	x -= g_mapInfos[g_scenario.mapScale].minX;
	y -= g_mapInfos[g_scenario.mapScale].minY;

	if (spriteID != 0xFFFF) {
		x *= g_scenario.mapScale + 1;
		y *= g_scenario.mapScale + 1;
		Shape_Draw(spriteID, x, y, 3, 0x4000);
	} else {
		const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP];

		x += wi->offsetX;
		y += wi->offsetY;
		Prim_FillRect_i(x, y, x, y, colour);
	}
}
#endif

void GUI_Widget_Viewport_RedrawMap(void)
{
#if 0
	const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];

	for (int y = 0; y < mapInfo->sizeY; y++) {
		for (int x = 0; x < mapInfo->sizeX; x++)
			GUI_Widget_Viewport_DrawTile(mapInfo->minX + x, mapInfo->minY + y);
	}
#else
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP];

	Video_DrawMinimap(wi->offsetX, wi->offsetY, g_scenario.mapScale, 0);
#endif

	Map_UpdateMinimapPosition(g_viewportPosition, true);
}
