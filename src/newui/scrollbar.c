/* scrollbar.c */

#include <assert.h>
#include <stdlib.h>

#include "scrollbar.h"

#include "../gui/gui.h"
#include "../input/input.h"
#include "../input/mouse.h"

static ScrollbarItem *s_scrollbar_item;
static int s_scrollbar_max_items;
int s_selectedHelpSubject;

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

bool
Scrollbar_ArrowUp_Click(Widget *w)
{
	WidgetScrollbar *ws = w->data;

	if (s_selectedHelpSubject <= ws->scrollPosition)
		Scrollbar_Scroll(ws, -1);

	s_selectedHelpSubject--;
	Scrollbar_Clamp(ws);

	return false;
}

bool
Scrollbar_ArrowDown_Click(Widget *w)
{
	WidgetScrollbar *ws = w->data;

	if (s_selectedHelpSubject >= ws->scrollPosition + ws->scrollPageSize - 1)
		Scrollbar_Scroll(ws, 1);

	s_selectedHelpSubject++;
	Scrollbar_Clamp(ws);

	return false;
}

void
Scrollbar_HandleEvent(Widget *w, int key)
{
	WidgetScrollbar *ws = w->data;

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
			Scrollbar_ArrowUp_Click(w);
			break;

		case SCANCODE_KEYPAD_2: /* NUMPAD 2 / ARROW DOWN */
			Scrollbar_ArrowDown_Click(w);
			break;

		case SCANCODE_KEYPAD_9: /* NUMPAD 9 / PAGE UP */
			for (int i = 0; i < ws->scrollPageSize; i++)
				Scrollbar_ArrowUp_Click(w);
			break;

		case SCANCODE_KEYPAD_3: /* NUMPAD 3 / PAGE DOWN */
			for (int i = 0; i < ws->scrollPageSize; i++)
				Scrollbar_ArrowDown_Click(w);
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
