/* menu.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <ctype.h>
#include "buildcfg.h"
#include "enum_string.h"
#include "types.h"
#include "../os/common.h"
#include "../os/math.h"
#include "../os/sleep.h"
#include "../os/strings.h"

#include "menu.h"

#include "actionpanel.h"
#include "halloffame.h"
#include "mentat.h"
#include "menubar.h"
#include "savemenu.h"
#include "scrollbar.h"
#include "strategicmap.h"
#include "../audio/audio.h"
#include "../common_a5.h"
#include "../config.h"
#include "../cutscene.h"
#include "../enhancement.h"
#include "../file.h"
#include "../gfx.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/mentat.h"
#include "../ini.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../mapgenerator/skirmish.h"
#include "../opendune.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../string.h"
#include "../table/sound.h"
#include "../timer/timer.h"
#include "../video/video.h"
#include "../wsa.h"

enum {
	SUBTITLE_FADE_TICKS = 6,
	MENU_FADE_TICKS = 10
};

/* Menu loops can return a MenuAction to transition between screens,
 * or return itself to stay in the same menu.
 *
 * Return (MENU_REDRAW | curr_menu) to force a redraw.
 * Return next_menu to fade in and out.
 * Return (MENU_NOTRANSITION | next_menu) to skip fading.
 * Return (MENU_BLINK_CONFIRM | next_menu) to blink before fading.
 *
 * MENU_FADE_IN and MENU_FADE_OUT are used internally.
 */
enum MenuAction {
	MENU_MAIN_MENU,
	MENU_PICK_HOUSE,
	MENU_CONFIRM_HOUSE,
	MENU_SECURITY,
	MENU_BRIEFING,
	MENU_BRIEFING_WIN,
	MENU_BRIEFING_LOSE,
	MENU_PLAY_A_GAME,
	MENU_LOAD_GAME,
	MENU_PLAY_SKIRMISH,
	MENU_BATTLE_SUMMARY,
	MENU_SKIRMISH_SUMMARY,
	MENU_HALL_OF_FAME,
	MENU_EXTRAS,
	MENU_PLAY_CUTSCENE,
	MENU_CAMPAIGN_CUTSCENE,
	MENU_STRATEGIC_MAP,
	MENU_EXIT_GAME,

	MENU_NO_TRANSITION = 0x0100,
	MENU_FADE_IN = 0x0200,
	MENU_FADE_OUT = 0x0400,
	MENU_BLINK_CONFIRM = 0x0800,
	MENU_REDRAW = 0x8000
};

enum ExtrasMenu {
	EXTRASMENU_CUTSCENE,
	EXTRASMENU_GALLERY,
	EXTRASMENU_JUKEBOX,
	EXTRASMENU_SKIRMISH,
	EXTRASMENU_OPTIONS,

	EXTRASMENU_MAX
};

static int64_t subtitle_timer;
static Widget *main_menu_widgets;
static Widget *pick_house_widgets;
static Widget *extras_widgets;
static Widget *briefing_yes_no_widgets;
static Widget *briefing_proceed_repeat_widgets;

static enum ExtrasMenu extras_page;
static int extras_credits;
static int main_menu_campaign_selected;
static bool skirmish_regenerate_map;
static int64_t skirmish_radar_timer;

static void Extras_ShowScrollbar(void);
static void Extras_HideScrollbar(void);
static void Extras_DrawRadioButton(Widget *w);
static bool Extras_ClickRadioButton(Widget *w);

/*--------------------------------------------------------------*/

static Widget *
MainMenu_InitMenuItem(int nth, uint16 index, const char *string)
{
	const WidgetProperties *prop21 = &g_widgetProperties[WINDOWID_MAINMENU_ITEM];
	Widget *w;

	w = GUI_Widget_Allocate(index, 0, 0, 0, SHAPE_INVALID, STR_NULL);

	w->parentID = WINDOWID_MAINMENU_FRAME;
	w->offsetX = prop21->xBase;
	w->offsetY = prop21->yBase + (g_fontCurrent->height * nth);
	w->height = g_fontCurrent->height - 1;

	w->drawModeNormal = DRAW_MODE_TEXT;
	w->drawModeSelected = DRAW_MODE_TEXT;
	w->drawModeDown = DRAW_MODE_TEXT;
	w->drawParameterNormal.text = string;
	w->drawParameterSelected.text = w->drawParameterNormal.text;
	w->drawParameterDown.text = w->drawParameterNormal.text;
	w->fgColourSelected = prop21->fgColourSelected;
	w->fgColourDown = prop21->fgColourSelected;
	w->flags.clickAsHover = false;

	return w;
}

static void
MainMenu_InitWidgets(void)
{
	struct {
		enum MenuAction index;
		const char *string;
		uint16 stringID;
		int shortcut2;
	} menuitem[] = {
		{ MENU_PLAY_A_GAME,  NULL, STR_PLAY_A_GAME, -1 },
		{ MENU_LOAD_GAME,    NULL, STR_LOAD_GAME, -1, },
		{ MENU_EXTRAS,       "Options and Extras", STR_NULL, SCANCODE_O },
		{ MENU_HALL_OF_FAME, NULL, STR_HALL_OF_FAME, -1 },
		{ MENU_EXIT_GAME,    NULL, STR_EXIT_GAME, -1 },
	};

	int maxWidth = 0;

	/* Select g_fontNew8p with shadow. */
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

	main_menu_widgets = NULL;
	for (unsigned int i = 0; i < lengthof(menuitem); i++) {
		const char *str = (menuitem[i].string != NULL) ? menuitem[i].string : String_Get_ByIndex(menuitem[i].stringID);
		Widget *w;

		w = MainMenu_InitMenuItem(i, menuitem[i].index, str);

		if (menuitem[i].stringID != STR_NULL) {
			w->stringID = menuitem[i].stringID;
			GUI_Widget_SetShortcuts(w);
		}

		if (menuitem[i].shortcut2 > 0) {
			w->shortcut2 = menuitem[i].shortcut2;
		}

		main_menu_widgets = GUI_Widget_Link(main_menu_widgets, w);
		maxWidth = max(maxWidth, Font_GetStringWidth(w->drawParameterNormal.text) + 7);
	}

	for (Widget *w = main_menu_widgets; w != NULL; w = GUI_Widget_GetNext(w)) {
		w->width = maxWidth;
	}

	/* Add widget to change campaign. */
	Widget *w = GUI_Widget_Allocate(100, 0, 0, 104, SHAPE_INVALID, STR_NULL);
	w->width = SCREEN_WIDTH;
	w->height = g_fontIntro->height;
	w->drawModeNormal = DRAW_MODE_NONE;
	w->drawModeSelected = DRAW_MODE_NONE;
	w->drawModeDown = DRAW_MODE_NONE;
	w->drawParameterNormal.text = g_campaign_list[g_campaign_selected].name;
	w->drawParameterSelected.text = w->drawParameterNormal.text;
	w->drawParameterDown.text = w->drawParameterNormal.text;
	w->flags.clickAsHover = false;
	main_menu_widgets = GUI_Widget_Link(main_menu_widgets, w);

	subtitle_timer = -2 * SUBTITLE_FADE_TICKS;
}

static void
PickHouse_InitWidgets(void)
{
	struct {
		uint16 index;
		int x, y;
		int w, h;
		enum Scancode shortcut;
	} menuitem[] = {
		{ HOUSE_HARKONNEN, 208, 56, 96, 104, SCANCODE_H },
		{ HOUSE_ATREIDES,   16, 56, 96, 104, SCANCODE_A },
		{ HOUSE_ORDOS,     112, 56, 96, 104, SCANCODE_O },
		{ HOUSE_FREMEN,     16, 56, 96, 104, SCANCODE_F },
		{ HOUSE_SARDAUKAR, 208, 56, 96, 104, SCANCODE_S },
		{ HOUSE_MERCENARY, 112, 56, 96, 104, SCANCODE_M },
		{ 14, 118, 180, 5, 10, SCANCODE_KEYPAD_4 },
		{ 16, 196, 180, 5, 10, SCANCODE_KEYPAD_6 },
		{ 10, 118, 180, 196 + 5 - 118, 10, 0 },
	};

	const int width = Font_GetStringWidth("Start level: 9");

	menuitem[6].x = (SCREEN_WIDTH - width) / 2 - 6;
	menuitem[7].x = (SCREEN_WIDTH + width) / 2;
	menuitem[8].x = menuitem[6].x;
	menuitem[8].w = menuitem[7].x + 5 - menuitem[6].x;

	for (unsigned int i = 0; i < lengthof(menuitem); i++) {
		Widget *w;

		w = GUI_Widget_Allocate(menuitem[i].index, menuitem[i].shortcut, menuitem[i].x, menuitem[i].y, SHAPE_INVALID, STR_NULL);
		w->width = menuitem[i].w;
		w->height = menuitem[i].h;
		memset(&w->flags, 0, sizeof(w->flags));
		w->flags.loseSelect = true;
		w->flags.buttonFilterLeft = 1;
		w->flags.buttonFilterRight = 1;

		pick_house_widgets = GUI_Widget_Link(pick_house_widgets, w);
	}
}

static void
Briefing_InitWidgets(void)
{
	Widget *w;

	w = GUI_Widget_Allocate(1, SCANCODE_Y, 168, 168, SHAPE_YES, STR_YES);
	briefing_yes_no_widgets = GUI_Widget_Link(briefing_yes_no_widgets, w);

	w = GUI_Widget_Allocate(2, SCANCODE_N, 240, 168, SHAPE_NO, STR_NO);
	briefing_yes_no_widgets = GUI_Widget_Link(briefing_yes_no_widgets, w);

	w = GUI_Widget_Allocate(3, SCANCODE_P, 168, 168, SHAPE_PROCEED, STR_PROCEED);
	briefing_proceed_repeat_widgets = GUI_Widget_Link(briefing_proceed_repeat_widgets, w);

	w = GUI_Widget_Allocate(4, SCANCODE_R, 240, 168, SHAPE_REPEAT, STR_REPEAT);
	briefing_proceed_repeat_widgets = GUI_Widget_Link(briefing_proceed_repeat_widgets, w);
}

