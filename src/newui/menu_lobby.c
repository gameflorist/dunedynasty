/* menu_lobby.c */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "enum_string.h"
#include "../os/math.h"

#include "menu.h"

#include "chatbox.h"
#include "editbox.h"
#include "halloffame.h"
#include "scrollbar.h"
#include "../audio/audio.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../input/input.h"
#include "../input/mouse.h"
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

static char s_net_name[MAX_NAME_LEN + 1] = "Name";
static char s_host_addr[MAX_ADDR_LEN + 1] = "0.0.0.0";
static char s_host_port[MAX_PORT_LEN + 1] = DEFAULT_PORT_STR;
static char s_join_addr[MAX_ADDR_LEN + 1] = "localhost";
static char s_join_port[MAX_PORT_LEN + 1] = DEFAULT_PORT_STR;
static char s_chat_buf[MAX_CHAT_LEN + 1];

static Widget *pick_lobby_widgets;
static Widget *skirmish_lobby_widgets;
static Widget *multiplayer_lobby_widgets;
static int64_t lobby_radar_timer;
bool lobby_regenerate_map;

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
	w = GUI_Widget_Allocate(10, 0, 11 + (102 - width) / 2, 148, 0xFFFE, STR_LAUNCH);
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
	w->data = s_host_addr;
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
	w->data = s_host_port;
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
	w->data = s_join_addr;
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
	w->data = s_join_port;
	pick_lobby_widgets = GUI_Widget_Link(pick_lobby_widgets, w);
}

static void
SkirmishLobby_InitWidgets(void)
{
	Widget *w;

	const int width
		= 6 + max(Font_GetStringWidth(String_Get_ByIndex(STR_PREVIOUS)),
				Font_GetStringWidth(String_Get_ByIndex(STR_START_GAME)));

	/* Previous. */
	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, 160 - width - 18, 178, 0xFFFE, STR_PREVIOUS);
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
	w = GUI_Widget_Allocate(2, 0, 178, 178, 0xFFFE, STR_START_GAME);
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

	skirmish_lobby_widgets = Scrollbar_Allocate(skirmish_lobby_widgets, 0, 50, 0, 74, false);
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

	const int width
		= 6 + max(Font_GetStringWidth(String_Get_ByIndex(STR_PREVIOUS)),
				Font_GetStringWidth(String_Get_ByIndex(STR_START_GAME)));

	/* Previous. */
	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, 160 - width - 18, 178, 0xFFFE, STR_PREVIOUS);
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
	w = GUI_Widget_Allocate(2, 0, 178, 178, 0xFFFE, STR_START_GAME);
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
	w = GUI_Widget_Allocate(8, 0, 76, 42, 0xFFFE, STR_NULL);
	w->width = 168;
	w->height = 10;
	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;
	w->drawParameterNormal.proc = EditBox_DrawCentred;
	w->drawParameterSelected.proc = EditBox_DrawCentred;
	w->drawParameterDown.proc = EditBox_DrawCentred;
	w->data = s_net_name;
	multiplayer_lobby_widgets = GUI_Widget_Link(multiplayer_lobby_widgets, w);

	/* Regenerate map. */
	w = GUI_Widget_Allocate(9, 0, 129, 98, SHAPE_INVALID, STR_NULL);
	w->width = 62;
	w->height = 62;
	multiplayer_lobby_widgets = GUI_Widget_Link(multiplayer_lobby_widgets, w);

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
	SkirmishLobby_InitWidgets();
	MultiplayerLobby_InitWidgets();
}

void
Lobby_FreeWidgets(void)
{
	Menu_FreeWidgets(pick_lobby_widgets);
	Menu_FreeWidgets(skirmish_lobby_widgets);
	Menu_FreeWidgets(multiplayer_lobby_widgets);
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
		Font_Select(g_fontIntro);
		g_fontCharOffset = 0;
	}

	/* Skirmish */
	Prim_DrawBorder( 11, 86, 102, 83, 1, false, false, 3);
	GUI_DrawText("Skirmish", 25, 96, 144, 0);

	Shape_Draw(SHAPE_INFANTRY, 46, 118, WINDOWID_MAINMENU_FRAME, 0);
	Prim_DrawBorder(46, 118, 32, 24, 1, false, false, 4);

	/* Multiplayer. */
	Prim_DrawBorder(120, 86, 189, 83, 1, false, false, 3);
	GUI_DrawText("Multiplayer", 170, 96, 144, 0);

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x1);
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
		editbox = GUI_EditBox(s_host_addr, sizeof(s_host_addr), w, EDITBOX_ADDRESS);

		if (editbox == SCANCODE_ENTER)
			GUI_Widget_MakeSelected(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 22), false);
	}
	else if ((w = GUI_Widget_Get_ByIndex(pick_lobby_widgets, 22))->state.selected) {
		editbox = GUI_EditBox(s_host_port, sizeof(s_host_port), w, EDITBOX_PORT);
	}
	else if ((w = GUI_Widget_Get_ByIndex(pick_lobby_widgets, 31))->state.selected) {
		editbox = GUI_EditBox(s_join_addr, sizeof(s_join_addr), w, EDITBOX_ADDRESS);

		if (editbox == SCANCODE_ENTER)
			GUI_Widget_MakeSelected(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 32), false);
	}
	else if ((w = GUI_Widget_Get_ByIndex(pick_lobby_widgets, 32))->state.selected) {
		editbox = GUI_EditBox(s_join_port, sizeof(s_join_port), w, EDITBOX_PORT);
	}
	else {
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
			if (Net_CreateServer(s_host_addr, atoi(s_host_port), s_net_name))
				return MENU_NO_TRANSITION | MENU_MULTIPLAYER_LOBBY;
			break;

		case 0x8000 | 30: /* join */
			GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(pick_lobby_widgets, 30), false);
			if (Net_ConnectToServer(s_join_addr, atoi(s_join_port), s_net_name))
				return MENU_NO_TRANSITION | MENU_MULTIPLAYER_LOBBY;
			break;

		default:
			break;
	}

	return MENU_REDRAW | MENU_PICK_LOBBY;
}

