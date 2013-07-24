/** @file src/gui/widget.c %Widget routines. */

#include <assert.h>
#include <stdlib.h>
#include "enum_string.h"
#include "../os/math.h"
#include "../gfx.h"

#include "widget.h"

#include "gui.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../house.h"
#include "../newui/scrollbar.h"
#include "../string.h"
#include "../sprites.h"
#include "../video/video.h"


uint8 g_paletteActive[3 * 256];
uint8 g_palette1[3 * 256];
uint8 g_palette2[3 * 256];
uint8 g_paletteMapping1[256];
uint8 g_paletteMapping2[256];

Widget *g_widgetLinkedListHead = NULL;
Widget *g_widgetLinkedListTail = NULL;
Widget *g_widgetInvoiceTail = NULL;
Widget *g_widgetMentatFirst = NULL;
Widget *g_widgetMentatTail = NULL;
Widget *g_widgetMentatScrollUp = NULL;
Widget *g_widgetMentatScrollDown = NULL;
Widget *g_widgetMentatScrollbar = NULL;

/** Layout and other properties of the widgets. */
WidgetProperties g_widgetProperties[WINDOWID_MAX] = {
	/*  x    y     w    h   p4  norm sel */
	{ 0*8,   0, 40*8, 200,  15,  12,  0}, /*  0 */
	{ 1*8,  75, 29*8,  70,  15,  15,  0}, /*  1: modal message */
	{ 0*8,  40, 30*8, 160,  15,  20,  0}, /*  2: viewport */
	{32*8, 136,  8*8,  64,  15,  12,  0}, /*  3: minimap */
	{32*8,  44,  8*8,   9,  29, 116,  0}, /*  4: credits label */
	{32*8,   4,  8*8,   9,  29, 116,  0}, /*  5: credits */
	{32*8,  42,  8*8,  82,  15,  20,  0}, /*  6: action panel */
	{ 1*8,  21, 38*8,  14,  12, 116,  0}, /*  7: status bar */
	{16*8,  48, 23*8, 112,  15, 233,  0}, /*  8: mentat picture */
	{ 2*8, 176, 36*8,  11,  15,  20,  0}, /*  9: mentat security prompt */
	{ 0*8,  40, 40*8, 160,  29,  20,  0}, /* 10 */
	{16*8,  48, 23*8, 112,  29,  20,  0}, /* 11 */
	{ 9*8,  80, 22*8, 112,  29, 116,  0}, /* 12 */
	{12*8, 140, 16*8,  42, 236, 233,  0}, /* 13: main menu frame */
	{ 2*8,  89, 36*8,  60,   0,   0,  0}, /* 14: save game frame */
	{ 4*8, 110, 32*8,  12, 232, 235,  0}, /* 15: save game edit box */
	{ 5*8,  48, 30*8, 134,   0,   0,  0}, /* 16: options menu */
	{ 3*8,  36, 36*8, 148,   0,   0,  0}, /* 17: save/load game */
	{ 1*8,  72, 38*8,  52,   0,   0,  0}, /* 18: yes/no dialog */
	{ 0*8,   0,  0*8,   0,   0,   0,  0}, /* 19: hall of fame */
	{ 2*8,  24, 36*8, 152,  12,  12,  0}, /* 20 */
	{ 1*8,   6, 12*8,   3,   0,  15,  6}, /* 21: main menu item */
	{ 0*8,   0, 1024,1024,   0,   0,  0}  /* 22: texture rendering pseudo-widget */
};

uint16 g_curWidgetIndex;          /*!< Index of the currently selected widget in #g_widgetProperties. */
uint16 g_curWidgetXBase;          /*!< Horizontal base position of the currently selected widget. */
uint16 g_curWidgetYBase;          /*!< Vertical base position of the currently selected widget. */
uint16 g_curWidgetWidth;          /*!< Width of the currently selected widget. */
uint16 g_curWidgetHeight;         /*!< Height of the currently selected widget. */
uint8  g_curWidgetFGColourBlink;  /*!< Blinking colour of the currently selected widget. */
uint8  g_curWidgetFGColourNormal; /*!< Normal colour of the currently selected widget. */

static bool s_widgetReset; /*!< If true, the widgets will be redrawn. */

