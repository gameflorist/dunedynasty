/** @file src/opendune.c Gameloop and other main routines. */

#ifdef __APPLE__
/* We need Allegro to mangle main, and Allegro can't define the
 * _al_mangled_main prototype for us.
 */
#include <allegro5/allegro.h>

extern int _al_mangled_main(int argc, char **argv);
#endif

#if defined(_WIN32)
	#if defined(_MSC_VER)
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif /* _MSC_VER */
	#include <io.h>
	#include <windows.h>
#endif /* _WIN32 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "enum_string.h"
#include "os/common.h"
#include "os/error.h"
#include "os/math.h"
#include "os/strings.h"

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
#include "gameloop.h"
#include "gfx.h"
#include "gui/font.h"
#include "gui/gui.h"
#include "gui/mentat.h"
#include "gui/widget.h"
#include "house.h"
#include "ini.h"
#include "input/input.h"
#include "input/mouse.h"
#include "map.h"
#include "mods/multiplayer.h"
#include "net/net.h"
#include "net/server.h"
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
#include "table/widgetinfo.h"
#include "team.h"
#include "timer/timer.h"
#include "tools/coord.h"
#include "tools/random_lcg.h"
#include "tools/random_xorshift.h"
#include "unit.h"
#include "video/video.h"


uint32 g_hintsShown1 = 0;          /*!< A bit-array to indicate which hints has been show already (0-31). */
uint32 g_hintsShown2 = 0;          /*!< A bit-array to indicate which hints has been show already (32-63). */
bool   g_inGame;
enum GameMode g_gameMode = GM_NORMAL;
enum GameOverlay g_gameOverlay;
uint16 g_campaignID = 0;
uint16 g_scenarioID = 1;
uint16 g_activeAction = 0xFFFF;      /*!< Action the controlled unit will do. */

bool   g_debugGame = false;        /*!< When true, you can control the AI. */
bool   g_debugScenario = false;    /*!< When true, you can review the scenario. There is no fog. The game is not running (no unit-movement, no structure-building, etc). You can click on individual tiles. */

void *g_readBuffer = NULL;
uint32 g_readBufferSize = 0;

static bool  s_debugForceWin = false; /*!< When true, you immediately win the level. */

uint16 g_validateStrictIfZero = 0; /*!< 0 = strict validation, basically: no-cheat-mode. */
uint16 g_selectionType = 0;
uint16 g_selectionTypeNew = 0;

int16 g_musicInBattle = 0; /*!< 0 = no battle, 1 = fight is going on, -1 = music of fight is going on is active. */

