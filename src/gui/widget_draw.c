/* $Id$ */

/** @file src/gui/widget_draw.c %Widget drawing routines. */

#include <stdio.h>
#include "types.h"

#include "font.h"
#include "gui.h"
#include "widget.h"
#include "../config.h"
#include "../enhancement.h"
#include "../opendune.h"
#include "../gfx.h"
#include "../house.h"
#include "../map.h"
#include "../newui/actionpanel.h"
#include "../pool/house.h"
#include "../pool/unit.h"
#include "../sprites.h"
#include "../string.h"
#include "../structure.h"
#include "../table/strings.h"
#include "../table/widgetinfo.h"
#include "../tile.h"
#include "../unit.h"
#include "../video/video.h"

/**
 * Draw a text button widget to the display, relative to its parent.
 *
 * @param w The widget (which is a button) to draw.
 */
void GUI_Widget_TextButton_Draw(Widget *w)
{
	uint16 oldScreenID;
	uint16 positionX, positionY;
	uint16 width, height;
	uint16 state;
	uint8 colour;

	if (w == NULL) return;

	oldScreenID = GFX_Screen_SetActive(2);

	positionX = w->offsetX + (g_widgetProperties[w->parentID].xBase << 3);
	positionY = w->offsetY +  g_widgetProperties[w->parentID].yBase;
	width     = w->width;
	height    = w->height;

	g_widgetProperties[19].xBase  = positionX >> 3;
	g_widgetProperties[19].yBase  = positionY;
	g_widgetProperties[19].width  = width >> 3;
	g_widgetProperties[19].height = height;

	state  = (w->state.s.selected) ? 0 : 2;
	colour = (w->state.s.hover2) ? 231 : 232;

	GUI_Widget_DrawBorder(19, state, 1);

	if (w->stringID == STR_CANCEL || w->stringID == STR_PREVIOUS || w->stringID == STR_YES || w->stringID == STR_NO) {
		GUI_DrawText_Wrapper(GUI_String_Get_ByIndex(w->stringID), positionX + (width / 2), positionY + 2, colour, 0, 0x122);
	} else {
		GUI_DrawText_Wrapper(GUI_String_Get_ByIndex(w->stringID), positionX + 3, positionY + 2, colour, 0, 0x22);
	}

	if (oldScreenID == 0) {
		GUI_Mouse_Hide_InRegion(positionX, positionY, positionX + width, positionY + height);
		GUI_Screen_Copy(positionX >> 3, positionY, positionX >> 3, positionY, width >> 3, height, 2, 0);
		GUI_Mouse_Show_InRegion();
	}

	GFX_Screen_SetActive(oldScreenID);
}

/**
 * Draw a sprite button widget to the display, relative to 0,0.
 *
 * @param w The widget (which is a button) to draw.
 */
void GUI_Widget_SpriteButton_Draw(Widget *w)
{
#if 0
	uint16 oldScreenID;
	uint16 positionX, positionY;
	uint16 width, height;
	uint16 spriteID;
	bool buttonDown;

	if (w == NULL) return;

	spriteID = 0;
	if (Unit_AnySelected()) {
		const Unit *u = Unit_FirstSelected(NULL);
		const UnitInfo *ui = &g_table_unitInfo[u->o.type];

		spriteID = ui->o.spriteID;
	} else {
		const StructureInfo *si;
		Structure *s;

		s = Structure_Get_ByPackedTile(g_selectionPosition);
		if (s == NULL) return;
		si = &g_table_structureInfo[s->o.type];

		spriteID = si->o.spriteID;
	}

	oldScreenID = g_screenActiveID;
	if (oldScreenID == 0) {
		GFX_Screen_SetActive(2);
	}

	buttonDown = w->state.s.hover2;

	positionX = w->offsetX;
	positionY = w->offsetY;
	width     = w->width;
	height    = w->height;

	GUI_DrawWiredRectangle(positionX - 1, positionY - 1, positionX + width, positionY + height, 12);

	GUI_DrawSprite(g_screenActiveID, g_sprites[spriteID], positionX, positionY, 0, 0x100, g_paletteMapping1, buttonDown ? 1 : 0);

	GUI_DrawBorder(positionX, positionY, width, height, buttonDown ? 0 : 1, false);

	if (oldScreenID != 0) return;

	GUI_Mouse_Hide_InRegion(positionX - 1, positionY - 1, positionX + width + 1, positionY + height + 1);
	GFX_Screen_Copy2(positionX - 1, positionY - 1, positionX - 1, positionY - 1, width + 2, height + 2, 2, 0, false);
	GUI_Mouse_Show_InRegion();

	GFX_Screen_SetActive(0);
#else
	VARIABLE_NOT_USED(w);
#endif
}

