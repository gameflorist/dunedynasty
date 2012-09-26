/* scrollbar.c */

#include <assert.h>
#include <stdlib.h>

#include "scrollbar.h"

#include "../gui/gui.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../string.h"
#include "../table/strings.h"

static ScrollbarItem *s_scrollbar_item;
static int s_scrollbar_max_items;
static int s_selectedHelpSubject;

ScrollbarItem *
Scrollbar_AllocItem(Widget *w)
{
	WidgetScrollbar *ws = w->data;
	const int i = ws->scrollMax;

	if (s_scrollbar_max_items <= i + 1) {
		const int new_max = (s_scrollbar_max_items <= 0) ? 16 : 2 * s_scrollbar_max_items;

		s_scrollbar_item = realloc(s_scrollbar_item, new_max * sizeof(s_scrollbar_item[0]));
		s_scrollbar_max_items = new_max;
	}

	ws->scrollMax++;
	return &s_scrollbar_item[i];
}

void
Scrollbar_FreeItems(void)
{
	free(s_scrollbar_item);
	s_scrollbar_item = NULL;

	s_scrollbar_max_items = 0;
}

ScrollbarItem *
Scrollbar_GetSelectedItem(const Widget *w)
{
	const WidgetScrollbar *ws = w->data;
	assert(0 <= s_selectedHelpSubject && s_selectedHelpSubject < ws->scrollMax);

	return &s_scrollbar_item[s_selectedHelpSubject];
}

static void
Scrollbar_Scroll(WidgetScrollbar *scrollbar, uint16 scroll)
{
	scrollbar->scrollPosition += scroll;

	if ((int16)scrollbar->scrollPosition >= scrollbar->scrollMax - scrollbar->scrollPageSize) {
		scrollbar->scrollPosition = scrollbar->scrollMax - scrollbar->scrollPageSize;
	}

	if ((int16)scrollbar->scrollPosition <= 0) scrollbar->scrollPosition = 0;

	GUI_Widget_Scrollbar_CalculatePosition(scrollbar);
}

static void
Scrollbar_Clamp(const WidgetScrollbar *ws)
{
	if (s_selectedHelpSubject < ws->scrollPosition)
		s_selectedHelpSubject = ws->scrollPosition;

	if (s_selectedHelpSubject > ws->scrollPosition + ws->scrollPageSize - 1)
		s_selectedHelpSubject = ws->scrollPosition + ws->scrollPageSize - 1;
}

static void
Scrollbar_SelectUp(Widget *w)
{
	WidgetScrollbar *ws = w->data;

	if (s_selectedHelpSubject <= ws->scrollPosition)
		Scrollbar_Scroll(ws, -1);

	s_selectedHelpSubject--;
	Scrollbar_Clamp(ws);
}

static void
Scrollbar_SelectDown(Widget *w)
{
	WidgetScrollbar *ws = w->data;

	if (s_selectedHelpSubject >= ws->scrollPosition + ws->scrollPageSize - 1)
		Scrollbar_Scroll(ws, 1);

	s_selectedHelpSubject++;
	Scrollbar_Clamp(ws);
}

bool
Scrollbar_ArrowUp_Click(Widget *w)
{
	WidgetScrollbar *ws = w->data;

	Scrollbar_Scroll(ws, -1);
	Scrollbar_Clamp(ws);
	return false;
}

bool
Scrollbar_ArrowDown_Click(Widget *w)
{
	WidgetScrollbar *ws = w->data;

	Scrollbar_Scroll(ws, 1);
	Scrollbar_Clamp(ws);
	return false;
}

void
Scrollbar_HandleEvent(Widget *w, int key)
{
	WidgetScrollbar *ws = w->data;

	switch (key) {
		case 0x80 | MOUSE_ZAXIS:
			if (g_mouseDZ > 0) {
				Scrollbar_ArrowUp_Click(w);
			}
			else if (g_mouseDZ < 0) {
				Scrollbar_ArrowDown_Click(w);
			}
			break;

		case SCANCODE_KEYPAD_8: /* NUMPAD 8 / ARROW UP */
			Scrollbar_SelectUp(w);
			break;

		case SCANCODE_KEYPAD_2: /* NUMPAD 2 / ARROW DOWN */
			Scrollbar_SelectDown(w);
			break;

		case SCANCODE_KEYPAD_9: /* NUMPAD 9 / PAGE UP */
			for (int i = 0; i < ws->scrollPageSize; i++)
				Scrollbar_SelectUp(w);
			break;

		case SCANCODE_KEYPAD_3: /* NUMPAD 3 / PAGE DOWN */
			for (int i = 0; i < ws->scrollPageSize; i++)
				Scrollbar_SelectDown(w);
			break;
	}
}

