/* menubar.c */

#include <assert.h>
#include <stdio.h>

#include "menubar.h"

#include "../audio/driver.h"
#include "../common_a5.h"
#include "../config.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../opendune.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../timer/timer.h"
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
MenuBar_DrawStatusBar(const char *line1, const char *line2, uint8 fg1, uint8 fg2, int offset)
{
	const int x = g_widgetProperties[WINDOWID_STATUSBAR].xBase*8;
	const int y = g_widgetProperties[WINDOWID_STATUSBAR].yBase;
	const int w = g_widgetProperties[WINDOWID_STATUSBAR].width*8;
	const int h = g_widgetProperties[WINDOWID_STATUSBAR].height;

	Video_SetClippingArea(x, y, w, h);

	if (offset == 0) {
		GUI_DrawText_Wrapper(line1, x, y - offset + 2, fg1, 0, 0x012);
	}
	else {
		GUI_DrawText_Wrapper(line2, x, y - offset + 2, fg2, 0, 0x012);
		GUI_DrawText_Wrapper(line1, x, y - offset + 13, fg1, 0, 0x012);
	}

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}

void
MenuBar_Draw(enum HouseType houseID)
{
	Widget *w;

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

	/* Mentat. */
	w = GUI_Widget_Get_ByIndex(g_widgetLinkedListHead, 1);
	GUI_Widget_Draw(w);

	/* Options. */
	w = GUI_Widget_Get_ByIndex(g_widgetLinkedListHead, 2);
	GUI_Widget_Draw(w);

	Shape_DrawRemap(SHAPE_CREDITS_LABEL, houseID, TRUE_DISPLAY_WIDTH - 128, 0, 0, 0);
}

/*--------------------------------------------------------------*/

bool
MenuBar_ClickOptions(Widget *w)
{
	VARIABLE_NOT_USED(w);

	if (g_gameOverlay != GAMEOVERLAY_NONE)
		return false;

	g_gameOverlay = GAMEOVERLAY_OPTIONS;
	Video_SetCursor(SHAPE_CURSOR_NORMAL);
	Timer_SetTimer(TIMER_GAME, false);
	GUI_Window_Create(&g_optionsWindowDesc);
	return true;
}

static void
MenuBar_TickOptions(void)
{
	const WindowDesc *desc = &g_optionsWindowDesc;

	Video_ShadeScreen(128);
	GUI_Widget_DrawWindow(desc);
	GUI_Widget_DrawAll(g_widgetLinkedListTail);

	const int widgetID = GUI_Widget_HandleEvents(g_widgetLinkedListTail);
	switch (widgetID) {
		case 0x8000 | 30: /* STR_LOAD_A_GAME */
			g_gameOverlay = GAMEOVERLAY_LOAD_GAME;
			GUI_Widget_InitSaveLoad(false);
			break;

		case 0x8000 | 31: /* STR_SAVE_THIS_GAME */
			g_gameOverlay = GAMEOVERLAY_SAVE_GAME;
			GUI_Widget_InitSaveLoad(true);
			break;

		case 0x8000 | 32: /* STR_GAME_CONTROLS */
			g_gameOverlay = GAMEOVERLAY_GAME_CONTROLS;
			GUI_Window_Create(&g_gameControlWindowDesc);
			break;

		case 0x8000 | 33: /* STR_RESTART_SCENARIO */
			g_gameMode = GM_RESTART;
			break;

		case 0x8000 | 34: /* STR_PICK_ANOTHER_HOUSE */
			g_gameMode = GM_PICKHOUSE;
			break;

		case 0x8000 | 35: /* STR_CONTINUE_GAME */
			g_gameOverlay = GAMEOVERLAY_NONE;
			break;

		case 0x8000 | 36: /* STR_QUIT_PLAYING */
			g_gameMode = GM_QUITGAME;
			break;

		default:
			break;
	}
}

