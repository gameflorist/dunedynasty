/* savemenu.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <ctype.h>
#include <string.h>
#include "enum_string.h"
#include "multichar.h"
#include "../os/endian.h"
#include "../os/math.h"
#include "../os/strings.h"

#include "savemenu.h"

#include "scrollbar.h"
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

static Widget *s_scrollbar;
static int s_last_index;

char g_savegameDesc[5][51];                                 /*!< Array of savegame descriptions for the SaveLoad window. */

static bool
SaveMenu_IsValidFilename(const ALLEGRO_PATH *path)
{
	const char *extension = al_get_path_extension(path);
	if (strcasecmp(extension, ".DAT") != 0)
		return false;

	const char *basename = al_get_path_basename(path);
	if (strncasecmp(basename, "_SAVE", 5) != 0)
		return false;

	const char *digit = basename + 5;
	while (*digit != '\0') {
		if (!isdigit(*digit))
			return false;

		digit++;
	}

	return true;
}

static void
SaveMenu_FindSavedGames(bool save, Widget *scrollbar)
{
	char dirname[1024];
	File_MakeCompleteFilename(dirname, sizeof(dirname), SEARCHDIR_PERSONAL_DATA_DIR, "", false);

	WidgetScrollbar *ws = scrollbar->data;
	ws->scrollMax = 0;

	ALLEGRO_FS_ENTRY *e = al_create_fs_entry(dirname);
	if (e != NULL) {
		if (al_open_directory(e)) {
			ALLEGRO_FS_ENTRY *f = al_read_directory(e);
			while (f != NULL) {
				ALLEGRO_PATH *path = al_create_path(al_get_fs_entry_name(f));

				/* Filename should be _SAVE###.DAT */
				if (SaveMenu_IsValidFilename(path)) {
					const char *filename = al_get_path_filename(path);
					ScrollbarItem *si = Scrollbar_AllocItem(scrollbar, SCROLLBAR_ITEM);
					int index;

					strncpy(si->text, filename, sizeof(si->text));
					sscanf(filename + 5, "%d", &index);
					s_last_index = max(index, s_last_index);
				}

				al_destroy_path(path);
				al_destroy_fs_entry(f);
				f = al_read_directory(e);
			}

			al_close_directory(e);
		}

		al_destroy_fs_entry(e);
	}

	/* If saving, generate a new name. */
	if (save) {
		ScrollbarItem *si = Scrollbar_AllocItem(scrollbar, SCROLLBAR_ITEM);
		snprintf(si->text, sizeof(si->text), "_SAVE%03d.DAT", s_last_index + 1);
	}

	Scrollbar_Sort(scrollbar);
	GUI_Widget_Scrollbar_Init(scrollbar, ws->scrollMax, 5, 0);
}

static void
SaveMenu_FillSavegameDesc(bool save, Widget *scrollbar)
{
	WidgetScrollbar *ws = scrollbar->data;

	for (int i = 0; i < 5; i++) {
		const int entry = ws->scrollPosition + i;
		char *desc = g_savegameDesc[i];
		*desc = '\0';

		ScrollbarItem *si = Scrollbar_GetItem(scrollbar, entry);
		if (si == NULL)
			continue;

		if (save && entry == 0) {
			strcpy(desc, String_Get_ByIndex(STR_EMPTY_SLOT_));
			continue;
		}

		const char *filename = si->text;
		if (!File_Exists_Personal(filename)) continue;

		uint8 fileId = ChunkFile_Open_Personal(filename);
		ChunkFile_Read(fileId, HTOBE32(CC_NAME), desc, 50);
		ChunkFile_Close(fileId);
	}
}

static void
SaveMenu_FreeScrollbar(void)
{
	Widget *w = s_scrollbar;

	while (w != NULL) {
		Widget *next = w->next;

		if (w == s_scrollbar) {
			GUI_Widget_Free_WithScrollbar(w);
		}
		else {
			free(w);
		}

		w = next;
	}

	s_scrollbar = NULL;
}

/* return values:
 * -2: game was saved.
 * -1: cancel clicked.
 *  0: stay in save game loop.
 */
