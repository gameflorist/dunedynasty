/* halloffame.c */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../os/math.h"

#include "halloffame.h"

#include "../gfx.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../opendune.h"
#include "../string.h"
#include "../table/strings.h"
#include "../timer/timer.h"
#include "../video/video.h"

#define MAX_RANKS   12

static const struct {
	int stringID;
	int score;
} rank_scores[MAX_RANKS] = {
	{271,   25}, /* "Sand Flea" */
	{272,   50}, /* "Sand Snake" */
	{273,  100}, /* "Desert Mongoose" */
	{274,  150}, /* "Sand Warrior" */
	{275,  200}, /* "Dune Trooper" */
	{276,  300}, /* "Squad Leader" */
	{277,  400}, /* "Outpost Commander" */
	{278,  500}, /* "Base Commander" */
	{279,  700}, /* "Warlord" */
	{280, 1000}, /* "Chief Warlord" */
	{281, 1400}, /* "Ruler of Arrakis" */
	{282, 1800}  /* "Emperor" */
};

HallOfFameData g_hall_of_fame_state;

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
HallOfFame_DrawBackground(enum HouseType houseID, bool hallOfFame)
{
	Video_DrawCPS("FAME.CPS");

	if (houseID <= HOUSE_ORDOS) {
		HallOfFame_DrawEmblem(houseID, houseID);
	}
	else {
		/* XXX: would be nice to use the highest score's house. */
		HallOfFame_DrawEmblem(HOUSE_HARKONNEN, HOUSE_ATREIDES);
	}

	if (hallOfFame) {
		GUI_DrawFilledRectangle(8, 80, 311, 191, 116);
	}
}

/*--------------------------------------------------------------*/

void
HallOfFame_InitRank(int score, HallOfFameData *fame)
{
	int stringID = rank_scores[MAX_RANKS - 1].stringID;

	for (unsigned int i = 0; i < MAX_RANKS; i++) {
		if (rank_scores[i].score > score) {
			stringID = rank_scores[i].stringID;
			break;
		}
	}

	fame->rank = String_Get_ByIndex(stringID);

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x122);
	const int w = Font_GetStringWidth(fame->rank);

	/* Note: we want to fade out FAME.CPS to keep the black shadows. */
	fame->rank_half_width = w / 2;
	fame->rank_aux = Video_InitFadeInCPS("FAME.CPS", SCREEN_WIDTH / 2 - fame->rank_half_width, 49, w, g_fontCurrent->height, false);
}

void
HallOfFame_DrawScoreTime(int score, int64_t ticks_played)
{
	char buffer[64];

	snprintf(buffer, sizeof(buffer), String_Get_ByIndex(STR_TIME_DH_DM), (int)ticks_played / 60, (int)ticks_played % 60);

	if (ticks_played < 60) {
		char *hours = strchr(buffer, '0');
		while (*hours != ' ')
			memmove(hours, hours + 1, strlen(hours));
	}

	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_SCORE_D), 72, 15, 15, 0, 0x22, score);
	GUI_DrawText_Wrapper(buffer, 248, 15, 15, 0, 0x222);
	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_YOU_HAVE_ATTAINED_THE_RANK_OF), SCREEN_WIDTH / 2, 38, 15, 0, 0x122);
}

void
HallOfFame_DrawRank(const HallOfFameData *fame)
{
	GUI_DrawText_Wrapper(fame->rank, SCREEN_WIDTH / 2, 49, 6, 0, 0x122);
	Video_DrawFadeIn(fame->rank_aux, SCREEN_WIDTH / 2 - fame->rank_half_width, 49);
}

static void
HallOfFame_DrawYouEnemyLabel(int y)
{
	const char *str_you = String_Get_ByIndex(STR_YOU);
	const char *str_enemy = String_Get_ByIndex(STR_ENEMY);
	const int loc06 = max(Font_GetStringWidth(str_you), Font_GetStringWidth(str_enemy));
	const int loc18 = loc06 + 19;

	GUI_DrawText_Wrapper(str_you, loc18 - 4, y, 0xF, 0, 0x221);
	GUI_DrawText_Wrapper(str_enemy, loc18 - 4, y + (101 - 92), 0xF, 0, 0x221);
}

