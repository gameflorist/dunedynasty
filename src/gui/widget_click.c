/** @file src/gui/widget_click.c %Widget clicking handling routines. */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "enum_string.h"
#include "multichar.h"
#include "types.h"
#include "../os/sleep.h"

#include "gui.h"
#include "widget.h"
#include "../audio/audio.h"
#include "../config.h"
#include "../enhancement.h"
#include "../gfx.h"
#include "../house.h"
#include "../map.h"
#include "../newui/actionpanel.h"
#include "../opendune.h"
#include "../pool/house.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../shape.h"
#include "../sprites.h"
#include "../structure.h"
#include "../timer/timer.h"
#include "../unit.h"
#include "../video/video.h"

#if 0
/* Moved to newui/savemenu.c. */
static char *GenerateSavegameFilename(uint16 number);

/* Moved to newui/scrollbar.c. */
static void GUI_Widget_Scrollbar_Scroll(WidgetScrollbar *scrollbar, uint16 scroll);
#endif

/**
 * Handles Click event for a sprite/text button.
 *
 * @param w The widget.
 * @return False, always.
 */
bool GUI_Widget_SpriteTextButton_Click(Widget *w)
{
	Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);

	if (s == NULL)
		return false;

	if (s->o.type == STRUCTURE_STARPORT) {
		const House *h = House_Get_ByIndex(s->o.houseID);

		if (h->starportLinkedID == 0xFFFF) {
			return ActionPanel_ClickStarport(w, s);
		}
		else {
			return false;
		}
	}

	switch (g_productionStringID) {
		default: break;

		case STR_PLACE_IT:
		case STR_COMPLETED:
		case STR_ON_HOLD:
		case STR_BUILD_IT:
		case STR_D_DONE:
			return ActionPanel_ClickFactory(w, s);

		case STR_LAUNCH:
		case STR_FREMEN:
		case STR_SABOTEUR:
			return ActionPanel_ClickPalace(w, s);
	}
	return false;
}

#if 0
/* Moved to newui/scrollbar.c. */
extern bool GUI_Widget_Scrollbar_ArrowUp_Click(Widget *w);
extern bool GUI_Widget_Scrollbar_ArrowDown_Click(Widget *w);
extern bool GUI_Widget_Scrollbar_Click(Widget *w);
#endif

/**
 * Handles Click event for unit commands button.
 *
 * @param w The widget.
 * @return True, always.
 */
static bool GUI_Widget_TextButton_Click_(Widget *w, ActionType ref, Unit *u)
{
	const UnitInfo *ui;
	const ActionInfo *ai;
	const uint16 *actions;
	ActionType action;
	uint16 *found;
	ActionType unitAction;

	ui = &g_table_unitInfo[u->o.type];

	actions = ui->o.actionsPlayer;
	if (Unit_GetHouseID(u) != g_playerHouseID && u->o.type != UNIT_HARVESTER) {
		actions = g_table_actionsAI;
	}

	action = actions[w->index - 8];
	if ((action == ref) ||
	    (action == ACTION_RETREAT && ref == ACTION_RETURN) ||
	    (action == ACTION_RETURN && ref == ACTION_RETREAT)) {
	}
	else {
		return true;
	}

	unitAction = u->nextActionID;
	if (unitAction == ACTION_INVALID) {
		unitAction = u->actionID;
	}

	if (u->deviated != 0) {
		Unit_Deviation_Decrease(u, 5);
		if (u->deviated == 0) {
			GUI_Widget_MakeNormal(w, false);
			return true;
		}
	}

	GUI_Widget_MakeSelected(w, false);

	ai = &g_table_actionInfo[action];

	/* For single selection, we enter this loop when the selection
	 * type changes from SELECTIONTYPE_UNIT to SELECTIONTYPE_TARGET.
	 * For multiple selection, we need to abort the outer loop.
	 */
	if (ai->selectionType != g_selectionType) {
		u->deviationDecremented = true;
		g_unitActive = u;
		g_activeAction = action;
		GUI_ChangeSelectionType(ai->selectionType);

		return false;
	}
	else {
		u->deviationDecremented = false;
	}

	Object_Script_Variable4_Clear(&u->o);
	u->targetAttack = 0;
	u->targetMove = 0;
	u->route[0] = 0xFF;

	Unit_SetAction(u, action);

	if (ui->movementType == MOVEMENT_FOOT)
		Audio_PlaySample(ai->soundID, 255, 0.0f);

	if (unitAction == action) return true;

	found = memchr(actions, unitAction, 4);
	if (found == NULL) return true;

	GUI_Widget_MakeNormal(GUI_Widget_Get_ByIndex(g_widgetLinkedListHead, found - actions + 8), false);

	return true;
}

