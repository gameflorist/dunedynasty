#ifndef INPUT_MOUSE_H
#define INPUT_MOUSE_H

#include <stdbool.h>
#include "scancode.h"

extern int g_mouseX;
extern int g_mouseY;
extern int g_mouseClickX;
extern int g_mouseClickY;

extern void Mouse_Init(void);
extern void Mouse_EventHandler(int x, int y, enum Scancode state);
extern bool Mouse_InRegion(int x1, int y1, int x2, int y2);

#endif
