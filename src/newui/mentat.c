/* mentat.c */

#include <assert.h>
#include <stdlib.h>

#include "mentat.h"

#include "../input/input.h"
#include "../shape.h"
#include "../video/video.h"

static const struct {
	int eyesX, eyesY;
	int mouthX, mouthY;
	int shoulderX, shoulderY;
	int accessoryX, accessoryY;
} mentat_data[HOUSE_MAX] = {
	{ 0x20,0x58, 0x20,0x68, 0x80,0x68, 0x00,0x00 },
	{ 0x28,0x50, 0x28,0x60, 0x80,0x80, 0x48,0x98 },
	{ 0x10,0x50, 0x10,0x60, 0x80,0x80, 0x58,0x90 },
	{ 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00 },
	{ 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00 },
	{ 0x40,0x50, 0x38,0x60, 0x00,0x00, 0x00,0x00 },
};

static int movingEyesSprite;
static int movingMouthSprite;
static int otherSprite;

void
Mentat_DrawBackground(enum HouseType houseID)
{
	const char *mentat_background[HOUSE_MAX] = {
		"MENTATH.CPS", "MENTATA.CPS", "MENTATO.CPS",
		"MENTATM.CPS", "MENTATM.CPS", "MENTATM.CPS"
	};
	assert(houseID < HOUSE_MAX);

	Video_DrawCPS(mentat_background[houseID]);
}

static void
Mentat_DrawEyes(enum HouseType houseID)
{
	const enum ShapeID shapeID = SHAPE_MENTAT_EYES + houseID * 15 + movingEyesSprite;

	Shape_Draw(shapeID, mentat_data[houseID].eyesX, mentat_data[houseID].eyesY, 0, 0);
}

static void
Mentat_DrawMouth(enum HouseType houseID)
{
	const enum ShapeID shapeID = SHAPE_MENTAT_MOUTH + houseID * 15 + movingMouthSprite;

	Shape_Draw(shapeID, mentat_data[houseID].mouthX, mentat_data[houseID].mouthY, 0, 0);
}

static void
Mentat_DrawShoulder(enum HouseType houseID)
{
	if (houseID <= HOUSE_ORDOS) {
		const enum ShapeID shapeID = SHAPE_MENTAT_SHOULDER + houseID * 15;

		Shape_Draw(shapeID, mentat_data[houseID].shoulderX, mentat_data[houseID].shoulderY, 0, 0);
	}
}

static void
Mentat_DrawAccessory(enum HouseType houseID)
{
	if (houseID == HOUSE_ATREIDES || houseID == HOUSE_ORDOS) {
		const enum ShapeID shapeID = SHAPE_MENTAT_ACCESSORY + houseID * 15 + abs(otherSprite);

		Shape_Draw(shapeID, mentat_data[houseID].accessoryX, mentat_data[houseID].accessoryY, 0, 0);
	}
}

void
Mentat_Draw(enum HouseType houseID)
{
	assert(houseID < HOUSE_MAX);

	Mentat_DrawEyes(houseID);
	Mentat_DrawMouth(houseID);
	Mentat_DrawShoulder(houseID);
	Mentat_DrawAccessory(houseID);
}
