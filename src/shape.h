#ifndef SHAPE_H
#define SHAPE_H

#include "gui/widget.h"
#include "house.h"

enum ShapeID {
	SHAPE_INVALID = 0xFFFF
};

extern int Shape_Width(enum ShapeID shapeID);
extern int Shape_Height(enum ShapeID shapeID);

extern void Shape_Draw(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags);
extern void Shape_DrawRemap(enum ShapeID shapeID, enum HouseType houseID, int x, int y, enum WindowID windowID, int flags);

#endif