static enum GameMode
GameLoop_Server_IsHouseFinished(enum HouseType houseID)
{
	const House *h = House_Get_ByIndex(houseID);
	bool finish = false;
	bool win = false;

	if (s_debugForceWin)
		return GM_WIN;

	if (g_campaign_selected == CAMPAIGNID_MULTIPLAYER
			&& g_multiplayer.state[houseID] != MP_HOUSE_PLAYING)
		return GM_NORMAL;

	/* Check structures remaining. */
	if ((g_scenario.winFlags & 0x3) || (g_scenario.loseFlags & 0x3)) {
		PoolFindStruct find;
		bool foundFriendly = false;
		bool foundEnemy = false;

		find.houseID = HOUSE_INVALID;
		find.type    = 0xFFFF;
		find.index   = 0xFFFF;

		/* Calculate how many structures are left on the map */
		while (true) {
			if (foundFriendly && foundEnemy)
				break;

			const Structure *s = Structure_Find(&find);
			if (s == NULL) break;

			if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL) continue;
			if (s->o.type == STRUCTURE_TURRET) continue;
			if (s->o.type == STRUCTURE_ROCKET_TURRET) continue;

			if (g_campaign_selected == CAMPAIGNID_MULTIPLAYER
					&& (g_multiplayer.state[s->o.houseID] == MP_HOUSE_UNUSED
					 || g_multiplayer.state[s->o.houseID] == MP_HOUSE_LOST)) {
				continue;
			}

			if (House_AreAllied(s->o.houseID, houseID)) {
				foundFriendly = true;
			}
			else {
				foundEnemy = true;
			}
		}

		if (g_campaign_selected == CAMPAIGNID_SKIRMISH
		 || g_campaign_selected == CAMPAIGNID_MULTIPLAYER) {
			find.houseID = HOUSE_INVALID;
			find.type    = UNIT_MCV;
			find.index   = 0xFFFF;

			while (true) {
				if (foundFriendly && foundEnemy)
					break;

				const Unit *u = Unit_Find(&find);
				if (u == NULL) break;

				const enum HouseType h2 = Unit_GetHouseID(u);
				if (g_campaign_selected == CAMPAIGNID_MULTIPLAYER
						&& (g_multiplayer.state[h2] == MP_HOUSE_UNUSED
						 || g_multiplayer.state[h2] == MP_HOUSE_LOST)) {
					continue;
				}

				if (House_AreAllied(h2, houseID)) {
					foundFriendly = true;
				}
				else {
					foundEnemy = true;
				}
			}
		}

		if (g_scenario.winFlags & 0x3) {
			if ((g_scenario.winFlags & 0x1) && !foundEnemy)
				finish = true;
			if ((g_scenario.winFlags & 0x2) && !foundFriendly)
				finish = true;
		}

		if (g_scenario.loseFlags & 0x3) {
			win = true;
			if (g_scenario.loseFlags & 0x1)
				win = win && (!foundEnemy);
			if (g_scenario.loseFlags & 0x2)
				win = win && foundFriendly;
		}
	}

	/* Check spice quota. */
	if ((g_scenario.winFlags & 0x4) || (g_scenario.loseFlags & 0x4)) {
		bool reached_quota;

		/* SINGLE PLAYER -- Wait until the counter ticks over the quota. */
		if (g_host_type == HOSTTYPE_NONE) {
			reached_quota
				= (g_playerCredits != 0xFFFF)
				&& (g_playerCredits >= g_playerHouse->creditsQuota);
		}
		else {
			reached_quota
				= (h->credits != 0xFFFF)
				&& (h->credits >= h->creditsQuota);
		}

		if (reached_quota) {
			if (g_scenario.winFlags & 0x4)
				finish = true;
			if (g_scenario.loseFlags & 0x4)
				win = true;
		}
	}

	/* Check scenario timeout.
	 *
	 * ENHANCEMENT -- This code originally had '<' instead of '>=',
	 * which makes no sense.  At least this way is sensible, allowing
	 * win-after-timeout (survival) and lose-after-timeout (countdown)
	 * missions.
	 *
	 * survival: winFlags = 11, loseFlags = 9.
	 * lose:     winFlags = 11, loseFlags = 1.
	 */
	if ((g_scenario.winFlags & 0x8) || (g_scenario.loseFlags & 0x8)) {
		if (g_timerGame - g_tickScenarioStart >= g_scenario.timeOut) {
			if (g_scenario.winFlags & 0x8)
				finish = true;
			if (g_scenario.loseFlags & 0x8)
				win = true;
		}
	}

	if (finish) {
		return win ? GM_WIN : GM_LOSE;
	}
	else {
		return GM_NORMAL;
	}
}

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

/**
 * Checks if the level comes to an end. If so, it shows all end-level stuff,
 *  and prepares for the next level.
 */
void GameLoop_LevelEnd(void)
{
	static int64_t l_levelEndTimer = 0;

	if (l_levelEndTimer >= g_timerGame && !s_debugForceWin)
		return;

	/* You have to play at least 7200 ticks before you can win the game */
	if (!s_debugForceWin
			&& g_timerGame - g_tickScenarioStart < 7200
			&& g_campaign_selected != CAMPAIGNID_SKIRMISH
			&& g_campaign_selected != CAMPAIGNID_MULTIPLAYER)
		return;

	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		if (!House_IsHuman(houseID))
			continue;

		enum GameMode gm = GameLoop_Server_IsHouseFinished(houseID);
		if (gm != GM_NORMAL) {
			House *h = House_Get_ByIndex(houseID);

			Server_Send_WinLose(houseID, (gm == GM_WIN));

			h->flags.doneFullScaleAttack = false;
			s_debugForceWin = false;
		}
	}

	l_levelEndTimer = g_timerGame + 300;
}

#if 0
static void GameLoop_DrawMenu(const char **strings);
static void GameLoop_DrawText2(const char *string, uint16 left, uint16 top, uint8 fgColourNormal, uint8 fgColourSelected, uint8 bgColour);
static bool GameLoop_IsInRange(uint16 x, uint16 y, uint16 minX, uint16 minY, uint16 maxX, uint16 maxY);
static uint16 GameLoop_HandleEvents(const char **strings);
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
		w->flags.requiresClick = (wi->flags & 0x0001) ? true : false;
		w->flags.notused1 = (wi->flags & 0x0002) ? true : false;
		w->flags.clickAsHover = (wi->flags & 0x0004) ? true : false;
		w->flags.invisible = (wi->flags & 0x0008) ? true : false;
		w->flags.greyWhenInvisible = (wi->flags & 0x0010) ? true : false;
		w->flags.noClickCascade = (wi->flags & 0x0020) ? true : false;
		w->flags.loseSelect = (wi->flags & 0x0040) ? true : false;
		w->flags.notused2 = (wi->flags & 0x0080) ? true : false;
		w->flags.buttonFilterLeft = (wi->flags >> 8) & 0x0f;
		w->flags.buttonFilterRight = (wi->flags >> 12) & 0x0f;

		g_widgetLinkedListHead = GUI_Widget_Insert(g_widgetLinkedListHead, w);
	}
}

