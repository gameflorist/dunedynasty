/* menu.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include "types.h"
#include "../os/math.h"
#include "../os/sleep.h"

#include "menu.h"

#include "mentat.h"
#include "../common_a5.h"
#include "../gfx.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../opendune.h"
#include "../string.h"
#include "../table/strings.h"
#include "../timer/timer.h"
#include "../video/video.h"

enum MenuAction {
	MENU_MAIN_MENU,
	MENU_PICK_HOUSE,
	MENU_BRIEFING,
	MENU_PLAY_A_GAME,
	MENU_EXIT_GAME,

	MENU_REDRAW = 0x8000
};

static ALLEGRO_TRANSFORM identity_transform;
static ALLEGRO_TRANSFORM menu_transform;

static Widget *main_menu_widgets;
static Widget *pick_house_widgets;
static Widget *briefing_yes_no_widgets;
static Widget *briefing_proceed_repeat_widgets;

/*--------------------------------------------------------------*/

static void
Menu_InitTransform(void)
{
	const double preferred_aspect = (double)SCREEN_WIDTH / SCREEN_HEIGHT;
	const double w = TRUE_DISPLAY_WIDTH;
	const double h = TRUE_DISPLAY_HEIGHT;
	const double aspect = w/h;

	float scale = 1.0f;
	float offx = 0.0f;
	float offy = 0.0f;

	if (aspect < preferred_aspect - 0.001) {
		const double newh = w / preferred_aspect;
		offy = (h - newh) / 2;
		scale = newh / SCREEN_HEIGHT;
	}
	else if (aspect > preferred_aspect + 0.001) {
		const double neww = h * preferred_aspect;
		offx = (w - neww) / 2;
		scale = neww / SCREEN_WIDTH;
	}

	g_mouse_transform_scale = scale;
	g_mouse_transform_offx = offx;
	g_mouse_transform_offy = offy;

	al_identity_transform(&identity_transform);
	al_build_transform(&menu_transform, offx, offy, scale, scale, 0.0f);
	al_use_transform(&menu_transform);
}

static void
MainMenu_InitWidgets(void)
{
	const int menuitem[] = {
		STR_PLAY_A_GAME,
		STR_REPLAY_INTRODUCTION,
		STR_LOAD_GAME,
		STR_HALL_OF_FAME,
		STR_EXIT_GAME,
		STR_NULL
	};

	WidgetProperties *prop13 = &g_widgetProperties[WINDOWID_MAINMENU_FRAME];
	WidgetProperties *prop21 = &g_widgetProperties[WINDOWID_MAINMENU_ITEM];
	int maxWidth = 0;

	/* Select g_fontNew8p with shadow. */
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

	main_menu_widgets = NULL;
	prop13->height = 11;
	for (int i = 0; menuitem[i] != STR_NULL; i++) {
		Widget *w;

		w = GUI_Widget_Allocate(menuitem[i], 0, 0, 0, SHAPE_INVALID, STR_NULL);

		w->parentID = WINDOWID_MAINMENU_FRAME;
		w->offsetX = prop21->xBase*8;
		w->offsetY = prop21->yBase + (g_fontCurrent->height * i);
		w->height = g_fontCurrent->height - 1;

		w->drawModeNormal = DRAW_MODE_TEXT;
		w->drawModeSelected = DRAW_MODE_TEXT;
		w->drawModeDown = DRAW_MODE_TEXT;
		w->drawParameterNormal.text = String_Get_ByIndex(menuitem[i]);
		w->drawParameterSelected.text = w->drawParameterNormal.text;
		w->drawParameterDown.text = w->drawParameterNormal.text;
		w->fgColourSelected = prop21->fgColourNormal;
		w->fgColourDown = prop21->fgColourSelected;
		w->flags.s.requiresClick = true;

		GUI_Widget_MakeNormal(main_menu_widgets, false);
		main_menu_widgets = GUI_Widget_Link(main_menu_widgets, w);

		prop13->height += g_fontCurrent->height;
		maxWidth = max(maxWidth, Font_GetStringWidth(w->drawParameterNormal.text) + 7);
	}

	prop13->width = maxWidth/8 + 2;
	prop13->xBase = (SCREEN_WIDTH/8 - prop13->width)/2;
	prop13->yBase = 160 - prop13->height/2;

	for (Widget *w = main_menu_widgets; w != NULL; w = GUI_Widget_GetNext(w)) {
		w->width = maxWidth;
	}
}

