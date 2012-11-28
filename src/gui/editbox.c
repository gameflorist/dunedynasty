/** @file src/gui/editbox.c Editbox routines. */

#include <stdio.h>
#include "types.h"
#include "../os/sleep.h"

#include "font.h"
#include "gui.h"
#include "widget.h"
#include "../gfx.h"
#include "../input/input.h"
#include "../timer/timer.h"
#include "../video/video.h"

static unsigned char
GUI_EditBox_ScancodeToChar(enum Scancode key)
{
	const unsigned char char_10[] = "1234567890-";
	const unsigned char char_qp[] = "qwertyuiop";
	const unsigned char char_al[] = "asdfghjkl";
	const unsigned char char_zm[] = "zxcvbnm,./";
	unsigned char c = '\0';

	if (SCANCODE_1 <= key && key <= SCANCODE_MINUS)
		return char_10[key - SCANCODE_1];

	if (key == SCANCODE_SPACE)
		return ' ';

	if (SCANCODE_Q <= key && key <= SCANCODE_P)
		c = char_qp[key - SCANCODE_Q];

	if (SCANCODE_A <= key && key <= SCANCODE_L)
		c = char_al[key - SCANCODE_A];

	if (SCANCODE_Z <= key && key <= SCANCODE_SLASH)
		c = char_zm[key - SCANCODE_Z];

	if (('a' <= c && c <= 'z') && Input_Test(SCANCODE_LSHIFT))
		c += 'A' - 'a';

	return c;
}

/**
 * Draw a blinking cursor, used inside the EditBox.
 *
 * @param positionX Where to draw the cursor on the X position.
 * @param resetBlink If true, the blinking is reset and restarted.
 */
static void GUI_EditBox_BlinkCursor(uint16 positionX, bool resetBlink)
{
	static int64_t tickEditBox = 0;          /* Ticker for cursor blinking. */
	static bool   editBoxShowCursor = false; /* Cursor is active. */

	if (resetBlink) {
		tickEditBox = Timer_GetTicks() + 20;
		editBoxShowCursor = true;
	}
	else if (Timer_GetTicks() >= tickEditBox) {
		tickEditBox = Timer_GetTicks() + 20;
		editBoxShowCursor = !editBoxShowCursor;
	}

	/* resetBlink is always true when editting, and false when drawing. */
	if (!resetBlink) {
		Prim_FillRect_i(positionX, g_curWidgetYBase, positionX + Font_GetCharWidth('W'), g_curWidgetYBase + g_curWidgetHeight - 1, (editBoxShowCursor) ? g_curWidgetFGColourBlink : g_curWidgetFGColourNormal);
	}
}

/**
 * Show an EditBox and handles the input.
 * @param text The text to edit. Uses the pointer to make the modifications.
 * @param maxLength The maximum length of the text.
 * @param unknown1 Unknown.
 * @param w The widget this editbox is attached to.
 * @param tickProc The function to call every tick, for animation etc.
 * @param unknown4 Unknown.
 *
 * Return:
 * -1: cancel
 *  0: continue editting
 * 1+: other events (key/widgetID)
 */
int GUI_EditBox(char *text, uint16 maxLength, uint16 unknown1, Widget *w, uint16 (*tickProc)(void), uint16 unknown4)
{
	VARIABLE_NOT_USED(unknown1);

	uint16 positionX;
	uint16 maxWidth;
	uint16 textWidth;
	uint16 textLength;
	uint16 returnValue = 0x0;
	char *t;

	positionX = g_curWidgetXBase;

	textWidth = 0;
	textLength = 0;
	maxWidth = g_curWidgetWidth - Font_GetCharWidth('W') - 1;
	t = text;

	/* Calculate the length and width of the current string */
	for (; *t != '\0'; t++) {
		textWidth += Font_GetCharWidth(*t);
		textLength++;

		if (textWidth >= maxWidth) break;
	}
	*t = '\0';

	if ((unknown4 & 0x1) != 0) {
		unknown4 |= 0x4;
	}

#if 0
	if ((unknown4 & 0x4) != 0) Widget_PaintCurrentWidget();

	GUI_DrawText_Wrapper(text, positionX, g_curWidgetYBase, g_curWidgetFGColourBlink, g_curWidgetFGColourNormal, 0);
	GUI_EditBox_BlinkCursor(positionX + textWidth, false);
#endif

	while (Input_IsInputAvailable()) {
		uint16 keyWidth;
		uint16 key;

		if (tickProc != NULL) {
			returnValue = tickProc();
			if (returnValue != 0)
				return returnValue;
		}

		key = GUI_Widget_HandleEvents(w);

		if (key == 0x0) continue;

		if ((key & 0x8000) != 0)
			return key;

		if (key == 0x2B)
			return 0x2B;

		if (key == SCANCODE_ENTER) {
			*t = '\0';
			return SCANCODE_ENTER;
		}

		/* Handle backspace */
		if (key == SCANCODE_BACKSPACE) {
			if (textLength == 0) continue;

			textWidth -= Font_GetCharWidth(*(t - 1));
			textLength--;
			*(--t) = '\0';

			GUI_EditBox_BlinkCursor(positionX + textWidth, true);
			continue;
		}

		/* Names can't start with a space, and should be alpha-numeric */
		if (key == SCANCODE_SPACE && textLength == 0)
			continue;

		key = GUI_EditBox_ScancodeToChar(key);
		if (key == '\0')
			continue;

		keyWidth = Font_GetCharWidth(key);
		if (textWidth + keyWidth >= maxWidth || textLength >= maxLength) continue;

		/* Add char to the text */
		*t = key & 0xFF;
		*(++t) = '\0';
		textLength++;

		GUI_EditBox_BlinkCursor(positionX + textWidth, true);
	}

	return 0;
}

void
GUI_EditBox_Draw(const char *text)
{
	uint16 positionX = g_curWidgetXBase;
	uint16 textWidth = Font_GetStringWidth(text);

	Widget_PaintCurrentWidget();
	GUI_DrawText_Wrapper(text, positionX, g_curWidgetYBase, g_curWidgetFGColourBlink, g_curWidgetFGColourNormal, 0);
	GUI_EditBox_BlinkCursor(positionX + textWidth, false);
}
