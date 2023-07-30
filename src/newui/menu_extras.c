/* menu_extras.c */

#include <assert.h>
#include <string.h>
#include <allegro5/allegro.h>
#include "enum_string.h"
#include "../os/common.h"
#include "../os/math.h"
#include "../os/strings.h"

#include "menu.h"

#include "actionpanel.h"
#include "mentat.h"
#include "scrollbar.h"
#include "../enhancement.h"
#include "../audio/audio.h"
#include "../config.h"
#include "../cutscene.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/mentat.h"
#include "../gui/widget.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../mods/skirmish.h"
#include "../opendune.h"
#include "../scenario.h"
#include "../shape.h"
#include "../string.h"
#include "../timer/timer.h"
#include "../video/video.h"
#include "../wsa.h"

enum ExtrasMenuCategory {
	EXTRASMENU_CATEGORY_OPTIONS,
	EXTRASMENU_CATEGORY_EXTRAS,

	EXTRASMENU_CATEGORY_MAX
};

enum ExtrasMenu {
	// Options
	EXTRASMENU_VIDEO_OPTIONS,
	EXTRASMENU_MUSIC_OPTIONS,
	EXTRASMENU_GAMEPLAY_OPTIONS,

	// Extras
	EXTRASMENU_CUTSCENES,
	EXTRASMENU_GALLERY,
	EXTRASMENU_JUKEBOX,
	EXTRASMENU_HISCORES,

	EXTRASMENU_MAX
};

static Widget *extras_widgets;
static enum ExtrasMenu extras_page;
static int extras_credits;
static int currentDisplayMode;

static void Extras_DrawMenuHeader(Widget *w);
static void Extras_DrawMenuItem(Widget *w);
static bool Extras_ClickMenuItem(Widget *w);

/*--------------------------------------------------------------*/

static Widget *
Extras_AllocateAndLinkMenuItem(Widget *list, int index, uint16 shortcut, int x, int y)
{
	Widget *w;

	w = GUI_Widget_Allocate(index, shortcut, x, y, SHAPE_INVALID, STR_NULL);
	w->width = 32;
	w->height = 8;
	w->flags.buttonFilterLeft = 0x4;
	w->clickProc = Extras_ClickMenuItem;

	w->drawModeNormal = DRAW_MODE_CUSTOM_PROC;
	w->drawModeSelected = DRAW_MODE_CUSTOM_PROC;
	w->drawModeDown = DRAW_MODE_CUSTOM_PROC;
	w->drawParameterNormal.proc = Extras_DrawMenuItem;
	w->drawParameterSelected.proc = Extras_DrawMenuItem;
	w->drawParameterDown.proc = Extras_DrawMenuItem;

	return GUI_Widget_Link(list, w);
}


static Widget *
Extras_AllocateMenuHeader(Widget *list, int index, int x, int y)
{
	Widget *w;

	w = GUI_Widget_Allocate(index, 0, x, y, SHAPE_INVALID, STR_NULL);
	w->width = 32;
	w->height = 34;
	w->flags.buttonFilterLeft = 0x4;

	w->drawModeNormal = DRAW_MODE_CUSTOM_PROC;
	w->drawModeSelected = DRAW_MODE_CUSTOM_PROC;
	w->drawModeDown = DRAW_MODE_CUSTOM_PROC;
	w->drawParameterNormal.proc = Extras_DrawMenuHeader;
	w->drawParameterSelected.proc = Extras_DrawMenuHeader;
	w->drawParameterDown.proc = Extras_DrawMenuHeader;

	return GUI_Widget_Link(list, w);
}