static Widget *
Extras_AllocateAndLinkRadioButton(Widget *list, int index, uint16 shortcut, int x, int y)
{
	Widget *w;

	w = GUI_Widget_Allocate(index, shortcut, x, y, SHAPE_INVALID, STR_NULL);
	w->width = 32;
	w->height = 24;
	w->flags.buttonFilterLeft = 0x4;
	w->clickProc = Extras_ClickRadioButton;

	w->drawModeNormal = DRAW_MODE_CUSTOM_PROC;
	w->drawModeSelected = DRAW_MODE_CUSTOM_PROC;
	w->drawModeDown = DRAW_MODE_CUSTOM_PROC;
	w->drawParameterNormal.proc = Extras_DrawRadioButton;
	w->drawParameterSelected.proc = Extras_DrawRadioButton;
	w->drawParameterDown.proc = Extras_DrawRadioButton;

	return GUI_Widget_Link(list, w);
}

static void
Extras_InitWidgets(void)
{
	Widget *w;

	w = GUI_Widget_Allocate(2, SCANCODE_S, 160, 168, SHAPE_SEND_ORDER, STR_NULL); /* Start game. */
	extras_widgets = GUI_Widget_Link(extras_widgets, w);

	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, 160, 168 + 8, SHAPE_RESUME_GAME, STR_NULL);
	w->shortcut = SCANCODE_P;
	extras_widgets = GUI_Widget_Link(extras_widgets, w);

	w = GUI_Widget_Allocate(9, 0, 241, 81, SHAPE_INVALID, STR_NULL); /* Regenerate map. */
	w->width = 62;
	w->height = 62;
	extras_widgets = GUI_Widget_Link(extras_widgets, w);

	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 20, SCANCODE_F1, 72, 24);
	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 21, SCANCODE_F2, 72, 56);
	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 22, SCANCODE_F3, 72, 88);
	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 23, SCANCODE_F4, 72, 120);
	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 24, SCANCODE_F5, 72, 152);

	extras_widgets = Scrollbar_Allocate(extras_widgets, WINDOWID_STARPORT_INVOICE, -8, 4, 3, false);
}

static void
Menu_AddCampaign(ALLEGRO_PATH *path)
{
	if (strcasecmp("skirmish", al_get_path_tail(path)) == 0)
		return;

	al_set_path_filename(path, "META.INI");
	const char *meta = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

	if (!File_Exists_Ex(SEARCHDIR_ABSOLUTE, meta))
		return;

	char *source = File_ReadWholeFile_Ex(SEARCHDIR_ABSOLUTE, meta);
	char buffer[120];
	int house[3];

	Ini_GetString("CAMPAIGN", "House", NULL, buffer, sizeof(buffer), source);
	String_Trim(buffer);

	if (sscanf(buffer, "%d,%d,%d", &house[0], &house[1], &house[2]) < 3) {
		free(source);
		return;
	}

	if (!(HOUSE_HARKONNEN <= house[0] && house[0] < HOUSE_MAX)) house[0] = HOUSE_INVALID;
	if (!(HOUSE_HARKONNEN <= house[1] && house[1] < HOUSE_MAX)) house[1] = HOUSE_INVALID;
	if (!(HOUSE_HARKONNEN <= house[2] && house[2] < HOUSE_MAX)) house[2] = HOUSE_INVALID;

	if ((house[0] == HOUSE_INVALID) && (house[1] == HOUSE_INVALID) && (house[2] == HOUSE_INVALID)) {
		free(source);
		return;
	}

	bool campaign_added = false;
	char value[1024];
	int i;

	al_set_path_filename(path, NULL);
	i = snprintf(value, sizeof(value), "%s/", al_get_path_tail(path));

	Ini_GetString("PAK", "Scenarios", NULL, value + i, sizeof(value) - i, source);

	while ((value[i] != '\0') && (isspace(value[i]))) {
		i++;
	}

	/* Check PAK file listed. */
	if (value[i] != '\0') {
		al_set_path_filename(path, value + i);
		const char *fullpath = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

		if (File_Exists_Ex(SEARCHDIR_ABSOLUTE, fullpath))
			campaign_added = true;
	}

	/* Check loose region files. */
	if (!campaign_added) {
		for (int h = 0; h < 3; h++) {
			if (house[h] == HOUSE_INVALID)
				continue;

			snprintf(value, sizeof(value), "REGION%c.INI", g_table_houseInfo[house[h]].name[0]);

			al_set_path_filename(path, value);
			const char *fullpath = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

			if (!File_Exists_Ex(SEARCHDIR_ABSOLUTE, fullpath))
				house[h] = HOUSE_INVALID;
		}
	}

	/* Add campaign to list. */
	if ((house[0] != HOUSE_INVALID) || (house[1] != HOUSE_INVALID) || (house[2] != HOUSE_INVALID)) {
		al_set_path_filename(path, NULL);
		const char *module = al_get_path_tail(path);

		Campaign *camp = Campaign_Alloc(module);

		for (int h = 0; h < 3; h++) {
			camp->house[h] = house[h];
		}

		Ini_GetString("CAMPAIGN", "Name", module, camp->name, sizeof(camp->name), source);
	}

	free(source);
}

static void
Menu_ScanCampaigns(void)
{
	char dirname[1024];
	ALLEGRO_FS_ENTRY *e, *f;

	snprintf(dirname, sizeof(dirname), "%s/campaign/", g_dune_data_dir);

	e = al_create_fs_entry(dirname);
	if (!e)
		return;

	if (!al_open_directory(e)) {
		al_destroy_fs_entry(e);
		return;
	}

	f = al_read_directory(e);
	while (f != NULL) {
		const bool is_directory = (al_get_fs_entry_mode(f) & ALLEGRO_FILEMODE_ISDIR);

		if (is_directory) {
			ALLEGRO_PATH *path = al_create_path_for_directory(al_get_fs_entry_name(f));
			Menu_AddCampaign(path);
			al_destroy_path(path);
		}

		al_destroy_fs_entry(f);
		f = al_read_directory(e);
	}

	al_close_directory(e);
	al_destroy_fs_entry(e);
}

static void
Menu_Init(void)
{
	Menu_ScanCampaigns();

	MainMenu_InitWidgets();
	PickHouse_InitWidgets();
	Briefing_InitWidgets();
	Extras_InitWidgets();
	StrategicMap_Init();

	Widget *w = GUI_Widget_Get_ByIndex(main_menu_widgets, 100);

	if (g_campaign_total <= 2) { /* Dune II and skirmish campaigns only. */
		GUI_Widget_MakeInvisible(w);
	}
	else {
		Config_GetCampaign();
		w->drawParameterNormal.text = g_campaign_list[g_campaign_selected].name;
		w->drawParameterDown.text = w->drawParameterNormal.text;
	}

	g_skirmish.seed = rand() & 0x7FFF;

	A5_UseTransform(SCREENDIV_MENU);
}

static void
Menu_FreeWidgets(Widget *w)
{
	while (w != NULL) {
		Widget *next = w->next;

		free(w);

		w = next;
	}
}

static void
Menu_Uninit(void)
{
	Menu_FreeWidgets(main_menu_widgets);
	Menu_FreeWidgets(pick_house_widgets);
	Menu_FreeWidgets(extras_widgets);
	Menu_FreeWidgets(briefing_yes_no_widgets);
	Menu_FreeWidgets(briefing_proceed_repeat_widgets);
}

static void
Menu_LoadPalette(void)
{
	File_ReadBlockFile("IBM.PAL", g_palette1, 3 * 256);

	/* Fix the windtrap colour which you normally wouldn't see as the
	 * palette cycling runs in the menu (but you can if you're really
	 * quick about it).
	 */
	g_palette1[3*WINDTRAP_COLOUR + 0] = 0x00;
	g_palette1[3*WINDTRAP_COLOUR + 1] = 0x00;
	g_palette1[3*WINDTRAP_COLOUR + 2] = 0x00;

	Video_SetPalette(g_palette1, 0, 256);
}

/*--------------------------------------------------------------*/

static void
MainMenu_Initialise(Widget *w)
{
	Menu_LoadPalette();
	Widget_SetCurrentWidget(0);

	WidgetProperties *prop13 = &g_widgetProperties[WINDOWID_MAINMENU_FRAME];

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

	prop13->height = 11 + 5 * g_fontCurrent->height;
	prop13->width = w->width + 2*8;
	prop13->xBase = (SCREEN_WIDTH - prop13->width)/2;
	prop13->yBase = 160 - prop13->height/2;

	while (w != NULL) {
		GUI_Widget_MakeNormal(w, false);
		w = GUI_Widget_GetNext(w);
	}
}

static void
MainMenu_Draw(Widget *widget)
{
	Video_DrawCPS(SEARCHDIR_GLOBAL_DATA_DIR, String_GenerateFilename("TITLE"));

	const int64_t curr_ticks = Timer_GetTicks();
	const Widget *w = GUI_Widget_Get_ByIndex(widget, 100);
	const char *subtitle;
	int alpha = 0;
	int colour = (w->state.hover1) ? 192 : 144;

	if (curr_ticks - subtitle_timer < SUBTITLE_FADE_TICKS) {
		/* Interesting colours: 192, 216, 231, 240. */
		subtitle = w->drawParameterDown.text;
		alpha = 0xFF * (curr_ticks - subtitle_timer) / SUBTITLE_FADE_TICKS;
		colour = 192;
	}
	else {
		subtitle = w->drawParameterNormal.text;

		if (curr_ticks - subtitle_timer - SUBTITLE_FADE_TICKS < SUBTITLE_FADE_TICKS) {
			alpha = 0xFF - 0xFF * (curr_ticks - subtitle_timer - SUBTITLE_FADE_TICKS) / SUBTITLE_FADE_TICKS;
			colour = 144;
		}
	}

	Font_Select(g_fontIntro);
	g_fontCharOffset = 0;

	unsigned char colours[16];
	const int x = (SCREEN_WIDTH - Font_GetStringWidth(subtitle)) / 2;
	const int y = 104;

	memset(colours, 0, sizeof(colours));

	for (int i = 0; i < 6; i++)
		colours[i + 1] = 215 + i;

	GUI_InitColors(colours, 0, 15);
	Prim_FillRect_i(0, y, SCREEN_WIDTH, y + g_fontIntro->height, 0);
	GUI_DrawText(subtitle, x, y, colour, 0);

	if (alpha > 0)
		Prim_FillRect_RGBA(0, y, SCREEN_WIDTH, y + g_fontIntro->height, 0, 0, 0, alpha);

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);
	GUI_Widget_DrawBorder(WINDOWID_MAINMENU_FRAME, 2, 1);
	GUI_Widget_DrawAll(widget);

	GUI_DrawText_Wrapper(DUNE_DYNASTY_VERSION, SCREEN_WIDTH, SCREEN_HEIGHT - 9, 133, 0, 0x221);
}

