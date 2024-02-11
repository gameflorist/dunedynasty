/* menu_lobby.c */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "enum_string.h"
#include "../os/math.h"

#include "menu.h"
#include "menu_lobby.h"

#include "chatbox.h"
#include "editbox.h"
#include "halloffame.h"
#include "scrollbar.h"
#include "../audio/audio.h"
#include "../enhancement.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../mods/mapgenerator.h"
#include "../mods/multiplayer.h"
#include "../mods/skirmish.h"
#include "../net/client.h"
#include "../net/net.h"
#include "../scenario.h"
#include "../shape.h"
#include "../string.h"
#include "../timer/timer.h"
#include "../video/video.h"
#include "../wsa.h"

static Widget *pick_lobby_widgets;
static Widget *skirmish_lobby_widgets;
static Widget *map_options_lobby_widgets;
static Widget *multiplayer_lobby_widgets;
static int64_t lobby_radar_timer;
enum MapGeneratorMode lobby_map_generator_mode;

char map_options_fixed_seed[5 + 1] = "";
char map_options_starting_credits[5 + 1] = "0";
enum MapLoseCondition map_options_lose_condition = MAP_LOSE_CONDITION_STRUCTURES;
enum MapSeedMode map_options_seed_mode = MAP_SEED_MODE_RANDOM;
char map_options_spice_fields_min[3 + 1] = "0";
char map_options_spice_fields_max[3 + 1] = "0";
enum MapWormCount map_options_worm_count = MAP_WORM_COUNT_2;

/* Values to restore, if the user cancels map options dialog: */
Skirmish map_options_saved_skirmish;
Multiplayer map_options_saved_multiplayer;
enum MapSeedMode map_options_saved_seed_mode;
enum MapLoseCondition map_options_saved_lose_condition;
enum MapWormCount map_options_saved_worm_count;

/*--------------------------------------------------------------*/

static void
PickLobby_InitWidgets(void)
{
	Widget *w;
	int width;

	/* Previous (main menu). */
	width = Font_GetStringWidth(String_Get_ByIndex(STR_MAIN_MENU)) + 20;
	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, (320 - width) / 2, 176, 0xFFFE, STR_MAIN_MENU);
	w->width  = width;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);

	/* Skirmish. */
	width = Font_GetStringWidth(String_Get_ByIndex(STR_START)) + 20;
	w = GUI_Widget_Allocate(10, 0, 11 + (102 - width) / 2, 148, 0xFFFE, STR_START);
	w->width  = width;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);

	/* Host. */
	w = GUI_Widget_Allocate(20, 0, 266, 128, 0xFFFE, STR_HOST);
	w->width  = 37;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);

	w = GUI_Widget_Allocate(21, 0, 125, 128, 0xFFFE, STR_NULL);
	w->width  = 90;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = g_host_addr;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);

	w = GUI_Widget_Allocate(22, 0, 217, 128, 0xFFFE, STR_NULL);
	w->width  = 46;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = g_host_port;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);

	/* Join. */
	w = GUI_Widget_Allocate(30, 0, 266, 148, 0xFFFE, STR_JOIN);
	w->width  = 37;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);

	w = GUI_Widget_Allocate(31, 0, 125, 148, 0xFFFE, STR_NULL);
	w->width  = 90;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = g_join_addr;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);

	w = GUI_Widget_Allocate(32, 0, 217, 148, 0xFFFE, STR_NULL);
	w->width  = 46;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = g_join_port;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);
}

static void
Lobby_ShowHideStartButton(Widget *w, bool show)
{
	w = GUI_Widget_Get_ByIndex(w, 2);

	if (show) {
		GUI_Widget_MakeVisible(w);
	} else {
		GUI_Widget_MakeInvisible(w);
	}
}

static bool
SkirmishLobby_IsPlayable(void)
{
	const bool is_playable
		= (lobby_map_generator_mode == MAP_GENERATOR_STOP)
		&&  Skirmish_IsPlayable();

	Lobby_ShowHideStartButton(skirmish_lobby_widgets, is_playable);
	return is_playable;
}

static void
Lobby_UpdateMap(void)
{
	if (g_campaign_selected == CAMPAIGNID_SKIRMISH) {
		if (lobby_map_generator_mode != MAP_GENERATOR_STOP) {
			bool success = Skirmish_GenerateMap(lobby_map_generator_mode);
			lobby_map_generator_mode
				= MapGenerator_TransitionState(lobby_map_generator_mode, success);
			SkirmishLobby_IsPlayable();
		}
	} else { /* Multiplayer mode */
		if (lobby_map_generator_mode != MAP_GENERATOR_STOP) {
			lobby_map_generator_mode = Multiplayer_GenerateMap(lobby_map_generator_mode);

			if (Net_HasServerRole() && (lobby_map_generator_mode == MAP_GENERATOR_STOP))
				g_sendScenario = true;
		}
	}
}

static enum NetEvent
Lobby_HandleMessages(void)
{
	enum NetEvent e = NETEVENT_NORMAL;
	if (Net_HasServerRole()) {
		Server_RecvMessages();
		Server_SendMessages();
	} else {
		Client_SendMessages();
		e = Client_RecvMessages();
	}
	return e;
}

static bool
MapOptionsLobby_IsReadOnly(void)
{
	bool isEditable = (g_campaign_selected == CAMPAIGNID_SKIRMISH)
			|| ((g_campaign_selected == CAMPAIGNID_MULTIPLAYER) && Net_HasServerRole());
	return !isEditable;
}

static void
MapOptionsLobby_ChangeSeedMode(enum MapSeedMode seed_mode)
{
	for(int m = 0; m < 3; m++) {
		Widget *w = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, MAP_OPTIONS_WIDGET_SEED_RANDOM + m);
		w->drawParameterNormal.sprite = ((int)seed_mode == m) ? SHAPE_RADIO_BUTTON_ON: SHAPE_RADIO_BUTTON_OFF;
		w->state.selected = ((int)seed_mode == m) ? 1: 0;
	}
	if (seed_mode == MAP_SEED_MODE_FIXED) {
		const uint32 seed = atoi(map_options_fixed_seed) & 0x7FFF;
		const bool is_skirmish = (g_campaign_selected == CAMPAIGNID_SKIRMISH);
		if (is_skirmish) {
			g_skirmish.seed = seed;
		} else {
			g_multiplayer.test_seed = seed;
		}
		lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
	}

	if (seed_mode == MAP_SEED_MODE_SURPRISE) {
		/* We want the map to be a surprise, so generate a new one. */
		lobby_map_generator_mode = MAP_GENERATOR_TRY_RAND_ELSE_RAND;
	}

	map_options_seed_mode = seed_mode;
}

static void
MapOptionsLobby_ChangeLoseCondition(enum MapLoseCondition lose_condition)
{
	for(int m = 0; m < 2; m++) {
		Widget *w = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, MAP_OPTIONS_WIDGET_LOSE_CONDITION_STRUCTURES + m);
		w->drawParameterNormal.sprite = ((int)lose_condition == m) ? SHAPE_RADIO_BUTTON_ON: SHAPE_RADIO_BUTTON_OFF;
		w->state.selected = ((int)lose_condition == m) ? 1: 0;
	}
	if (g_campaign_selected == CAMPAIGNID_SKIRMISH) {
		g_skirmish.lose_condition = lose_condition;
	} else {
		g_multiplayer.lose_condition = lose_condition;
	}
	map_options_lose_condition = lose_condition;
}

