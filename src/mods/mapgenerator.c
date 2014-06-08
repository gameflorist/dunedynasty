/* mapgenerator.c */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "mapgenerator.h"

#include "../map.h"
#include "../pool/pool_house.h"
#include "../pool/pool_structure.h"
#include "../pool/pool_team.h"
#include "../pool/pool_unit.h"

static struct {
	Tile map[MAP_SIZE_MAX * MAP_SIZE_MAX];
	uint16 mapSpriteID[MAP_SIZE_MAX * MAP_SIZE_MAX];
	struct HousePool *house_pool;
	struct StructurePool *structure_pool;
	struct TeamPool *team_pool;
	struct UnitPool *unit_pool;
} s_world_state;
assert_compile(sizeof(s_world_state.map) == sizeof(g_map));
assert_compile(sizeof(s_world_state.mapSpriteID) == sizeof(g_mapSpriteID));

/*--------------------------------------------------------------*/

enum MapGeneratorMode
MapGenerator_TransitionState(enum MapGeneratorMode mode, bool success)
{
	if (success) {
		return MAP_GENERATOR_STOP;
	}

	switch (mode) {
		case MAP_GENERATOR_TRY_TEST_ELSE_RAND:
		case MAP_GENERATOR_TRY_RAND_ELSE_RAND:
			return MAP_GENERATOR_TRY_RAND_ELSE_RAND;

		case MAP_GENERATOR_STOP:
		case MAP_GENERATOR_TRY_TEST_ELSE_STOP:
		case MAP_GENERATOR_TRY_RAND_ELSE_STOP:
		case MAP_GENERATOR_FINAL:
		default:
			return MAP_GENERATOR_STOP;
	}
}

uint32
MapGenerator_PickRandomSeed(void)
{
	/* DuneMaps only supports 15 bit maps seeds, so there. */
	return rand() & 0x7FFF;
}

/*--------------------------------------------------------------*/

void
MapGenerator_SaveWorldState(void)
{
	memcpy(s_world_state.map, g_map, sizeof(g_map));
	memcpy(s_world_state.mapSpriteID, g_mapSpriteID, sizeof(g_mapSpriteID));
	s_world_state.house_pool = HousePool_Save();
	s_world_state.structure_pool = StructurePool_Save();
	s_world_state.team_pool = TeamPool_Save();
	s_world_state.unit_pool = UnitPool_Save();
}

void
MapGenerator_LoadWorldState(void)
{
	UnitPool_Load(s_world_state.unit_pool);
	TeamPool_Load(s_world_state.team_pool);
	StructurePool_Load(s_world_state.structure_pool);
	HousePool_Load(s_world_state.house_pool);
	memcpy(g_mapSpriteID, s_world_state.mapSpriteID, sizeof(g_mapSpriteID));
	memcpy(g_map, s_world_state.map, sizeof(g_map));
}
