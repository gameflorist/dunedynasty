/* $Id$ */

/** @file src/opendune.c Gameloop and other main routines. */

#if defined(_WIN32)
	#if defined(_MSC_VER)
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif /* _MSC_VER */
	#include <io.h>
	#include <windows.h>
#endif /* _WIN32 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "types.h"
#include "os/common.h"
#include "os/error.h"
#include "os/math.h"
#include "os/strings.h"
#include "os/sleep.h"

#include "opendune.h"

#include "animation.h"
#include "audio/driver.h"
#include "audio/sound.h"
#include "common_a5.h"
#include "config.h"
#include "crashlog/crashlog.h"
#include "cutscene.h"
#include "explosion.h"
#include "file.h"
#include "gfx.h"
#include "gui/font.h"
#include "gui/gui.h"
#include "gui/mentat.h"
#include "gui/security.h"
#include "gui/widget.h"
#include "house.h"
#include "ini.h"
#include "input/input.h"
#include "input/mouse.h"
#include "map.h"
#include "pool/pool.h"
#include "pool/house.h"
#include "pool/unit.h"
#include "pool/structure.h"
#include "pool/team.h"
#include "scenario.h"
#include "shape.h"
#include "sprites.h"
#include "string.h"
#include "structure.h"
#include "table/sound.h"
#include "table/strings.h"
#include "table/widgetinfo.h"
#include "team.h"
#include "tile.h"
#include "timer/timer.h"
#include "tools.h"
#include "unit.h"
#include "video/video.h"


char *window_caption = "OpenDUNE - Pre v0.8";

uint32 g_hintsShown1 = 0;          /*!< A bit-array to indicate which hints has been show already (0-31). */
uint32 g_hintsShown2 = 0;          /*!< A bit-array to indicate which hints has been show already (32-63). */
GameMode g_gameMode = GM_NORMAL;
uint16 g_campaignID = 0;
uint16 g_scenarioID = 1;
uint16 g_activeAction = 0xFFFF;      /*!< Action the controlled unit will do. */
int64_t g_tickScenarioStart = 0;     /*!< The tick the scenario started in. */
static uint32 s_tickGameTimeout = 0; /*!< The tick the game will timeout. */

bool   g_debugGame = false;        /*!< When true, you can control the AI. */
bool   g_debugScenario = false;    /*!< When true, you can review the scenario. There is no fog. The game is not running (no unit-movement, no structure-building, etc). You can click on individual tiles. */
bool   g_debugSkipDialogs = false; /*!< When non-zero, you immediately go to house selection, and skip all intros. */

void *g_readBuffer = NULL;
uint32 g_readBufferSize = 0;

static bool  s_debugForceWin = false; /*!< When true, you immediately win the level. */

static uint16 s_var_8052 = 0;

static uint8 s_enableLog = 0; /*!< 0 = off, 1 = record game, 2 = playback game (stored in 'dune.log'). */

uint16 g_var_38BC = 0;
bool g_var_38F8 = true;
uint16 g_selectionType = 0;
uint16 g_selectionTypeNew = 0;
bool g_viewport_forceRedraw = false;
bool g_var_3A14 = false;

int16 g_musicInBattle = 0; /*!< 0 = no battle, 1 = fight is going on, -1 = music of fight is going on is active. */

/**
 * Check if a level is finished, based on the values in WinFlags.
 *
 * @return True if and only if the level has come to an end.
 */
static bool GameLoop_IsLevelFinished(void)
{
	bool finish = false;

	if (s_debugForceWin) return true;

	/* You have to play at least 7200 ticks before you can win the game */
	if (g_timerGame - g_tickScenarioStart < 7200) return false;

	/* Check for structure counts hitting zero */
	if ((g_scenario.winFlags & 0x3) != 0) {
		PoolFindStruct find;
		uint16 countStructureEnemy = 0;
		uint16 countStructureFriendly = 0;

		find.houseID = HOUSE_INVALID;
		find.type    = 0xFFFF;
		find.index   = 0xFFFF;

		/* Calculate how many structures are left on the map */
		while (true) {
			Structure *s;

			s = Structure_Find(&find);
			if (s == NULL) break;

			if (s->o.type == STRUCTURE_TURRET) continue;
			if (s->o.type == STRUCTURE_ROCKET_TURRET) continue;

			if (s->o.houseID == g_playerHouseID) {
				countStructureFriendly++;
			} else {
				countStructureEnemy++;
			}
		}

		if ((g_scenario.winFlags & 0x1) != 0 && countStructureEnemy == 0) {
			finish = true;
		}
		if ((g_scenario.winFlags & 0x2) != 0 && countStructureFriendly == 0) {
			finish = true;
		}
	}

	/* Check for reaching spice quota */
	if ((g_scenario.winFlags & 0x4) != 0 && g_playerCredits != 0xFFFF) {
		if (g_playerCredits >= g_playerHouse->creditsQuota) {
			finish = true;
		}
	}

	/* Check for reaching timeout */
	if ((g_scenario.winFlags & 0x8) != 0) {
		/* XXX -- This code was with '<' instead of '>=', which makes
		 *  no sense. As it is unused, who knows what the intentions
		 *  were. This at least makes it sensible. */
		if (g_timerGame >= s_tickGameTimeout) {
			finish = true;
		}
	}

	return finish;
}

/**
 * Check if a level is won, based on the values in LoseFlags.
 *
 * @return True if and only if the level has been won by the human.
 */