static void
MapOptionsLobby_ChangeWormCount(enum MapWormCount worm_count)
{
	for(int m = 0; m < 4; m++) {
		Widget *w = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, MAP_OPTIONS_WIDGET_WORM_COUNT_0 + m);
		w->drawParameterNormal.sprite = ((int)worm_count == m) ? SHAPE_RADIO_BUTTON_ON: SHAPE_RADIO_BUTTON_OFF;
		w->state.selected = ((int)worm_count == m) ? 1: 0;
	}
	if (g_campaign_selected == CAMPAIGNID_SKIRMISH) {
		g_skirmish.worm_count = worm_count;
	} else {
		g_multiplayer.worm_count = worm_count;
	}
	map_options_worm_count = worm_count;
}

static void
MapOptionsLobby_UpdateReadOnlyView(void)
{
	if ((g_campaign_selected == CAMPAIGNID_MULTIPLAYER) && !Net_HasServerRole()) {
		/* Propagate server changes in read-only mode. */
		snprintf(map_options_starting_credits, sizeof(map_options_starting_credits), "%d", g_multiplayer.credits);
		if (g_multiplayer.seed_mode == MAP_SEED_MODE_SURPRISE) {
			strcpy(map_options_fixed_seed, "");
		} else {
			snprintf(map_options_fixed_seed, sizeof(map_options_fixed_seed), "%d", g_multiplayer.next_seed);
		}
		LandscapeGeneratorParams params = g_multiplayer.landscape_params;
		snprintf(map_options_spice_fields_min, sizeof(map_options_spice_fields_min), "%d", params.min_spice_fields);
		snprintf(map_options_spice_fields_max, sizeof(map_options_spice_fields_max), "%d", params.max_spice_fields);

		GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 5)->state.selected = (enhancement_fog_of_war) ? 1: 0;
		GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 6)->state.selected = (enhancement_insatiable_sandworms) ? 1: 0;

		if (map_options_seed_mode != g_multiplayer.seed_mode) {
			map_options_seed_mode = g_multiplayer.seed_mode;
			MapOptionsLobby_ChangeSeedMode(map_options_seed_mode);
		}

		if (map_options_lose_condition != g_multiplayer.lose_condition) {
			map_options_lose_condition = g_multiplayer.lose_condition;
			MapOptionsLobby_ChangeLoseCondition(map_options_lose_condition);
		}

		if (map_options_worm_count != g_multiplayer.worm_count) {
			map_options_worm_count = g_multiplayer.worm_count;
			MapOptionsLobby_ChangeWormCount(map_options_worm_count);
		}
	}
}

static bool
MapOptionsLobby_ClickMapSeedRadioButton(Widget *radio)
{
	MapOptionsLobby_ChangeSeedMode(radio->index - MAP_OPTIONS_WIDGET_SEED_RANDOM);

	return true;
}

static bool
MapOptionsLobby_ClickLoseConditionRadioButton(Widget *radio)
{
	MapOptionsLobby_ChangeLoseCondition(radio->index - MAP_OPTIONS_WIDGET_LOSE_CONDITION_STRUCTURES);

	return true;
}

static bool
MapOptionsLobby_ClickWormCountRadioButton(Widget *radio)
{
	MapOptionsLobby_ChangeWormCount(radio->index - MAP_OPTIONS_WIDGET_WORM_COUNT_0);

	return true;
}

static void
MapOptionsLobby_InitWidgets(void)
{
	Widget *w;

	const int lineHeight = MAP_OPTIONS_GUI_LINE_HEIGHT;
	int offsetY;

	/* Starting credits. */
	w = GUI_Widget_Allocate(24, 0, MAP_OPTIONS_GUI_MAIN_X + 4, MAP_OPTIONS_GUI_MAIN_Y + 1*lineHeight, 0xFFFE, STR_NULL);
	w->width  = 46;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = map_options_starting_credits;
	w->flags.greyWhenInvisible = true;
	w->flags.invisible = true;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);

	/* lose condition radio buttons. */
	const int loseConditionRadioOffsetX[] = {0,68};
	for (int choice = 0; choice < 2; choice++) {
		w = GUI_Widget_Allocate(MAP_OPTIONS_WIDGET_LOSE_CONDITION_STRUCTURES + choice, 0, MAP_OPTIONS_GUI_MAIN_X + loseConditionRadioOffsetX[choice],
				MAP_OPTIONS_GUI_MAIN_Y + 44, SHAPE_RADIO_BUTTON_OFF, STR_NULL);
		w->width = (choice == MAP_OPTIONS_WIDGET_LOSE_CONDITION_STRUCTURES) ? 84 : 46;
		w->clickProc = MapOptionsLobby_ClickLoseConditionRadioButton;
		w->drawParameterNormal.sprite = (choice == (int)map_options_lose_condition) ? SHAPE_RADIO_BUTTON_ON: SHAPE_RADIO_BUTTON_OFF;
		w->state.selected = (choice == (int)map_options_lose_condition) ? 1: 0;
		w->flags.greyWhenInvisible = true;
		map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);
	}

	/* Map selection mode. */

	/* Seed radio buttons. */
	const int mapOffsetY[] = {13, 26, 39};
	for (int choice = 0; choice < 3; choice++) {
		w = GUI_Widget_Allocate(MAP_OPTIONS_WIDGET_SEED_RANDOM + choice, 0, MAP_OPTIONS_GUI_MAP_X + 4,
				MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[choice], SHAPE_RADIO_BUTTON_OFF, STR_NULL);
		w->width = (choice == MAP_SEED_MODE_SURPRISE) ? 84 : 46;
		w->clickProc = MapOptionsLobby_ClickMapSeedRadioButton;
		w->drawParameterNormal.sprite = (choice == (int)map_options_seed_mode) ? SHAPE_RADIO_BUTTON_ON: SHAPE_RADIO_BUTTON_OFF;
		w->state.selected = (choice == (int)map_options_seed_mode) ? 1: 0;
		w->flags.greyWhenInvisible = true;
		map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);
	}

	/* Map fixed seed input. */
	w = GUI_Widget_Allocate(23, 0, MAP_OPTIONS_GUI_MAP_X + 50, MAP_OPTIONS_GUI_MAP_Y + 26 - 2, 0xFFFE, STR_NULL);
	w->width  = 46;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = map_options_fixed_seed;
	w->flags.greyWhenInvisible = true;
	w->flags.invisible = true;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);

	/* Spice fields min input */
	w = GUI_Widget_Allocate(3, 0, MAP_OPTIONS_GUI_MAP_X + 22, MAP_OPTIONS_GUI_MAP_Y + 64 -2, 0xFFFE, STR_NULL);
	w->width  = 32;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = map_options_spice_fields_min;
	w->flags.greyWhenInvisible = true;
	w->flags.invisible = true;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);

	/* Spice fields max input */
	w = GUI_Widget_Allocate(4, 0, MAP_OPTIONS_GUI_MAP_X + 78, MAP_OPTIONS_GUI_MAP_Y + 64 -2, 0xFFFE, STR_NULL);
	w->width  = 32;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = map_options_spice_fields_max;
	w->flags.greyWhenInvisible = true;
	w->flags.invisible = true;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);

	/* worm count radio buttons. */
	for (int choice = 0; choice < 4; choice++) {
		w = GUI_Widget_Allocate(MAP_OPTIONS_WIDGET_WORM_COUNT_0 + choice, 0, MAP_OPTIONS_GUI_MAP_X + 45 + (choice*22), MAP_OPTIONS_GUI_MAP_Y + 76 + 2, SHAPE_RADIO_BUTTON_OFF, STR_NULL);
		w->width = 22;
		w->clickProc = MapOptionsLobby_ClickWormCountRadioButton;
		w->drawParameterNormal.sprite = (choice == (int)map_options_worm_count) ? SHAPE_RADIO_BUTTON_ON: SHAPE_RADIO_BUTTON_OFF;
		w->state.selected = (choice == (int)map_options_worm_count) ? 1: 0;
		w->flags.greyWhenInvisible = true;
		map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);
	}

	/* Enhancement: Fog of War */
	offsetY = MAP_OPTIONS_GUI_MAIN_Y + 13;
	w = GUI_Widget_Allocate(5, 0, MAP_OPTIONS_GUI_MAIN_X, offsetY + 4 * lineHeight,
			SHAPE_CHECKBOX_OFF, STR_NULL);
	w->width = 110;
	w->flags.greyWhenInvisible = true;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);

	/* Enhancement: Insatiable sand worms */
	offsetY = MAP_OPTIONS_GUI_MAIN_Y + 17;
	w = GUI_Widget_Allocate(6, 0, MAP_OPTIONS_GUI_MAIN_X, offsetY + 5 * lineHeight,
			SHAPE_CHECKBOX_OFF, STR_NULL);
	w->width = 110;
	w->flags.greyWhenInvisible = true;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);

	/* Regenerate map. */
	w = GUI_Widget_Allocate(9, 0, MAP_OPTIONS_GUI_MAP_X + 119, MAP_OPTIONS_GUI_MAP_Y + 9, SHAPE_INVALID, STR_NULL);
	w->width = 62;
	w->height = 62;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);

	/* Cancel/Previous and Apply. */
	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, 0, 0, 0xFFFE, STR_CANCEL);
	w->width  = Font_GetStringWidth(String_Get_ByIndex(STR_CANCEL)) + 12;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);

	w = GUI_Widget_Allocate(2, 0, 0, 0, 0xFFFE, STR_APPLY);
	w->width  = Font_GetStringWidth(String_Get_ByIndex(STR_APPLY)) + 12;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->flags.greyWhenInvisible = false;
	w->flags.invisible = true;
	map_options_lobby_widgets = GUI_Widget_Link(map_options_lobby_widgets, w);
}

