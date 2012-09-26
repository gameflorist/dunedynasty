#ifndef NEWUI_SCROLLBAR_H
#define NEWUI_SCROLLBAR_H

#include "../gui/widget.h"

typedef struct ScrollbarItem {
	char text[128];
	uint32 offset;
	bool no_desc;
	bool is_category;
} ScrollbarItem;

extern ScrollbarItem *Scrollbar_AllocItem(Widget *w);
extern void Scrollbar_FreeItems(void);
extern ScrollbarItem *Scrollbar_GetSelectedItem(const Widget *w);
extern bool Scrollbar_ArrowUp_Click(Widget *w);
extern bool Scrollbar_ArrowDown_Click(Widget *w);
extern void Scrollbar_HandleEvent(Widget *w, int key);
extern bool Scrollbar_Click(Widget *w);
extern void Scrollbar_DrawItems(Widget *w);

extern Widget *ScrollListArea_Allocate(Widget *scrollbar);

#endif