static void
MainMenu_SetupBlink(Widget *w, int widgetID)
{
	widgetID &= ~0x8000;

	while (w != NULL) {
		if (w->index == widgetID) {
			GUI_Widget_MakeSelected(w, false);
		}
		else {
			GUI_Widget_MakeNormal(w, false);
		}

		w = GUI_Widget_GetNext(w);
	}
}

static bool
MainMenu_IsDirty(Widget *w)
{
	while (w != NULL) {
		if (w->state.hover1 != w->state.hover1Last)
			return true;

		w = GUI_Widget_GetNext(w);
	}

	return false;
}

static enum MenuAction
MainMenu_Loop(void)
{
	const int64_t curr_ticks = Timer_GetTicks();
	const int widgetID = GUI_Widget_HandleEvents(main_menu_widgets);
	bool redraw = false;

	Audio_PlayMusicIfSilent(MUSIC_MAIN_MENU);

	switch (widgetID) {
		case 0x8000 | MENU_PLAY_A_GAME:
			g_campaignID = 0;
			Campaign_Load();
			MainMenu_SetupBlink(main_menu_widgets, widgetID);
			return MENU_BLINK_CONFIRM | MENU_PICK_HOUSE;

		case 0x8000 | MENU_EXTRAS:
			main_menu_campaign_selected = g_campaign_selected;
			MainMenu_SetupBlink(main_menu_widgets, widgetID);
			return MENU_BLINK_CONFIRM | MENU_EXTRAS;

		case 0x8000 | MENU_LOAD_GAME:
			SaveMenu_InitSaveLoad(false);
			Campaign_Load();
			MainMenu_SetupBlink(main_menu_widgets, widgetID);
			return MENU_BLINK_CONFIRM | MENU_LOAD_GAME;

		case 0x8000 | MENU_HALL_OF_FAME:
			Campaign_Load();
			MainMenu_SetupBlink(main_menu_widgets, widgetID);
			return MENU_BLINK_CONFIRM | MENU_HALL_OF_FAME;

		case 0x8000 | MENU_EXIT_GAME:
			MainMenu_SetupBlink(main_menu_widgets, widgetID);
			return MENU_BLINK_CONFIRM | MENU_EXIT_GAME;

		case 0x8000 | 100: /* Switch campaign. */
			if (curr_ticks - subtitle_timer < 2 * SUBTITLE_FADE_TICKS)
				break;

			Widget *subtitle = GUI_Widget_Get_ByIndex(main_menu_widgets, 100);
			subtitle->drawParameterDown.text = subtitle->drawParameterNormal.text;

			if (subtitle->state.buttonState & 0x04) {
				g_campaign_selected++;

				if (g_campaign_selected == CAMPAIGNID_SKIRMISH)
					g_campaign_selected++;

				if (g_campaign_selected >= g_campaign_total)
					g_campaign_selected = CAMPAIGNID_DUNE_II;
			}
			else {
				g_campaign_selected--;

				if (g_campaign_selected == CAMPAIGNID_SKIRMISH)
					g_campaign_selected--;

				if (g_campaign_selected < 0)
					g_campaign_selected = g_campaign_total - 1;
			}

			subtitle->drawParameterNormal.text = g_campaign_list[g_campaign_selected].name;

			subtitle_timer = Timer_GetTicks();
			break;

		default:
			break;
	}

	if (curr_ticks - subtitle_timer <= 2 * SUBTITLE_FADE_TICKS)
		redraw = true;

	if (MainMenu_IsDirty(main_menu_widgets))
		redraw = true;

	return (redraw ? MENU_REDRAW : 0) | MENU_MAIN_MENU;
}

static bool
MainMenu_BlinkLoop(int64_t blink_start)
{
	const unsigned char blink_col[2][3] = {
		{ 0xFF, 0x00, 0x00 },
		{ 0xFF, 0xFF, 0xFF }
	};

	const int64_t ticks = Timer_GetTicks() - blink_start;

	if (ticks >= 20) {
		Video_SetPalette(blink_col[1], 6, 1);
		return true;
	}
	else {
		Video_SetPalette(blink_col[(ticks / 3) & 0x1], 6, 1);
		return false;
	}
}

/*--------------------------------------------------------------*/

static void
PickHouse_Initialise(void)
{
	const int offsetX[3] = { 16, 112, 208 };

	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		Widget *w = GUI_Widget_Get_ByIndex(pick_house_widgets, houseID);
		GUI_Widget_MakeInvisible(w);
	}

	for (int i = 0; i < 3; i++) {
		const enum HouseType houseID = g_campaign_list[g_campaign_selected].house[i];

		if (houseID != HOUSE_INVALID) {
			Widget *w = GUI_Widget_Get_ByIndex(pick_house_widgets, houseID);
			w->offsetX = offsetX[i];
			GUI_Widget_MakeVisible(w);
		}
	}

	g_strategicRegionBits = 0;
}

static void
PickHouse_Draw(void)
{
	Video_DrawCPS(SEARCHDIR_CAMPAIGN_DIR, String_GenerateFilename("HERALD"));

	if (g_campaign_list[g_campaign_selected].house[0] == HOUSE_INVALID)
		Prim_FillRect_RGBA( 16.0f, 50.0f,  16.0f + 96.0f, 150.0f, 0x00, 0x00, 0x00, 0xC0);

	if (g_campaign_list[g_campaign_selected].house[1] == HOUSE_INVALID)
		Prim_FillRect_RGBA(112.0f, 50.0f, 112.0f + 96.0f, 150.0f, 0x00, 0x00, 0x00, 0xC0);

	if (g_campaign_list[g_campaign_selected].house[2] == HOUSE_INVALID)
		Prim_FillRect_RGBA(208.0f, 50.0f, 208.0f + 96.0f, 150.0f, 0x00, 0x00, 0x00, 0xC0);

	const Widget *w = GUI_Widget_Get_ByIndex(pick_house_widgets, 10);
	uint8 fg = (w->state.hover1) ? 130 : 133;

	/* Width is 70/72 with the EU font, 73-75 with the US font. */
	const ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];
	const int width = Font_GetStringWidth("Start level: 9");
	GUI_DrawText_Wrapper("Start level: %d", (SCREEN_WIDTH - width) / 2, SCREEN_HEIGHT - 20, fg, 0, 0x22, g_campaignID + 1);

	const float xl = (SCREEN_WIDTH - width) / 2 - 7.0f;
	Video_SetClippingArea(div->scalex * (xl + 1) + div->x, div->scaley * (SCREEN_HEIGHT - 18) + div->y, div->scalex * 3, div->scaley * 5);
	Shape_DrawTint(SHAPE_CURSOR_LEFT, xl, SCREEN_HEIGHT - 21, fg, 0, 0);

	const float xr = (SCREEN_WIDTH + width) / 2 - 2.0f;
	Video_SetClippingArea(div->scalex * (xr + 5) + div->x, div->scaley * (SCREEN_HEIGHT - 18) + div->y, div->scalex * 3, div->scaley * 5);
	Shape_DrawTint(SHAPE_CURSOR_RIGHT, xr, SCREEN_HEIGHT - 21, fg, 0, 0);

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}

static int
PickHouse_Loop(void)
{
	const int widgetID = GUI_Widget_HandleEvents(pick_house_widgets);
	bool redraw = false;

	Audio_PlayMusicIfSilent(MUSIC_MAIN_MENU);

	switch (widgetID) {
		case SCANCODE_ESCAPE:
			return MENU_MAIN_MENU;

		case 0x8000 | HOUSE_HARKONNEN:
		case 0x8000 | HOUSE_ATREIDES:
		case 0x8000 | HOUSE_ORDOS:
		case 0x8000 | HOUSE_FREMEN:
		case 0x8000 | HOUSE_SARDAUKAR:
		case 0x8000 | HOUSE_MERCENARY:
			g_playerHouseID = widgetID & (~0x8000);
			g_scenarioID = 1;

			/* Fremen, Sardaukar, and Mercenaries don't have special house samples. */
			if (g_playerHouseID <= HOUSE_ORDOS) {
				Audio_LoadSampleSet(SAMPLESET_BENE_GESSERIT);
			}
			else {
				Audio_LoadSampleSet(g_table_houseInfo[g_playerHouseID].sampleSet);
			}

			Audio_PlayVoice(VOICE_HARKONNEN + g_playerHouseID);

			while (Audio_Poll())
				Timer_Sleep(1);

			Audio_LoadSampleSet(g_table_houseInfo[g_playerHouseID].sampleSet);

			return MENU_CONFIRM_HOUSE;

		case 0x8000 | 14:
			if (g_campaignID >= 1) {
				g_campaignID--;
				redraw = true;
			}
			break;

		case 0x8000 | 16:
			if (g_campaignID < 8) {
				g_campaignID++;
				redraw = true;
			}
			break;

		default:
			break;
	}

	Widget *w = GUI_Widget_Get_ByIndex(pick_house_widgets, 10);
	if (w->state.hover1 != w->state.hover1Last)
		redraw = true;

	return (redraw ? MENU_REDRAW : 0) | MENU_PICK_HOUSE;
}

/*--------------------------------------------------------------*/

static enum MenuAction
Security_InputLoop(enum HouseType houseID, MentatState *mentat)
{
	bool redraw = Input_IsInputAvailable();

	const int res = GUI_EditBox(mentat->security_prompt, sizeof(mentat->security_prompt) - 1, 9, NULL, NULL, 0);

	if (res == SCANCODE_ENTER) {
		const char *answer = String_Get_ByIndex(mentat->security_question + 2);

		houseID = g_table_houseRemap6to3[houseID];

		if ((enhancement_security_question == SECURITY_QUESTION_ACCEPT_ALL) ||
			(strcasecmp(mentat->security_prompt, answer) == 0)) {
			mentat->state = MENTAT_SHOW_TEXT;
			strncpy(mentat->buf, String_Get_ByIndex(STR_SECURITY_CORRECT_HARKONNEN + houseID * 3), sizeof(mentat->buf));
			mentat->text = mentat->buf;
			MentatBriefing_SplitText(mentat);

			return MENU_BLINK_CONFIRM | MENU_BRIEFING;
		}
		else {
			mentat->state = MENTAT_SECURITY_INCORRECT;
			strncpy(mentat->buf, String_Get_ByIndex(STR_SECURITY_WRONG_HARKONNEN + houseID * 3), sizeof(mentat->buf));
			mentat->text = mentat->buf;
			MentatBriefing_SplitText(mentat);
			mentat->security_lives--;

			if (mentat->security_lives <= 0) {
				return MENU_EXIT_GAME;
			}
		}
	}

	return (redraw ? MENU_REDRAW : 0) | MENU_SECURITY;
}

