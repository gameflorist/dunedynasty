/* viewport.c */

#include <assert.h>

#include "viewport.h"

#include "../enhancement.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../map.h"
#include "../opendune.h"
#include "../sprites.h"
#include "../table/widgetinfo.h"
#include "../tile.h"
#include "../video/video.h"

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
