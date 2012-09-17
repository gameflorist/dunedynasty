/* $Id$ */

/** @file src/gui/mentat.c Mentat gui routines. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fourcc.h"
#include "types.h"
#include "../os/endian.h"
#include "../os/sleep.h"
#include "../os/strings.h"

#include "mentat.h"

#include "font.h"
#include "gui.h"
#include "widget.h"
#include "../audio/driver.h"
#include "../audio/sound.h"
#include "../config.h"
#include "../enhancement.h"
#include "../file.h"
#include "../gfx.h"
#include "../house.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../load.h"
#include "../newui/mentat.h"
#include "../opendune.h"
#include "../scenario.h"
#include "../shape.h"
#include "../sprites.h"
#include "../string.h"
#include "../table/strings.h"
#include "../timer/timer.h"
#include "../tools.h"
#include "../video/video.h"
#include "../wsa.h"

/**
 * Information about the mentat.
 */
static const uint8 s_unknownHouseData[6][8] = {
	{0x20,0x58,0x20,0x68,0x00,0x00,0x80,0x68}, /* Harkonnen mentat. */
	{0x28,0x50,0x28,0x60,0x48,0x98,0x80,0x80}, /* Atreides mentat. */
	{0x10,0x50,0x10,0x60,0x58,0x90,0x80,0x80}, /* Ordos mentat. */
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	{0x40,0x50,0x38,0x60,0x00,0x00,0x00,0x00}, /* Intro houses (mercenaries) mentat. */
};


static uint8 *s_mentatSprites[3][5];

bool g_interrogation;      /*!< Asking a security question (changes mentat eye movement). */
bool g_disableOtherMovement; /*!< Disable moving of the other object. */

static bool s_selectMentatHelp = false; /*!< Selecting from the list of in-game help subjects. */
static uint8 *s_helpSubjects = NULL;

static char s_mentatFilename[13];
static uint16 s_topHelpList;
static uint16 s_selectedHelpSubject;
static uint16 s_numberHelpSubjects;

#if 0
/* Moved to gui/menu_opendune.c. */
static void GUI_Mentat_ShowDialog(uint8 houseID, uint16 stringID, const char *wsaFilename, uint16 musicID);
#endif

void GUI_Mentat_HelpListLoop(int key)
{
	if (key != 0x8001) {
		Widget *w = g_widgetMentatTail;

		s_selectMentatHelp = true;

		if ((key & 0x7F) == MOUSE_ZAXIS) {
			if (g_mouseDZ < 0) {
				key = SCANCODE_KEYPAD_2;
			}
			else if (g_mouseDZ > 0) {
				key = SCANCODE_KEYPAD_8;
			}
		}

		switch (key) {
			case SCANCODE_KEYPAD_8: /* NUMPAD 8 / ARROW UP */
				if (s_selectedHelpSubject != 0) {
					GUI_Mentat_List_Click(GUI_Widget_Get_ByIndex(w, s_selectedHelpSubject + 2));
					break;
				}

				GUI_Widget_Scrollbar_ArrowUp_Click(g_widgetMentatScrollbar);
				break;

			case SCANCODE_KEYPAD_2: /* NUMPAD 2 / ARROW DOWN */
				if (s_selectedHelpSubject < 10) {
					GUI_Mentat_List_Click(GUI_Widget_Get_ByIndex(w, s_selectedHelpSubject + 4));
					break;
				}

				GUI_Widget_Scrollbar_ArrowDown_Click(g_widgetMentatScrollbar);
				break;

			case SCANCODE_PGUP: /* NUMPAD 9 / PAGE UP */
				for (int i = 0; i < 11; i++)
					GUI_Widget_Scrollbar_ArrowUp_Click(g_widgetMentatScrollbar);
				break;

			case SCANCODE_PGDN: /* NUMPAD 3 / PAGE DOWN */
				for (int i = 0; i < 11; i++)
					GUI_Widget_Scrollbar_ArrowDown_Click(g_widgetMentatScrollbar);
				break;

			case MOUSE_LMB:
				break;

			case SCANCODE_ENTER:
			case SCANCODE_KEYPAD_5:
			case SCANCODE_SPACE:
				GUI_Mentat_List_Click(GUI_Widget_Get_ByIndex(w, s_selectedHelpSubject + 3));
				break;

			default: break;
		}

		s_selectMentatHelp = false;
	}
}

