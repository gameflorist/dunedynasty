/* shape.c */

#include <assert.h>

#include "shape.h"

#include "sprites.h"
#include "video/video.h"

#define Video_DrawShape             VideoA5_DrawShape
#define Video_DrawShapeScale        VideoA5_DrawShapeScale
#define Video_DrawShapeGrey         VideoA5_DrawShapeGrey
#define Video_DrawShapeGreyScale    VideoA5_DrawShapeGreyScale
#define Video_DrawShapeTint         VideoA5_DrawShapeTint

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
		x += g_widgetProperties[windowID].xBase;
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
	Video_DrawShape(shapeID, HOUSE_HARKONNEN, x, y, flags & 0x303);
}

void
Shape_DrawScale(enum ShapeID shapeID, int x, int y, int w, int h, enum WindowID windowID, int flags)
{
	assert(!(flags & 0x8000));

	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);
	Video_DrawShapeScale(shapeID, x, y, w, h, flags & 0x3);
}

void
Shape_DrawRemap(enum ShapeID shapeID, enum HouseType houseID, int x, int y, enum WindowID windowID, int flags)
{
	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);
	Video_DrawShape(shapeID, houseID, x, y, flags & 0x303);
}

void
Shape_DrawGrey(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags)
{
	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);
	Video_DrawShapeGrey(shapeID, x, y, flags & 0x3);
}

void
Shape_DrawGreyScale(enum ShapeID shapeID, int x, int y, int w, int h, enum WindowID windowID, int flags)
{
	assert(!(flags & 0x8000));

	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);
	Video_DrawShapeGreyScale(shapeID, x, y, w, h, flags & 0x3);
}

void
Shape_DrawTint(enum ShapeID shapeID, int x, int y, unsigned char c, enum WindowID windowID, int flags)
{
	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);
	Video_DrawShapeTint(shapeID, x, y, c, flags & 0x3);
}
