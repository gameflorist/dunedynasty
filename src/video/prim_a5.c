/* prim_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "prim.h"

extern ALLEGRO_COLOR paltoRGB[256];

/*--------------------------------------------------------------*/

void
Prim_Line(float x1, float y1, float x2, float y2, uint8 c, float thickness)
{
	al_draw_line(x1, y1, x2, y2, paltoRGB[c], thickness);
}

void
Prim_Hline(int x1, int y, int x2, uint8 c)
{
	assert(x1 <= x2);
	al_draw_line(x1, y + 0.5f, x2 + 0.99f, y + 0.5f, paltoRGB[c], 1.0f);
}

void
Prim_Vline(int x, int y1, int y2, uint8 c)
{
	assert(y1 <= y2);
	al_draw_line(x + 0.5f, y1, x + 0.5f, y2 + 0.99f, paltoRGB[c], 1.0f);
}

/*--------------------------------------------------------------*/

void
Prim_Rect(float x1, float y1, float x2, float y2, uint8 c, float thickness)
{
	al_draw_rectangle(x1, y1, x2, y2, paltoRGB[c], thickness);
}

void
Prim_Rect_i(int x1, int y1, int x2, int y2, uint8 c)
{
	assert(x1 <= x2);
	assert(y1 <= y2);

	al_draw_rectangle(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, paltoRGB[c], 1.0f);
}

/*--------------------------------------------------------------*/

void
Prim_FillRect(float x1, float y1, float x2, float y2, uint8 c)
{
	al_draw_filled_rectangle(x1, y1, x2, y2, paltoRGB[c]);
}

void
Prim_FillRect_i(int x1, int y1, int x2, int y2, uint8 c)
{
	assert(x1 <= x2);
	assert(y1 <= y2);

	al_draw_filled_rectangle(x1 + 0.01f, y1 + 0.01f, x2 + 0.99f, y2 + 0.99f, paltoRGB[c]);
}

void
Prim_FillRect_RGBA(int x1, int y1, int x2, int y2,
		unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
{
	assert(x1 <= x2);
	assert(y1 <= y2);

	al_draw_filled_rectangle(x1 + 0.01f, y1 + 0.01f, x2 + 0.99f, y2 + 0.99f, al_map_rgba(r, g, b, alpha));
}
