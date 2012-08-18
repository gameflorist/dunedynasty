/* strategicmap.c */

#include <assert.h>

#include "strategicmap.h"

#include "../config.h"
#include "../opendune.h"
#include "../video/video.h"

static void
StrategicMap_DrawEmblem(enum HouseType houseID)
{
	const struct {
		int x, y;
	} emblem[3] = {
		{ 0*8, 152 }, { 33*8, 152 }, { 1*8, 24 }
	};
	assert(houseID <= HOUSE_ORDOS);

	Video_DrawCPSRegion("MAPMACH.CPS", emblem[houseID].x, emblem[houseID].y, emblem[HOUSE_HARKONNEN].x, emblem[HOUSE_HARKONNEN].y, 7*8, 40);
	Video_DrawCPSRegion("MAPMACH.CPS", emblem[houseID].x, emblem[houseID].y, emblem[HOUSE_ATREIDES].x, emblem[HOUSE_ATREIDES].y, 7*8, 40);
}

void
StrategicMap_DrawBackground(enum HouseType houseID)
{
	const enum CPSID conquest =
		(g_config.language == LANGUAGE_FRENCH) ? CPS_CONQUEST_FR :
		(g_config.language == LANGUAGE_GERMAN) ? CPS_CONQUEST_DE :
		CPS_CONQUEST_EN;

	Video_DrawCPS("MAPMACH.CPS");
	StrategicMap_DrawEmblem(houseID);
	Video_DrawCPSSpecial(conquest, houseID, 8, 0);
}