/**
 * Draw a sprite/text button widget to the display, relative to 0,0.
 *
 * @param w The widget (which is a button) to draw.
 */
void GUI_Widget_SpriteTextButton_Draw(Widget *w)
{
#if 0
	uint16 oldScreenID;
	Structure *s;
	uint16 positionX, positionY;
	uint16 width, height;
	uint16 spriteID;
	uint16 percentDone;
	bool buttonDown;

	if (w == NULL) return;

	spriteID    = 0;
	percentDone = 0;

	s = Structure_Get_ByPackedTile(g_selectionPosition);
	if (s == NULL) return;

	GUI_UpdateProductionStringID();

	oldScreenID = g_screenActiveID;
	if (oldScreenID == 0) {
		GFX_Screen_SetActive(2);
	}

	buttonDown = w->state.s.hover2;

	positionX = w->offsetX;
	positionY = w->offsetY;
	width     = w->width;
	height    = w->height;

	GUI_DrawWiredRectangle(positionX - 1, positionY - 1, positionX + width, positionY + height, 12);
	GUI_DrawBorder(positionX, positionY, width, height, buttonDown ? 0 : 1, true);

	switch (g_productionStringID) {
		case STR_LAUNCH:
			spriteID = 0x1E;
			break;

		case STR_FREMEN:
			spriteID = 0x5E;
			break;

		case STR_SABOTEUR:
			spriteID = 0x60;
			break;

		case STR_UPGRADINGD_DONE:
		default:
			spriteID = 0x0;
			break;

		case STR_PLACE_IT:
		case STR_COMPLETED:
		case STR_ON_HOLD:
		case STR_BUILD_IT:
		case STR_D_DONE:
			if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
				const StructureInfo *si;
				uint16 spriteWidth;
				uint16 x, y;
				uint8 *sprite;

				GUI_DrawSprite(g_screenActiveID, g_sprites[63], positionX + 37, positionY + 5, 0, 0x100, g_paletteMapping1, buttonDown ? 2 : 0);

				sprite = g_sprites[24];
				spriteWidth = Sprite_GetWidth(sprite) + 1;

				si = &g_table_structureInfo[s->objectType];

				for (y = 0; y < g_table_structure_layoutSize[si->layout].height; y++) {
					for (x = 0; x < g_table_structure_layoutSize[si->layout].width; x++) {
						GUI_DrawSprite(g_screenActiveID, sprite, positionX + x * spriteWidth + 38, positionY + y * spriteWidth + 6, 0, 0);
					}
				}

				spriteID = si->o.spriteID;
			} else {
				const UnitInfo *ui;

				ui = &g_table_unitInfo[s->objectType];
				spriteID = ui->o.spriteID;
			}
			break;
	}

	if (spriteID != 0) GUI_DrawSprite(g_screenActiveID, g_sprites[spriteID], positionX + 2, positionY + 2, 0, 0x100, g_paletteMapping1, buttonDown ? 1 : 0);

	if (g_productionStringID == STR_D_DONE) {
		uint16 buildTime;
		uint16 timeLeft;

		if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
			const StructureInfo *si;

			si = &g_table_structureInfo[s->objectType];
			buildTime = si->o.buildTime;
		} else if (s->o.type == STRUCTURE_REPAIR) {
			const UnitInfo *ui;

			if (s->o.linkedID == 0xFF) return;

			ui = &g_table_unitInfo[Unit_Get_ByIndex(s->o.linkedID)->o.type];
			buildTime = ui->o.buildTime;
		} else {
			const UnitInfo *ui;

			ui = &g_table_unitInfo[s->objectType];
			buildTime = ui->o.buildTime;
		}

		timeLeft = buildTime - (s->countDown + 255) / 256;
		percentDone = 100 * timeLeft / buildTime;
	}

	if (g_productionStringID == STR_UPGRADINGD_DONE) {
		percentDone = 100 - s->upgradeTimeLeft;

		GUI_DrawText_Wrapper(
			String_Get_ByIndex(g_productionStringID),
			positionX + 1,
			positionY + height - 19,
			buttonDown ? 0xE : 0xF,
			0,
			0x021,
			percentDone
		);
	} else {
		GUI_DrawText_Wrapper(
			String_Get_ByIndex(g_productionStringID),
			positionX + width / 2,
			positionY + height - 9,
			(g_productionStringID == STR_PLACE_IT) ? 0xEF : (buttonDown ? 0xE : 0xF),
			0,
			0x121,
			percentDone
		);
	}

	if (g_productionStringID == STR_D_DONE || g_productionStringID == STR_UPGRADINGD_DONE) {
		w->shortcut = GUI_Widget_GetShortcut(*String_Get_ByIndex(STR_ON_HOLD));
	} else {
		w->shortcut = GUI_Widget_GetShortcut(*String_Get_ByIndex(g_productionStringID));
	}

	if (oldScreenID != 0x0) return;

	GUI_Mouse_Hide_InRegion(positionX - 1, positionY - 1, positionX + width + 1, positionY + height + 1);
	GFX_Screen_Copy2(positionX - 1, positionY - 1, positionX - 1, positionY - 1, width + 2, height + 2, 2, 0, false);
	GUI_Mouse_Show_InRegion();

	GFX_Screen_SetActive(0);
