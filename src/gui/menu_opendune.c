/* gui/menu_opendune.c */

/* gui/gui.c */

MSVC_PACKED_BEGIN
typedef struct StrategicMapData {
	/* 0000(2)   */ PACK int16 index;      /*!< ?? */
	/* 0002(2)   */ PACK int16 arrow;      /*!< ?? */
	/* 0004(2)   */ PACK int16 offsetX;    /*!< ?? */
	/* 0006(2)   */ PACK int16 offsetY;    /*!< ?? */
} GCC_PACKED StrategicMapData;
MSVC_PACKED_END
assert_compile(sizeof(StrategicMapData) == 0x8);

static void GUI_Widget_SetProperties(uint16 index, uint16 xpos, uint16 ypos, uint16 width, uint16 height)
{
	g_widgetProperties[index].xBase  = xpos;
	g_widgetProperties[index].yBase  = ypos;
	g_widgetProperties[index].width  = width;
	g_widgetProperties[index].height = height;

	if (g_curWidgetIndex == index) Widget_SetCurrentWidget(index);
}

uint16 GUI_HallOfFame_Tick(void)
{
	static int64_t l_timerNext = 0;
	static int16 colouringDirection = 1;

	if (l_timerNext >= Timer_GetTicks()) return 0;
	l_timerNext = Timer_GetTicks() + 2;

	if (*s_palette1_houseColour >= 63) {
		colouringDirection = -1;
	} else if (*s_palette1_houseColour <= 35) {
		colouringDirection = 1;
	}

	*s_palette1_houseColour += colouringDirection;

	GFX_SetPalette(g_palette1);

	return 0;
}

static void GUI_HallOfFame_DrawRank(uint16 score, bool fadeIn)
{
	GUI_DrawText_Wrapper(String_Get_ByIndex(_rankScores[GUI_HallOfFame_GetRank(score)].rankString), SCREEN_WIDTH / 2, 49, 6, 0, 0x122);

	if (!fadeIn) return;

	GUI_Screen_FadeIn(10, 49, 10, 49, 20, 12, 2, 0);
}

static void GUI_HallOfFame_DrawBackground(uint16 score, bool hallOfFame)
{
	uint16 oldScreenID;
	uint16 colour;
	uint16 offset;

	oldScreenID = GFX_Screen_SetActive(2);;

	HallOfFame_DrawBackground(g_playerHouseID, hallOfFame);

	if (hallOfFame) {
		if (score != 0xFFFF) GUI_HallOfFame_DrawRank(score, false);
	}

	if (score != 0xFFFF) {
		char buffer[64];
		snprintf(buffer, sizeof(buffer), String_Get_ByIndex(STR_TIME_DH_DM), s_ticksPlayed / 60, s_ticksPlayed % 60);

		if (s_ticksPlayed < 60) {
			char *hours = strchr(buffer, '0');
			while (*hours != ' ') memmove(hours, hours + 1, strlen(hours));
		}

		/* "Score: %d" */
		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_SCORE_D), 72, 15, 15, 0, 0x22, score);
		GUI_DrawText_Wrapper(buffer, 248, 15, 15, 0, 0x222);
		/* "You have attained the rank of" */
		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_YOU_HAVE_ATTAINED_THE_RANK_OF), SCREEN_WIDTH / 2, 38, 15, 0, 0x122);
	} else {
		/* "Hall of Fame" */
		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_HALL_OF_FAME2), SCREEN_WIDTH / 2, 15, 15, 0, 0x122);
	}

	switch (g_playerHouseID) {
		case HOUSE_HARKONNEN:
			colour = 149;
			offset = 0;
			break;

		default:
			colour = 165;
			offset = 2;
			break;

		case HOUSE_ORDOS:
			colour = 181;
			offset = 1;
			break;
	}

	s_palette1_houseColour = g_palette1 + 255 * 3;
	memcpy(s_palette1_houseColour, g_palette1 + colour * 3, 3);
	s_palette1_houseColour += offset;

	if (!hallOfFame) GUI_HallOfFame_Tick();

	GFX_Screen_SetActive(oldScreenID);
}

static void GUI_EndStats_Sleep(uint16 delay)
{
	for (int timeout = 0; timeout < delay; timeout++) {
		GUI_HallOfFame_Tick();
		Video_Tick();
		Timer_Wait();
	}
}

/**
 * Shows the stats at end of scenario.
 * @param killedAllied The amount of destroyed allied units.
 * @param killedEnemy The amount of destroyed enemy units.
 * @param destroyedAllied The amount of destroyed allied structures.
 * @param destroyedEnemy The amount of destroyed enemy structures.
 * @param harvestedAllied The amount of spice harvested by allies.
 * @param harvestedEnemy The amount of spice harvested by enemies.
 * @param score The base score.
 * @param houseID The houseID of the player.
 */
