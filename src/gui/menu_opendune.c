/* gui/menu_opendune.c */

/* gui/gui.c */

static void GUI_Widget_SetProperties(uint16 index, uint16 xpos, uint16 ypos, uint16 width, uint16 height)
{
	g_widgetProperties[index].xBase  = xpos;
	g_widgetProperties[index].yBase  = ypos;
	g_widgetProperties[index].width  = width;
	g_widgetProperties[index].height = height;

	if (g_curWidgetIndex == index) Widget_SetCurrentWidget(index);
}

/**
 * Show pick house screen.
 */
uint8 GUI_PickHouse(void)
{
	uint16 oldScreenID;
	Widget *w = NULL;
	uint8 palette[3 * 256];
	uint16 i;
	HouseType houseID;

	houseID = HOUSE_MERCENARY;

	memset(palette, 0, 256 * 3);

	Driver_Voice_Play(NULL, 0xFF);

	Voice_LoadVoices(5);

	while (true) {
		uint16 yes_no;

		for (i = 0; i < 3; i++) {
			static uint8 l_var_2BAC[3][3] = {
				/* x, y, shortcut */
				{ 16, 56, SCANCODE_A },
				{ 112, 56, SCANCODE_O },
				{ 208, 56, SCANCODE_H },
			};
			Widget *w2;

			w2 = GUI_Widget_Allocate(i + 1, l_var_2BAC[i][2], l_var_2BAC[i][0], l_var_2BAC[i][1], 0xFFFF, 0);

			w2->flags.all = 0x0;
			w2->flags.s.loseSelect = true;
			w2->flags.s.buttonFilterLeft = 1;
			w2->flags.s.buttonFilterRight = 1;
			w2->width  = 96;
			w2->height = 104;

			w = GUI_Widget_Link(w, w2);
		}

		Sprites_LoadImage(String_GenerateFilename("HERALD"), 3, NULL);

		GUI_Mouse_Hide_Safe();
		GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, 2, 0);
		GUI_SetPaletteAnimated(g_palette1, 15);
		GUI_Mouse_Show_Safe();

		houseID = HOUSE_INVALID;

		while (houseID == HOUSE_INVALID) {
			uint16 key = GUI_Widget_HandleEvents(w);

			GUI_PaletteAnimate();

			if ((key & 0x800) != 0) key = 0;

			switch (key) {
				case 0x8001: houseID = HOUSE_ATREIDES; break;
				case 0x8002: houseID = HOUSE_ORDOS; break;
				case 0x8003: houseID = HOUSE_HARKONNEN; break;
				default: break;
			}

			Video_Tick();
			sleepIdle();
		}

		GUI_Mouse_Hide_Safe();

		if (g_enableVoices != 0) {
			Sound_Output_Feedback(houseID + 62);

			while (Sound_StartSpeech()) sleepIdle();
		}

		while (w != NULL) {
			Widget *next = w->next;

			free(w);

			w = next;
		}

		GUI_SetPaletteAnimated(palette, 15);

		if (g_debugSkipDialogs || g_debugScenario) break;

		w = GUI_Widget_Link(w, GUI_Widget_Allocate(1, GUI_Widget_GetShortcut(String_Get_ByIndex(STR_YES)[0]), 168, 168, 373, 0));
		w = GUI_Widget_Link(w, GUI_Widget_Allocate(2, GUI_Widget_GetShortcut(String_Get_ByIndex(STR_NO)[0]), 240, 168, 375, 0));

		g_playerHouseID = HOUSE_MERCENARY;

		oldScreenID = GFX_Screen_SetActive(0);

		GUI_Mouse_Show_Safe();

		strncpy(g_readBuffer, String_Get_ByIndex(STR_HOUSE_HARKONNENFROM_THE_DARK_WORLD_OF_GIEDI_PRIME_THE_SAVAGE_HOUSE_HARKONNEN_HAS_SPREAD_ACROSS_THE_UNIVERSE_A_CRUEL_PEOPLE_THE_HARKONNEN_ARE_RUTHLESS_TOWARDS_BOTH_FRIEND_AND_FOE_IN_THEIR_FANATICAL_PURSUIT_OF_POWER + houseID * 40), g_readBufferSize);
		GUI_Mentat_Show(g_readBuffer, House_GetWSAHouseFilename(houseID), NULL, false);

		Sprites_LoadImage(String_GenerateFilename("MISC"), 3, g_palette1);

		GUI_Mouse_Hide_Safe();

		GUI_Screen_Copy(0, 0, 0, 0, 26, 24, 2, 0);

		GUI_Screen_Copy(0, 24 * (houseID + 1), 26, 0, 13, 24, 2, 0);

		GUI_Widget_DrawAll(w);

		GUI_Mouse_Show_Safe();

		while (true) {
			yes_no = GUI_Mentat_Loop(House_GetWSAHouseFilename(houseID), NULL, NULL, true, w);

			if ((yes_no & 0x8000) != 0) break;

			Video_Tick();
			sleepIdle();
		}

		if (yes_no == 0x8001) {
			Driver_Music_FadeOut();
		} else {
			GUI_SetPaletteAnimated(palette, 15);
		}

		while (w != NULL) {
			Widget *next = w->next;

			free(w);

			w = next;
		}

		Load_Palette_Mercenaries();
		Sprites_LoadTiles();

		GFX_Screen_SetActive(oldScreenID);

		while (Driver_Voice_IsPlaying()) sleepIdle();

		if (yes_no == 0x8001) break;

		Video_Tick();
		sleepIdle();
	}

	Music_Play(0);

	GUI_Palette_CreateRemap(houseID);

	Input_History_Clear();

	GUI_Mouse_Show_Safe();

	GUI_SetPaletteAnimated(palette, 15);

	return houseID;
}

