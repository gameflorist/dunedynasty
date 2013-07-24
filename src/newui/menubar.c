/* menubar.c */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "enum_string.h"
#include "../os/common.h"
#include "../os/math.h"
#include "../os/sleep.h"

#include "menubar.h"

#include "mentat.h"
#include "savemenu.h"
#include "scrollbar.h"
#include "slider.h"
#include "viewport.h"
#include "../audio/audio.h"
#include "../common_a5.h"
#include "../config.h"
#include "../enhancement.h"
#include "../gfx.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/mentat.h"
#include "../gui/widget.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../opendune.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../string.h"
#include "../table/widgetinfo.h"
#include "../timer/timer.h"
#include "../tools/random_lcg.h"
#include "../video/video.h"
#include "../wsa.h"

static enum {
	RADAR_ANIMATION_NONE,
	RADAR_ANIMATION_ACTIVATE,
	RADAR_ANIMATION_DEACTIVATE
} radar_animation_state;

static int64_t radar_animation_timer;
static int s_save_entry;

void
MenuBar_DrawCredits(int credits_new, int credits_old, int offset, int x0)
{
	const int digit_w = 10;
	const int y = g_widgetProperties[WINDOWID_CREDITS].yBase;

	char char_old[7];
	char char_new[7];

	snprintf(char_old, sizeof(char_old), "%6d", credits_old);
	snprintf(char_new, sizeof(char_new), "%6d", credits_new);

	for (int i = 0; i < 6; i++) {
		const enum ShapeID shape_old = SHAPE_CREDITS_NUMBER_0 + char_old[i] - '0';
		const enum ShapeID shape_new = SHAPE_CREDITS_NUMBER_0 + char_new[i] - '0';
		const int x = x0 - digit_w * (6 - i);

		if (char_old[i] != char_new[i]) {
			if (char_old[i] != ' ')
				Shape_Draw(shape_old, x, y + offset, 0, 0);

			if (char_new[i] != ' ')
				Shape_Draw(shape_new, x, y + 8 + offset, 0, 0);
		}
		else {
			if (char_new[i] != ' ')
				Shape_Draw(shape_new, x, y + 1, 0, 0);
		}
	}
}

void
MenuBar_DrawStatusBar(const char *line1, const char *line2,
		bool scrollInProgress, int x, int y, int offset)
{
	const enum ScreenDivID divID = A5_SaveTransform();
	const ScreenDiv *div = &g_screenDiv[divID];
	const int h = g_widgetProperties[WINDOWID_STATUSBAR].height;

	Video_SetClippingArea(0, div->scaley * y + div->y, TRUE_DISPLAY_WIDTH, div->scaley * h);

	if (scrollInProgress) {
		GUI_DrawText_Wrapper(line2, x, y - offset + 2, 12, 0, 0x012);
		GUI_DrawText_Wrapper(line1, x, y - offset + 13, 12, 0, 0x012);
	}
	else {
		GUI_DrawText_Wrapper(line1, x, y - offset + 2, 12, 0, 0x012);
	}

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}

static bool
MenuBar_DrawRadarAnimation(void)
{
	if (radar_animation_state == RADAR_ANIMATION_NONE)
		return false;

	const int64_t curr_ticks = Timer_GetTicks();

	if (curr_ticks - radar_animation_timer >= RADAR_ANIMATION_FRAME_COUNT * RADAR_ANIMATION_DELAY) {
		radar_animation_state = RADAR_ANIMATION_NONE;
		return false;
	}

	const int x = g_widgetProperties[WINDOWID_MINIMAP].xBase;
	const int y = g_widgetProperties[WINDOWID_MINIMAP].yBase;

	int frame = (curr_ticks - radar_animation_timer) / RADAR_ANIMATION_DELAY;
	frame = clamp(0, frame, RADAR_ANIMATION_FRAME_COUNT);

	if (radar_animation_state == RADAR_ANIMATION_ACTIVATE)
		frame = RADAR_ANIMATION_FRAME_COUNT - frame - 1;

	Video_DrawWSAStatic(frame, x, y);
	return true;
}

