/* mouse.c */

#include <assert.h>
#include <stdlib.h>

#include "mouse.h"

#include "input.h"
#include "../config.h"
#include "../video/video.h"

int g_mouseX;
int g_mouseY;
int g_mouseDX;
int g_mouseDY;
int g_mouseDZ;
int g_mouseClickX;
int g_mouseClickY;
bool g_mousePanning;
bool g_mouseHidden;

void
Mouse_Init(void)
{
	g_mouseX = -16;
	g_mouseY = -16;
}

void
Mouse_SwitchHWCursor(void)
{
	if (g_gameConfig.hardwareCursor) {
		Video_ShowCursor();
	}
	else {
		Video_HideHWCursor();
	}
}

void
Mouse_EventHandler(bool apply_transform, int x, int y, int dz, enum Scancode state)
{
	if (apply_transform) {
		const ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];
		x = ((float)x - div->x) / div->scalex;
		y = ((float)y - div->y) / div->scaley;
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
		*mouseX = (g_mouseX - g_screenDiv[div].x) / g_screenDiv[div].scalex;

	if (mouseY != NULL)
		*mouseY = (g_mouseY - g_screenDiv[div].y) / g_screenDiv[div].scaley;
}

void
Mouse_TransformFromDiv(enum ScreenDivID div, int *mouseX, int *mouseY)
{
	if (mouseX != NULL)
		*mouseX = (g_screenDiv[div].scalex * g_mouseX) + g_screenDiv[div].x;

	if (mouseY != NULL)
		*mouseY = (g_screenDiv[div].scaley * g_mouseY) + g_screenDiv[div].y;
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