static bool GameLoop_IsLevelWon(void)
{
	bool win = false;

	if (s_debugForceWin) return true;

	/* Check for structure counts hitting zero */
	if ((g_scenario.loseFlags & 0x3) != 0) {
		PoolFindStruct find;
		uint16 countStructureEnemy = 0;
		uint16 countStructureFriendly = 0;

		find.houseID = HOUSE_INVALID;
		find.type    = 0xFFFF;
		find.index   = 0xFFFF;

		/* Calculate how many structures are left on the map */
		while (true) {
			Structure *s;

			s = Structure_Find(&find);
			if (s == NULL) break;

			if (s->o.type == STRUCTURE_WALL) continue;
			if (s->o.type == STRUCTURE_TURRET) continue;
			if (s->o.type == STRUCTURE_ROCKET_TURRET) continue;

			if (s->o.houseID == g_playerHouseID) {
				countStructureFriendly++;
			} else {
				countStructureEnemy++;
			}
		}

		win = true;
		if ((g_scenario.loseFlags & 0x1) != 0) {
			win = win && (countStructureEnemy == 0);
		}
		if ((g_scenario.loseFlags & 0x2) != 0) {
			win = win && (countStructureFriendly != 0);
		}
	}

	/* Check for reaching spice quota */
	if (!win && (g_scenario.loseFlags & 0x4) != 0 && g_playerCredits != 0xFFFF) {
		win = (g_playerCredits >= g_playerHouse->creditsQuota);
	}

	/* Check for reaching timeout */
	if (!win && (g_scenario.loseFlags & 0x8) != 0) {
		win = (g_timerGame < s_tickGameTimeout);
	}

	return win;
}

#if 0
/* Moved to cutscene.c. */
static void GameLoop_PrepareAnimation(const HouseAnimation_Animation *animation, const HouseAnimation_Subtitle *subtitle, uint16 arg_8062, const HouseAnimation_SoundEffect *soundEffect);
#endif

static void Memory_ClearBlock(uint16 index)
{
	memset(GFX_Screen_Get_ByIndex(index), 0, GFX_Screen_GetSize_ByIndex(index));
}

#if 0
/* Moved to cutscene.c. */
static void GameLoop_FinishAnimation(void);
static void GameLoop_PlaySoundEffect(uint8 animation);
static void GameLoop_DrawText(char *string, uint16 top);
static void GameLoop_PlaySubtitle(uint8 animation);
static uint16 GameLoop_PalettePart_Update(bool finishNow);
static void GameLoop_PlayAnimation(void);
static void GameLoop_LevelEndAnimation(void);
#endif

void GameLoop_Uninit(void)
{
	while (g_widgetLinkedListHead != NULL) {
		Widget *w = g_widgetLinkedListHead;
		g_widgetLinkedListHead = w->next;

		free(w);
	}

	Script_ClearInfo(g_scriptStructure);
	Script_ClearInfo(g_scriptTeam);

	free(g_readBuffer); g_readBuffer = NULL;

	free(g_palette1); g_palette1 = NULL;
	free(g_palette2); g_palette2 = NULL;
	free(g_paletteMapping1); g_paletteMapping1 = NULL;
	free(g_paletteMapping2); g_paletteMapping2 = NULL;
}

#if 0
static void GameCredits_SwapScreen(uint16 top, uint16 height, uint16 screenID, void *buffer);
static void GameCredits_Play(char *data, uint16 windowID, uint16 memory, uint16 screenID, uint16 delay);
static void GameCredits_LoadPalette(void);
static void GameLoop_GameCredits(void);
static void GameLoop_GameEndAnimation(void);
#endif

/**
 * Checks if the level comes to an end. If so, it shows all end-level stuff,
 *  and prepares for the next level.
 */
static void GameLoop_LevelEnd(void)
{
	static uint32 levelEndTimer = 0;

	if (levelEndTimer >= g_timerGame && !s_debugForceWin) return;

	if (GameLoop_IsLevelFinished()) {
		Music_Play(0);

		Video_SetCursor(SHAPE_CURSOR_NORMAL);

		Sound_Output_Feedback(0xFFFE);

		GUI_ChangeSelectionType(SELECTIONTYPE_MENTAT);

		if (GameLoop_IsLevelWon()) {
			Sound_Output_Feedback(40);

			GUI_DisplayModalMessage(String_Get_ByIndex(STR_YOU_HAVE_SUCCESSFULLY_COMPLETED_YOUR_MISSION), 0xFFFF);

			GUI_Mentat_ShowWin();

			Sprites_UnloadTiles();

			g_campaignID++;

			GUI_EndStats_Show(g_scenario.killedAllied, g_scenario.killedEnemy, g_scenario.destroyedAllied, g_scenario.destroyedEnemy, g_scenario.harvestedAllied, g_scenario.harvestedEnemy, g_scenario.score, g_playerHouseID);

			if (g_campaignID == 9) {
				GUI_Mouse_Hide_Safe();

				GUI_SetPaletteAnimated(g_palette2, 15);
				GUI_ClearScreen(0);
				GameLoop_GameEndAnimation();
				PrepareEnd();
				exit(0);
			}

			GUI_Mouse_Hide_Safe();
			GameLoop_LevelEndAnimation();
			GUI_Mouse_Show_Safe();

			File_ReadBlockFile("IBM.PAL", g_palette1, 256 * 3);

			g_scenarioID = GUI_StrategicMap_Show(g_campaignID, true);

			GUI_SetPaletteAnimated(g_palette2, 15);

			if (g_campaignID == 1 || g_campaignID == 7) {
				if (!GUI_Security_Show()) {
					PrepareEnd();
					exit(0);
				}
			}
		} else {
			Sound_Output_Feedback(41);

			GUI_DisplayModalMessage(String_Get_ByIndex(STR_YOU_HAVE_FAILED_YOUR_MISSION), 0xFFFF);

			GUI_Mentat_ShowLose();

			Sprites_UnloadTiles();

			g_scenarioID = GUI_StrategicMap_Show(g_campaignID, false);
		}

		g_playerHouse->flags.doneFullScaleAttack = false;

		Sprites_LoadTiles();

		g_gameMode = GM_RESTART;
		s_debugForceWin = false;
	}

	levelEndTimer = g_timerGame + 300;
}

#if 0
/* Moved to cutscene.c. */
static void Gameloop_Logos(void);
static void GameLoop_GameIntroAnimation(void);
#endif

static uint16 GameLoop_B4E6_0000(uint16 arg06, uint32 arg08, uint16 arg0C)
{
	uint16 i = 0;

	if (arg08 == 0xFFFFFFFF) return arg06;

	while (arg06 != 0) {
		if ((arg08 & (1 << (arg0C + i))) != 0) arg06--;
		i++;
	}

	while (true) {
		if ((arg08 & (1 << (arg0C + i))) != 0) break;
		i++;
	}

	return i;
}