void GUI_Mentat_LoadHelpSubjects(bool init)
{
	static uint8 *helpDataList = NULL;

	uint8 fileID;
	uint32 length;
	uint32 counter;
	uint8 *helpSubjects;
	uint16 i;

	if (init) {
		helpDataList = GFX_Screen_Get_ByIndex(5);

		s_topHelpList = 0;
		s_selectedHelpSubject = 0;

		sprintf(s_mentatFilename, "MENTAT%c", g_table_houseInfo[g_playerHouseID].name[0]);
		strcpy(s_mentatFilename, String_GenerateFilename(s_mentatFilename));
	}

	fileID = ChunkFile_Open(s_mentatFilename);
	length = ChunkFile_Read(fileID, HTOBE32(CC_NAME), helpDataList, GFX_Screen_GetSize_ByIndex(5));
	ChunkFile_Close(fileID);

	s_numberHelpSubjects = 0;
	helpSubjects = helpDataList;

	counter = 0;
	while (counter < length) {
		uint8 size = *helpSubjects;

		counter += size;

		if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
			if (strcmp((char *)helpSubjects + 7, "Frigatte") == 0)
				strcpy((char *)helpSubjects + 7, "Frigate");
		}

		if (helpSubjects[size - 1] > g_campaignID + 1) {
			while (size-- != 0) *helpSubjects++ = '\0';
			continue;
		}

		helpSubjects[size - 1] = size;
		helpSubjects += size;
		s_numberHelpSubjects++;
	}

	helpSubjects = helpDataList;

	while (*helpSubjects == '\0') helpSubjects++;

	for (i = 0; i < s_topHelpList; i++) helpSubjects = String_NextString(helpSubjects);

	s_helpSubjects = helpSubjects;
}

void GUI_Mentat_Draw(bool force)
{
	Widget *line;
	Widget *w = g_widgetMentatTail;
	uint8 *helpSubjects = s_helpSubjects;
	uint16 i;
	VARIABLE_NOT_USED(force);

	Widget_SetAndPaintCurrentWidget(8);

	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_SELECT_SUBJECT), g_curWidgetXBase + 16, g_curWidgetYBase + 2, 12, 0, 0x12);
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x11);

	line = GUI_Widget_Get_ByIndex(w, 3);
	for (i = 0; i < 11; i++) {
		line->drawParameterDown.text     = (char *)helpSubjects + 7;
		line->drawParameterSelected.text = (char *)helpSubjects + 7;
		line->drawParameterNormal.text   = (char *)helpSubjects + 7;

		if (helpSubjects[6] == '0') {
			line->offsetX          = 16;
			line->fgColourSelected = 11;
			line->fgColourDown     = 11;
			line->fgColourNormal   = 11;
			line->stringID         = 0x30;
		} else {
			uint8 colour = (i == s_selectedHelpSubject) ? 8 : 15;
			line->offsetX          = 24;
			line->fgColourSelected = colour;
			line->fgColourDown     = colour;
			line->fgColourNormal   = colour;
			line->stringID         = 0x31;
		}

		GUI_Widget_MakeNormal(line, false);
		GUI_Widget_Draw(line);

		line = GUI_Widget_GetNext(line);
		helpSubjects = String_NextString(helpSubjects);
	}

	GUI_Widget_Scrollbar_Init(GUI_Widget_Get_ByIndex(w, 15), s_numberHelpSubjects, 11, s_topHelpList);

	GUI_Widget_Draw(GUI_Widget_Get_ByIndex(w, 16));
	GUI_Widget_Draw(GUI_Widget_Get_ByIndex(w, 17));
}

#if 0
/* Moved to gui/menu_opendune.c */
static void GUI_Mentat_ShowHelpList(bool proceed);
extern bool GUI_Widget_Mentat_Click(Widget *w);
extern uint16 GUI_Mentat_Show(char *stringBuffer, const char *wsaFilename, Widget *w, bool unknown);
extern void GUI_Mentat_ShowBriefing(void);
extern void GUI_Mentat_ShowWin(void);
extern void GUI_Mentat_ShowLose(void);
#endif

