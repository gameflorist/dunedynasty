#ifndef NEWUI_STRATEGICMAP_H
#define NEWUI_STRATEGICMAP_H

#include "../house.h"

enum {
	/* Note: region numbers start from 1. */
	STRATEGIC_MAP_MAX_REGIONS       = 27,
	STRATEGIC_MAP_MAX_PROGRESSION   = 20
};

typedef struct StrategicMapData {
	enum HouseType owner[1 + STRATEGIC_MAP_MAX_REGIONS];

	struct {
		enum HouseType houseID;
		int region;
		char text[128];
	} progression[STRATEGIC_MAP_MAX_PROGRESSION];
} StrategicMapData;

extern StrategicMapData g_strategic_map;

extern void StrategicMap_Init(void);
extern void StrategicMap_DrawBackground(enum HouseType houseID);
extern void StrategicMap_DrawRegions(const StrategicMapData *map);
extern void StrategicMap_ReadOwnership(int campaignID, StrategicMapData *map);
extern void StrategicMap_ReadProgression(enum HouseType houseID, int campaignID, StrategicMapData *map);

#endif