/* gui/mentat.c */

/**
 * Show the Mentat screen with a dialog (Proceed / Repeat).
 * @param houseID The house to show the mentat of.
 * @param stringID The string to show.
 * @param wsaFilename The WSA to show.
 * @param musicID The Music to play.
 */
static void GUI_Mentat_ShowDialog(uint8 houseID, uint16 stringID, const char *wsaFilename, uint16 musicID)
{
	Widget *w1, *w2;

	if (g_debugSkipDialogs) return;

	w1 = GUI_Widget_Allocate(1, GUI_Widget_GetShortcut(String_Get_ByIndex(STR_PROCEED)[0]), 168, 168, 379, 0);
	w2 = GUI_Widget_Allocate(2, GUI_Widget_GetShortcut(String_Get_ByIndex(STR_REPEAT)[0]), 240, 168, 381, 0);

	w1 = GUI_Widget_Link(w1, w2);

	Sound_Output_Feedback(0xFFFE);

	Driver_Voice_Play(NULL, 0xFF);

	Music_Play(musicID);

	stringID += STR_HOUSE_HARKONNENFROM_THE_DARK_WORLD_OF_GIEDI_PRIME_THE_SAVAGE_HOUSE_HARKONNEN_HAS_SPREAD_ACROSS_THE_UNIVERSE_A_CRUEL_PEOPLE_THE_HARKONNEN_ARE_RUTHLESS_TOWARDS_BOTH_FRIEND_AND_FOE_IN_THEIR_FANATICAL_PURSUIT_OF_POWER + houseID * 40;

	do {
		strncpy(g_readBuffer, String_Get_ByIndex(stringID), g_readBufferSize);
		sleepIdle();
	} while (GUI_Mentat_Show(g_readBuffer, wsaFilename, w1, true) == 0x8002);

	free(w2);
	free(w1);

	if (musicID != 0xFFFF) Driver_Music_FadeOut();
}

/**
 * Show the briefing screen.
 */