static void
GUI_Mentat_SetSprites(enum HouseType houseID)
{
	memset(s_mentatSprites, 0, sizeof(s_mentatSprites));

	for (int i = 0; i < 5; i++) {
		s_mentatSprites[0][i] = g_sprites[387 + houseID * 15 + i];
	}

	for (int i = 0; i < 5; i++) {
		s_mentatSprites[1][i] = g_sprites[392 + houseID * 15 + i];
	}

	for (int i = 0; i < 4; i++) {
		s_mentatSprites[2][i] = g_sprites[398 + houseID * 15 + i];
	}
}

#if 0
/* Moved to gui/menu_opendune.c. */
extern void GUI_Mentat_Display(const char *wsaFilename, uint8 houseID);
#endif

/**
 * Draw sprites and handle mouse in a mentat screen.
 * @param speakingMode If \c 1, the mentat is speaking.
 */
void GUI_Mentat_Animation(uint16 speakingMode)
{
	static int64_t movingEyesTimer = 0;      /* Timer when to change the eyes sprite. */
	static uint16 movingEyesNextSprite = 0; /* If not 0, it decides the movingEyesNextSprite */
	static int64_t movingMouthTimer = 0;
	static int64_t movingOtherTimer = 0;

	int s_eyesLeft, s_eyesTop, s_eyesRight, s_eyesBottom;
	int s_mouthLeft, s_mouthTop, s_mouthRight, s_mouthBottom;

	GUI_Mentat_SetSprites(g_playerHouseID);
	Mentat_GetEyePositions(g_playerHouseID, &s_eyesLeft, &s_eyesTop, &s_eyesRight, &s_eyesBottom);
	Mentat_GetMouthPositions(g_playerHouseID, &s_mouthLeft, &s_mouthTop, &s_mouthRight, &s_mouthBottom);

	uint16 i;

	if (movingOtherTimer < Timer_GetTicks() && !g_disableOtherMovement) {
		if (movingOtherTimer != 0) {
			if (s_mentatSprites[2][1 + abs(otherSprite)] == NULL) {
				otherSprite = 1 - otherSprite;
			} else {
				otherSprite++;
			}
		}

		switch (g_playerHouseID) {
			case HOUSE_HARKONNEN:
				movingOtherTimer = Timer_GetTicks() + 300 * 60;
				break;
			case HOUSE_ATREIDES:
				movingOtherTimer = Timer_GetTicks() + 60 * Tools_RandomRange(1,3);
				break;
			case HOUSE_ORDOS:
				if (otherSprite != 0) {
					movingOtherTimer = Timer_GetTicks() + 6;
				} else {
					movingOtherTimer = Timer_GetTicks() + 60 * Tools_RandomRange(10, 19);
				}
				break;
			default:
				break;
		}
	}

	if (speakingMode == 1) {
		if (movingMouthTimer < Timer_GetTicks()) {
			uint8 *sprite;

			movingMouthSprite = Tools_RandomRange(0, 4);
			sprite = s_mentatSprites[1][movingMouthSprite];

			GUI_Mouse_Hide_InRegion(s_mouthLeft, s_mouthTop, s_mouthLeft + Sprite_GetWidth(sprite), s_mouthTop + Sprite_GetHeight(sprite));
			GUI_DrawSprite(0, sprite, s_mouthLeft, s_mouthTop, 0, 0);
			GUI_Mouse_Show_InRegion();

			switch (movingMouthSprite) {
				case 0:
					movingMouthTimer = Timer_GetTicks() + Tools_RandomRange(7, 30);
					break;
				case 1:
				case 2:
				case 3:
					movingMouthTimer = Timer_GetTicks() + Tools_RandomRange(6, 10);
					break;
				case 4:
					movingMouthTimer = Timer_GetTicks() + Tools_RandomRange(5, 6);
					break;
				default:
					break;
			}
		}
	} else {
		if (Input_Test(MOUSE_LMB) == 0 && Input_Test(MOUSE_RMB) == 0) {
			if (movingMouthSprite != 0) {
				movingMouthSprite = 0;
				movingMouthTimer = 0;
			}
		} else if (Mouse_InRegion(s_mouthLeft, s_mouthTop, s_mouthRight, s_mouthBottom) != 0) {
			if (movingMouthTimer != 0xFFFFFFFF) {
				movingMouthTimer = 0xFFFFFFFF;
				movingMouthSprite = Tools_RandomRange(1, 4);
			}
		} else {
			if (movingMouthSprite != 0) {
				movingMouthSprite = 0;
				movingMouthTimer = 0;
			}
		}
	}

	if (Input_Test(MOUSE_LMB) != 0 || Input_Test(MOUSE_RMB) != 0) {
		if (Mouse_InRegion(s_eyesLeft, s_eyesTop, s_eyesRight, s_eyesBottom) != 0) {
			if (movingEyesSprite != 0x4) {
				movingEyesSprite = (movingEyesSprite == 3) ? 4 : 3;
				movingEyesNextSprite = 0;
				movingEyesTimer = 0;
			}

			return;
		}
	}

	if (Mouse_InRegion((int)s_eyesLeft - 16, (int)s_eyesTop - 8, s_eyesRight + 16, s_eyesBottom + 24) != 0) {
		if (Mouse_InRegion((int)s_eyesLeft - 8, s_eyesBottom, s_eyesRight + 8, SCREEN_HEIGHT - 1) != 0) {
			i = 3;
		} else {
			if (Mouse_InRegion(s_eyesRight, (int)s_eyesTop - 8, s_eyesRight + 16, s_eyesBottom + 8) != 0) {
				i = 2;
			} else {
				i = (Mouse_InRegion((int)s_eyesLeft - 16, (int)s_eyesTop - 8, s_eyesLeft, s_eyesBottom + 8) == 0) ? 0 : 1;
			}
		}

		if (i != movingEyesSprite) {
			movingEyesSprite = i;
			movingEyesNextSprite = 0;
			movingEyesTimer = Timer_GetTicks();
		}
	} else {
		if (movingEyesTimer >= Timer_GetTicks()) return;

		if (movingEyesNextSprite != 0) {
			movingEyesSprite = movingEyesNextSprite;
			movingEyesNextSprite = 0;

			if (movingEyesSprite != 4) {
				movingEyesTimer = Timer_GetTicks() + Tools_RandomRange(20, 180);
			} else {
				movingEyesTimer = Timer_GetTicks() + Tools_RandomRange(12, 30);
			}
		} else {
			i = 0;
			switch (speakingMode) {
				case 0:
					i = Tools_RandomRange(0, 7);
					if (i > 5) {
						i = 1;
					} else {
						if (i == 5) {
							i = 4;
						}
					}
					break;

				case 1:
					if (movingEyesSprite != ((!g_interrogation) ? 0 : 3)) {
						i = 0;
					} else {
						i = Tools_RandomRange(0, 17);
						if (i > 9) {
							i = 0;
						} else {
							if (i >= 5) {
								i = 4;
							}
						}
					}
					break;

				default:
					i = Tools_RandomRange(0, 15);
					if (i > 10) {
						i = 2;
					} else {
						if (i >= 5) {
							i = 4;
						}
					}
					break;
			}

			if ((i == 2 && movingEyesSprite == 1) || (i == 1 && movingEyesSprite == 2)) {
				movingEyesNextSprite = i;
				movingEyesSprite = 0;
				movingEyesTimer = Timer_GetTicks() + Tools_RandomRange(1, 5);
			} else {
				if (i != movingEyesSprite && (i == 4 || movingEyesSprite == 4)) {
					movingEyesNextSprite = i;
					movingEyesSprite = 3;
					movingEyesTimer = Timer_GetTicks();
				} else {
					movingEyesSprite = i;
					if (i != 4) {
						movingEyesTimer = Timer_GetTicks() + Tools_RandomRange(15, 180);
					} else {
						movingEyesTimer = Timer_GetTicks() + Tools_RandomRange(6, 60);
					}
				}
			}

			if (g_interrogation && movingEyesSprite == 0) movingEyesSprite = 3;
		}
	}
}

