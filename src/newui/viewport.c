/* viewport.c */

#include <assert.h>

#include "viewport.h"

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
}