#else
	Structure *s = Structure_Get_ByPackedTile(g_selectionPosition);

	if (s == NULL)
		return;

	if (s->o.type == STRUCTURE_PALACE) {
		ActionPanel_DrawPalace(w, s);
	}
	else {
		ActionPanel_DrawFactory(w, s);
	}
#endif
}

/**
 * Draw a text button widget to the display, relative to 0,0.
 *
 * @param w The widget (which is a button) to draw.
 */
void GUI_Widget_TextButton2_Draw(Widget *w)
{
	uint16 oldScreenID;
	uint16 stringID;
	uint16 positionX, positionY;
	uint16 width, height;
	uint8 colour;
	bool buttonSelected;
	bool buttonDown;

	if (w == NULL) return;

	oldScreenID = g_screenActiveID;
	if (oldScreenID == 0) {
		GFX_Screen_SetActive(2);
	}

	stringID = w->stringID;

	buttonSelected = w->state.s.selected;
	buttonDown     = w->state.s.hover2;

	positionX = w->offsetX;
	positionY = w->offsetY;
	width     = w->width;
	height    = w->height;

	GUI_DrawWiredRectangle(positionX - 1, positionY - 1, positionX + width, positionY + height, 12);
	GUI_DrawBorder(positionX, positionY, width, height, buttonDown ? 0 : 1, true);

	colour = 0xF;
	if (buttonSelected) {
		colour = 0x6;
	} else if (buttonDown) {
		colour = 0xE;
	}

	if (!buttonDown && stringID == STR_REPAIR) {
		colour = 0xEF;
	}

	GUI_DrawText_Wrapper(
		String_Get_ByIndex(stringID),
		positionX + width / 2,
		positionY + 1,
		colour,
		0,
		0x121
	);

	GUI_Widget_SetShortcuts(w);

	if (oldScreenID != 0) return;

	GUI_Mouse_Hide_InRegion(positionX - 1, positionY - 1, positionX + width + 1, positionY + height + 1);
	GFX_Screen_Copy2(positionX - 1, positionY - 1, positionX - 1, positionY - 1, width + 2, height + 2, 2, 0, false);
	GUI_Mouse_Show_InRegion();

	GFX_Screen_SetActive(0);
}

/**
 * Draw a scrollbar widget to the display, relative to its parent.
 *
 * @param w The widget (which is a scrollbar) to draw.
 */