/**
 * Select a new subject, move the list of help subjects displayed, if necessary.
 * @param difference Number of subjects to jump.
 */
void GUI_Mentat_SelectHelpSubject(int16 difference)
{
	if (difference > 0) {
		if (difference + s_topHelpList + 11 > s_numberHelpSubjects) {
			difference = s_numberHelpSubjects - (s_topHelpList + 11);
		}
		s_topHelpList += difference;

		while (difference-- != 0) {
			s_helpSubjects = String_NextString(s_helpSubjects);
		}
		return;
	}

	if (difference < 0) {
		difference = -difference;

		if ((int16)s_topHelpList < difference) {
			difference = s_topHelpList;
		}

		s_topHelpList -= difference;

		while (difference-- != 0) {
			s_helpSubjects = String_PrevString(s_helpSubjects);
		}
		return;
	}
}

/** Create the widgets of the mentat help screen. */
void GUI_Mentat_Create_HelpScreen_Widgets(void)
{
	static char empty[2] = "";
	uint16 ypos;
	Widget *w;
	int i;

	if (g_widgetMentatScrollbar != NULL) {
		GUI_Widget_Free_WithScrollbar(g_widgetMentatScrollbar);
		g_widgetMentatScrollbar = NULL;
	}

	free(g_widgetMentatScrollUp); g_widgetMentatScrollUp = NULL;
	free(g_widgetMentatScrollDown); g_widgetMentatScrollDown = NULL;

	g_widgetMentatTail = NULL;
	ypos = 8;

	w = calloc(13, sizeof(Widget));

	for (i = 0; i < 13; i++) {
		w->index = i + 2;

		w->flags.all = 0;
		w->flags.s.buttonFilterLeft = 9;
		w->flags.s.buttonFilterRight = 1;

		w->clickProc = &GUI_Mentat_List_Click;

		w->drawParameterDown.text     = empty;
		w->drawParameterSelected.text = empty;
		w->drawParameterNormal.text   = empty;

		w->drawModeNormal = DRAW_MODE_TEXT;

		w->state.all      = 0;

		w->offsetX        = 24;
		w->offsetY        = ypos;
		w->width          = 0x88;
		w->height         = 8;
		w->parentID       = 8;

		if (g_widgetMentatTail != NULL) {
			g_widgetMentatTail = GUI_Widget_Link(g_widgetMentatTail, w);
		} else {
			g_widgetMentatTail = w;
		}

		ypos += 8;
		w++;
	}

	GUI_Widget_MakeInvisible(g_widgetMentatTail);
	GUI_Widget_MakeInvisible(w - 1);

	g_widgetMentatScrollbar = GUI_Widget_Allocate_WithScrollbar(15, 8, 168, 24, 8, 72, &GUI_Mentat_ScrollBar_Draw);

	g_widgetMentatTail = GUI_Widget_Link(g_widgetMentatTail, g_widgetMentatScrollbar);

	g_widgetMentatScrollDown = GUI_Widget_Allocate3(16, 0, 168, 96, g_sprites[385], g_sprites[386], GUI_Widget_Get_ByIndex(g_widgetMentatTail, 15), 1);
	g_widgetMentatScrollDown->shortcut  = 0;
	g_widgetMentatScrollDown->shortcut2 = 0;
	g_widgetMentatScrollDown->parentID  = 8;
	g_widgetMentatTail = GUI_Widget_Link(g_widgetMentatTail, g_widgetMentatScrollDown);

	g_widgetMentatScrollUp = GUI_Widget_Allocate3(17, 0, 168, 16, g_sprites[383], g_sprites[384], GUI_Widget_Get_ByIndex(g_widgetMentatTail, 15), 0);
	g_widgetMentatScrollUp->shortcut  = 0;
	g_widgetMentatScrollUp->shortcut2 = 0;
	g_widgetMentatScrollUp->parentID  = 8;
	g_widgetMentatTail = GUI_Widget_Link(g_widgetMentatTail, g_widgetMentatScrollUp);

	g_widgetMentatTail = GUI_Widget_Link(g_widgetMentatTail, g_widgetMentatFirst);
}