static void GameLoop_B4E6_0108(uint16 arg06, char **strings, uint32 arg0C, uint16 arg10, uint16 arg12)
{
	WidgetProperties *props;
	uint16 left;
	uint16 old;
	uint16 top;
	uint8 i;

	props = &g_widgetProperties[21 + arg06];
	top = g_curWidgetYBase + props->yBase;
	left = (g_curWidgetXBase + props->xBase) << 3;

	old = GameLoop_B4E6_0000(props->fgColourBlink, arg0C, arg10);

	GUI_Mouse_Hide_Safe();

	for (i = 0; i < props->height; i++) {
		uint16 index = GameLoop_B4E6_0000(i, arg0C, arg10);
		uint16 pos = top + ((g_fontCurrent->height + arg12) * i);

		if (index == old) {
			GUI_DrawText_Wrapper(strings[index], left, pos, props->fgColourSelected, 0, 0x22);
		} else {
			GUI_DrawText_Wrapper(strings[index], left, pos, props->fgColourNormal, 0, 0x22);
		}
	}

	s_var_8052 = arg12;

	GUI_Mouse_Show_Safe();

	Input_History_Clear();
}

static void GameLoop_DrawText2(char *string, uint16 left, uint16 top, uint8 fgColourNormal, uint8 fgColourSelected, uint8 bgColour)
{
	uint8 i;

	for (i = 0; i < 3; i++) {
		GUI_Mouse_Hide_Safe();

		GUI_DrawText_Wrapper(string, left, top, fgColourSelected, bgColour, 0x22);
		Timer_Sleep(2);

		GUI_DrawText_Wrapper(string, left, top, fgColourNormal, bgColour, 0x22);
		GUI_Mouse_Show_Safe();
		Timer_Sleep(2);
	}
}

static bool GameLoop_IsInRange(uint16 x, uint16 y, uint16 minX, uint16 minY, uint16 maxX, uint16 maxY)
{
	return x >= minX && x <= maxX && y >= minY && y <= maxY;
}

static uint16 GameLoop_HandleEvents(uint16 arg06, char **strings, uint32 arg10, uint16 arg14)
{
	uint8 last;
	uint16 result;
	uint16 key;
	uint16 top;
	uint16 left;
	uint16 minX;
	uint16 maxX;
	uint16 minY;
	uint16 maxY;
	uint16 lineHeight;
	uint8 fgColourNormal;
	uint8 fgColourSelected;
	uint8 old;
	WidgetProperties *props;
	uint8 current;

	props = &g_widgetProperties[21 + arg06];

	last = props->height - 1;
	old = props->fgColourBlink % (last + 1);
	current = old;

	result = 0xFFFF;

	top = g_curWidgetYBase + props->yBase;
	left = (g_curWidgetXBase + props->xBase) << 3;

	lineHeight = g_fontCurrent->height + s_var_8052;

	minX = (g_curWidgetXBase << 3) + (g_fontCurrent->maxWidth * props->xBase);
	minY = g_curWidgetYBase + props->yBase - s_var_8052 / 2;
	maxX = minX + (g_fontCurrent->maxWidth * props->width) - 1;
	maxY = minY + (props->height * lineHeight) - 1;

	fgColourNormal = props->fgColourNormal;
	fgColourSelected = props->fgColourSelected;

	key = 0;
	if (Input_IsInputAvailable() != 0) {
		key = Input_Wait() & 0x8FF;
	}

	if (g_var_7097 == 0) {
		uint16 y = g_mouseY;

		if (GameLoop_IsInRange(g_mouseX, y, minX, minY, maxX, maxY)) {
			current = (y - minY) / lineHeight;
		}
	}

	switch (key) {
		case 0x60: /* NUMPAD 8 / ARROW UP */
			if (current-- == 0) current = last;
			break;

		case 0x62: /* NUMPAD 2 / ARROW DOWN */
			if (current++ == last) current = 0;
			break;

		case 0x5B: /* NUMPAD 7 / HOME */
		case 0x65: /* NUMPAD 9 / PAGE UP */
			current = 0;
			break;

		case 0x5D: /* NUMPAD 1 / END */
		case 0x67: /* NUMPAD 3 / PAGE DOWN */
			current = last;
			break;

		case 0x41: /* MOUSE LEFT BUTTON */
		case 0x42: /* MOUSE RIGHT BUTTON */
			if (GameLoop_IsInRange(g_mouseClickX, g_mouseClickY, minX, minY, maxX, maxY)) {
				current = (g_mouseClickY - minY) / lineHeight;
				result = current;
			}
			break;

		case 0x2B: /* NUMPAD 5 / RETURN */
		case 0x3D: /* SPACE */
		case 0x61:
			result = current;
			break;

		default: {
			uint8 i;

			for (i = 0; i < props->height; i++) {
				char c1;
				char c2;

				c1 = toupper(*strings[GameLoop_B4E6_0000(i, arg10, arg14)]);
				c2 = toupper(Input_Keyboard_HandleKeys(key & 0xFF));

				if (c1 == c2) {
					result = i;
					current = i;
					break;
				}
			}
		} break;
	}

	if (current != old) {
		uint16 index;

		GUI_Mouse_Hide_Safe();

		index = GameLoop_B4E6_0000(old, arg10, arg14);

		GUI_DrawText_Wrapper(strings[index], left, top + (old * lineHeight), fgColourNormal, 0, 0x22);

		index = GameLoop_B4E6_0000(current, arg10, arg14);

		GUI_DrawText_Wrapper(strings[index], left, top + (current * lineHeight), fgColourSelected, 0, 0x22);

		GUI_Mouse_Show_Safe();
	}

	props->fgColourBlink = current;

	if (result == 0xFFFF) return 0xFFFF;

	result = GameLoop_B4E6_0000(result, arg10, arg14);

	GUI_Mouse_Hide_Safe();
	GameLoop_DrawText2(strings[result], left, top + (current * lineHeight), fgColourNormal, fgColourSelected, 0);
	GUI_Mouse_Show_Safe();

	return result;
}