void GUI_Widget_Scrollbar_Draw(Widget *w)
{
	WidgetScrollbar *scrollbar;
	uint16 positionX, positionY;
	uint16 width, height;
	uint16 scrollLeft, scrollTop;
	uint16 scrollRight, scrollBottom;

	if (w == NULL) return;
	if (w->flags.s.invisible) return;

	scrollbar = w->data;

	width  = w->width;
	height = w->height;

	positionX = w->offsetX;
	if (w->offsetX < 0) positionX += g_widgetProperties[w->parentID].width << 3;
	positionX += g_widgetProperties[w->parentID].xBase<< 3;

	positionY = w->offsetY;
	if (w->offsetY < 0) positionY += g_widgetProperties[w->parentID].height;
	positionY += g_widgetProperties[w->parentID].yBase;

	if (width > height) {
		scrollLeft   = scrollbar->position + 1;
		scrollTop    = 1;
		scrollRight  = scrollLeft + scrollbar->size - 1;
		scrollBottom = height - 2;
	} else {
		scrollLeft   = 1;
		scrollTop    = scrollbar->position + 1;
		scrollRight  = width - 2;
		scrollBottom = scrollTop + scrollbar->size - 1;
	}

	if (g_screenActiveID == 0) {
		GUI_Mouse_Hide_InRegion(positionX, positionY, positionX + width - 1, positionY + height - 1);
	}

	/* Draw background */
	GUI_DrawFilledRectangle(positionX, positionY, positionX + width - 1, positionY + height - 1, w->bgColourNormal);

	/* Draw where we currently are */
	GUI_DrawFilledRectangle(positionX + scrollLeft, positionY + scrollTop, positionX + scrollRight, positionY + scrollBottom, (scrollbar->pressed == 0) ? w->fgColourNormal : w->fgColourSelected);

	if (g_screenActiveID == 0) {
		GUI_Mouse_Show_InRegion();
	}

	/* Call custom callback function if set */
	if (scrollbar->drawProc != NULL) scrollbar->drawProc(w);

	scrollbar->dirty = 0;
}

/**
 * Gets the action type used to determine how to draw the panel on the right side of the screen.
 *
 * @param forceDraw Wether to draw the panel even if nothing changed.
 */