bool
Scrollbar_Click(Widget *w)
{
	WidgetScrollbar *scrollbar;
	uint16 positionX, positionY;

	scrollbar = w->data;

	positionX = w->offsetX;
	if (w->offsetX < 0) positionX += g_widgetProperties[w->parentID].width;
	positionX += g_widgetProperties[w->parentID].xBase;

	positionY = w->offsetY;
	if (w->offsetY < 0) positionY += g_widgetProperties[w->parentID].height;
	positionY += g_widgetProperties[w->parentID].yBase;

	if ((w->state.s.buttonState & 0x44) != 0) {
		scrollbar->pressed = 0;
	}

	if ((w->state.s.buttonState & 0x11) != 0) {
		int16 positionCurrent;
		int16 positionBegin;
		int16 positionEnd;

		scrollbar->pressed = 0;

		if (w->width > w->height) {
			positionCurrent = g_mouseX;
			positionBegin = positionX + scrollbar->position + 1;
		} else {
			positionCurrent = g_mouseY;
			positionBegin = positionY + scrollbar->position + 1;
		}

		positionEnd = positionBegin + scrollbar->size;

		if (positionCurrent <= positionEnd && positionCurrent >= positionBegin) {
			scrollbar->pressed = 1;
			scrollbar->pressedPosition = positionCurrent - positionBegin;
		} else {
			Scrollbar_Scroll(scrollbar, (positionCurrent < positionBegin ? -scrollbar->scrollPageSize : scrollbar->scrollPageSize));
			Scrollbar_Clamp(scrollbar);
		}
	}

	if ((w->state.s.buttonState & 0x22) != 0 && scrollbar->pressed != 0) {
		int16 position, size;

		if (w->width > w->height) {
			size = w->width - 2 - scrollbar->size;
			position = g_mouseX - scrollbar->pressedPosition - positionX - 1;
		} else {
			size = w->height - 2 - scrollbar->size;
			position = g_mouseY - scrollbar->pressedPosition - positionY - 1;
		}

		if (position < 0) {
			position = 0;
		} else if (position > size) {
			position = size;
		}

		if (scrollbar->position != position) {
			scrollbar->position = position;
			scrollbar->dirty = 1;
		}

		GUI_Widget_Scrollbar_CalculateScrollPosition(scrollbar);
		Scrollbar_Clamp(scrollbar);
	}

	return false;
}

void
Scrollbar_DrawItems(Widget *w)
{
	const WidgetScrollbar *ws = w->data;

	for (int i = 0; i < ws->scrollPageSize; i++) {
		const int n = ws->scrollPosition + i;

		if (!(0 <= n && n < ws->scrollMax))
			break;

		const ScrollbarItem *si = &s_scrollbar_item[n];
		const int y = g_widgetProperties[w->parentID].yBase + 16 + 8 * i;
		int x = g_widgetProperties[w->parentID].xBase;
		uint8 colour;

		if (si->is_category) {
			x += 16;
			colour = 11;
		}
		else {
			x += 24;
			colour = (n == s_selectedHelpSubject) ? 8 : 15;
		}

		GUI_DrawText_Wrapper(si->text, x, y, colour, 0, 0x11);
	}
}

static bool
ScrollListArea_Click(Widget *w)
{
	const WidgetProperties *wi = &g_widgetProperties[w->parentID];
	WidgetScrollbar *ws = w->data;

	if (wi->yBase + w->offsetY <= g_mouseY && g_mouseY < wi->yBase + w->offsetY + w->height) {
		const int y = (g_mouseY - w->offsetY - wi->yBase) / 8;

		s_selectedHelpSubject = ws->scrollPosition + y;
	}

	if ((w->state.s.buttonState & 0x11) == 0) return true;

	return false;
}

Widget *
ScrollListArea_Allocate(Widget *scrollbar)
{
	Widget *w = calloc(1, sizeof(Widget));

	w->index = 3;

	w->flags.all = 0;
	w->flags.s.buttonFilterLeft = 9;
	w->flags.s.buttonFilterRight = 1;

	w->clickProc = &ScrollListArea_Click;

	w->drawParameterNormal.text = String_Get_ByIndex(STR_NULL);
	w->drawParameterSelected.text = w->drawParameterNormal.text;
	w->drawParameterDown.text = w->drawParameterNormal.text;
	w->drawModeNormal = DRAW_MODE_TEXT;

	w->state.all = 0;

	w->offsetX = 24;
	w->offsetY = 16;
	w->width = 0x88;
	w->height = 8 * 11;
	w->parentID = WINDOWID_MENTAT_PICTURE;

	w->data = scrollbar->data;

	return w;
}