void GUI_EndStats_Show(uint16 killedAllied, uint16 killedEnemy, uint16 destroyedAllied, uint16 destroyedEnemy, uint16 harvestedAllied, uint16 harvestedEnemy, int16 score, uint8 houseID)
{
	uint16 loc06;
	uint16 oldScreenID;
	uint16 loc16;
	uint16 loc18;
	uint16 loc1A;
	uint16 loc32[3][2][2];
	uint16 i;

	s_ticksPlayed = ((g_timerGame - g_tickScenarioStart) / 3600) + 1;

	score = Update_Score(score, &harvestedAllied, &harvestedEnemy, houseID);

	loc16 = (g_scenarioID == 1) ? 2 : 3;

	GUI_Mouse_Hide_Safe();

	GUI_ChangeSelectionType(SELECTIONTYPE_MENTAT);

	oldScreenID = GFX_Screen_SetActive(2);

	GUI_HallOfFame_DrawBackground(score, false);

	GUI_DrawTextOnFilledRectangle(String_Get_ByIndex(STR_SPICE_HARVESTED_BY), 83);
	GUI_DrawTextOnFilledRectangle(String_Get_ByIndex(STR_UNITS_DESTROYED_BY), 119);
	if (g_scenarioID != 1) GUI_DrawTextOnFilledRectangle(String_Get_ByIndex(STR_BUILDINGS_DESTROYED_BY), 155);

	loc06 = max(Font_GetStringWidth(String_Get_ByIndex(STR_YOU)), Font_GetStringWidth(String_Get_ByIndex(STR_ENEMY)));

	loc18 = loc06 + 19;
	loc1A = 261 - loc18;

	for (i = 0; i < loc16; i++) {
		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_YOU), loc18 - 4,  92 + (i * 36), 0xF, 0, 0x221);
		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_ENEMY), loc18 - 4, 101 + (i * 36), 0xF, 0, 0x221);
	}

	Music_Play(17 + Tools_RandomRange(0, 5));

	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, 2, 0);

	Input_History_Clear();

	loc32[0][0][0] = harvestedAllied;
	loc32[0][1][0] = harvestedEnemy;
	loc32[1][0][0] = killedEnemy;
	loc32[1][1][0] = killedAllied;
	loc32[2][0][0] = destroyedEnemy;
	loc32[2][1][0] = destroyedAllied;

	for (i = 0; i < loc16; i++) {
		uint16 loc08 = loc32[i][0][0];
		uint16 loc0A = loc32[i][1][0];

		if (loc08 >= 65000) loc08 = 65000;
		if (loc0A >= 65000) loc0A = 65000;

		loc32[i][0][0] = loc08;
		loc32[i][1][0] = loc0A;

		loc06 = max(loc08, loc0A);

		loc32[i][0][1] = 1 + ((loc06 > loc1A) ? (loc06 / loc1A) : 0);
		loc32[i][1][1] = 1 + ((loc06 > loc1A) ? (loc06 / loc1A) : 0);
	}

	GUI_EndStats_Sleep(45);
	GUI_HallOfFame_DrawRank(score, true);
	GUI_EndStats_Sleep(45);

	for (i = 0; i < loc16; i++) {
		uint16 loc02;

		GUI_HallOfFame_Tick();

		for (loc02 = 0; loc02 < 2; loc02++) {
			uint8 colour;
			uint16 loc04;
			uint16 locdi;
			uint16 loc0E;
			uint16 loc10;
			uint16 loc0C;

			GUI_HallOfFame_Tick();

			colour = (loc02 == 0) ? 255 : 209;
			loc04 = loc18;

			locdi = 93 + (i * 36) + (loc02 * 9);

			loc0E = loc32[i][loc02][0];
			loc10 = loc32[i][loc02][1];

			for (loc0C = 0; loc0C < loc0E; loc0C += loc10) {
				GUI_DrawFilledRectangle(271, locdi, 303, locdi + 5, 226);

				GUI_DrawText_Wrapper("%u", 287, locdi - 1, 0x14, 0, 0x121, loc0C);

				GUI_HallOfFame_Tick();

				const int64_t timeout = 1 + Timer_GetTicks();

				GUI_DrawLine(loc04, locdi, loc04, locdi + 5, colour);

				loc04++;

				GUI_DrawLine(loc04, locdi + 1, loc04, locdi + 6, 12);

				GFX_Screen_Copy2(loc18, locdi, loc18, locdi, 304, 7, 2, 0, false);

				Driver_Sound_Play(52, 0xFF);

				GUI_EndStats_Sleep(timeout - Timer_GetTicks());
			}

			GUI_DrawFilledRectangle(271, locdi, 303, locdi + 5, 226);

			GUI_DrawText_Wrapper("%u", 287, locdi - 1, 0xF, 0, 0x121, loc0E);

			GFX_Screen_Copy2(loc18, locdi, loc18, locdi, 304, 7, 2, 0, false);

			Driver_Sound_Play(38, 0xFF);

			GUI_EndStats_Sleep(12);
		}

		GUI_EndStats_Sleep(60);
	}

	GUI_Mouse_Show_Safe();

	Input_History_Clear();

	while (true) {
		GUI_HallOfFame_Tick();
		if (Input_IsInputAvailable()) break;
		Video_Tick();
		sleepIdle();
	}

	Input_History_Clear();

	GUI_HallOfFame_Show(score);

	memset(g_palette1 + 255 * 3, 0, 3);

	GFX_Screen_SetActive(oldScreenID);

	Driver_Music_FadeOut();
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