void GUI_Mentat_ShowBriefing(void)
{
	GUI_Mentat_ShowDialog(g_playerHouseID, g_campaignID * 4 + 4, g_scenario.pictureBriefing, g_table_houseInfo[g_playerHouseID].musicBriefing);
}

/**
 * Show the win screen.
 */
void GUI_Mentat_ShowWin(void)
{
	GUI_Mentat_ShowDialog(g_playerHouseID, g_campaignID * 4 + 5, g_scenario.pictureWin, g_table_houseInfo[g_playerHouseID].musicWin);
}

/**
 * Show the lose screen.
 */
void GUI_Mentat_ShowLose(void)
{
	GUI_Mentat_ShowDialog(g_playerHouseID, g_campaignID * 4 + 6, g_scenario.pictureLose, g_table_houseInfo[g_playerHouseID].musicLose);
}

/* gui/menu_opendune.c. */

static void GUI_Widget_Undraw(Widget *w, uint8 colour)
{
	uint16 offsetX;
	uint16 offsetY;
	uint16 width;
	uint16 height;

	if (w == NULL) return;

	offsetX = w->offsetX + (g_widgetProperties[w->parentID].xBase << 3);
	offsetY = w->offsetY + g_widgetProperties[w->parentID].yBase;
	width = w->width;
	height = w->height;

	if (g_screenActiveID == 0) {
		GUI_Mouse_Hide_InRegion(offsetX, offsetY, offsetX + width, offsetY + height);
	}

	GUI_DrawFilledRectangle(offsetX, offsetY, offsetX + width, offsetY + height, colour);

	if (g_screenActiveID == 0) {
		GUI_Mouse_Show_InRegion();
	}
}

static void GUI_Window_BackupScreen(WindowDesc *desc)
{
	Widget_SetCurrentWidget(desc->index);

	GUI_Mouse_Hide_Safe();
	GFX_CopyToBuffer(g_curWidgetXBase * 8, g_curWidgetYBase, g_curWidgetWidth * 8, g_curWidgetHeight, GFX_Screen_Get_ByIndex(5));
	GUI_Mouse_Show_Safe();
}

static void GUI_Window_RestoreScreen(WindowDesc *desc)
{
	Widget_SetCurrentWidget(desc->index);

	GUI_Mouse_Hide_Safe();
	GFX_CopyFromBuffer(g_curWidgetXBase * 8, g_curWidgetYBase, g_curWidgetWidth * 8, g_curWidgetHeight, GFX_Screen_Get_ByIndex(5));
	GUI_Mouse_Show_Safe();
}

/**
 * Handles Click event for "Game controls" button.
 *
 * @param w The widget.
 */
static void GUI_Widget_GameControls_Click(Widget *w)
{
	WindowDesc *desc = &g_gameControlWindowDesc;
	bool loop;

	GUI_Window_BackupScreen(desc);

	GUI_Window_Create(desc);

	loop = true;
	while (loop) {
		Widget *w2 = g_widgetLinkedListTail;
		uint16 key = GUI_Widget_HandleEvents(w2);

		if ((key & 0x8000) != 0) {
			w = GUI_Widget_Get_ByIndex(w2, key & 0x7FFF);

			switch ((key & 0x7FFF) - 0x1E) {
				case 0:
					g_gameConfig.music ^= 0x1;
					if (g_gameConfig.music == 0) Driver_Music_Stop();
					break;

				case 1:
					g_gameConfig.sounds ^= 0x1;
					if (g_gameConfig.sounds == 0) Driver_Sound_Stop();
					break;

				case 2:
					if (++g_gameConfig.gameSpeed >= 5) g_gameConfig.gameSpeed = 0;
					break;

				case 3:
					g_gameConfig.hints ^= 0x1;
					break;

				case 4:
					g_gameConfig.autoScroll ^= 0x1;
					break;

				case 5:
					loop = false;
					break;

				default: break;
			}

			GUI_Widget_MakeNormal(w, false);

			GUI_Widget_Draw(w);
		}

		GUI_PaletteAnimate();
		Video_Tick();
		sleepIdle();
	}

	GUI_Window_RestoreScreen(desc);
}

