#ifndef NEWUI_MENUBAR_H
#define NEWUI_MENUBAR_H

#include "enum_string.h"
#include "enumeration.h"
#include "../shape.h"

struct Widget;

extern void MenuBar_DrawCredits(int credits_new, int credits_old, int offset, int x0);
extern void MenuBar_DrawStatusBar(const char *line1, const char *line2, bool scrollInProgress, int x, int y, int offset);
extern void MenuBar_Draw(enum HouseType houseID);
extern void MenuBar_StartRadarAnimation(bool activate);

extern void GUI_DisplayHint(enum HouseType houseID, enum StringID stringID, enum ShapeID shapeID);
extern void MenuBar_TickHintOverlay(void);
extern void MenuBar_DrawHintOverlay(void);

extern bool MenuBar_ClickMentat(struct Widget *w);
extern void MenuBar_TickMentatOverlay(void);
extern void MenuBar_DrawMentatOverlay(void);

extern bool MenuBar_ClickOptions(struct Widget *w);
extern void MenuBar_TickOptionsOverlay(void);
extern void MenuBar_DrawOptionsOverlay(void);

extern uint16 GUI_DisplayModalMessage(const char *str, uint16 shapeID, ...);

#endif
