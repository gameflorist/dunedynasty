#ifndef NEWUI_SCROLLBAR_H
#define NEWUI_SCROLLBAR_H

#include "../gui/widget.h"

enum ScrollbarItemType {
	SCROLLBAR_CATEGORY,
	SCROLLBAR_ITEM,
};

typedef struct ScrollbarItem {
	char text[64];
	enum ScrollbarItemType type;
	uint32 offset;
	bool no_desc;
} ScrollbarItem;

extern void GUI_Widget_Scrollbar_Init(Widget *w, int16 scrollMax, int16 scrollPageSize, int16 scrollPosition);
extern void GUI_Widget_Free_WithScrollbar(Widget *w);

extern Widget *Scrollbar_Allocate(Widget *list, enum WindowID parentID, bool set_mentat_widgets);
extern ScrollbarItem *Scrollbar_AllocItem(Widget *w, enum ScrollbarItemType type);
extern void Scrollbar_FreeItems(void);
extern ScrollbarItem *Scrollbar_GetSelectedItem(const Widget *w);
extern bool Scrollbar_ArrowUp_Click(Widget *w);
extern bool Scrollbar_ArrowDown_Click(Widget *w);
extern void Scrollbar_HandleEvent(Widget *w, int key);
extern bool Scrollbar_Click(Widget *w);

#endif