static void
Briefing_Initialise(enum MenuAction menu, MentatState *mentat)
{
	const enum MentatID mentatID = (menu == MENU_CONFIRM_HOUSE) ? MENTAT_BENE_GESSERIT : g_table_houseInfo[g_playerHouseID].mentat;

	Sprites_InitMentat(mentatID);

	if (menu == MENU_CONFIRM_HOUSE) {
		MentatBriefing_InitText(g_playerHouseID, -1, MENTAT_BRIEFING_ORDERS, mentat);
		MentatBriefing_InitWSA(g_playerHouseID, -1, MENTAT_BRIEFING_ORDERS, mentat);
	}
	else if (menu == MENU_SECURITY) {
		MentatSecurity_Initialise(g_playerHouseID, mentat);
	}
	else {
		const enum BriefingEntry entry =
			(menu == MENU_BRIEFING_WIN) ? MENTAT_BRIEFING_WIN :
			(menu == MENU_BRIEFING_LOSE) ? MENTAT_BRIEFING_LOSE :
			MENTAT_BRIEFING_ORDERS;

		MentatBriefing_InitText(g_playerHouseID, g_campaignID, entry, mentat);
		MentatBriefing_InitWSA(g_playerHouseID, g_scenarioID, entry, mentat);

		Audio_PlayMusic(MUSIC_STOP);
	}

	mentat->state = MENTAT_SHOW_TEXT;
}

static void
Briefing_Draw(enum MenuAction curr_menu, MentatState *mentat)
{
	const enum MentatID mentatID = (curr_menu == MENU_CONFIRM_HOUSE) ? MENTAT_BENE_GESSERIT : g_table_houseInfo[g_playerHouseID].mentat;

	Mentat_DrawBackground(mentatID);
	MentatBriefing_DrawWSA(mentat);
	Mentat_Draw(mentatID);

	if (mentat->state == MENTAT_SHOW_TEXT || mentat->state == MENTAT_SECURITY_INCORRECT) {
		MentatBriefing_DrawText(mentat);
	}
	else if (mentat->state == MENTAT_IDLE) {
		if (curr_menu == MENU_CONFIRM_HOUSE) {
			const char *misc = String_GenerateFilename("MISC");
			const int offset = g_campaign_list[g_campaign_selected].misc_cps[g_playerHouseID];

			Video_DrawCPSRegion(SEARCHDIR_CAMPAIGN_DIR, misc, 0, 0, 0, 0, 26*8, 24);
			Video_DrawCPSRegion(SEARCHDIR_CAMPAIGN_DIR, misc, 0, 24 * (offset + 1), 26*8, 0, 13*8, 24);

			GUI_Widget_DrawAll(briefing_yes_no_widgets);
		}
		else if (curr_menu == MENU_SECURITY) {
			MentatSecurity_Draw(mentat);
		}
		else {
			GUI_Widget_DrawAll(briefing_proceed_repeat_widgets);
		}
	}
}

static enum MenuAction
Briefing_Loop(enum MenuAction curr_menu, enum HouseType houseID, MentatState *mentat)
{
	const int64_t curr_ticks = Timer_GetTicks();
	const enum MentatID mentatID = (curr_menu == MENU_CONFIRM_HOUSE) ? MENTAT_BENE_GESSERIT : g_table_houseInfo[g_playerHouseID].mentat;

	bool redraw = false;
	int widgetID = 0;

	if (curr_ticks - mentat->wsa_timer >= 7) {
		const int64_t dt = curr_ticks - mentat->wsa_timer;
		mentat->wsa_timer = curr_ticks + dt % 7;
		mentat->wsa_frame += dt / 7;
		redraw = true;
	}

	if (curr_ticks >= mentat->speaking_timer) {
		mentat->speaking_mode = 0;
	}

	GUI_Mentat_Animation(mentatID, mentat->speaking_mode);

	if (!Audio_MusicIsPlaying()) {
		switch (curr_menu) {
			case MENU_BRIEFING_WIN:
				Audio_PlayMusic(g_table_houseInfo[houseID].musicWin);
				break;

			case MENU_BRIEFING_LOSE:
				Audio_PlayMusic(g_table_houseInfo[houseID].musicLose);
				break;

			case MENU_CONFIRM_HOUSE:
				/* Don't play music, but leave main menu music alone. */
				break;

			default:
				Audio_PlayMusic(g_table_houseInfo[houseID].musicBriefing);
				break;
		}
	}

	if (mentat->state == MENTAT_IDLE) {
		if (curr_menu == MENU_CONFIRM_HOUSE) {
			widgetID = GUI_Widget_HandleEvents(briefing_yes_no_widgets);
		}
		else if (curr_menu == MENU_SECURITY) {
			enum MenuAction ret = Security_InputLoop(g_playerHouseID, mentat);

			return (redraw ? MENU_REDRAW : 0) | ret;
		}
		else {
			widgetID = GUI_Widget_HandleEvents(briefing_proceed_repeat_widgets);
		}
	}
	else if (Input_IsInputAvailable()) {
		widgetID = Input_GetNextKey();
	}

	if ((!(widgetID & 0x8000)) && (widgetID & SCANCODE_RELEASE))
		widgetID = 0;

	switch (widgetID) {
		case 0:
			break;

		case 0x8001: /* yes */
			g_scenarioID = 1;
			return (g_campaignID == 0) ? MENU_BRIEFING : MENU_STRATEGIC_MAP;

		case 0x8002: /* no */
			return MENU_PICK_HOUSE;

		case 0x8003: /* proceed */
			if (curr_menu == MENU_BRIEFING_WIN) {
				return MENU_NO_TRANSITION | MENU_BATTLE_SUMMARY;
			}
			else if (curr_menu == MENU_BRIEFING_LOSE) {
				if (g_campaignID == 0) {
					return MENU_NO_TRANSITION | MENU_BRIEFING;
				}
				else {
					return MENU_STRATEGIC_MAP;
				}
			}

			return MENU_NO_TRANSITION | MENU_PLAY_A_GAME;

		case 0x8004: /* repeat */
			mentat->state = MENTAT_SHOW_TEXT;
			mentat->text = mentat->buf;
			mentat->lines = mentat->lines0;
			redraw = true;
			break;

		default:
			if (mentat->state == MENTAT_SHOW_TEXT) {
				MentatBriefing_AdvanceText(mentat);

				if (curr_menu == MENU_SECURITY && mentat->state == MENTAT_IDLE)
					MentatSecurity_PrepareQuestion(false, mentat);

				redraw = true;
			}
			else if (mentat->state == MENTAT_SECURITY_INCORRECT) {
				MentatSecurity_PrepareQuestion(true, mentat);
				mentat->state = MENTAT_IDLE;
				redraw = true;
			}
			break;
	}

	return (redraw ? MENU_REDRAW : 0) | curr_menu;
}

/*--------------------------------------------------------------*/

static void
PlayAGame_StartGame(bool new_game)
{
	A5_UseTransform(SCREENDIV_MAIN);
	GameLoop_Main(new_game);
	Audio_PlayMusic(MUSIC_STOP);
	A5_UseTransform(SCREENDIV_MENU);
}

static enum MenuAction
PlayAGame_Loop(bool new_game)
{
	PlayAGame_StartGame(new_game);

	switch (g_gameMode) {
		case GM_MENU:
		case GM_NORMAL:
			break;

		case GM_RESTART:
			return MENU_FADE_IN | MENU_BRIEFING;

		case GM_PICKHOUSE:
			return MENU_FADE_IN | MENU_PICK_HOUSE;

		case GM_WIN:
			g_strategicRegionBits = 0;

			/* Mark completion. */
			Campaign *camp = &g_campaign_list[g_campaign_selected];
			for (unsigned int h = 0; h < 3; h++) {
				if (camp->house[h] == g_playerHouseID) {
					camp->completion[h] |= (1 << (g_scenarioID - 1));
					Config_SaveCampaignCompletion();
					break;
				}
			}

			return MENU_NO_TRANSITION | MENU_BRIEFING_WIN;

		case GM_LOSE:
			return MENU_NO_TRANSITION | MENU_BRIEFING_LOSE;

		case GM_QUITGAME:
			return MENU_FADE_IN | MENU_MAIN_MENU;
	}

	return MENU_FADE_IN | MENU_MAIN_MENU;
}

static void
LoadGame_Draw(void)
{
	GUI_Widget_DrawWindow(&g_saveLoadWindowDesc);
	GUI_Widget_DrawAll(g_widgetLinkedListTail);
}

static enum MenuAction
LoadGame_Loop(void)
{
	const int ret = SaveMenu_SaveLoad_Click(false);
	bool redraw = false;

	if (ret == -1) {
		return MENU_MAIN_MENU;
	}
	else if (ret == -2) {
		return PlayAGame_Loop(false);
	}
	else if (ret == -3) {
		redraw = true;
	}

	Widget *w = g_widgetLinkedListTail;
	while (w != NULL) {
		if ((w->state.selected != w->state.selectedLast) ||
			(w->state.hover1 != w->state.hover1Last)) {
			redraw = true;
			break;
		}

		w = GUI_Widget_GetNext(w);
	}

	return (redraw ? MENU_REDRAW : 0) | MENU_LOAD_GAME;
}