void
MenuBar_Draw(enum HouseType houseID)
{
	const ScreenDiv *menubar = &g_screenDiv[SCREENDIV_MENUBAR];
	const ScreenDiv *sidebar = &g_screenDiv[SCREENDIV_SIDEBAR];
	const enum ScreenDivID prev_transform = A5_SaveTransform();
	const WidgetProperties *statusbar = &g_widgetProperties[WINDOWID_STATUSBAR];

	Widget *w;

	/* MenuBar. */
	A5_UseTransform(SCREENDIV_MENUBAR);

	for (int x = menubar->width - 136; x + 320 - 1 >= 184; x -= 320) {
		Video_DrawCPSSpecial(CPS_MENUBAR_MIDDLE, houseID, x, 0);
	}

	for (int x = menubar->width - 8 - 425; x + 425 - 1 >= 8; x -= 425) {
		Video_DrawCPSSpecial(CPS_STATUSBAR_MIDDLE, houseID, x, 17);
	}

	Video_DrawCPSSpecial(CPS_MENUBAR_LEFT, houseID, 0, 0);
	Video_DrawCPSSpecial(CPS_MENUBAR_RIGHT, houseID, menubar->width - 136, 0);
	Video_DrawCPSSpecial(CPS_STATUSBAR_LEFT, houseID, 0, 17);
	Video_DrawCPSSpecial(CPS_STATUSBAR_RIGHT, houseID, menubar->width - 8, 17);
	Shape_DrawRemap(SHAPE_CREDITS_LABEL, houseID, menubar->width - 128, 0, 0, 0);

	/* Mentat. */
	w = GUI_Widget_Get_ByIndex(g_widgetLinkedListHead, 1);
	GUI_Widget_Draw(w);

	/* Options. */
	w = GUI_Widget_Get_ByIndex(g_widgetLinkedListHead, 2);
	GUI_Widget_Draw(w);

	Video_SetClippingArea(0, menubar->scaley * 4, TRUE_DISPLAY_WIDTH, menubar->scaley * 9);
	GUI_DrawCredits(g_playerHouse->credits, (g_playerCredits == 0xFFFF) ? 2 : 1, TRUE_DISPLAY_WIDTH / menubar->scalex);
	GUI_DrawStatusBarText(statusbar->xBase, statusbar->yBase);

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);

	/* SideBar. */
	A5_UseTransform(SCREENDIV_SIDEBAR);

	for (int y = sidebar->height - 85 - 52; y + 52 - 1 >= 17; y -= 52) {
		Video_DrawCPSSpecial(CPS_SIDEBAR_MIDDLE, houseID, 0, y);
	}

	Video_DrawCPSSpecial(CPS_SIDEBAR_TOP, houseID, 0, 0);
	Video_DrawCPSSpecial(CPS_SIDEBAR_BOTTOM, houseID, 0, sidebar->height - 85);
	Prim_FillRect_i(16, sidebar->height - 64, 80, sidebar->height, 0);

	if (!MenuBar_DrawRadarAnimation()) {
		GUI_Widget_Viewport_RedrawMap();
	}

	A5_UseTransform(prev_transform);
}

void
MenuBar_StartRadarAnimation(bool activate)
{
	if (enhancement_nonblocking_radar_animation) {
		radar_animation_state = activate ? RADAR_ANIMATION_ACTIVATE : RADAR_ANIMATION_DEACTIVATE;
		radar_animation_timer = Timer_GetTicks();
	}
	else {
		Timer_SetTimer(TIMER_GAME, false);

		for (int frame = 0; frame < RADAR_ANIMATION_FRAME_COUNT; frame++) {
			const int x = g_widgetProperties[WINDOWID_MINIMAP].xBase;
			const int y = g_widgetProperties[WINDOWID_MINIMAP].yBase;

			GUI_DrawInterfaceAndRadar();
			Video_DrawWSAStatic(activate ? RADAR_ANIMATION_FRAME_COUNT - frame - 1 : frame, x, y);
			Video_Tick();
			Timer_Sleep(RADAR_ANIMATION_DELAY);
		}

		Timer_SetTimer(TIMER_GAME, true);
	}
}

/*--------------------------------------------------------------*/