static void GUI_Mentat_ShowHelp(void)
{
	MentatState *mentat = &g_mentat_state;

	struct {
		uint8  notused[8];
		uint32 length;
	} info;
	uint8 *subject;
	uint16 i;
	bool noDesc;
	uint8 fileID;
	uint32 offset;
	char *compressedText;
	char *desc;
	char *picture;
	char *text;

	subject = s_helpSubjects;

	for (i = 0; i < s_selectedHelpSubject; i++) subject = String_NextString(subject);

	noDesc = (subject[5] == '0');
	offset = HTOBE32(*(uint32 *)(subject + 1));

	fileID = ChunkFile_Open(s_mentatFilename);
	ChunkFile_Read(fileID, HTOBE32(CC_INFO), &info, 12);
	ChunkFile_Close(fileID);

	info.length = HTOBE32(info.length);

	text = g_readBuffer;
	compressedText = malloc(info.length);

	fileID = File_Open(s_mentatFilename, 1);
	File_Seek(fileID, offset, 0);
	File_Read(fileID, compressedText, info.length);
	String_Decompress(compressedText, text);
	String_TranslateSpecial(text, text);
	File_Close(fileID);

	while (*text != '*' && *text != '?') text++;

#if 0
	bool loc12;
	loc12 = (*text == '*');
#endif

	*text++ = '\0';

	if (noDesc) {
		uint16 index;

		picture = g_scenario.pictureBriefing;
		desc    = NULL;
		text    = (char *)g_readBuffer;

		index = *text - 44 + g_campaignID * 4 + STR_HOUSE_HARKONNENFROM_THE_DARK_WORLD_OF_GIEDI_PRIME_THE_SAVAGE_HOUSE_HARKONNEN_HAS_SPREAD_ACROSS_THE_UNIVERSE_A_CRUEL_PEOPLE_THE_HARKONNEN_ARE_RUTHLESS_TOWARDS_BOTH_FRIEND_AND_FOE_IN_THEIR_FANATICAL_PURSUIT_OF_POWER + g_playerHouseID * 40;

		strncpy(g_readBuffer, String_Get_ByIndex(index), g_readBufferSize);
	} else {
		picture = (char *)g_readBuffer;
		desc    = text;

		while (*text != '\0' && *text != 0xC) text++;
		if (*text != '\0') *text++ = '\0';
	}

	free(compressedText);
	mentat->state = MENTAT_PAUSE_DESCRIPTION;
	mentat->desc = desc;
	mentat->text = text;

	mentat->wsa = WSA_LoadFile(picture, GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), false);
	mentat->wsa_timer = Timer_GetTicks();
	mentat->wsa_frame = 0;

	if (enhancement_fix_typos && (g_gameConfig.language == LANGUAGE_ENGLISH) && (desc != NULL)) {
		/* Barracks: "Power Needs: 20" to 10. */
		if (strcasecmp("BARRAC.WSA", picture) == 0) {
			char *str = strstr(desc, "Power Needs: 20");
			if (str != NULL)
				str[13] = '1';
		}

		/* Frigate: text is in German. */
		else if (strcasecmp("FRIGATE.WSA", picture) == 0) {
			const char *fix_text = "These enormous spacecraft are used by CHOAM to transport bulk shipments from orbit to a starport.";
			const char *fix_desc = "CHOAM Frigate\rPlanetary Transport";

			strcpy(mentat->desc, fix_desc);
			mentat->text = mentat->desc + strlen(fix_desc) + 1;
			strcpy(mentat->text, fix_text);
		}

		/* Devastator: "It's armor and weapons are unequaled." */
		else if (strcasecmp("HARKTANK.WSA", picture) == 0 && g_playerHouseID == HOUSE_HARKONNEN) {
			char *str = strstr(text, "It's armor");
			if (str != NULL)
				memmove(str + 2, str + 3, strlen(str + 3) + 1);
		}

		/* Siege tank: "It's armor and weaponry has the power you will need to complete your conquest of this planet." */
		else if (strcasecmp("HTANK.WSA", picture) == 0 && g_playerHouseID == HOUSE_HARKONNEN) {
			char *str = strstr(text, "It's armor");
			if (str != NULL)
				memmove(str + 2, str + 3, strlen(str + 3) + 1);
		}

		/* Ornithoper: "The ornithopter is a fast and maneuverable, aircraft." */
		else if (strcasecmp("ORNI.WSA", picture) == 0 && g_playerHouseID == HOUSE_ORDOS) {
			char *str = strstr(text, "maneuverable, aircraft");
			if (str != NULL)
				memmove(str + 12, str + 13, strlen(str + 13) + 1);
		}

		/* Sardaukar: text is in German. */
		else if (strcasecmp("SARDUKAR.WSA", picture) == 0) {
			const char *fix_text = "These are the elite troops of the Emperor.";
			const char *fix_desc = "Sardaukar Troopers\rMobility: Foot\rFirepower: Heavy";

			strcpy(mentat->desc, fix_desc);
			mentat->text = mentat->desc + strlen(fix_desc) + 1;
			strcpy(mentat->text, fix_text);
		}

		/* Starport: "Power Needs: 80" to 50. */
		else if (strcasecmp("STARPORT.WSA", picture) == 0) {
			char *str = strstr(desc, "Power Needs: 80");
			if (str != NULL)
				str[13] = '5';
		}
	}