static enum MenuAction
PlaySkirmish_Loop(void)
{
	do {
		if (skirmish_regenerate_map) {
			if (!Skirmish_GenerateMap(false))
				return MENU_EXTRAS;
		}

		PlayAGame_StartGame(false);
		skirmish_regenerate_map = true;
	} while (g_gameMode == GM_RESTART);

	if (g_gameMode == GM_WIN) {
		Audio_PlayMusic(g_table_houseInfo[g_playerHouseID].musicWin);
		return MENU_FADE_IN | MENU_SKIRMISH_SUMMARY;
	}
	else if (g_gameMode == GM_LOSE) {
		Audio_PlayMusic(g_table_houseInfo[g_playerHouseID].musicLose);
		return MENU_FADE_IN | MENU_SKIRMISH_SUMMARY;
	}
	else {
		Audio_PlayMusic(MUSIC_MAIN_MENU);
		return MENU_EXTRAS;
	}
}

/*--------------------------------------------------------------*/

static void
BattleSummary_Initialise(enum HouseType houseID, HallOfFameData *fame)
{
	uint16 harvestedAllied = g_scenario.harvestedAllied;
	uint16 harvestedEnemy = g_scenario.harvestedEnemy;
	uint16 score = Update_Score(g_scenario.score, &harvestedAllied, &harvestedEnemy, houseID);

	fame->state = HALLOFFAME_PAUSE_START;
	fame->pause_timer = Timer_GetTicks() + 45;
	fame->score = score;
	fame->time = ((g_timerGame - g_tickScenarioStart) / 3600) + 1;

	HallOfFame_InitRank(fame->score, fame);

	fame->meter[0].max = harvestedAllied;
	fame->meter[1].max = harvestedEnemy;
	fame->meter[2].max = g_scenario.killedEnemy;
	fame->meter[3].max = g_scenario.killedAllied;
	fame->meter[4].max = g_scenario.destroyedEnemy;
	fame->meter[5].max = g_scenario.destroyedAllied;

	for (int i = 0; i < 6; i += 2) {
		const int ally = fame->meter[i + 0].max;
		const int enemy = fame->meter[i + 1].max;
		const int maxval = max(ally, enemy);
		const int maxwidth = 205;
		const int inc = 1 + ((maxval > maxwidth) ? (maxval / maxwidth) : 0);

		fame->meter[i + 0].inc = inc;
		fame->meter[i + 1].inc = inc;
		fame->meter[i + 0].width = 0;
		fame->meter[i + 1].width = 0;
	}

	fame->curr_meter_idx = 0;
	fame->curr_meter_val = 0;
	fame->meter_colour_dir = true;
	fame->meter_colour_timer = Timer_GetTicks();
}

static void
BattleSummary_Draw(enum HouseType houseID, int scenarioID, HallOfFameData *fame)
{
	HallOfFame_DrawBackground(houseID, false);
	HallOfFame_DrawScoreTime(fame->score, fame->time);

	if (fame->state >= HALLOFFAME_SHOW_RANK)
		HallOfFame_DrawRank(fame);

	HallOfFame_DrawSpiceHarvested(houseID, fame);
	HallOfFame_DrawUnitsDestroyed(houseID, fame);
	HallOfFame_DrawBuildingsDestroyed(houseID, scenarioID, fame);
}

static enum MenuAction
BattleSummary_TimerLoop(int curr_menu, int scenarioID, HallOfFameData *fame)
{
	const int64_t curr_ticks = Timer_GetTicks();

	if (curr_ticks - fame->meter_colour_timer >= (64 - 35) * 3) {
		fame->meter_colour_timer = curr_ticks;
		fame->meter_colour_dir = !fame->meter_colour_dir;
	}

	switch (fame->state) {
		case HALLOFFAME_PAUSE_START:
		case HALLOFFAME_PAUSE_RANK:
			if (curr_ticks >= fame->pause_timer)
				fame->state++;
			break;

		case HALLOFFAME_SHOW_RANK:
			if (Video_TickFadeIn(fame->rank_aux)) {
				fame->state = HALLOFFAME_PAUSE_RANK;
				fame->pause_timer = curr_ticks + 45;
			}
			break;

		case HALLOFFAME_SHOW_METER:
			fame->curr_meter_val += fame->meter[fame->curr_meter_idx].inc;
			if (fame->curr_meter_val >= fame->meter[fame->curr_meter_idx].max) {
				if ((fame->curr_meter_idx & 0x1) == 0) {
					fame->state = HALLOFFAME_PAUSE_METER;
					fame->pause_timer = curr_ticks + 12;
				}
				else {
					fame->state = HALLOFFAME_PAUSE_METER;
					fame->pause_timer = curr_ticks + 60 + 12;
				}
				Audio_PlayEffect(EFFECT_HALL_OF_FAME_END_METER);
			}
			else {
				fame->meter[fame->curr_meter_idx].width++;
				Audio_PlaySound(EFFECT_CREDITS_INCREASE);
			}
			break;

		case HALLOFFAME_PAUSE_METER:
			if (curr_ticks >= fame->pause_timer) {
				fame->curr_meter_idx++;
				fame->curr_meter_val = 0;

				if ((fame->curr_meter_idx >= 6) || (scenarioID == 1 && fame->curr_meter_idx >= 4)) {
					fame->state = HALLOFFAME_WAIT_FOR_INPUT;
					Input_History_Clear();
				}
				else {
					fame->state = HALLOFFAME_SHOW_METER;
				}
			}
			break;

		case HALLOFFAME_WAIT_FOR_INPUT:
			break;
	}

	return MENU_REDRAW | curr_menu;
}

static enum MenuAction
BattleSummary_InputLoop(int curr_menu, HallOfFameData *fame)
{
	switch (fame->state) {
		case HALLOFFAME_WAIT_FOR_INPUT:
			if (Input_IsInputAvailable()) {
				if (curr_menu == MENU_SKIRMISH_SUMMARY) {
					Audio_PlayMusic(MUSIC_MAIN_MENU);
					return MENU_EXTRAS;
				}

				GUI_HallOfFame_Show(g_playerHouseID, fame->score);
				g_campaignID++;
				return MENU_NO_TRANSITION | MENU_CAMPAIGN_CUTSCENE;
			}

			break;

		default:
			break;
	}

	return curr_menu;
}

/*--------------------------------------------------------------*/

static void
PickCutscene_Initialise(void)
{
	const struct {
		const char *text;
		uint32 animation;
	} cutscenes[] = {
		{ "Introduction",               HOUSEANIMATION_INTRO },
		{ "Harkonnen intermission 1",   HOUSEANIMATION_LEVEL4_HARKONNEN },
		{ "Harkonnen intermission 2",   HOUSEANIMATION_LEVEL8_HARKONNEN },
		{ "Harkonnen end game",         HOUSEANIMATION_LEVEL9_HARKONNEN },
		{ "Atreides intermission 1",    HOUSEANIMATION_LEVEL4_ATREIDES },
		{ "Atreides intermission 2",    HOUSEANIMATION_LEVEL8_ATREIDES },
		{ "Atreides end game",          HOUSEANIMATION_LEVEL9_ATREIDES },
		{ "Ordos intermission 1",       HOUSEANIMATION_LEVEL4_ORDOS },
		{ "Ordos intermission 2",       HOUSEANIMATION_LEVEL8_ORDOS },
		{ "Ordos end game",             HOUSEANIMATION_LEVEL9_ORDOS },
		{ "Credits",                    10 },
		{ NULL, 0 }
	};

	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	w->offsetY = 19;
	ws->itemHeight = 8;
	ws->scrollMax = 0;

	for (int i = 0; cutscenes[i].text != NULL; i++) {
		si = Scrollbar_AllocItem(w, SCROLLBAR_ITEM);

		snprintf(si->text, sizeof(si->text), "%s", cutscenes[i].text);
		si->d.offset = cutscenes[i].animation;
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 11, 0);
}

static enum MenuAction
PickCutscene_Loop(int widgetID)
{
	switch (widgetID) {
		case 0x8000 | 1: /* exit. */
			return MENU_MAIN_MENU;

		case 0x8000 | 3: /* list entry. */
		case SCANCODE_ENTER:
		case SCANCODE_KEYPAD_5:
		case SCANCODE_SPACE:
			/* Fade in/out between playing cutscenes. */
			return MENU_PLAY_CUTSCENE;
	}

	return MENU_EXTRAS;
}

static enum MenuAction
PlayCutscene_Loop(void)
{
	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 15);
	ScrollbarItem *si = Scrollbar_GetSelectedItem(w);

	switch (si->d.offset) {
		case HOUSEANIMATION_INTRO:
			GameLoop_GameIntroAnimation();
			break;

		case HOUSEANIMATION_LEVEL4_HARKONNEN:
		case HOUSEANIMATION_LEVEL4_ATREIDES:
		case HOUSEANIMATION_LEVEL4_ORDOS:
		case HOUSEANIMATION_LEVEL8_HARKONNEN:
		case HOUSEANIMATION_LEVEL8_ATREIDES:
		case HOUSEANIMATION_LEVEL8_ORDOS:
		case HOUSEANIMATION_LEVEL9_HARKONNEN:
		case HOUSEANIMATION_LEVEL9_ATREIDES:
		case HOUSEANIMATION_LEVEL9_ORDOS:
			Cutscene_PlayAnimation(si->d.offset);
			break;

		case 10: /* Credits. */
			GameLoop_GameCredits(HOUSE_INVALID);
			break;
	}

	return MENU_EXTRAS;
}

/*--------------------------------------------------------------*/

static void
PickGallery_Initialise(void)
{
	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	WidgetScrollbar *ws = w->data;

	w->offsetY = 19;
	ws->itemHeight = 8;

	/* Note: Use Harkonnen list which contains the Sardaukar and Frigate entries. */
	Mentat_LoadHelpSubjects(w, true, SEARCHDIR_GLOBAL_DATA_DIR, HOUSE_HARKONNEN, 9, true);

	g_mentat_state.wsa = NULL;
}

static void
PickGallery_Draw(MentatState *mentat)
{
	GUI_DrawStatusBarText(128, 21);
	MentatBriefing_DrawWSA(mentat);
}

static const ObjectInfo *
PickGallery_WSAtoObject(const char *str)
{
	if (str == NULL || str[0] == '\0')
		return NULL;

	for (enum StructureType s = STRUCTURE_SLAB_1x1; s < STRUCTURE_MAX; s++) {
		const ObjectInfo *oi = &g_table_structureInfo_original[s].o;

		if ((oi->wsa != NULL) && (strcasecmp(str, oi->wsa) == 0))
			return oi;
	}

	for (enum UnitType u = UNIT_CARRYALL; u < UNIT_MAX; u++) {
		const ObjectInfo *oi = &g_table_unitInfo_original[u].o;

		if ((oi->wsa != NULL) && (strcasecmp(str, oi->wsa) == 0))
			return oi;
	}

	return NULL;
}