static void Window_WidgetClick_Create(void)
{
	WidgetInfo *wi;

	for (wi = g_table_gameWidgetInfo; wi->index >= 0; wi++) {
		Widget *w;

		w = GUI_Widget_Allocate(wi->index, wi->shortcut, wi->offsetX, wi->offsetY, wi->spriteID, wi->stringID);

		if (wi->spriteID < 0) {
			w->width  = wi->width;
			w->height = wi->height;
		}

		w->clickProc = wi->clickProc;
		w->flags.all = wi->flags;

		g_widgetLinkedListHead = GUI_Widget_Insert(g_widgetLinkedListHead, w);
	}
}

static void ReadProfileIni(char *filename)
{
	char *source;
	char *key;
	char *keys;
	char buffer[120];
	uint16 locsi;

	if (filename == NULL) return;
	if (!File_Exists(filename)) return;

	source = GFX_Screen_Get_ByIndex(3);

	memset(source, 0, 32000);
	File_ReadBlockFile(filename, source, GFX_Screen_GetSize_ByIndex(3));

	keys = source + strlen(source) + 5000;
	*keys = '\0';

	Ini_GetString("construct", NULL, keys, keys, 2000, source);

	for (key = keys; *key != '\0'; key += strlen(key) + 1) {
		ObjectInfo *oi = NULL;
		uint16 count;
		uint8 type;
		uint16 buildCredits;
		uint16 buildTime;
		uint16 fogUncoverRadius;
		uint16 availableCampaign;
		uint16 sortPriority;
		uint16 priorityBuild;
		uint16 priorityTarget;
		uint16 hitpoints;

		type = Unit_StringToType(key);
		if (type != UNIT_INVALID) {
			oi = &g_table_unitInfo[type].o;
		} else {
			type = Structure_StringToType(key);
			if (type != STRUCTURE_INVALID) oi = &g_table_structureInfo[type].o;
		}

		if (oi == NULL) continue;

		Ini_GetString("construct", key, buffer, buffer, 120, source);
		count = sscanf(buffer, "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu", &buildCredits, &buildTime, &hitpoints, &fogUncoverRadius, &availableCampaign, &priorityBuild, &priorityTarget, &sortPriority);
		oi->buildCredits      = buildCredits;
		oi->buildTime         = buildTime;
		oi->hitpoints         = hitpoints;
		oi->fogUncoverRadius  = fogUncoverRadius;
		oi->availableCampaign = availableCampaign;
		oi->priorityBuild     = priorityBuild;
		oi->priorityTarget    = priorityTarget;
		if (count <= 7) continue;
		oi->sortPriority = (uint8)sortPriority;
	}

	if (g_debugGame) {
		for (locsi = 0; locsi < UNIT_MAX; locsi++) {
			ObjectInfo *oi = &g_table_unitInfo[locsi].o;

			sprintf(buffer, "%*s%4d,%4d,%4d,%4d,%4d,%4d,%4d,%4d",
				15 - (int)strlen(oi->name), "", oi->buildCredits, oi->buildTime, oi->hitpoints, oi->fogUncoverRadius,
				oi->availableCampaign, oi->priorityBuild, oi->priorityTarget, oi->sortPriority);

			Ini_SetString("construct", oi->name, buffer, source);
		}

		for (locsi = 0; locsi < STRUCTURE_MAX; locsi++) {
			ObjectInfo *oi = &g_table_unitInfo[locsi].o;

			sprintf(buffer, "%*s%4d,%4d,%4d,%4d,%4d,%4d,%4d,%4d",
				15 - (int)strlen(oi->name), "", oi->buildCredits, oi->buildTime, oi->hitpoints, oi->fogUncoverRadius,
				oi->availableCampaign, oi->priorityBuild, oi->priorityTarget, oi->sortPriority);

			Ini_SetString("construct", oi->name, buffer, source);
		}
	}

	*keys = '\0';

	Ini_GetString("combat", NULL, keys, keys, 2000, source);

	for (key = keys; *key != '\0'; key += strlen(key) + 1) {
		uint16 damage;
		uint16 movingSpeed;
		uint16 fireDelay;
		uint16 fireDistance;

		Ini_GetString("combat", key, buffer, buffer, 120, source);
		String_Trim(buffer);
		if (sscanf(buffer, "%hu,%hu,%hu,%hu", &fireDistance, &damage, &fireDelay, &movingSpeed) < 4) continue;

		for (locsi = 0; locsi < UNIT_MAX; locsi++) {
			UnitInfo *ui = &g_table_unitInfo[locsi];

			if (strcasecmp(ui->o.name, key) != 0) continue;

			ui->damage       = damage;
			ui->movingSpeed  = movingSpeed;
			ui->fireDelay    = fireDelay;
			ui->fireDistance = fireDistance;
			break;
		}
	}

	if (!g_debugGame) return;

	for (locsi = 0; locsi < UNIT_MAX; locsi++) {
		const UnitInfo *ui = &g_table_unitInfo[locsi];

		sprintf(buffer, "%*s%4d,%4d,%4d,%4d", 15 - (int)strlen(ui->o.name), "", ui->fireDistance, ui->damage, ui->fireDelay, ui->movingSpeed);
		Ini_SetString("combat", ui->o.name, buffer, source);
	}
}

/**
 * Intro menu.
 */