void
MapOptionsLobby_Initialise(void)
{
	const bool is_skirmish = (g_campaign_selected == CAMPAIGNID_SKIRMISH);
	uint16 starting_credits = is_skirmish ? g_skirmish.credits : g_multiplayer.credits;
	snprintf(map_options_starting_credits, sizeof(map_options_starting_credits), "%d", starting_credits);

	if (atoi(map_options_fixed_seed) == 0 && map_options_seed_mode != MAP_SEED_MODE_SURPRISE) {
		uint16 seed = is_skirmish ? g_skirmish.seed : g_multiplayer.next_seed;
		snprintf(map_options_fixed_seed, sizeof(map_options_fixed_seed), "%d", seed);
	}

	GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 5)->state.selected = (enhancement_fog_of_war) ? 1: 0;
	GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 6)->state.selected = (enhancement_insatiable_sandworms) ? 1: 0;

	LandscapeGeneratorParams params = is_skirmish ? g_skirmish.landscape_params: g_multiplayer.landscape_params;
	snprintf(map_options_spice_fields_min, sizeof(map_options_spice_fields_min), "%d", params.min_spice_fields);
	snprintf(map_options_spice_fields_max, sizeof(map_options_spice_fields_max), "%d", params.max_spice_fields);

	map_options_worm_count = is_skirmish ? g_skirmish.worm_count : g_multiplayer.worm_count;

	map_options_saved_skirmish = g_skirmish;
	map_options_saved_multiplayer = g_multiplayer;
	map_options_saved_seed_mode = map_options_seed_mode;
	map_options_saved_lose_condition = map_options_lose_condition;
	map_options_saved_worm_count = map_options_worm_count;

	int widgetIDs[] = {3, 4, 5, 6, 23, 24,
		MAP_OPTIONS_WIDGET_LOSE_CONDITION_STRUCTURES, MAP_OPTIONS_WIDGET_LOSE_CONDITION_STRUCTURES+1,
		MAP_OPTIONS_WIDGET_WORM_COUNT_0, MAP_OPTIONS_WIDGET_WORM_COUNT_0+1, MAP_OPTIONS_WIDGET_WORM_COUNT_0+2, MAP_OPTIONS_WIDGET_WORM_COUNT_0+3,
		MAP_OPTIONS_WIDGET_SEED_RANDOM,	MAP_OPTIONS_WIDGET_SEED_RANDOM+1, MAP_OPTIONS_WIDGET_SEED_RANDOM+2, -1};
	for (int i = 0; widgetIDs[i] > 0; i++)
		GUI_Widget_Get_ByIndex(map_options_lobby_widgets, widgetIDs[i])->flags.invisible = MapOptionsLobby_IsReadOnly();

	/* Update the state and position of the Previous/Cancel and Apply buttons. */
	int backString = MapOptionsLobby_IsReadOnly()? STR_PREVIOUS: STR_CANCEL;
	const int width = Font_GetStringWidth(String_Get_ByIndex(backString)) + 2 * 12
			+ ((MapOptionsLobby_IsReadOnly())? 0: Font_GetStringWidth(String_Get_ByIndex(STR_APPLY)) + 2 * 12);
	const int leftmost_button_x = 9 + (304 - width) / 2;
	Widget *widgetBack = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 1);
	widgetBack->offsetX = leftmost_button_x;
	widgetBack->offsetY = MAP_OPTIONS_GUI_MAIN_Y + 7 * MAP_OPTIONS_GUI_LINE_HEIGHT + 11;
	widgetBack->stringID = backString;
	widgetBack->width  = Font_GetStringWidth(String_Get_ByIndex(backString)) + 12;

	Widget *widgetApply = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 2);
	widgetApply->offsetX = leftmost_button_x + Font_GetStringWidth(String_Get_ByIndex(backString)) + 30;
	widgetApply->offsetY = MAP_OPTIONS_GUI_MAIN_Y + 7 * MAP_OPTIONS_GUI_LINE_HEIGHT + 11;
	widgetApply->flags.greyWhenInvisible = !MapOptionsLobby_IsReadOnly();

	MapOptionsLobby_ChangeSeedMode(map_options_seed_mode);
	MapOptionsLobby_ChangeLoseCondition(map_options_lose_condition);
	MapOptionsLobby_ChangeWormCount(map_options_worm_count);
}

static int
MapOptionsLobby_HandleMapAndMessages(bool sendScenario)
{
	bool is_multiplayer = (g_campaign_selected == CAMPAIGNID_MULTIPLAYER);
	if (is_multiplayer) {
		enum NetEvent e = NETEVENT_NORMAL;
		if (Net_HasServerRole()) {
			Lobby_UpdateMap();
			g_sendScenario = sendScenario;
			e = Lobby_HandleMessages();
		} else {
			e = Lobby_HandleMessages();
			MapOptionsLobby_UpdateReadOnlyView();
			Lobby_UpdateMap();
			g_sendScenario = sendScenario;
		}
		if (e == NETEVENT_DISCONNECT) {
			return MENU_NO_TRANSITION | MENU_PICK_LOBBY;
		} else if (e == NETEVENT_START_GAME) {
			return MENU_PLAY_MULTIPLAYER;
		}
	} else {
		Lobby_UpdateMap();
	}

	return MENU_MAP_OPTIONS;
}