static enum MenuAction
PickGallery_Loop(MentatState *mentat, int widgetID)
{
	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 15);
	bool perform_selection = false;
	bool redraw = false;

	if (mentat->wsa == NULL) {
		switch (widgetID) {
			case 0x8000 | 1: /* exit. */
				return MENU_MAIN_MENU;

			case 0x8000 | 3: /* list entry. */
			case SCANCODE_ENTER:
			case SCANCODE_KEYPAD_5:
			case SCANCODE_SPACE:
				GUI_DisplayText(NULL, -1);
				perform_selection = true;
				break;
		}
	}
	else {
		const int64_t curr_ticks = Timer_GetTicks();

		if ((WSA_GetFrameCount(mentat->wsa) > 1) && (curr_ticks - mentat->wsa_timer >= 7)) {
			const int64_t dt = curr_ticks - mentat->wsa_timer;
			mentat->wsa_timer = curr_ticks + dt % 7;
			mentat->wsa_frame += dt / 7;
			redraw = true;
		}

		if (widgetID == MOUSE_LMB) {
			/* WSA is positioned at 128, 48, size 184 x 112. */
			if (Mouse_InRegion(128, 48, 128 + 48, 160)) {
				widgetID = SCANCODE_KEYPAD_4;
			}
			else if (Mouse_InRegion(128 + 64, 48, 128 + 184, 160)) {
				widgetID = SCANCODE_KEYPAD_6;
			}
		}

		switch (widgetID) {
			case 0x8000 | 1:
				mentat->wsa = NULL;
				Extras_ShowScrollbar();
				break;

			case SCANCODE_KEYPAD_4: /* NUMPAD 4 / ARROW LEFT */
			case SCANCODE_KEYPAD_8: /* NUMPAD 8 / ARROW UP */
			case SCANCODE_KEYPAD_9: /* NUMPAD 9 / PAGE UP */
			case SCANCODE_BACKSPACE:
				Scrollbar_CycleUp(w);
				perform_selection = true;
				break;

			case SCANCODE_KEYPAD_2: /* NUMPAD 2 / ARROW DOWN */
			case SCANCODE_KEYPAD_3: /* NUMPAD 3 / PAGE DOWN */
			case SCANCODE_KEYPAD_6: /* NUMPAD 6 / ARROW RIGHT */
			case SCANCODE_SPACE:
				Scrollbar_CycleDown(w);
				perform_selection = true;
				break;
		}
	}

	if (perform_selection) {
		GUI_Mentat_ShowHelp(w, SEARCHDIR_GLOBAL_DATA_DIR, HOUSE_HARKONNEN, 9);

		if (mentat->wsa != NULL) {
			char *c = mentat->desc;
			while ((c != NULL) && (*c != '\0')) {
				if (*c == '\r' || *c == '\n') {
					*c = '\0';
					break;
				}
				else {
					c++;
				}
			}

			GUI_DisplayText(mentat->desc, 5);

			/* Dodgy: determine the object type from the WSA
			 * name since it is not stored in the list.
			 */
			const ObjectInfo *oi = PickGallery_WSAtoObject(g_readBuffer);

			if (oi != NULL) {
				extras_credits = oi->buildCredits;
			}
			else {
				extras_credits = 0;
			}

			Extras_HideScrollbar();
		}
	}

	return (redraw ? MENU_REDRAW : 0) | MENU_EXTRAS;
}

/*--------------------------------------------------------------*/

static void
PickMusic_Initialise(void)
{
	const struct {
		enum MusicID start, end;
		const char *name;
	} category[] = {
		{ MUSIC_IDLE1, MUSIC_BONUS, "Idle" },
		{ MUSIC_ATTACK1, MUSIC_ATTACK6, "Attack" },
		{ MUSIC_BRIEFING_HARKONNEN, MUSIC_END_GAME_ORDOS, "Mentat" },
		{ MUSIC_LOGOS, MUSIC_CREDITS, "Other" },
	};

	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	w->offsetY = 19;
	ws->itemHeight = 8;
	ws->scrollMax = 0;

	for (unsigned int c = 0; c < lengthof(category); c++) {
		bool lump_together = true;

		for (enum MusicID musicID = category[c].start; musicID <= category[c].end; musicID++) {
			if (g_table_music[musicID].count_found > 1) {
				lump_together = false;
				break;
			}
		}

		if (lump_together) {
			si = Scrollbar_AllocItem(w, SCROLLBAR_CATEGORY);
			snprintf(si->text, sizeof(si->text), "%s", category[c].name);
		}

		for (enum MusicID musicID = category[c].start; musicID <= category[c].end; musicID++) {
			const MusicList *l = &g_table_music[musicID];

			if (l->count_found == 0)
				continue;

			if (!lump_together) {
				si = Scrollbar_AllocItem(w, SCROLLBAR_CATEGORY);
				snprintf(si->text, sizeof(si->text), "%s", l->songname);
			}

			for (int s = 0; s < l->length; s++) {
				const MusicInfo *m = &l->song[s];

				if (!(m->enable & MUSIC_FOUND))
					continue;

				si = Scrollbar_AllocItem(w, SCROLLBAR_ITEM);
				si->d.offset = (s << 8) | musicID;

				if (m->songname != NULL) {
					snprintf(si->text, sizeof(si->text), "%s", m->songname);
				}
				else if (lump_together) {
					const char *sub = strchr(l->songname, ':');
					const char *str = (sub == NULL) ? l->songname : (sub + 2);

					snprintf(si->text, sizeof(si->text), "%s", str);
				}
				else {
					snprintf(si->text, sizeof(si->text), "%s", g_table_music_set[m->music_set].name);
				}
			}
		}
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 11, 0);
}

static void
PickMusic_Draw(MentatState *mentat)
{
	const ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x12);
	const int width = Font_GetStringWidth(music_message);

	Video_SetClippingArea(div->scalex * 128 + div->x, div->scaley * 23 + div->y, div->scalex * 184, div->scaley * 10);

	if (width <= 184) {
		GUI_DrawText_Wrapper(music_message, 128, 23, 12, 0, 0x12);
	}
	else {
		int dx = -(Timer_GetTicks() - mentat->desc_timer - 60 * 2) / 12;

		/* 2 second delay before scrolling. */
		if (dx > 0) {
			dx = 0;
		}
		else if (dx + width + 32 <= 0) {
			mentat->desc_timer = Timer_GetTicks() - 60 * 2;
			dx = 0;
		}

		GUI_DrawText_Wrapper(music_message, 128 + dx, 23, 12, 0, 0x12);
		GUI_DrawText_Wrapper(music_message, 128 + dx + width + 32, 23, 12, 0, 0x12);
	}

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}

static enum MenuAction
PickMusic_Loop(MentatState *mentat, int widgetID)
{
	Widget *scrollbar = GUI_Widget_Get_ByIndex(extras_widgets, 15);
	ScrollbarItem *si;

	switch (widgetID) {
		case 0x8000 | 1: /* exit. */
			return MENU_MAIN_MENU;

		case 0x8000 | 3: /* list entry. */
		case SCANCODE_ENTER:
		case SCANCODE_KEYPAD_5:
		case SCANCODE_SPACE:
			si = Scrollbar_GetSelectedItem(scrollbar);
			if (si->type == SCROLLBAR_CATEGORY)
				break;

			const enum MusicID musicID = (si->d.offset & 0xFF);
			const int s = (si->d.offset >> 8);

			mentat->desc_timer = Timer_GetTicks();
			Audio_PlayMusicFile(&g_table_music[musicID], &g_table_music[musicID].song[s]);
			break;
	}

	return MENU_EXTRAS;
}

/*--------------------------------------------------------------*/

static void
Skirmish_RequestRegeneration(bool force)
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

static void
Skirmish_Initialise(void)
{
	Widget *w;

	g_campaign_selected = CAMPAIGNID_SKIRMISH;
	Campaign_Load();

	Skirmish_GenerateMap(false);
	skirmish_regenerate_map = false;

	w = GUI_Widget_Get_ByIndex(extras_widgets, 1);
	w->offsetY = 184;

	w = GUI_Widget_Get_ByIndex(extras_widgets, 2);
	GUI_Widget_MakeVisible(w);

	w = GUI_Widget_Get_ByIndex(extras_widgets, 9);
	GUI_Widget_MakeVisible(w);

	w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	w->width = 92;

	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	w->offsetY = 22;
	ws->itemHeight = 14;
	ws->scrollMax = 0;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		si = Scrollbar_AllocItem(w, SCROLLBAR_BRAIN);
		si->d.brain = &g_skirmish.brain[h];
		snprintf(si->text, sizeof(si->text), "%s", g_table_houseInfo[h].name);
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);
}

static void
Skirmish_Draw(void)
{
	const int x1 = 240;
	const int y1 = 80;
	const int x2 = x1 + 63;
	const int y2 = y1 + 63;

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
}

static enum MenuAction
Skirmish_Loop(int widgetID)
{
	switch (widgetID) {
		case 0x8000 | 1: /* exit. */
			return MENU_MAIN_MENU;

		case 0x8000 | 2: /* start game. */
			if (Skirmish_IsPlayable()) {
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
				Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
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

					if (!Skirmish_GenerateMap(false))
						Skirmish_RequestRegeneration(true);
				}
			}
			break;

		case 0x8000 | 9:
			Skirmish_RequestRegeneration(false);
			break;
	}

	if (skirmish_regenerate_map) {
		if (Skirmish_GenerateMap(true))
			skirmish_regenerate_map = false;
	}

	return MENU_EXTRAS;
}

/*--------------------------------------------------------------*/

static void
Options_Initialise(void)
{
	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	w->offsetY = 22;
	ws->itemHeight = 14;
	ws->scrollMax = 0;

	si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
	si->d.checkbox = &enhancement_brutal_ai;
	snprintf(si->text, sizeof(si->text), "Brutal AI");

	si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
	si->d.checkbox = &enhancement_fog_of_war;
	snprintf(si->text, sizeof(si->text), "Fog of war");

	si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
	si->d.checkbox = &enhancement_insatiable_sandworms;
	snprintf(si->text, sizeof(si->text), "Insatiable sandworms");

	si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
	si->d.checkbox = &enhancement_raise_scenario_unit_cap;
	snprintf(si->text, sizeof(si->text), "Raise scenario unit cap");

	si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
	si->d.checkbox = &enhancement_true_game_speed_adjustment;
	snprintf(si->text, sizeof(si->text), "True game speed adjustment");

	si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
	si->d.checkbox = &g_gameConfig.hardwareCursor;
	snprintf(si->text, sizeof(si->text), "Hardware mouse cursor");

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);
}

