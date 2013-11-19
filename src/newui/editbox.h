#ifndef NEWUI_EDITBOX_H
#define NEWUI_EDITBOX_H

#include "types.h"

enum EditBoxMode {
	EDITBOX_FREEFORM,
	EDITBOX_WIDTH_LIMITED,
	EDITBOX_ADDRESS,
	EDITBOX_PORT
};

struct Widget;

extern int EditBox_Input(char *text, int maxLength, enum EditBoxMode mode, uint16 key);
extern int GUI_EditBox(char *text, int maxLength, struct Widget *w, enum EditBoxMode mode);
extern void EditBox_Draw(const char *text, int x, int y, int w, int h, int cursor_width, int col, int flags, bool draw_cursor);
extern void EditBox_DrawCentred(struct Widget *w);
extern void EditBox_DrawWithBorder(struct Widget *w);
extern void GUI_EditBox_Draw(const char *text);

#endif