bool
MenuBar_ClickMentat(Widget *w)
{
	MentatState *mentat = &g_mentat_state;
	VARIABLE_NOT_USED(w);

	if (g_gameOverlay != GAMEOVERLAY_NONE)
		return false;

	g_gameOverlay = GAMEOVERLAY_MENTAT;
	Sprites_InitMentat(g_table_houseInfo[g_playerHouseID].mentat);
	mentat->state = MENTAT_SHOW_CONTENTS;
	mentat->wsa = NULL;
	Video_SetCursor(SHAPE_CURSOR_NORMAL);
	g_musicInBattle = 0;
	Audio_PlayVoice(VOICE_STOP);
	Audio_PlayMusic(g_table_houseInfo[g_playerHouseID].musicBriefing);
	Timer_SetTimer(TIMER_GAME, false);

	g_widgetLinkedListTail = NULL;
	g_widgetMentatFirst = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, 200, 168, SHAPE_EXIT, 5);
	GUI_Mentat_Create_HelpScreen_Widgets();

	Widget *scrollbar = GUI_Widget_Get_ByIndex(g_widgetMentatTail, 15);
	const bool skip_advice = (g_campaign_selected == CAMPAIGNID_SKIRMISH);
	Mentat_LoadHelpSubjects(scrollbar, true, SEARCHDIR_CAMPAIGN_DIR, g_playerHouseID, g_campaignID, skip_advice);

	Mouse_TransformToDiv(SCREENDIV_MENU, &g_mouseX, &g_mouseY);
	return true;
}

void
MenuBar_TickMentatOverlay(void)
{
	if (MentatHelp_Tick(&g_mentat_state)) {
		free(g_widgetMentatFirst);
		g_widgetMentatFirst = NULL;

		free(g_widgetMentatTail);
		g_widgetMentatTail = NULL;

		GUI_Widget_Free_WithScrollbar(g_widgetMentatScrollbar);
		g_widgetMentatScrollbar = NULL;

		free(g_widgetMentatScrollUp);
		g_widgetMentatScrollUp = NULL;

		free(g_widgetMentatScrollDown);
		g_widgetMentatScrollDown = NULL;

		g_gameOverlay = GAMEOVERLAY_NONE;
		Timer_SetTimer(TIMER_GAME, true);
		Audio_PlayEffect(EFFECT_FADE_OUT);

		/* XXX: fix this rubbish. */
		Sprites_UnloadTiles();
		Sprites_LoadTiles();

		Audio_PlayMusic(MUSIC_RANDOM_IDLE);

		Mouse_TransformFromDiv(SCREENDIV_MENU, &g_mouseX, &g_mouseY);
	}
}

void
MenuBar_DrawMentatOverlay(void)
{
	const enum MentatID mentatID = g_table_houseInfo[g_playerHouseID].mentat;

	A5_UseTransform(SCREENDIV_MENU);

	MentatHelp_Draw(mentatID, &g_mentat_state);
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

	Mouse_TransformToDiv(SCREENDIV_MENU, &g_mouseX, &g_mouseY);
	return true;
}

static void
MenuBar_DrawGameControlLabel(Widget *w)
{
	const WidgetProperties *wi = &g_widgetProperties[w->parentID];

	if (g_gameConfig.language == LANGUAGE_FRENCH) {
		GUI_DrawText_Wrapper(w->data, wi->xBase + 40 - 24, w->offsetY + wi->yBase + 3, 232, 0, 0x22);
	}
	else {
		GUI_DrawText_Wrapper(w->data, w->offsetX + wi->xBase - 10, w->offsetY + wi->yBase + 3, 232, 0, 0x222);
	}
}

static bool
MenuBar_ClickRadioButton(Widget *radio)
{
	const int visible_widgets[3][4 + 5*3 + 1] = {
		{ 35, 90, 91, 92,
		  20, 30, 100, 21, 31, 101, 22, 32, 102, 23, 33, 103, 24, 34, 104,
		  -1
		},
		{ 35, 90, 91, 92,
		  40, 50, 110, 41, 51, 111, 42, 52, 112, 43, 53, 113, 44, 54, 114,
		  -1
		},
		{ 35, 90, 91, 92,
		  60, 70, 120, 61, 71, 121, 62, 72, 122, 63, 73, 123, 64, 74, 124,
		  -1
		},
	};

	const int page = radio->index - 90;
	Widget *w;

	w = g_widgetLinkedListTail;
	while (w != NULL) {
		bool visible = false;

		for (unsigned int i = 0; visible_widgets[page][i] >= 0; i++) {
			if (w->index == visible_widgets[page][i]) {
				visible = true;
				break;
			}
		}

		if (visible) {
			GUI_Widget_MakeVisible(w);
		}
		else {
			GUI_Widget_MakeInvisible(w);
		}

		if (w->clickProc == MenuBar_ClickRadioButton) {
			w->drawParameterNormal.sprite = SHAPE_RADIO_BUTTON_OFF;
			w->state.selected = 0;
		}

		w = GUI_Widget_GetNext(w);
	}

	/* Bit of a hack to make the radio button retain the ON sprite
	 * after we click something else.
	 */
	radio->drawParameterNormal.sprite = SHAPE_RADIO_BUTTON_ON;
	radio->state.selected = 1;

	return true;
}