Widget *GUI_Widget_GetNext(Widget *w)
{
	if (w->next == NULL) return NULL;
	return w->next;
}

/**
 * Find an existing Widget by the index number. It matches the first hit, and
 *  returns that widget to you.
 * @param w The first widget to start searching from.
 * @param index The index of the widget you are looking for.
 * @return The widget, or NULL if not found.
 */
Widget *GUI_Widget_Get_ByIndex(Widget *w, uint16 index)
{
	if (index == 0) return w;

	while (w != NULL) {
		if (w->index == index) return w;
		w = GUI_Widget_GetNext(w);
	}

	return NULL;
}

#if 0
static void GUI_Widget_DrawBlocked(Widget *w, uint8 colour);
#endif

/**
 * Make the widget invisible.
 * @param w The widget to make invisible.
 */
void GUI_Widget_MakeInvisible(Widget *w)
{
	if (w == NULL || w->flags.invisible) return;
	w->flags.invisible = true;
}

/**
 * Make the widget visible.
 * @param w The widget to make visible.
 */
void GUI_Widget_MakeVisible(Widget *w)
{
	if (w == NULL || !w->flags.invisible) return;
	w->flags.invisible = false;
}

/**
 * Draw a widget to the display.
 *
 * @param w The widget to draw.
 */
void GUI_Widget_Draw(Widget *w)
{
	uint16 positionLeft, positionRight;
	uint16 positionTop, positionBottom;
	uint16 offsetX, offsetY;
	uint16 drawMode;
	uint8 fgColour, bgColour;
	WidgetDrawParameter drawParam;

	if (w == NULL) return;

	if (w->flags.invisible) {
		if (!w->flags.greyWhenInvisible) return;

#if 0
		GUI_Widget_DrawBlocked(w, 12);
#endif
		return;
	}

	if (!w->state.hover2) {
		if (!w->state.selected) {
			drawMode  = w->drawModeNormal;
			drawParam = w->drawParameterNormal;
			fgColour  = w->fgColourNormal;
			bgColour  = w->bgColourNormal;
		} else {
			drawMode  = w->drawModeSelected;
			drawParam = w->drawParameterSelected;
			fgColour  = w->fgColourSelected;
			bgColour  = w->bgColourSelected;

		}
	} else {
		drawMode  = w->drawModeDown;
		drawParam = w->drawParameterDown;
		fgColour  = w->fgColourDown;
		bgColour  = w->bgColourDown;
	}

	offsetX = w->offsetX;
	if (w->offsetX < 0) {
		offsetX = g_widgetProperties[w->parentID].width + w->offsetX;
	}
	positionLeft = g_widgetProperties[w->parentID].xBase + g_screenDiv[w->div].x + offsetX;
	positionRight = positionLeft + w->width - 1;

	offsetY = w->offsetY;
	if (w->offsetY < 0) {
		offsetY = g_widgetProperties[w->parentID].height + w->offsetY;
	}
	positionTop = g_widgetProperties[w->parentID].yBase + g_screenDiv[w->div].y + offsetY;
	positionBottom = positionTop + w->height - 1;

	assert(drawMode < DRAW_MODE_MAX);
	if (drawMode != DRAW_MODE_NONE && drawMode != DRAW_MODE_CUSTOM_PROC && g_screenActiveID == SCREEN_0) {
		GUI_Mouse_Hide_InRegion(positionLeft, positionTop, positionRight, positionBottom);
	}

	switch (drawMode) {
		case DRAW_MODE_NONE: break;

		case DRAW_MODE_SPRITE: {
#if 0
			GUI_DrawSprite(g_screenActiveID, drawParam.sprite, positionLeft, positionTop, 0, 0x100, g_remap, 1);
#else
			Shape_DrawRemap(drawParam.sprite, g_playerHouseID, positionLeft, positionTop, 0, 0);
#endif
		} break;

		case DRAW_MODE_TEXT: {
			GUI_DrawText(drawParam.text, positionLeft, positionTop, fgColour, bgColour);
		} break;

		case DRAW_MODE_UNKNOWN3:
			Video_DrawIcon(drawParam.unknown, HOUSE_HARKONNEN, positionLeft, positionTop);
			break;

		case DRAW_MODE_CUSTOM_PROC: {
			if (drawParam.proc == NULL) return;
			drawParam.proc(w);
		} break;

		case DRAW_MODE_WIRED_RECTANGLE: {
			Prim_Rect_i(positionLeft, positionTop, positionRight, positionBottom, fgColour);
		} break;

		case DRAW_MODE_XORFILLED_RECTANGLE: {
#if 0
			GUI_DrawXorFilledRectangle(positionLeft, positionTop, positionRight, positionBottom, fgColour);
#endif
		} break;
	}

	if (drawMode != DRAW_MODE_NONE && drawMode != DRAW_MODE_CUSTOM_PROC && g_screenActiveID == SCREEN_0) {
		GUI_Mouse_Show_InRegion();
	}
}