static uint16 GUI_Widget_ActionPanel_GetActionType(bool forceDraw)
{
	static uint16 displayedActionType       = 1;
	static uint16 displayedHitpoints        = 0xFFFF;
	static uint16 displayedIndex            = 0xFFFF;
	static uint16 displayedCountdown        = 0xFFFF;
	static uint16 displayedObjectType       = 0xFFFF;
	static uint16 displayedStructureLoFlags = 0;
	static uint16 displayedStructureHiFlags = 0;
	static uint16 displayedLinkedID         = 0xFFFF;
	static uint16 displayedHouseID          = 0xFFFF;
	static uint16 displayedActiveAction     = 0xFFFF;
	static uint16 displayedMissileCountdown = 0;
	static uint16 displayedUpgradeTime      = 0xFFFF;
	static uint16 displayedStarportTime     = 0xFFFF;

	uint16 actionType = 0;
	Structure *s = NULL;
	Unit *u = NULL;

	if (g_selectionType == SELECTIONTYPE_PLACE) {
		if (displayedActionType != 7 || forceDraw) actionType = 7; /* Placement */
	} else if (g_unitHouseMissile != NULL) {
		if (displayedMissileCountdown != g_houseMissileCountdown || forceDraw) actionType = 8; /* House Missile */
	} else if (Unit_AnySelected()) {
		if (g_selectionType == SELECTIONTYPE_TARGET) {
			uint16 activeAction = g_activeAction;

			if (activeAction != displayedActiveAction || forceDraw) {
				switch (activeAction) {
					case ACTION_ATTACK: actionType = 4; break; /* Attack */
					case ACTION_MOVE:   actionType = 5; break; /* Movement */
					default:            actionType = 6; break; /* Harvest */
				}
			}

			if (actionType == displayedActionType && !forceDraw) actionType = 0;
		} else {
			u = Unit_GetForActionPanel();

			if (forceDraw
				|| u->o.index     != displayedIndex
				|| u->o.hitpoints != displayedHitpoints
				|| u->o.houseID   != displayedHouseID
				|| u->actionID    != displayedActiveAction) {
					actionType = 2; /* Unit */
			}
		}
	} else if (!Tile_IsOutOfMap(g_selectionPosition) && (g_map[g_selectionPosition].isUnveiled || g_debugScenario)) {
		if (Map_GetLandscapeType(g_selectionPosition) == LST_STRUCTURE) {
			s = Structure_Get_ByPackedTile(g_selectionPosition);

			if (forceDraw
				|| s->o.hitpoints       != displayedHitpoints
				|| s->o.index           != displayedIndex
				|| s->countDown         != displayedCountdown
				|| s->upgradeTimeLeft   != displayedUpgradeTime
				|| s->o.linkedID        != displayedLinkedID
				|| s->objectType        != displayedObjectType
				|| s->o.flags.half.high != displayedStructureHiFlags
				|| s->o.houseID         != displayedHouseID
				|| House_Get_ByIndex(s->o.houseID)->starportTimeLeft != displayedStarportTime
				|| s->o.flags.half.low  != displayedStructureLoFlags) {
					g_variable_37B2 = (s->o.hitpoints > (g_table_structureInfo[s->o.type].o.hitpoints / 2)) ? 1 : 0;
					actionType = 3; /* Structure */
			}
		} else {
			actionType = 1;
		}
	} else {
		actionType = 1;
	}

	switch (actionType) {
		case 8: /* House Missile */
			displayedMissileCountdown = g_houseMissileCountdown;
			displayedIndex = 0xFFFF;
			break;

		case 1:
		case 4: /* Attack */
		case 5: /* Movement */
		case 6: /* Harvest */
		case 7: /* Placement */
			displayedIndex = 0xFFFF;
			displayedMissileCountdown = 0xFFFF;
			if (!forceDraw && actionType == displayedActionType) actionType = 0;
			break;

		case 2: /* Unit */
			displayedHitpoints        = u->o.hitpoints;
			displayedIndex            = u->o.index;
			displayedActiveAction     = u->actionID;
			displayedMissileCountdown = 0xFFFF;
			displayedHouseID          = u->o.houseID;
			break;

		case 3: /* Structure */
			displayedHitpoints        = s->o.hitpoints;
			displayedIndex            = s->o.index;
			displayedObjectType       = s->objectType;
			displayedCountdown        = s->countDown;
			displayedUpgradeTime      = s->upgradeTimeLeft;
			displayedStructureHiFlags = s->o.flags.half.high;
			displayedLinkedID         = s->o.linkedID;
			displayedStructureLoFlags = s->o.flags.half.low;
			displayedHouseID          = s->o.houseID;
			displayedMissileCountdown = 0xFFFF;
			displayedStarportTime     = House_Get_ByIndex(s->o.houseID)->starportTimeLeft;
			break;

		case 0:
		default:
			break;
	}

	if (actionType != 0) displayedActionType = actionType;

	return actionType;
}

/**
 * Draw the panel on the right side of the screen, with the actions of the
 *  selected item.
 *
 * @param forceDraw Wether to draw the panel even if nothing changed.
 */
