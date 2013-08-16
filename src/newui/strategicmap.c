/* strategicmap.c */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enum_string.h"
#include "../os/math.h"

#include "strategicmap.h"

#include "../audio/audio.h"
#include "../config.h"
#include "../enhancement.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../ini.h"
#include "../input/mouse.h"
#include "../opendune.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../string.h"
#include "../table/locale.h"
#include "../table/sound.h"
#include "../timer/timer.h"
#include "../video/video.h"

static struct {
	int x, y;
} region_data[1 + STRATEGIC_MAP_MAX_REGIONS];

StrategicMapData g_strategic_map_state;
uint32 g_strategicRegionBits;   /* bits designating regions attempted. */

uint16
StrategicMap_CampaignChoiceToScenarioID(int campaignID, int nth)
{
	if (campaignID == 0)
		return 1;

	assert(0 <= nth && nth <= 2);

	uint16 scenarioID = 3 * (campaignID - 1) + 2 + nth;
	if (campaignID > 7) scenarioID--;
	if (campaignID > 8) scenarioID--;

	return scenarioID;
}

static void
StrategicMap_SetRegionAttempted(int region)
{
	g_strategicRegionBits |= (1 << region);
}

static bool
StrategicMap_IsRegionAttempted(int region)
{
	return (g_strategicRegionBits & (1 << region));
}

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
StrategicMap_DrawPlanet(const StrategicMapData *map)
{
	const char *cps[3] = { "PLANET.CPS", "PLANET.CPS", "DUNERGN.CPS" };
	const int idx = map->state - STRATEGIC_MAP_SHOW_PLANET;
	assert(0 <= idx && idx < 3);

	Video_DrawCPSRegion(SEARCHDIR_GLOBAL_DATA_DIR, cps[idx], 8, 24, 8, 24, 304, 120);

	if (map->region_aux != NULL)
		Video_DrawFadeIn(map->region_aux);
}

static void
StrategicMap_DrawEmblem(unsigned char emblemID)
{
	const struct {
		int x, y;
	} emblem[3] = {
		{ 0*8, 152 }, { 33*8, 152 }, { 1*8, 24 }
	};

	int x, y;

	if (emblemID < 3) {
		x = emblem[emblemID].x;
		y = emblem[emblemID].y;
	}
	else {
		x = emblem[2].x + 56 * (emblemID - 2);
		y = emblem[2].y;
	}

	Video_DrawCPSRegion(SEARCHDIR_CAMPAIGN_DIR, "MAPMACH.CPS", x, y, emblem[0].x, emblem[0].y, 7*8, 40);
	Video_DrawCPSRegion(SEARCHDIR_CAMPAIGN_DIR, "MAPMACH.CPS", x, y, emblem[1].x, emblem[1].y, 7*8, 40);
}

static void
StrategicMap_DrawBackground(enum HouseType houseID)
{
	const enum CPSID conquest
		= (g_gameConfig.language == LANGUAGE_FRENCH) ? CPS_CONQUEST_FR
		: (g_gameConfig.language == LANGUAGE_GERMAN) ? CPS_CONQUEST_DE
		: (g_gameConfig.language == LANGUAGE_ITALIAN) ? CPS_CONQUEST_FR
		: (g_gameConfig.language == LANGUAGE_SPANISH) ? CPS_CONQUEST_DE
		: CPS_CONQUEST_EN;

	Video_DrawCPS(SEARCHDIR_CAMPAIGN_DIR, "MAPMACH.CPS");
	StrategicMap_DrawEmblem(g_campaign_list[g_campaign_selected].mapmach_cps[houseID]);
	Video_DrawCPSSpecial(conquest, houseID, 8, 0);
}

static void
StrategicMap_DrawText(const StrategicMapData *map)
{
	const ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];
	const int64_t dt = Timer_GetTicks() - map->text_timer;
	const int offset = min(dt/3.0 + (175 - 185), (175 - 172 - 1));

	Video_SetClippingArea(8*8 * div->scalex + div->x, 165 * div->scaley + div->y, 24*8 * div->scalex, 14 * div->scaley);

	if (map->text1 != NULL)
		GUI_DrawText_Wrapper(map->text1, 64, 165 + offset, 12, 0, 0x12);

	if (map->text2 != NULL)
		GUI_DrawText_Wrapper(map->text2, 64, 177 + offset, 12, 0, 0x12);

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}

static void
StrategicMap_DrawRegion(enum HouseType houseID, int region)
{
	Shape_DrawRemap(SHAPE_MAP_PIECE + region, houseID, region_data[region].x, region_data[region].y, 0, 0);
}

