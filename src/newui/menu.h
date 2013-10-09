#ifndef NEWUI_MENU_H
#define NEWUI_MENU_H

#include "types.h"

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

struct MentatState;
struct Widget;

extern struct Widget *main_menu_widgets;
extern bool skirmish_regenerate_map;

extern void Menu_FreeWidgets(struct Widget *w);
extern void Menu_LoadPalette(void);
extern void MainMenu_SelectCampaign(int campaignID, int delta);
extern void Menu_Run(void);

extern void Extras_InitWidgets(void);
extern void Extras_FreeWidgets(void);
extern void Extras_Initialise(struct MentatState *mentat);
extern void Extras_Draw(struct MentatState *mentat);
extern enum MenuAction Extras_Loop(struct MentatState *mentat);
extern enum MenuAction PlayCutscene_Loop(void);

#endif
