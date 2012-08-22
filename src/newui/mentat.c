/* mentat.c */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mentat.h"

#include "../file.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../gui/mentat.h"
#include "../ini.h"
#include "../input/input.h"
#include "../shape.h"
#include "../string.h"
#include "../table/strings.h"
#include "../timer/timer.h"
#include "../tools.h"
#include "../video/video.h"
#include "../wsa.h"

static const struct {
	int eyesX, eyesY;
	int mouthX, mouthY;
	int shoulderX, shoulderY;
	int accessoryX, accessoryY;
} mentat_data[HOUSE_MAX] = {
	{ 0x20,0x58, 0x20,0x68, 0x80,0x68, 0x00,0x00 },
	{ 0x28,0x50, 0x28,0x60, 0x80,0x80, 0x48,0x98 },
	{ 0x10,0x50, 0x10,0x60, 0x80,0x80, 0x58,0x90 },
	{ 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00 },
	{ 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00 },
	{ 0x40,0x50, 0x38,0x60, 0x00,0x00, 0x00,0x00 },
};

int movingEyesSprite;
int movingMouthSprite;
int otherSprite;

MentatState g_mentat_state;

void
Mentat_GetEyePositions(enum HouseType houseID, int *left, int *top, int *right, int *bottom)
{
	const enum ShapeID shapeID = SHAPE_MENTAT_EYES + houseID * 15;
	assert(houseID < HOUSE_MAX);

	*left = mentat_data[houseID].eyesX;
	*top = mentat_data[houseID].eyesY;
	*right = *left + Shape_Width(shapeID);
	*bottom = *top + Shape_Height(shapeID);
}

void
Mentat_GetMouthPositions(enum HouseType houseID, int *left, int *top, int *right, int *bottom)
{
	const enum ShapeID shapeID = SHAPE_MENTAT_MOUTH + houseID * 15;
	assert(houseID < HOUSE_MAX);

	*left = mentat_data[houseID].mouthX;
	*top = mentat_data[houseID].mouthY;
	*right = *left + Shape_Width(shapeID);
	*bottom = *top + Shape_Height(shapeID);
}

void
Mentat_DrawBackground(enum HouseType houseID)
{
	const char *mentat_background[HOUSE_MAX] = {
		"MENTATH.CPS", "MENTATA.CPS", "MENTATO.CPS",
		"MENTATM.CPS", "MENTATM.CPS", "MENTATM.CPS"
	};
	assert(houseID < HOUSE_MAX);

	Video_DrawCPS(mentat_background[houseID]);
}

static void
Mentat_DrawEyes(enum HouseType houseID)
{
	const enum ShapeID shapeID = SHAPE_MENTAT_EYES + houseID * 15 + movingEyesSprite;

	Shape_Draw(shapeID, mentat_data[houseID].eyesX, mentat_data[houseID].eyesY, 0, 0);
}

static void
Mentat_DrawMouth(enum HouseType houseID)
{
	const enum ShapeID shapeID = SHAPE_MENTAT_MOUTH + houseID * 15 + movingMouthSprite;

	Shape_Draw(shapeID, mentat_data[houseID].mouthX, mentat_data[houseID].mouthY, 0, 0);
}

static void
Mentat_DrawShoulder(enum HouseType houseID)
{
	if (houseID <= HOUSE_ORDOS) {
		const enum ShapeID shapeID = SHAPE_MENTAT_SHOULDER + houseID * 15;

		Shape_Draw(shapeID, mentat_data[houseID].shoulderX, mentat_data[houseID].shoulderY, 0, 0);
	}
}

static void
Mentat_DrawAccessory(enum HouseType houseID)
{
	if (houseID == HOUSE_ATREIDES || houseID == HOUSE_ORDOS) {
		const enum ShapeID shapeID = SHAPE_MENTAT_ACCESSORY + houseID * 15 + abs(otherSprite);

		Shape_Draw(shapeID, mentat_data[houseID].accessoryX, mentat_data[houseID].accessoryY, 0, 0);
	}
}