/**
 * Check a widget for events like 'hover' or 'click'. Also check the keyboard
 *  buffer if there was any key which should active us.
 *
 * @param w The widget to handle events for. If the widget has a valid next
 *   pointer, those widgets are handled too.
  * @return The last key pressed, or 0 if the key pressed was handled (or if
 *   there was no key press).
 */
uint16 GUI_Widget_HandleEvents(Widget *w)
{
	static Widget *l_widget_selected     = NULL;
	static Widget *l_widget_last         = NULL;
	static uint16  l_widget_button_state = 0x0;

	uint16 buttonState;
	uint16 returnValue;
	enum Scancode key;

	/* Get the key from the buffer, if there was any key pressed */
	key = 0;
	if (Input_IsInputAvailable()) {
		key = Input_GetNextKey();
	}

	if (w == NULL) return key & 0x7FFF;

	/* First time this window is being drawn? */
	if (w != l_widget_last || s_widgetReset) {
		l_widget_last         = w;
		l_widget_selected     = NULL;
		l_widget_button_state = 0x0;
		s_widgetReset = false;

		/* Check for left click */
		if (Input_Test(MOUSE_LMB)) l_widget_button_state |= 0x0200;

		/* Check for right click */
		if (Input_Test(MOUSE_RMB)) l_widget_button_state |= 0x2000;

#if 0
		/* Draw all the widgets */
		for (; w != NULL; w = GUI_Widget_GetNext(w)) {
			GUI_Widget_Draw(w);
		}
#endif
	}

	buttonState = 0;
	/* if (g_var_7097 == 0) */
	{
		uint16 buttonStateChange = 0;

		/* See if the key was a mouse button action */
		if ((key & 0x8000) != 0) {
			/* if ((key & 0x00FF) == 0xC7) buttonStateChange = 0x1000; */
			/* if ((key & 0x00FF) == 0xC6) buttonStateChange = 0x0100; */
		} else {
			if ((key & 0x007F) == MOUSE_RMB) buttonStateChange = 0x1000;
			if ((key & 0x007F) == MOUSE_LMB) buttonStateChange = 0x0100;
		}

		/* Mouse button up */
		if ((key & SCANCODE_RELEASE) != 0) {
			buttonStateChange <<= 2;
		}

#if 0
		if (buttonStateChange != 0) {
			mouseX = g_mouseClickX;
			mouseY = g_mouseClickY;
		}
#endif

		/* Disable when release, enable when click */
		l_widget_button_state &= ~((buttonStateChange & 0x4400) >> 1);
		l_widget_button_state |=   (buttonStateChange & 0x1100) << 1;

		buttonState |= buttonStateChange;
		buttonState |= l_widget_button_state;
		buttonState |= (l_widget_button_state << 2) ^ 0x8800;
	}

	w = l_widget_last;
	if (l_widget_selected != NULL) {
		w = l_widget_selected;

		if (w->flags.invisible) {
			l_widget_selected = NULL;
		}
	}

	returnValue = 0;
	for (; w != NULL; w = GUI_Widget_GetNext(w)) {
		uint16 positionX, positionY;
		bool triggerWidgetHover;
		bool widgetHover;
		bool widgetClick;

		if (w->flags.invisible) continue;

		int mouseX, mouseY;
		Mouse_TransformToDiv(w->div, &mouseX, &mouseY);

		/* Store the previous button state */
		w->state.selectedLast = w->state.selected;
		w->state.hover1Last = w->state.hover1;

		positionX = w->offsetX;
		if (w->offsetX < 0) positionX += g_widgetProperties[w->parentID].width;
		positionX += g_widgetProperties[w->parentID].xBase;

		positionY = w->offsetY;
		if (w->offsetY < 0) positionY += g_widgetProperties[w->parentID].height;
		positionY += g_widgetProperties[w->parentID].yBase;

		widgetHover = false;
		w->state.keySelected = false;

		/* Check if the mouse is inside the widget */
		if (positionX <= mouseX && mouseX <= positionX + w->width && positionY <= mouseY && mouseY <= positionY + w->height) {
			widgetHover = true;
		}

		/* Check if there was a keypress for the widget */
		if ((key & 0x7F) != 0 && ((key & 0x7F) == w->shortcut || (key & 0x7F) == w->shortcut2) && !(key & SCANCODE_RELEASE)) {
			widgetHover = true;
			w->state.keySelected = true;
			key = 0;

			buttonState = 0;
			if ((key & 0x7F) == w->shortcut2) buttonState = (w->flags.buttonFilterRight) << 12;
			if (buttonState == 0) buttonState = (w->flags.buttonFilterLeft) << 8;

			l_widget_selected = w;
		}

		/* Update the hover state */
		w->state.hover1 = false;
		w->state.hover2 = false;
		if (widgetHover) {
			/* Button pressed, and click is hover */
			if ((buttonState & 0x3300) != 0 && w->flags.clickAsHover && (w == l_widget_selected || l_widget_selected == NULL)) {
				w->state.hover1 = true;
				w->state.hover2 = true;

				/* If we don't have a selected widget yet, this will be the one */
				if (l_widget_selected == NULL) {
					l_widget_selected = w;
				}
			}
			/* No button pressed, and click not is hover */
			if ((buttonState & 0x8800) != 0 && !w->flags.clickAsHover) {
				w->state.hover1 = true;
				w->state.hover2 = true;
			}
		}

		/* Check if we should trigger the hover activation */
		triggerWidgetHover = widgetHover;
		if (l_widget_selected != NULL && l_widget_selected->flags.loseSelect) {
			triggerWidgetHover = (l_widget_selected == w) ? true : false;
		}

		widgetClick = false;
		if (triggerWidgetHover) {
			uint8 buttonLeftFiltered;
			uint8 buttonRightFiltered;

			/* We click this widget for the first time */
			if ((buttonState & 0x1100) != 0 && l_widget_selected == NULL) {
				l_widget_selected = w;
				key = 0;
			}

			buttonLeftFiltered = (buttonState >> 8) & w->flags.buttonFilterLeft;
			buttonRightFiltered = (buttonState >> 12) & w->flags.buttonFilterRight;

			/* Check if we want to consider this as click */
			if ((buttonLeftFiltered != 0 || buttonRightFiltered != 0) && (widgetHover || !w->flags.requiresClick)) {

				if ((buttonLeftFiltered & 1) || (buttonRightFiltered & 1)) {
					/* Widget click */
					w->state.selected = !w->state.selected;
					returnValue = w->index | 0x8000;
					widgetClick = true;

					if (w->flags.clickAsHover) {
						w->state.hover1 = true;
						w->state.hover2 = true;
					}
					l_widget_selected = w;
				} else if ((buttonLeftFiltered & 2) || (buttonRightFiltered & 2)) {
					/* Widget was already clicked */
					if (!w->flags.clickAsHover) {
						w->state.hover1 = true;
						w->state.hover2 = true;
					}
					if (!w->flags.requiresClick) widgetClick = true;
				} else if ((buttonLeftFiltered & 4) || (buttonRightFiltered & 4)) {
					/* Widget release */
					if (!w->flags.requiresClick || (w->flags.requiresClick && w == l_widget_selected)) {
						w->state.selected = !w->state.selected;
						returnValue = w->index | 0x8000;
						widgetClick = true;
					}

					if (!w->flags.clickAsHover) {
						w->state.hover1 = false;
						w->state.hover2 = false;
					}
				} else {
					/* Widget was already released */
					if (w->flags.clickAsHover) {
						w->state.hover1 = true;
						w->state.hover2 = true;
					}
					if (!w->flags.requiresClick) widgetClick = true;
				}
			}
		}

		/* fakeClick = false; */
		/* Check if we are hovering and have mouse button down */
		if (widgetHover && (buttonState & 0x2200) != 0) {
			w->state.hover1 = true;
			w->state.hover2 = true;

			if (!w->flags.clickAsHover && !w->state.selected) {
				/* fakeClick = true; */
				w->state.selected = true;
			}
		}

		if ((!widgetHover) && (buttonState & 0x2200) != 0) {
			w->state.selected = false;
		}

		/* Check if we are not pressing a button */
		if ((buttonState & 0x8800) == 0x8800) {
			l_widget_selected = NULL;

			if (!widgetHover || w->flags.clickAsHover) {
				w->state.hover1 = false;
				w->state.hover2 = false;
			}
		}

		if (!widgetHover && l_widget_selected == w && !w->flags.loseSelect) {
			l_widget_selected = NULL;
		}

#if 0
		/* When the state changed, redraw */
		if (w->state.selected != w->state.selectedLast || w->state.hover1 != w->state.hover1Last) {
			GUI_Widget_Draw(w);
		}

		/* Reset click state when we were faking it */
		if (fakeClick) {
			w->state.selected = false;
		}
#endif

		/* XXX: Always click hovered object on mouse wheel event. */
		if (widgetHover && ((key & 0x7F) == MOUSE_ZAXIS)) {
			widgetClick = true;
		}

		if (widgetClick) {
			w->state.buttonState = buttonState >> 8;

			/* If Click was successful, don't handle any other widgets */
			if (w->clickProc != NULL && w->clickProc(w)) break;

			/* On click, don't handle any other widgets */
			if (w->flags.noClickCascade) break;
		}

		/* If we are selected and we lose selection on leave, don't try other widgets */
		if (w == l_widget_selected && w->flags.loseSelect) break;
	}

	if (returnValue != 0) return returnValue;
	return key & 0x7FFF;
}