int
SaveMenu_Savegame_Click(uint16 key)
{
	const uint16 loc08 = 1;
	char *saveDesc = g_savegameDesc[key];
	Widget *w = g_widgetLinkedListTail;
	Widget *scrollbar = s_scrollbar;
	int loc0A = GUI_EditBox(saveDesc, 50, 15, g_widgetLinkedListTail, NULL, loc08);

	if ((loc0A & 0x8000) == 0)
		return 0;

	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(w, loc0A & 0x7FFF), false);

	switch (loc0A & 0x7FFF) {
		case 0x1E:
			if (*saveDesc == '\0')
				break;

			const WidgetScrollbar *ws = scrollbar->data;
			const int entry = ws->scrollPosition + key;
			const ScrollbarItem *si = Scrollbar_GetItem(scrollbar, entry);
			SaveFile(si->text, saveDesc);
			SaveMenu_FreeScrollbar();
			return -2;

		case 0x1F:
			SaveMenu_FreeScrollbar();
			return -1;

		default:
			break;
	}

	return 0;
}

static void
SaveMenu_UpdateArrows(Widget *scrollbar)
{
	WidgetScrollbar *ws = scrollbar->data;
	Widget *w;

	w = &g_table_windowWidgets[7];
	if (ws->scrollPosition > 0) {
		GUI_Widget_MakeVisible(w);
	}
	else {
		GUI_Widget_MakeInvisible(w);
	}

	w = &g_table_windowWidgets[8];
	if (ws->scrollPosition + ws->scrollPageSize < ws->scrollMax) {
		GUI_Widget_MakeVisible(w);
	}
	else {
		GUI_Widget_MakeInvisible(w);
	}
}

void
SaveMenu_InitSaveLoad(bool save)
{
	WindowDesc *desc = &g_saveLoadWindowDesc;

	s_scrollbar = Scrollbar_Allocate(NULL, 0, 0, 0, 0, false);

	s_last_index = -1;
	SaveMenu_FindSavedGames(save, s_scrollbar);
	SaveMenu_FillSavegameDesc(save, s_scrollbar);

	desc->stringID = save ? STR_SELECT_A_POSITION_TO_SAVE_TO : STR_SELECT_A_SAVED_GAME_TO_LOAD;

	GUI_Window_Create(desc);

	WidgetScrollbar *ws = s_scrollbar->data;
	if (ws->scrollMax >= 5 && desc->addArrows) {
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

	SaveMenu_UpdateArrows(s_scrollbar);
}

/* return values:
 * -3: scroll button pressed.
 * -2: game was loaded.
 * -1: cancel clicked.
 *  0: stay in save/load game loop.
 * 1+: begin save game entry.
 */
int
SaveMenu_SaveLoad_Click(bool save)
{
	Widget *scrollbar = s_scrollbar;
	Widget *w = g_widgetLinkedListTail;
	uint16 key = GUI_Widget_HandleEvents(w);
	int ret = 0;

	if (key == (0x80 | MOUSE_ZAXIS)) {
		if ((g_mouseDZ > 0) && (!g_table_windowWidgets[7].flags.invisible)) {
			key = 0x8025;
		}
		else if ((g_mouseDZ < 0) && (!g_table_windowWidgets[8].flags.invisible)) {
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
					Scrollbar_ArrowUp_Click(scrollbar);
				}
				else {
					Scrollbar_ArrowDown_Click(scrollbar);
				}

				SaveMenu_UpdateArrows(scrollbar);
				SaveMenu_FillSavegameDesc(save, scrollbar);
				GUI_Widget_MakeNormal(w2, false);
				return -3;

			case 0x23: /* Cancel */
				SaveMenu_FreeScrollbar();
				return -1;

			default:
				if (!save) {
					const WidgetScrollbar *ws = scrollbar->data;
					const int entry = ws->scrollPosition + (key - 0x1E);
					const ScrollbarItem *si = Scrollbar_GetItem(scrollbar, entry);
					LoadFile(si->text);
					SaveMenu_FreeScrollbar();
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
