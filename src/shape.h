#ifndef SHAPE_H
#define SHAPE_H

#include "gui/widget.h"
#include "house.h"

enum ShapeID {
	SHAPE_CURSOR_NORMAL = 0,
	SHAPE_CURSOR_UP = 1,
	SHAPE_CURSOR_RIGHT = 2,
	SHAPE_CURSOR_DOWN = 3,
	SHAPE_CURSOR_LEFT = 4,
	SHAPE_CURSOR_TARGET = 5,
	SHAPE_SELECTED_UNIT = 6,
	SHAPE_CREDITS_LABEL = 11,
	SHAPE_CREDITS_NUMBER_0 = 14,
	SHAPE_HEALTH_INDICATOR = 27,
	SHAPE_ATTACK = 28,
	SHAPE_MOVE = 29,
	SHAPE_DEATH_HAND = 30,
	SHAPE_CONCRETE_SLAB = 65,
	SHAPE_FREMEN_SQUAD = 94,
	SHAPE_SARDAUKAR = 95,
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