/**
 * Get shortcut key for the given char.
 *
 * @param c The char to get the shortcut for.
 * @return The shortcut key.
 */
uint8 GUI_Widget_GetShortcut(uint8 c)
{
	const enum Scancode shortcuts[26] = {
		SCANCODE_A, SCANCODE_B, SCANCODE_C, SCANCODE_D, SCANCODE_E,
		SCANCODE_F, SCANCODE_G, SCANCODE_H, SCANCODE_I, SCANCODE_J,
		SCANCODE_K, SCANCODE_L, SCANCODE_M, SCANCODE_N, SCANCODE_O,
		SCANCODE_P, SCANCODE_Q, SCANCODE_R, SCANCODE_S, SCANCODE_T,
		SCANCODE_U, SCANCODE_V, SCANCODE_W, SCANCODE_X, SCANCODE_Y,
		SCANCODE_Z
	};

	if ('A' <= c && c <= 'Z') {
		return shortcuts[c - 'A'];
	}
	else if ('a' <= c && c <= 'z') {
		return shortcuts[c - 'a'];
	}
	else {
		return 0;
	}
}

void
GUI_Widget_SetShortcuts(Widget *w)
{
	const char *c = String_Get_ByIndex(w->stringID);

	if (c == NULL) {
		w->shortcut = 0;
		w->shortcut2 = 0;
		return;
	}

	w->shortcut = GUI_Widget_GetShortcut(c[0]);
	w->shortcut2 = 0;

	switch (w->stringID) {
		case STR_GUARD:
			/* S for stop. */
			w->shortcut2 = SCANCODE_S;
			break;
		case STR_HARVEST:
			/* H is used to go to construction yard. */
			w->shortcut = SCANCODE_A;
			break;
		case STR_EXIT_GAME:
			w->shortcut2 = SCANCODE_ESCAPE;
			break;
		case STR_CANCEL:
			w->shortcut2 = SCANCODE_ESCAPE;
			break;
		default:
			break;
	}

#if 0
	if (g_config.language == LANGUAGE_FRENCH) {
		if (buttons[i]->stringID == STR_MOVE) buttons[i]->shortcut2 = 0x27;
		if (buttons[i]->stringID == STR_RETURN) buttons[i]->shortcut2 = 0x13;
	}
	if (g_config.language == LANGUAGE_GERMAN) {
		if (buttons[i]->stringID == STR_GUARD) buttons[i]->shortcut2 = 0x17;
	}
#endif
}