#if 0
	GUI_Mentat_Loop(picture, desc, text, loc12 ? 1 : 0, g_widgetMentatFirst);

	GUI_Widget_MakeNormal(g_widgetMentatFirst, false);

	GUI_Mentat_LoadHelpSubjects(false);

	GUI_Mentat_Create_HelpScreen_Widgets();

	GUI_Mentat_Draw(true);
#endif
}

/**
 * Handles Click event for list in mentat window.
 *
 * @param w The widget.
 */
bool GUI_Mentat_List_Click(Widget *w)
{
	uint16 index;
	Widget *w2;

	index = s_selectedHelpSubject + 3;

	if (w->index != index) {
		w2 = GUI_Widget_Get_ByIndex(g_widgetMentatTail, index);

		GUI_Widget_MakeNormal(w, false);
		GUI_Widget_MakeNormal(w2, false);

		if (w2->stringID == 0x31) {
			w2->fgColourDown   = 15;
			w2->fgColourNormal = 15;
		}

		if (w->stringID == 0x31) {
			w->fgColourDown   = 8;
			w->fgColourNormal = 8;
		}

		s_selectedHelpSubject = w->index - 3;
		return true;
	}

	if ((w->state.s.buttonState & 0x11) == 0 && !s_selectMentatHelp) return true;

	if (w->stringID != 0x31) return true;

	GUI_Widget_MakeNormal(w, false);

	GUI_Mentat_ShowHelp();

#if 0
	GUI_Mentat_Draw(true);

	Input_HandleInput(0x841);
	Input_HandleInput(0x842);
#endif
	return false;
}