static void GameLoop_GameIntroAnimationMenu(void)
{
	static const uint16 mainMenuStrings[][6] = {
		{STR_PLAY_A_GAME, STR_REPLAY_INTRODUCTION, STR_EXIT_GAME, STR_NULL,         STR_NULL,         STR_NULL}, /* Neither HOF nor save. */
		{STR_PLAY_A_GAME, STR_REPLAY_INTRODUCTION, STR_LOAD_GAME, STR_EXIT_GAME,    STR_NULL,         STR_NULL}, /* Has a save game. */
		{STR_PLAY_A_GAME, STR_REPLAY_INTRODUCTION, STR_EXIT_GAME, STR_HALL_OF_FAME, STR_NULL,         STR_NULL}, /* Has a HOF. */
		{STR_PLAY_A_GAME, STR_REPLAY_INTRODUCTION, STR_LOAD_GAME, STR_EXIT_GAME,    STR_HALL_OF_FAME, STR_NULL}  /* Has a HOF and a save game. */
	};

	bool loc02 = false;
	bool loc06;

	Input_Flags_ClearBits(INPUT_FLAG_KEY_RELEASE | INPUT_FLAG_UNKNOWN_0400 | INPUT_FLAG_UNKNOWN_0100 |
	                      INPUT_FLAG_UNKNOWN_0080 | INPUT_FLAG_UNKNOWN_0040 | INPUT_FLAG_UNKNOWN_0020 |
	                      INPUT_FLAG_UNKNOWN_0008 | INPUT_FLAG_UNKNOWN_0004 | INPUT_FLAG_UNKNOWN_0002);

	Timer_SetTimer(TIMER_GUI, true);

	g_campaignID = 0;
	g_scenarioID = 1;
	g_playerHouseID = HOUSE_INVALID;
	g_debugScenario = false;
	g_table_landscapeInfo[LST_SPICE].radarColour = 0xD7;
	g_table_landscapeInfo[LST_SPICE].spriteID = 0x35;
	g_table_landscapeInfo[LST_THICK_SPICE].radarColour = 0xD8;
	g_table_landscapeInfo[LST_THICK_SPICE].spriteID = 0x35;
	g_selectionType = SELECTIONTYPE_MENTAT;
	g_selectionTypeNew = SELECTIONTYPE_MENTAT;

	g_palette1 = calloc(1, 256 * 3);
	g_palette2 = calloc(1, 256 * 3);

	g_readBufferSize = 0x2EE0;
	g_readBuffer = calloc(1, g_readBufferSize);

	ReadProfileIni("PROFILE.INI");

	free(g_readBuffer); g_readBuffer = NULL;

	File_ReadBlockFile("IBM.PAL", g_palette_998A, 256 * 3);

	memmove(g_palette1, g_palette_998A, 256 * 3);

	GUI_ClearScreen(0);

	Video_SetPalette(g_palette1, 0, 256);

	GFX_SetPalette(g_palette1);
	GFX_SetPalette(g_palette2);

	g_paletteMapping1 = malloc(256);
	g_paletteMapping2 = malloc(256);

	GUI_Palette_CreateMapping(g_palette1, g_paletteMapping1, 0xC, 0x55);
	g_paletteMapping1[0xFF] = 0xFF;
	g_paletteMapping1[0xDF] = 0xDF;
	g_paletteMapping1[0xEF] = 0xEF;

	GUI_Palette_CreateMapping(g_palette1, g_paletteMapping2, 0xF, 0x55);
	g_paletteMapping2[0xFF] = 0xFF;
	g_paletteMapping2[0xDF] = 0xDF;
	g_paletteMapping2[0xEF] = 0xEF;

	Script_LoadFromFile("TEAM.EMC", g_scriptTeam, g_scriptFunctionsTeam, NULL);
	Script_LoadFromFile("BUILD.EMC", g_scriptStructure, g_scriptFunctionsStructure, NULL);

	GUI_Palette_CreateRemap(HOUSE_MERCENARY);

	Video_SetCursor(SHAPE_CURSOR_NORMAL);

	while (g_mouseHiddenDepth > 1) {
		GUI_Mouse_Show_Safe();
	}

	Window_WidgetClick_Create();
	GameOptions_Load();
	Unit_Init();
	Team_Init();
	House_Init();
	Structure_Init();

	loc06 = true;

	GUI_Mouse_Show_Safe();

	if (!g_debugSkipDialogs) {
		uint16 stringID;
		uint16 maxWidth;
		bool hasSave;
		bool hasFame;
		bool loc10;

		loc10 = true;

		hasSave = File_Exists("_save000.dat");
		hasFame = File_Exists("SAVEFAME.DAT");

		if (hasSave || File_Exists("ONETIME.DAT")) s_var_37B4 = true;

		stringID = STR_REPLAY_INTRODUCTION;

		while (true) {
			char *strings[6];

			switch (stringID) {
				case STR_REPLAY_INTRODUCTION:
					Music_Play(0);

					free(g_readBuffer);
					g_readBufferSize = (g_enableVoices == 0) ? 0x2EE0 : 0x6D60;
					g_readBuffer = calloc(1, g_readBufferSize);

					GUI_Mouse_Hide_Safe();

					Driver_Music_FadeOut();

					GameLoop_GameIntroAnimation();

					Sound_Output_Feedback(0xFFFE);

					File_ReadBlockFile("IBM.PAL", g_palette_998A, 256 * 3);
					memmove(g_palette1, g_palette_998A, 256 * 3);

					if (!s_var_37B4) {
						uint8 fileID;

						fileID = File_Open("ONETIME.DAT", 2);
						File_Close(fileID);
						s_var_37B4 = true;
					}

					Music_Play(0);

					free(g_readBuffer);
					g_readBufferSize = (g_enableVoices == 0) ? 0x2EE0 : 0x4E20;
					g_readBuffer = calloc(1, g_readBufferSize);

					GUI_Mouse_Show_Safe();

					Music_Play(28);

					loc06 = true;
					break;

				case STR_EXIT_GAME:
					PrepareEnd();
					exit(0);
					break;

				case STR_HALL_OF_FAME:
					GUI_HallOfFame_Show(0xFFFF);

					GFX_SetPalette(g_palette2);

					hasFame = File_Exists("SAVEFAME.DAT");
					loc06 = true;
					break;

				case STR_LOAD_GAME:
					GUI_Mouse_Hide_Safe();
					GUI_SetPaletteAnimated(g_palette2, 30);
					GUI_ClearScreen(0);
					GUI_Mouse_Show_Safe();

					GFX_SetPalette(g_palette1);

					if (GUI_Widget_SaveLoad_Click(false)) {
						loc02 = true;
						loc10 = false;
						if (g_gameMode == GM_RESTART) break;
						g_gameMode = GM_NORMAL;
					} else {
						GFX_SetPalette(g_palette2);

						loc06 = true;
					}
					break;

				default: break;
			}

			if (loc06) {
				uint16 index = (hasFame ? 2 : 0) + (hasSave ? 1 : 0);
				uint16 i;

				g_widgetProperties[21].height = 0;

				for (i = 0; i < 6; i++) {
					strings[i] = NULL;

					if (mainMenuStrings[index][i] == 0) {
						if (g_widgetProperties[21].height == 0) g_widgetProperties[21].height = i;
						continue;
					}

					strings[i] = String_Get_ByIndex(mainMenuStrings[index][i]);
				}

				GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

				maxWidth = 0;

				for (i = 0; i < g_widgetProperties[21].height; i++) {
					if (Font_GetStringWidth(strings[i]) <= maxWidth) continue;
					maxWidth = Font_GetStringWidth(strings[i]);
				}

				maxWidth += 7;

				g_widgetProperties[21].width  = maxWidth >> 3;
				g_widgetProperties[13].width  = g_widgetProperties[21].width + 2;
				g_widgetProperties[13].xBase  = 19 - (maxWidth >> 4);
				g_widgetProperties[13].yBase  = 160 - ((g_widgetProperties[21].height * g_fontCurrent->height) >> 1);
				g_widgetProperties[13].height = (g_widgetProperties[21].height * g_fontCurrent->height) + 11;

				Sprites_LoadImage(String_GenerateFilename("TITLE"), 3, NULL);

				GUI_Mouse_Hide_Safe();

				GUI_ClearScreen(0);

				GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, 2, 0);

				GUI_SetPaletteAnimated(g_palette1, 30);

				GUI_DrawText_Wrapper("V1.07", 319, 192, 133, 0, 0x231, 0x39);
				GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

				Widget_SetCurrentWidget(13);

				GUI_Widget_DrawBorder(13, 2, 1);

				GameLoop_B4E6_0108(0, strings, 0xFFFF, 0, 0);

				GUI_Mouse_Show_Safe();

				loc06 = false;
			}

			if (!loc10) break;

			stringID = GameLoop_HandleEvents(0, strings, 0xFF, 0);

			if (stringID != 0xFFFF) {
				uint16 index = (hasFame ? 2 : 0) + (hasSave ? 1 : 0);
				stringID = mainMenuStrings[index][stringID];
			}

			GUI_PaletteAnimate();

			if (stringID == STR_PLAY_A_GAME) break;

			Video_Tick();
			sleepIdle();
		}
	} else {
		Music_Play(0);

		free(g_readBuffer);
		g_readBufferSize = (g_enableVoices == 0) ? 0x2EE0 : 0x4E20;
		g_readBuffer = calloc(1, g_readBufferSize);
	}

	GUI_Mouse_Hide_Safe();

	s_var_37B4 = false;

	GUI_DrawFilledRectangle(g_curWidgetXBase << 3, g_curWidgetYBase, (g_curWidgetXBase + g_curWidgetWidth) << 3, g_curWidgetYBase + g_curWidgetHeight, 12);

	if (!loc02) {
		Voice_LoadVoices(5);

		GUI_SetPaletteAnimated(g_palette2, 15);

		GUI_ClearScreen(0);
	}

	Input_History_Clear();

	if (s_enableLog != 0) Mouse_SetMouseMode((uint8)s_enableLog, "DUNE.LOG");

	if (!loc02) {
		if (g_playerHouseID == HOUSE_INVALID) {
			GUI_Mouse_Show_Safe();

			g_playerHouseID = HOUSE_MERCENARY;
			g_playerHouseID = GUI_PickHouse();

			GUI_Mouse_Hide_Safe();
		}

		Sprites_LoadTiles();

		GUI_Palette_CreateRemap(g_playerHouseID);

		Voice_LoadVoices(g_playerHouseID);

		GUI_Mouse_Show_Safe();

		if (g_campaignID != 0) g_scenarioID = GUI_StrategicMap_Show(g_campaignID, true);

		Game_LoadScenario(g_playerHouseID, g_scenarioID);
		if (!g_debugScenario && !g_debugSkipDialogs) GUI_Mentat_ShowBriefing();

		GUI_Mouse_Hide_Safe();

		GUI_ChangeSelectionType(g_debugScenario ? SELECTIONTYPE_DEBUG : SELECTIONTYPE_STRUCTURE);
	}

	GFX_SetPalette(g_palette1);

	return;
}