static void GUI_StrategicMap_AnimateArrows(void)
{
	static int64_t l_arrowAnimationTimeout = 0;
	static int s_arrowAnimationState = 0;

	if (l_arrowAnimationTimeout >= Timer_GetTicks()) return;
	l_arrowAnimationTimeout = Timer_GetTicks() + 7;

	s_arrowAnimationState = (s_arrowAnimationState + 1) % 4;

	memcpy(g_palette1 + 251 * 3, s_var_81BA + s_arrowAnimationState * 3, 4 * 3);

	GFX_SetPalette(g_palette1);
}

static void GUI_StrategicMap_AnimateSelected(uint16 selected, StrategicMapData *data)
{
	char key[4];
	char buffer[81];
	int16 x;
	int16 y;
	uint8 *sprite;
	uint16 width;
	uint16 height;
	uint16 i;

	GUI_Palette_CreateRemap(g_playerHouseID);

	for (i = 0; i < 20; i++) {
		GUI_StrategicMap_AnimateArrows();

		if (data[i].index == 0 || data[i].index == selected) continue;

		GUI_Mouse_Hide_Safe();
		GFX_Screen_Copy2(i * 16, 0, data[i].offsetX, data[i].offsetY, 16, 16, 2, 0, false);
		GUI_Mouse_Show_Safe();
	}

	sprintf(key, "%d", selected);

	Ini_GetString("PIECES", key, NULL, buffer, sizeof(buffer) - 1, g_fileRegionINI);
	sscanf(buffer, "%hd,%hd", &x, &y);

	sprite = g_sprites[477 + selected];
	width  = Sprite_GetWidth(sprite);
	height = Sprite_GetHeight(sprite);

	x += 8;
	y += 24;

	GUI_Mouse_Hide_Safe();
	GFX_Screen_Copy2(x, y, 16, 16, width, height, 0, 2, false);
	GUI_Mouse_Show_Safe();

	GFX_Screen_Copy2(16, 16, 176, 16, width, height, 2, 2, false);

	GUI_DrawSprite(2, sprite, 16, 16, 0, 0x100, g_remap, 1);

	for (i = 0; i < 20; i++) {
		GUI_StrategicMap_AnimateArrows();

		if (data[i].index != selected) continue;

		GUI_DrawSprite(2, g_sprites[505 + data[i].arrow], data[i].offsetX + 16 - x, data[i].offsetY + 16 - y, 0, 0x100, g_remap, 1);
	}

	for (i = 0; i < 4; i++) {
		GUI_Mouse_Hide_Safe();
		GFX_Screen_Copy2((i % 2 == 0) ? 16 : 176, 16, x, y, width, height, 2, 0, false);
		GUI_Mouse_Show_Safe();

		for (int timeout = 0; timeout < 20; timeout++) {
			GUI_StrategicMap_AnimateArrows();
			Video_Tick();
			Timer_Wait();
		}
	}
}

/**
 * Get region bit of the strategic map.
 * @param region Region to obtain.
 * @return Value of the region bit.
 */
static bool GUI_StrategicMap_GetRegion(uint16 region)
{
	return (g_strategicRegionBits & (1 << region)) != 0;
}

/**
 * Set or reset a region of the strategic map.
 * @param region Region to change.
 * @param set Region must be set.
 */
static void GUI_StrategicMap_SetRegion(uint16 region, bool set)
{
	if (set) {
		g_strategicRegionBits |= (1 << region);
	} else {
		g_strategicRegionBits &= ~(1 << region);
	}
}

static int16 GUI_StrategicMap_ClickedRegion(void)
{
	uint16 key;

	GUI_StrategicMap_AnimateArrows();

	if (!Input_IsInputAvailable()) return 0;

	key = Input_GetNextKey();
	if (key != MOUSE_LMB) return 0;

	return g_fileRgnclkCPS[(g_mouseClickY - 24) * 304 + g_mouseClickX - 8];
}

static bool GUI_StrategicMap_FastForwardToggleWithESC(void)
{
	if (!Input_IsInputAvailable())
		return s_strategicMapFastForward;

	if (Input_GetNextKey() != SCANCODE_ESCAPE)
		return s_strategicMapFastForward;

	s_strategicMapFastForward = !s_strategicMapFastForward;

	Input_History_Clear();

	return s_strategicMapFastForward;
}

static void GUI_StrategicMap_DrawText(const char *string)
{
	static int64_t l_timerNext = 0;
	uint16 oldScreenID;
	uint16 y;

	oldScreenID = GFX_Screen_SetActive(2);

	GUI_Screen_Copy(8, 165, 8, 186, 24, 14, 0, 2);

	GUI_DrawFilledRectangle(64, 172, 255, 185, GFX_GetPixel(64, 186));

	GUI_DrawText_Wrapper(string, 64, 175, 12, 0, 0x12);

	Timer_Sleep(Timer_GetTicks() + 90 - l_timerNext);

	for (y = 185; y > 172; y--) {
		GUI_Screen_Copy(8, y, 8, 165, 24, 14, 2, 0);

		for (int timeout = 0; timeout < 3; timeout++) {
			if (GUI_StrategicMap_FastForwardToggleWithESC()) break;
			Video_Tick();
			Timer_Wait();
		}
	}

	l_timerNext = Timer_GetTicks() + 90;

	GFX_Screen_SetActive(oldScreenID);
}