static void ShadeScreen(void)
{
	uint16 i;

	memmove(g_palette_998A, g_palette1, 256 * 3);

	for (i = 0; i < 256 * 3; i++) g_palette1[i] = g_palette1[i] / 2;

	for (i = 0; i < 8; i++) memmove(g_palette1 + ((231 + i) * 3), &g_palette_998A[(231 + i) * 3], 3);

	GFX_SetPalette(g_palette_998A);
}

static void UnshadeScreen(void)
{
	memmove(g_palette1, g_palette_998A, 256 * 3);

	GFX_SetPalette(g_palette1);
}

static bool GUI_YesNo(uint16 stringID)
{
	WindowDesc *desc = &g_yesNoWindowDesc;
	bool loop = true;
	bool ret = false;

	desc->stringID = stringID;

	GUI_Window_BackupScreen(desc);

	GUI_Window_Create(desc);

	while (loop) {
		uint16 key = GUI_Widget_HandleEvents(g_widgetLinkedListTail);

		if ((key & 0x8000) != 0) {
			switch (key & 0x7FFF) {
				case 0x1E: ret = true; break;
				case 0x1F: ret = false; break;
				default: break;
			}
			loop = false;
		}

		GUI_PaletteAnimate();
		Video_Tick();
		sleepIdle();
	}

	GUI_Window_RestoreScreen(desc);

	return ret;
}

/**
 * Handles Click event for "Options" button.
 *
 * @param w The widget.
 * @return False, always.
 */
bool GUI_Widget_Options_Click(Widget *w)
{
	const uint16 cursor = g_cursorSpriteID;

	WindowDesc *desc = &g_optionsWindowDesc;
	bool loop;

	Video_SetCursor(SHAPE_CURSOR_NORMAL);

	Sprites_UnloadTiles();

	memmove(g_palette_998A, g_paletteActive, 256 * 3);

	Driver_Voice_Play(NULL, 0xFF);

	Timer_SetTimer(TIMER_GAME, false);

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

	ShadeScreen();

	GUI_Window_BackupScreen(desc);

	GUI_Window_Create(desc);

	loop = true;

	while (loop) {
		Widget *w2 = g_widgetLinkedListTail;
		uint16 key = GUI_Widget_HandleEvents(w2);

		if ((key & 0x8000) != 0) {
			w = GUI_Widget_Get_ByIndex(w2, key);

			GUI_Window_RestoreScreen(desc);

			switch ((key & 0x7FFF) - 0x1E) {
				case 0:
					if (GUI_Widget_SaveLoad_Click(false)) loop = false;
					break;

				case 1:
					if (GUI_Widget_SaveLoad_Click(true)) loop = false;
					break;

				case 2:
					GUI_Widget_GameControls_Click(w);
					break;

				case 3:
					/* "Are you sure you wish to restart?" */
					if (!GUI_YesNo(0x76)) break;

					loop = false;
					g_gameMode = GM_RESTART;
					break;

				case 4:
					/* "Are you sure you wish to pick a new house?" */
					if (!GUI_YesNo(0x77)) break;

					loop = false;
					Driver_Music_FadeOut();
					g_gameMode = GM_PICKHOUSE;
					break;

				case 5:
					loop = false;
					break;

				case 6:
					/* "Are you sure you want to quit playing?" */
					loop = !GUI_YesNo(0x65);
					g_var_38F8 = loop;

					Sound_Output_Feedback(0xFFFE);

					while (Driver_Voice_IsPlaying()) sleepIdle();
					break;

				default: break;
			}

			if (g_var_38F8 && loop) {
				GUI_Window_BackupScreen(desc);

				GUI_Window_Create(desc);
			}
		}

		GUI_PaletteAnimate();
		Video_Tick();
		sleepIdle();
	}

	g_textDisplayNeedsUpdate = true;

	Sprites_LoadTiles();
	GUI_DrawInterfaceAndRadar(0);

	UnshadeScreen();

	GUI_Widget_MakeSelected(w, false);

	Timer_SetTimer(TIMER_GAME, true);

	GameOptions_Save();

	Structure_Recount();
	Unit_Recount();

	Video_SetCursor(cursor);

	return false;
}