static void InGame_Numpad_Move(uint16 key)
{
	if (key == 0) return;

	switch (key) {
		case 0x0010: /* TAB */
			Map_SelectNext(true);
			return;

		case 0x0110: /* SHIFT TAB */
			Map_SelectNext(false);
			return;

		case 0x005C: /* NUMPAD 4 / ARROW LEFT */
		case 0x045C:
		case 0x055C:
			Map_MoveDirection(6);
			return;

		case 0x0066: /* NUMPAD 6 / ARROW RIGHT */
		case 0x0466:
		case 0x0566:
			Map_MoveDirection(2);
			return;

		case 0x0060: /* NUMPAD 8 / ARROW UP */
		case 0x0460:
		case 0x0560:
			Map_MoveDirection(0);
			return;

		case 0x0062: /* NUMPAD 2 / ARROW DOWN */
		case 0x0462:
		case 0x0562:
			Map_MoveDirection(4);
			return;

		case 0x005B: /* NUMPAD 7 / HOME */
		case 0x045B:
		case 0x055B:
			Map_MoveDirection(7);
			return;

		case 0x005D: /* NUMPAD 1 / END */
		case 0x045D:
		case 0x055D:
			Map_MoveDirection(5);
			return;

		case 0x0065: /* NUMPAD 9 / PAGE UP */
		case 0x0465:
		case 0x0565:
			Map_MoveDirection(1);
			return;

		case 0x0067: /* NUMPAD 3 / PAGE DOWN */
		case 0x0467:
		case 0x0567:
			Map_MoveDirection(3);
			return;

		default: return;
	}
}

/**
 * Main game loop.
 */