static enum MenuAction
Options_Loop(int widgetID)
{
	Widget *scrollbar = GUI_Widget_Get_ByIndex(extras_widgets, 15);
	ScrollbarItem *si;

	switch (widgetID) {
		case 0x8000 | 1: /* exit. */
			return MENU_MAIN_MENU;

		case 0x8000 | 3: /* list entry. */
		case SCANCODE_ENTER:
		case SCANCODE_KEYPAD_5:
		case SCANCODE_SPACE:
			si = Scrollbar_GetSelectedItem(scrollbar);
			if ((si != NULL) && (si->type == SCROLLBAR_CHECKBOX)) {
				*(si->d.checkbox) = !(*(si->d.checkbox));

				if (si->d.checkbox == &g_gameConfig.hardwareCursor)
					Mouse_SwitchHWCursor();
			}

			break;
	}

	return MENU_EXTRAS;
}

/*--------------------------------------------------------------*/

static void
Extras_ShowScrollbar(void)
{
	/* Show scroll list. */
	Widget *scrollbar = GUI_Widget_Get_ByIndex(extras_widgets, 15);
	WidgetScrollbar *ws = scrollbar->data;

	GUI_Widget_MakeVisible(GUI_Widget_Get_ByIndex(extras_widgets, 3));
	GUI_Widget_MakeVisible(scrollbar);

	if (ws->scrollMax > ws->scrollPageSize) {
		GUI_Widget_MakeVisible(GUI_Widget_Get_ByIndex(extras_widgets, 16));
		GUI_Widget_MakeVisible(GUI_Widget_Get_ByIndex(extras_widgets, 17));
	}
	else {
		GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 16));
		GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 17));
	}
}

static void
Extras_HideScrollbar(void)
{
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 3));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 15));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 16));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 17));
}

static void
Extras_Initialise(MentatState *mentat)
{
	extras_credits = 0;
	mentat->desc_timer = Timer_GetTicks();
	g_playerHouseID = HOUSE_HARKONNEN;
	Menu_LoadPalette();

	/* Reset credits. */
	GUI_DrawCredits(extras_credits, 2, SCREEN_WIDTH);

	/* Restore previous selection. */
	const enum ExtrasMenu new_extras_page = extras_page;
	extras_page = EXTRASMENU_MAX;
	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(extras_widgets, 20 + new_extras_page), true);

	/* XXX: dodgy hack to clear the last selected widgets. */
	GUI_Widget_HandleEvents(main_menu_widgets);
}

static void
Extras_DrawRadioButton(Widget *w)
{
	const enum ExtrasMenu page = w->index - 20;
	const enum ShapeID shapeID[EXTRASMENU_MAX] = {
		SHAPE_TROOPERS, SHAPE_ORNITHOPTER, SHAPE_SONIC_TANK, SHAPE_SARDAUKAR, SHAPE_MCV
	};
	assert(page <= EXTRASMENU_MAX);

	if (page == EXTRASMENU_JUKEBOX && !g_enable_audio) {
		Shape_DrawGrey(shapeID[page], w->offsetX, w->offsetY, 0, 0);
	}
	else {
		Shape_Draw(shapeID[page], w->offsetX, w->offsetY, 0, 0);
	}

	if (page == extras_page)
		ActionPanel_HighlightIcon(HOUSE_HARKONNEN, w->offsetX, w->offsetY, false);
}

static void
Extras_Draw(MentatState *mentat)
{
	Video_DrawCPS(SEARCHDIR_GLOBAL_DATA_DIR, "CHOAM.CPS");
	Video_DrawCPSRegion(SEARCHDIR_GLOBAL_DATA_DIR, "CHOAM.CPS", 56, 104, 56, 136, 64, 64);

	if (extras_page != EXTRASMENU_SKIRMISH)
		Video_DrawCPSRegion(SEARCHDIR_GLOBAL_DATA_DIR, "FAME.CPS", 90, 32, 150, 168, 140, 32);

	/* Credits label may need to be replaced for other languages. */
	Shape_Draw(SHAPE_CREDITS_LABEL, SCREEN_WIDTH - 128, 0, 0, 0);

	const ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];
	Video_SetClippingArea(0, div->scaley * 4 + div->y, TRUE_DISPLAY_WIDTH, div->scaley * 9);
	GUI_DrawCredits(extras_credits, 1, SCREEN_WIDTH);
	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);

	const char *headline = NULL;
	Widget_SetAndPaintCurrentWidget(WINDOWID_STARPORT_INVOICE);

	switch (extras_page) {
		case EXTRASMENU_CUTSCENE:
			headline = "Select Cutscene:";
			break;

		case EXTRASMENU_GALLERY:
			if (mentat->wsa == NULL) {
				headline = String_Get_ByIndex(STR_SELECT_SUBJECT);
			}
			else {
				PickGallery_Draw(mentat);
			}
			break;

		case EXTRASMENU_JUKEBOX:
			if (g_enable_audio) {
				headline = "Select a Song:";
			}
			else {
				GUI_DrawText_Wrapper("MUSIC IS OFF", 220, 99, 6, 0, 0x132);
			}
			break;

		case EXTRASMENU_SKIRMISH:
			headline = "Skirmish:";
			Skirmish_Draw();
			break;

		case EXTRASMENU_OPTIONS:
			headline = "Enhancement Options:";
			break;

		default:
			break;
	}

	if (headline != NULL) {
		const WidgetProperties *wi = &g_widgetProperties[WINDOWID_STARPORT_INVOICE];
		GUI_DrawText_Wrapper(headline, wi->xBase + 8, wi->yBase + 5, 12, 0, 0x12);
	}

	if ((extras_page != EXTRASMENU_GALLERY) || (mentat->wsa == NULL))
		PickMusic_Draw(mentat);

	GUI_DrawText_Wrapper(NULL, 0, 0, 15, 0, 0x11);
	GUI_Widget_DrawAll(extras_widgets);
}

static bool
Extras_ClickRadioButton(Widget *w)
{
	const enum ExtrasMenu new_extras_page = w->index - 20;

	if ((extras_page == new_extras_page) || (new_extras_page >= EXTRASMENU_MAX))
		return false;

	extras_page = new_extras_page;

	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 2));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 4));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 9));
	GUI_Widget_Get_ByIndex(extras_widgets, 1)->offsetY = 168 + 8;
	GUI_Widget_Get_ByIndex(extras_widgets, 3)->width = 0x88;

	switch (extras_page) {
		case EXTRASMENU_CUTSCENE:
			PickCutscene_Initialise();
			break;

		case EXTRASMENU_GALLERY:
			PickGallery_Initialise();
			break;

		case EXTRASMENU_JUKEBOX:
			if (!g_enable_audio) {
				Extras_HideScrollbar();
				return true;
			}

			PickMusic_Initialise();
			break;

		case EXTRASMENU_SKIRMISH:
			Skirmish_Initialise();
			break;

		case EXTRASMENU_OPTIONS:
			Options_Initialise();
			break;

		default:
			break;
	}

	Extras_ShowScrollbar();

	return true;
}

static enum MenuAction
Extras_Loop(MentatState *mentat)
{
	static ScrollbarItem *last_si;
	static int64_t l_pause;

	const int64_t curr_ticks = Timer_GetTicks();
	enum MenuAction res = MENU_EXTRAS;
	bool redraw = false;

	int widgetID = GUI_Widget_HandleEvents(extras_widgets);

	/* If we changed the extras menu page. */
	if (0x8000 + 20 <= widgetID && widgetID < 0x8000 + 20 + EXTRASMENU_MAX)
		return MENU_REDRAW | MENU_EXTRAS;

	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 15);
	if ((widgetID & 0x8000) == 0 && !(extras_page == EXTRASMENU_GALLERY && mentat->wsa != NULL))
		Scrollbar_HandleEvent(w, widgetID);

	switch (extras_page) {
		case EXTRASMENU_CUTSCENE:
			res = PickCutscene_Loop(widgetID);
			break;

		case EXTRASMENU_GALLERY:
			res = PickGallery_Loop(mentat, widgetID);
			break;

		case EXTRASMENU_JUKEBOX:
			res = PickMusic_Loop(mentat, widgetID);
			break;

		case EXTRASMENU_SKIRMISH:
			res = Skirmish_Loop(widgetID);
			break;

		case EXTRASMENU_OPTIONS:
			res = Options_Loop(widgetID);
			break;

		default:
			break;
	}

	if ((res & 0xFF) != MENU_EXTRAS) {
		Audio_StopMusicUnlessMenu();

		if ((res & 0xFF) == MENU_MAIN_MENU)
			g_campaign_selected = main_menu_campaign_selected;
	}
	else if (!Audio_MusicIsPlaying()) {
		if (l_pause < curr_ticks) {
			l_pause = curr_ticks + 60 * 2;
		}
		else if (l_pause < curr_ticks + 60) {
			Audio_PlayMusicNextInSequence();
			mentat->desc_timer = curr_ticks;
		}
	}

	ScrollbarItem *si = Scrollbar_GetSelectedItem(w);
	if (last_si != si) {
		last_si = si;
		redraw = true;
	}

	if (curr_ticks - mentat->speaking_timer >= 3) {
		mentat->speaking_timer = curr_ticks;
		redraw = true;
	}

	return (redraw ? MENU_REDRAW : 0) | res;
}

/*--------------------------------------------------------------*/

