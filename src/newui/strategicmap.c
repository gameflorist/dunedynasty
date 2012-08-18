/* strategicmap.c */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strategicmap.h"

#include "../ini.h"
#include "../config.h"
#include "../opendune.h"
#include "../sprites.h"
#include "../string.h"
#include "../video/video.h"

static struct {
	int x, y;
} region_data[1 + STRATEGIC_MAP_MAX_REGIONS];

StrategicMapData g_strategic_map;

void
StrategicMap_Init(void)
{
	g_playerHouseID = HOUSE_HARKONNEN;
	Sprites_CPS_LoadRegionClick();

	for (int region = 1; region <= STRATEGIC_MAP_MAX_REGIONS; region++) {
		char key[4];
		char buf[128];

		snprintf(key, sizeof(key), "%d", region);

		if (Ini_GetString("PIECES", key, NULL, buf, sizeof(buf), g_fileRegionINI) == NULL)
			exit(1);

		if (sscanf(buf, "%d,%d", &region_data[region].x, &region_data[region].y) != 2)
			exit(1);

		region_data[region].x += 8;
		region_data[region].y += 24;
	}
}

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

void
StrategicMap_DrawRegions(const StrategicMapData *map)
{
	for (int region = 1; region <= STRATEGIC_MAP_MAX_REGIONS; region++) {
		const enum HouseType houseID = map->owner[region];

		if (houseID == HOUSE_INVALID)
			continue;

		assert(houseID < HOUSE_MAX);
		Shape_DrawRemap(SHAPE_MAP_PIECE + region, houseID, region_data[region].x, region_data[region].y, 0, 0);
	}
}

void
StrategicMap_ReadOwnership(int campaignID, StrategicMapData *map)
{
	for (int region = 0; region <= STRATEGIC_MAP_MAX_REGIONS; region++) {
		map->owner[region] = HOUSE_INVALID;
	}

	for (int i = 0; i < campaignID; i++) {
		char category[16];
		snprintf(category, sizeof(category), "GROUP%d", i);

		for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
			char key[4];
			strncpy(key, g_table_houseInfo[houseID].name, 3);
			key[3] = '\0';

			char buf[128];
			if (Ini_GetString(category, key, NULL, buf, sizeof(buf), g_fileRegionINI) == NULL)
				continue;

			char *s = buf;
			while (*s != '\0') {
				const int region = atoi(s);

				if (region != 0)
					map->owner[region] = houseID;

				while (*s != '\0') {
					if (*s++ == ',')
						break;
				}
			}
		}
	}
}

void
StrategicMap_ReadProgression(enum HouseType houseID, int campaignID, StrategicMapData *map)
{
	int count = 0;

	char category[16];
	snprintf(category, sizeof(category), "GROUP%d", campaignID);

	for (int i = 0; i < HOUSE_MAX; i++) {
		const enum HouseType h = (houseID + i) % HOUSE_MAX;

		char key[16];
		strncpy(key, g_table_houseInfo[h].name, 3);
		key[3] = '\0';

		char buf[128];
		if (Ini_GetString(category, key, NULL, buf, sizeof(buf), g_fileRegionINI) == NULL)
			continue;

		char *s = buf;
		while (*s != '\0') {
			const int region = atoi(s);

			if (region != 0) {
				snprintf(key, sizeof(key), "%sTXT%d", g_languageSuffixes[g_config.language], region);
				Ini_GetString(category, key, NULL, map->progression[count].text, sizeof(map->progression[count].text), g_fileRegionINI);
			}

			while (*s != '\0') {
				if (*s++ == ',')
					break;
			}

			map->progression[count].houseID = h;
			map->progression[count].region = region;
			count++;
		}
	}

	for (; count < STRATEGIC_MAP_MAX_PROGRESSION; count++) {
		map->progression[count].houseID = HOUSE_INVALID;
		map->progression[count].region = -1;
	}
}