enum MenuAction
MapOptionsLobby_Loop(void)
{
	bool is_multiplayer = (g_campaign_selected == CAMPAIGNID_MULTIPLAYER);
	int result = MapOptionsLobby_HandleMapAndMessages(false);
	if (result != MENU_MAP_OPTIONS) return result;
	Widget *w;
	int editbox = 0;
	LandscapeGeneratorParams *params = is_multiplayer ? &g_multiplayer.landscape_params: &g_skirmish.landscape_params;

	if ((w = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 24))->state.selected) {
		/* EditBox: Starting credits. */
		editbox = GUI_EditBox(map_options_starting_credits, sizeof(map_options_starting_credits), w, EDITBOX_PORT);
		uint16 credits = clamp(MAP_OPTIONS_STARTING_CREDITS_MIN, atoi(map_options_starting_credits), MAP_OPTIONS_STARTING_CREDITS_MAX);
		if (is_multiplayer) {
			g_multiplayer.credits = credits;
		} else {
			g_skirmish.credits = credits;
		}
	} else if ((w = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 23))->state.selected) {
		/* EditBox: Fixed seed. */
		const uint32 override_seed_old = atoi(map_options_fixed_seed);
		editbox = GUI_EditBox(map_options_fixed_seed, sizeof(map_options_fixed_seed), w, EDITBOX_PORT);
		const uint32 override_seed_new = atoi(map_options_fixed_seed);
		if (override_seed_new != override_seed_old)
			MapOptionsLobby_ChangeSeedMode(MAP_SEED_MODE_FIXED);

	} else if ((w = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 3))->state.selected) {
		/* EditBox: Minimum spice fields. */
		editbox = GUI_EditBox(map_options_spice_fields_min, sizeof(map_options_spice_fields_min), w, EDITBOX_WIDTH_LIMITED);
		if ((int)params->min_spice_fields != atoi(map_options_spice_fields_min)) {
			params->min_spice_fields = atoi(map_options_spice_fields_min);
			lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
		}

	} else if ((w = GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 4))->state.selected) {
		/* EditBox: Maximum spice fields. */
		editbox = GUI_EditBox(map_options_spice_fields_max, sizeof(map_options_spice_fields_max), w, EDITBOX_WIDTH_LIMITED);
		if ((int)params->max_spice_fields != atoi(map_options_spice_fields_max)) {
			params->max_spice_fields = atoi(map_options_spice_fields_max);
			lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
		}
	}

	if (editbox == SCANCODE_ENTER || editbox == SCANCODE_ESCAPE)
		GUI_Widget_MakeNormal(w, false);

	Audio_PlayMusicIfSilent(MUSIC_MAIN_MENU);

	int widgetID = GUI_Widget_HandleEvents(map_options_lobby_widgets);

	if (widgetID == 0x8001) { /* Cancel was pressed. */
		GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 1), false);
		g_skirmish = map_options_saved_skirmish;
		g_multiplayer = map_options_saved_multiplayer;
		map_options_seed_mode = map_options_saved_seed_mode;
		map_options_lose_condition = map_options_saved_lose_condition;
		map_options_worm_count = map_options_saved_worm_count;
		return MENU_NO_TRANSITION | (is_multiplayer ? MENU_MULTIPLAYER_LOBBY: MENU_SKIRMISH_LOBBY);
	}

	if (widgetID == 0x8002) { /* Apply was pressed. */
		GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 2), false);
		g_multiplayer.seed_mode = map_options_seed_mode;
		g_multiplayer.worm_count = map_options_worm_count;
		g_skirmish.worm_count = map_options_worm_count;
		enhancement_fog_of_war = (GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 5)->state.selected != 0);
		enhancement_insatiable_sandworms = (GUI_Widget_Get_ByIndex(map_options_lobby_widgets, 6)->state.selected != 0);
		int result = MapOptionsLobby_HandleMapAndMessages(true);
		if (result == MENU_MAP_OPTIONS) {
			result = MENU_NO_TRANSITION | (is_multiplayer ? MENU_MULTIPLAYER_LOBBY: MENU_SKIRMISH_LOBBY);
		}
		return result;
	}

	if (widgetID == 0x8009) {
		if (is_multiplayer) {
			if ((lobby_map_generator_mode == MAP_GENERATOR_STOP) && Net_HasServerRole()) {
				lobby_map_generator_mode = MAP_GENERATOR_TRY_RAND_ELSE_RAND;
				MapOptionsLobby_ChangeSeedMode(MAP_SEED_MODE_RANDOM);
			}
		} else {
			lobby_map_generator_mode = MAP_GENERATOR_TRY_RAND_ELSE_RAND;
			MapOptionsLobby_ChangeSeedMode(MAP_SEED_MODE_RANDOM);
		}
	}

	return MENU_REDRAW | MENU_MAP_OPTIONS;
}

static void
SkirmishLobby_InitWidgets(void)
{
	Widget *w;

	const int button_previous_width = Font_GetStringWidth(String_Get_ByIndex(STR_PREVIOUS));
	const int button_map_options_width = Font_GetStringWidth(String_Get_ByIndex(STR_MAP_OPTIONS));
	const int button_start_width = Font_GetStringWidth(String_Get_ByIndex(STR_START_GAME));
	const int width = 6 + max(button_previous_width, max(button_map_options_width, button_start_width));
	const int space_between_buttons = 16;
	const int leftmost_button_x = 6 + (312 - (3 * width + 2 * space_between_buttons)) / 2;

	/* Previous. */
	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, leftmost_button_x, 178, 0xFFFE, STR_PREVIOUS);
	w->width  = width;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	skirmish_lobby_widgets = GUI_Widget_Link(skirmish_lobby_widgets, w);

	/* Map options. */
	w = GUI_Widget_Allocate(20, 0, leftmost_button_x + width + space_between_buttons, 178, 0xFFFE, STR_MAP_OPTIONS);
	w->width  = width;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	skirmish_lobby_widgets = GUI_Widget_Link(skirmish_lobby_widgets, w);

	/* Start Game. */
	w = GUI_Widget_Allocate(2, 0, leftmost_button_x + 2 * (width + space_between_buttons), 178, 0xFFFE, STR_START_GAME);
	w->width  = width;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.greyWhenInvisible = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	skirmish_lobby_widgets = GUI_Widget_Link(skirmish_lobby_widgets, w);

	/* Regenerate map. */
	w = GUI_Widget_Allocate(9, 0, 181, 98, SHAPE_INVALID, STR_NULL);
	w->width = 62;
	w->height = 62;
	skirmish_lobby_widgets = GUI_Widget_Link(skirmish_lobby_widgets, w);

	/* House allegiance selection. */
	skirmish_lobby_widgets = Scrollbar_Allocate(skirmish_lobby_widgets, 0, 30, 0, 74, false);
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 15));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 16));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 17));

	w = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 3);
	w->width = 92;

	WidgetScrollbar *ws = w->data;
	ws->itemHeight = 14;
}

static void
MultiplayerLobby_InitWidgets(void)
{
	Widget *w;

	const int button_previous_width = Font_GetStringWidth(String_Get_ByIndex(STR_PREVIOUS));
	const int button_map_options_width = Font_GetStringWidth(String_Get_ByIndex(STR_MAP_OPTIONS));
	const int button_start_width = Font_GetStringWidth(String_Get_ByIndex(STR_START_GAME));
	const int width = 6 + max(button_previous_width, max(button_map_options_width, button_start_width));
	const int space_between_buttons = 16;
	const int leftmost_button_x = 6 + (312 - (3 * width + 2 * space_between_buttons)) / 2;

	/* Previous. */
	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, leftmost_button_x, 178, 0xFFFE, STR_PREVIOUS);
	w->width  = width;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	multiplayer_lobby_widgets = GUI_Widget_Link(multiplayer_lobby_widgets, w);

	/* Map options. */
	w = GUI_Widget_Allocate(20, 0, leftmost_button_x + width + space_between_buttons, 178, 0xFFFE, STR_MAP_OPTIONS);
	w->width  = width;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	multiplayer_lobby_widgets = GUI_Widget_Link(multiplayer_lobby_widgets, w);

	/* Start Game. */
	w = GUI_Widget_Allocate(2, 0, leftmost_button_x + 2 * (width + space_between_buttons), 178, 0xFFFE, STR_START_GAME);
	w->width  = width;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.greyWhenInvisible = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	multiplayer_lobby_widgets = GUI_Widget_Link(multiplayer_lobby_widgets, w);

	/* Name entry. */
	w = GUI_Widget_Allocate(8, 0, 230, 87, 0xFFFE, STR_NULL);
	w->width  = 74;
	w->height = 12;
	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawWithBorder;
	w->drawParameterSelected.proc = EditBox_DrawWithBorder;
	w->drawParameterDown.proc = EditBox_DrawWithBorder;
	w->data = g_net_name;
	multiplayer_lobby_widgets = GUI_Widget_Link(multiplayer_lobby_widgets, w);

	/* Regenerate map. */
	w = GUI_Widget_Allocate(9, 0, 129, 98, SHAPE_INVALID, STR_NULL);
	w->width = 62;
	w->height = 62;
	multiplayer_lobby_widgets = GUI_Widget_Link(multiplayer_lobby_widgets, w);

	/* House selection. */
	multiplayer_lobby_widgets = Scrollbar_Allocate(multiplayer_lobby_widgets, 0, -6, 0, 74, false);
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 15));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 16));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 17));

	w = GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 3);
	w->width = 100;

	WidgetScrollbar *ws = w->data;
	ws->itemHeight = 14;
}

