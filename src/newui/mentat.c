/* mentat.c */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fourcc.h"
#include "../os/endian.h"

#include "mentat.h"

#include "scrollbar.h"
#include "../config.h"
#include "../enhancement.h"
#include "../file.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../gui/mentat.h"
#include "../ini.h"
#include "../input/input.h"
#include "../opendune.h"
#include "../shape.h"
#include "../string.h"
#include "../table/strings.h"
#include "../timer/timer.h"
#include "../tools.h"
#include "../video/video.h"
#include "../wsa.h"

static const struct {
	const char *background;
	enum ShapeID eyes; int eyesX, eyesY;
	enum ShapeID mouth; int mouthX, mouthY;
	enum ShapeID shoulder; int shoulderX, shoulderY;
	enum ShapeID accessory; int accessoryX, accessoryY;
} mentat_data[MENTAT_MAX] = {
	{	"MENTATH.CPS",
		SHAPE_MENTAT_EYES + 15 * 0, 0x20,0x58,
		SHAPE_MENTAT_MOUTH + 15 * 0, 0x20,0x68,
		SHAPE_MENTAT_SHOULDER + 15 * 0, 0x80,0x68,
		SHAPE_INVALID, 0,0
	},
	{	"MENTATA.CPS",
		SHAPE_MENTAT_EYES + 15 * 1, 0x28,0x50,
		SHAPE_MENTAT_MOUTH + 15 * 1, 0x28,0x60,
		SHAPE_MENTAT_SHOULDER + 15 * 1, 0x80,0x80,
		SHAPE_MENTAT_ACCESSORY + 15 * 1, 0x48,0x98
	},
	{	"MENTATO.CPS",
		SHAPE_MENTAT_EYES + 15 * 2, 0x10,0x50,
		SHAPE_MENTAT_MOUTH + 15 * 2, 0x10,0x60,
		SHAPE_MENTAT_SHOULDER + 15 * 2, 0x80,0x80,
		SHAPE_MENTAT_ACCESSORY + 15 * 2, 0x58,0x90
	},
	{	"MENTATM.CPS",
		SHAPE_MENTAT_EYES + 15 * 5, 0x40,0x50,
		SHAPE_MENTAT_MOUTH + 15 * 5, 0x38,0x60,
		SHAPE_INVALID, 0,0,
		SHAPE_INVALID, 0,0
	},
	{	NULL,
		SHAPE_INVALID, 0,0,
		SHAPE_INVALID, 0,0,
		SHAPE_INVALID, 0,0,
		SHAPE_INVALID, 0,0
	},
};

int movingEyesSprite;
int movingMouthSprite;
int otherSprite;

MentatState g_mentat_state;

/*--------------------------------------------------------------*/

static void
GUI_Mentat_HelpListLoop(int key)
{
	if (key != 0x8001) {
		Widget *w = GUI_Widget_Get_ByIndex(g_widgetMentatTail, 15);

		switch (key) {
			case 0x80 | MOUSE_ZAXIS:
			case SCANCODE_KEYPAD_8: /* NUMPAD 8 / ARROW UP */
			case SCANCODE_KEYPAD_2: /* NUMPAD 2 / ARROW DOWN */
			case SCANCODE_KEYPAD_9: /* NUMPAD 9 / PAGE UP */
			case SCANCODE_KEYPAD_3: /* NUMPAD 3 / PAGE DOWN */
				Scrollbar_HandleEvent(w, key);
				break;

			case MOUSE_LMB:
				break;

			case 0x8003:
			case SCANCODE_ENTER:
			case SCANCODE_KEYPAD_5:
			case SCANCODE_SPACE:
				GUI_Mentat_ShowHelp();
				break;

			default: break;
		}
	}
}