static void GameLoop_Main(void)
{
	static int64_t l_timerNext = 0;
	static uint32 l_timerUnitStatus = 0;
	static int16  l_selectionState = -2;

	uint16 key;

	String_Init();
	Sprites_Init();
	Sprites_LoadTiles();
	VideoA5_InitSprites();

	GameLoop_GameIntroAnimationMenu();

	Timer_SetTimer(TIMER_GAME, true);

	GUI_Mouse_Show_Safe();

	Music_Play(Tools_RandomRange(0, 5) + 8);

	while (true) {
		if (g_gameMode == GM_PICKHOUSE) {
			Music_Play(28);

			g_playerHouseID = HOUSE_MERCENARY;
			g_playerHouseID = GUI_PickHouse();

			GUI_Mouse_Hide_Safe();

			Memory_ClearBlock(1);

			Sprites_LoadTiles();

			GUI_Palette_CreateRemap(g_playerHouseID);

			Voice_LoadVoices(g_playerHouseID);

			GUI_Mouse_Show_Safe();

			g_gameMode = GM_RESTART;
			g_scenarioID = 1;
			g_campaignID = 0;
			g_strategicRegionBits = 0;
		}

		if (g_selectionTypeNew != g_selectionType) {
			GUI_ChangeSelectionType(g_selectionTypeNew);
		}

		GUI_PaletteAnimate();

		if (g_gameMode == GM_RESTART) {
			GUI_ChangeSelectionType(SELECTIONTYPE_MENTAT);

			Game_LoadScenario(g_playerHouseID, g_scenarioID);
			if (!g_debugScenario && !g_debugSkipDialogs) GUI_Mentat_ShowBriefing();

			g_gameMode = GM_NORMAL;

			GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);

			Music_Play(Tools_RandomRange(0, 8) + 8);
			l_timerNext = Timer_GetTicks() + 300;
		}

		if (l_selectionState != g_selectionState) {
			Map_SetSelectionObjectPosition(0xFFFF);
			Map_SetSelectionObjectPosition(g_selectionRectanglePosition);
			l_selectionState = g_selectionState;
		}

		if (!Driver_Voice_IsPlaying() && !Sound_StartSpeech()) {
			if (g_gameConfig.music == 0) {
				Music_Play(2);

				g_musicInBattle = 0;
			} else if (g_musicInBattle > 0) {
				Music_Play(Tools_RandomRange(0, 5) + 17);
				l_timerNext = Timer_GetTicks() + 300;
				g_musicInBattle = -1;
			} else {
				g_musicInBattle = 0;
				if (g_enableSoundMusic != 0 && Timer_GetTicks() > l_timerNext) {
					if (!Driver_Music_IsPlaying()) {
						Music_Play(Tools_RandomRange(0, 8) + 8);
						l_timerNext = Timer_GetTicks() + 300;
					}
				}
			}
		}

		GFX_Screen_SetActive(0);

		key = GUI_Widget_HandleEvents(g_widgetLinkedListHead);

		GUI_DrawInterfaceAndRadar(0);

		if (g_selectionType == SELECTIONTYPE_TARGET || g_selectionType == SELECTIONTYPE_PLACE || g_selectionType == SELECTIONTYPE_UNIT || g_selectionType == SELECTIONTYPE_STRUCTURE) {
			if (g_unitSelected != NULL) {
				if (l_timerUnitStatus < g_timerGame) {
					Unit_DisplayStatusText(g_unitSelected);
					l_timerUnitStatus = g_timerGame + 300;
				}

				if (g_selectionType != SELECTIONTYPE_TARGET) {
					g_selectionPosition = Tile_PackTile(Tile_Center(g_unitSelected->o.position));
				}
			}

			GUI_Widget_ActionPanel_Draw(false);

			InGame_Numpad_Move(key);

			GUI_DrawCredits(g_playerHouseID, 0);

			g_timerGame = Timer_GameTicks();

			GameLoop_Team();
			GameLoop_Unit();
			GameLoop_Structure();
			GameLoop_House();

			GUI_DrawScreen(0);
		}

		GUI_DisplayText(NULL, 0);

		if (g_var_38F8 && !g_debugScenario) {
			GameLoop_LevelEnd();
		}

		if (!g_var_38F8) break;

		Video_Tick();
		sleepIdle();
	}

	GUI_Mouse_Hide_Safe();

	if (s_enableLog != 0) Mouse_SetMouseMode(INPUT_MOUSE_MODE_NORMAL, "DUNE.LOG");

	GUI_Mouse_Hide_Safe();

	Widget_SetCurrentWidget(0);

	GFX_Screen_SetActive(2);

	GFX_ClearScreen();

	GUI_Screen_FadeIn(g_curWidgetXBase, g_curWidgetYBase, g_curWidgetXBase, g_curWidgetYBase, g_curWidgetWidth, g_curWidgetHeight, 2, 0);
}

static bool Unknown_25C4_000E(void)
{
	if (!Video_Init()) return false;

	Mouse_Init();

	g_var_7097 = -1;

	GFX_Init();
	GFX_ClearScreen();

	if (!Font_Init()) {
		Error(
			"--------------------------\n"
			"ERROR LOADING DATA FILE\n"
			"\n"
			"Did you copy the Dune2 1.07eu data files into the data directory ?\n"
			"\n"
		);

		return false;
	}

	Font_Select(g_fontNew8p);

	g_palette_998A = calloc(256 * 3, sizeof(uint8));

	memset(&g_palette_998A[45], 63, 3);

	GFX_SetPalette(g_palette_998A);

	srand((unsigned)time(NULL));

	Widget_SetCurrentWidget(0);

	return true;
}

#if defined(__APPLE__)
int SDL_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif /* __APPLE__ */
{
#if defined(_WIN32)
	#if defined(__MINGW32__) && defined(__STRICT_ANSI__)
		int __cdecl __MINGW_NOTHROW _fileno (FILE*);
	#endif
	FILE *err = fopen("error.log", "w");
	FILE *out = fopen("output.log", "w");

	#if defined(_MSC_VER)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	if (err != NULL) _dup2(_fileno(err), _fileno(stderr));
	if (out != NULL) _dup2(_fileno(out), _fileno(stdout));
	FreeConsole();
#endif
	CrashLog_Init();

	if (A5_Init() == false)
		exit(1);

	VARIABLE_NOT_USED(argc);
	VARIABLE_NOT_USED(argv);

	if (!Config_Read("dune.cfg", &g_config)) {
		Error("Missing dune.cfg file.\n");
		exit(1);
	}

	Input_Init();

	Drivers_All_Init();

	if (!Unknown_25C4_000E()) exit(1);

	g_var_7097 = 0;

	GameLoop_Main();

	printf("%s\n", String_Get_ByIndex(STR_THANK_YOU_FOR_PLAYING_DUNE_II));

	PrepareEnd();
	exit(0);
}

