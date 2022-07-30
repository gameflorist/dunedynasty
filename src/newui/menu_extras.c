/* menu_extras.c */

#include <assert.h>
#include <string.h>
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

enum ExtrasMenu {
	EXTRASMENU_CUTSCENE,
	EXTRASMENU_GALLERY,
	EXTRASMENU_JUKEBOX,
	EXTRASMENU_CAMPAIGN,
	EXTRASMENU_OPTIONS,

	EXTRASMENU_MAX
};

static Widget *extras_widgets;
static enum ExtrasMenu extras_page;
static int extras_credits;

static void Extras_DrawRadioButton(Widget *w);
static bool Extras_ClickRadioButton(Widget *w);

/*--------------------------------------------------------------*/

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

void
Extras_InitWidgets(void)
{
	Widget *w;

	w = GUI_Widget_Allocate(1, SCANCODE_ESCAPE, 160, 168 + 8, SHAPE_RESUME_GAME, STR_NULL);
	w->shortcut = SCANCODE_P;
	extras_widgets = GUI_Widget_Link(extras_widgets, w);

	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 20, SCANCODE_F1, 72, 24);
	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 21, SCANCODE_F2, 72, 56);
	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 22, SCANCODE_F3, 72, 88);
	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 23, SCANCODE_F4, 72, 120);
	extras_widgets = Extras_AllocateAndLinkRadioButton(extras_widgets, 24, SCANCODE_F5, 72, 152);

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
Extras_DrawRadioButton(Widget *w)
{
	const enum ExtrasMenu page = w->index - 20;
	const enum ShapeID shapeID[EXTRASMENU_MAX] = {
		SHAPE_TROOPERS, SHAPE_ORNITHOPTER, SHAPE_SONIC_TANK, SHAPE_PALACE, SHAPE_MCV
	};
	assert(page <= EXTRASMENU_MAX);

	if (page == EXTRASMENU_JUKEBOX && !g_enable_audio) {
		Shape_DrawGrey(shapeID[page], w->offsetX, w->offsetY, 0, 0);
	} else {
		Shape_Draw(shapeID[page], w->offsetX, w->offsetY, 0, 0);
	}

	if (page == extras_page)
		ActionPanel_HighlightIcon(HOUSE_HARKONNEN, w->offsetX, w->offsetY, false);
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
		case EXTRASMENU_CUTSCENE:
			headline = "Select Cutscene:";
			break;

		case EXTRASMENU_GALLERY:
			if (mentat->wsa == NULL) {
				headline = String_Get_ByIndex(STR_SELECT_SUBJECT);
			} else {
				PickGallery_Draw(mentat);
			}
			break;

		case EXTRASMENU_JUKEBOX:
			if (g_enable_audio) {
				headline = "Select a Song:";
			} else {
				GUI_DrawText_Wrapper("MUSIC IS OFF", 220, 99, 6, 0, 0x132);
			}
			break;

		case EXTRASMENU_CAMPAIGN:
			headline = String_Get_ByIndex(STR_HALL_OF_FAME);
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

	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 4));
	GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(extras_widgets, 9));

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

		case EXTRASMENU_CAMPAIGN:
			PickCampaign_Initialise();
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
		case EXTRASMENU_CUTSCENE:
			res = PickCutscene_Loop(widgetID);
			break;

		case EXTRASMENU_GALLERY:
			res = PickGallery_Loop(mentat, widgetID);
			break;

		case EXTRASMENU_JUKEBOX:
			res = PickMusic_Loop(mentat, widgetID);
			break;

		case EXTRASMENU_CAMPAIGN:
			res = PickCampaign_Loop(widgetID);
			break;

		case EXTRASMENU_OPTIONS:
			res = Options_Loop(widgetID);
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