void
GUI_Mentat_LoadHelpSubjects(bool init)
{
	if (!init)
		return;

	char *helpSubjects = GFX_Screen_Get_ByIndex(5);
	uint8 fileID;
	uint32 length;
	uint32 counter;

	snprintf(s_mentatFilename, sizeof(s_mentatFilename), "MENTAT%c", g_table_houseInfo[g_playerHouseID].name[0]);
	snprintf(s_mentatFilename, sizeof(s_mentatFilename), "%s", String_GenerateFilename(s_mentatFilename));

	/* Be careful here as Fremen, Sardaukar, and Mercenaries don't have mentat advice. */
	if (!File_Exists_Ex(SEARCHDIR_CAMPAIGN_DIR, s_mentatFilename)) {
		s_mentatFilename[6] = g_table_houseInfo[g_playerHouseID].prefixChar;
	}

	fileID = ChunkFile_Open_Ex(SEARCHDIR_CAMPAIGN_DIR, s_mentatFilename);
	length = ChunkFile_Read(fileID, HTOBE32(CC_NAME), helpSubjects, GFX_Screen_GetSize_ByIndex(5));
	ChunkFile_Close(fileID);

	Widget *w = GUI_Widget_Get_ByIndex(g_widgetMentatTail, 15);
	WidgetScrollbar *ws = w->data;
	ws->scrollMax = 0;

	counter = 0;
	while (counter < length) {
		const uint8 size = *helpSubjects;

		counter += size;

		if (helpSubjects[size - 1] > g_campaignID + 1) {
			helpSubjects += size;
			continue;
		}

		ScrollbarItem *si = Scrollbar_AllocItem(w);
		si->offset = HTOBE32(*(uint32 *)(helpSubjects + 1));
		si->no_desc = (helpSubjects[5] == '0');
		si->is_category = (helpSubjects[6] == '0');
		snprintf(si->text, sizeof(si->text), "%s", helpSubjects + 7);

		if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
			if (strcmp(si->text, "Frigatte") == 0)
				strcpy(si->text, "Frigate");
		}

		helpSubjects += size;
	}

	GUI_Widget_Scrollbar_Init(w, ws->scrollMax, 11, 0);
}

void
GUI_Mentat_Draw(bool force)
{
	Widget *w = g_widgetMentatTail;
	VARIABLE_NOT_USED(force);

	Widget_SetAndPaintCurrentWidget(8);

	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_SELECT_SUBJECT), g_curWidgetXBase + 16, g_curWidgetYBase + 2, 12, 0, 0x12);

	GUI_Widget_Draw(GUI_Widget_Get_ByIndex(w, 15));
	GUI_Widget_Draw(GUI_Widget_Get_ByIndex(w, 16));
	GUI_Widget_Draw(GUI_Widget_Get_ByIndex(w, 17));
}

/*--------------------------------------------------------------*/

void
Mentat_GetEyePositions(enum MentatID mentatID, int *left, int *top, int *right, int *bottom)
{
	assert(mentatID < MENTAT_MAX);

	*left = mentat_data[mentatID].eyesX;
	*top = mentat_data[mentatID].eyesY;
	*right = *left + Shape_Width(mentat_data[mentatID].eyes);
	*bottom = *top + Shape_Height(mentat_data[mentatID].eyes);
}

void
Mentat_GetMouthPositions(enum MentatID mentatID, int *left, int *top, int *right, int *bottom)
{
	assert(mentatID < MENTAT_MAX);

	*left = mentat_data[mentatID].mouthX;
	*top = mentat_data[mentatID].mouthY;
	*right = *left + Shape_Width(mentat_data[mentatID].mouth);
	*bottom = *top + Shape_Height(mentat_data[mentatID].mouth);
}

void
Mentat_DrawBackground(enum MentatID mentatID)
{
	assert(mentatID < MENTAT_MAX);

	if (mentatID == MENTAT_CUSTOM) {
		char background[16];
		snprintf(background, sizeof(background), "MENTAT%c.CPS", g_table_houseInfo[g_playerHouseID].name[0]);
		Video_DrawCPS(SEARCHDIR_CAMPAIGN_DIR, background);
	}
	else {
		Video_DrawCPS(SEARCHDIR_GLOBAL_DATA_DIR, mentat_data[mentatID].background);
	}
}

static void
Mentat_DrawEyes(enum MentatID mentatID)
{
	const enum ShapeID shapeID = mentat_data[mentatID].eyes;

	if (shapeID != SHAPE_INVALID)
		Shape_Draw(shapeID + movingEyesSprite, mentat_data[mentatID].eyesX, mentat_data[mentatID].eyesY, 0, 0);
}

static void
Mentat_DrawMouth(enum MentatID mentatID)
{
	const enum ShapeID shapeID = mentat_data[mentatID].mouth;

	if (shapeID != SHAPE_INVALID)
		Shape_Draw(shapeID + movingMouthSprite, mentat_data[mentatID].mouthX, mentat_data[mentatID].mouthY, 0, 0);
}

