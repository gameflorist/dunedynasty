/* menu_lobby.c */

#include <assert.h>
#include <string.h>
#include "enum_string.h"
#include "../os/math.h"

#include "menu.h"

#include "halloffame.h"
#include "scrollbar.h"
#include "../audio/audio.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../input/input.h"
#include "../mods/skirmish.h"
#include "../scenario.h"
#include "../shape.h"
#include "../string.h"
#include "../timer/timer.h"
#include "../video/video.h"
#include "../wsa.h"

static Widget *skirmish_lobby_widgets;
static int64_t skirmish_radar_timer;
bool skirmish_regenerate_map;

/*--------------------------------------------------------------*/

void
Lobby_InitWidgets(void)
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

void
Lobby_FreeWidgets(void)
{
	Menu_FreeWidgets(skirmish_lobby_widgets);
}

/*--------------------------------------------------------------*/

static bool
SkirmishLobby_IsPlayable(void)
{
	bool is_playable = Skirmish_IsPlayable();

	Widget *w = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 2);
	if (is_playable) {
		GUI_Widget_MakeVisible(w);
	}
	else {
		GUI_Widget_MakeInvisible(w);
	}

	return is_playable;
}

static void
SkirmishLobby_RequestRegeneration(bool force)
{
	/* If radar animation is still going, do not restart it. */
	if (Timer_GetTicks() - skirmish_radar_timer < RADAR_ANIMATION_FRAME_COUNT * RADAR_ANIMATION_DELAY) {
		if (force)
			skirmish_regenerate_map = true;
	}
	else {
		skirmish_regenerate_map = true;
		skirmish_radar_timer = Timer_GetTicks();
		Audio_PlaySound(SOUND_RADAR_STATIC);
	}
}

void
SkirmishLobby_Initialise(void)
{
	g_campaign_selected = CAMPAIGNID_SKIRMISH;
	Campaign_Load();

	Skirmish_GenerateMap(false);
	skirmish_regenerate_map = false;
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

void
SkirmishLobby_Draw(void)
{
	const int x1 = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 9)->offsetX - 1;
	const int y1 = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 9)->offsetY - 1;
	const int x2 = x1 + 63;
	const int y2 = y1 + 63;

	HallOfFame_DrawBackground(HOUSE_INVALID, HALLOFFAMESTYLE_CLEAR_BACKGROUND);
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x122);
	GUI_DrawText_Wrapper("Skirmish", SCREEN_WIDTH / 2, 15, 15, 0, 0x122);
	Prim_FillRect_i(x1, y1, x2, y2, 12);

	if (Timer_GetTicks() - skirmish_radar_timer < RADAR_ANIMATION_FRAME_COUNT * RADAR_ANIMATION_DELAY) {
		const int frame = max(0, (Timer_GetTicks() - skirmish_radar_timer) / RADAR_ANIMATION_DELAY);

		GUI_DrawText_Wrapper("Generating", x1 + 32, y1 - 8, 31, 0, 0x111);
		VideoA5_DrawWSAStatic(RADAR_ANIMATION_FRAME_COUNT - frame - 1, x1, y1);
	}
	else if (skirmish_regenerate_map) {
		GUI_DrawText_Wrapper("Generating", x1 + 32, y1 - 8, 31, 0, 0x111);
		VideoA5_DrawWSAStatic(0, x1, y1);
	}
	else {
		GUI_DrawText_Wrapper("Map %u", x1 + 32, y1 - 8, 31, 0, 0x111, g_skirmish.seed);
		Video_DrawMinimap(x1 + 1, y1 + 1, 0, 1);
	}

	GUI_HallOfFame_SetColourScheme(true);

	GUI_DrawText_Wrapper(NULL, 0, 0, 15, 0, 0x11);
	GUI_Widget_DrawAll(skirmish_lobby_widgets);

	GUI_HallOfFame_SetColourScheme(false);

	/* XXX -- GUI_Widget_TextButton2_Draw sets the shortcuts in case
	 * strings change: e.g. a (attack), then c (cancel).
	 * Find other use cases.
	 */
	Widget *w = GUI_Widget_Get_ByIndex(skirmish_lobby_widgets, 1);
	w->shortcut = SCANCODE_ESCAPE;
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
			return MENU_MAIN_MENU;

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
						SkirmishLobby_RequestRegeneration(true);
				}
			}
			break;

		case 0x8000 | 9:
			SkirmishLobby_RequestRegeneration(false);
			break;
	}

	if (skirmish_regenerate_map) {
		conditions_changed = true;

		if (Skirmish_GenerateMap(true))
			skirmish_regenerate_map = false;
	}

	if (conditions_changed) {
		SkirmishLobby_IsPlayable();
	}

	return MENU_REDRAW | MENU_SKIRMISH_LOBBY;
}