static uint16 GUI_StrategicMap_ScenarioSelection(uint16 campaignID)
{
	uint16 count;
	char key[6];
	bool loop = true;
	bool loc12 = true;
	char category[16];
	StrategicMapData data[20];
	uint16 scenarioID;
	uint16 region;
	uint16 i;

	GUI_Palette_CreateRemap(g_playerHouseID);

	sprintf(category, "GROUP%d", campaignID);

	memset(data, 0, 20 * sizeof(StrategicMapData));

	for (i = 0; i < 20; i++) {
		char buffer[81];

		sprintf(key, "REG%d", i + 1);

		if (Ini_GetString(category, key, NULL, buffer, sizeof(buffer) - 1, g_fileRegionINI) == NULL) break;

		sscanf(buffer, "%hd,%hd,%hd,%hd", &data[i].index, &data[i].arrow, &data[i].offsetX, &data[i].offsetY);

		if (data[i].index != 0 && !GUI_StrategicMap_GetRegion(data[i].index)) loc12 = false;

		GFX_Screen_Copy2(data[i].offsetX, data[i].offsetY, i * 16, 152, 16, 16, 2, 2, false);
		GFX_Screen_Copy2(data[i].offsetX, data[i].offsetY, i * 16, 0, 16, 16, 2, 2, false);
		GUI_DrawSprite(2, g_sprites[505 + data[i].arrow], i * 16, 152, 0, 0x100, g_remap, 1);
	}

	count = i;

	if (loc12) {
		for (i = 0; i < count; i++) {
			GUI_StrategicMap_SetRegion(data[i].index, false);
		}
	} else {
		for (i = 0; i < count; i++) {
			if (GUI_StrategicMap_GetRegion(data[i].index)) data[i].index = 0;
		}
	}

	GUI_Mouse_Hide_Safe();

	for (i = 0; i < count; i++) {
		if (data[i].index == 0) continue;

		GFX_Screen_Copy2(i * 16, 152, data[i].offsetX, data[i].offsetY, 16, 16, 2, 0, false);
	}

	GUI_Mouse_Show_Safe();
	Input_History_Clear();

	while (loop) {
		region = GUI_StrategicMap_ClickedRegion();
		Video_Tick();

		if (region == 0) {
			sleepIdle();
			continue;
		}

		for (i = 0; i < count; i++) {
			GUI_StrategicMap_AnimateArrows();

			if (data[i].index == region) {
				loop = false;
				scenarioID = i;
				break;
			}
		}

		sleepIdle();
	}

	GUI_StrategicMap_SetRegion(region, true);

	GUI_StrategicMap_DrawText("");

	GUI_StrategicMap_AnimateSelected(region, data);

	scenarioID += (campaignID - 1) * 3 + 2;

	if (campaignID > 7) scenarioID--;
	if (campaignID > 8) scenarioID--;

	return scenarioID;
}

static void GUI_StrategicMap_ReadHouseRegions(uint8 houseID, uint16 campaignID)
{
	char key[4];
	char buffer[100];
	char groupText[16];
	char *s = buffer;

	strncpy(key, g_table_houseInfo[houseID].name, 3);
	key[3] = '\0';

	snprintf(groupText, sizeof(groupText), "GROUP%d", campaignID);

	if (Ini_GetString(groupText, key, NULL, buffer, sizeof(buffer) - 1, g_fileRegionINI) == NULL) return;

	while (*s != '\0') {
		uint16 region = atoi(s);

		if (region != 0) g_regions[region] = houseID;

		while (*s != '\0') {
			if (*s++ == ',') break;
		}
	}
}

static void GUI_StrategicMap_DrawRegion(uint8 houseId, uint16 region, bool progressive)
{
	char key[4];
	char buffer[81];
	int16 x;
	int16 y;
	uint8 *sprite;

	GUI_Palette_CreateRemap(houseId);

	sprintf(key, "%d", region);

	Ini_GetString("PIECES", key, NULL, buffer, sizeof(buffer), g_fileRegionINI);
	sscanf(buffer, "%hd,%hd", &x, &y);

	sprite = g_sprites[477 + region];

	GUI_DrawSprite(3, sprite, x + 8, y + 24, 0, 0x100, g_remap, 1);

	if (!progressive) return;

	GUI_Screen_FadeIn2(x + 8, y + 24, Sprite_GetWidth(sprite), Sprite_GetHeight(sprite), 2, 0, GUI_StrategicMap_FastForwardToggleWithESC() ? 0 : 1, false);
}

