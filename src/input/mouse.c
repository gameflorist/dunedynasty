/* mouse.c */

#include <assert.h>
#include <stdlib.h>

#include "mouse.h"

#include "input.h"

float g_mouse_transform_scale;
float g_mouse_transform_offx;
float g_mouse_transform_offy;
int g_mouseX;
int g_mouseY;
int g_mouseDZ;
int g_mouseClickX;
int g_mouseClickY;
bool g_warpMouse;

void
Mouse_Init(void)
{
	g_mouse_transform_scale = 1.0f;
	g_mouse_transform_offx = 0.0f;
	g_mouse_transform_offy = 0.0f;
}

void
Mouse_EventHandler(bool apply_transform, int x, int y, int dz, enum Scancode state)
{
	if (apply_transform) {
		x = ((float)x - g_mouse_transform_offx) / g_mouse_transform_scale;
		y = ((float)y - g_mouse_transform_offy) / g_mouse_transform_scale;
	}

	g_mouseX = x;
	g_mouseY = y;
	g_mouseDZ = dz;

	if (dz != 0)
		Input_EventHandler(SCANCODE_RELEASE | MOUSE_ZAXIS);

	if (state == 0)
		return;

	g_mouseClickX = x;
	g_mouseClickY = y;

	Input_EventHandler(state);
}

void
Mouse_TransformToDiv(enum ScreenDivID div, int *mouseX, int *mouseY)
{
	if (mouseX != NULL)
		*mouseX = (g_mouseX - g_screenDiv[div].x) / g_screenDiv[div].scale;

	if (mouseY != NULL)
		*mouseY = (g_mouseY - g_screenDiv[div].y) / g_screenDiv[div].scale;
}

bool
Mouse_InRegion(int x1, int y1, int x2, int y2)
{
	return (x1 <= g_mouseX && g_mouseX <= x2) &&
	       (y1 <= g_mouseY && g_mouseY <= y2);
}

bool
Mouse_InRegion_Div(enum ScreenDivID div, int x1, int y1, int x2, int y2)
{
	int mouseX, mouseY;
	Mouse_TransformToDiv(div, &mouseX, &mouseY);

	return (x1 <= mouseX && mouseX <= x2) &&
	       (y1 <= mouseY && mouseY <= y2);
}