static enum MenuAction
StrategicMap_InputLoop(int campaignID, StrategicMapData *map)
{
	if (map->state == STRATEGIC_MAP_SELECT_REGION) {
		int scenario = StrategicMap_SelectRegion(map, g_mouseClickX, g_mouseClickY);

		if (scenario >= 0) {
			map->blink_scenario = scenario;
			g_scenarioID = StrategicMap_CampaignChoiceToScenarioID(campaignID, scenario);
			map->state = STRATEGIC_MAP_BLINK_REGION;
			StrategicMap_AdvanceText(map, true);

			if ((enhancement_security_question != SECURITY_QUESTION_SKIP) && (g_campaign_list[g_campaign_selected].intermission) &&
					(g_gameMode == GM_WIN) && (campaignID == 1 || campaignID == 7)) {
				return MENU_BLINK_CONFIRM | MENU_SECURITY;
			}
			else {
				return MENU_BLINK_CONFIRM | MENU_BRIEFING;
			}
		}
	}
	else {
		if (Input_IsInputAvailable()) {
			const int key = Input_GetNextKey();

			if (key == SCANCODE_ESCAPE || key == SCANCODE_SPACE || key == MOUSE_LMB || key == MOUSE_RMB)
				map->fast_forward = true;
		}
	}

	return MENU_STRATEGIC_MAP;
}

/*--------------------------------------------------------------*/

static void
Menu_DrawFadeIn(int64_t fade_start)
{
	int alpha = 0xFF - 0xFF * (Timer_GetTicks() - fade_start) / MENU_FADE_TICKS;

	if (alpha <= 0)
		return;

	Video_ShadeScreen(alpha);
}

static void
Menu_DrawFadeOut(int64_t fade_start)
{
	int alpha = 0xFF * (Timer_GetTicks() - fade_start) / MENU_FADE_TICKS;

	if (alpha >= 0xFF)
		alpha = 0xFF;

	Video_ShadeScreen(alpha);
}

void
Menu_Run(void)
{
	enum MenuAction curr_menu = MENU_FADE_IN | MENU_MAIN_MENU;
	enum MenuAction next_menu = curr_menu;
	int64_t fade_start = Timer_GetTicks();
	int64_t last_redraw_time = 0;
	bool initialise_menu = true;
	bool redraw = true;

	Menu_Init();

	al_flush_event_queue(g_a5_input_queue);

	while (curr_menu != MENU_EXIT_GAME) {
		/* Initialise. */
		if (initialise_menu) {
			initialise_menu = false;

			switch (curr_menu & 0xFF) {
				case MENU_MAIN_MENU:
					MainMenu_Initialise(main_menu_widgets);
					break;

				case MENU_PICK_HOUSE:
					PickHouse_Initialise();
					break;

				case MENU_CONFIRM_HOUSE:
				case MENU_SECURITY:
				case MENU_BRIEFING:
				case MENU_BRIEFING_WIN:
				case MENU_BRIEFING_LOSE:
					Briefing_Initialise(curr_menu & 0xFF, &g_mentat_state);
					break;

				case MENU_BATTLE_SUMMARY:
				case MENU_SKIRMISH_SUMMARY:
					BattleSummary_Initialise(g_playerHouseID, &g_hall_of_fame_state);
					break;

				case MENU_EXTRAS:
					Extras_Initialise(&g_mentat_state);
					break;

				case MENU_STRATEGIC_MAP:
					StrategicMap_Initialise(g_playerHouseID, g_campaignID, &g_strategic_map_state);
					break;

				default:
					break;
			}
		}

		/* Draw. */
		if (redraw && (last_redraw_time < Timer_GetTicks())) {
			redraw = false;
			al_clear_to_color(al_map_rgb(0, 0, 0));

			switch (curr_menu & 0xFF) {
				case MENU_MAIN_MENU:
					MainMenu_Draw(main_menu_widgets);
					break;

				case MENU_PICK_HOUSE:
					PickHouse_Draw();
					break;

				case MENU_CONFIRM_HOUSE:
				case MENU_SECURITY:
				case MENU_BRIEFING:
				case MENU_BRIEFING_WIN:
				case MENU_BRIEFING_LOSE:
					Briefing_Draw(curr_menu & 0xFF, &g_mentat_state);
					break;

				case MENU_LOAD_GAME:
					LoadGame_Draw();
					break;

				case MENU_BATTLE_SUMMARY:
				case MENU_SKIRMISH_SUMMARY:
					BattleSummary_Draw(g_playerHouseID, g_scenarioID, &g_hall_of_fame_state);
					break;

				case MENU_EXTRAS:
					Extras_Draw(&g_mentat_state);
					break;

				case MENU_STRATEGIC_MAP:
					StrategicMap_Draw(g_playerHouseID, &g_strategic_map_state, fade_start);
					break;

				default:
					break;
			}

			if (curr_menu & MENU_FADE_IN) {
				Menu_DrawFadeIn(fade_start);
			}
			else if ((curr_menu & (MENU_BLINK_CONFIRM | MENU_FADE_OUT)) == MENU_FADE_OUT) {
				Menu_DrawFadeOut(fade_start);
			}

			Video_Tick();
			last_redraw_time = Timer_GetTicks();
		}

		/* Menu input. */
		ALLEGRO_EVENT event;

		al_wait_for_event(g_a5_input_queue, &event);
		if (InputA5_ProcessEvent(&event, true))
			redraw = true;

		curr_menu = (curr_menu & ~MENU_REDRAW);

		enum MenuAction res = curr_menu;
		switch ((int)curr_menu) {
			case MENU_MAIN_MENU:
				res = MainMenu_Loop();
				break;

			case MENU_MAIN_MENU | MENU_BLINK_CONFIRM | MENU_FADE_OUT:
				if (MainMenu_BlinkLoop(fade_start))
					res &= ~MENU_BLINK_CONFIRM;
				break;

			case MENU_PICK_HOUSE:
				res = PickHouse_Loop();
				break;

			case MENU_CONFIRM_HOUSE:
				res = Briefing_Loop(curr_menu, HOUSE_INVALID, &g_mentat_state);
				break;

			case MENU_SECURITY:
				res = Briefing_Loop(curr_menu, g_playerHouseID, &g_mentat_state);
				break;

			case MENU_SECURITY | MENU_BLINK_CONFIRM | MENU_FADE_OUT:
				if (MentatSecurity_CorrectLoop(&g_mentat_state, fade_start))
					res = MENU_NO_TRANSITION | MENU_BRIEFING;
				break;

			case MENU_BRIEFING:
			case MENU_BRIEFING_WIN:
			case MENU_BRIEFING_LOSE:
				res = Briefing_Loop(curr_menu, g_playerHouseID, &g_mentat_state);
				break;

			case MENU_PLAY_A_GAME:
				res = PlayAGame_Loop(true);
				break;

			case MENU_LOAD_GAME:
				res = LoadGame_Loop();
				break;

			case MENU_PLAY_SKIRMISH:
				res = PlaySkirmish_Loop();
				break;

			case MENU_BATTLE_SUMMARY:
			case MENU_SKIRMISH_SUMMARY:
				if (event.type == ALLEGRO_EVENT_TIMER) {
					res = BattleSummary_TimerLoop(curr_menu, g_scenarioID, &g_hall_of_fame_state);
				}
				else {
					res = BattleSummary_InputLoop(curr_menu, &g_hall_of_fame_state);
				}
				break;

			case MENU_HALL_OF_FAME:
				GUI_HallOfFame_Show(HOUSE_INVALID, 0xFFFF);
				res = MENU_MAIN_MENU;
				break;

			case MENU_EXTRAS:
				res = Extras_Loop(&g_mentat_state);
				break;

			case MENU_PLAY_CUTSCENE:
				res = PlayCutscene_Loop();
				break;

			case MENU_CAMPAIGN_CUTSCENE:
				if (g_campaignID == 9) {
					/* Only show end game for Dune II campaign. */
					if (g_campaign_list[g_campaign_selected].intermission) {
						GameLoop_GameEndAnimation();
					}
					else {
						GameLoop_GameCredits(g_playerHouseID);
					}

					res = MENU_MAIN_MENU;
				}
				else {
					/* Only show intermission for Dune II campaign. */
					if (g_campaign_list[g_campaign_selected].intermission)
						GameLoop_LevelEndAnimation();

					res = MENU_STRATEGIC_MAP;
				}
				break;

			case MENU_STRATEGIC_MAP:
				if (event.type == ALLEGRO_EVENT_TIMER) {
					if (StrategicMap_TimerLoop(&g_strategic_map_state))
						res |= MENU_REDRAW;
				}
				else {
					res = StrategicMap_InputLoop(g_campaignID, &g_strategic_map_state);
				}
				break;

			case MENU_STRATEGIC_MAP | MENU_BLINK_CONFIRM | MENU_FADE_OUT:
				if (StrategicMap_BlinkLoop(&g_strategic_map_state, fade_start))
					res &=~ MENU_BLINK_CONFIRM;
				break;

			case MENU_EXIT_GAME:
			case MENU_REDRAW:
				break;

			default:
				if (curr_menu & MENU_FADE_OUT) {
					if (Timer_GetTicks() >= fade_start + MENU_FADE_TICKS) {
						curr_menu = MENU_FADE_OUT | next_menu;
						res = next_menu;
						initialise_menu = true;
					}
				}
				else if (curr_menu & MENU_FADE_IN) {
					if (Timer_GetTicks() >= fade_start + MENU_FADE_TICKS) {
						res &= ~MENU_FADE_IN;
						Input_History_Clear();
					}
				}
				break;
		}

		/* Menu transition. */
		if ((curr_menu & 0xFF) != (res & 0xFF)) {
			redraw = true;

			if (res & MENU_NO_TRANSITION) {
				curr_menu = (res & 0xFF);
				initialise_menu = true;
			}
			else if (res & MENU_FADE_IN) {
				curr_menu = (res & (MENU_FADE_IN | 0xFF));
				fade_start = Timer_GetTicks();
				initialise_menu = true;
			}
			else {
				if (res & MENU_BLINK_CONFIRM)
					curr_menu |= MENU_BLINK_CONFIRM;

				curr_menu = MENU_FADE_OUT | curr_menu;
				next_menu = MENU_FADE_IN | (res & 0xFF);
				fade_start = Timer_GetTicks();
			}
		}
		else if ((curr_menu^res) & (MENU_BLINK_CONFIRM | MENU_FADE_OUT | MENU_FADE_IN)) {
			redraw = true;

			curr_menu = (res & ~MENU_REDRAW);
			fade_start = Timer_GetTicks();
		}
		else {
			if (res & (MENU_REDRAW | MENU_BLINK_CONFIRM | MENU_FADE_OUT | MENU_FADE_IN))
				redraw = true;

			curr_menu = (res & ~MENU_REDRAW);
		}
	}

	Menu_Uninit();
}