/*--------------------------------------------------------------*/

static void
Lobby_ShowHideStartButton(Widget *w, bool show)
{
	w = GUI_Widget_Get_ByIndex(w, 2);

	if (show) {
		GUI_Widget_MakeVisible(w);
	}
	else {
		GUI_Widget_MakeInvisible(w);
	}
}

static void
Lobby_ResetRadarAnimation(void)
{
	lobby_radar_timer = Timer_GetTicks();
	Audio_PlaySound(SOUND_RADAR_STATIC);
}

static void
Lobby_RequestRegeneration(bool force)
{
	/* If radar animation is still going, do not restart it. */
	if (Timer_GetTicks() - lobby_radar_timer < RADAR_ANIMATION_FRAME_COUNT * RADAR_ANIMATION_DELAY) {
		if (force)
			lobby_regenerate_map = true;
	}
	else {
		lobby_regenerate_map = true;
		Lobby_ResetRadarAnimation();
	}
}

static bool
SkirmishLobby_IsPlayable(void)
{
	bool is_playable = Skirmish_IsPlayable();
	Lobby_ShowHideStartButton(skirmish_lobby_widgets, is_playable);
	return is_playable;
}

void
SkirmishLobby_Initialise(void)
{
	g_campaign_selected = CAMPAIGNID_SKIRMISH;
	Campaign_Load();

	Skirmish_GenerateMap(false);
	lobby_regenerate_map = false;
	SkirmishLobby_IsPlayable();

	Widget *w = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	ws->scrollMax = 0;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		si = Scrollbar_AllocItem(w, SCROLLBAR_BRAIN);
		si->d.brain = &g_skirmish.brain[h];
		snprintf(si->text, sizeof(si->text), "%s", g_table_houseInfo[h].name);
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);

	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 1), false);
	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 2), false);
}

static void
Lobby_Draw(const char *heading, uint32 seed, Widget *w)
{
	const int x1 = GUI_Widget_Get_ByIndex(w, 9)->offsetX - 1;
	const int y1 = GUI_Widget_Get_ByIndex(w, 9)->offsetY - 1;
	const int x2 = x1 + 63;
	const int y2 = y1 + 63;

	HallOfFame_DrawBackground(HOUSE_INVALID, HALLOFFAMESTYLE_CLEAR_BACKGROUND);
	Prim_FillRect_i(x1, y1, x2, y2, 12);
	GUI_DrawText_Wrapper(heading, SCREEN_WIDTH / 2, 15, 15, 0, 0x122);

	if (Timer_GetTicks() - lobby_radar_timer < RADAR_ANIMATION_FRAME_COUNT * RADAR_ANIMATION_DELAY) {
		const int frame = max(0, (Timer_GetTicks() - lobby_radar_timer) / RADAR_ANIMATION_DELAY);

		GUI_DrawText_Wrapper("Generating", x1 + 32, y1 - 8, 31, 0, 0x111);
		VideoA5_DrawWSAStatic(RADAR_ANIMATION_FRAME_COUNT - frame - 1, x1, y1);
	}
	else if (lobby_regenerate_map) {
		GUI_DrawText_Wrapper("Generating", x1 + 32, y1 - 8, 31, 0, 0x111);
		VideoA5_DrawWSAStatic(0, x1, y1);
	}
	else {
		GUI_DrawText_Wrapper("Map %u", x1 + 32, y1 - 8, 31, 0, 0x111, seed);
		Video_DrawMinimap(x1 + 1, y1 + 1, 0, 1);
	}

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
SkirmishLobby_Draw(void)
{
	GUI_HallOfFame_SetColourScheme(true);

	Lobby_Draw("Skirmish",
			g_skirmish.seed,
			skirmish_lobby_widgets);

	GUI_HallOfFame_SetColourScheme(false);
}

enum MenuAction
SkirmishLobby_Loop(void)
{
	int widgetID = GUI_Widget_HandleEvents(skirmish_lobby_widgets);
	bool conditions_changed = false;
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
					if (g_skirmish.brain[h] == BRAIN_HUMAN) {
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
				enum Brain new_brain = *(si->d.brain);

				if (Input_Test(MOUSE_RMB)) {
					new_brain = BRAIN_NONE;
				}
				else {
					const int change_player = (widgetID == SCANCODE_KEYPAD_4) ? -1 : 1;

					new_brain = ((new_brain + change_player) % (BRAIN_CPU_ALLY + 1));

					/* Skip over human player if one is already selected. */
					for (enum HouseType h = HOUSE_HARKONNEN; (new_brain == BRAIN_HUMAN) && (h < HOUSE_MAX); h++) {
						if (g_skirmish.brain[h] == BRAIN_HUMAN)
							new_brain = ((new_brain + change_player) % (BRAIN_CPU_ALLY + 1));
					}
				}

				if (*(si->d.brain) != new_brain) {
					*(si->d.brain) = new_brain;
					conditions_changed = true;

					if (!Skirmish_GenerateMap(false))
						Lobby_RequestRegeneration(true);
				}
			}
			break;

		case 0x8000 | 9:
			Lobby_RequestRegeneration(false);
			break;
	}

	if (lobby_regenerate_map) {
		conditions_changed = true;

		if (Skirmish_GenerateMap(true))
			lobby_regenerate_map = false;
	}

	if (conditions_changed) {
		SkirmishLobby_IsPlayable();
	}

	return MENU_REDRAW | MENU_SKIRMISH_LOBBY;
}