void
Extras_InitWidgets(void)
{
	Widget *w;

	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, 160, 168 + 8, SHAPE_RESUME_GAME, STR_NULL);
	w->shortcut = SCANCODE_P;
	extras_widgets = GUI_Widget_Link(extras_widgets, w);

	// Options
	extras_widgets = Extras_AllocateMenuHeader(extras_widgets, 10, 72, 20); // Header
	extras_widgets = Extras_AllocateAndLinkMenuItem(extras_widgets, 20, SCANCODE_F1, 72, 60); // Video
	extras_widgets = Extras_AllocateAndLinkMenuItem(extras_widgets, 21, SCANCODE_F2, 72, 70); // Music
	extras_widgets = Extras_AllocateAndLinkMenuItem(extras_widgets, 22, SCANCODE_F3, 72, 80); // Enhancements

	// Extras
	extras_widgets = Extras_AllocateMenuHeader(extras_widgets, 11, 72, 100); // Header
	extras_widgets = Extras_AllocateAndLinkMenuItem(extras_widgets, 23, SCANCODE_F4, 72, 140); // Cutscene
	extras_widgets = Extras_AllocateAndLinkMenuItem(extras_widgets, 24, SCANCODE_F5, 72, 150); // Gallery
	extras_widgets = Extras_AllocateAndLinkMenuItem(extras_widgets, 25, SCANCODE_F6, 72, 160); // Jukebox
	extras_widgets = Extras_AllocateAndLinkMenuItem(extras_widgets, 26, SCANCODE_F7, 72, 170); // Hiscores

	extras_widgets = Scrollbar_Allocate(extras_widgets, WINDOWID_STARPORT_INVOICE, -8, 4, 3, false);
}

void
Extras_FreeWidgets(void)
{
	Menu_FreeWidgets(extras_widgets);
}

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
	} else {
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

enum MenuAction
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
	} else {
		const int64_t curr_ticks = Timer_GetTicks();

		if ((WSA_GetFrameCount(mentat->wsa) > 1) && (curr_ticks - mentat->wsa_timer >= 7)) {
			const int64_t dt = curr_ticks - mentat->wsa_timer;
			mentat->wsa_timer = curr_ticks + dt % 7;
			mentat->wsa_frame += dt / 7;
			redraw = true;
		}

		if (widgetID == SCANCODE_MOUSE_LMB) {
			/* WSA is positioned at 128, 48, size 184 x 112. */
			if (Mouse_InRegion(128, 48, 128 + 48, 160)) {
				widgetID = SCANCODE_KEYPAD_4;
			} else if (Mouse_InRegion(128 + 64, 48, 128 + 184, 160)) {
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
				} else {
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
			} else {
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
		{ MUSIC_ATTACK1, MUSIC_ATTACK_OTHER, "Attack" },
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
					snprintf(si->text, sizeof(si->text), "%s (%s)", m->songname, g_table_music_set[m->music_set].name);
				} else if (lump_together) {
					const char *sub = strchr(l->songname, ':');
					const char *str = (sub == NULL) ? l->songname : (sub + 2);

					snprintf(si->text, sizeof(si->text), "%s", str);
				} else {
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
	} else {
		int dx = -(Timer_GetTicks() - mentat->desc_timer - 60 * 2) / 12;

		/* 2 second delay before scrolling. */
		if (dx > 0) {
			dx = 0;
		} else if (dx + width + 32 <= 0) {
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
PickCampaign_Initialise(void)
{
	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	w->offsetY = 19;
	ws->itemHeight = 8;
	ws->scrollMax = 0;

	for (int i = 0; i < g_campaign_total; i++) {
		if (i == CAMPAIGNID_SKIRMISH || i == CAMPAIGNID_MULTIPLAYER)
			continue;

		si = Scrollbar_AllocItem(w, SCROLLBAR_ITEM);
		si->d.offset = i;

		const char *name
			= (i == CAMPAIGNID_DUNE_II) ? "Dune II"
			: g_campaign_list[i].name;

		snprintf(si->text, sizeof(si->text), "%s", name);
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 11, 0);
}

static enum MenuAction
PickCampaign_Loop(int widgetID)
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
			MainMenu_SelectCampaign(si->d.offset, 0);
			return MENU_HALL_OF_FAME;
	}

	return MENU_EXTRAS;
}

/*--------------------------------------------------------------*/

static void
VideoOptions_Initialize(void)
{
	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;	

	currentDisplayMode = VideoA5_GetCurrentDisplayMode();

	w->offsetY = 22;
	ws->itemHeight = 14;
	ws->scrollMax = 0;

	si = Scrollbar_AllocItem(w, SCROLLBAR_INFO);
	snprintf(si->text, sizeof(si->text), "%s", "Restart game to apply changes.");

	si = Scrollbar_AllocItem(w, SCROLLBAR_CATEGORY);
	snprintf(si->text, sizeof(si->text), "%s", "Window Mode");

	const char *windowModeTexts[3] = {
		"Windowed", "Fullscreen", "Fullscreen Window"
	};

	for(enum WindowMode windowMode = WM_WINDOWED; windowMode <= WM_FULLSCREEN_WINDOW; windowMode++) {
		si = Scrollbar_AllocItem(w, SCROLLBAR_RADIO);
		si->d.radio.group = "windowMode";
		si->d.radio.value = (int)windowMode;
		si->d.radio.currentValue = &g_gameConfig.windowMode;

		snprintf(si->text, sizeof(si->text), "%s", windowModeTexts[windowMode]);
	}

	si = Scrollbar_AllocItem(w, SCROLLBAR_CATEGORY);
	snprintf(si->text, sizeof(si->text), "%s", "Resolution");

	struct DisplayMode* resolutions = VideoA5_GetDisplayModes();
	int numDisplayModes = VideoA5_GetNumDisplayModes();
	for (int i=0; i<numDisplayModes; ++i) {
		if (resolutions[i].width != 0) {
			si = Scrollbar_AllocItem(w, SCROLLBAR_RADIO);
			si->d.radio.group = "resolution";
			si->d.radio.value = i;
			si->d.radio.currentValue = &currentDisplayMode;
			snprintf(si->text, sizeof(si->text), "%dx%d", resolutions[i].width, resolutions[i].height);
		}
	}

	si = Scrollbar_AllocItem(w, SCROLLBAR_CATEGORY);
	snprintf(si->text, sizeof(si->text), "%s", "Other Options");

	si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
	si->d.checkbox = &g_gameConfig.hardwareCursor;
	snprintf(si->text, sizeof(si->text), "Hardware mouse cursor");

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);
}

static enum MenuAction
VideoOptions_Loop(int widgetID)
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
			if (si != NULL) {
				if (si->type == SCROLLBAR_CHECKBOX) {
					*(si->d.checkbox) = !(*(si->d.checkbox));
				}
				else if (si->type == SCROLLBAR_RADIO) {
					if (si->d.radio.group == "windowMode") {
						g_gameConfig.windowMode = si->d.radio.value;
					}
					else if (si->d.radio.group == "resolution") {
						struct DisplayMode* resolutions = VideoA5_GetDisplayModes();
						g_gameConfig.displayMode.width = resolutions[si->d.radio.value].width;
						g_gameConfig.displayMode.height = resolutions[si->d.radio.value].height;
						currentDisplayMode = si->d.radio.value;
					}
				}
			}

			break;
	}

	return MENU_EXTRAS;
}