/**
 * Allocates a widget.
 *
 * @param index The index for the allocated widget.
 * @param shortcut The shortcut for the allocated widget.
 * @param offsetX The x position for the allocated widget.
 * @param offsetY The y position for the allocated widget.
 * @param spriteID The sprite to draw on the allocated widget (0xFFFF for none).
 * @param stringID The string to print on the allocated widget.
 * @return The allocated widget.
 */
Widget *GUI_Widget_Allocate(uint16 index, uint16 shortcut, uint16 offsetX, uint16 offsetY, uint16 spriteID, uint16 stringID)
{
	Widget *w;
	uint8  drawMode;
	WidgetDrawParameter drawParam1;
	WidgetDrawParameter drawParam2;

	w = (Widget *)calloc(1, sizeof(Widget));

	w->index            = index;
	w->shortcut         = shortcut;
	w->shortcut2        = shortcut;
	w->parentID         = 0;
	w->fgColourSelected = 0xB;
	w->bgColourSelected = 0xC;
	w->fgColourNormal   = 0xF;
	w->bgColourNormal   = 0xC;
	w->stringID         = stringID;
	w->offsetX          = offsetX;
	w->offsetY          = offsetY;
	w->div              = SCREENDIV_MAIN;

	w->flags.requiresClick = true;
	w->flags.clickAsHover = true;
	w->flags.loseSelect = true;
	w->flags.buttonFilterLeft = 4;
	w->flags.buttonFilterRight = 4;

	switch ((int16)spriteID + 4) {
		case 0:
			drawMode        = DRAW_MODE_CUSTOM_PROC;
			drawParam1.proc = &GUI_Widget_SpriteButton_Draw;
			drawParam2.proc = &GUI_Widget_SpriteButton_Draw;
			break;

		case 1:
			drawMode        = DRAW_MODE_CUSTOM_PROC;
			drawParam1.proc = &GUI_Widget_SpriteTextButton_Draw;
			drawParam2.proc = &GUI_Widget_SpriteTextButton_Draw;

			if (stringID == STR_NULL) break;

			if (String_Get_ByIndex(stringID) != NULL) w->shortcut = GUI_Widget_GetShortcut(*String_Get_ByIndex(stringID));
			break;

		case 2:
			drawMode        = DRAW_MODE_CUSTOM_PROC;
			drawParam1.proc = &GUI_Widget_TextButton2_Draw;
			drawParam2.proc = &GUI_Widget_TextButton2_Draw;
			break;

		case 3:
			drawMode           = DRAW_MODE_NONE;
			drawParam1.unknown = 0;
			drawParam2.unknown = 0;
			break;

		default:
			drawMode = DRAW_MODE_SPRITE;
			drawParam1.sprite = spriteID;
			drawParam2.sprite = spriteID + 1;

			w->width  = Shape_Width(drawParam1.sprite);
			w->height = Shape_Height(drawParam1.sprite);
			break;
	}

	w->drawModeSelected = drawMode;
	w->drawModeDown     = drawMode;
	w->drawModeNormal   = drawMode;
	w->drawParameterNormal   = drawParam1;
	w->drawParameterDown     = drawParam2;
	w->drawParameterSelected = (spriteID == 0x19) ? drawParam2 : drawParam1;

	return w;
}