static void
MenuBar_TickSaveLoadGame(enum GameOverlay overlay)
{
	static int l_save_entry;

	Video_ShadeScreen(128);

	if (overlay == GAMEOVERLAY_SAVE_ENTRY) {
		const WindowDesc *desc = &g_savegameNameWindowDesc;

		GUI_Widget_DrawWindow(desc);
		GUI_Widget_DrawAll(g_widgetLinkedListTail);

		const int ret = GUI_Widget_Savegame_Click(l_save_entry);

		if (ret == -1) {
			g_gameOverlay = GAMEOVERLAY_OPTIONS;
			GUI_Window_Create(&g_optionsWindowDesc);
		}
		else if (ret == -2) {
			g_gameOverlay = GAMEOVERLAY_NONE;
		}
	}
	else {
		const WindowDesc *desc = &g_saveLoadWindowDesc;

		GUI_Widget_DrawWindow(desc);
		GUI_Widget_DrawAll(g_widgetLinkedListTail);

		const bool save = (overlay == GAMEOVERLAY_SAVE_GAME);
		const int ret = GUI_Widget_SaveLoad_Click(save);

		if (ret == -1) {
			g_gameOverlay = GAMEOVERLAY_OPTIONS;
			GUI_Window_Create(&g_optionsWindowDesc);
		}
		else if (ret == -2) {
			g_gameOverlay = GAMEOVERLAY_NONE;
		}
		else if (ret > 0) {
			g_gameOverlay = GAMEOVERLAY_SAVE_ENTRY;
			GUI_Window_Create(&g_savegameNameWindowDesc);
			l_save_entry = ret - 0x1E;
		}
	}
}

static void
MenuBar_TickGameControls(void)
{
	const WindowDesc *desc = &g_gameControlWindowDesc;

	Video_ShadeScreen(128);
	GUI_Widget_DrawWindow(desc);
	GUI_Widget_DrawAll(g_widgetLinkedListTail);

	const int widgetID = GUI_Widget_HandleEvents(g_widgetLinkedListTail);
	switch (widgetID) {
		case 0x8000 | 30: /* STR_MUSIC_IS */
			g_gameConfig.music ^= 0x1;
			if (g_gameConfig.music == 0)
				Driver_Music_Stop();
			break;

		case 0x8000 | 31: /* STR_SOUNDS_ARE */
			g_gameConfig.sounds ^= 0x1;
			if (g_gameConfig.sounds == 0)
				Driver_Sound_Stop();
			break;

		case 0x8000 | 32: /* STR_GAME_SPEED */
			if (++g_gameConfig.gameSpeed >= 5)
				g_gameConfig.gameSpeed = 0;
			break;

		case 0x8000 | 33: /* STR_HINTS_ARE */
			g_gameConfig.hints ^= 0x1;
			break;

		case 0x8000 | 34: /* STR_AUTO_SCROLL_IS */
			g_gameConfig.autoScroll ^= 0x1;
			break;

		case 0x8000 | 35: /* STR_PREVIOUS */
			g_gameOverlay = GAMEOVERLAY_OPTIONS;
			GUI_Window_Create(&g_optionsWindowDesc);
			GameOptions_Save();
			break;

		default:
			break;
	}
}

void
MenuBar_TickOptionsOverlay(void)
{
	A5_UseMenuTransform();

	switch (g_gameOverlay) {
		case GAMEOVERLAY_OPTIONS:
			MenuBar_TickOptions();
			break;

		case GAMEOVERLAY_LOAD_GAME:
		case GAMEOVERLAY_SAVE_GAME:
		case GAMEOVERLAY_SAVE_ENTRY:
			MenuBar_TickSaveLoadGame(g_gameOverlay);
			break;

		case GAMEOVERLAY_GAME_CONTROLS:
			MenuBar_TickGameControls();
			break;

		default:
			break;
	}

	A5_UseIdentityTransform();

	if (g_gameOverlay == GAMEOVERLAY_NONE) {
		Timer_SetTimer(TIMER_GAME, true);
		Structure_Recount();
		Unit_Recount();
	}
}
