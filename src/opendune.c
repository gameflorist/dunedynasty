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
#include "os/common.h"
#include "os/error.h"
#include "os/math.h"
#include "os/strings.h"
#include "os/sleep.h"

#include "opendune.h"

#include "ai.h"
#include "animation.h"
#include "audio/audio.h"
#include "common_a5.h"
#include "config.h"
#include "crashlog/crashlog.h"
#include "cutscene.h"
#include "enhancement.h"
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
#include "newui/actionpanel.h"
#include "newui/menu.h"
#include "newui/menubar.h"
#include "newui/viewport.h"
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


uint32 g_hintsShown1 = 0;          /*!< A bit-array to indicate which hints has been show already (0-31). */
uint32 g_hintsShown2 = 0;          /*!< A bit-array to indicate which hints has been show already (32-63). */
GameMode g_gameMode = GM_NORMAL;
enum GameOverlay g_gameOverlay;
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

#if 0
static uint8 s_enableLog = 0; /*!< 0 = off, 1 = record game, 2 = playback game (stored in 'dune.log'). */
#endif

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
static void Memory_ClearBlock(uint16 index);
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
		Audio_PlayMusic(MUSIC_STOP);
		Audio_PlayVoice(VOICE_STOP);

		Video_SetCursor(SHAPE_CURSOR_NORMAL);

		if (GameLoop_IsLevelWon()) {
			Audio_PlayVoice(VOICE_YOUR_MISSION_IS_COMPLETE);

			GUI_DisplayModalMessage(String_Get_ByIndex(STR_YOU_HAVE_SUCCESSFULLY_COMPLETED_YOUR_MISSION), 0xFFFF);
#if 0
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
#else
			g_gameMode = GM_WIN;
#endif
		} else {
			Audio_PlayVoice(VOICE_YOU_HAVE_FAILED_YOUR_MISSION);

			GUI_DisplayModalMessage(String_Get_ByIndex(STR_YOU_HAVE_FAILED_YOUR_MISSION), 0xFFFF);
#if 0
			GUI_Mentat_ShowLose();

			Sprites_UnloadTiles();

			g_scenarioID = GUI_StrategicMap_Show(g_campaignID, false);
#else
			g_gameMode = GM_LOSE;
#endif
		}

		GUI_ChangeSelectionType(SELECTIONTYPE_MENTAT);

		g_playerHouse->flags.doneFullScaleAttack = false;

#if 0
		Sprites_LoadTiles();

		g_gameMode = GM_RESTART;
#endif
		s_debugForceWin = false;
		return;
	}

	levelEndTimer = g_timerGame + 300;
}

#if 0
/* Moved to cutscene.c. */
static void Gameloop_Logos(void);
static void GameLoop_GameIntroAnimation(void);

/* Moved to gui/menu_opendune.c. */
static uint16 GameLoop_B4E6_0000(uint16 arg06, uint32 arg08, uint16 arg0C);
static void GameLoop_B4E6_0108(uint16 arg06, char **strings, uint32 arg0C, uint16 arg10, uint16 arg12);
static void GameLoop_DrawText2(char *string, uint16 left, uint16 top, uint8 fgColourNormal, uint8 fgColourSelected, uint8 bgColour);
static bool GameLoop_IsInRange(uint16 x, uint16 y, uint16 minX, uint16 minY, uint16 maxX, uint16 maxY);
static uint16 GameLoop_HandleEvents(uint16 arg06, char **strings, uint32 arg10, uint16 arg14);
#endif