static void
StrategicMap_DrawRegions(const StrategicMapData *map)
{
	for (int region = 1; region <= STRATEGIC_MAP_MAX_REGIONS; region++) {
		const enum HouseType houseID = map->owner[region];

		if (houseID == HOUSE_INVALID)
			continue;

		StrategicMap_DrawRegion(houseID, region);
	}
}

static void
StrategicMap_DrawRegionFadeIn(const StrategicMapData *map)
{
	if (map->region_aux != NULL)
		Video_DrawFadeIn(map->region_aux);
}

static void
StrategicMap_DrawArrow(enum HouseType houseID, int scenario, const StrategicMapData *map)
{
	const int64_t curr_ticks = Timer_GetTicks();
	const int frame = map->arrow_frame + (curr_ticks - map->arrow_timer) / STRATEGIC_MAP_ARROW_ANIMATION_DELAY;

	/* Blink the arrows leading to scenarios we haven't completed in the past. */
	if ((map->state == STRATEGIC_MAP_SELECT_REGION) && (frame & 0x31) == 0x21) {
		const Campaign *camp = &g_campaign_list[g_campaign_selected];

		for (unsigned int h = 0; h < 3; h++) {
			if (camp->house[h] == houseID) {
				const uint16 scenarioID = StrategicMap_CampaignChoiceToScenarioID(g_campaignID, scenario);

				if (!(camp->completion[h] & (1 << (scenarioID - 1))))
					return;

				break;
			}
		}
	}

	const enum ShapeID shapeID = map->arrow[scenario].shapeID;
	const enum ShapeID tintID = SHAPE_ARROW_TINT + 5 * (shapeID - SHAPE_ARROW);
	const int x = map->arrow[scenario].x;
	const int y = map->arrow[scenario].y;
	const uint8 c = 144 + houseID * 16;

	Shape_Draw(shapeID, x, y, 0, 0);
	Shape_DrawTint(tintID + 0, x, y, STRATEGIC_MAP_ARROW_EDGE_COLOUR + houseID * 16, 0, 0);
	Shape_DrawTint(tintID + 1, x, y, c + ((frame + 0) & 0x3), 0, 0);
	Shape_DrawTint(tintID + 2, x, y, c + ((frame + 1) & 0x3), 0, 0);
	Shape_DrawTint(tintID + 3, x, y, c + ((frame + 2) & 0x3), 0, 0);
	Shape_DrawTint(tintID + 4, x, y, c + ((frame + 3) & 0x3), 0, 0);
}

static void
StrategicMap_DrawArrows(enum HouseType houseID, int blink_scenario, const StrategicMapData *map)
{
	for (int i = 0; i < STRATEGIC_MAP_MAX_ARROWS; i++) {
		if (map->arrow[i].index <= 0)
			continue;

		/* Draw only arrows pointing to the selected region. */
		if ((blink_scenario >= 0) && (map->arrow[i].index != map->arrow[blink_scenario].index))
			continue;

		StrategicMap_DrawArrow(houseID, i, map);
	}
}

static void
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

			const char *s = buf;
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

static void
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
				snprintf(key, sizeof(key), "%sTXT%d",
						g_table_languageInfo[g_gameConfig.language].suffix,
						region);
				Ini_GetString(category, key, NULL, map->progression[count].text, sizeof(map->progression[count].text), g_fileRegionINI);

				/* Attempt non-language-specific TXT. */
				if (map->progression[count].text[0] == '\0') {
					snprintf(key, sizeof(key), "TXT%d", region);
					Ini_GetString(category, key, NULL, map->progression[count].text, sizeof(map->progression[count].text), g_fileRegionINI);
				}

				if (enhancement_fix_typos && (map->progression[count].text[0] != '\0')) {
					char *str = map->progression[count].text;

					while (*str == ' ' || *str == '\t')
						str++;

					if (str != map->progression[count].text)
						memmove(map->progression[count].text, str, strlen(str) + 1);
				}
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

static void
StrategicMap_ReadArrows(int campaignID, StrategicMapData *map)
{
	char category[16];
	int count = 0;
	bool any_unattempted_regions = false;

	snprintf(category, sizeof(category), "GROUP%d", campaignID);

	for (int i = 0; i < 5; i++) {
		char key[8];
		char buf[128];
		int index, shapeID, x, y;

		snprintf(key, sizeof(key), "REG%d", i + 1);

		if (Ini_GetString(category, key, NULL, buf, sizeof(buf), g_fileRegionINI) == NULL)
			break;

		if (sscanf(buf, "%d,%d,%d,%d", &index, &shapeID, &x, &y) != 4)
			continue;

		assert(count < STRATEGIC_MAP_MAX_ARROWS);

		if ((index != 0) && !StrategicMap_IsRegionAttempted(index))
			any_unattempted_regions = true;

		map->arrow[count].index = index;
		map->arrow[count].shapeID = SHAPE_ARROW + shapeID;
		map->arrow[count].x = x;
		map->arrow[count].y = y;
		count++;
	}

	if (any_unattempted_regions) {
		for (int i = 0; i < count; i++) {
			if (StrategicMap_IsRegionAttempted(map->arrow[i].index))
				map->arrow[i].index = -1;
		}
	}
	else {
		g_strategicRegionBits = 0;
	}

	for (; count < STRATEGIC_MAP_MAX_ARROWS; count++) {
		map->arrow[count].index = -1;
		map->arrow[count].shapeID = SHAPE_INVALID;
	}
}