/**
 * Handles Click event for "Resume Game" button.
 *
 * @return True, always.
 */
bool GUI_Widget_HOF_Resume_Click(Widget *w)
{
	VARIABLE_NOT_USED(w);

	g_var_81E6 = true;

	return true;
}

/* opendune.c */

static uint16 s_var_8052 = 0;

/* main menu unknown. */
static uint16 GameLoop_B4E6_0000(uint16 arg06, uint32 arg08, uint16 arg0C)
{
	uint16 i = 0;

	if (arg08 == 0xFFFFFFFF) return arg06;

	while (arg06 != 0) {
		if ((arg08 & (1 << (arg0C + i))) != 0) arg06--;
		i++;
	}

	while (true) {
		if ((arg08 & (1 << (arg0C + i))) != 0) break;
		i++;
	}

	return i;
}

/* main menu widget drawing. */
static void GameLoop_B4E6_0108(uint16 arg06, char **strings, uint32 arg0C, uint16 arg10, uint16 arg12)
{
	WidgetProperties *props;
	uint16 left;
	uint16 old;
	uint16 top;
	uint8 i;

	props = &g_widgetProperties[21 + arg06];
	top = g_curWidgetYBase + props->yBase;
	left = (g_curWidgetXBase + props->xBase) << 3;

	old = GameLoop_B4E6_0000(props->fgColourBlink, arg0C, arg10);

	GUI_Mouse_Hide_Safe();

	for (i = 0; i < props->height; i++) {
		uint16 index = GameLoop_B4E6_0000(i, arg0C, arg10);
		uint16 pos = top + ((g_fontCurrent->height + arg12) * i);

		if (index == old) {
			GUI_DrawText_Wrapper(strings[index], left, pos, props->fgColourSelected, 0, 0x22);
		} else {
			GUI_DrawText_Wrapper(strings[index], left, pos, props->fgColourNormal, 0, 0x22);
		}
	}

	s_var_8052 = arg12;

	GUI_Mouse_Show_Safe();

	Input_History_Clear();
}

/* main menu text blinking. */
static void GameLoop_DrawText2(char *string, uint16 left, uint16 top, uint8 fgColourNormal, uint8 fgColourSelected, uint8 bgColour)
{
	uint8 i;

	for (i = 0; i < 3; i++) {
		GUI_Mouse_Hide_Safe();

		GUI_DrawText_Wrapper(string, left, top, fgColourSelected, bgColour, 0x22);
		Timer_Sleep(2);

		GUI_DrawText_Wrapper(string, left, top, fgColourNormal, bgColour, 0x22);
		GUI_Mouse_Show_Safe();
		Timer_Sleep(2);
	}
}

/* main menu mouse in widget. */
static bool GameLoop_IsInRange(uint16 x, uint16 y, uint16 minX, uint16 minY, uint16 maxX, uint16 maxY)
{
	return x >= minX && x <= maxX && y >= minY && y <= maxY;
}