/**
 * Prepare the map (after loading scenario or savegame). Does some basic
 *  sanity-check and corrects stuff all over the place.
 */
void Game_Prepare(void)
{
	PoolFindStruct find;
	uint16 oldSelectionType;
	Tile *t;
	int i;

	g_var_38BC++;

	oldSelectionType = g_selectionType;
	g_selectionType = SELECTIONTYPE_MENTAT;

	Structure_Recount();
	Unit_Recount();
	Team_Recount();

	t = &g_map[0];
	for (i = 0; i < 64 * 64; i++, t++) {
		Structure *s;
		Unit *u;

		u = Unit_Get_ByPackedTile(i);
		s = Structure_Get_ByPackedTile(i);

		if (u == NULL || !u->o.flags.s.used) t->hasUnit = false;
		if (s == NULL || !s->o.flags.s.used) t->hasStructure = false;
		if (t->isUnveiled) Map_UnveilTile(i, g_playerHouseID);
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;

		if (u->o.flags.s.isNotOnMap) continue;

		Unit_RemoveFog(u);
		Unit_UpdateMap(1, u);
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;

		if (s->o.flags.s.isNotOnMap) continue;

		Structure_RemoveFog(s);

		if (s->o.type == STRUCTURE_STARPORT && s->o.linkedID != 0xFF) {
			Unit *u = Unit_Get_ByIndex(s->o.linkedID);

			if (!u->o.flags.s.used || !u->o.flags.s.isNotOnMap) {
				s->o.linkedID = 0xFF;
				s->countDown = 0;
			} else {
				Structure_SetState(s, STRUCTURE_STATE_READY);
			}
		}

		Script_Load(&s->o.script, s->o.type);

		if (s->o.type == STRUCTURE_PALACE) {
			House_Get_ByIndex(s->o.houseID)->palacePosition = s->o.position;
		}

		if (House_Get_ByIndex(s->o.houseID)->palacePosition.tile != 0) continue;
		House_Get_ByIndex(s->o.houseID)->palacePosition = s->o.position;
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		House *h;

		h = House_Find(&find);
		if (h == NULL) break;

		h->structuresBuilt = Structure_GetStructuresBuilt(h);
		House_UpdateCreditsStorage((uint8)h->index);
		House_CalculatePowerAndCredit(h);
	}

	GUI_Palette_CreateRemap(g_playerHouseID);

	Map_SetSelection(g_selectionPosition);

	if (g_structureActiveType != 0xFFFF) {
		Map_SetSelectionSize(g_table_structureInfo[g_structureActiveType].layout);
	} else {
		Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);

		if (s != NULL) Map_SetSelectionSize(g_table_structureInfo[s->o.type].layout);
	}

	Voice_LoadVoices(g_playerHouseID);

	g_tickHousePowerMaintenance = max(g_timerGame + 70, g_tickHousePowerMaintenance);
	g_viewport_forceRedraw = true;
	g_playerCredits = 0xFFFF;

	g_selectionType = oldSelectionType;
	g_var_38BC--;
}

/**
 * Initialize a game, by setting most variables to zero, cleaning the map, etc
 *  etc.
 */
void Game_Init(void)
{
	Unit_Init();
	Structure_Init();
	Team_Init();
	House_Init();

	memset(g_animations, 0, ANIMATION_MAX * sizeof(Animation));
	memset(g_explosions, 0, EXPLOSION_MAX * sizeof(Explosion));
	memset(g_map, 0, 64 * 64 * sizeof(Tile));

	memset(g_displayedViewport, 0, sizeof(g_displayedViewport));
	memset(g_displayedMinimap,  0, sizeof(g_displayedMinimap));
	memset(g_changedTilesMap,   0, sizeof(g_changedTilesMap));
	memset(g_dirtyViewport,     0, sizeof(g_dirtyViewport));
	memset(g_dirtyMinimap,      0, sizeof(g_dirtyMinimap));

	memset(g_mapSpriteID, 0, 64 * 64 * sizeof(uint16));
	memset(g_starportAvailable, 0, sizeof(g_starportAvailable));

	Sound_Output_Feedback(0xFFFE);

	g_playerCreditsNoSilo     = 0;
	g_houseMissileCountdown   = 0;
	g_selectionState          = 0; /* Invalid. */
	g_structureActivePosition = 0;

	g_unitHouseMissile = NULL;
	g_unitActive       = NULL;
	g_structureActive  = NULL;

	g_activeAction          = 0xFFFF;
	g_structureActiveType   = 0xFFFF;

	GUI_DisplayText(NULL, -1);
}

/**
 * Load a scenario is a safe way, and prepare the game.
 * @param houseID The House which is going to play the game.
 * @param scenarioID The Scenario to load.
 */
void Game_LoadScenario(uint8 houseID, uint16 scenarioID)
{
	Sound_Output_Feedback(0xFFFE);

	Game_Init();

	g_var_38BC++;

	if (!Scenario_Load(scenarioID, houseID)) {
		GUI_DisplayModalMessage("No more scenarios!", 0xFFFF);

		PrepareEnd();
		exit(0);
	}

	Game_Prepare();

	if (scenarioID < 5) {
		g_hintsShown1 = 0;
		g_hintsShown2 = 0;
	}

	g_var_38BC--;
}

/**
 * Close down facilities used by the program. Always called just before the
 *  program terminates.
 */
void PrepareEnd(void)
{
	free(g_palette_998A); g_palette_998A = NULL;

	GameLoop_Uninit();

	String_Uninit();
	Sprites_Uninit();
	Font_Uninit();
	Voice_UnloadVoices();

	Drivers_All_Uninit();

	if (g_mouseFileID != 0xFF) Mouse_SetMouseMode(INPUT_MOUSE_MODE_NORMAL, NULL);

	GFX_Uninit();
	Video_Uninit();
	A5_Uninit();
}
