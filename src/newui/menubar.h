#ifndef NEWUI_MENUBAR_H
#define NEWUI_MENUBAR_H

#include "../gui/widget.h"
#include "../house.h"

extern void MenuBar_DrawCredits(int credits_new, int credits_old, int offset);
extern void MenuBar_DrawStatusBar(const char *line1, const char *line2, uint8 fg1, uint8 fg2, int offset);
extern void MenuBar_Draw(enum HouseType houseID);

extern bool MenuBar_ClickOptions(Widget *w);
extern void MenuBar_TickOptionsOverlay(void);

#endif