static void Window_WidgetClick_Create(void)
{
	WidgetInfo *wi;

	while (g_widgetLinkedListHead != NULL) {
		Widget *w = g_widgetLinkedListHead;
		g_widgetLinkedListHead = w->next;

		free(w);
	}

	g_widgetLinkedListHead = NULL;

	for (wi = g_table_gameWidgetInfo; wi->index >= 0; wi++) {
		Widget *w;

		w = GUI_Widget_Allocate(wi->index, wi->shortcut, wi->offsetX, wi->offsetY, wi->spriteID, wi->stringID);
		w->div = wi->div;

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
#if 0
	static const uint16 mainMenuStrings[][6] = {
		{STR_PLAY_A_GAME, STR_REPLAY_INTRODUCTION, STR_EXIT_GAME, STR_NULL,         STR_NULL,         STR_NULL}, /* Neither HOF nor save. */
		{STR_PLAY_A_GAME, STR_REPLAY_INTRODUCTION, STR_LOAD_GAME, STR_EXIT_GAME,    STR_NULL,         STR_NULL}, /* Has a save game. */
		{STR_PLAY_A_GAME, STR_REPLAY_INTRODUCTION, STR_EXIT_GAME, STR_HALL_OF_FAME, STR_NULL,         STR_NULL}, /* Has a HOF. */
		{STR_PLAY_A_GAME, STR_REPLAY_INTRODUCTION, STR_LOAD_GAME, STR_EXIT_GAME,    STR_HALL_OF_FAME, STR_NULL}  /* Has a HOF and a save game. */
	};

	bool loc02 = false;

	Input_Flags_ClearBits(INPUT_FLAG_KEY_RELEASE | INPUT_FLAG_UNKNOWN_0400 | INPUT_FLAG_UNKNOWN_0100 |
	                      INPUT_FLAG_UNKNOWN_0080 | INPUT_FLAG_UNKNOWN_0040 | INPUT_FLAG_UNKNOWN_0020 |
	                      INPUT_FLAG_UNKNOWN_0008 | INPUT_FLAG_UNKNOWN_0004 | INPUT_FLAG_UNKNOWN_0002);
#endif

	Timer_SetTimer(TIMER_GUI, true);

	g_campaignID = 0;
	g_scenarioID = 1;
	g_playerHouseID = HOUSE_INVALID;
	g_debugScenario = false;
	g_selectionType = SELECTIONTYPE_MENTAT;
	g_selectionTypeNew = SELECTIONTYPE_MENTAT;

	memset(g_palette1, 0, 3 * 256);
	memset(g_palette2, 0, 3 * 256);

	g_readBufferSize = 0x2EE0;
	g_readBuffer = calloc(1, g_readBufferSize);

	ReadProfileIni("PROFILE.INI");

	free(g_readBuffer); g_readBuffer = NULL;

	File_ReadBlockFile("IBM.PAL", g_palette_998A, 256 * 3);

	memmove(g_palette1, g_palette_998A, 256 * 3);

	GUI_ClearScreen(0);

	GFX_SetPalette(g_palette1);
	GFX_SetPalette(g_palette2);

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

#if 0
	while (g_mouseHiddenDepth > 1) {
		GUI_Mouse_Show_Safe();
	}
#endif

	Window_WidgetClick_Create();
	Unit_Init();
	UnitAI_ClearSquads();
	Team_Init();
	House_Init();
	Structure_Init();

#if 0
	if (!g_debugSkipDialogs) {
		uint16 stringID;
		uint16 maxWidth;
		bool hasSave;
		bool hasFame;
		bool loc06 = true;
		bool loc10;

		loc10 = true;

		hasSave = File_Exists_Personal("_SAVE000.DAT");
		hasFame = File_Exists_Personal("SAVEFAME.DAT");

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

					hasFame = File_Exists_Personal("SAVEFAME.DAT");
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

				g_widgetProperties[21].width  = maxWidth;
				g_widgetProperties[13].width  = g_widgetProperties[21].width + 2*8;
				g_widgetProperties[13].xBase  = 19*8 - maxWidth;
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
		Audio_PlayMusic(MUSIC_STOP);

		free(g_readBuffer);
		g_readBufferSize = (g_enableVoices == 0) ? 0x2EE0 : 0x4E20;
		g_readBuffer = calloc(1, g_readBufferSize);
	}

	GUI_DrawFilledRectangle(g_curWidgetXBase, g_curWidgetYBase, g_curWidgetXBase + g_curWidgetWidth, g_curWidgetYBase + g_curWidgetHeight, 12);

	if (!loc02) {
		Audio_LoadSampleSet(HOUSE_MERCENARY);

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
#else
	{
		Audio_PlayMusic(MUSIC_STOP);

		free(g_readBuffer);
		g_readBufferSize = 0x6D60;
		g_readBuffer = calloc(1, g_readBufferSize);

		Menu_Run();
	}
#endif

	GFX_SetPalette(g_palette1);

	return;
}

static void InGame_Numpad_Move(uint16 key)
{
	if (key == 0) return;

	switch (key) {
#if 0
		case SCANCODE_TAB: /* TAB, SHIFT TAB */
			Map_SelectNext(true);
			Map_SelectNext(false);
			return;
#endif

		case SCANCODE_KEYPAD_8: /* NUMPAD 8 / ARROW UP */
			Map_MoveDirection( 0 * g_gameConfig.scrollSpeed, -1 * g_gameConfig.scrollSpeed);
			return;

		case SCANCODE_KEYPAD_9: /* NUMPAD 9 / PAGE UP */
			Map_MoveDirection( 1 * g_gameConfig.scrollSpeed, -1 * g_gameConfig.scrollSpeed);
			return;

		case SCANCODE_KEYPAD_6: /* NUMPAD 6 / ARROW RIGHT */
			Map_MoveDirection( 1 * g_gameConfig.scrollSpeed,  0 * g_gameConfig.scrollSpeed);
			return;

		case SCANCODE_KEYPAD_3: /* NUMPAD 3 / PAGE DOWN */
			Map_MoveDirection( 1 * g_gameConfig.scrollSpeed,  1 * g_gameConfig.scrollSpeed);
			return;

		case SCANCODE_KEYPAD_2: /* NUMPAD 2 / ARROW DOWN */
			Map_MoveDirection( 0 * g_gameConfig.scrollSpeed,  1 * g_gameConfig.scrollSpeed);
			return;

		case SCANCODE_KEYPAD_1: /* NUMPAD 1 / END */
			Map_MoveDirection(-1 * g_gameConfig.scrollSpeed,  1 * g_gameConfig.scrollSpeed);
			return;

		case SCANCODE_KEYPAD_4: /* NUMPAD 4 / ARROW LEFT */
			Map_MoveDirection(-1 * g_gameConfig.scrollSpeed,  0 * g_gameConfig.scrollSpeed);
			return;

		case SCANCODE_KEYPAD_7: /* NUMPAD 7 / HOME */
			Map_MoveDirection(-1 * g_gameConfig.scrollSpeed, -1 * g_gameConfig.scrollSpeed);
			return;

		case SCANCODE_1:
		case SCANCODE_2:
		case SCANCODE_3:
		case SCANCODE_4:
		case SCANCODE_5:
		case SCANCODE_6:
		case SCANCODE_7:
		case SCANCODE_8:
		case SCANCODE_9:
		case SCANCODE_0:
			Viewport_Hotkey(key - SCANCODE_1 + SQUADID_1);
			break;

		case SCANCODE_H:
			Viewport_Homekey();
			break;

		case SCANCODE_F5:
			Audio_DisplayMusicName();
			break;

		case SCANCODE_F6:
		case SCANCODE_F7:
			{
				const bool increase = (key == SCANCODE_F7);
				const bool adjust_current_track_only = (Input_Test(SCANCODE_LSHIFT) || Input_Test(SCANCODE_RSHIFT));

				Audio_AdjustMusicVolume(increase, adjust_current_track_only);
				Audio_DisplayMusicName();
			}
			break;

		case SCANCODE_OPENBRACE:
		case SCANCODE_CLOSEBRACE:
			{
				enum ScreenDivID divID = (key == SCANCODE_OPENBRACE) ? SCREENDIV_MENUBAR : SCREENDIV_SIDEBAR;
				ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];
				ScreenDiv *div = &g_screenDiv[divID];
				const int oldh = viewport->height;

				div->scale = (div->scale >= 1.5f) ? 1.0f : 2.0f;
				A5_InitTransform(false);
				GameLoop_TweakWidgetDimensions();
				g_factoryWindowTotal = -1;
				Map_MoveDirection(0, oldh - viewport->height);
			}
			break;

		case SCANCODE_TILDE:
			enhancement_draw_health_bars = !enhancement_draw_health_bars;
			break;

		case SCANCODE_MINUS:
		case SCANCODE_EQUALS:
			{
				const float scaling_factor[] = { 1.0f, 1.5f, 2.0f, 3.0f };
				ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];

				int curr;
				for (curr = 0; curr < (int)lengthof(scaling_factor); curr++) {
					if (viewport->scale <= scaling_factor[curr])
						break;
				}

				int new_scale = curr + (key == SCANCODE_EQUALS ? 1 : -1);
				new_scale = clamp(0, new_scale, (int)lengthof(scaling_factor) - 1);

				if (new_scale != curr) {
					const int tilex = Tile_GetPackedX(g_viewportPosition);
					const int tiley = Tile_GetPackedY(g_viewportPosition);
					const int viewport_cx = TILE_SIZE * tilex + g_viewport_scrollOffsetX + viewport->width / 2;
					const int viewport_cy = TILE_SIZE * tiley + g_viewport_scrollOffsetY + viewport->height / 2;

					viewport->scale = scaling_factor[new_scale];
					A5_InitTransform(false);
					GameLoop_TweakWidgetDimensions();
					Map_CentreViewport(viewport_cx, viewport_cy);
				}
			}
			break;

#if 0
		/* Debugging. */
		case SCANCODE_F9:
			Tile_RemoveFogInRadius(Tile_UnpackTile(Tile_PackXY(32,32)), 64);
			break;

		case SCANCODE_F10:
			s_debugForceWin = true;
			break;
#endif

		default: return;
	}
}