void
Lobby_InitWidgets(void)
{
	PickLobby_InitWidgets();
	MapOptionsLobby_InitWidgets();
	SkirmishLobby_InitWidgets();
	MultiplayerLobby_InitWidgets();
}

void
Lobby_FreeWidgets(void)
{
	Menu_FreeWidgets(pick_lobby_widgets);
	Menu_FreeWidgets(skirmish_lobby_widgets);
	Menu_FreeWidgets(multiplayer_lobby_widgets);
	Menu_FreeWidgets(map_options_lobby_widgets);
}

/*--------------------------------------------------------------*/

void
PickLobby_Draw(void)
{
	GUI_HallOfFame_SetColourScheme(true);
	HallOfFame_DrawBackground(HOUSE_INVALID, HALLOFFAMESTYLE_CLEAR_BACKGROUND);

	{
		unsigned char colours[16];
		memset(colours, 0, sizeof(colours));

		for (int i = 0; i < 6; i++)
			colours[i + 1] = 215 + i;

		GUI_InitColors(colours, 0, 15);
		g_fontCharOffset = 0;
	}

	// make texts shadowed
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x1);

	GUI_DrawText_Wrapper("Skirmish and Multiplayer", SCREEN_WIDTH / 2, 15, 15, 0, 0x122);

	/* Skirmish */
	Prim_DrawBorder( 11, 86, 102, 83, 1, false, false, 3);
	GUI_DrawText_Wrapper("Skirmish", 61, 96, 0xF, 0, 0x122);

	Shape_Draw(SHAPE_INFANTRY, 46, 118, WINDOWID_MAINMENU_FRAME, 0);
	Prim_DrawBorder(46, 118, 32, 24, 1, false, false, 4);

	/* Multiplayer. */
	Prim_DrawBorder(120, 86, 189, 83, 1, false, false, 3);
	GUI_DrawText("Multiplayer", 186, 96, 0xF, 0);
	GUI_DrawText_Wrapper("Address", 127, 119, 0xF, 0, 0x21);

	GUI_Widget_DrawAll(pick_lobby_widgets);

	GUI_HallOfFame_SetColourScheme(false);

	/* XXX -- GUI_Widget_TextButton2_Draw sets the shortcuts in case
	 * strings change: e.g. a (attack), then c (cancel).
	 * Find other use cases.
	 */
	GUI_Widget_Get_ByIndex(pick_lobby_widgets, 1)->shortcut = SCANCODE_ESCAPE;
}

enum MenuAction
PickLobby_Loop(void)
{
	int widgetID = 0;
	int editbox = 0;
	Widget *w;

	Audio_PlayMusicIfSilent(MUSIC_MAIN_MENU);

	if ((w = GUI_Widget_Get_ByIndex(pick_lobby_widgets, 21))->state.selected) {
		editbox = GUI_EditBox(g_host_addr, sizeof(g_host_addr), w, EDITBOX_ADDRESS);

		if (editbox == SCANCODE_ENTER)
			GUI_Widget_MakeSelected(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 22), false);
	} else if ((w = GUI_Widget_Get_ByIndex(pick_lobby_widgets, 22))->state.selected) {
		editbox = GUI_EditBox(g_host_port, sizeof(g_host_port), w, EDITBOX_PORT);
	} else if ((w = GUI_Widget_Get_ByIndex(pick_lobby_widgets, 31))->state.selected) {
		editbox = GUI_EditBox(g_join_addr, sizeof(g_join_addr), w, EDITBOX_ADDRESS);

		if (editbox == SCANCODE_ENTER)
			GUI_Widget_MakeSelected(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 32), false);
	} else if ((w = GUI_Widget_Get_ByIndex(pick_lobby_widgets, 32))->state.selected) {
		editbox = GUI_EditBox(g_join_port, sizeof(g_join_port), w, EDITBOX_PORT);
	} else {
		widgetID = GUI_Widget_HandleEvents(pick_lobby_widgets);
	}

	if (editbox == SCANCODE_ENTER || editbox == SCANCODE_ESCAPE)
		GUI_Widget_MakeNormal(w, false);

	switch (widgetID) {
		case 0x8000 | 1: /* exit. */
			GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 1), false);
			return MENU_MAIN_MENU;

		case 0x8000 | 10: /* skirmish. */
			GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 10), false);
			return MENU_NO_TRANSITION | MENU_SKIRMISH_LOBBY;

		case 0x8000 | 20: /* host */
			GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 20), false);
			if (Net_CreateServer(g_host_addr, atoi(g_host_port), g_net_name))
				return MENU_NO_TRANSITION | MENU_MULTIPLAYER_LOBBY;
			break;

		case 0x8000 | 30: /* join */
			GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 30), false);
			if (Net_ConnectToServer(g_join_addr, atoi(g_join_port), g_net_name))
				return MENU_NO_TRANSITION | MENU_MULTIPLAYER_LOBBY;
			break;

		default:
			break;
	}

	return MENU_REDRAW | MENU_PICK_LOBBY;
}

/*--------------------------------------------------------------*/

static void
Lobby_DrawRadar(Widget *w, uint32 next_seed)
{
	static uint32 l_seed = 0xFFFF;
	const bool is_skirmish = (g_campaign_selected == CAMPAIGNID_SKIRMISH);
	const uint32 seed = is_skirmish ? g_skirmish.seed : g_multiplayer.test_seed;

	if (l_seed != seed) {
		l_seed = seed;

		const bool radar_animating
			= (Timer_GetTicks() - lobby_radar_timer)
			< (RADAR_ANIMATION_FRAME_COUNT * RADAR_ANIMATION_DELAY);

		if (!radar_animating) {
			lobby_radar_timer = Timer_GetTicks();
			Audio_PlaySound(SOUND_RADAR_STATIC);
		}
	}

	const int x1 = GUI_Widget_Get_ByIndex(w, 9)->offsetX + 1;
	const int y1 = GUI_Widget_Get_ByIndex(w, 9)->offsetY - 1;
	const int x2 = x1 + 63;
	const int y2 = y1 + 63;

	Prim_FillRect_i(x1, y1, x2, y2, 12);

	const int64_t ticks = Timer_GetTicks() - lobby_radar_timer;
	if (ticks < RADAR_ANIMATION_FRAME_COUNT * RADAR_ANIMATION_DELAY) {
		int frame = max(0, ticks / RADAR_ANIMATION_DELAY);
		if (map_options_seed_mode == MAP_SEED_MODE_SURPRISE) {
			/* Play the animation in reverse order, i.e. the radar shuts down. */
			frame = RADAR_ANIMATION_FRAME_COUNT - frame - 1;
		}

		GUI_DrawText_Wrapper("Generating", x1 + 32, y1 - 8, 31, 0, 0x111);
		VideoA5_DrawWSAStatic(RADAR_ANIMATION_FRAME_COUNT - frame - 1, x1, y1);
	} else {
		if (map_options_seed_mode != MAP_SEED_MODE_SURPRISE) {
			GUI_DrawText_Wrapper("Map %u", x1 + 32, y1 - 8, 31, 0, 0x111, next_seed);
			Video_DrawMinimap(x1 + 1, y1 + 1, 0, MINIMAP_RESTORE);
		} else {
			GUI_DrawText_Wrapper("Map hidden", x1 + 32, y1 - 8, 31, 0, 0x111);
		}
	}

	uint16 credits = is_skirmish ? g_skirmish.credits : g_multiplayer.credits;
	credits = clamp(MAP_OPTIONS_STARTING_CREDITS_MIN, credits, MAP_OPTIONS_STARTING_CREDITS_MAX);
	if (w != map_options_lobby_widgets) {
		GUI_DrawText_Wrapper("Credits: %u", x1 + 30, y1 + 66, 31, 0, 0x111, credits);
	}
}

