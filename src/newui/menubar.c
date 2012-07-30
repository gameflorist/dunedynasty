/* menubar.c */

#include <assert.h>
#include <stdio.h>

#include "menubar.h"

#include "../gfx.h"
#include "../video/video.h"

void
MenuBar_DrawCredits(int credits_new, int credits_old, int offset)
{
	const int digit_w = 10;

	char char_old[7];
	char char_new[7];

	snprintf(char_old, sizeof(char_old), "%6d", credits_old);
	snprintf(char_new, sizeof(char_new), "%6d", credits_new);

	Video_SetClippingArea(TRUE_DISPLAY_WIDTH - digit_w * 6, 4, digit_w * 6, 9);

	for (int i = 0; i < 6; i++) {
		const enum ShapeID shape_old = SHAPE_CREDITS_NUMBER_0 + char_old[i] - '0';
		const enum ShapeID shape_new = SHAPE_CREDITS_NUMBER_0 + char_new[i] - '0';
		const int x = TRUE_DISPLAY_WIDTH - digit_w * (6 - i);

		if (char_old[i] != char_new[i]) {
			if (char_old[i] != ' ')
				Shape_Draw(shape_old, x, offset, 0, 0);

			if (char_new[i] != ' ')
				Shape_Draw(shape_new, x, 8 + offset, 0, 0);
		}
		else {
			if (char_new[i] != ' ')
				Shape_Draw(shape_new, x, 5, 0, 0);
		}
	}

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}

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

	Shape_DrawRemap(SHAPE_CREDITS_LABEL, houseID, TRUE_DISPLAY_WIDTH - 128, 0, 0, 0);
}