/* main menu input. */
static uint16 GameLoop_HandleEvents(uint16 arg06, char **strings, uint32 arg10, uint16 arg14)
{
	uint8 last;
	uint16 result;
	uint16 key;
	uint16 top;
	uint16 left;
	uint16 minX;
	uint16 maxX;
	uint16 minY;
	uint16 maxY;
	uint16 lineHeight;
	uint8 fgColourNormal;
	uint8 fgColourSelected;
	uint8 old;
	WidgetProperties *props;
	uint8 current;

	props = &g_widgetProperties[21 + arg06];

	last = props->height - 1;
	old = props->fgColourBlink % (last + 1);
	current = old;

	result = 0xFFFF;

	top = g_curWidgetYBase + props->yBase;
	left = (g_curWidgetXBase + props->xBase) << 3;

	lineHeight = g_fontCurrent->height + s_var_8052;

	minX = (g_curWidgetXBase << 3) + (g_fontCurrent->maxWidth * props->xBase);
	minY = g_curWidgetYBase + props->yBase - s_var_8052 / 2;
	maxX = minX + (g_fontCurrent->maxWidth * props->width) - 1;
	maxY = minY + (props->height * lineHeight) - 1;

	fgColourNormal = props->fgColourNormal;
	fgColourSelected = props->fgColourSelected;

	key = 0;
	if (Input_IsInputAvailable()) {
		key = Input_GetNextKey();
	}

	/* if (g_var_7097 == 0) */
	{
		uint16 y = g_mouseY;

		if (GameLoop_IsInRange(g_mouseX, y, minX, minY, maxX, maxY)) {
			current = (y - minY) / lineHeight;
		}
	}

	switch (key) {
		case SCANCODE_KEYPAD_8: /* NUMPAD 8 / ARROW UP */
			if (current-- == 0) current = last;
			break;

		case SCANCODE_KEYPAD_2: /* NUMPAD 2 / ARROW DOWN */
			if (current++ == last) current = 0;
			break;

		case SCANCODE_KEYPAD_7: /* NUMPAD 7 / HOME */
		case SCANCODE_KEYPAD_9: /* NUMPAD 9 / PAGE UP */
			current = 0;
			break;

		case SCANCODE_KEYPAD_1: /* NUMPAD 1 / END */
		case SCANCODE_KEYPAD_3: /* NUMPAD 3 / PAGE DOWN */
			current = last;
			break;

		case MOUSE_LMB: /* MOUSE LEFT BUTTON */
		case MOUSE_RMB: /* MOUSE RIGHT BUTTON */
			if (GameLoop_IsInRange(g_mouseClickX, g_mouseClickY, minX, minY, maxX, maxY)) {
				current = (g_mouseClickY - minY) / lineHeight;
				result = current;
			}
			break;

		case SCANCODE_ENTER:
		case SCANCODE_KEYPAD_5: /* NUMPAD 5 / RETURN */
		case SCANCODE_SPACE: /* SPACE */
		/* case 0x61: */
			result = current;
			break;

		default: {
#if 0
			uint8 i;

			for (i = 0; i < props->height; i++) {
				char c1;
				char c2;

				c1 = toupper(*strings[GameLoop_B4E6_0000(i, arg10, arg14)]);
				c2 = toupper(Input_Keyboard_HandleKeys(key & 0xFF));

				if (c1 == c2) {
					result = i;
					current = i;
					break;
				}
			}
#endif
		} break;
	}

	if (current != old) {
		uint16 index;

		GUI_Mouse_Hide_Safe();

		index = GameLoop_B4E6_0000(old, arg10, arg14);

		GUI_DrawText_Wrapper(strings[index], left, top + (old * lineHeight), fgColourNormal, 0, 0x22);

		index = GameLoop_B4E6_0000(current, arg10, arg14);

		GUI_DrawText_Wrapper(strings[index], left, top + (current * lineHeight), fgColourSelected, 0, 0x22);

		GUI_Mouse_Show_Safe();
	}

	props->fgColourBlink = current;

	if (result == 0xFFFF) return 0xFFFF;

	result = GameLoop_B4E6_0000(result, arg10, arg14);

	GUI_Mouse_Hide_Safe();
	GameLoop_DrawText2(strings[result], left, top + (current * lineHeight), fgColourNormal, fgColourSelected, 0);
	GUI_Mouse_Show_Safe();

	return result;
}