bool GUI_Widget_TextButton_Click(Widget *w)
{
	Unit *ref = Unit_GetForActionPanel();

	if (ref == NULL)
		return true;

	UnitInfo *ui = &g_table_unitInfo[ref->o.type];
	ActionType action = ui->o.actionsPlayer[w->index - 8];

	int iter;
	for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
		if (GUI_Widget_TextButton_Click_(w, action, u) == false)
			break;
	}

	if (enhancement_play_additional_voices)
		Audio_PlaySample(SAMPLE_BUTTON, 128, 0.0f);

	return true;
}

/**
 * Handles Click event for current selection name.
 *
 * @return False, always.
 */
bool GUI_Widget_Name_Click(Widget *w)
{
	int cx = 0, cy = 0, count = 0;
	VARIABLE_NOT_USED(w);

	Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);
	if (s != NULL) {
		cx = (s->o.position.x >> 4) + TILE_SIZE * g_selectionWidth / 2;
		cy = (s->o.position.y >> 4) + TILE_SIZE * g_selectionHeight / 2;
		Map_CentreViewport(cx, cy);
		return false;
	}

	int iter;
	Unit *u = Unit_FirstSelected(&iter);

	while (u != NULL) {
		cx += (u->o.position.x >> 4);
		cy += (u->o.position.y >> 4);
		count++;

		u = Unit_NextSelected(&iter);
	}

	if (count > 0) {
		Map_CentreViewport(cx / count, cy / count);
	}

	return false;
}

/**
 * Handles Click event for "Cancel" button.
 *
 * @return True, always.
 */
bool GUI_Widget_Cancel_Click(Widget *w)
{
	VARIABLE_NOT_USED(w);

	if (g_structureActiveType != 0xFFFF) {
		Structure *s  = Structure_Get_ByPackedTile(g_structureActivePosition);
		Structure *s2 = g_structureActive;

		assert(s2 != NULL);

		if (s != NULL) {
			s->o.linkedID = s2->o.index & 0xFF;
		} else {
			Structure_Free(s2);
		}

		g_structureActive = NULL;
		g_structureActiveType = 0xFFFF;

		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);

		g_selectionState = 0; /* Invalid. */
	}

	if (g_unitActive == NULL) return true;

	int iter;
	for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
		if (Unit_GetHouseID(u) != g_playerHouseID)
			continue;

		u->deviationDecremented = false;
	}

	g_unitActive = NULL;
	g_activeAction = 0xFFFF;

	Video_SetCursor(SHAPE_CURSOR_NORMAL);
	GUI_ChangeSelectionType(SELECTIONTYPE_UNIT);

	return true;
}

#if 0
/**
 * Handles Click event for current selection picture.
 *
 * @return False, always.
 */
bool GUI_Widget_Picture_Click(Widget *w)
{
	Structure *s;

	VARIABLE_NOT_USED(w);

	if (Unit_AnySelected()) {
		/* Unit_DisplayStatusText(g_unitSelected); */

		return false;
	}

	s = Structure_Get_ByPackedTile(g_selectionPosition);

	if (s == NULL || !g_table_structureInfo[s->o.type].o.flags.factory) return false;

	Structure_BuildObject(s, 0xFFFF);

	return false;
}
#else
bool
GUI_Widget_Picture_Click(Widget *w)
{
	VARIABLE_NOT_USED(w);

	Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);
	if (s == NULL)
		return false;

	if ((s->o.type == STRUCTURE_CONSTRUCTION_YARD) && (g_productionStringID == STR_PLACE_IT)) {
		ActionPanel_BeginPlacementMode(s);
	}
	else if ((s->o.type == STRUCTURE_PALACE) && (s->countDown == 0)) {
		Structure_ActivateSpecial(s);
	}
	else if ((s->o.type == STRUCTURE_STARPORT) && (!BuildQueue_IsEmpty(&s->queue))) {
		ActionPanel_ClickStarportOrder(s);
	}
	else if ((s->o.type == STRUCTURE_REPAIR) && (s->o.linkedID != 0xFF)) {
		Structure_SetState(s, STRUCTURE_STATE_READY);
	}

	return false;
}
#endif

/**
 * Handles Click event for "Repair/Upgrade" button.
 *
 * @param w The widget.
 * @return False, always.
 */
bool GUI_Widget_RepairUpgrade_Click(Widget *w)
{
	Structure *s;

	s = Structure_Get_ByPackedTile(g_selectionPosition);

	if (Structure_SetRepairingState(s, -1, w)) return false;
	Structure_SetUpgradingState(s, -1, w);

	return false;
}

#if 0
static void GUI_Widget_Undraw(Widget *w, uint8 colour);
#endif