static void
HallOfFame_DrawMeter(enum HouseType houseID, const HallOfFameData *fame, int meter)
{
	const bool ally = ((meter & 0x1) == 0);
	const int x = 52;
	const int y = (ally ? 92 : 101) + 36 * (meter / 2) + 1;
	const int w = fame->meter[meter].width;
	const int h = 5;

	if (fame->state < HALLOFFAME_SHOW_METER)
		return;

	if (meter > fame->curr_meter_idx)
		return;

	if (w > 0) {
		if (ally) {
			const int64_t dt = Timer_GetTicks() - fame->meter_colour_timer;
			const int idx =
				(houseID == HOUSE_ATREIDES) ? 2 :
				(houseID == HOUSE_ORDOS) ? 1 : 0;

			int c[3] = { 0, 0, 0 };

			if (fame->meter_colour_dir) {
				c[idx] = 35 * 4 + 4.0f/3.0f * dt;
			}
			else {
				c[idx] = 63 * 4 - 4.0f/3.0f * dt;
			}

			c[idx] = clamp(35 * 4, c[idx], 63 * 4);
			GUI_DrawFilledRectRGBA(x, y, x + w - 1, y + h - 1, c[0], c[1], c[2], 0xFF);
		}
		else {
			GUI_DrawFilledRectangle(x, y, x + w - 1, y + h - 1, 209);
		}

		GUI_DrawLine(x + 1, y + h, x + w, y + h, 12);
		GUI_DrawLine(x + w, y + 1, x + w, y + h, 12);
	}

	const bool done =
		(meter < fame->curr_meter_idx) ||
		(meter == fame->curr_meter_idx && fame->state == HALLOFFAME_PAUSE_METER);

	if (done) {
		GUI_DrawText_Wrapper("%u", 287, y - 1, 0x0F, 0, 0x121, fame->meter[meter].max);
	}
	else {
		GUI_DrawText_Wrapper("%u", 287, y - 1, 0x14, 0, 0x121, fame->curr_meter_val);
	}
}

void
HallOfFame_DrawSpiceHarvested(enum HouseType houseID, const HallOfFameData *fame)
{
	const int y = 92;

	GUI_DrawTextOnFilledRectangle(String_Get_ByIndex(STR_SPICE_HARVESTED_BY), 83);
	HallOfFame_DrawYouEnemyLabel(y);

	HallOfFame_DrawMeter(houseID, fame, 0);
	HallOfFame_DrawMeter(houseID, fame, 1);
}

void
HallOfFame_DrawUnitsDestroyed(enum HouseType houseID, const HallOfFameData *fame)
{
	const int y = 92 + 36 * 1;

	Video_DrawCPSRegion("FAME.CPS", 8, 80, 8, 116, 304, 36);
	GUI_DrawTextOnFilledRectangle(String_Get_ByIndex(STR_UNITS_DESTROYED_BY), 119);
	HallOfFame_DrawYouEnemyLabel(y);

	HallOfFame_DrawMeter(houseID, fame, 2);
	HallOfFame_DrawMeter(houseID, fame, 3);
}

void
HallOfFame_DrawBuildingsDestroyed(enum HouseType houseID, int scenarioID, const HallOfFameData *fame)
{
	const int y = 92 + 36 * 2;

	if (scenarioID == 1) {
		GUI_DrawFilledRectangle(8, 152, 8 + 304 - 1, 191, 116);
	}
	else {
		Video_DrawCPSRegion("FAME.CPS", 8, 80, 8, 152, 304, 36);
		GUI_DrawTextOnFilledRectangle(String_Get_ByIndex(STR_BUILDINGS_DESTROYED_BY), 155);
		HallOfFame_DrawYouEnemyLabel(y);

		HallOfFame_DrawMeter(houseID, fame, 4);
		HallOfFame_DrawMeter(houseID, fame, 5);
	}
}