static bool
MenuBar_ClickMusicVolumeSlider(Widget *w)
{
	if (Slider_Click(w)) {
		const SliderData *data = w->data;

		music_volume = (float)data->curr / (data->max - data->min);
		Audio_AdjustMusicVolume(0.0f, false);
	}

	return true;
}

static bool
MenuBar_ClickSoundVolumeSlider(Widget *w)
{
	if (Slider_Click(w) || w->state.buttonState & 0x01) {
		const enum SampleID sampleID = Tools_RandomLCG_Range(SAMPLE_AFFIRMATIVE, SAMPLE_MOVING_OUT);
		const SliderData *data = w->data;

		sound_volume = (float)data->curr / (data->max - data->min);
		Audio_PlaySample(sampleID, 255, 0.0f);
	}

	return true;
}

static bool
MenuBar_ClickScrollSpeedSlider(Widget *w)
{
	if (Slider_Click(w)) {
		const SliderData *data = w->data;

		g_gameConfig.scrollSpeed = (data->curr == data->min) ? 2 : (4 * data->curr);
	}

	return true;
}

static bool
MenuBar_ClickPanSensitivitySlider(Widget *w)
{
	if (Slider_Click(w)) {
		const SliderData *data = w->data;

		g_gameConfig.panSensitivity = 0.25f * data->curr;
	}

	return true;
}

