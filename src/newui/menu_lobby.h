#ifndef NEWUI_MENU_LOBBY_H
#define NEWUI_MENU_LOBBY_H

#include "../gui/widget.h"

enum MapOptions {
	MAP_OPTIONS_WIDGET_SEED_RANDOM = 20,
	MAP_OPTIONS_WIDGET_LOSE_CONDITION_STRUCTURES = 50,
	MAP_OPTIONS_STARTING_CREDITS_MIN = 1000,
	MAP_OPTIONS_STARTING_CREDITS_MAX = 10000,
	MAP_OPTIONS_SPICE_MIN = 0,
	MAP_OPTIONS_SPICE_MAX = 128,
	MAP_OPTIONS_GUI_LINE_HEIGHT = 12,
	MAP_OPTIONS_GUI_MAIN_X = 11,
	MAP_OPTIONS_GUI_MAIN_Y = 83,
	MAP_OPTIONS_GUI_MAIN_W = 110,
	MAP_OPTIONS_GUI_MAIN_H = 90,
	MAP_OPTIONS_GUI_MAP_X = 126,
	MAP_OPTIONS_GUI_MAP_Y = MAP_OPTIONS_GUI_MAIN_Y,
	MAP_OPTIONS_GUI_MAP_W = 183,
	MAP_OPTIONS_GUI_MAP_H = 90,
	MAP_OPTIONS_GUI_ERROR_X = SCREEN_WIDTH / 2,
	MAP_OPTIONS_GUI_ERROR_Y = 43,
	MAP_OPTIONS_GUI_ERROR_Y_LINE_1 = 38,
	MAP_OPTIONS_GUI_ERROR_Y_LINE_2 = 48
};

extern void Lobby_InitWidgets(void);
extern void Lobby_FreeWidgets(void);

extern void PickLobby_Draw(void);
extern enum MenuAction PickLobby_Loop(void);

extern void MapOptionsLobby_Initialise(void);
extern void MapOptionsLobby_Draw(void);
extern enum MenuAction MapOptionsLobby_Loop(void);

extern void SkirmishLobby_Initialise(void);
extern void SkirmishLobby_Draw(void);
extern enum MenuAction SkirmishLobby_Loop(void);

extern void MultiplayerLobby_Initialise(void);
extern void MultiplayerLobby_Draw(void);
extern enum MenuAction MultiplayerLobby_Loop(void);

#endif
