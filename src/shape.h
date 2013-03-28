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
	SHAPE_MENTAT = 7,
	SHAPE_OPTIONS = 9,
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
	SHAPE_PALACE = 66,
	SHAPE_LIGHT_VEHICLE = 67,
	SHAPE_HEAVY_VEHICLE = 68,
	SHAPE_HIGH_TECH = 69,
	SHAPE_HOUSE_OF_IX = 70,
	SHAPE_WOR_TROOPER = 71,
	SHAPE_CONSTRUCTION_YARD = 72,
	SHAPE_WINDTRAP = 73,
	SHAPE_BARRACKS = 74,
	SHAPE_STARPORT = 75,
	SHAPE_REFINERY = 76,
	SHAPE_REPAIR = 77,
	SHAPE_WALL = 78,
	SHAPE_TURRET = 79,
	SHAPE_ROCKET_TURRET = 80,
	SHAPE_SILO = 81,
	SHAPE_OUTPOST = 82,
	SHAPE_LARGE_CONCRETE_SLAB = 83,

	SHAPE_SIEGE_TANK = 84,
	SHAPE_LAUNCHER = 85,
	SHAPE_QUAD = 86,
	SHAPE_DEVASTATOR = 87,
	SHAPE_TROOPER = 88,
	SHAPE_CARRYALL = 89,
	SHAPE_TANK = 90,
	SHAPE_SONIC_TANK = 91,
	SHAPE_TRIKE = 92,
	SHAPE_INFANTRY = 93,
	SHAPE_FREMEN_SQUAD = 94,
	SHAPE_SARDAUKAR = 95,
	SHAPE_SABOTEUR = 96,
	SHAPE_ORNITHOPTER = 97,
	SHAPE_DEVIATOR = 98,
	SHAPE_RAIDER_TRIKE = 99,
	SHAPE_HARVESTER = 100,
	SHAPE_MCV = 101,
	SHAPE_SOLDIER = 102,
	SHAPE_TROOPERS = 103,
	SHAPE_FREMEN = 104,
	SHAPE_SANDWORM = 105,

	SHAPE_BLUR_SMALL = 159,
	SHAPE_BLUR_MEDIUM = 160,
	SHAPE_BLUR_LARGE = 161,
	SHAPE_CHOAM_UP = 355,
	SHAPE_CHOAM_DOWN = 357,
	SHAPE_UPGRADE = 359,
	SHAPE_SEND_ORDER = 361, /* Replaced with "START GAME". */
	SHAPE_BUILD_THIS = 363,
	SHAPE_RESUME_GAME = 365, /* Replaced with "PREVIOUS". */
	SHAPE_CHOAM_PLUS = 367,
	SHAPE_CHOAM_MINUS = 369,
	SHAPE_INVOICE = 371,
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

	/* Actually only 8 visible arrows; shape 505 is a invisible 1x1 shape. */
	SHAPE_ARROW = 505,
	SHAPE_ARROW_FINAL = 513,

	/* 553 .. 598: SHAPE_ARROW .. SHAPE_ARROW_FINAL, 5 white masks each. */
	SHAPE_ARROW_TINT = 598 - 5 * 9,
	SHAPE_ARROW_TINT_FINAL = 598,

	/* 599 .. 639: SHAPE_CONCRETE_SLAB .. SHAPE_FREMEN greyed out. */
	SHAPE_CONCRETE_SLAB_GREY = 599,
	SHAPE_SANDWORM_GREY = 639,

	/* Final normal shape is 524 (CREDIT11.SHP). */
	SHAPE_MAX = 525,
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