static void
MenuBar_CreateGameControls(void)
{
	/* 20, 30, 100  -- music label, on/off, slider.
	 * 21, 31, 101  -- sound label, on/off, slider.
	 * 22, 32       -- game speed label, button.
	 * 23, 33       -- hint label, on/off.
	 * 24, 34       -- subtitles label, on/off.
	 *
	 * 40, 50       -- control style label, mode.
	 * 41, 51       -- mouse wheel label, mode.
	 * 42, 52       -- scrolling edge label, on/off.
	 * 43, 53, 113  -- auto scroll label, on/off, slider.
	 * 44, 54, 114  -- pan sensitivity label, slider.
	 *
	 * 60, 70   -- health bars label, off/on/always on.
	 * 61, 71   -- hi-res overlays label, off/on.
	 * 62, 72   -- smooth unit animation label, off/on.
	 * 63, 73   -- infantry squad death label, off/on.
	 * 64, 74   -- hardware mouse cursor label, off/on.
	 *
	 * 25 -- previous.
	 * 90, 91, 92   -- radio buttons.
	 */

	const WindowDesc *desc = &g_gameControlWindowDesc;
	Widget *w;

	GUI_Window_Create(&g_gameControlWindowDesc);

	/* Radio buttons. */
	for (int page = 0; page < 3; page++) {
		w = GUI_Widget_Allocate(90 + page, 0, 8 + 10 * page, g_widgetProperties[g_gameControlWindowDesc.index].height - 17, SHAPE_RADIO_BUTTON_OFF, STR_NULL);
		w->parentID = g_gameControlWindowDesc.index;
		w->clickProc = MenuBar_ClickRadioButton;
		w->state.selected = 0;
		g_widgetLinkedListTail = GUI_Widget_Link(g_widgetLinkedListTail, w);
	}

	/* Labels. */
	const struct {
		uint16 index;
		uint16 position;
		const char *str;
	} label[] = {
		{ 20, 0, String_Get_ByIndex(STR_MUSIC_IS) },
		{ 21, 1, String_Get_ByIndex(STR_SOUNDS_ARE) },
		{ 22, 2, String_Get_ByIndex(STR_GAME_SPEED) },
		{ 23, 3, String_Get_ByIndex(STR_HINTS_ARE) },
		{ 24, 4, "Subtitles are" },
		{ 40, 0, "Control style" },
		{ 41, 1, "Mouse wheel" },
		{ 42, 2, "Scrolling edge" },
		{ 43, 3, String_Get_ByIndex(STR_AUTO_SCROLL_IS) },
		{ 44, 4, "Pan sensitivity" },
		{ 60, 0, "Health bars" },
		{ 61, 1, "Hi-res overlays" },
		{ 62, 2, "Smooth unit animation" },
		{ 63, 3, "Infantry squad corpses" },
		{ 64, 4, "Hardware mouse cursor" },
	};

	for (unsigned int i = 0; i < lengthof(label); i++) {
		const uint16 x = desc->widgets[label[i].position].offsetX + (label[i].index >= 60 ? 40 : 0);
		const uint16 y = desc->widgets[label[i].position].offsetY;

		w = GUI_Widget_Allocate(label[i].index, 0, x, y, -2, STR_NULL);
		w->parentID = g_gameControlWindowDesc.index;
		w->data = (void *)label[i].str;
		w->drawParameterDown.proc = MenuBar_DrawGameControlLabel;
		w->drawParameterNormal.proc = MenuBar_DrawGameControlLabel;
		w->drawParameterSelected.proc = MenuBar_DrawGameControlLabel;

		g_widgetLinkedListTail = GUI_Widget_Link(g_widgetLinkedListTail, w);
	}

	/* Buttons. */
	const struct {
		uint16 index;
		uint16 position, w;
	} button[] = {
		{ 50, 0, 104 }, /* Control style. */
		{ 51, 1, 104 }, /* Mouse wheel. */
		{ 52, 2, 104 }, /* Scrolling edge. */
		{ 53, 3,  46 }, /* Auto scroll. */
		{ 70, 0,  64 }, /* Health bars. */
		{ 71, 1,  64 }, /* Hi-res vector graphics. */
		{ 72, 2,  64 }, /* Smooth unit animation. */
		{ 73, 3,  64 }, /* Infantry squad corpses. */
		{ 74, 4,  64 }, /* Hardware mouse cursor. */
	};

	for (unsigned int i = 0; i < lengthof(button); i++) {
		const uint16 x = desc->widgets[button[i].position].offsetX + (button[i].index >= 60 ? 40 : 0);
		const uint16 y = desc->widgets[button[i].position].offsetY;

		w = GUI_Widget_Allocate(button[i].index, 0, x, y, -2, -button[i].index);
		w->parentID = g_gameControlWindowDesc.index;
		w->flags = g_table_windowWidgets[0].flags;
		w->width = button[i].w;
		w->height = 15;
		w->drawParameterNormal.proc = GUI_Widget_TextButton_Draw;
		w->drawParameterSelected.proc = GUI_Widget_TextButton_Draw;
		w->drawParameterDown.proc = GUI_Widget_TextButton_Draw;

		g_widgetLinkedListTail = GUI_Widget_Link(g_widgetLinkedListTail, w);
	}

	/* Sliders. */
	const struct {
		uint16 index;
		uint16 x, position;
		int min, max, tics;
		bool (*clickProc)(Widget *widget);
	} slider[] = {
		{ 100, 180, 0, 0, 20, 2, MenuBar_ClickMusicVolumeSlider },
		{ 101, 180, 1, 0, 20, 2, MenuBar_ClickSoundVolumeSlider },
		{ 113, 180, 3, 0,  4, 1, MenuBar_ClickScrollSpeedSlider },
		{ 114, 131, 4, 2,  8, 1, MenuBar_ClickPanSensitivitySlider },
	};

	for (unsigned int i = 0; i < lengthof(slider); i++) {
		const uint16 width = 50 + (180 - slider[i].x);
		const uint16 y = 24 + 17 * slider[i].position;
		SliderData *data;

		w = Slider_Allocate(slider[i].index, g_gameControlWindowDesc.index, slider[i].x, y, width, 12);
		w->clickProc = slider[i].clickProc;

		data = w->data;
		data->min = slider[i].min;
		data->max = slider[i].max;
		data->tics = slider[i].tics;

		switch (w->index) {
			case 100: data->curr = 20 * music_volume; break;
			case 101: data->curr = 20 * sound_volume; break;
			case 113: data->curr = clamp(data->min, (g_gameConfig.scrollSpeed / 4), data->max); break;
			case 114: data->curr = clamp(data->min, g_gameConfig.panSensitivity / 0.25f, data->max); break;
			default: assert(false); break;
		}

		g_widgetLinkedListTail = GUI_Widget_Link(g_widgetLinkedListTail, w);
	}

	GUI_Widget_MakeSelected(GUI_Widget_Get_ByIndex(g_widgetLinkedListTail, 90), true);
}