static void GUI_StrategicMap_PrepareRegions(uint16 campaignID)
{
	uint16 i;

	for (i = 0; i < campaignID; i++) {
		GUI_StrategicMap_ReadHouseRegions(HOUSE_HARKONNEN, i + 1);
		GUI_StrategicMap_ReadHouseRegions(HOUSE_ATREIDES, i + 1);
		GUI_StrategicMap_ReadHouseRegions(HOUSE_ORDOS, i + 1);
		GUI_StrategicMap_ReadHouseRegions(HOUSE_SARDAUKAR, i + 1);
	}

	for (i = 0; i < g_regions[0]; i++) {
		if (g_regions[i + 1] == 0xFFFF) continue;

		GUI_StrategicMap_DrawRegion((uint8)g_regions[i + 1], i + 1, false);
	}
}

static void GUI_StrategicMap_ShowProgression(uint16 campaignID)
{
	char key[10];
	char category[10];
	char buffer[100];
	uint16 i;

	sprintf(category, "GROUP%d", campaignID);

	for (i = 0; i < 6; i++) {
		uint8 houseID = (g_playerHouseID + i) % 6;
		char *s = buffer;

		strncpy(key, g_table_houseInfo[houseID].name, 3);
		key[3] = '\0';

		if (Ini_GetString(category, key, NULL, buffer, 99, g_fileRegionINI) == NULL) continue;

		while (*s != '\0') {
			uint16 region = atoi(s);

			if (region != 0) {
				char buffer[81];

				sprintf(key, "%sTXT%d", g_languageSuffixes[g_config.language], region);

				if (Ini_GetString(category, key, NULL, buffer, sizeof(buffer), g_fileRegionINI) != NULL) {
					GUI_StrategicMap_DrawText(buffer);
				}

				GUI_StrategicMap_DrawRegion(houseID, region, true);
			}

			while (*s != '\0') {
				if (*s++ == ',') break;
			}
		}
	}

	GUI_StrategicMap_DrawText("");
}

uint16 GUI_StrategicMap_Show(uint16 campaignID, bool win)
{
	uint16 scenarioID;
	uint16 previousCampaignID;
	uint16 x;
	uint16 y;
	uint16 oldScreenID;
	uint8 palette[3 * 256];
	uint8 loc316[12];

	if (campaignID == 0) return 1;

	Timer_Sleep(10);
	Music_Play(0x1D);

	memset(palette, 0, 256 * 3);

	previousCampaignID = campaignID - (win ? 1 : 0);
	oldScreenID = GFX_Screen_SetActive(4);

	GUI_SetPaletteAnimated(palette, 15);

	Sprites_LoadImage("MAPMACH.CPS", 5, g_palette_998A);

	GUI_Palette_RemapScreen(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 5, g_remap);

	x = 0;
	y = 0;

	switch (g_playerHouseID) {
		case HOUSE_HARKONNEN:
			x = 0;
			y = 152;
			break;

		default:
			x = 33;
			y = 152;
			break;

		case HOUSE_ORDOS:
			x = 1;
			y = 24;
			break;
	}

	memcpy(loc316, g_palette1 + 251 * 3, 12);
	memcpy(s_var_81BA, g_palette1 + (144 + (g_playerHouseID * 16)) * 3, 4 * 3);
	memcpy(s_var_81BA + 4 * 3, s_var_81BA, 4 * 3);

	GUI_Screen_Copy(x, y, 0, 152, 7, 40, 4, 4);
	GUI_Screen_Copy(x, y, 33, 152, 7, 40, 4, 4);

	switch (g_config.language) {
		case LANGUAGE_GERMAN:
			GUI_Screen_Copy(1, 120, 1, 0, 38, 24, 4, 4);
			break;

		case LANGUAGE_FRENCH:
			GUI_Screen_Copy(1, 96, 1, 0, 38, 24, 4, 4);
			break;

		default: break;
	}

	GUI_DrawFilledRectangle(8, 24, 311, 143, 12);

	GUI_Mouse_Hide_Safe();
	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, 4, 0);
	GUI_SetPaletteAnimated(g_palette1, 15);
	GUI_Mouse_Show_Safe();

	s_strategicMapFastForward = false;

	if (win && campaignID == 1) {
		Sprites_LoadImage("PLANET.CPS", 3, g_palette_998A);

		GUI_StrategicMap_DrawText(String_Get_ByIndex(STR_THREE_HOUSES_HAVE_COME_TO_DUNE));

		GUI_Screen_FadeIn2(8, 24, 304, 120, 2, 0, 0, false);

		Input_History_Clear();

		Sprites_CPS_LoadRegionClick();

		for (int timeout = 0; timeout < 120; timeout++) {
			if (GUI_StrategicMap_FastForwardToggleWithESC()) break;
			Video_Tick();
			Timer_Wait();
		}

		Sprites_LoadImage("DUNEMAP.CPS", 3 , g_palette_998A);

		GUI_StrategicMap_DrawText(String_Get_ByIndex(STR_TO_TAKE_CONTROL_OF_THE_LAND));

		GUI_Screen_FadeIn2(8, 24, 304, 120, 2, 0, GUI_StrategicMap_FastForwardToggleWithESC() ? 0 : 1, false);

		for (int timeout = 0; timeout < 60; timeout++) {
			if (GUI_StrategicMap_FastForwardToggleWithESC()) break;
			Video_Tick();
			Timer_Wait();
		}

		GUI_StrategicMap_DrawText(String_Get_ByIndex(STR_THAT_HAS_BECOME_DIVIDED));
	} else {
		Sprites_CPS_LoadRegionClick();
	}

	Sprites_LoadImage("DUNERGN.CPS", 3, g_palette_998A);

	GFX_Screen_SetActive(2);

	GUI_StrategicMap_PrepareRegions(previousCampaignID);

	if (GUI_StrategicMap_FastForwardToggleWithESC()) {
		GUI_Screen_Copy(1, 24, 1, 24, 38, 120, 2, 0);
	} else {
		GUI_Screen_FadeIn2(8, 24, 304, 120, 2, 0, 0, false);
	}

	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, 0, 2);

	if (campaignID != previousCampaignID) GUI_StrategicMap_ShowProgression(campaignID);

	GUI_Mouse_Show_Safe();

	if (*g_regions >= campaignID) {
		GUI_StrategicMap_DrawText(String_Get_ByIndex(STR_SELECT_YOUR_NEXT_REGION));

		scenarioID = GUI_StrategicMap_ScenarioSelection(campaignID);
	} else {
		scenarioID = 0;
	}

	Driver_Music_FadeOut();

	GFX_Screen_SetActive(oldScreenID);

	Input_History_Clear();

	memcpy(g_palette1 + 251 * 3, loc316, 12);

	GUI_SetPaletteAnimated(palette, 15);

	GUI_Mouse_Hide_Safe();
	GUI_ClearScreen(0);
	GUI_Mouse_Show_Safe();

	GFX_SetPalette(g_palette1);

	return scenarioID;
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
 * Shows the Help window.
 * @param proceed Display a "Proceed" button if true, "Exit" otherwise.
 */