void GUI_Widget_ActionPanel_Draw(bool forceDraw)
{
	const StructureInfo *si;
	const ObjectInfo *oi;
	const UnitInfo *ui;
	uint16 actionType;
	uint16 oldScreenID, loc06;
	bool isNotPlayerOwned;
	Object *o;
	Unit *u;
	Structure *s;
	House *h;
	Widget *buttons[4];
	Widget *widget24, *widget28, *widget2C, *widget30, *widget34;
	int i;

	o  = NULL;
	u  = NULL;
	s  = NULL;
	h  = NULL;

	oi = NULL;
	ui = NULL;
	si = NULL;
	isNotPlayerOwned = false;

	actionType = GUI_Widget_ActionPanel_GetActionType(forceDraw);

	switch (actionType) {
		case 2: { /* Unit */
			u  = Unit_GetForActionPanel();
			ui = &g_table_unitInfo[u->o.type];

			o  = &u->o;
			oi = &ui->o;

			isNotPlayerOwned = (g_playerHouseID == Unit_GetHouseID(u)) ? false : true;

			h = House_Get_ByIndex(u->o.houseID);
		} break;

		case 3: { /* Structure */
			s  = Structure_Get_ByPackedTile(g_selectionPosition);
			si = &g_table_structureInfo[s->o.type];

			o  = &s->o;
			oi = &si->o;

			isNotPlayerOwned = (g_playerHouseID == s->o.houseID) ? false : true;

			h = House_Get_ByIndex(s->o.houseID);

			if (s->upgradeTimeLeft == 0 && Structure_IsUpgradable(s)) s->upgradeTimeLeft = 100;
			GUI_UpdateProductionStringID();
		} break;

		case 7: { /* Placement */
			si = &g_table_structureInfo[g_structureActiveType];

			o = NULL;
			oi = &si->o;

			isNotPlayerOwned = false;

			h = House_Get_ByIndex(g_playerHouseID);
		} break;

		case 8: { /* House Missile */
			u  = g_unitHouseMissile;
			ui = &g_table_unitInfo[u->o.type];

			o  = &u->o;
			oi = &ui->o;

			isNotPlayerOwned = (g_playerHouseID == Unit_GetHouseID(u)) ? false : true;

			h = House_Get_ByIndex(g_playerHouseID);
		} break;

		case 4: /* Attack */
		case 5: /* Movement */
		case 6: /* Harvest */
		default: /* Default */
			break;

	}

	oldScreenID = g_screenActiveID;
	loc06 = g_curWidgetIndex;

	if (actionType != 0) {
		Widget *w = g_widgetLinkedListHead;

		oldScreenID = GFX_Screen_SetActive(2);

		loc06 = Widget_SetCurrentWidget(6);

		widget30 = GUI_Widget_Get_ByIndex(w, 7);
		GUI_Widget_MakeInvisible(widget30);

		widget24 = GUI_Widget_Get_ByIndex(w, 4);
		GUI_Widget_MakeInvisible(widget24);

		widget28 = GUI_Widget_Get_ByIndex(w, 6);
		GUI_Widget_MakeInvisible(widget28);

		widget2C = GUI_Widget_Get_ByIndex(w, 5);
		GUI_Widget_MakeInvisible(widget2C);

		widget34 = GUI_Widget_Get_ByIndex(w, 3);
		GUI_Widget_MakeInvisible(widget34);

		/* Create the 4 buttons */
		for (i = 0; i < 4; i++) {
			buttons[i] = GUI_Widget_Get_ByIndex(w, i + 8);
			GUI_Widget_MakeInvisible(buttons[i]);
		}

		GUI_Widget_DrawBorder(g_curWidgetIndex, 0, 0);
	}

	if (actionType > 1) {
		uint16 stringID = STR_NULL;
		uint16 spriteID = 0xFFFF;

		switch (actionType) {
			case 4: stringID = STR_TARGET; break; /* Attack */
			case 5: stringID = STR_MOVEMENT; break; /* Movement */
			case 6: stringID = STR_HARVEST;  break; /* Harvest */

			case 2: /* Unit */
			case 3: /* Structure */
			case 7: /* Placement */
			case 8: /* House Missile */
				stringID = oi->stringID_abbrev;
				break;

			default: break;
		}

		if (stringID != STR_NULL) {
			const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_NAME];

			GUI_DrawText_Wrapper(String_Get_ByIndex(stringID), wi->offsetX + wi->width/2, wi->offsetY + 1, 29, 0, 0x111);
		}

		switch (actionType) {
			case 3: /* Structure */
#if 0
				if (oi->flags.factory && !isNotPlayerOwned) {
					GUI_Widget_MakeVisible(widget28);
				}
#endif
				/* Fall through */
			case 7: /* Placement */
			case 2: /* Unit */
				spriteID = oi->spriteID;
				if (enhancement_special_trooper_portaits && (u != NULL)) {
					if ((u->o.houseID == HOUSE_FREMEN) && (u->o.type == UNIT_TROOPER)) {
						spriteID = SHAPE_FREMEN;
					}
					else if ((u->o.houseID == HOUSE_FREMEN) && (u->o.type == UNIT_TROOPERS)) {
						spriteID = SHAPE_FREMEN_SQUAD;
					}
					else if ((u->o.houseID == HOUSE_SARDAUKAR) && (u->o.type == UNIT_TROOPER || u->o.type == UNIT_TROOPERS)) {
						spriteID = SHAPE_SARDAUKAR;
					}
				}
				/* Fall through */
			case 4: /* Attack */
			case 5: /* Movement */
			case 6: /* Harvest */
			case 8: /* House Missile */
				ActionPanel_DrawPortrait(actionType, spriteID);
				break;

			default:
				break;
		}

		/* Unit / Structure */
		if (actionType == 2 || actionType == 3) {
			ActionPanel_DrawHealthBar(o->hitpoints, oi->hitpoints);
		}

		if (!isNotPlayerOwned || g_debugGame) {
			switch (actionType) {
				case 2: /* Unit */
				{
					const uint16 *actions;
					uint16 actionCurrent;
					int i;

					GUI_Widget_MakeVisible(widget34);

					actionCurrent = (u->nextActionID != ACTION_INVALID) ? u->nextActionID : u->actionID;

					actions = oi->actionsPlayer;
					if (isNotPlayerOwned && o->type != UNIT_HARVESTER) actions = g_table_actionsAI;

					for (i = 0; i < 4; i++) {
						buttons[i]->stringID = g_table_actionInfo[actions[i]].stringID;
						GUI_Widget_SetShortcuts(buttons[i]);
						GUI_Widget_MakeVisible(buttons[i]);

						if (actions[i] == actionCurrent) {
							GUI_Widget_MakeSelected(buttons[i], false);
						} else {
							GUI_Widget_MakeNormal(buttons[i], false);
						}
					}
				} break;

				case 3: /* Structure */
					GUI_Widget_MakeVisible(widget34);

					if (o->flags.s.upgrading) {
						widget24->stringID = STR_UPGRADING;

						GUI_Widget_MakeVisible(widget24);
						GUI_Widget_MakeSelected(widget24, false);
					} else if (o->hitpoints != oi->hitpoints) {
						if (o->flags.s.repairing) {
							widget24->stringID = STR_REPAIRING;

							GUI_Widget_MakeVisible(widget24);
							GUI_Widget_MakeSelected(widget24, false);
						} else {
							widget24->stringID = STR_REPAIR;

							GUI_Widget_MakeVisible(widget24);
							GUI_Widget_MakeNormal(widget24, false);
						}
					} else if (s->upgradeTimeLeft != 0) {
						widget24->stringID = STR_UPGRADE;

						GUI_Widget_MakeVisible(widget24);
						GUI_Widget_MakeNormal(widget24, false);
					}

					if ((oi->flags.factory && o->type != STRUCTURE_STARPORT) ||
					    (o->type == STRUCTURE_STARPORT && (h->starportLinkedID == 0xFFFF)) ||
					    (o->type == STRUCTURE_PALACE && s->countDown == 0)) {
						GUI_Widget_MakeVisible(widget2C);
						GUI_Widget_Draw(widget2C);
					}

					ActionPanel_DrawStructureDescription(s);
					break;

				case 4: /* Attack */
					GUI_Widget_MakeVisible(widget30);
					ActionPanel_DrawActionDescription(STR_SELECTTARGET, 259, 76, g_curWidgetFGColourBlink);
					break;

				case 5: /* Movement */
					GUI_Widget_MakeVisible(widget30);
					ActionPanel_DrawActionDescription(STR_SELECTDESTINATION, 259, 76, g_curWidgetFGColourBlink);
					break;

				case 6: /* Harvest */
					GUI_Widget_MakeVisible(widget30);
					ActionPanel_DrawActionDescription(STR_SELECTPLACE_TOHARVEST, 259, 76, g_curWidgetFGColourBlink);
					break;

				case 7: /* Placement */
					GUI_Widget_MakeVisible(widget30);
					ActionPanel_DrawActionDescription(STR_SELECTLOCATION_TOBUILD, 259, 84, g_curWidgetFGColourBlink);
					break;

				case 8: /* House Missile */
					ActionPanel_DrawMissileCountdown(g_curWidgetFGColourBlink, (int)g_houseMissileCountdown - 1);
					break;

				default:
					break;
			}
		}
	}

	if (actionType != 0) {
		GUI_Widget_Draw(widget24);
		GUI_Widget_Draw(widget28);
		GUI_Widget_Draw(widget2C);
		GUI_Widget_Draw(widget30);
		GUI_Widget_Draw(widget34);

		for (i = 0; i < 4; i++) {
			GUI_Widget_Draw(buttons[i]);
		}
	}

	if (actionType > 1) {
		Widget_SetCurrentWidget(loc06);

		GFX_Screen_SetActive(oldScreenID);
	}
}