#if 0
/* Moved to scenario.c. */
static void ReadProfileIni(void);
#endif

void
GameLoop_TweakWidgetDimensions(void)
{
	const ScreenDiv *menubar = &g_screenDiv[SCREENDIV_MENUBAR];
	const ScreenDiv *sidebar = &g_screenDiv[SCREENDIV_SIDEBAR];
	const ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];

	/* 20% taller buttons to simulate 20% taller buttons on CRTs. */
	if ((g_aspect_correction == ASPECT_RATIO_CORRECTION_PARTIAL || g_aspect_correction == ASPECT_RATIO_CORRECTION_AUTO) &&
			(110 - 40 + 6 + 12 < sidebar->height - 16 - 64)) {
		g_table_gameWidgetInfo[GAME_WIDGET_PICTURE].height = 23;
		g_table_gameWidgetInfo[GAME_WIDGET_BUILD_PLACE].offsetY = 87 - 40 + 2 + 3;
		g_table_gameWidgetInfo[GAME_WIDGET_BUILD_PLACE].height = sidebar->height - g_table_gameWidgetInfo[GAME_WIDGET_BUILD_PLACE].offsetY - (14 + g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].height) - 1;
		g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].offsetY = 76 - 40 + 1;
		g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].height = 12;
		g_table_gameWidgetInfo[GAME_WIDGET_CANCEL].height = 12;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_1].offsetY = 77 - 40;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_1].height = 12;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_2].offsetY = 88 - 40 + 2;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_2].height = 12;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_3].offsetY = 99 - 40 + 4;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_3].height = 12;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_4].offsetY = 110 - 40 + 6;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_4].height = 12;
	}
	else {
		g_table_gameWidgetInfo[GAME_WIDGET_PICTURE].height = 24;
		g_table_gameWidgetInfo[GAME_WIDGET_BUILD_PLACE].offsetY = 87 - 40 + 2;
		g_table_gameWidgetInfo[GAME_WIDGET_BUILD_PLACE].height = sidebar->height - g_table_gameWidgetInfo[GAME_WIDGET_BUILD_PLACE].offsetY - (14 + g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP].height) - 1;
		g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].offsetY = 76 - 40;
		g_table_gameWidgetInfo[GAME_WIDGET_REPAIR_UPGRADE].height = 10;
		g_table_gameWidgetInfo[GAME_WIDGET_CANCEL].height = 10;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_1].offsetY = 77 - 40;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_1].height = 10;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_2].offsetY = 88 - 40;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_2].height = 10;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_3].offsetY = 99 - 40;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_3].height = 10;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_4].offsetY = 110 - 40;
		g_table_gameWidgetInfo[GAME_WIDGET_UNIT_COMMAND_4].height = 10;
	}

	if (g_gameConfig.scrollAlongScreenEdge) {
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].width = TRUE_DISPLAY_WIDTH;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].height = 5;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].offsetX = 0;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].offsetY = 0;

		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].width = 5;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].height = TRUE_DISPLAY_HEIGHT;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].offsetX = TRUE_DISPLAY_WIDTH - g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].width;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].offsetY = 0;

		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].width = 5;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].height = TRUE_DISPLAY_HEIGHT;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].offsetX = 0;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].offsetY = 0;

		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].width = TRUE_DISPLAY_WIDTH;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].height = 5;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].offsetX = 0;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].offsetY = TRUE_DISPLAY_HEIGHT - g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].height;
	}
	else {
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].width = viewport->scalex * viewport->width;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].height = menubar->scaley * 16;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].offsetX = 0;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].offsetY = viewport->y - g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_UP].height;

		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].width = sidebar->scalex * 10;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].height = viewport->scaley * viewport->height;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].offsetX = viewport->scalex * viewport->width;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_RIGHT].offsetY = viewport->y;

		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].width = 5; /* 2px is a little hard when in a window. */
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].height = viewport->scaley * viewport->height;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].offsetX = 0;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_LEFT].offsetY = viewport->y;

		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].width = viewport->scalex * viewport->width;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].height = 5; /* 2px is a little hard when in a window. */
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].offsetX = 0;
		g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].offsetY = TRUE_DISPLAY_HEIGHT - g_table_gameWidgetInfo[GAME_WIDGET_SCROLL_DOWN].height;
	}

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
	g_factoryWindowTotal = -1;

	Window_WidgetClick_Create();
}

