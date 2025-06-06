#ifndef NEWUI_VIEWPORT_H
#define NEWUI_VIEWPORT_H

#include "types.h"
#include "../gui/widget.h"
#include "../unit.h"

struct House;
struct Structure;

extern void Viewport_Init(void);
extern void Viewport_Hotkey(enum SquadID squad);
extern void Viewport_NextBuilding(void);
extern void Viewport_FocusOnStructure(const struct Structure *s);
extern void Viewport_Homekey(void);
extern void Viewport_DrawTiles(void);
extern void Viewport_DrawTileFog(void);
extern void Viewport_DrawSandworm(const Unit *u);
extern void Viewport_DrawUnit(const Unit *u, int windowX, int windowY, bool render_for_blur_effect);
extern void Viewport_DrawAirUnit(const Unit *u);
extern void Viewport_DrawRallyPoint(void);
extern void Viewport_DrawMovementIndicator(void);
extern void Viewport_DrawSelectionHealthBars(void);
extern void Viewport_DrawSelectionBox(void);
extern void Viewport_DrawPanCursor(void);
extern void Viewport_RenderBrush(int x, int y, int blurx);
extern void Viewport_Server_Place(struct House *h, struct Structure *s, uint16 packed);
extern bool Viewport_Click(Widget *w);

#endif
