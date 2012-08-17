/* actionpanel.c */

#include <assert.h>
#include "../os/math.h"

#include "actionpanel.h"

#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../string.h"
#include "../table/strings.h"
#include "../video/video.h"

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