void
Mentat_Draw(enum HouseType houseID)
{
	assert(houseID < HOUSE_MAX);

	Mentat_DrawEyes(houseID);
	Mentat_DrawMouth(houseID);
	Mentat_DrawShoulder(houseID);
	Mentat_DrawAccessory(houseID);
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
	const int stringID
		= STR_HOUSE_HARKONNENFROM_THE_DARK_WORLD_OF_GIEDI_PRIME_THE_SAVAGE_HOUSE_HARKONNEN_HAS_SPREAD_ACROSS_THE_UNIVERSE_A_CRUEL_PEOPLE_THE_HARKONNEN_ARE_RUTHLESS_TOWARDS_BOTH_FRIEND_AND_FOE_IN_THEIR_FANATICAL_PURSUIT_OF_POWER
		+ (houseID * 40) + ((campaignID + 1) * 4) + entry;
	assert(entry <= MENTAT_BRIEFING_ADVICE);

	strncpy(mentat->buf, String_Get_ByIndex(stringID), sizeof(mentat->buf));
	mentat->desc = NULL;
	mentat->text = mentat->buf;
	MentatBriefing_SplitText(mentat);
}

static void
MentatBriefing_DrawDescription(const MentatState *mentat)
{
	if (mentat->desc) {
		const WidgetProperties *wi = &g_widgetProperties[WINDOWID_MENTAT_PICTURE];

		GUI_DrawText_Wrapper(mentat->desc, wi->xBase*8 + 5, wi->yBase + 3, g_curWidgetFGColourBlink, 0, 0x31);
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

		mentat->wsa = WSA_LoadFile(wsaFilename, GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), false);
	}
	else {
		char filename[16];
		snprintf(filename, sizeof(filename), "SCEN%c%03d.INI", g_table_houseInfo[houseID].name[0], scenarioID);

		char *buf = File_ReadWholeFile(filename);
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

	mentat->security_question = Tools_RandomRange(0, questionsCount - 1) * 3 + STR_SECURITY_QUESTIONS;
	mentat->wsa = WSA_LoadFile(String_Get_ByIndex(mentat->security_question + 1), GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), false);

#if 0
	printf("Correct answer is: %s.\n", String_Get_ByIndex(mentat->security_question + 2));
#endif
}

void
MentatSecurity_Initialise(enum HouseType houseID, MentatState *mentat)
{
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

	mentat->security_prompt[0] = '\0';
}

void
MentatSecurity_Draw(MentatState *mentat)
{
	const WidgetProperties *wi = &g_widgetProperties[WINDOWID_MENTAT_EDIT_BOX];
	const uint16 old_widget = Widget_SetCurrentWidget(WINDOWID_MENTAT_EDIT_BOX);

	MentatBriefing_DrawText(mentat);

	GUI_DrawBorder(wi->xBase*8 - 6, wi->yBase - 6, wi->width*8 + 12, wi->height + 12, 1, true);
	GUI_DrawBorder(wi->xBase*8 - 2, wi->yBase - 2, wi->width*8 + 4, wi->height + 4, 2, false);

	GUI_EditBox(mentat->security_prompt, sizeof(mentat->security_prompt) - 1, 9, NULL, NULL, 0);
	Widget_SetCurrentWidget(old_widget);
}

bool
MentatSecurity_CorrectLoop(MentatState *mentat, int64_t blink_start)
{
	const int64_t curr_ticks = Timer_GetTicks();

	if (curr_ticks >= mentat->speaking_timer)
		mentat->speaking_mode = 0;

	GUI_Mentat_Animation(mentat->speaking_mode);

	if (curr_ticks - blink_start >= 120) {
		g_disableOtherMovement = false;
		g_interrogation = false;
		return true;
	}

	return false;
}

/*--------------------------------------------------------------*/

static void
MentatHelp_Draw(enum HouseType houseID, MentatState *mentat)
{
	Mentat_DrawBackground(houseID);
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

	Mentat_Draw(houseID);
}

bool
MentatHelp_Tick(enum HouseType houseID, MentatState *mentat)
{
	MentatHelp_Draw(houseID, mentat);

	if (Timer_GetTicks() >= mentat->speaking_timer) {
		mentat->speaking_mode = 0;
	}

	GUI_Mentat_Animation(mentat->speaking_mode);

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
	else if (mentat->state == MENTAT_SHOW_DESCRIPTION) {
		if (Timer_GetTicks() >= mentat->desc_timer)
			MentatBriefing_AdvanceDesc(mentat);
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