#if 0
/* Moved to newui/scrollbar.c. */
static uint16 GUI_Widget_Scrollbar_CalculateSize(WidgetScrollbar *scrollbar);
extern Widget *GUI_Widget_Allocate_WithScrollbar(uint16 index, uint16 parentID, uint16 offsetX, uint16 offsetY, int16 width, int16 height, ScrollbarDrawProc *drawProc);
extern Widget *GUI_Widget_Allocate3(uint16 index, uint16 parentID, uint16 offsetX, uint16 offsetY, uint16 sprite1, uint16 sprite2, Widget *widget2, uint16 unknown1A);
#endif

/**
 * Make the Widget selected.
 *
 * @param w The widget to make selected.
 * @param clickProc Wether to execute the widget clickProc.
 */
void GUI_Widget_MakeSelected(Widget *w, bool clickProc)
{
	if (w == NULL || w->flags.invisible) return;

	w->state.selectedLast = w->state.selected;

	w->state.selected = true;

	if (!clickProc || w->clickProc == NULL) return;

	w->clickProc(w);
}

/**
 * Reset the Widget to a normal state (not selected, not clicked).
 *
 * @param w The widget to reset.
 * @param clickProc Wether to execute the widget clickProc.
 */
void GUI_Widget_MakeNormal(Widget *w, bool clickProc)
{
	if (w == NULL || w->flags.invisible) return;

	w->state.selectedLast = w->state.selected;
	w->state.hover1Last = w->state.hover2;

	w->state.selected = false;
	w->state.hover1 = false;
	w->state.hover2 = false;;

	if (!clickProc || w->clickProc == NULL) return;

	w->clickProc(w);
}

