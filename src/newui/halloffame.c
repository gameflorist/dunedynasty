/* halloffame.c */

#include <assert.h>

#include "halloffame.h"

#include "../gfx.h"
#include "../video/video.h"

static void
HallOfFame_DrawEmblem(enum HouseType houseL, enum HouseType houseR)
{
	const struct {
		int x, y;
	} emblem[3] = {
		{ 8, 136 }, { 8 + 56 * 1, 136 }, { 8 + 56 * 2, 136 }
	};
	assert(houseL <= HOUSE_ORDOS);
	assert(houseR <= HOUSE_ORDOS);

	Video_DrawCPSRegion("FAME.CPS", emblem[houseL].x, emblem[houseL].y, 0, 8, 7*8, 56);
	Video_DrawCPSRegion("FAME.CPS", emblem[houseR].x, emblem[houseR].y, SCREEN_WIDTH - 7*8, 8, 7*8, 56);
}

void
HallOfFame_DrawBackground(enum HouseType houseID)
{
	Video_DrawCPS("FAME.CPS");

	if (houseID <= HOUSE_ORDOS) {
		HallOfFame_DrawEmblem(houseID, houseID);
	}
	else {
		/* XXX: would be nice to use the highest score's house. */
		HallOfFame_DrawEmblem(HOUSE_HARKONNEN, HOUSE_ATREIDES);
	}

	GUI_DrawFilledRectangle(8, 136, 175, 191, 116);
}
