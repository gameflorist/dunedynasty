/* shape.c */

#include <assert.h>

#include "shape.h"

#include "sprites.h"
#include "video/video.h"

int
Shape_Width(enum ShapeID shapeID)
{
	if (shapeID == SHAPE_INVALID)
		return 0;

	return g_sprites[shapeID][3];
}

int
Shape_Height(enum ShapeID shapeID)
{
	if (shapeID == SHAPE_INVALID)
		return 0;

	return g_sprites[shapeID][2];
}

static void
Shape_FixXY(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags, int *retx, int *rety)
{
	if (flags & 0x4000) {
		x += g_widgetProperties[windowID].xBase*8;
		y += g_widgetProperties[windowID].yBase;
	}

	if (flags & 0x8000) {
		x -= Shape_Width(shapeID) / 2;
		y -= Shape_Height(shapeID) / 2;
	}

	*retx = x;
	*rety = y;
}

void
Shape_Draw(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags)
{
	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);
	Video_DrawShape(shapeID, HOUSE_HARKONNEN, x, y, flags & 0x3);
}

void
Shape_DrawRemap(enum ShapeID shapeID, enum HouseType houseID, int x, int y, enum WindowID windowID, int flags)
{
	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);
	Video_DrawShape(shapeID, houseID, x, y, flags & 0x3);
}

void
Shape_DrawGrey(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags)
{
	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);
	Video_DrawShapeGrey(shapeID, x, y, flags & 0x3);
}
