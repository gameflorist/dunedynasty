#ifndef NEWUI_VIEWPORT_H
#define NEWUI_VIEWPORT_H

#include "../gui/widget.h"
#include "../unit.h"

extern void Viewport_Target(Unit *u, enum ActionType action, uint16 packed);
extern void Viewport_Place(void);
extern void Viewport_DrawTiles(void);
extern void Viewport_DrawSelectedUnit(int x, int y);
extern void Viewport_DrawRallyPoint(void);
extern void Viewport_DrawSelectionBox(void);
extern bool Viewport_Click(Widget *w);
extern void Viewport_InterpolateMovement(const Unit *u, uint16 *x, uint16 *y);

#endif