/*--------------------------------------------------------------*/

static void
MusicOptions_Initialize(void)
{
	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	w->offsetY = 22;
	ws->itemHeight = 14;
	ws->scrollMax = 0;

	si = Scrollbar_AllocItem(w, SCROLLBAR_INFO);
	snprintf(si->text, sizeof(si->text), "%s", "Restart game to apply changes.");

	// Get lists of available and unavailable music sets (midi and adlib are always available).
	enum MusicSet availableSets[NUM_MUSIC_SETS] = { MUSICSET_DUNE2_ADLIB, MUSICSET_DUNE2_MIDI, MUSICSET_FLUIDSYNTH, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID};
	enum MusicSet unavailableSets[NUM_MUSIC_SETS] = {MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID, MUSICSET_INVALID};
	int unavailableSetsCount = 0;
	// Use main menu music to determine existance of music sets, since this song is present in all sets.
	const MusicList *l = &g_table_music[MUSIC_MAIN_MENU];
	for (int i = 0; i < l->length-1; i++) {
		const MusicInfo *m = &l->song[i];
		if (m->music_set == MUSICSET_DUNE2_ADLIB || m->music_set == MUSICSET_DUNE2_MIDI || m->music_set == MUSICSET_FLUIDSYNTH)
			continue;
		const bool musicSetFound = m->enable & MUSIC_FOUND;
		if (musicSetFound) {
			availableSets[i] = m->music_set;
		}
		else {
			unavailableSets[i] = m->music_set;
			unavailableSetsCount++;
		}
	}

	// List available music sets with checkboxes to enable them for random play.	
	si = Scrollbar_AllocItem(w, SCROLLBAR_CATEGORY);
	snprintf(si->text, sizeof(si->text), "%s", "Availabe Music Sets");
	si = Scrollbar_AllocItem(w, SCROLLBAR_INDENTED_INFO);
	snprintf(si->text, sizeof(si->text), "%s", "Enable for random play:");
	for (int i = 0; i < NUM_MUSIC_SETS; i++) {
		if (availableSets[i] != MUSICSET_INVALID) {
			si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
			si->d.checkbox = &g_table_music_set[availableSets[i]].enable;
			snprintf(si->text, sizeof(si->text), g_table_music_set[availableSets[i]].name);
		}
	}

	// List unavailable music sets.
	if(unavailableSetsCount > 0) {
		si = Scrollbar_AllocItem(w, SCROLLBAR_CATEGORY);
		snprintf(si->text, sizeof(si->text), "%s", "Unavailable Music Sets");
		si = Scrollbar_AllocItem(w, SCROLLBAR_INDENTED_INFO);
		snprintf(si->text, sizeof(si->text), "%s", "(See README for installation infos)");
		for (int i = 0; i < NUM_MUSIC_SETS; i++) {
			if (unavailableSets[i] != MUSICSET_INVALID) {
				si = Scrollbar_AllocItem(w, SCROLLBAR_INDENTED_INFO);
				snprintf(si->text, sizeof(si->text), g_table_music_set[unavailableSets[i]].name);
			}
		}
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);
}