static void
MenuBar_UninitGameControls(void)
{
	Widget *w = g_widgetLinkedListTail;
	while (w != NULL) {
		Widget *next = GUI_Widget_GetNext(w);

		if (30 <= w->index && w->index <= 38) { /* Widgets stored in g_table_windowWidgets. */
		}
		else if (w->index >= 100) { /* Sliders. */
			Slider_Free(w);
		}
		else { /* Regular widgets. */
			free(w);
		}

		w = next;
	}
}

static void
MenuBar_TickOptions(void)
{
	const int widgetID = GUI_Widget_HandleEvents(g_widgetLinkedListTail);
	switch (widgetID) {
		case 0x8000 | 30: /* STR_LOAD_A_GAME */
			g_gameOverlay = GAMEOVERLAY_LOAD_GAME;
			SaveMenu_InitSaveLoad(false);
			break;

		case 0x8000 | 31: /* STR_SAVE_THIS_GAME */
			g_gameOverlay = GAMEOVERLAY_SAVE_GAME;
			SaveMenu_InitSaveLoad(true);
			break;

		case 0x8000 | 32: /* STR_GAME_CONTROLS */
			g_gameOverlay = GAMEOVERLAY_GAME_CONTROLS;
			MenuBar_CreateGameControls();
			break;

		case 0x8000 | 33: /* STR_RESTART_SCENARIO */
			g_gameOverlay = GAMEOVERLAY_CONFIRM_RESTART;
			g_yesNoWindowDesc.stringID = STR_ARE_YOU_SURE_YOU_WISH_TO_RESTART;
			GUI_Window_Create(&g_yesNoWindowDesc);
			break;

		case 0x8000 | 34: /* STR_PICK_ANOTHER_HOUSE */
			g_gameOverlay = GAMEOVERLAY_CONFIRM_PICK_HOUSE;
			g_yesNoWindowDesc.stringID = STR_ARE_YOU_SURE_YOU_WISH_TO_PICK_A_NEW_HOUSE;
			GUI_Window_Create(&g_yesNoWindowDesc);
			break;

		case 0x8000 | 35: /* STR_CONTINUE_GAME */
			g_gameOverlay = GAMEOVERLAY_NONE;
			break;

		case 0x8000 | 36: /* STR_QUIT_PLAYING */
			g_gameOverlay = GAMEOVERLAY_CONFIRM_QUIT;
			g_yesNoWindowDesc.stringID = STR_ARE_YOU_SURE_YOU_WANT_TO_QUIT_PLAYING;
			GUI_Window_Create(&g_yesNoWindowDesc);
			break;

		default:
			break;
	}
}

static void
MenuBar_TickSaveLoadGame(enum GameOverlay overlay)
{
	if (overlay == GAMEOVERLAY_SAVE_ENTRY) {
		const int ret = SaveMenu_Savegame_Click(s_save_entry);

		if (ret == -1) {
			g_gameOverlay = GAMEOVERLAY_OPTIONS;
			GUI_Window_Create(&g_optionsWindowDesc);
		}
		else if (ret == -2) {
			g_gameOverlay = GAMEOVERLAY_NONE;
		}
	}
	else {
		const bool save = (overlay == GAMEOVERLAY_SAVE_GAME);
		const int ret = SaveMenu_SaveLoad_Click(save);

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
			s_save_entry = ret - 0x1E;

			char *saveDesc = g_savegameDesc[s_save_entry];
			if (*saveDesc == '[') *saveDesc = '\0';
		}
	}
}

