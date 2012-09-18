#ifndef INPUT_MOUSE_H
#define INPUT_MOUSE_H

#include <stdbool.h>
#include "scancode.h"
#include "../gfx.h"

extern float g_mouse_transform_scale;
extern float g_mouse_transform_offx;
extern float g_mouse_transform_offy;
extern int g_mouseX;
extern int g_mouseY;
extern int g_mouseDZ;
extern int g_mouseClickX;
extern int g_mouseClickY;

extern void Mouse_Init(void);
extern void Mouse_TransformToDiv(enum ScreenDivID div, int *mouseX, int *mouseY);
extern void Mouse_EventHandler(bool apply_transform, int x, int y, int dz, enum Scancode state);
extern bool Mouse_InRegion(int x1, int y1, int x2, int y2);

#endif
