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

void
Prim_Rect_RGBA(float x1, float y1, float x2, float y2, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha, float thickness)
{
	al_draw_rectangle(x1, y1, x2, y2, al_map_rgba(r, g, b, alpha), thickness);
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
Prim_FillRect_RGBA(float x1, float y1, float x2, float y2,
		unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
{
	assert(x1 <= x2);
	assert(y1 <= y2);

	al_draw_filled_rectangle(x1, y1, x2, y2, al_map_rgba(r, g, b, alpha));
}

/*--------------------------------------------------------------*/

#if 0
void
Prim_FillTriangle(float x1, float y1, float x2, float y2, float x3, float y3, uint8 c)
{
	al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, paltoRGB[c]);
}
#endif

/*--------------------------------------------------------------*/

extern uint8 s_colourBorderSchema[5][4];

void
Prim_DrawBorder(float x, float y, float w, float h, int thickness, bool outline, bool fill, int colour_scheme)
{
	const int idx[60] = {
		/* Black outline. */
		0, 1, 2,
		0, 2, 3,

		/* Background. */
		4, 5, 6,
		4, 6, 7,

		/* Top, left borders. */
		8,  9, 10,
		8, 10, 11,
		8, 11, 12,
		8, 12, 13,

		/* Bottom, right borders. */
		14, 15, 16,
		14, 16, 17,
		14, 17, 18,
		14, 18, 19,

		/* Corners. */
		20, 21, 22,
		20, 22, 23,
		24, 25, 26,
		24, 26, 27,

		/* Corners, when thickness = 2. */
		28, 23, 29,
		28, 29, 30,
		31, 32, 33,
		31, 33, 25,
	};

	const uint8 *scheme = s_colourBorderSchema[colour_scheme];

	ALLEGRO_VERTEX vtx[34];
	float sz = thickness;
	int start = 0;
	int end = 60;

	memset(vtx, 0, sizeof(vtx));

	if (!fill) {
		start += 12;

		if (outline)
			Prim_Rect(x - 0.5f, y - 0.5f, x + w + 0.5f, y + h + 0.5f, 12, 1.0f);
	}
	else if (!outline) {
		start += 6;
	}
	else {
		/* Black outline. */
		for (int i = 0; i <= 3; i++)
			vtx[i].color = paltoRGB[12];

		vtx[0].x = x     - 1.0f, vtx[0].y = y - 1.0f;
		vtx[1].x = x + w + 1.0f, vtx[1].y = y - 1.0f;
		vtx[2].x = x + w + 1.0f, vtx[2].y = y + h + 1.0f;
		vtx[3].x = x     - 1.0f, vtx[3].y = y + h + 1.0f;
	}

	if (fill) {
		/* Background. */
		for (int i = 4; i <= 7; i++)
			vtx[i].color = paltoRGB[scheme[0]];

		vtx[4].x = x,       vtx[4].y = y;
		vtx[5].x = x + w,   vtx[5].y = y;
		vtx[6].x = x + w,   vtx[6].y = y + h;
		vtx[7].x = x,       vtx[7].y = y + h;
	}

	/* Top, left borders. */
	for (int i = 8; i <= 13; i++)
		vtx[i].color = paltoRGB[scheme[2]];

	vtx[ 8].x = x,          vtx[ 8].y = y;
	vtx[ 9].x = x + w,      vtx[ 9].y = y;
	vtx[10].x = x + w - sz, vtx[10].y = y + sz;
	vtx[11].x = x + sz,     vtx[11].y = y + sz;
	vtx[12].x = x + sz,     vtx[12].y = y + h - sz;
	vtx[13].x = x,          vtx[13].y = y + h;

	/* Bottom, right borders. */
	for (int i = 14; i <= 19; i++)
		vtx[i].color = paltoRGB[scheme[1]];

	vtx[14].x = x + w,      vtx[14].y = y + h;
	vtx[15].x = vtx[ 9].x,  vtx[15].y = vtx[ 9].y;
	vtx[16].x = vtx[10].x,  vtx[16].y = vtx[10].y;
	vtx[17].x = x + w - sz, vtx[17].y = y + h - sz;
	vtx[18].x = vtx[12].x,  vtx[18].y = vtx[12].y;
	vtx[19].x = vtx[13].x,  vtx[19].y = vtx[13].y;

	/* Corners. */
	vtx[20].x = x + w - 1.0f,   vtx[20].y = y;
	vtx[21].x = x + w,          vtx[21].y = y;
	vtx[22].x = x + w,          vtx[22].y = y + 1.0f;
	vtx[23].x = x + w - 1.0f,   vtx[23].y = y + 1.0f;

	vtx[24].x = x,          vtx[24].y = y + h - 1.0f;
	vtx[25].x = x + 1.0f,   vtx[25].y = y + h - 1.0f;
	vtx[26].x = x + 1.0f,   vtx[26].y = y + h;
	vtx[27].x = x,          vtx[27].y = y + h;

	if (thickness <= 1) {
		for (int i = 20; i <= 27; i++)
			vtx[i].color = paltoRGB[scheme[3]];

		end -= 12;
	}
	else {
		for (int i = 20; i <= 33; i++)
			vtx[i].color = paltoRGB[scheme[3]];

		vtx[28].x = x + w - 2.0f,   vtx[28].y = y + 1.0f;
		vtx[29].x = x + w - 1.0f,   vtx[29].y = y + 2.0f;
		vtx[30].x = x + w - 2.0f,   vtx[30].y = y + 2.0f;

		vtx[31].x = x + 1.0f,   vtx[31].y = y + h - 2.0f;
		vtx[32].x = x + 2.0f,   vtx[32].y = y + h - 2.0f;
		vtx[33].x = x + 2.0f,   vtx[33].y = y + h - 1.0f;
	}

	al_draw_indexed_prim(vtx, NULL, NULL, &idx[start], end - start, ALLEGRO_PRIM_TRIANGLE_LIST);
}
