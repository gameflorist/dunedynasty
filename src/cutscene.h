/** @file src/cutscene.h Introduction movie and cutscenes definitions. */

#ifndef CUTSCENE_H
#define CUTSCENE_H

#include "enumeration.h"

enum HouseAnimationType {
	HOUSEANIMATION_INTRO            = 0,
	HOUSEANIMATION_LEVEL4_HARKONNEN = 1,
	HOUSEANIMATION_LEVEL4_ATREIDES  = 2,
	HOUSEANIMATION_LEVEL4_ORDOS     = 3,
	HOUSEANIMATION_LEVEL8_HARKONNEN = 4,
	HOUSEANIMATION_LEVEL8_ATREIDES  = 5,
	HOUSEANIMATION_LEVEL8_ORDOS     = 6,
	HOUSEANIMATION_LEVEL9_HARKONNEN = 7,
	HOUSEANIMATION_LEVEL9_ATREIDES  = 8,
	HOUSEANIMATION_LEVEL9_ORDOS     = 9,

	HOUSEANIMATION_MAX
};

extern void Cutscene_PlayAnimation(enum HouseAnimationType anim);
extern void GameLoop_LevelEndAnimation(void);
extern void GameLoop_GameCredits(enum HouseType houseID);
extern void GameLoop_GameEndAnimation(void);
extern void GameLoop_GameIntroAnimation(void);

#endif