void
StrategicMap_AdvanceText(StrategicMapData *map, bool force)
{
	const char *str = NULL;

	switch (map->state) {
		case STRATEGIC_MAP_SHOW_PLANET:
			str = String_Get_ByIndex(STR_THREE_HOUSES_HAVE_COME_TO_DUNE);
			break;

		case STRATEGIC_MAP_SHOW_SURFACE:
			str = String_Get_ByIndex(STR_TO_TAKE_CONTROL_OF_THE_LAND);
			break;

		case STRATEGIC_MAP_SHOW_DIVISION:
			str = String_Get_ByIndex(STR_THAT_HAS_BECOME_DIVIDED);
			break;

		case STRATEGIC_MAP_SHOW_TEXT:
			str = map->progression[map->curr_progression].text;
			break;

		case STRATEGIC_MAP_SELECT_REGION:
			str = String_Get_ByIndex(STR_SELECT_YOUR_NEXT_REGION);
			break;

		case STRATEGIC_MAP_SHOW_PROGRESSION:
		case STRATEGIC_MAP_BLINK_REGION:
		case STRATEGIC_MAP_BLINK_END:
			break;
	}

	if (force || (str != NULL && str[0] != '\0')) {
		map->text2 = map->text1;
		map->text1 = str;
		map->text_timer = Timer_GetTicks();
	}
}

static bool
StrategicMap_GetNextProgression(StrategicMapData *map)
{
	for (int i = map->curr_progression + 1; i < STRATEGIC_MAP_MAX_PROGRESSION; i++) {
		if (map->progression[i].houseID != HOUSE_INVALID) {
			map->curr_progression = i;
			return true;
		}
	}

	map->curr_progression = 0;
	return false;
}

/*--------------------------------------------------------------*/

void
StrategicMap_Initialise(enum HouseType houseID, int campaignID, StrategicMapData *map)
{
	g_playerHouseID = houseID;
	Sprites_CPS_LoadRegionClick();

	if (g_gameMode == GM_LOSE) {
		StrategicMap_ReadOwnership(campaignID + 1, map);
		StrategicMap_ReadArrows(campaignID, map);
		map->state = STRATEGIC_MAP_SELECT_REGION;
	}
	else {
		StrategicMap_ReadOwnership(campaignID, map);
		StrategicMap_ReadProgression(houseID, campaignID, map);
		StrategicMap_ReadArrows(campaignID, map);
		map->state = (campaignID == 1) ? STRATEGIC_MAP_SHOW_PLANET : STRATEGIC_MAP_SHOW_TEXT;
	}

	map->fast_forward = false;
	map->curr_progression = 0;
	map->region_aux = NULL;

	map->text1 = NULL;
	map->text2 = NULL;
	StrategicMap_AdvanceText(map, true);

	Audio_PlayMusic(MUSIC_STOP);
}

void
StrategicMap_Draw(enum HouseType houseID, StrategicMapData *map, int64_t fade_start)
{
	StrategicMap_DrawBackground(houseID);

	if (map->state < STRATEGIC_MAP_SHOW_TEXT) {
		StrategicMap_DrawPlanet(map);
		StrategicMap_DrawText(map);
		return;
	}

	Video_DrawCPSRegion(SEARCHDIR_GLOBAL_DATA_DIR, "DUNERGN.CPS", 8, 24, 8, 24, 304, 120);
	StrategicMap_DrawRegions(map);
	StrategicMap_DrawText(map);

	if (map->state == STRATEGIC_MAP_SHOW_PROGRESSION) {
		StrategicMap_DrawRegionFadeIn(map);
	}
	else if (map->state == STRATEGIC_MAP_SELECT_REGION) {
		StrategicMap_DrawArrows(houseID, -1, map);
	}
	else if (map->state == STRATEGIC_MAP_BLINK_REGION) {
		const int64_t curr_ticks = Timer_GetTicks();

		if ((((curr_ticks - fade_start) / 20) & 0x1) == 0)
			StrategicMap_DrawRegion(houseID, map->arrow[map->blink_scenario].index);

		StrategicMap_DrawArrows(houseID, map->blink_scenario, map);
	}
	else if (map->state == STRATEGIC_MAP_BLINK_END) {
		StrategicMap_DrawArrows(houseID, map->blink_scenario, map);
	}
}