static void
Mentat_DrawShoulder(enum MentatID mentatID)
{
	const enum ShapeID shapeID = mentat_data[mentatID].shoulder;

	if (shapeID != SHAPE_INVALID)
		Shape_Draw(shapeID, mentat_data[mentatID].shoulderX, mentat_data[mentatID].shoulderY, 0, 0);
}

static void
Mentat_DrawAccessory(enum MentatID mentatID)
{
	const enum ShapeID shapeID = mentat_data[mentatID].accessory;

	if (shapeID != SHAPE_INVALID)
		Shape_Draw(shapeID + abs(otherSprite), mentat_data[mentatID].accessoryX, mentat_data[mentatID].accessoryY, 0, 0);
}

void
Mentat_Draw(enum MentatID mentatID)
{
	assert(mentatID < MENTAT_MAX);

	Mentat_DrawEyes(mentatID);
	Mentat_DrawMouth(mentatID);
	Mentat_DrawShoulder(mentatID);
	Mentat_DrawAccessory(mentatID);
}

/*--------------------------------------------------------------*/

static void
MentatBriefing_SplitDesc(MentatState *mentat)
{
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x31);

	mentat->desc_lines = GUI_SplitText(mentat->desc, 184 + 10, '\0');
	mentat->desc_timer = Timer_GetTicks() + 15;
}

void
MentatBriefing_SplitText(MentatState *mentat)
{
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x32);

	mentat->lines0 = GUI_Mentat_SplitText(mentat->text, 304);
	mentat->lines = mentat->lines0;

	mentat->speaking_mode = 1;
	mentat->speaking_timer = Timer_GetTicks() + 4 * strlen(mentat->text);
}

void
MentatBriefing_InitText(enum HouseType houseID, int campaignID, enum BriefingEntry entry,
		MentatState *mentat)
{
	const int stringID = ((campaignID + 1) * 4) + entry;
	assert(entry <= MENTAT_BRIEFING_ADVICE);

	strncpy(mentat->buf, String_GetMentatString(houseID, stringID), sizeof(mentat->buf));
	mentat->desc = NULL;
	mentat->text = mentat->buf;
	MentatBriefing_SplitText(mentat);
}

static void
MentatBriefing_DrawDescription(const MentatState *mentat)
{
	if (mentat->desc) {
		const WidgetProperties *wi = &g_widgetProperties[WINDOWID_MENTAT_PICTURE];

		GUI_DrawText_Wrapper(mentat->desc, wi->xBase + 5, wi->yBase + 3, g_curWidgetFGColourBlink, 0, 0x31);
	}
}

void
MentatBriefing_DrawText(const MentatState *mentat)
{
	if (mentat->lines > 0)
		GUI_DrawText_Wrapper(mentat->text, 4, 1, g_curWidgetFGColourBlink, 0, 0x32);
}

static void
MentatBriefing_AdvanceDesc(MentatState *mentat)
{
	if (mentat->desc_lines > 1) {
		mentat->desc_timer = Timer_GetTicks() + 15;
		mentat->desc_lines--;

		char *c = mentat->desc;
		while (*c != '\0') {
			c++;
		}

		*c = '\n';
	}

	if (mentat->desc_lines <= 1) {
		mentat->state = MENTAT_SHOW_TEXT;
		MentatBriefing_SplitText(mentat);
	}
}

void
MentatBriefing_AdvanceText(MentatState *mentat)
{
	while (mentat->text[0] != '\0') {
		mentat->text++;
	}

	mentat->text++;
	mentat->lines--;

	if (mentat->lines <= 0) {
		mentat->state = MENTAT_IDLE;
		mentat->speaking_mode = 0;
	}
	else {
		mentat->speaking_mode = 1;
		mentat->speaking_timer = Timer_GetTicks() + 4 * strlen(mentat->text);
	}
}

/*--------------------------------------------------------------*/