static void
PickHouse_InitWidgets(void)
{
	const struct {
		int x, y;
		enum Scancode shortcut;
	} menuitem[3] = {
		{ 208, 56, SCANCODE_H },
		{  16, 56, SCANCODE_A },
		{ 112, 56, SCANCODE_O },
	};

	for (enum HouseType house = HOUSE_HARKONNEN; house <= HOUSE_ORDOS; house++) {
		Widget *w;

		w = GUI_Widget_Allocate(house, menuitem[house].shortcut, menuitem[house].x, menuitem[house].y, SHAPE_INVALID, STR_NULL);

		w->width = 96;
		w->height = 104;
		w->flags.all = 0x0;
		w->flags.s.loseSelect = true;
		w->flags.s.buttonFilterLeft = 1;
		w->flags.s.buttonFilterRight = 1;

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

static void
Menu_Init(void)
{
	MainMenu_InitWidgets();
	PickHouse_InitWidgets();
	Briefing_InitWidgets();
	Menu_InitTransform();
}

/*--------------------------------------------------------------*/

static void
MainMenu_Draw(void)
{
	Video_DrawCPS(String_GenerateFilename("TITLE"));

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);
	GUI_Widget_DrawBorder(WINDOWID_MAINMENU_FRAME, 2, 1);
	GUI_Widget_DrawAll(main_menu_widgets);

	GUI_DrawText_Wrapper("v1.07", SCREEN_WIDTH, SCREEN_HEIGHT - 9, 133, 0, 0x222);
}

static enum MenuAction
MainMenu_Loop(void)
{
	const int widgetID = GUI_Widget_HandleEvents(main_menu_widgets);

	switch (widgetID) {
		case 0x8000 | STR_PLAY_A_GAME:
			g_playerHouseID = HOUSE_MERCENARY;
			return MENU_PICK_HOUSE;

		case 0x8000 | STR_REPLAY_INTRODUCTION:
		case 0x8000 | STR_LOAD_GAME:
		case 0x8000 | STR_HALL_OF_FAME:
			break;

		case SCANCODE_ESCAPE:
		case 0x8000 | STR_EXIT_GAME:
			return MENU_EXIT_GAME;

		default:
			break;
	}

	return MENU_MAIN_MENU;
}

/*--------------------------------------------------------------*/

static void
PickHouse_Draw(void)
{
	Video_DrawCPS(String_GenerateFilename("HERALD"));

	GUI_Widget_DrawAll(pick_house_widgets);
}

static int
PickHouse_Loop(void)
{
	const int widgetID = GUI_Widget_HandleEvents(pick_house_widgets);

	switch (widgetID) {
		case SCANCODE_ESCAPE:
			return MENU_MAIN_MENU;

		case 0x8000 | HOUSE_HARKONNEN:
		case 0x8000 | HOUSE_ATREIDES:
		case 0x8000 | HOUSE_ORDOS:
			g_playerHouseID = widgetID & (~0x8000);
			return MENU_BRIEFING;

		default:
			break;
	}

	return MENU_PICK_HOUSE;
}

/*--------------------------------------------------------------*/

static void
Briefing_Initialise(void)
{
	MentatState *mentat = &g_mentat_state;

	MentatBriefing_InitText(g_playerHouseID, g_campaignID, MENTAT_BRIEFING_ORDERS, mentat);
	mentat->state = MENTAT_SHOW_TEXT;
}

static void
Briefing_Draw(void)
{
	MentatState *mentat = &g_mentat_state;

	Mentat_DrawBackground(g_playerHouseID);
	Mentat_Draw(g_playerHouseID);

	if (mentat->state == MENTAT_SHOW_TEXT) {
		MentatBriefing_DrawText(&g_mentat_state);
	}
	else if (mentat->state == MENTAT_IDLE) {
		GUI_Widget_DrawAll(briefing_proceed_repeat_widgets);
	}
}

static enum MenuAction
Briefing_Loop(void)
{
	MentatState *mentat = &g_mentat_state;
	bool redraw = false;
	int widgetID;

	widgetID = GUI_Widget_HandleEvents(briefing_proceed_repeat_widgets);
	if ((!(widgetID & 0x8000)) && (widgetID & SCANCODE_RELEASE))
		widgetID = 0;

	switch (widgetID) {
		case 0:
			break;

		case 0x8003: /* proceed */
			return MENU_PLAY_A_GAME;

		case 0x8004: /* repeat */
			mentat->state = MENTAT_SHOW_TEXT;
			mentat->text = mentat->buf;
			mentat->lines = mentat->lines0;
			redraw = true;
			break;

		default:
			if (mentat->state == MENTAT_SHOW_TEXT) {
				MentatBriefing_AdvanceText(mentat);
				redraw = true;
			}
			break;
	}

	return (redraw ? MENU_REDRAW : 0) | MENU_BRIEFING;
}

/*--------------------------------------------------------------*/

void
Menu_Run(void)
{
	enum MenuAction curr_menu = MENU_MAIN_MENU;
	bool redraw = true;

	Menu_Init();

	al_flush_event_queue(g_a5_input_queue);

	while (curr_menu != MENU_EXIT_GAME) {
		ALLEGRO_EVENT event;

		if (redraw) {
			redraw = false;
			al_clear_to_color(al_map_rgb(0, 0, 0));

			switch (curr_menu) {
				case MENU_MAIN_MENU:
					MainMenu_Draw();
					break;

				case MENU_PICK_HOUSE:
					PickHouse_Draw();
					break;

				case MENU_BRIEFING:
					Briefing_Draw();
					break;

				default:
					break;
			}

			Video_Tick();
		}

		al_wait_for_event(g_a5_input_queue, &event);
		switch (event.type) {
			case ALLEGRO_EVENT_DISPLAY_EXPOSE:
				redraw = true;
				break;

			default:
				InputA5_ProcessEvent(&event, true);
				break;
		}

		enum MenuAction res = curr_menu;
		switch (curr_menu) {
			case MENU_MAIN_MENU:
				res = MainMenu_Loop();
				break;

			case MENU_PICK_HOUSE:
				res = PickHouse_Loop();
				break;

			case MENU_BRIEFING:
				res = Briefing_Loop();
				break;

			case MENU_PLAY_A_GAME:
				goto play_a_game;

			case MENU_EXIT_GAME:
			case MENU_REDRAW:
				break;
		}

		if (curr_menu != res)
			redraw = true;

		if ((curr_menu & ~MENU_REDRAW) != (res & ~MENU_REDRAW)) {
			switch (res & ~MENU_REDRAW) {
				case MENU_BRIEFING:
					Briefing_Initialise();
					break;

				default:
					break;
			}
		}

		curr_menu = (res & ~MENU_REDRAW);
	}

	PrepareEnd();
	exit(0);

play_a_game:

	al_use_transform(&identity_transform);
	return;
}
