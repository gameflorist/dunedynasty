#ifndef NEWUI_STRATEGICMAP_H
#define NEWUI_STRATEGICMAP_H

#include <inttypes.h>
#include "../house.h"
#include "../shape.h"

enum {
	STRATEGIC_MAP_ARROW_ANIMATION_DELAY = 7,

	/* Note: region numbers start from 1. */
	STRATEGIC_MAP_MAX_REGIONS       = 27,
	STRATEGIC_MAP_MAX_PROGRESSION   = 20,
	STRATEGIC_MAP_MAX_ARROWS        = 4
};

enum StrategicMapState {
	STRATEGIC_MAP_SHOW_PLANET,
	STRATEGIC_MAP_SHOW_SURFACE,
	STRATEGIC_MAP_SHOW_DIVISION,
	STRATEGIC_MAP_SHOW_TEXT,
	STRATEGIC_MAP_SHOW_PROGRESSION,
	STRATEGIC_MAP_SELECT_REGION,
	STRATEGIC_MAP_BLINK_REGION,
	STRATEGIC_MAP_BLINK_END
};

typedef struct StrategicMapData {
	enum StrategicMapState state;
	enum HouseType owner[1 + STRATEGIC_MAP_MAX_REGIONS];
	bool fast_forward;

	const char *text1;
	const char *text2;
	int64_t text_timer;
	int curr_progression;

	struct {
		enum HouseType houseID;
		int region;
		char text[128];
	} progression[STRATEGIC_MAP_MAX_PROGRESSION];

	struct FadeInAux *region_aux;

	struct {
		int index;
		enum ShapeID shapeID;
		int x, y;
	} arrow[STRATEGIC_MAP_MAX_ARROWS];

	int arrow_frame;
	int64_t arrow_timer;

	int blink_scenario;
} StrategicMapData;

extern StrategicMapData g_strategic_map_state;
extern uint32 g_strategicRegionBits;

extern uint16 StrategicMap_CampaignChoiceToScenarioID(int campaignID, int nth);
extern void StrategicMap_Init(void);
extern void StrategicMap_AdvanceText(StrategicMapData *map, bool force);
extern void StrategicMap_Initialise(enum HouseType houseID, int campaignID, StrategicMapData *map);
extern void StrategicMap_Draw(enum HouseType houseID, StrategicMapData *map, int64_t fade_start);
extern bool StrategicMap_TimerLoop(StrategicMapData *map);
extern int StrategicMap_SelectRegion(const StrategicMapData *map, int x, int y);
extern bool StrategicMap_BlinkLoop(StrategicMapData *map, int64_t blink_start);

#endif