static void
MenuBar_TickGameControls(void)
{
	const int widgetID = GUI_Widget_HandleEvents(g_widgetLinkedListTail);
	Widget *w;

	switch (widgetID) {
		case 0x8000 | 30: /* STR_MUSIC_IS */
			g_enable_music = !g_enable_music;
			if (!g_enable_music)
				Audio_PlayMusic(MUSIC_STOP);
			break;

		case 0x8000 | 31: /* STR_SOUNDS_ARE */
			w = GUI_Widget_Get_ByIndex(g_widgetLinkedListTail, 31);
			if (w->state.buttonState & 0x04) {
				g_enable_sound_effects++;
				if (g_enable_sound_effects > SOUNDEFFECTS_SYNTH_AND_SAMPLES)
					g_enable_sound_effects = SOUNDEFFECTS_NONE;
			}
			else {
				if (g_enable_sound_effects == SOUNDEFFECTS_NONE)
					g_enable_sound_effects = SOUNDEFFECTS_SYNTH_AND_SAMPLES;
				else
					g_enable_sound_effects--;
			}
			g_enable_voices = !(g_enable_sound_effects == SOUNDEFFECTS_NONE || g_enable_sound_effects == SOUNDEFFECTS_SYNTH_ONLY);
			break;

		case 0x8000 | 32: /* STR_GAME_SPEED */
			w = GUI_Widget_Get_ByIndex(g_widgetLinkedListTail, 32);
			if (w->state.buttonState & 0x04) {
				if (++g_gameConfig.gameSpeed >= 5)
					g_gameConfig.gameSpeed = 0;
			}
			else {
				if (--g_gameConfig.gameSpeed < 0)
					g_gameConfig.gameSpeed = 4;
			}
			break;

		case 0x8000 | 33: g_gameConfig.hints ^= 0x1; break;
		case 0x8000 | 34: g_enable_subtitles ^= 0x1; break;

		case 0x8000 | 35: /* STR_PREVIOUS */
			g_gameOverlay = GAMEOVERLAY_OPTIONS;
			MenuBar_UninitGameControls();

			/* Reinitialise widget positions in case we change scroll along screen edge. */
			GameLoop_TweakWidgetDimensions();

			GUI_Window_Create(&g_optionsWindowDesc);
			break;

		case 0x8000 | 50: g_gameConfig.leftClickOrders ^= 0x1; break;
		case 0x8000 | 51: g_gameConfig.holdControlToZoom ^= 0x1; break;
		case 0x8000 | 52: g_gameConfig.scrollAlongScreenEdge ^= 0x1; break;
		case 0x8000 | 53: g_gameConfig.autoScroll ^= 0x1; break;

		case 0x8000 | 70: /* Health bars. */
			w = GUI_Widget_Get_ByIndex(g_widgetLinkedListTail, 70);
			if (w->state.buttonState & 0x04) {
				enhancement_draw_health_bars = (enhancement_draw_health_bars + 1) % NUM_HEALTH_BAR_MODES;
			}
			else {
				enhancement_draw_health_bars = (enhancement_draw_health_bars + NUM_HEALTH_BAR_MODES - 1) % NUM_HEALTH_BAR_MODES;
			}
			break;

		case 0x8000 | 71: enhancement_high_res_overlays ^= 0x1; break;
		case 0x8000 | 72: enhancement_smooth_unit_animation = (enhancement_smooth_unit_animation == SMOOTH_UNIT_ANIMATION_DISABLE) ? SMOOTH_UNIT_ANIMATION_ENABLE : SMOOTH_UNIT_ANIMATION_DISABLE; break;
		case 0x8000 | 73: enhancement_infantry_squad_death_animations ^= 0x1; break;
		case 0x8000 | 74:
			g_gameConfig.hardwareCursor ^= 0x1;
			Mouse_SwitchHWCursor();
			break;

		default:
			break;
	}
}

static void
MenuBar_TickConfirmation(enum GameOverlay overlay)
{
	const int widgetID = GUI_Widget_HandleEvents(g_widgetLinkedListTail);
	switch (widgetID) {
		case 0x8000 | 30: /* Yes */
			if (overlay == GAMEOVERLAY_CONFIRM_RESTART) {
				g_gameMode = GM_RESTART;
			}
			else if (overlay == GAMEOVERLAY_CONFIRM_PICK_HOUSE) {
				g_gameMode = GM_PICKHOUSE;
			}
			else {
				g_gameMode = GM_QUITGAME;
			}
			break;

		case 0x8000 | 31: /* No */
			g_gameOverlay = GAMEOVERLAY_OPTIONS;
			GUI_Window_Create(&g_optionsWindowDesc);
			break;

		default:
			break;
	}
}