bool
StrategicMap_TimerLoop(StrategicMapData *map)
{
	const int64_t curr_ticks = Timer_GetTicks();
	bool redraw = true;

	Audio_PlayMusicIfSilent(MUSIC_STRATEGIC_MAP);

	switch (map->state) {
		case STRATEGIC_MAP_SHOW_PLANET:
			if (map->fast_forward || (curr_ticks - map->text_timer >= 120)) {
				map->region_aux = Video_InitFadeInCPS("DUNEMAP.CPS", 8, 24, 304, 120, true);
				map->state++;
				StrategicMap_AdvanceText(map, false);
			}
			break;

		case STRATEGIC_MAP_SHOW_SURFACE:
		case STRATEGIC_MAP_SHOW_DIVISION:
			Video_TickFadeIn(map->region_aux);
			if (map->fast_forward || (curr_ticks - map->text_timer >= 120 + 60)) {
				if (map->state == STRATEGIC_MAP_SHOW_SURFACE) {
					map->region_aux = Video_InitFadeInCPS("DUNEMAP.CPS", 8, 24, 304, 120, false);
				}
				else {
					map->region_aux = NULL;
				}

				map->state++;
				StrategicMap_AdvanceText(map, false);
			}
			break;

		case STRATEGIC_MAP_SHOW_TEXT:
			if (map->fast_forward || (curr_ticks - map->text_timer >= 12 * 3)) {
				const enum ShapeID shapeID = SHAPE_MAP_PIECE + map->progression[map->curr_progression].region;
				const enum HouseType houseID = map->progression[map->curr_progression].houseID;
				const int region = map->progression[map->curr_progression].region;

				map->state = STRATEGIC_MAP_SHOW_PROGRESSION;
				map->region_aux = Video_InitFadeInShape(shapeID, houseID, region_data[region].x, region_data[region].y);
			}
			break;

		case STRATEGIC_MAP_SHOW_PROGRESSION:
			if (map->fast_forward || Video_TickFadeIn(map->region_aux)) {
				const int curr = map->curr_progression;

				map->owner[map->progression[curr].region] = map->progression[curr].houseID;

				if (StrategicMap_GetNextProgression(map)) {
					map->state = STRATEGIC_MAP_SHOW_TEXT;
					map->region_aux = NULL;
				}
				else {
					map->state = STRATEGIC_MAP_SELECT_REGION;
					map->fast_forward = false;
					map->region_aux = NULL;
					map->arrow_frame = 0;
					map->arrow_timer = curr_ticks;

					if (enhancement_play_additional_voices)
						Audio_PlayVoice(VOICE_SELECT_YOUR_NEXT_CONQUEST);
				}

				StrategicMap_AdvanceText(map, false);
			}
			break;

		case STRATEGIC_MAP_SELECT_REGION:
			Audio_Poll();
			if (curr_ticks - map->text_timer >= 3 * 12)
				redraw = false;
			break;

		case STRATEGIC_MAP_BLINK_REGION:
		case STRATEGIC_MAP_BLINK_END:
			break;
	}

	if (curr_ticks - map->arrow_timer >= STRATEGIC_MAP_ARROW_ANIMATION_DELAY) {
		const int64_t dt = curr_ticks - map->arrow_timer;
		map->arrow_timer = curr_ticks + dt % STRATEGIC_MAP_ARROW_ANIMATION_DELAY;
		map->arrow_frame += dt / STRATEGIC_MAP_ARROW_ANIMATION_DELAY;
		redraw = true;
	}

	return redraw;
}

int
StrategicMap_SelectRegion(const StrategicMapData *map, int x, int y)
{
	x = x - 8;
	y = y - 24;

	if (!((0 <= x && x < 304) && (0 <= y && y < 120)))
		return -1;

	const int region = g_fileRgnclkCPS[304*y + x];

	for (int i = 0; i < STRATEGIC_MAP_MAX_ARROWS; i++) {
		if (map->arrow[i].index == region) {
			StrategicMap_SetRegionAttempted(region);
			return i;
		}
	}

	return -1;
}

bool
StrategicMap_BlinkLoop(StrategicMapData *map, int64_t blink_start)
{
	const int64_t ticks = Timer_GetTicks() - blink_start;

	if (ticks >= 4 * 20) {
		map->state = STRATEGIC_MAP_BLINK_END;
		return true;
	}

	return false;
}
