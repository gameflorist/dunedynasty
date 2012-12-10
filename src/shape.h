#ifndef SHAPE_H
#define SHAPE_H

#include "gui/widget.h"
#include "house.h"
#include "unit.h"

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
	SHAPE_STRUCTURE_LAYOUT_BLOCK = 24,
	SHAPE_RADIO_BUTTON_OFF = 25,
	SHAPE_RADIO_BUTTON_ON = 26,
	SHAPE_HEALTH_INDICATOR = 27,
	SHAPE_ATTACK = 28,
	SHAPE_MOVE = 29,
	SHAPE_DEATH_HAND = 30,
	SHAPE_SAVE_LOAD_SCROLL_UP = 59,
	SHAPE_SAVE_LOAD_SCROLL_UP_PRESSED = 60,
	SHAPE_SAVE_LOAD_SCROLL_DOWN = 61,
	SHAPE_SAVE_LOAD_SCROLL_DOWN_PRESSED = 62,
	SHAPE_STRUCTURE_LAYOUT_OUTLINE = 63,
	SHAPE_CONCRETE_SLAB = 65,
	SHAPE_FREMEN_SQUAD = 94,
	SHAPE_SARDAUKAR = 95,
	SHAPE_SABOTEUR = 96,
	SHAPE_FREMEN = 104,
	SHAPE_BLUR_SMALL = 159,
	SHAPE_BLUR_MEDIUM = 160,
	SHAPE_BLUR_LARGE = 161,
	SHAPE_YES = 373,
	SHAPE_NO = 375,
	SHAPE_EXIT = 377,
	SHAPE_PROCEED = 379,
	SHAPE_REPEAT = 381,
	SHAPE_SCROLLBAR_UP = 383,
	SHAPE_SCROLLBAR_UP_PRESSED = 384,
	SHAPE_SCROLLBAR_DOWN = 385,
	SHAPE_SCROLLBAR_DOWN_PRESSED = 386,
	SHAPE_MENTAT_EYES = 387,
	SHAPE_MENTAT_MOUTH = 392,
	SHAPE_MENTAT_SHOULDER = 397,
	SHAPE_MENTAT_ACCESSORY = 398,
	SHAPE_MAP_PIECE = 477,
	SHAPE_ARROW = 505,
	SHAPE_ARROW_FINAL = 513,

	/* Final normal shape is 524: CREDIT11.SHP. */

	/* 567 .. 599: SHAPE_ARROW .. SHAPE_ARROW_FINAL, 4 white masks each. */
	SHAPE_ARROW_TINT = 564,
	SHAPE_ARROW_TINT_FINAL = 599,

	/* 600 .. 639: SHAPE_CONCRETE_SLAB .. SHAPE_FREMEN greyed out. */
	SHAPE_CONCRETE_SLAB_GREY = 600,
	SHAPE_FREMEN_GREY = 639,

	SHAPE_INVALID = 0xFFFF
};

enum ShapeFlag {
	SHAPE_HFLIP     = 0x0001,
	SHAPE_VFLIP     = 0x0002,
	SHAPE_REMAP     = 0x0100, /* Use Shape_DrawRemap instead. */
	SHAPE_HIGHLIGHT = 0x0100,
	SHAPE_BLUR      = 0x0200,
	SHAPE_SHADOW    = 0x0300,
	SHAPE_OFFSET    = 0x4000, /* Add window xBase, yBase. */
	SHAPE_CENTRED   = 0x8000
};

extern int Shape_Width(enum ShapeID shapeID);
extern int Shape_Height(enum ShapeID shapeID);

extern void Shape_Draw(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags);
extern void Shape_DrawScale(enum ShapeID shapeID, int x, int y, int w, int h, enum WindowID windowID, int flags);
extern void Shape_DrawRemap(enum ShapeID shapeID, enum HouseType houseID, int x, int y, enum WindowID windowID, int flags);
extern void Shape_DrawRemapRotate(enum ShapeID shapeID, enum HouseType houseID, int x, int y, const dir24 *orient, enum WindowID windowID, int flags);
extern void Shape_DrawGrey(enum ShapeID shapeID, int x, int y, enum WindowID windowID, int flags);
extern void Shape_DrawGreyScale(enum ShapeID shapeID, int x, int y, int w, int h, enum WindowID windowID, int flags);
extern void Shape_DrawTint(enum ShapeID shapeID, int x, int y, unsigned char c, enum WindowID windowID, int flags);

#endif