static enum MenuAction
MusicOptions_Loop(int widgetID)
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
			}

			break;
	}

	return MENU_EXTRAS;
}

/*--------------------------------------------------------------*/

static void
GameplayOptions_Initialize(void)
{
	Widget *w = GUI_Widget_Get_ByIndex(extras_widgets, 3);
	WidgetScrollbar *ws = w->data;
	ScrollbarItem *si;

	w->offsetY = 22;
	ws->itemHeight = 14;
	ws->scrollMax = 0;	

	si = Scrollbar_AllocItem(w, SCROLLBAR_INFO);
	snprintf(si->text, sizeof(si->text), "%s", "Enhancements over original Dune II:");

	si = Scrollbar_AllocItem(w, SCROLLBAR_CHECKBOX);
	si->d.checkbox = &enhancement_skip_introduction;
	snprintf(si->text, sizeof(si->text), "Skip introduction video");

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
	si->d.checkbox = &enhancement_attack_dir_consistency;
	snprintf(si->text, sizeof(si->text), "Consistent directional damage");

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 6, 0);
}

static enum MenuAction
GameplayOptions_Loop(int widgetID)
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

void
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
Extras_DrawMenuHeader(Widget *w)
{
	const enum ExtrasMenuCategory categoryIndex = w->index - 10;
	const char *text[EXTRASMENU_CATEGORY_MAX] = {
		"Options", "Extras"
	};
	const enum ShapeID shapeID[EXTRASMENU_CATEGORY_MAX] = {
		SHAPE_MCV, SHAPE_PALACE
	};
	assert(categoryIndex <= EXTRASMENU_CATEGORY_MAX);

	Shape_Draw(shapeID[categoryIndex], w->offsetX, w->offsetY, 0, 0);
	GUI_DrawText_Wrapper(text[categoryIndex], w->offsetX + 15, w->offsetY + 26, 15, 0, 0x122);
}

static void
Extras_DrawMenuItem(Widget *w)
{
	const enum ExtrasMenu page = w->index - 20;
	const char *text[EXTRASMENU_MAX] = {
		"Video", "Music", "Gameplay", "Cutscenes", "Gallery", "Jukebox", "Hiscores"
	};
	assert(page <= EXTRASMENU_MAX);

	const int8_t textColor = (page == extras_page) ? 6 : 15;
	GUI_DrawText_Wrapper(text[page], w->offsetX + 15, w->offsetY, textColor, 0, 0x111);
}

