/* menubar.c */

#include <assert.h>

#include "menubar.h"

#include "../gfx.h"
#include "../video/video.h"

void
MenuBar_Draw(enum HouseType houseID)
{
	for (int y = TRUE_DISPLAY_HEIGHT - 83 - 52; y + 52 - 1 >= 40 + 17; y -= 52) {
		Video_DrawCPSSpecial(CPS_SIDEBAR_MIDDLE, houseID, TRUE_DISPLAY_WIDTH - 80, y);
	}

	for (int x = TRUE_DISPLAY_WIDTH - 136 - 320; x + 320 - 1 >= 184; x -= 320) {
		Video_DrawCPSSpecial(CPS_MENUBAR_MIDDLE, houseID, x, 0);
	}

	for (int x = TRUE_DISPLAY_WIDTH - 8 - 425; x + 425 - 1 >= 8; x -= 425) {
		Video_DrawCPSSpecial(CPS_STATUSBAR_MIDDLE, houseID, x, 17);
	}

	Video_DrawCPSSpecial(CPS_MENUBAR_LEFT, houseID, 0, 0);
	Video_DrawCPSSpecial(CPS_MENUBAR_RIGHT, houseID, TRUE_DISPLAY_WIDTH - 136, 0);
	Video_DrawCPSSpecial(CPS_STATUSBAR_LEFT, houseID, 0, 17);
	Video_DrawCPSSpecial(CPS_STATUSBAR_RIGHT, houseID, TRUE_DISPLAY_WIDTH - 8, 17);
	Video_DrawCPSSpecial(CPS_SIDEBAR_TOP, houseID, TRUE_DISPLAY_WIDTH - 80, 40);
	Video_DrawCPSSpecial(CPS_SIDEBAR_BOTTOM, houseID, TRUE_DISPLAY_WIDTH - 80, TRUE_DISPLAY_HEIGHT - 83);
	GUI_DrawFilledRectangle(TRUE_DISPLAY_WIDTH - 64, TRUE_DISPLAY_HEIGHT - 64, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT, 0);
}
