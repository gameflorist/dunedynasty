/* mouse.c */

#include <assert.h>

#include "mouse.h"

#include "input.h"

int g_mouseX;
int g_mouseY;
int g_mouseClickX;
int g_mouseClickY;

void
Mouse_Init(void)
{
}

void
Mouse_EventHandler(int x, int y, enum Scancode state)
{
	g_mouseX = x;
	g_mouseY = y;

	if (state == 0)
		return;

	g_mouseClickX = x;
	g_mouseClickY = y;

	Input_EventHandler(state);
}

bool
Mouse_InRegion(int x1, int y1, int x2, int y2)
{
	return (x1 <= g_mouseX && g_mouseX <= x2) &&
	       (y1 <= g_mouseY && g_mouseY <= y2);
}