static void GUI_Mentat_ShowHelpList(bool proceed)
{
	uint16 oldScreenID;

	oldScreenID = GFX_Screen_SetActive(2);

	Input_History_Clear();

	GUI_Mentat_Display(NULL, g_playerHouseID);

	g_widgetMentatFirst = GUI_Widget_Allocate(1, GUI_Widget_GetShortcut(*String_Get_ByIndex(STR_EXIT)), 200, 168, proceed ? 379 : 377, 5);
	g_widgetMentatFirst->shortcut2 = 'n';

	GUI_Mentat_Create_HelpScreen_Widgets();

	GUI_Mouse_Hide_Safe();
	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, 2, 0);
	GUI_Mouse_Show_Safe();

	GUI_Mentat_LoadHelpSubjects(true);

	GUI_Mentat_Draw(true);

	GFX_Screen_SetActive(0);

	GUI_Mentat_HelpListLoop();

	free(g_widgetMentatFirst); g_widgetMentatFirst = NULL;

	Load_Palette_Mercenaries();

	GUI_Widget_Free_WithScrollbar(g_widgetMentatScrollbar);
	g_widgetMentatScrollbar = NULL;

	free(g_widgetMentatScrollUp); g_widgetMentatScrollUp = NULL;
	free(g_widgetMentatScrollDown); g_widgetMentatScrollDown = NULL;

	GFX_Screen_SetActive(oldScreenID);
}

/**
 * Handle clicks on the Mentat widget.
 * @return True, always.
 */
bool GUI_Widget_Mentat_Click(Widget *w)
{
	VARIABLE_NOT_USED(w);

	Video_SetCursor(SHAPE_CURSOR_NORMAL);

	Sound_Output_Feedback(0xFFFE);

	Driver_Voice_Play(NULL, 0xFF);

	Music_Play(g_table_houseInfo[g_playerHouseID].musicBriefing);

	Sprites_UnloadTiles();

	Timer_SetTimer(TIMER_GAME, false);

	GUI_Mentat_ShowHelpList(false);

	Timer_SetTimer(TIMER_GAME, true);

	Driver_Sound_Play(1, 0xFF);

	Sprites_LoadTiles();

	g_textDisplayNeedsUpdate = true;

	GUI_DrawInterfaceAndRadar(0);

	Music_Play(Tools_RandomRange(0, 5) + 8);

	return true;
}

/**
 * Show the Mentat screen.
 * @param spriteBuffer The buffer of the strings.
 * @param wsaFilename The WSA to show.
 * @param w The widgets to handle. Can be NULL for no widgets.
 * @param unknown A boolean.
 * @return Return value of GUI_Widget_HandleEvents() or f__B4DA_0AB8_002A_AAB2() (latter when no widgets).
 */