void
SkirmishLobby_Initialise(void)
{
	g_campaign_selected = CAMPAIGNID_SKIRMISH;
	Campaign_Load();

	lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;

	Widget *w = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	ws->scrollMax = 0;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		si = Scrollbar_AllocItem(w, SCROLLBAR_PLAYER_CONFIG);
		si->d.player_config = &g_skirmish.player_config[h];
		snprintf(si->text, sizeof(si->text), "%s", g_table_houseInfo[h].name);
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);

	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 1), false);
	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 2), false);
}

static void
Lobby_Draw(const char *heading, uint32 next_seed, Widget *w)
{
	HallOfFame_DrawBackground(HOUSE_INVALID, HALLOFFAMESTYLE_CLEAR_BACKGROUND);
	GUI_DrawText_Wrapper(heading, SCREEN_WIDTH / 2, 15, 15, 0, 0x122);

	Lobby_DrawRadar(w, next_seed);

	GUI_DrawText_Wrapper(NULL, 0, 0, 15, 0, 0x11);
	GUI_Widget_DrawAll(w);

	/* XXX -- GUI_Widget_TextButton2_Draw sets the shortcuts in case
	 * strings change: e.g. a (attack), then c (cancel).
	 * Find other use cases.
	 */
	GUI_Widget_Get_ByIndex(w, 1)->shortcut = SCANCODE_ESCAPE;
	GUI_Widget_Get_ByIndex(w, 2)->shortcut = 0;
}

void
MapOptionsLobby_Draw(void)
{
	const bool is_skirmish = (g_campaign_selected == CAMPAIGNID_SKIRMISH);
	const int lineHeight = MAP_OPTIONS_GUI_LINE_HEIGHT;
	bool issuesFound = false;
	int offsetY;

	GUI_HallOfFame_SetColourScheme(true);
	HallOfFame_DrawBackground(HOUSE_INVALID, HALLOFFAMESTYLE_CLEAR_BACKGROUND);
	if (is_skirmish) {
		GUI_DrawText_Wrapper("Skirmish", SCREEN_WIDTH / 2, 15, 15, 0, 0x122);
	} else {
		GUI_DrawText_Wrapper("Multiplayer", SCREEN_WIDTH / 2, 15, 15, 0, 0x122);
	}

	/* Starting credits */
	Prim_DrawBorder(MAP_OPTIONS_GUI_MAIN_X - 2, MAP_OPTIONS_GUI_MAIN_Y - 2,
			MAP_OPTIONS_GUI_MAIN_W + 4, MAP_OPTIONS_GUI_MAIN_H + 4, 1, false, false, 4);
	GUI_DrawText_Wrapper("Starting credits:", MAP_OPTIONS_GUI_MAIN_X, MAP_OPTIONS_GUI_MAIN_Y, 0xF, 0, 0x22);
	uint16 credits_proposed = atoi(map_options_starting_credits);
	if (credits_proposed < MAP_OPTIONS_STARTING_CREDITS_MIN) {
		GUI_DrawText_Wrapper("x", MAP_OPTIONS_GUI_MAIN_X + 52, MAP_OPTIONS_GUI_MAIN_Y + 1*lineHeight + 2, 0xE7, 0, 0x21);
		GUI_DrawText_Wrapper("Assign at least %u credits", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y, 0xE7, 0, 0x122, MAP_OPTIONS_STARTING_CREDITS_MIN);
		issuesFound = true;
	}
	if (credits_proposed > MAP_OPTIONS_STARTING_CREDITS_MAX) {
		GUI_DrawText_Wrapper("x", MAP_OPTIONS_GUI_MAIN_X + 52, MAP_OPTIONS_GUI_MAIN_Y + 1*lineHeight + 2, 0xE7, 0, 0x21);
		if (!issuesFound) {
			GUI_DrawText_Wrapper("Assign at most %u credits", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y, 0xE7, 0, 0x122, MAP_OPTIONS_STARTING_CREDITS_MAX);
			issuesFound = true;
		}
	}

	/* Win Condition */
	offsetY = MAP_OPTIONS_GUI_MAIN_Y + 6;
	GUI_DrawText_Wrapper("Lose condition:", MAP_OPTIONS_GUI_MAIN_X, offsetY + 2*lineHeight, 0xF, 0, 0x22);
	GUI_DrawText_Wrapper("Structures", MAP_OPTIONS_GUI_MAIN_X + 10, offsetY + 3*lineHeight + 2, (map_options_lose_condition == MAP_LOSE_CONDITION_STRUCTURES) ? 0x8: 0xF, 0, 0x21);
	GUI_DrawText_Wrapper("Units", MAP_OPTIONS_GUI_MAIN_X + 78, offsetY + 3*lineHeight + 2, (map_options_lose_condition == MAP_LOSE_CONDITION_UNITS) ? 0x8: 0xF, 0, 0x21);

	/* Enhancement: Fog of War */
	offsetY = MAP_OPTIONS_GUI_MAIN_Y + 12;
	GUI_DrawText_Wrapper("Fog of War", MAP_OPTIONS_GUI_MAIN_X + 12, offsetY + 4*lineHeight, 0xF, 0, 0x22);
	offsetY = MAP_OPTIONS_GUI_MAIN_Y + 16;
	GUI_DrawText_Wrapper("Insatiable worms", MAP_OPTIONS_GUI_MAIN_X + 12, offsetY + 5*lineHeight, 0xF, 0, 0x22);

	/* Map and map options */
	Prim_DrawBorder(MAP_OPTIONS_GUI_MAP_X - 2, MAP_OPTIONS_GUI_MAP_Y - 2,
			MAP_OPTIONS_GUI_MAP_W + 4, MAP_OPTIONS_GUI_MAP_H + 4, 1, false, false, 4);
	const int mapOffsetY[] = {0, 13, 26, 39, 51, 64, 76};

	/* Map Seed */
	GUI_DrawText_Wrapper("Map seed:", MAP_OPTIONS_GUI_MAP_X, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[0], 0xF, 0, 0x22);
	GUI_DrawText_Wrapper("Random", MAP_OPTIONS_GUI_MAP_X + 16, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[1],
			(map_options_seed_mode == MAP_SEED_MODE_RANDOM) ? 0x8: 0xF, 0, 0x21);
	GUI_DrawText_Wrapper("Fixed:", MAP_OPTIONS_GUI_MAP_X + 16, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[2],
			(map_options_seed_mode == MAP_SEED_MODE_FIXED) ? 0x8: 0xF, 0, 0x21);
	GUI_DrawText_Wrapper("Surprise me!", MAP_OPTIONS_GUI_MAP_X + 16, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[3],
			(map_options_seed_mode == MAP_SEED_MODE_SURPRISE) ? 0x8: 0xF, 0, 0x21);

	const uint32 override_seed = atoi(map_options_fixed_seed);
	const uint32 current_seed = is_skirmish ? g_skirmish.seed : g_multiplayer.test_seed;
	if ((map_options_seed_mode == MAP_SEED_MODE_FIXED) && (override_seed != current_seed)) {
		/* Show an error indication, because the requested seed could not be applied.*/
		GUI_DrawText_Wrapper("x", MAP_OPTIONS_GUI_MAP_X + 97, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[2], 0xE7, 0, 0x21);
		if (!issuesFound) {
			GUI_DrawText_Wrapper("Map rejected. Enough build space?",
					MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y, 0xE7, 0, 0x122, MAP_OPTIONS_STARTING_CREDITS_MAX);
			issuesFound = true;
		}
	}

	/* Spice fields */
	GUI_DrawText_Wrapper("Spice fields:", MAP_OPTIONS_GUI_MAP_X, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[4], 0xF, 0, 0x22);
	GUI_DrawText_Wrapper("Min:", MAP_OPTIONS_GUI_MAP_X, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[5], 0xF, 0, 0x21);
	GUI_DrawText_Wrapper("Max:", MAP_OPTIONS_GUI_MAP_X + 55, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[5], 0xF, 0, 0x21);
	const int spice_min = atoi(map_options_spice_fields_min);
	if (spice_min < MAP_OPTIONS_SPICE_MIN) {
		GUI_DrawText_Wrapper("x", MAP_OPTIONS_GUI_MAP_X + 110, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[5], 0xE7, 0, 0x21);
		if (!issuesFound) {
			GUI_DrawText_Wrapper("Enter at least %u of min.", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_1, 0xE7, 0, 0x122, MAP_OPTIONS_SPICE_MIN);
			GUI_DrawText_Wrapper("spice field spawn points", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_2, 0xE7, 0, 0x122);
			issuesFound = true;
		}
	}
	const int spice_max = atoi(map_options_spice_fields_max);
	if (spice_max > MAP_OPTIONS_SPICE_MAX) {
		GUI_DrawText_Wrapper("x", MAP_OPTIONS_GUI_MAP_X + 110, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[5], 0xE7, 0, 0x21);
		if (!issuesFound) {
			GUI_DrawText_Wrapper("Enter at most %u of max.", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_1, 0xE7, 0, 0x122, MAP_OPTIONS_SPICE_MAX);
			GUI_DrawText_Wrapper("spice field spawn points", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_2, 0xE7, 0, 0x122);
			issuesFound = true;
		}
	}
	if (spice_min > spice_max) {
		GUI_DrawText_Wrapper("x", MAP_OPTIONS_GUI_MAP_X + 110, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[5], 0xE7, 0, 0x21);
		if (!issuesFound) {
			GUI_DrawText_Wrapper("Min. spice larger than max.", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y, 0xE7, 0, 0x122);
			issuesFound = true;
		}
	}

	/* Worms */
	GUI_DrawText_Wrapper("Worms:", MAP_OPTIONS_GUI_MAP_X, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[6], 0xF, 0, 0x22);
	GUI_DrawText_Wrapper("0", MAP_OPTIONS_GUI_MAP_X + 55, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[6]+2, (map_options_worm_count == MAP_WORM_COUNT_0) ? 0x8: 0xF, 0, 0x21);
	GUI_DrawText_Wrapper("1", MAP_OPTIONS_GUI_MAP_X + 55 + 22, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[6]+2, (map_options_worm_count == MAP_WORM_COUNT_1) ? 0x8: 0xF, 0, 0x21);
	GUI_DrawText_Wrapper("2", MAP_OPTIONS_GUI_MAP_X + 55 + 22*2, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[6]+2, (map_options_worm_count == MAP_WORM_COUNT_2) ? 0x8: 0xF, 0, 0x21);
	GUI_DrawText_Wrapper("3", MAP_OPTIONS_GUI_MAP_X + 55 + 22*3, MAP_OPTIONS_GUI_MAP_Y + mapOffsetY[6]+2, (map_options_worm_count == MAP_WORM_COUNT_3) ? 0x8: 0xF, 0, 0x21);


	const uint32 next_seed = is_skirmish ? g_skirmish.seed : g_multiplayer.next_seed;
	Lobby_DrawRadar(map_options_lobby_widgets, next_seed);

	bool enableApplyButton = !MapOptionsLobby_IsReadOnly() && (lobby_map_generator_mode == MAP_GENERATOR_STOP) && !issuesFound;
	Lobby_ShowHideStartButton(map_options_lobby_widgets, enableApplyButton);

	GUI_Widget_DrawAll(map_options_lobby_widgets);

	GUI_HallOfFame_SetColourScheme(false);
}

