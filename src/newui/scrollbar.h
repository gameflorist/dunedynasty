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

extern Widget *GUI_Widget_Allocate_WithScrollbar(uint16 index, enum WindowID parentID, uint16 offsetX, uint16 offsetY, int16 width, int16 height, ScrollbarDrawProc *drawProc);
extern Widget *GUI_Widget_Allocate3(uint16 index, enum WindowID parentID, uint16 offsetX, uint16 offsetY, uint16 sprite1, uint16 sprite2, Widget *widget2, uint16 unknown1A);
extern void GUI_Widget_Scrollbar_Init(Widget *w, int16 scrollMax, int16 scrollPageSize, int16 scrollPosition);
extern void GUI_Widget_Free_WithScrollbar(Widget *w);

extern ScrollbarItem *Scrollbar_AllocItem(Widget *w, enum ScrollbarItemType type);
extern void Scrollbar_FreeItems(void);
extern ScrollbarItem *Scrollbar_GetSelectedItem(const Widget *w);
extern bool Scrollbar_ArrowUp_Click(Widget *w);
extern bool Scrollbar_ArrowDown_Click(Widget *w);
extern void Scrollbar_HandleEvent(Widget *w, int key);
extern bool Scrollbar_Click(Widget *w);

extern Widget *ScrollListArea_Allocate(Widget *scrollbar);

#endif