uint16 GUI_Mentat_Show(char *stringBuffer, const char *wsaFilename, Widget *w, bool unknown)
{
	uint16 ret;

	Sprites_UnloadTiles();

	GUI_Mentat_Display(wsaFilename, g_playerHouseID);

	GFX_Screen_SetActive(2);

	Widget_SetAndPaintCurrentWidget(8);

	if (wsaFilename != NULL) {
		void *wsa;

		wsa = WSA_LoadFile(wsaFilename, GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), false);
		WSA_DisplayFrame(wsa, 0, g_curWidgetXBase * 8, g_curWidgetYBase, 2);
		WSA_Unload(wsa);
	}

	GUI_DrawSprite(2, g_sprites[397 + g_playerHouseID * 15], g_shoulderLeft, g_shoulderTop, 0, 0);
	GFX_Screen_SetActive(0);

	GUI_Mouse_Hide_Safe();
	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, 2, 0);
	GUI_Mouse_Show_Safe();

	GUI_SetPaletteAnimated(g_palette1, 15);

	ret = GUI_Mentat_Loop(wsaFilename, NULL, stringBuffer, true, NULL);

	if (w != NULL) {
		do {
			GUI_Widget_DrawAll(w);
			ret = GUI_Widget_HandleEvents(w);

			GUI_PaletteAnimate();
			GUI_Mentat_Animation(0);

			Video_Tick();
			sleepIdle();
		} while ((ret & 0x8000) == 0);
	}

	Input_History_Clear();

	if (unknown) {
		Load_Palette_Mercenaries();
		Sprites_LoadTiles();
	}

	return ret;
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

/**
 * Display a mentat.
 * @param houseFilename Filename of the house.
 * @param houseID ID of the house.
 */
void GUI_Mentat_Display(const char *wsaFilename, uint8 houseID)
{
	char textBuffer[16];
	uint16 oldScreenID;

	snprintf(textBuffer, sizeof(textBuffer), "MENTAT%c.CPS", g_table_houseInfo[houseID].name[0]);
	Sprites_LoadImage(textBuffer, 3, g_palette_998A);

	oldScreenID = GFX_Screen_SetActive(2);

	if (houseID == HOUSE_MERCENARY) {
		File_ReadBlockFile("BENE.PAL", g_palette1, 256 * 3);
	}

	GUI_Mentat_SetSprites(houseID);

	g_shoulderLeft = s_unknownHouseData[houseID][6];
	g_shoulderTop  = s_unknownHouseData[houseID][7];

	Widget_SetAndPaintCurrentWidget(8);

	if (wsaFilename != NULL) {
		void *wsa;

		wsa = WSA_LoadFile(wsaFilename, GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), false);
		WSA_DisplayFrame(wsa, 0, g_curWidgetXBase * 8, g_curWidgetYBase, 2);
		WSA_Unload(wsa);
	}

	GUI_DrawSprite(2, g_sprites[397 + houseID * 15], g_shoulderLeft, g_shoulderTop, 0, 0);
	GFX_Screen_SetActive(oldScreenID);
}

static bool GUI_Mentat_DrawInfo(char *text, uint16 left, uint16 top, uint16 height, uint16 skip, int16 lines, uint16 flags)
{
	uint16 oldScreenID;

	if (lines <= 0) return false;

	oldScreenID = GFX_Screen_SetActive(4);

	while (skip-- != 0) text += strlen(text) + 1;

	while (lines-- != 0) {
		if (*text != '\0') GUI_DrawText_Wrapper(text, left, top, g_curWidgetFGColourBlink, 0, flags);
		top += height;
		text += strlen(text) + 1;
	}

	GFX_Screen_SetActive(oldScreenID);

	return true;
}

