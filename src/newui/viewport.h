#ifndef NEWUI_VIEWPORT_H
#define NEWUI_VIEWPORT_H

#include "../gui/widget.h"

extern void Viewport_DrawTiles(void);
extern void Viewport_DrawSelectedUnit(int x, int y);
extern void Viewport_DrawSelectionBox(void);
extern bool Viewport_Click(Widget *w);

#endif
