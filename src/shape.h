#ifndef SHAPE_H
#define SHAPE_H

#include "gui/widget.h"
#include "house.h"

enum ShapeID {
	SHAPE_CONCRETE_SLAB = 65,
	SHAPE_FREMEN = 104,

	/* 600 .. 639: SHAPE_CONCRETE_SLAB .. SHAPE_FREMEN greyed out. */
	SHAPE_CONCRETE_SLAB_GREY = 600,
	SHAPE_FREMEN_GREY = 639,

	SHAPE_INVALID = 0xFFFF
};

extern int Shape_Width(enum ShapeID shapeID);
extern int Shape_Height(enum ShapeID shapeID);

extern void Shape_Draw(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags);
extern void Shape_DrawRemap(enum ShapeID shapeID, enum HouseType houseID, int x, int y, enum WindowID windowID, int flags);
extern void Shape_DrawGrey(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags);
extern void Shape_DrawTint(enum ShapeID shapeID, int x, int y, unsigned char c, enum WindowID windowID, int flags);

#endif