void
GameLoop_TweakWidgetDimensions(void)
{
	const ScreenDiv *sidebar = &g_screenDiv[SCREENDIV_SIDEBAR];
	const ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];

	g_table_gameWidgetInfo[GAME_WIDGET_BUILD_PLACE].height = sidebar->height - g_table_gameWidgetInfo[GAME_WIDGET_BUILD_PLACE].offsetY - (14 + g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].height);

	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].offsetX = 0;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].offsetY = 0;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].width = TRUE_DISPLAY_WIDTH;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].height = 5;

	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].offsetX = TRUE_DISPLAY_WIDTH - 5;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].offsetY = 0;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].width = 5;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].height = TRUE_DISPLAY_HEIGHT;

	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].offsetX = 0;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].offsetY = 0;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].width = 5;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].height = TRUE_DISPLAY_HEIGHT;

	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].offsetX = 0;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].offsetY = TRUE_DISPLAY_HEIGHT - 5;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].width = TRUE_DISPLAY_WIDTH;
	g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].height = 5;

	g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].offsetY = sidebar->height - g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].height;

	g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT].offsetY = 0;
	g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT].width = viewport->width;
	g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT].height = viewport->height;

	g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT_FALLBACK].width = TRUE_DISPLAY_WIDTH;
	g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT_FALLBACK].height = TRUE_DISPLAY_HEIGHT;

	/* gui/widget.c */
	g_widgetProperties[WINDOWID_VIEWPORT].yBase = g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT].offsetY;
	g_widgetProperties[WINDOWID_MINIMAP].xBase = g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].offsetX;
	g_widgetProperties[WINDOWID_MINIMAP].yBase = g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].offsetY;
	g_widgetProperties[WINDOWID_ACTIONPANEL_FRAME].xBase = 16;
	g_widgetProperties[WINDOWID_ACTIONPANEL_FRAME].yBase = 2;
	g_widgetProperties[WINDOWID_ACTIONPANEL_FRAME].height = sidebar->height - (2 + 12 + g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].height);

	Window_WidgetClick_Create();
}