void
MenuBar_TickOptionsOverlay(void)
{
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

		case GAMEOVERLAY_CONFIRM_RESTART:
		case GAMEOVERLAY_CONFIRM_PICK_HOUSE:
		case GAMEOVERLAY_CONFIRM_QUIT:
			MenuBar_TickConfirmation(g_gameOverlay);
			break;

		default:
			break;
	}

	if (g_gameOverlay == GAMEOVERLAY_NONE) {
		Timer_SetTimer(TIMER_GAME, true);
		Structure_Recount();
		Unit_Recount();

		Mouse_TransformFromDiv(SCREENDIV_MENU, &g_mouseX, &g_mouseY);
	}
}

void
MenuBar_DrawOptionsOverlay(void)
{
	A5_UseTransform(SCREENDIV_MENU);

	Video_ShadeScreen(128);

	switch (g_gameOverlay) {
		case GAMEOVERLAY_OPTIONS:
			GUI_Widget_DrawWindow(&g_optionsWindowDesc);
			break;

		case GAMEOVERLAY_LOAD_GAME:
		case GAMEOVERLAY_SAVE_GAME:
			GUI_Widget_DrawWindow(&g_saveLoadWindowDesc);
			break;

		case GAMEOVERLAY_SAVE_ENTRY:
			GUI_Widget_DrawWindow(&g_savegameNameWindowDesc);
			GUI_Widget_Savegame_Draw(s_save_entry);
			break;

		case GAMEOVERLAY_GAME_CONTROLS:
			GUI_Widget_DrawWindow(&g_gameControlWindowDesc);
			break;

		case GAMEOVERLAY_CONFIRM_RESTART:
		case GAMEOVERLAY_CONFIRM_PICK_HOUSE:
		case GAMEOVERLAY_CONFIRM_QUIT:
			GUI_Widget_DrawWindow(&g_yesNoWindowDesc);
			break;

		default:
			break;
	}

	GUI_Widget_DrawAll(g_widgetLinkedListTail);
}

/*--------------------------------------------------------------*/

uint16
GUI_DisplayModalMessage(const char *str, uint16 shapeID, ...)
{
	const ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];
	const ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];
	const enum ScreenDivID prev_div = A5_SaveTransform();

	WidgetProperties *w = &g_widgetProperties[WINDOWID_MODAL_MESSAGE];
	char textBuffer[768];

	va_list ap;

	va_start(ap, shapeID);
	vsnprintf(textBuffer, sizeof(textBuffer), str, ap);
	va_end(ap);

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

	const int lines = GUI_SplitText(textBuffer, (w->width - ((shapeID == SHAPE_INVALID) ? 2*8 : 7*8)) - 6, '\r');
	w->height = g_fontCurrent->height * max(lines, 3) + 18;

	/* Stop panning mode and show the cursor for this blocking dialog. */
	Viewport_Init();
	Video_ShowCursor();
	Input_History_Clear();

	bool redraw = true;
	while (true) {
		if (redraw) {
			redraw = false;

			A5_UseTransform(SCREENDIV_MAIN);

			GUI_DrawInterfaceAndRadar();
			Video_ShadeScreen(128);

			A5_UseTransform(SCREENDIV_MENU);

			/* Centre the dialog box to the viewport. */
			const int old_x = w->xBase;
			w->xBase = ((viewport->scalex * viewport->width) / div->scalex - w->width) / 2;

			GUI_Widget_DrawBorder(WINDOWID_MODAL_MESSAGE, 1, 1);
			GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

			if (shapeID != SHAPE_INVALID) {
				Shape_Draw(shapeID, 7, 8, WINDOWID_MODAL_MESSAGE, 0x4000);
				GUI_DrawText(textBuffer, w->xBase + 5*8, w->yBase + 8, w->fgColourBlink, 0);
			}
			else {
				GUI_DrawText(textBuffer, w->xBase + 1*8, w->yBase + 8, w->fgColourBlink, 0);
			}

			w->xBase = old_x;

			Video_Tick();
		}

		const bool narrator_speaking = Audio_Poll();

		if (Input_Tick(true))
			redraw = true;

		if (Input_IsInputAvailable()) {
			const int key = Input_GetNextKey();

			if (narrator_speaking)
				continue;

			if ((key == MOUSE_LMB) || (key == MOUSE_RMB) || (key == SCANCODE_ESCAPE) || (key == SCANCODE_SPACE))
				break;
		}

		sleepIdle();
	}

	A5_UseTransform(prev_div);

	/* Not sure. */
	return 0;
}