/*--------------------------------------------------------------*/

void
MultiplayerLobby_Initialise(void)
{
	g_campaign_selected = CAMPAIGNID_MULTIPLAYER;
	Campaign_Load();

	lobby_regenerate_map = false;

	Widget *w = GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	ws->scrollMax = 0;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		si = Scrollbar_AllocItem(w, SCROLLBAR_BRAIN);
		si->d.brain = &g_skirmish.brain[h];
		snprintf(si->text, sizeof(si->text), "%s", g_table_houseInfo[h].name);
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);

	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 1), false);
	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 2), false);

	s_chat_buf[0] = '\0';
}

void
MultiplayerLobby_Draw(void)
{
	bool can_issue_start = false;

	GUI_HallOfFame_SetColourScheme(true);

	Lobby_ShowHideStartButton(multiplayer_lobby_widgets, can_issue_start);

	Lobby_Draw("Multiplayer",
			0,
			multiplayer_lobby_widgets);

	ChatBox_Draw(s_chat_buf,
			!GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 8)->state.selected);

	GUI_HallOfFame_SetColourScheme(false);
}

enum MenuAction
MultiplayerLobby_Loop(void)
{
	if (g_host_type == HOSTTYPE_DEDICATED_SERVER
	 || g_host_type == HOSTTYPE_CLIENT_SERVER) {
		Server_SendMessages();
		Server_RecvMessages();
	}
	else {
		Client_SendMessages();
		enum NetEvent e = Client_RecvMessages();
		if (e == NETEVENT_DISCONNECT) {
			return MENU_NO_TRANSITION | MENU_PICK_LOBBY;
		}
	}

	int widgetID = 0;
	Widget *w;

	Audio_PlayMusicIfSilent(MUSIC_MAIN_MENU);

	if ((w = GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 8))->state.selected) {
		int editbox = GUI_EditBox(s_net_name, sizeof(s_net_name), w, EDITBOX_FREEFORM);

		if (editbox == SCANCODE_ENTER || editbox == SCANCODE_ESCAPE || !w->state.selected) {
			if (Client_Send_PrefName(s_net_name)) {
				GUI_Widget_MakeNormal(w, false);
			}
			else {
				GUI_Widget_MakeSelected(w, false);
			}
		}
	}
	else {
		widgetID = GUI_Widget_HandleEvents(multiplayer_lobby_widgets);

		w = GUI_Widget_Get_ByIndex(multiplayer_lobby_widgets, 15);
		if ((widgetID & 0x8000) == 0)
			Scrollbar_HandleEvent(w, widgetID);
	}

	switch (widgetID) {
		case 0x8000 | 1: /* exit. */
			Net_Disconnect();
			return MENU_NO_TRANSITION | MENU_PICK_LOBBY;

		case SCANCODE_ENTER:
			Net_Send_Chat(s_chat_buf);
			s_chat_buf[0] = '\0';
			break;

		default:
			EditBox_Input(s_chat_buf, sizeof(s_chat_buf), EDITBOX_FREEFORM, widgetID);
			break;

		case 0:
			break;
	}

	return MENU_REDRAW | MENU_MULTIPLAYER_LOBBY;
}
