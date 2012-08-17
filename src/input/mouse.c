/* mouse.c */

#include <assert.h>

#include "mouse.h"

#include "input.h"

float g_mouse_transform_scale;
float g_mouse_transform_offx;
float g_mouse_transform_offy;
int g_mouseX;
int g_mouseY;
int g_mouseClickX;
int g_mouseClickY;

void
Mouse_Init(void)
{
	g_mouse_transform_scale = 1.0f;
	g_mouse_transform_offx = 0.0f;
	g_mouse_transform_offy = 0.0f;
}

void
Mouse_EventHandler(bool apply_transform, int x, int y, enum Scancode state)
{
	if (apply_transform) {
		x = ((float)x - g_mouse_transform_offx) / g_mouse_transform_scale;
		y = ((float)y - g_mouse_transform_offy) / g_mouse_transform_scale;
	}

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
