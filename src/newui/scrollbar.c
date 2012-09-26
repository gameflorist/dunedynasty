/* scrollbar.c */

#include <assert.h>

#include "scrollbar.h"

#include "../input/mouse.h"

static void
Scrollbar_Scroll(WidgetScrollbar *scrollbar, uint16 scroll)
{
	scrollbar->scrollPosition += scroll;

	if ((int16)scrollbar->scrollPosition >= scrollbar->scrollMax - scrollbar->scrollPageSize) {
		scrollbar->scrollPosition = scrollbar->scrollMax - scrollbar->scrollPageSize;
	}

	if ((int16)scrollbar->scrollPosition <= 0) scrollbar->scrollPosition = 0;

	GUI_Widget_Scrollbar_CalculatePosition(scrollbar);

	GUI_Widget_Scrollbar_Draw(scrollbar->parent);
}

bool
Scrollbar_ArrowUp_Click(Widget *w)
{
	Scrollbar_Scroll(w->data, -1);

	return false;
}

bool
Scrollbar_ArrowDown_Click(Widget *w)
{
	Scrollbar_Scroll(w->data, 1);

	return false;
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
		GUI_Widget_Scrollbar_Draw(w);
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

		if (scrollbar->dirty != 0) GUI_Widget_Scrollbar_Draw(w);
	}

	return false;
}