void
SkirmishLobby_Draw(void)
{
	GUI_HallOfFame_SetColourScheme(true);

	Lobby_Draw("Skirmish",
			g_skirmish.seed,
			skirmish_lobby_widgets);

	if (!Skirmish_IsPlayable()) {
		GUI_DrawText_Wrapper("Choose your House", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_1, 0xE7, 0, 0x122);
		GUI_DrawText_Wrapper("and at least 1 enemy", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_2, 0xE7, 0, 0x122);
	}

	GUI_HallOfFame_SetColourScheme(false);
}

enum MenuAction
SkirmishLobby_Loop(void)
{
	Lobby_UpdateMap();

	int widgetID = GUI_Widget_HandleEvents(skirmish_lobby_widgets);

	Widget *w;

	w = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 15);
	if ((widgetID & 0x8000) == 0)
		Scrollbar_HandleEvent(w, widgetID);

	Audio_PlayMusicIfSilent(MUSIC_MAIN_MENU);

	switch (widgetID) {
		case 0x8000 | 1: /* exit. */
			return MENU_NO_TRANSITION | MENU_PICK_LOBBY;

		case 0x8000 | 2: /* start game. */
			if (SkirmishLobby_IsPlayable()) {
				g_playerHouseID = HOUSE_INVALID;

				for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
					if (g_skirmish.player_config[h].brain == BRAIN_HUMAN) {
						g_playerHouseID = h;
						break;
					}
				}

				assert(g_playerHouseID != HOUSE_INVALID);
				return MENU_PLAY_SKIRMISH;
			}
			break;

		case 0x8000 | 3: /* list entry. */
		case SCANCODE_ENTER:
		case SCANCODE_KEYPAD_4:
		case SCANCODE_KEYPAD_5:
		case SCANCODE_KEYPAD_6:
		case SCANCODE_SPACE:
			{
				w = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 3);
				ScrollbarItem *si = Scrollbar_GetSelectedItem(w);
				PlayerConfig *pc = si->d.player_config;
				enum Brain new_brain = pc->brain;
				enum PlayerTeam new_team = pc->team;				
				bool clickedOnTeam = g_mouseX < w->offsetX + 14;

				if (clickedOnTeam) {
					const int change_team = (widgetID == SCANCODE_KEYPAD_4) | Input_Test(SCANCODE_MOUSE_RMB) ? -1 : 1;
					new_team = ((new_team + change_team) % (TEAM_6 + 1));
					if (new_team == TEAM_NONE)
						new_team = Input_Test(SCANCODE_MOUSE_RMB) ? TEAM_6 : TEAM_1;
				} else {
					const int change_player = (widgetID == SCANCODE_KEYPAD_4) ? -1 : 1;

					new_brain = ((new_brain + change_player) % (BRAIN_CPU + 1));

					/* Skip over human player if one is already selected. */
					for (enum HouseType h = HOUSE_HARKONNEN; (new_brain == BRAIN_HUMAN) && (h < HOUSE_MAX); h++) {
						if (g_skirmish.player_config[h].brain == BRAIN_HUMAN)
							new_brain = ((new_brain + change_player) % (BRAIN_CPU + 1));
					}
				}

				if (pc->brain != new_brain) {
					pc->brain = new_brain;
					lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
				}

				if (pc->team != new_team) {
					pc->team = new_team;
					lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
				}
			}
			break;

		case 0x8000 | 9:
			lobby_map_generator_mode = MAP_GENERATOR_TRY_RAND_ELSE_RAND;
			MapOptionsLobby_ChangeSeedMode(MAP_SEED_MODE_RANDOM);
			break;

		case 0x8000 | 20:
			GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 20), false);
			return MENU_NO_TRANSITION | MENU_MAP_OPTIONS;
	}

	return MENU_REDRAW | MENU_SKIRMISH_LOBBY;
}