/**
 * Intro menu.
 */
static void GameLoop_GameIntroAnimationMenu(void)
{
	Timer_SetTimer(TIMER_GUI, true);

	g_campaignID = 0;
	g_scenarioID = 1;
	g_playerHouseID = HOUSE_INVALID;
	g_debugScenario = false;
	g_selectionType = SELECTIONTYPE_MENTAT;
	g_selectionTypeNew = SELECTIONTYPE_MENTAT;

	memset(g_palette1, 0, 3 * 256);
	memset(g_palette2, 0, 3 * 256);

	g_readBufferSize = 12000;
	g_readBuffer = NULL;

	File_ReadBlockFile("IBM.PAL", g_palette_998A, 256 * 3);

	memmove(g_palette1, g_palette_998A, 256 * 3);

	GUI_ClearScreen(SCREEN_0);

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

	Window_WidgetClick_Create();
	Unit_Init();
	UnitAI_ClearSquads();
	Team_Init();
	House_Init();
	Structure_Init();

	{
		Audio_PlayMusic(MUSIC_STOP);

		free(g_readBuffer);
		g_readBufferSize = 0x6D60;
		g_readBuffer = calloc(1, g_readBufferSize);

		Menu_Run();
	}

	GFX_SetPalette(g_palette1);
}

/**
 * Main game loop.
 */
void GameLoop_Main(bool new_game)
{
	assert(g_playerHouseID != HOUSE_INVALID);

	Mouse_TransformFromDiv(SCREENDIV_MENU, &g_mouseX, &g_mouseY);

	Sprites_UnloadTiles();
	Sprites_LoadTiles();
	Viewport_Init();

	GUI_Palette_CreateRemap(g_playerHouseID);
	Audio_LoadSampleSet(g_table_houseInfo[g_playerHouseID].sampleSet);

	if (new_game) {
		Game_LoadScenario(g_playerHouseID, g_scenarioID);
		GUI_ChangeSelectionType(g_debugScenario ? SELECTIONTYPE_DEBUG : SELECTIONTYPE_STRUCTURE);
	}

	Timer_ResetScriptTimers();

	/* Note: original game chose only MUSIC_IDLE1 .. MUSIC_IDLE6. */
	Audio_PlayMusic(MUSIC_STOP);
	g_musicInBattle = 0;

	g_gameMode = GM_NORMAL;
	g_gameOverlay = GAMEOVERLAY_NONE;

	Net_Synchronise();
	Timer_SetTimer(TIMER_GAME, true);
	Timer_RegisterSource();

	GameLoop_Loop();

	Timer_UnregisterSource();

	Audio_PlayVoice(VOICE_STOP);
	Widget_SetCurrentWidget(0);
	g_selectionPosition = 0xFFFF;
	Unit_UnselectAll();

#if 0
	/* XXX: This fading effect doesn't work. */
	GFX_Screen_SetActive(SCREEN_1);
	GFX_ClearScreen();
	GUI_Screen_FadeIn(g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetWidth/8, g_curWidgetHeight, SCREEN_1, SCREEN_0);
#endif

	Video_SetCursor(SHAPE_CURSOR_NORMAL);
	if (g_gameOverlay == GAMEOVERLAY_NONE)
		Mouse_TransformToDiv(SCREENDIV_MENU, &g_mouseX, &g_mouseY);
}

static bool Unknown_25C4_000E(void)
{
	memcpy(g_table_houseInfo, g_table_houseInfo_original, sizeof(g_table_houseInfo_original));
	memcpy(g_table_structureInfo, g_table_structureInfo_original, sizeof(g_table_structureInfo_original));
	memcpy(g_table_unitInfo, g_table_unitInfo_original, sizeof(g_table_unitInfo_original));

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
	Tools_RandomLCG_Seed((unsigned)time(NULL));
	Random_Xorshift_Seed(rand(), rand(), rand(), rand());

	Widget_SetCurrentWidget(0);

	return true;
}