void
MentatBriefing_InitWSA(enum HouseType houseID, int scenarioID, enum BriefingEntry entry, MentatState *mentat)
{
	const char *key[3] = { "BriefPicture", "WinPicture", "LosePicture" };
	const char *def[3] = { "HARVEST.WSA", "WIN1.WSA", "LOSTBILD.WSA" };
	assert(entry <= MENTAT_BRIEFING_ADVICE);

	if (scenarioID <= 0) {
		const char *wsaFilename = House_GetWSAHouseFilename(houseID);

		/* Be careful here because Fremen, Sardaukar, and Mercenaries
		 * don't have house WSAs in Dune II.
		 */
		if (!File_Exists_Ex(SEARCHDIR_CAMPAIGN_DIR, wsaFilename)) {
			wsaFilename = House_GetWSAHouseFilename(g_table_houseRemap6to3[houseID]);
		}

		mentat->wsa = WSA_LoadFile(wsaFilename, GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), false);
	}
	else {
		char filename[16];
		snprintf(filename, sizeof(filename), "SCEN%c%03d.INI", g_table_houseInfo[houseID].name[0], scenarioID);

		char *buf = File_ReadWholeFile_Ex(SEARCHDIR_CAMPAIGN_DIR, filename);
		if (buf == NULL) {
			mentat->wsa = NULL;
			return;
		}

		char wsaFilename[16];
		Ini_GetString("BASIC", key[entry], def[entry], wsaFilename, sizeof(wsaFilename), buf);
		free(buf);

		mentat->wsa = WSA_LoadFile(wsaFilename, GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), false);
	}

	mentat->wsa_timer = Timer_GetTicks();
	mentat->wsa_frame = 0;
}

void
MentatBriefing_DrawWSA(MentatState *mentat)
{
	if (mentat->wsa == NULL)
		return;

	const int64_t curr_ticks = Timer_GetTicks();
	const int frame = mentat->wsa_frame + (curr_ticks - mentat->wsa_timer) / 7;

	if (!Video_DrawWSA(mentat->wsa, frame, 0, 0, 128, 48, 184, 112)) {
		Video_DrawWSA(mentat->wsa, 0, 0, 0, 128, 48, 184, 112);
		mentat->wsa_timer = curr_ticks;
		mentat->wsa_frame = 0;
	}
}

/*--------------------------------------------------------------*/

static void
MentatSecurity_PickQuestion(MentatState *mentat)
{
	const int questionsCount = atoi(String_Get_ByIndex(STR_SECURITY_COUNT));

	mentat->security_question = Tools_RandomLCG_Range(0, questionsCount - 1) * 3 + STR_SECURITY_QUESTIONS;
	mentat->wsa = WSA_LoadFile(String_Get_ByIndex(mentat->security_question + 1), GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), false);
}

void
MentatSecurity_Initialise(enum HouseType houseID, MentatState *mentat)
{
	houseID = g_table_houseRemap6to3[houseID];

	g_disableOtherMovement = true;
	g_interrogation = true;
	mentat->security_lives = 3;

	strncpy(mentat->buf, String_Get_ByIndex(STR_SECURITY_TEXT_HARKONNEN + houseID * 3), sizeof(mentat->buf));
	mentat->text = mentat->buf;
	MentatBriefing_SplitText(mentat);

	MentatSecurity_PickQuestion(mentat);
}

void
MentatSecurity_PrepareQuestion(bool pick_new_question, MentatState *mentat)
{
	if (pick_new_question)
		MentatSecurity_PickQuestion(mentat);

	strncpy(mentat->buf, String_Get_ByIndex(mentat->security_question), sizeof(mentat->buf));
	mentat->text = mentat->buf;
	MentatBriefing_SplitText(mentat);

	/* If we accept any answer, then fill in the real answer so people
	 * won't go looking up the manual.
	 */
	if (enhancement_security_question == SECURITY_QUESTION_ACCEPT_ALL) {
		snprintf(mentat->security_prompt, sizeof(mentat->security_prompt), "%s", String_Get_ByIndex(mentat->security_question + 2));
	}
	else {
		mentat->security_prompt[0] = '\0';
	}
}

void
MentatSecurity_Draw(MentatState *mentat)
{
	const WidgetProperties *wi = &g_widgetProperties[WINDOWID_MENTAT_EDIT_BOX];
	const uint16 old_widget = Widget_SetCurrentWidget(WINDOWID_MENTAT_EDIT_BOX);

	MentatBriefing_DrawText(mentat);

	Prim_DrawBorder(wi->xBase - 6, wi->yBase - 6, wi->width + 12, wi->height + 12, 1, false, true, 1);
	Prim_DrawBorder(wi->xBase - 2, wi->yBase - 2, wi->width + 4, wi->height + 4, 1, false, false, 2);

	GUI_EditBox_Draw(mentat->security_prompt);
	Widget_SetCurrentWidget(old_widget);
}