void GUI_Mentat_ScrollBar_Draw(Widget *w)
{
	GUI_Mentat_SelectHelpSubject(GUI_Get_Scrollbar_Position(w) - s_topHelpList);
#if 0
	GUI_Mentat_Draw(false);
#endif
}

#if 0
/* Moved to gui/menu_opendune.c. */
static bool GUI_Mentat_DrawInfo(char *text, uint16 left, uint16 top, uint16 height, uint16 skip, int16 lines, uint16 flags);
extern uint16 GUI_Mentat_Loop(const char *wsaFilename, char *pictureDetails, char *text, bool arg12, Widget *w);
#endif

uint16 GUI_Mentat_SplitText(char *str, uint16 maxWidth)
{
	uint16 lines = 0;
	uint16 height = 0;

	if (str == NULL) return 0;

	while (*str != '\0') {
		uint16 width = 0;

		while (width < maxWidth && *str != '.' && *str != '!' && *str != '?' && *str != '\0' && *str != '\r') {
			width += Font_GetCharWidth(*str++);
		}

		if (width >= maxWidth) {
			while (*str != ' ') width -= Font_GetCharWidth(*str--);
		}

		height++;

		if ((*str != '\0' && (*str == '.' || *str == '!' || *str == '?' || *str == '\r')) || height >= 3) {
			while (*str != '\0' && (*str == ' ' || *str == '.' || *str == '!' || *str == '?' || *str == '\r')) str++;

			if (*str != '\0') str[-1] = '\0';
			height = 0;
			lines++;
			continue;
		}

		if (*str == '\0') {
			lines++;
			height = 0;
			continue;
		}

		*str++ = '\r';
	}

	return lines;
}

#if 0
/* Moved to gui/menu_opendune.c. */
extern uint16 GUI_Mentat_Tick(void);
#endif
