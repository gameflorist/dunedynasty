/* savemenu.c */

#include <assert.h>
#include <string.h>
#include "fourcc.h"
#include "../os/endian.h"
#include "../os/math.h"
#include "../os/strings.h"

#include "savemenu.h"

#include "../audio/audio.h"
#include "../file.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../load.h"
#include "../save.h"
#include "../shape.h"
#include "../string.h"
#include "../table/strings.h"

char g_savegameDesc[5][51];                                 /*!< Array of savegame descriptions for the SaveLoad window. */
static uint16 s_savegameIndexBase = 0;
static uint16 s_savegameCountOnDisk = 0;                    /*!< Amount of savegames on disk. */

static char *GenerateSavegameFilename(uint16 number)
{
	static char filename[13];
	sprintf(filename, "_SAVE%03d.DAT", number);
	return filename;
}

static uint16
GetSavegameCount(void)
{
	for (int i = 0;; i++) {
		if (!File_Exists_Personal(GenerateSavegameFilename(i))) return i;
	}
}

static void
FillSavegameDesc(bool save)
{
	for (int i = 0; i < 5; i++) {
		char *desc = g_savegameDesc[i];
		*desc = '\0';

		if (s_savegameIndexBase - i < 0) continue;

		if (s_savegameIndexBase - i == s_savegameCountOnDisk) {
			if (!save) continue;

			strcpy(desc, String_Get_ByIndex(STR_EMPTY_SLOT_));
			continue;
		}

		const char *filename = GenerateSavegameFilename(s_savegameIndexBase - i);
		if (!File_Exists_Personal(filename)) continue;

		uint8 fileId = ChunkFile_Open_Personal(filename);
		ChunkFile_Read(fileId, HTOBE32(CC_NAME), desc, 50);
		ChunkFile_Close(fileId);
	}
}

/* return values:
 * -2: game was saved.
 * -1: cancel clicked.
 *  0: stay in save game loop.
 */
int
GUI_Widget_Savegame_Click(uint16 key)
{
	const uint16 loc08 = 1;
	char *saveDesc = g_savegameDesc[key];
	Widget *w = g_widgetLinkedListTail;
	int loc0A = GUI_EditBox(saveDesc, 50, 15, g_widgetLinkedListTail, NULL, loc08);

	if ((loc0A & 0x8000) == 0)
		return 0;

	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(w, loc0A & 0x7FFF), false);

	switch (loc0A & 0x7FFF) {
		case 0x1E:
			if (*saveDesc == '\0')
				break;

			SaveFile(GenerateSavegameFilename(s_savegameIndexBase - key), saveDesc);
			return -2;

		case 0x1F:
			return -1;

		default:
			break;
	}

	return 0;
}

static void
UpdateArrows(bool save, bool force)
{
	static uint16 previousIndex = 0;
	Widget *w;

	if (!force && s_savegameIndexBase == previousIndex) return;

	previousIndex = s_savegameIndexBase;

	w = &g_table_windowWidgets[7];
	if (s_savegameCountOnDisk - (save ? 0 : 1) > s_savegameIndexBase) {
		GUI_Widget_MakeVisible(w);
	}
	else {
		GUI_Widget_MakeInvisible(w);
	}

	w = &g_table_windowWidgets[8];
	if (s_savegameIndexBase >= 5) {
		GUI_Widget_MakeVisible(w);
	}
	else {
		GUI_Widget_MakeInvisible(w);
	}
}

void
GUI_Widget_InitSaveLoad(bool save)
{
	WindowDesc *desc = &g_saveLoadWindowDesc;

	s_savegameCountOnDisk = GetSavegameCount();

	s_savegameIndexBase = max(0, s_savegameCountOnDisk - (save ? 0 : 1));

	FillSavegameDesc(save);

	desc->stringID = save ? STR_SELECT_A_POSITION_TO_SAVE_TO : STR_SELECT_A_SAVED_GAME_TO_LOAD;

	GUI_Window_Create(desc);

	if (s_savegameCountOnDisk >= 5 && desc->addArrows) {
		Widget *w = &g_table_windowWidgets[7];

		w->drawParameterNormal.sprite   = SHAPE_SAVE_LOAD_SCROLL_UP;
		w->drawParameterSelected.sprite = SHAPE_SAVE_LOAD_SCROLL_UP_PRESSED;
		w->drawParameterDown.sprite     = SHAPE_SAVE_LOAD_SCROLL_UP_PRESSED;
		w->next             = NULL;
		w->parentID         = desc->index;

		GUI_Widget_MakeNormal(w, false);
		GUI_Widget_MakeInvisible(w);

		g_widgetLinkedListTail = GUI_Widget_Link(g_widgetLinkedListTail, w);

		w = &g_table_windowWidgets[8];

		w->drawParameterNormal.sprite   = SHAPE_SAVE_LOAD_SCROLL_DOWN;
		w->drawParameterSelected.sprite = SHAPE_SAVE_LOAD_SCROLL_DOWN_PRESSED;
		w->drawParameterDown.sprite     = SHAPE_SAVE_LOAD_SCROLL_DOWN_PRESSED;
		w->next             = NULL;
		w->parentID         = desc->index;

		GUI_Widget_MakeNormal(w, false);
		GUI_Widget_MakeInvisible(w);

		g_widgetLinkedListTail = GUI_Widget_Link(g_widgetLinkedListTail, w);
	}

	UpdateArrows(save, true);
}

/* return values:
 * -3: scroll button pressed.
 * -2: game was loaded.
 * -1: cancel clicked.
 *  0: stay in save/load game loop.
 * 1+: begin save game entry.
 */
int
GUI_Widget_SaveLoad_Click(bool save)
{
	Widget *w = g_widgetLinkedListTail;
	uint16 key = GUI_Widget_HandleEvents(w);
	int ret = 0;

	UpdateArrows(save, false);

	if (key == (0x80 | MOUSE_ZAXIS)) {
		if ((g_mouseDZ > 0) && (!g_table_windowWidgets[7].flags.s.invisible)) {
			key = 0x8025;
		}
		else if ((g_mouseDZ < 0) && (!g_table_windowWidgets[8].flags.s.invisible)) {
			key = 0x8026;
		}
	}

	if (key & 0x8000) {
		Widget *w2;

		key &= 0x7FFF;
		w2 = GUI_Widget_Get_ByIndex(w, key);

		switch (key) {
			case 0x25: /* Up */
			case 0x26: /* Down */
				if (key == 0x25) {
					s_savegameIndexBase = min(s_savegameCountOnDisk - (save ? 0 : 1), s_savegameIndexBase + 1);
				}
				else {
					s_savegameIndexBase = max(0, s_savegameIndexBase - 1);
				}

				FillSavegameDesc(save);
				GUI_Widget_MakeNormal(w2, false);
				return -3;

			case 0x23: /* Cancel */
				return -1;

			default:
				if (!save) {
					LoadFile(GenerateSavegameFilename(s_savegameIndexBase - (key - 0x1E)));
					Audio_LoadSampleSet(g_table_houseInfo[g_playerHouseID].sampleSet);
					return -2;
				}
				else {
					return key;
				}
		}
	}

	return ret;
}