/**
 * Draw the border around a widget.
 * @param widgetIndex The widget index to draw the border around.
 * @param borderType The type of border. 0 = normal, 1 = thick depth, 2 = double, 3 = thin depth.
 * @param pressed True if the button is pressed.
 */
void GUI_Widget_DrawBorder(uint16 widgetIndex, uint16 borderType, bool pressed)
{
	static const uint16 borderIndexSize[][2] = {
		{0, 0}, {2, 4}, {1, 1}, {2, 1}
	};

	uint16 left   = g_widgetProperties[widgetIndex].xBase << 3;
	uint16 top    = g_widgetProperties[widgetIndex].yBase;
	uint16 width  = g_widgetProperties[widgetIndex].width << 3;
	uint16 height = g_widgetProperties[widgetIndex].height;

	uint16 colourSchemaIndex = (pressed) ? 2 : 0;
	uint16 colourSchemaIndexDiff;
	uint16 size;

	if (g_screenActiveID == 0) {
		GUI_Mouse_Hide_InRegion(left, top, left + width, top + height);
	}

	GUI_DrawBorder(left, top, width, height, colourSchemaIndex + 1, true);

	colourSchemaIndexDiff = borderIndexSize[borderType][0];
	size = borderIndexSize[borderType][1];

	if (size != 0) {
		GUI_DrawBorder(left + size, top + size, width - (size * 2), height - (size * 2), colourSchemaIndexDiff + colourSchemaIndex, false);
	}

	if (g_screenActiveID == 0) {
		GUI_Mouse_Show_InRegion();
	}
}