void GUI_Window_Create(WindowDesc *desc)
{
	uint8 i;

	if (desc == NULL) return;

	g_widgetLinkedListTail = NULL;

	GFX_Screen_SetActive(SCREEN_1);

	uint16 old_widget = Widget_SetCurrentWidget(desc->index);

	for (i = 0; i < desc->widgetCount; i++) {
		Widget *w = &g_table_windowWidgets[i];

		if (GUI_String_Get_ByIndex(desc->widgets[i].stringID) == NULL) continue;

		w->next      = NULL;
		w->offsetX   = desc->widgets[i].offsetX;
		w->offsetY   = desc->widgets[i].offsetY;
		w->width     = desc->widgets[i].width;
		w->height    = desc->widgets[i].height;
		w->shortcut  = 0;
		w->shortcut2 = 0;

		if (desc != &g_savegameNameWindowDesc) {
			if (desc->widgets[i].labelStringId != STR_NULL) {
				w->shortcut = GUI_Widget_GetShortcut(*GUI_String_Get_ByIndex(desc->widgets[i].labelStringId));
			} else {
				w->shortcut = GUI_Widget_GetShortcut(*GUI_String_Get_ByIndex(desc->widgets[i].stringID));
			}
		}

		w->shortcut2 = desc->widgets[i].shortcut2;

		w->stringID = desc->widgets[i].stringID;
		w->drawModeNormal   = DRAW_MODE_CUSTOM_PROC;
		w->drawModeSelected = DRAW_MODE_CUSTOM_PROC;
		w->drawModeDown     = DRAW_MODE_CUSTOM_PROC;
		w->drawParameterNormal.proc   = &GUI_Widget_TextButton_Draw;
		w->drawParameterSelected.proc = &GUI_Widget_TextButton_Draw;
		w->drawParameterDown.proc     = &GUI_Widget_TextButton_Draw;
		w->parentID = desc->index;
		memset(&w->state, 0, sizeof(w->state));

		g_widgetLinkedListTail = GUI_Widget_Link(g_widgetLinkedListTail, w);

		GUI_Widget_MakeVisible(w);
		GUI_Widget_MakeNormal(w, false);
	}

#if 0
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
#endif

	Widget_SetCurrentWidget(old_widget);

	GFX_Screen_SetActive(SCREEN_0);
}

#if 0
static void GUI_Window_BackupScreen(WindowDesc *desc);
static void GUI_Window_RestoreScreen(WindowDesc *desc);
static void GUI_Widget_GameControls_Click(Widget *w);
static void ShadeScreen(void);
static void UnshadeScreen(void);
static bool GUI_YesNo(uint16 stringID);
extern bool GUI_Widget_Options_Click(Widget *w);

/* Moved to newui/savemenu.c. */
static uint16 GetSavegameCount(void);
static void FillSavegameDesc(bool save);
extern int GUI_Widget_Savegame_Click(uint16 key);
static void UpdateArrows(bool save, bool force);
extern void GUI_Widget_InitSaveLoad(bool save);
extern int GUI_Widget_SaveLoad_Click(bool save);
#endif

/**
 * Handles Click event for "Clear List" button.
 *
 * @param w The widget.
 * @return True, always.
 */
/* return values:
 * -1: cancel clicked.
 *  0: stay in loop.
 *  1: clear clicked.
 */
int GUI_Widget_HOF_ClearList_Click(Widget *w)
{
	const int ret = GUI_Widget_HandleEvents(w);

	/* "Are you sure you want to clear the high scores?" */
	if (ret == (0x8000 | 30)) { /* Yes */
		HallOfFameStruct *data = w->data;

		memset(data, 0, 128);

		if (File_Exists_Personal("SAVEFAME.DAT"))
			File_Delete_Personal("SAVEFAME.DAT");
		GUI_Widget_MakeNormal(w, false);
		return 1;
	}
	else if (ret == (0x8000 | 31)) { /* No */
		GUI_Widget_MakeNormal(w, false);
		return -1;
	}

	return 0;
}

#if 0
extern bool GUI_Widget_HOF_Resume_Click(Widget *w);
extern bool GUI_Production_List_Click(Widget *w);
extern bool GUI_Production_ResumeGame_Click(Widget *w);
extern bool GUI_Production_Upgrade_Click(Widget *w);
static void GUI_FactoryWindow_ScrollList(int16 step);
static void GUI_FactoryWindow_FailScrollList(int16 step);
extern bool GUI_Production_Down_Click(Widget *w);
extern bool GUI_Production_Up_Click(Widget *w);
static void GUI_Purchase_ShowInvoice(void);
extern bool GUI_Purchase_Invoice_Click(Widget *w);
extern bool GUI_Production_BuildThis_Click(Widget *w);
extern bool GUI_Purchase_Plus_Click(Widget *w);
extern bool GUI_Purchase_Minus_Click(Widget *w);
#endif