int main(int argc, char **argv)
{
	VARIABLE_NOT_USED(argc);
	VARIABLE_NOT_USED(argv);

	CrashLog_Init();
	FileHash_Init();
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
	Audio_LoadSampleSet(SAMPLESET_INVALID);
	String_Init();

	Campaign *camp;

	/* Create the Dune 2 campaign: CAMPAIGNID_DUNE_II. */
	camp = Campaign_Alloc(NULL);
	camp->house[0] = HOUSE_ATREIDES;
	camp->house[1] = HOUSE_ORDOS;
	camp->house[2] = HOUSE_HARKONNEN;
	camp->intermission = true;
	snprintf(camp->name, sizeof(camp->name), "%s", String_Get_ByIndex(STR_THE_BATTLE_FOR_ARRAKIS));

	/* Create the skirmish campaign: CAMPAIGNID_SKIRMISH. */
	camp = Campaign_Alloc("skirmish");
	snprintf(camp->name, sizeof(camp->name), "Skirmish");

	/* Create the multiplayer campaign: CAMPAIGNID_MULTIPLAYER. */
	camp = Campaign_Alloc("multiplayer");
	snprintf(camp->name, sizeof(camp->name), "Multiplayer");

	Sprites_Init();
	Sprites_LoadTiles();
	VideoA5_InitSprites();
	GameLoop_TweakWidgetDimensions();
	Audio_PlayVoice(VOICE_STOP);

	Net_Initialise();

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

	g_validateStrictIfZero++;

	oldSelectionType = g_selectionType;
	g_selectionType = SELECTIONTYPE_MENTAT;

	Structure_Recount();
	Unit_Recount();
	Team_Recount();

	for (uint16 packed = 0; packed < MAP_SIZE_MAX * MAP_SIZE_MAX; packed++) {
		const Structure *s = Structure_Get_ByPackedTile(packed);
		const Unit *u = Unit_Get_ByPackedTile(packed);
		Tile *t = &g_map[packed];
		FogOfWarTile *f = &g_mapVisible[packed];

		if (u == NULL || !u->o.flags.s.used) t->hasUnit = false;
		if (s == NULL || !s->o.flags.s.used) t->hasStructure = false;

		if (Map_IsUnveiledToHouse(g_playerHouseID, packed)) {
			const int64_t backup = f->timeout;

			Map_UnveilTile(g_playerHouseID, UNVEILCAUSE_INITIALISATION,
					packed);

			f->timeout = backup;
		}
	}

	Map_Client_UpdateFogOfWar();

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;

		if (u->o.flags.s.isNotOnMap) continue;

		Unit_RemoveFog(UNVEILCAUSE_INITIALISATION, u);
		Unit_UpdateMap(1, u);
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;
		if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL) continue;

		if (s->o.flags.s.isNotOnMap) continue;

		Structure_RemoveFog(UNVEILCAUSE_INITIALISATION, s);

		if (s->o.type == STRUCTURE_STARPORT && s->o.linkedID != 0xFF) {
			Unit *u = Unit_Get_ByIndex(s->o.linkedID);

			if (!u->o.flags.s.used || !u->o.flags.s.isNotOnMap) {
				s->o.linkedID = 0xFF;
				s->countDown = 0;
			} else {
				Structure_Server_SetState(s, STRUCTURE_STATE_READY);
			}
		}

		Script_Load(&s->o.script, s->o.type);

		if (s->o.type == STRUCTURE_PALACE) {
			House_Get_ByIndex(s->o.houseID)->palacePosition = s->o.position;
		}

		if ((House_Get_ByIndex(s->o.houseID)->palacePosition.x != 0) || (House_Get_ByIndex(s->o.houseID)->palacePosition.y != 0)) continue;
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

	g_tickHousePowerMaintenance = max(g_timerGame + 70, g_tickHousePowerMaintenance);
	g_playerCredits = 0xFFFF;

	g_selectionType = oldSelectionType;
	g_validateStrictIfZero--;
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
	Map_ResetFogOfWar();

	memset(g_mapSpriteID, 0, 64 * 64 * sizeof(uint16));
	memset(g_starportAvailable, 0, sizeof(g_starportAvailable));

	Audio_PlayVoice(VOICE_STOP);

	g_selectionState          = 0; /* Invalid. */
	g_structureActivePosition = 0;

	g_unitActive       = NULL;
	g_structureActive  = NULL;

	g_activeAction          = 0xFFFF;
	g_structureActiveType   = 0xFFFF;

	GUI_DisplayText(NULL, -1);
}

/**
 * Load a scenario in a safe way, and prepare the game.
 * @param houseID The House which is going to play the game.
 * @param scenarioID The Scenario to load.
 */
void Game_LoadScenario(uint8 houseID, uint16 scenarioID)
{
	Audio_PlayVoice(VOICE_STOP);

	Game_Init();

	g_validateStrictIfZero++;

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

	g_validateStrictIfZero--;
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

	GFX_Uninit();
	Video_Uninit();
	A5_Uninit();

	free(g_campaign_list);
	g_campaign_total = 0;
}