bool
MentatSecurity_CorrectLoop(MentatState *mentat, int64_t blink_start)
{
	const int64_t curr_ticks = Timer_GetTicks();
	bool end = (curr_ticks - blink_start >= 120);

	if (curr_ticks >= mentat->speaking_timer)
		mentat->speaking_mode = 0;

	GUI_Mentat_Animation(g_table_houseInfo[g_playerHouseID].mentat, mentat->speaking_mode);

	if (Input_IsInputAvailable()) {
		if (!(Input_GetNextKey() & SCANCODE_RELEASE))
			end = true;
	}

	if (end) {
		g_disableOtherMovement = false;
		g_interrogation = false;
		return true;
	}

	return false;
}

/*--------------------------------------------------------------*/

void
MentatHelp_Draw(enum MentatID mentatID, MentatState *mentat)
{
	Mentat_DrawBackground(mentatID);
	MentatBriefing_DrawWSA(mentat);

	if (mentat->state == MENTAT_SHOW_CONTENTS) {
		GUI_Mentat_Draw(true);
	}
	else {
		if (mentat->state >= MENTAT_SHOW_DESCRIPTION)
			MentatBriefing_DrawDescription(mentat);

		if (mentat->state >= MENTAT_SHOW_TEXT)
			MentatBriefing_DrawText(mentat);
	}

	if (mentat->state == MENTAT_SHOW_CONTENTS || mentat->state == MENTAT_IDLE) {
		GUI_Widget_Draw(g_widgetMentatFirst);
	}

	Mentat_Draw(mentatID);
}

void
MentatHelp_TickPauseDescription(MentatState *mentat)
{
	if (Timer_GetTicks() >= mentat->desc_timer) {
		MentatBriefing_SplitDesc(mentat);

		if (mentat->desc_lines == 1) {
			mentat->state = MENTAT_SHOW_TEXT;
			MentatBriefing_SplitText(mentat);
		}
		else {
			mentat->state = MENTAT_SHOW_DESCRIPTION;
			mentat->desc_timer = Timer_GetTicks() + 15;
		}
	}
}

void
MentatHelp_TickShowDescription(MentatState *mentat)
{
	if (Timer_GetTicks() >= mentat->desc_timer)
		MentatBriefing_AdvanceDesc(mentat);
}

bool
MentatHelp_Tick(MentatState *mentat)
{
	if (Timer_GetTicks() >= mentat->speaking_timer) {
		mentat->speaking_mode = 0;
	}

	GUI_Mentat_Animation(g_table_houseInfo[g_playerHouseID].mentat, mentat->speaking_mode);

	if (mentat->state == MENTAT_SHOW_CONTENTS) {
		const int widgetID = GUI_Widget_HandleEvents(g_widgetMentatTail);

		if (widgetID == 0x8001) {
			return true;
		}
		else {
			GUI_Mentat_HelpListLoop(widgetID);

			if (mentat->state == MENTAT_PAUSE_DESCRIPTION)
				mentat->desc_timer = Timer_GetTicks() + 30;
		}
	}
	else if (mentat->state == MENTAT_PAUSE_DESCRIPTION) {
		MentatHelp_TickPauseDescription(mentat);
	}
	else if (mentat->state == MENTAT_SHOW_DESCRIPTION) {
		MentatHelp_TickShowDescription(mentat);
	}
	else if (mentat->state == MENTAT_SHOW_TEXT) {
		if (Input_IsInputAvailable()) {
			const int key = Input_GetNextKey();

			if (key == SCANCODE_ESCAPE || key == SCANCODE_SPACE || key == MOUSE_LMB || key == MOUSE_RMB)
				MentatBriefing_AdvanceText(mentat);
		}
	}
	else {
		const int widgetID = GUI_Widget_HandleEvents(g_widgetMentatFirst);

		if (widgetID == 0x8001) {
			mentat->state = MENTAT_SHOW_CONTENTS;
			mentat->wsa = NULL;
			GUI_Mentat_LoadHelpSubjects(false);
		}
	}

	return false;
}