void
GUI_Widget_DrawWindow(const WindowDesc *desc)
{
	const WidgetProperties *wi = &g_widgetProperties[desc->index];
	const char *title = GUI_String_Get_ByIndex(desc->stringID);

	GUI_Widget_DrawBorder(desc->index, 2, true);

	if (title != NULL) {
		GUI_DrawText_Wrapper(title, wi->xBase*8 + wi->width*8 / 2, wi->yBase + 6 + ((desc == &g_yesNoWindowDesc) ? 2 : 0), 238, 0, 0x122);
	}

	if (GUI_String_Get_ByIndex(desc->widgets[0].stringID) == NULL) {
		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_THERE_ARE_NO_SAVED_GAMES_TO_LOAD), (wi->xBase + 2) << 3, wi->yBase + 42, 232, 0, 0x22);
	}

	for (int i = 0; i < desc->widgetCount; i++) {
		const Widget *w = &g_table_windowWidgets[i];

		if (g_gameConfig.language == LANGUAGE_FRENCH) {
			GUI_DrawText_Wrapper(GUI_String_Get_ByIndex(desc->widgets[i].labelStringId), wi->xBase*8 + 40, w->offsetY + wi->yBase + 3, 232, 0, 0x22);
		}
		else {
			GUI_DrawText_Wrapper(GUI_String_Get_ByIndex(desc->widgets[i].labelStringId), w->offsetX + wi->xBase*8 - 10, w->offsetY + wi->yBase + 3, 232, 0, 0x222);
		}
	}
}

/**
 * Draw all widgets, starting with the one given by the parameters.
 * @param w First widget of the chain to draw.
 */
void GUI_Widget_DrawAll(Widget *w)
{
	while (w != NULL) {
		GUI_Widget_Draw(w);
		w = GUI_Widget_GetNext(w);
	}
}
