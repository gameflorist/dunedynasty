#ifndef MODS_SKIRMISH_H
#define MODS_SKIRMISH_H

#include "enumeration.h"
#include "landscape.h"
#include "mapgenerator.h"

typedef struct Skirmish {
	/* Initial credits. */
	uint16 credits;

	uint32 seed;
	LandscapeGeneratorParams landscape_params;

	PlayerConfig player_config[HOUSE_MAX];

	enum MapWormCount worm_count;
	
} Skirmish;

struct SkirmishData;

extern Skirmish g_skirmish;

extern uint16 Skirmish_FindStartLocation(enum HouseType houseID, uint16 dist_threshold, struct SkirmishData *sd);

extern void Skirmish_Initialise(void);
extern bool Skirmish_IsPlayable(void);
extern void Skirmish_Prepare(void);
extern void Skirmish_StartScenario(void);
extern bool Skirmish_GenerateMap1(bool is_playable);
extern bool Skirmish_GenerateMap(enum MapGeneratorMode mode);
extern bool Skirmish_GenHouses(struct SkirmishData *sd);
void Skirmish_GenUnitsAI(enum HouseType houseID);

#endif