/**
 * Main game loop.
 */
void GameLoop_Main(bool new_game)
{
	static int64_t l_timerNext = 0;
	static int64_t l_timerUnitStatus = 0;
	static int16  l_selectionState = -2;

	uint16 key;

	Sprites_UnloadTiles();
	Sprites_LoadTiles();

	GUI_Palette_CreateRemap(g_playerHouseID);
	Audio_LoadSampleSet(g_playerHouseID);

	if (new_game)
		Game_LoadScenario(g_playerHouseID, g_scenarioID);

	GUI_ChangeSelectionType(g_debugScenario ? SELECTIONTYPE_DEBUG : SELECTIONTYPE_STRUCTURE);

	Timer_SetTimer(TIMER_GAME, true);

	/* Note: original game chose only MUSIC_IDLE1 .. MUSIC_IDLE6. */
	Audio_PlayMusic(MUSIC_IDLE1);

	g_gameMode = GM_NORMAL;
	g_gameOverlay = GAMEOVERLAY_NONE;
	Timer_RegisterSource();

	int frames_skipped = 0;
	while (g_gameMode == GM_NORMAL) {
		Timer_WaitForEvent();
		const int64_t curr_ticks = Timer_GameTicks();

		if (g_gameOverlay == GAMEOVERLAY_NONE && g_timerGame != curr_ticks) {
			g_timerGame = curr_ticks;
		}
		else if (g_gameOverlay != GAMEOVERLAY_NONE) {
		}
		else {
			continue;
		}

#if 0
		if (g_gameMode == GM_PICKHOUSE) {
			Music_Play(28);

			g_playerHouseID = HOUSE_MERCENARY;
			g_playerHouseID = GUI_PickHouse();

			Memory_ClearBlock(1);

			Sprites_LoadTiles();

			GUI_Palette_CreateRemap(g_playerHouseID);

			Voice_LoadVoices(g_playerHouseID);

			g_gameMode = GM_RESTART;
			g_scenarioID = 1;
			g_campaignID = 0;
			g_strategicRegionBits = 0;
		}
#endif

		if (g_selectionTypeNew != g_selectionType) {
			GUI_ChangeSelectionType(g_selectionTypeNew);
		}

		GUI_PaletteAnimate();

#if 0
		if (g_gameMode == GM_RESTART) {
			GUI_ChangeSelectionType(SELECTIONTYPE_MENTAT);

			Game_LoadScenario(g_playerHouseID, g_scenarioID);
			if (!g_debugScenario && !g_debugSkipDialogs) GUI_Mentat_ShowBriefing();

			g_gameMode = GM_NORMAL;

			GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);

			Music_Play(Tools_RandomRange(0, 8) + 8);
			l_timerNext = Timer_GetTicks() + 300;
		}
#endif

		if (l_selectionState != g_selectionState) {
			Map_SetSelectionObjectPosition(0xFFFF);
			Map_SetSelectionObjectPosition(g_selectionRectanglePosition);
			l_selectionState = g_selectionState;
		}

		const bool narrator_speaking = Audio_Poll();
		if (!narrator_speaking) {
			if (!g_enable_audio || !g_enable_music) {
				g_musicInBattle = 0;
			} else if (g_musicInBattle > 0) {
				Audio_PlayMusic(MUSIC_ATTACK1);
				l_timerNext = Timer_GetTicks() + 300;
				g_musicInBattle = -1;
			} else {
				if (Timer_GetTicks() > l_timerNext) {
					if (!Audio_MusicIsPlaying()) {
						Audio_PlayMusic(MUSIC_IDLE1);
						l_timerNext = Timer_GetTicks() + 300;
						g_musicInBattle = 0;
					}
				}
			}
		}

		GFX_Screen_SetActive(0);

		if (g_gameOverlay == GAMEOVERLAY_NONE) {
			key = GUI_Widget_HandleEvents(g_widgetLinkedListHead);
		}
		else {
			key = 0;
		}

		Input_History_Clear();

		if (g_selectionType == SELECTIONTYPE_TARGET || g_selectionType == SELECTIONTYPE_PLACE || g_selectionType == SELECTIONTYPE_UNIT || g_selectionType == SELECTIONTYPE_STRUCTURE) {
			if (Unit_AnySelected()) {
				if (l_timerUnitStatus < g_timerGame) {
					Unit_DisplayGroupStatusText();
					l_timerUnitStatus = g_timerGame + 300;
				}

				if (g_selectionType != SELECTIONTYPE_TARGET) {
					const Unit *u = Unit_FirstSelected(NULL);
					g_selectionPosition = Tile_PackTile(Tile_Center(u->o.position));
				}
			}

			InGame_Numpad_Move(key);

			UnitAI_SquadLoop();
			GameLoop_Team();
			GameLoop_Unit();
			GameLoop_Structure();
			GameLoop_House();
		}

		if (g_var_38F8 && !g_debugScenario) {
			GameLoop_LevelEnd();
		}

		if (g_gameOverlay == GAMEOVERLAY_NONE) {
			Input_Tick(false);
		}
		else if (g_gameOverlay == GAMEOVERLAY_MENTAT) {
			Input_Tick(true);
			MenuBar_TickMentatOverlay();
		}
		else {
			Input_Tick(true);
			MenuBar_TickOptionsOverlay();
		}

		if (!g_var_38F8) break;

		if (frames_skipped > 4 || Timer_QueueIsEmpty()) {
			frames_skipped = 0;

			GUI_DrawInterfaceAndRadar();

			if (g_gameOverlay == GAMEOVERLAY_NONE) {
			}
			else if (g_gameOverlay == GAMEOVERLAY_MENTAT) {
				MenuBar_DrawMentatOverlay();
			}
			else {
				MenuBar_DrawOptionsOverlay();
			}

			Video_Tick();
		}
		else {
			frames_skipped++;
		}
	}

	Timer_UnregisterSource();