/*--------------------------------------------------------------*/

void
MultiplayerLobby_Initialise(void)
{
	g_campaign_selected = CAMPAIGNID_MULTIPLAYER;
	Campaign_Load();

	if (Net_HasServerRole()) {
		lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
	} else {
		lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_STOP;
	}

	Widget *w = GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	ws->scrollMax = 0;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		si = Scrollbar_AllocItem(w, SCROLLBAR_PLAYER_CONFIG);
		si->d.player_config = &g_multiplayer.player_config[h];
		snprintf(si->text, sizeof(si->text), "%s", g_table_houseInfo[h].name);
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);

	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 1), false);
	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 2), false);

	g_chat_buf[0] = '\0';
}

void
MultiplayerLobby_Draw(void)
{

	GUI_HallOfFame_SetColourScheme(true);

	bool is_playable = Net_IsPlayable() && (lobby_map_generator_mode == MAP_GENERATOR_STOP);
	bool can_issue_start = Net_HasServerRole() && is_playable;

	Lobby_ShowHideStartButton(multiplayer_lobby_widgets, can_issue_start);

	Lobby_Draw("Multiplayer",
			g_multiplayer.next_seed,
			multiplayer_lobby_widgets);

	if (Net_GetClientHouse(g_local_client_id) == HOUSE_INVALID) {
		GUI_DrawText_Wrapper("Select your House!", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y, 0xE7, 0, 0x122);
	}	
	else if (!is_playable && Net_HasServerRole()) {
		if (!Net_HasAtLeastTwoPlayers()) {
			GUI_DrawText_Wrapper("At least 2 human", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_1, 0xE7, 0, 0x122);
			GUI_DrawText_Wrapper("players required", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_2, 0xE7, 0, 0x122);
		}
		else if (!Net_HasAllPlayersAssigned()) {
			GUI_DrawText_Wrapper("All players must", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_1, 0xE7, 0, 0x122);
			GUI_DrawText_Wrapper("select their House", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y_LINE_2, 0xE7, 0, 0x122);
		}
		else if (!Net_HasAtLeastTwoTeams()) {
			GUI_DrawText_Wrapper("At least 2 teams required", MAP_OPTIONS_GUI_ERROR_X, MAP_OPTIONS_GUI_ERROR_Y, 0xE7, 0, 0x122);
		}
	}

	ChatBox_Draw(g_chat_buf,
			!GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 8)->state.selected);

	GUI_DrawText_Wrapper("Name:", 200, 89, 0xF, 0, 0x21);

	/* XXX -- GUI_Widget_TextButton2_Draw changes the shortcuts. */
	GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 20)->shortcut = 0;

	GUI_HallOfFame_SetColourScheme(false);
}

enum MenuAction
MultiplayerLobby_Loop(void)
{
	enum NetEvent e = NETEVENT_NORMAL;
	if (Net_HasServerRole()) {
		Lobby_UpdateMap();
		e = Lobby_HandleMessages();
	} else {
		e = Lobby_HandleMessages();
		Lobby_UpdateMap();
	}
	if (e == NETEVENT_DISCONNECT) {
		return MENU_NO_TRANSITION | MENU_PICK_LOBBY;
	} else if (e == NETEVENT_START_GAME) {
		return MENU_PLAY_MULTIPLAYER;
	}

	int widgetID = 0;
	Widget *w;

	Audio_PlayMusicIfSilent(MUSIC_MAIN_MENU);

	if ((w = GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 8))->state.selected) {
		int editbox = GUI_EditBox(g_net_name, sizeof(g_net_name), w, EDITBOX_FREEFORM);

		if (editbox == SCANCODE_ENTER || editbox == SCANCODE_ESCAPE || !w->state.selected) {
			if (Client_Send_PrefName(g_net_name)) {
				GUI_Widget_MakeNormal(w, false);
			} else {
				GUI_Widget_MakeSelected(w, false);
			}
		}
	} else {
		widgetID = GUI_Widget_HandleEvents(multiplayer_lobby_widgets);

		w = GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 15);
		if ((widgetID & 0x8000) == 0)
			Scrollbar_HandleEvent(w, widgetID);
	}

	switch (widgetID) {
		case 0x8000 | 1: /* exit. */
			Net_Disconnect();
			return MENU_NO_TRANSITION | MENU_PICK_LOBBY;

		case 0x8000 | 2: /* start game. */
			if (Net_HasServerRole()) {
				if (Server_Send_StartGame())
					return MENU_PLAY_MULTIPLAYER;
			}
			break;

		case 0x8000 | 3: /* list entry. */
		case SCANCODE_KEYPAD_4:
		case SCANCODE_KEYPAD_5:
		case SCANCODE_KEYPAD_6:
			{
				w = GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 3);
				ScrollbarItem *si = Scrollbar_GetSelectedItem(w);
				PlayerConfig *pc = si->d.player_config;
				enum Brain new_brain = pc->brain;
				enum PlayerTeam new_team = pc->team;
				bool clickedOnTeam = g_mouseX < w->offsetX + 14;

				if (Net_HasServerRole() && clickedOnTeam) {
					const int change_team = (widgetID == SCANCODE_KEYPAD_4) | Input_Test(SCANCODE_MOUSE_RMB) ? -1 : 1;
					new_team = ((pc->team + change_team) % (TEAM_6 + 1));
					if (new_team == TEAM_NONE)
						new_team = Input_Test(SCANCODE_MOUSE_RMB) ? TEAM_6 : TEAM_1;

					if (pc->team != new_team) {
						pc->team = new_team;
						lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
					}
					break;
				}

				bool send_pref_house = true;
				const bool clientHasHouseSelected = Net_GetClientHouse(g_local_client_id) != HOUSE_INVALID;
				enum HouseType prefHouse = pc->houseID;

				if (Net_HasServerRole()) {
					if (pc->brain == BRAIN_CPU || Input_Test(SCANCODE_MOUSE_RMB)) {
						new_brain = BRAIN_NONE;
					}
					else if (pc->brain == BRAIN_NONE && !clientHasHouseSelected) {
						new_brain = BRAIN_HUMAN;
					}
					else {
						new_brain = BRAIN_CPU;
					}

					if (new_brain != BRAIN_HUMAN) {
						g_multiplayer.client[pc->houseID] = 0;
						pc->brain = new_brain;
						lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
						send_pref_house = false;
					}
				}
				else if (Input_Test(SCANCODE_MOUSE_RMB)) {
					prefHouse = HOUSE_INVALID;
				}
				else {
					prefHouse = pc->houseID;
				}
				
				if (send_pref_house) {
					Client_Send_PrefHouse(prefHouse);
				}
			}
			break;

		case SCANCODE_ENTER:
			Net_Send_Chat(g_chat_buf);
			g_chat_buf[0] = '\0';
			break;

		case 0x8000 | 9:
			if ((lobby_map_generator_mode == MAP_GENERATOR_STOP) && Net_HasServerRole()) {
				lobby_map_generator_mode = MAP_GENERATOR_TRY_RAND_ELSE_RAND;
			}
			break;

		case 0x8000 | 20:
			GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 20), false);
			return MENU_NO_TRANSITION | MENU_MAP_OPTIONS;

		default:
			EditBox_Input(g_chat_buf, sizeof(g_chat_buf), EDITBOX_FREEFORM, widgetID);
			break;

		case 0:
			break;
	}

	return MENU_REDRAW | MENU_MULTIPLAYER_LOBBY;
}
