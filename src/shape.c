/* shape.c
 *
 * Flags are:
 *  0x__01 = flip horizontally.
 *  0x__02 = flip vertically.
 *  0x__70 = blur effect frame.  New, originally just a static variable.
 *  0x__F0 = shadow effect darkness.
 *  0x01__ = remap for house.  Use Shape_DrawRemap instead.
 *  0x02__ = blur effect.
 *  0x03__ = shadow.
 *  0x10__ = ?
 *  0x20__ = remap (greymap?).  Use Shape_DrawGrey instead.
 *  0x40__ = position relative to window.
 *  0x80__ = centre sprite.
 */

#include <assert.h>
#include <stdlib.h>
#include "os/math.h"

#include "shape.h"

#include "enhancement.h"
#include "sprites.h"
#include "timer/timer.h"
#include "video/video.h"

#define Video_DrawShape             VideoA5_DrawShape
#define Video_DrawShapeRotate       VideoA5_DrawShapeRotate
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
	Video_DrawShape(shapeID, HOUSE_HARKONNEN, x, y, flags & 0x373);
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
	Video_DrawShape(shapeID, houseID, x, y, flags & 0x3F3);
}

void
Shape_DrawRemapRotate(enum ShapeID shapeID, enum HouseType houseID, int x, int y, const dir24 *orient, enum WindowID windowID, int flags)
{
	Shape_FixXY(shapeID, x, y, windowID, flags, &x, &y);

	const double frame = Timer_GetUnitRotationFrame();
	const int speed = orient->speed;

	/* Based on Unit_Rotate. */
	int diff = orient->target - orient->current;
	int orient256;

	if (diff > 128) diff -= 256;
	if (diff < -128) diff += 256;

	if (abs(speed) >= abs(diff)) {
		orient256 = orient->current + diff * frame;
	}
	else {
		orient256 = orient->current + speed * frame;
	}

	Video_DrawShapeRotate(shapeID, houseID, x, y, orient256, flags & 0x3F0);
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