/**
 * Link a widget to another widget, where the new widget is linked at the end
 *  of the list of the first widget.
 * @param w1 Widget to which the other widget is added.
 * @param w2 Widget which is added to the first widget (at the end of his chain).
 * @return The first widget of the chain.
 */
Widget *GUI_Widget_Link(Widget *w1, Widget *w2)
{
	Widget *first = w1;

	s_widgetReset = true;

	if (w2 == NULL) return w1;
	w2->next = NULL;
	if (w1 == NULL) return w2;

	while (w1->next != NULL) w1 = w1->next;

	w1->next = w2;
	return first;
}

#if 0
/* Moved to newui/scrollbar.c. */
extern uint16 GUI_Get_Scrollbar_Position(Widget *w);
extern void GUI_Widget_Scrollbar_Init(Widget *w, int16 scrollMax, int16 scrollPageSize, int16 scrollPosition);
extern uint16 GUI_Widget_Scrollbar_CalculatePosition(WidgetScrollbar *scrollbar);
extern uint16 GUI_Widget_Scrollbar_CalculateScrollPosition(WidgetScrollbar *scrollbar);
extern void GUI_Widget_Free_WithScrollbar(Widget *w);
#endif

/**
 * Insert a widget into a list of widgets.
 * @param w1 Widget to which the other widget is added.
 * @param w2 Widget which is added to the first widget (ordered by index).
 * @return The first widget of the chain.
 */
Widget *GUI_Widget_Insert(Widget *w1, Widget *w2)
{
	Widget *first;
	Widget *prev;

	if (w1 == NULL) return w2;
	if (w2 == NULL) return w1;

	if (w2->index <= w1->index) {
		w2->next = w1;
		return w2;
	}

	first = w1;
	prev = w1;

	while (w2->index > w1->index && w1->next != NULL) {
		prev = w1;
		w1 = w1->next;
	}

	if (w2->index > w1->index) {
		w1 = GUI_Widget_Link(first, w2);
	} else {
		prev->next = w2;
		w2->next = w1;
	}

	s_widgetReset = true;

	return first;
}

/**
 * Select a widget as current widget.
 * @param index %Widget number to select.
 * @return Index of the previous selected widget.
 */
uint16 Widget_SetCurrentWidget(uint16 index)
{
	uint16 oldIndex = g_curWidgetIndex;
	g_curWidgetIndex = index;

	g_curWidgetXBase          = g_widgetProperties[index].xBase;
	g_curWidgetYBase          = g_widgetProperties[index].yBase;
	g_curWidgetWidth          = g_widgetProperties[index].width;
	g_curWidgetHeight         = g_widgetProperties[index].height;
	g_curWidgetFGColourBlink  = g_widgetProperties[index].fgColourBlink;
	g_curWidgetFGColourNormal = g_widgetProperties[index].fgColourNormal;

	return oldIndex;
}

/**
 * Select a widget as current widget and draw its exterior.
 * @param index %Widget number to select.
 * @return Index of the previous selected widget.
 */
uint16 Widget_SetAndPaintCurrentWidget(uint16 index)
{
	index = Widget_SetCurrentWidget(index);

	Widget_PaintCurrentWidget();

	return index;
}

/**
 * Draw the exterior of the currently selected widget.
 */
void Widget_PaintCurrentWidget(void)
{
	Prim_FillRect_i(g_curWidgetXBase, g_curWidgetYBase, (g_curWidgetXBase + g_curWidgetWidth) - 1, g_curWidgetYBase + g_curWidgetHeight - 1, g_curWidgetFGColourNormal);
}
