/* editbox.c */

#include <assert.h>
#include <string.h>

#include "editbox.h"

#include "../gui/font.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../input/input.h"
#include "../timer/timer.h"
#include "../video/video.h"

static int64_t s_tickEditBox = 0;
static bool s_editBoxShowCursor = false;

static void
EditBox_ResetBlink(void)
{
	s_tickEditBox = Timer_GetTicks() + 20;
	s_editBoxShowCursor = true;
}

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

/* Return values:
 * -1: cancel
 *  0: continue editting
 * 1+: other events (key/widgetID)
 */
int
EditBox_Input(char *text, int maxLength, enum EditBoxMode mode, uint16 key)
{
	if (key == 0
			|| (key & 0x8000) != 0
			|| key == SCANCODE_ESCAPE) {
		return key;
	}

	int textLength = strlen(text);
	char *t = text + textLength;

	if (key == SCANCODE_ENTER) {
		*t = '\0';
		return SCANCODE_ENTER;
	}

	/* Handle backspace. */
	if (key == SCANCODE_BACKSPACE) {
		if (textLength != 0) {
			*(--t) = '\0';
			EditBox_ResetBlink();
		}
		return 0;
	}

	if (textLength + 1 >= maxLength)
		return 0;

	/* Names can't start with a space, and should be alpha-numeric. */
	if (key == SCANCODE_SPACE) {
		if (textLength == 0
				|| mode == EDITBOX_ADDRESS
				|| mode == EDITBOX_PORT)
			return 0;
	}

	key = GUI_EditBox_ScancodeToChar(key);
	if (key == '\0')
		return 0;

	if (mode == EDITBOX_WIDTH_LIMITED) {
		const int maxWidth = g_curWidgetWidth - Font_GetCharWidth('W') - 1;
		const int textWidth = Font_GetStringWidth(text);
		const int keyWidth = Font_GetCharWidth(key);

		if (textWidth + keyWidth >= maxWidth)
			return 0;
	}
	else if (mode == EDITBOX_PORT) {
		if (!('0' <= key && key <= '9'))
			return 0;
	}

	/* Add char to the text */
	*t = key & 0xFF;
	*(++t) = '\0';

	EditBox_ResetBlink();
	return 0;
}

int
GUI_EditBox(char *text, int maxLength, Widget *w, enum EditBoxMode mode)
{
	while (Input_IsInputAvailable()) {
		uint16 key;

		key = GUI_Widget_HandleEvents(w);
		key = EditBox_Input(text, maxLength, mode, key);
		if (key != 0)
			return key;
	}

	return 0;
}

static void
EditBox_BlinkCursor(int x, int y, int w, int h, int fg)
{
	if (Timer_GetTicks() >= s_tickEditBox) {
		s_tickEditBox = Timer_GetTicks() + 20;
		s_editBoxShowCursor = !s_editBoxShowCursor;
	}

	if (s_editBoxShowCursor)
		Prim_FillRect_i(x, y, x + w, y + h - 1, fg);
}

void
EditBox_Draw(const char *text, int x, int y, int w, int h, int cursor_width,
		int col, int flags, bool draw_cursor)
{
	const ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];
	Video_SetClippingArea(div->scalex * (x + 1) + div->x, 0,
			div->scalex * w, TRUE_DISPLAY_HEIGHT);

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, flags);

	/* If small text, add a space between the last letter and the cursor. */
	const int cursor_space = (flags & 0x1) ? 1 : 0;

	const int text_width = Font_GetStringWidth(text);
	if ((flags & 0x100) && (text_width <= w)) {
		x += (w - text_width) / 2;
	}
	else if (text_width + cursor_space + cursor_width + 2 > w) {
		x += w - (text_width + cursor_space + cursor_width + 2);
	}

	flags &= ~0x100;
	GUI_DrawText_Wrapper("%s", x, y, col, 0, flags, text);

	if (draw_cursor) {
		EditBox_BlinkCursor(x + text_width + cursor_space, y, cursor_width, h, col);
	}

	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
}

void
EditBox_DrawCentred(Widget *w)
{
	EditBox_Draw(w->data,
			w->offsetX, w->offsetY, w->width, w->height, Font_GetCharWidth('W'),
			w->fgColourNormal, 0x122, w->state.selected);
}

void
EditBox_DrawWithBorder(Widget *w)
{
	Prim_DrawBorder(w->offsetX, w->offsetY, w->width, w->height,
			1, false, true, 4);

	EditBox_Draw(w->data,
			w->offsetX + 2, w->offsetY + 2, w->width - 4, w->height - 4, 4,
			w->fgColourNormal, 0x21, w->state.selected);
}

void
GUI_EditBox_Draw(const char *text)
{
	Widget_PaintCurrentWidget();

	EditBox_Draw(text,
			g_curWidgetXBase, g_curWidgetYBase,
			g_curWidgetWidth, g_curWidgetHeight, Font_GetCharWidth('W'),
			g_curWidgetFGColourBlink, 0x20, true);
}