uint16 GUI_Mentat_Loop(const char *wsaFilename, char *pictureDetails, char *text, bool arg12, Widget *w)
{
	uint16 oldScreenID;
	uint16 old07AE;
	void *wsa;
	uint16 descLines;
	bool dirty;
	bool done;
	bool textDone;
	uint16 frame;
	uint16 mentatSpeakingMode;
	uint16 result;
	int64_t textTick;
	uint32 textDelay;
	uint16 lines;
	uint16 textLines;
	uint16 step;

	dirty = false;
	textTick = 0;
	textDelay = 0;

	old07AE = Widget_SetCurrentWidget(8);
	oldScreenID = GFX_Screen_SetActive(4);

	wsa = NULL;

	if (wsaFilename != NULL) {
		wsa = WSA_LoadFile(wsaFilename, GFX_Screen_Get_ByIndex(3), GFX_Screen_GetSize_ByIndex(3), false);
	}

	step = 0;
	if (wsa == NULL) {
		Widget_PaintCurrentWidget();
		step = 1;
	}

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x31);

	descLines = GUI_SplitText(pictureDetails, (g_curWidgetWidth << 3) + 10, '\0');

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x32);

	textLines = GUI_Mentat_SplitText(text, 304);

	mentatSpeakingMode = 2;
	lines = 0;
	frame = 0;
	int64_t timeout = Timer_GetTicks();
	int64_t descTick = 30 + timeout;

	Input_History_Clear();

	textDone = false;
	done = false;
	result = 0;
	while (!done) {
		uint16 key;

		GFX_Screen_SetActive(0);

		key = GUI_Widget_HandleEvents(w);

		GUI_PaletteAnimate();

		if (key != 0) {
			if ((key & 0x800) == 0) {
				if (w != NULL) {
					if ((key & 0x8000) != 0 && result == 0) result = key;
				} else {
					if (textDone) result = key;
				}
			} else {
				key = 0;
			}
		}

		switch (step) {
			case 0:
				if (key == 0) break;
				step = 1;
				/* FALL-THROUGH */

			case 1:
				if (key != 0) {
					if (result != 0) {
						step = 5;
						break;
					}
					lines = descLines;
					dirty = true;
				} else {
					if (Timer_GetTicks() > descTick) {
						descTick = Timer_GetTicks() + 15;
						lines++;
						dirty = true;
					}
				}

				if (lines < descLines && lines <= 12) break;

				step = (text != NULL) ? 2 : 4;
				lines = descLines;
				break;

			case 2:
				GUI_Mouse_Hide_InRegion(0, 0, SCREEN_WIDTH, 40);
				GUI_Screen_Copy(0, 0, 0, 160, SCREEN_WIDTH / 8, 40, 0, 4);
				GUI_Mouse_Show_InRegion();

				step = 3;
				key = 1;
				/* FALL-THROUGH */

			case 3:
				if (mentatSpeakingMode == 2 && textTick < Timer_GetTicks()) key = 1;

				if ((key != 0 && textDone) || result != 0) {
					GUI_Mouse_Hide_InRegion(0, 0, SCREEN_WIDTH, 40);
					GUI_Screen_Copy(0, 160, 0, 0, SCREEN_WIDTH / 8, 40, 4, 0);
					GUI_Mouse_Show_InRegion();

					step = 4;
					mentatSpeakingMode = 0;
					break;
				}

				if (key != 0) {
					GUI_Screen_Copy(0, 160, 0, 0, SCREEN_WIDTH / 8, 40, 4, 4);

					if (textLines-- != 0) {
						GFX_Screen_SetActive(4);
						GUI_DrawText_Wrapper(text, 4, 1, g_curWidgetFGColourBlink, 0, 0x32);
						mentatSpeakingMode = 1;
						textDelay = strlen(text) * 4;
						textTick = Timer_GetTicks() + textDelay;

						if (textLines != 0) {
							while (*text++ != '\0') {}
						} else {
							textDone = true;
						}

						GFX_Screen_SetActive(0);
					}

					GUI_Mouse_Hide_InRegion(0, 0, SCREEN_WIDTH, 40);
					GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, 40, 4, 0);
					GUI_Mouse_Show_InRegion();
					break;
				}

				if (mentatSpeakingMode == 0 || textTick > Timer_GetTicks()) break;

				mentatSpeakingMode = 2;
				textTick += textDelay + textDelay / 2;
				break;

			case 4:
				if (result != 0 || w == NULL) step = 5;
				break;

			case 5:
				dirty = true;
				done = true;
				break;

			default: break;
		}

		GUI_Mentat_Animation(mentatSpeakingMode);

		if (wsa != NULL && Timer_GetTicks() >= timeout) {
			timeout = Timer_GetTicks() + 7;

			do {
				if (step == 0 && frame > 4) step = 1;

				if (!WSA_DisplayFrame(wsa, frame++, g_curWidgetXBase << 3, g_curWidgetYBase, 4)) {
					if (step == 0) step = 1;

					if (arg12 != 0) {
						frame = 0;
					} else {
						WSA_Unload(wsa);
						wsa = NULL;
					}
				}
			} while (frame == 0);
			dirty = true;
		}

		if (!dirty) {
			Video_Tick();
			sleepIdle();
			continue;
		}

		GUI_Mentat_DrawInfo(pictureDetails, (g_curWidgetXBase << 3) + 5, g_curWidgetYBase + 3, 8, 0, lines, 0x31);

		GUI_DrawSprite(4, g_sprites[397 + g_playerHouseID * 15], g_shoulderLeft, g_shoulderTop, 0, 0);
		GUI_Mouse_Hide_InWidget(g_curWidgetIndex);
		GUI_Screen_Copy(g_curWidgetXBase, g_curWidgetYBase, g_curWidgetXBase, g_curWidgetYBase, g_curWidgetWidth, g_curWidgetHeight, 4, 0);
		GUI_Mouse_Show_InWidget();
		dirty = false;

		Video_Tick();
		sleepIdle();
	}

	if (wsa != NULL) WSA_Unload(wsa);

	GFX_Screen_SetActive(4);
	GUI_DrawSprite(4, g_sprites[397 + g_playerHouseID * 15], g_shoulderLeft, g_shoulderTop, 0, 0);
	GUI_Mouse_Hide_InWidget(g_curWidgetIndex);
	GUI_Screen_Copy(g_curWidgetXBase, g_curWidgetYBase, g_curWidgetXBase, g_curWidgetYBase, g_curWidgetWidth, g_curWidgetHeight, 4, 0);
	GUI_Mouse_Show_InWidget();
	Widget_SetCurrentWidget(old07AE);
	GFX_Screen_SetActive(oldScreenID);

	Input_History_Clear();

	return result;
}

uint16 GUI_Mentat_Tick(void)
{
	GUI_Mentat_Animation((g_interrogationTimer < Timer_GetTicks()) ? 0 : 1);

	return 0;
}

/* gui/widget.c. */

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