void
Extras_Draw(MentatState *mentat)
{
	Video_DrawCPS(SEARCHDIR_GLOBAL_DATA_DIR, "CHOAM.CPS");
	Video_DrawCPSRegion(SEARCHDIR_GLOBAL_DATA_DIR, "CHOAM.CPS", 56, 104, 56, 136, 64, 64);
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

		case EXTRASMENU_VIDEO_OPTIONS:
			headline = "Video Options";
			break;

		case EXTRASMENU_MUSIC_OPTIONS:
			headline = "Music Options";
			break;

		case EXTRASMENU_GAMEPLAY_OPTIONS:
			headline = "Gameplay Options";
			break;
			
		case EXTRASMENU_CUTSCENES:
			headline = "Select Cutscene";
			break;

		case EXTRASMENU_GALLERY:
			if (mentat->wsa == NULL) {
				headline = "Select Subject";
			} else {
				PickGallery_Draw(mentat);
			}
			break;

		case EXTRASMENU_JUKEBOX:
			if (g_enable_audio) {
				headline = "Select a Song";
			} else {
				GUI_DrawText_Wrapper("MUSIC IS OFF", 220, 99, 6, 0, 0x132);
			}
			break;

		case EXTRASMENU_HISCORES:
			headline = String_Get_ByIndex(STR_HALL_OF_FAME);
			break;

		default:
			break;
	}

	if (headline != NULL) {
		const WidgetProperties *wi = &g_widgetProperties[WINDOWID_STARPORT_INVOICE];
		GUI_DrawText_Wrapper(headline, wi->xBase + 8, wi->yBase + 5, 15, 0, 0x22);
	}

	if ((extras_page != EXTRASMENU_GALLERY) || (mentat->wsa == NULL))
		PickMusic_Draw(mentat);

	GUI_DrawText_Wrapper(NULL, 0, 0, 15, 0, 0x11);
	GUI_Widget_DrawAll(extras_widgets);
}

static bool
Extras_ClickMenuItem(Widget *w)
{
	const enum ExtrasMenu new_extras_page = w->index - 20;

	if ((extras_page == new_extras_page) || (new_extras_page >= EXTRASMENU_MAX))
		return false;

	extras_page = new_extras_page;

	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 4));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 9));

	switch (extras_page) {

		case EXTRASMENU_VIDEO_OPTIONS:
			VideoOptions_Initialize();
			break;

		case EXTRASMENU_MUSIC_OPTIONS:
			MusicOptions_Initialize();
			break;

		case EXTRASMENU_GAMEPLAY_OPTIONS:
			GameplayOptions_Initialize();
			break;

		case EXTRASMENU_CUTSCENES:
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

		case EXTRASMENU_HISCORES:
			PickCampaign_Initialise();
			break;

		default:
			break;
	}

	Extras_ShowScrollbar();

	return true;
}

enum MenuAction
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
	if ((widgetID & 0x8000) == 0 && !w->flags.invisible)
		Scrollbar_HandleEvent(w, widgetID);

	switch (extras_page) {

		case EXTRASMENU_VIDEO_OPTIONS:
			res = VideoOptions_Loop(widgetID);
			break;
		case EXTRASMENU_MUSIC_OPTIONS:
			res = MusicOptions_Loop(widgetID);
			break;
		case EXTRASMENU_GAMEPLAY_OPTIONS:
			res = GameplayOptions_Loop(widgetID);
			break;

		case EXTRASMENU_CUTSCENES:
			res = PickCutscene_Loop(widgetID);
			break;

		case EXTRASMENU_GALLERY:
			res = PickGallery_Loop(mentat, widgetID);
			break;

		case EXTRASMENU_JUKEBOX:
			res = PickMusic_Loop(mentat, widgetID);
			break;

		case EXTRASMENU_HISCORES:
			res = PickCampaign_Loop(widgetID);
			break;

		default:
			break;
	}

	if ((res & 0xFF) != MENU_EXTRAS) {
		Audio_StopMusicUnlessMenu();
	} else if (!Audio_MusicIsPlaying()) {
		if (l_pause < curr_ticks) {
			l_pause = curr_ticks + 60 * 2;
		} else if (l_pause < curr_ticks + 60) {
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