#if 0
	if (s_enableLog != 0) Mouse_SetMouseMode(INPUT_MOUSE_MODE_NORMAL, "DUNE.LOG");
#endif

	Widget_SetCurrentWidget(0);

	GFX_Screen_SetActive(2);

	GFX_ClearScreen();

	GUI_Screen_FadeIn(g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetWidth/8, g_curWidgetHeight, 2, 0);
}

static bool Unknown_25C4_000E(void)
{
	if (!Video_Init()) return false;

	/* g_var_7097 = -1; */

	GFX_Init();
	GFX_ClearScreen();

	if (!Font_Init()) {
		Error(
			"--------------------------\n"
			"ERROR LOADING DATA FILE\n"
			"\n"
			"Did you copy the Dune2 1.07eu data files into the data directory\n"
			"%s/data ?\n"
			"\n",
			g_dune_data_dir
		);

		return false;
	}

	Font_Select(g_fontNew8p);

	memset(g_palette_998A, 0, 3 * 256);

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
	VARIABLE_NOT_USED(argc);
	VARIABLE_NOT_USED(argv);

	CrashLog_Init();
	Mouse_Init();

	if (A5_InitOptions() == false)
		exit(1);

#if defined(_WIN32)
	#if defined(__MINGW32__) && defined(__STRICT_ANSI__)
		int __cdecl __MINGW_NOTHROW _fileno (FILE*);
	#endif

	char filename[1024];

	snprintf(filename, sizeof(filename), "%s/error.log", g_personal_data_dir);
	FILE *err = fopen(filename, "w");

	snprintf(filename, sizeof(filename), "%s/output.log", g_personal_data_dir);
	FILE *out = fopen(filename, "w");

	#if defined(_MSC_VER)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	if (err != NULL) _dup2(_fileno(err), _fileno(stderr));
	if (out != NULL) _dup2(_fileno(out), _fileno(stdout));
	FreeConsole();
#endif

	if (!Unknown_25C4_000E()) exit(1);

	if (A5_Init() == false)
		exit(1);

	Input_Init();

	/* g_var_7097 = 0; */

	Audio_ScanMusic();
	String_Init();
	Sprites_Init();
	Sprites_LoadTiles();
	VideoA5_InitSprites();
	GameLoop_TweakWidgetDimensions();
	Audio_PlayVoice(VOICE_STOP);
	GameLoop_GameIntroAnimationMenu();

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

	Audio_LoadSampleSet(g_playerHouseID);

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
	UnitAI_ClearSquads();
	Team_Init();
	House_Init();

	Animation_Init();
	Explosion_Init();
	memset(g_map, 0, 64 * 64 * sizeof(Tile));

	memset(g_mapSpriteID, 0, 64 * 64 * sizeof(uint16));
	memset(g_starportAvailable, 0, sizeof(g_starportAvailable));

	Audio_PlayVoice(VOICE_STOP);

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
	Audio_PlayVoice(VOICE_STOP);

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
	Animation_Uninit();
	Explosion_Uninit();

	GameLoop_Uninit();

	String_Uninit();
	Sprites_Uninit();
	Font_Uninit();

#if 0
	if (g_mouseFileID != 0xFF) Mouse_SetMouseMode(INPUT_MOUSE_MODE_NORMAL, NULL);
#endif

	GFX_Uninit();
	Video_Uninit();
	A5_Uninit();
}
