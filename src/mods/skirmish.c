/* skirmish.c */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../os/common.h"
#include "../os/math.h"

#include "skirmish.h"

#include "multiplayer.h"
#include "../ai.h"
#include "../enhancement.h"
#include "../gui/gui.h"
#include "../map.h"
#include "../opendune.h"
#include "../pool/pool.h"
#include "../pool/pool_structure.h"
#include "../pool/pool_unit.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../tile.h"
#include "../timer/timer.h"
#include "../tools/coord.h"
#include "../tools/random_general.h"
#include "../tools/random_lcg.h"
#include "../video/video.h"

typedef struct {
	int x, y;
	uint16 packed;
	int parent;
} BuildableTile;

typedef struct {
	int start;
	int end;
	bool used;
} Island;

typedef struct SkirmishData {
	/* List of buildable tiles. */
	BuildableTile buildable[MAP_SIZE_MAX * MAP_SIZE_MAX];

	/* Packed index -> island index. */
	int islandID[MAP_SIZE_MAX * MAP_SIZE_MAX];

	int nislands;
	int nislands_unused;
	Island *island;
} SkirmishData;

typedef struct {
	enum StructureType type;
	int priority;               /* low priority is better! */
	bool availableToAlly;
	uint8 availableHouse;
} SkirmishBuildOrder;

static const SkirmishBuildOrder buildorder[] = {
	/* tech level 1: refinery. */
	{ STRUCTURE_CONSTRUCTION_YARD,   1,  true, 0xFF },
	{ STRUCTURE_WINDTRAP,            2,  true, 0xFF },
	{ STRUCTURE_REFINERY,            3,  true, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },

	/* tech level 2: light factory. */
	{ STRUCTURE_REFINERY,           13, false, 0xFF },
	{ STRUCTURE_LIGHT_VEHICLE,       4,  true, 0xFF },
	{ STRUCTURE_WINDTRAP,           16, false, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },

	/* tech level 3: quads. */
	{ STRUCTURE_SILO,               98, false, 0xFF },
	{ STRUCTURE_OUTPOST,             5,  true, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },

	/* tech level 4: tank. */
	{ STRUCTURE_WINDTRAP,            9,  true, 0xFF },
	{ STRUCTURE_HEAVY_VEHICLE,       6,  true, 0xFF },
	{ STRUCTURE_TURRET,             10,  true, 0xFF },
	{ STRUCTURE_TURRET,             21, false, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },

	/* tech level 5: launcher. */
	{ STRUCTURE_WINDTRAP,           22, false, 0xFF },
	{ STRUCTURE_HIGH_TECH,           7,  true, 0xFF },
	{ STRUCTURE_ROCKET_TURRET,      11,  true, 0xFF },
	{ STRUCTURE_ROCKET_TURRET,      12, false, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },

	/* tech level 6: siege tank. */
	{ STRUCTURE_HEAVY_VEHICLE,      14, false, 0xFF },
	{ STRUCTURE_TURRET,             23, false, 0xFF },
	{ STRUCTURE_ROCKET_TURRET,      15, false, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },

	/* tech level 7: ix tank. */
	{ STRUCTURE_WINDTRAP,           18,  true, 0xFF },
	{ STRUCTURE_HOUSE_OF_IX,         8,  true, 0xFF },
	{ STRUCTURE_TURRET,             24, false, 0xFF },
	{ STRUCTURE_ROCKET_TURRET,      17,  true, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },

	/* tech level 8: palace. */
	{ STRUCTURE_WINDTRAP,           25, false, 0xFF },
	{ STRUCTURE_PALACE,             19,  true, 0xFF },
	{ STRUCTURE_ROCKET_TURRET,      26, false, 0xFF },
	{ STRUCTURE_ROCKET_TURRET,      27, false, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },

	/* tech level 9: harder. */
	{ STRUCTURE_CONSTRUCTION_YARD,  20, false, 0xFF },
	{ STRUCTURE_ROCKET_TURRET,      28, false, 0xFF },
	{ STRUCTURE_ROCKET_TURRET,      29, false, 0xFF },
	{ STRUCTURE_INVALID,            99, false, 0    },
};

Skirmish g_skirmish;

/*--------------------------------------------------------------*/

void
Skirmish_Initialise(void)
{
	memset(&g_skirmish, 0, sizeof(g_skirmish));

	g_skirmish.credits = 1500;
	g_skirmish.seed = MapGenerator_PickRandomSeed();

	g_skirmish.landscape_params.min_spice_fields = 24;
	g_skirmish.landscape_params.max_spice_fields = 48;
	g_skirmish.starting_army = MAP_STARTING_ARMY_LARGE;
	g_skirmish.lose_condition = MAP_LOSE_CONDITION_STRUCTURES;
	g_skirmish.worm_count = MAP_WORM_COUNT_2;
	
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		PlayerConfig pc;
		pc.brain = BRAIN_NONE;
		pc.team = (enum PlayerTeam)((int)h +1);
		pc.houseID = h;
		pc.matchType = MATCHTYPE_SKIRMISH;
		g_skirmish.player_config[h] = pc;
	}
}

bool
Skirmish_IsPlayable(void)
{
	bool found_human = false;
	bool found_enemy = false;
	enum PlayerTeam human_team;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		if (g_skirmish.player_config[h].brain == BRAIN_HUMAN) {
			found_human = true;
			human_team = g_skirmish.player_config[h].team;
		}
	}

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		if (g_skirmish.player_config[h].brain == BRAIN_CPU && g_skirmish.player_config[h].team != human_team) {
			found_enemy = true;
		}
	}

	return found_human && found_enemy;
}

static int
Skirmish_PickRandomIsland(const SkirmishData *sd)
{
	if (sd->nislands_unused <= 0)
		return -1;

	int r = Tools_RandomLCG_Range(0, sd->nislands_unused - 1);
	for (int i = 0; i < sd->nislands; i++) {
		if (sd->island[i].used)
			continue;

		if (r <= 0)
			return i;

		r--;
	}

	return -1;
}

static void
Skirmish_FindClosestStructures(enum HouseType houseID, uint16 packed,
		uint16 *dist_ally, uint16 *dist_enemy)
{
	PoolFindStruct find;

	*dist_ally = 0xFFFF;
	*dist_enemy = 0xFFFF;

	for (const Structure *s = Structure_FindFirst(&find, HOUSE_INVALID, STRUCTURE_INVALID);
			s != NULL;
			s = Structure_FindNext(&find)) {
		if (Structure_SharesPoolElement(s->o.type))
			continue;

		const uint16 dist = Tile_GetDistancePacked(Tile_PackTile(s->o.position), packed);
		if (House_AreAllied(houseID, s->o.houseID)) {
			*dist_ally = min(dist, *dist_ally);
		} else {
			*dist_enemy = min(dist, *dist_enemy);
		}
	}
}

static uint16
Skirmish_PickRandomLocation(uint16 acceptableLstFlags, uint16 unacceptableLstFlags)
{
	const MapInfo *mi = &g_mapInfos[0];
	const int x = mi->minX + Tools_RandomLCG_Range(0, mi->sizeX - 1);
	const int y = mi->minY + Tools_RandomLCG_Range(0, mi->sizeY - 1);

	const uint16 packed = Tile_PackXY(x, y);
	if (g_map[packed].hasUnit)
		return 0;

	const enum LandscapeType lst = Map_GetLandscapeType(packed);
	const uint16 lstFlag = (1 << lst);
	if ((acceptableLstFlags & lstFlag) && !(unacceptableLstFlags & lstFlag))
		return packed;

	return 0;
}

static bool
Skirmish_ResetAlliances(void)
{
	enum HouseType playerHouseID = HOUSE_INVALID;

	memset(g_table_houseAlliance, 0, sizeof(g_table_houseAlliance));

	/* Assign alliances. */
	for (enum HouseType h1 = HOUSE_HARKONNEN; h1 < HOUSE_NEUTRAL; h1++) {
		if (g_skirmish.player_config[h1].brain == BRAIN_NONE)
			continue;

		if (g_skirmish.player_config[h1].brain == BRAIN_HUMAN)
			playerHouseID = h1;

		g_table_houseAlliance[h1][h1] = HOUSEALLIANCE_ALLIES;

		for (enum HouseType h2 = h1 + 1; h2 < HOUSE_NEUTRAL; h2++) {
			if (g_skirmish.player_config[h2].brain == BRAIN_NONE)
				continue;

			if (g_skirmish.player_config[h1].team == g_skirmish.player_config[h2].team) {
				g_table_houseAlliance[h1][h2] = HOUSEALLIANCE_ALLIES;
				g_table_houseAlliance[h2][h1] = HOUSEALLIANCE_ALLIES;
			} else {
				g_table_houseAlliance[h1][h2] = HOUSEALLIANCE_ENEMIES;
				g_table_houseAlliance[h2][h1] = HOUSEALLIANCE_ENEMIES;
			}
		}
	}

	/* Make sure, Fremen and Saboteurs belong to the houses that spawned them. */
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		HouseInfo *hi = &g_table_houseInfo[h];

		if (g_skirmish.player_config[h].brain == BRAIN_NONE)
			continue;

		if (hi->specialWeapon == HOUSE_WEAPON_FREMEN) {
			hi->superWeapon.fremen.owner = h;
		}
		
		if (hi->specialWeapon == HOUSE_WEAPON_SABOTEUR) {
			hi->superWeapon.saboteur.owner = h;
		}
	}

	g_playerHouseID = playerHouseID;
	return true;
}

static void
Skirmish_TweakTechTree(void)
{
	/* Disallow CPU ornithopters since they start with a prebuilt
	 * base.  They need hi-tech for carryalls, and IX tanks are fun.
	 */
	g_table_unitInfo[UNIT_ORNITHOPTER].o.availableHouse &= (1 << g_playerHouseID);
}

void
Skirmish_Prepare(void)
{
	Skirmish_ResetAlliances();
	Skirmish_TweakTechTree();
}

void
Skirmish_StartScenario(void)
{
	PoolFindStruct find;
	assert(g_playerHouseID != HOUSE_INVALID);

	Game_Prepare();
	g_tickScenarioStart = g_timerGame;

	GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
	Scenario_CentreViewport(g_playerHouseID);

	const Unit *u = Unit_FindFirst(&find, g_playerHouseID, UNIT_MCV);
	assert(u != NULL);

	Tile_RemoveFogInRadius(1 << g_playerHouseID, UNVEILCAUSE_LONG,
			u->o.position, 10);
}

static void
Skirmish_GenGeneral(uint32 seed)
{
	memset(&g_scenario, 0, sizeof(Scenario));
	g_scenario.winFlags = 3;
	g_scenario.loseFlags = 1;
	g_scenario.mapSeed = seed;
	g_scenario.mapScale = 0;
	g_scenario.timeOut = 0;
}

static void
Skirmish_GenSpiceBlooms(void)
{
	const uint16 acceptableLst = (1 << LST_NORMAL_SAND) | (1 << LST_ENTIRELY_DUNE) | (1 << LST_PARTIAL_DUNE);

	for (int count = Tools_RandomLCG_Range(5, 10); count > 0; count--) {
		const uint16 packed = Skirmish_PickRandomLocation(acceptableLst, 0);
		if (packed == 0)
			continue;

		if ((Tools_Random_256() & 0x3) == 0) {
			Scenario_Load_Map_Field(packed, &g_map[packed]);
		} else {
			Scenario_Load_Map_Bloom(packed, &g_map[packed]);
		}
	}
}

/* Use breadth first flood fill to create a list of buildable tiles
 * around x0, y0.  Only fill tiles that are part of the same source
 * island (similar to bucket fill same colour).
 */
static int
Skirmish_FindBuildableArea(int island, int x0, int y0,
		SkirmishData *sd, BuildableTile *buildable)
{
	const int dx[4] = {  0, 1, 0, -1 };
	const int dy[4] = { -1, 0, 1,  0 };
	int *islandID = sd->islandID;
	int n = 0;

	/* Starting case. */
	do {
		if (!(Map_InRangeX(x0) && Map_InRangeY(y0)))
			break;

		const uint16 packed = Tile_PackXY(x0, y0);
		if (islandID[packed] != island)
			break;

		const enum LandscapeType lst = Map_GetLandscapeType(packed);
		const LandscapeInfo *li = &g_table_landscapeInfo[lst];
		if (!(li->isValidForStructure || li->isValidForStructure2))
			break;

		buildable[n].x = x0;
		buildable[n].y = y0;
		buildable[n].packed = packed;
		buildable[n].parent = 0;
		islandID[packed] = sd->nislands;
		n++;
	} while (false);

	for (int i = 0; i < n; i++) {
		const int r = Tools_Random_256() & 0x3;

		for (int j = 0; j < 4; j++) {
			const int x = buildable[i].x + dx[(r + j) & 0x3];
			const int y = buildable[i].y + dy[(r + j) & 0x3];
			if (!(Map_InRangeX(x) && Map_InRangeY(y)))
				continue;

			const uint16 packed = Tile_PackXY(x, y);
			if (islandID[packed] != island)
				continue;

			const enum LandscapeType lst = Map_GetLandscapeType(packed);
			const LandscapeInfo *li = &g_table_landscapeInfo[lst];
			if (!(li->isValidForStructure || li->isValidForStructure2))
				continue;

			buildable[n].x = x;
			buildable[n].y = y;
			buildable[n].packed = packed;
			buildable[n].parent = i;
			islandID[packed] = sd->nislands;
			n++;
		}
	}

	return n;
}

static bool
Skirmish_IsIslandEnclosed(int start, int end, const SkirmishData *sd)
{
	const int dx[4] = {  0, 1, 0, -1 };
	const int dy[4] = { -1, 0, 1,  0 };
	const uint16 islandID = sd->islandID[sd->buildable[start].packed];

	for (int i = start; i < end; i++) {
		for (int j = 0; j < 4; j++) {
			const int x = sd->buildable[i].x + dx[j];
			const int y = sd->buildable[i].y + dy[j];
			if (!(Map_InRangeX(x) && Map_InRangeY(y)))
				continue;

			const uint16 packed = Tile_PackXY(x, y);
			const enum LandscapeType lst = Map_GetLandscapeType(packed);
			if (sd->islandID[packed] == islandID)
				continue;

			if (!(lst == LST_ENTIRELY_MOUNTAIN || lst == LST_PARTIAL_MOUNTAIN || lst == LST_WALL || lst == LST_STRUCTURE))
				return false;
		}
	}

	return true;
}

static void
Skirmish_DivideIsland(enum HouseType houseID, int island, SkirmishData *sd)
{
	int start = sd->island[island].start;
	const int len = sd->island[island].end - sd->island[island].start;
	BuildableTile *orig = malloc(len * sizeof(orig[0]));
	memcpy(orig, sd->buildable + start, len * sizeof(orig[0]));

	for (int i = 0; i < len; i++) {
		const int area = Skirmish_FindBuildableArea(island, orig[i].x, orig[i].y, sd, sd->buildable + start);

		if (area >= 50) {
			sd->island = realloc(sd->island, (sd->nislands + 1) * sizeof(sd->island[0]));
			assert(sd->island != NULL);

			sd->island[sd->nislands].start = start;
			sd->island[sd->nislands].end = start + area;
			sd->island[sd->nislands].used = false;

			start += area;
			sd->nislands++;
			sd->nislands_unused++;
		} else if (area > 0 && houseID != HOUSE_INVALID) {
			/* Fill enclosed areas with walls to prevent units getting trapped. */
			if (Skirmish_IsIslandEnclosed(start, start + area, sd)) {
				g_validateStrictIfZero++;

				for (int j = start; j < start + area; j++) {
					Structure_Create(STRUCTURE_INDEX_INVALID, STRUCTURE_WALL, houseID, sd->buildable[j].packed);
				}

				g_validateStrictIfZero--;
			}
		}
	}

	sd->island[island].used = true;
	sd->nislands_unused--;
	free(orig);
}

static int
Skirmish_BuildOrder_Sorter(const void *a, const void *b)
{
	const SkirmishBuildOrder *pa = a;
	const SkirmishBuildOrder *pb = b;

	return pa->priority - pb->priority;
}

int
Skirmish_GetTeamCount()
{
	const bool is_multiplayer = (g_campaign_selected == CAMPAIGNID_MULTIPLAYER);

	PlayerConfig *pc = g_skirmish.player_config;
	if (is_multiplayer) {
		pc = g_multiplayer.player_config;
	}

	int team_count = 0;
	for (enum PlayerTeam t = TEAM_1; t < TEAM_MAX; t++) {
		for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
			if (pc[h].brain != BRAIN_NONE && pc[h].team == t) {
				team_count++;
				break;
			}
		}
	}
	return team_count;
}

static bool
Skirmish_GenStructuresAI(enum HouseType houseID, SkirmishData *sd)
{
	const bool is_multiplayer = (g_campaign_selected == CAMPAIGNID_MULTIPLAYER);

	PlayerConfig *pc = g_skirmish.player_config;
	if (is_multiplayer) {
		pc = g_multiplayer.player_config;
	}

	int team_count = Skirmish_GetTeamCount();

	uint16 tech_level = 0;
	uint16 structure = 0;
	int structure_count = 0;
	int structure_threshold = 100;
	bool is_cpu_allied_with_player = pc[houseID].brain == BRAIN_CPU && pc[g_playerHouseID].brain == pc[houseID].brain;

	int cpu_count = 0;
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		if (pc[h].brain == BRAIN_CPU)
			cpu_count++;
	}
	assert(cpu_count != 0);

	int max_structure_count = 60 / cpu_count;
	if (is_multiplayer) {
		max_structure_count = 10;
	}

	/* First pass finds out what to build. */
	tech_level = 0;
	structure = 0;
	structure_count = 0;
	for (structure = 0; (structure < lengthof(buildorder)) && (tech_level <= g_campaignID); structure++) {
		if (buildorder[structure].type == STRUCTURE_INVALID) {
			tech_level++;
		} else if (!buildorder[structure].availableToAlly && is_cpu_allied_with_player) {
		} else if ((buildorder[structure].availableHouse & (1 << houseID)) == 0) {
		} else {
			structure_count++;
		}
	}

	if (structure_count > max_structure_count) {
		structure_count = max_structure_count;

		SkirmishBuildOrder *bo = malloc(structure * sizeof(SkirmishBuildOrder));
		assert(bo != NULL);

		memcpy(bo, buildorder, structure * sizeof(SkirmishBuildOrder));
		qsort(bo, structure, sizeof(SkirmishBuildOrder), Skirmish_BuildOrder_Sorter);
		structure_threshold = bo[structure_count].priority;

		free(bo);
	}

	/* Max/min distance to ally/enemy becomes difficult with many teams. */
	int max_dist_ally = 16;
	int min_dist_enemy = 24;
	if (team_count == 5) {
		max_dist_ally = 20;
		min_dist_enemy = 20;
	}
	if (team_count == 6) {
		max_dist_ally = 25;
		min_dist_enemy = 15;
	}

	/* Second pass builds structures below the threshold priority. */
	tech_level = 0;
	structure = 0;
	for (int attempts = 0; attempts < 100; attempts++) {
		int island = Skirmish_PickRandomIsland(sd);
		int range = 8;
		if (island < 0)
			return false;

		/* Re-flood-fill the island, using a new starting point. */
		{
			const int r = Tools_RandomLCG_Range(sd->island[island].start, sd->island[island].end - 1);

			uint16 dist_ally, dist_enemy;
			Skirmish_FindClosestStructures(houseID, sd->buildable[r].packed, &dist_ally, &dist_enemy);

			if ((dist_ally > max_dist_ally && dist_ally != 0xFFFF) ||
			    (dist_enemy < min_dist_enemy))
				continue;

			const int area = Skirmish_FindBuildableArea(island, sd->buildable[r].x, sd->buildable[r].y,
					sd, sd->buildable + sd->island[island].start);
			assert(area == sd->island[island].end - sd->island[island].start);
			(void)area;

			for (int i = sd->island[island].start; i < sd->island[island].end; i++)
				sd->islandID[sd->buildable[i].packed] = island;
		}

		/* Use no-cheat-mode to verify land is buildable. */
		g_validateStrictIfZero--;
		assert(g_validateStrictIfZero == 0);

		/* Place structures. */
		while (structure_count > 0) {
			assert(structure < lengthof(buildorder));

			if (buildorder[structure].type == STRUCTURE_INVALID) {
				tech_level++;
				structure++;
				continue;
			} else if (!buildorder[structure].availableToAlly && is_cpu_allied_with_player) {
				structure++;
				structure_count--;
				continue;
			} else if (buildorder[structure].priority >= structure_threshold) {
				structure++;
				continue;
			}

			const enum StructureType type = buildorder[structure].type;
			const StructureInfo *si = &g_table_structureInfo[type];
			const int r = Tools_RandomLCG_Range(0, range - 1);
			const uint16 packed = sd->buildable[sd->island[island].start + r].packed;

			if (Structure_IsValidBuildLandscape(packed, type) != 0) {
				Structure *s = Structure_Create(STRUCTURE_INDEX_INVALID, type, houseID, packed);
				assert(s != NULL);

				s->o.hitpoints = si->o.hitpoints;
				s->o.flags.s.degrades = false;
				s->state = STRUCTURE_STATE_IDLE;

				if (House_AreAllied(g_playerHouseID, houseID))
					s->o.seenByHouses = 0xFF;

				if (s->o.type == STRUCTURE_PALACE)
					s->countDown = g_table_houseInfo[houseID].specialCountDown;

				range = min(range + 4, sd->island[island].end - sd->island[island].start);
				structure++;
				structure_count--;
			} else {
				range++;

				if (range > sd->island[island].end - sd->island[island].start)
					break;
			}
		}

		/* Connect structures on this island with concrete slabs. */
		for (range--; range > 0; range--) {
			const int self = sd->island[island].start + range;
			uint16 packed = sd->buildable[self].packed;

			const enum LandscapeType lst = Map_GetLandscapeType(packed);
			if (lst != LST_STRUCTURE && lst != LST_CONCRETE_SLAB)
				continue;

			const int parent = sd->island[island].start + sd->buildable[self].parent;
			packed = sd->buildable[parent].packed;
			if (Structure_IsValidBuildLandscape(packed, STRUCTURE_SLAB_1x1) == 0)
				continue;

			g_validateStrictIfZero++;
			Structure_Create(STRUCTURE_INDEX_INVALID, STRUCTURE_SLAB_1x1, houseID, packed);
			g_validateStrictIfZero--;
		}

		/* Finished building on this island.  Create sub-islands from
		 * remaining buildable tiles.
		 */
		g_validateStrictIfZero++;
		Skirmish_DivideIsland(houseID, island, sd);

		if (structure_count <= 0)
			return true;
	}

	return false;
}

uint16
Skirmish_FindStartLocation(enum HouseType houseID, uint16 dist_threshold, SkirmishData *sd)
{
	const MapInfo *mi = &g_mapInfos[0];

	/* Pick a tile that is not too close to the edge, and not too
	 * close to the enemy.
	 */
	for (int attempts = 0; attempts < 100; attempts++) {
		const int island = Skirmish_PickRandomIsland(sd);
		if (island < 0)
			return 0;

		int r = Tools_RandomLCG_Range(sd->island[island].start, sd->island[island].end - 1);
		if (!(mi->minX + 4 <= sd->buildable[r].x && sd->buildable[r].x < mi->minX + mi->sizeX - 4))
			continue;

		if (!(mi->minY + 3 <= sd->buildable[r].y && sd->buildable[r].y < mi->minY + mi->sizeY - 3))
			continue;

		PoolFindStruct find;
		const Structure *s;
		const Unit *u;

		for (s = Structure_FindFirst(&find, HOUSE_INVALID, STRUCTURE_INVALID);
				s != NULL;
				s = Structure_FindNext(&find)) {
			if (Structure_SharesPoolElement(s->o.type))
				continue;

			if (House_AreAllied(houseID, s->o.houseID))
				continue;

			const uint16 dist = Tile_GetDistancePacked(Tile_PackTile(s->o.position), sd->buildable[r].packed);
			if (dist < dist_threshold)
				break;
		}

		if (s != NULL)
			continue;

		for (u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
				u != NULL;
				u = Unit_FindNext(&find)) {
			if (House_AreAllied(houseID, u->o.houseID))
				continue;

			const uint16 dist = Tile_GetDistancePacked(Tile_PackTile(u->o.position), sd->buildable[r].packed);
			if (dist < dist_threshold)
				break;
		}

		if (u != NULL)
			continue;

		return sd->buildable[r].packed;
	}

	return 0;
}

bool
Skirmish_GenUnitsHuman(enum HouseType houseID, struct SkirmishData *sd)
{

	const bool is_multiplayer = (g_campaign_selected == CAMPAIGNID_MULTIPLAYER);

	enum MapStartingArmy starting_army = g_skirmish.starting_army;
	if (is_multiplayer) {
		starting_army = g_multiplayer.starting_army;
	}

	enum UnitType units[7] = {
		UNIT_MCV, UNIT_TROOPERS, UNIT_QUAD, UNIT_QUAD, UNIT_TANK, UNIT_SIEGE_TANK, UNIT_SIEGE_TANK
	};

	static const int delta[7] = {
		0, -4, 4,
		-MAP_SIZE_MAX * 3 - 2, -MAP_SIZE_MAX * 3 + 2,
		 MAP_SIZE_MAX * 3 - 2,  MAP_SIZE_MAX * 3 + 2,
	};

	assert_compile(lengthof(delta) == lengthof(units));

	int team_count = Skirmish_GetTeamCount();

	// Adjust distance threshold based on team count.
	// Higher team counts need lower thresholds
	// to increase map generation success.
	int dist_threshold = 32;
	if (team_count == 5) {
		dist_threshold = 28;
	}
	if (team_count == 6) {
		dist_threshold = 24;
	}

	const uint16 start_location = Skirmish_FindStartLocation(houseID, dist_threshold, sd);
	if (!Map_IsValidPosition(start_location))
		return false;

	static const int unitRowDelta[3] = {0, -1, 1};
	static const int unitColDelta[7] = {0, -1, 1, -2, 2, -3, 3};

	for (unsigned int i = 0; i < lengthof(units); i++) {

		const uint16 unit_init_location = start_location + delta[i];
		uint16 final_unit_location = 0;

		for (unsigned int r = 0; r < lengthof(unitRowDelta); r++) {
			for (unsigned int c = 0; c < lengthof(unitColDelta); c++) {
				const uint16 try_unit_location = unit_init_location + (unitRowDelta[r] * MAP_SIZE_MAX) + unitColDelta[c];
				const enum LandscapeType lst = Map_GetLandscapeType(try_unit_location);
				if (lst != LST_ENTIRELY_MOUNTAIN && lst != LST_PARTIAL_MOUNTAIN && lst != LST_WALL && lst != LST_STRUCTURE && lst != LST_BLOOM_FIELD) {
					final_unit_location = try_unit_location;
					break;
				}
			}
			if (final_unit_location != 0)
				break;
		}

		// If we didn't find a suitable location, use the initial (MCV) location.
		if (final_unit_location == 0)
			final_unit_location = start_location;

		Scenario_Create_Unit(houseID, units[i], 256, Tile_UnpackTile(final_unit_location), 127,
				g_table_unitInfo[units[i]].o.actionsPlayer[3]);
		
		if (starting_army == MAP_STARTING_ARMY_SMALL && i > 1)
			break;
	}

	return true;
}

void
Skirmish_GenUnitsAI(enum HouseType houseID)
{
	const uint16 unacceptableLst = (1 << LST_WALL) | (1 << LST_STRUCTURE) | (1 << LST_BLOOM_FIELD);

	const Structure *factory[8];
	unsigned int nfactories = 0;
	PoolFindStruct find;

	for (const Structure *s = Structure_FindFirst(&find, houseID, STRUCTURE_INVALID);
			s != NULL;
			s = Structure_FindNext(&find)) {
		/* Do not produce from hi-tech. */
		if (s->o.type == STRUCTURE_LIGHT_VEHICLE ||
		    s->o.type == STRUCTURE_HEAVY_VEHICLE ||
		    s->o.type == STRUCTURE_WOR_TROOPER ||
		    s->o.type == STRUCTURE_BARRACKS) {
			factory[nfactories] = s;
			nfactories++;

			if (nfactories >= lengthof(factory))
				break;
		}
	}

	if (nfactories == 0)
		return;

	for (int count = 8; count > 0;) {
		const uint16 packed = Skirmish_PickRandomLocation(0xFF, unacceptableLst);
		if (packed == 0)
			continue;

		uint16 dist_ally;
		uint16 dist_enemy;
		Skirmish_FindClosestStructures(houseID, packed, &dist_ally, &dist_enemy);
		if (dist_ally > 8)
			continue;

		const enum LandscapeType lst = Map_GetLandscapeType(packed);
		enum UnitType type;

		if (lst == LST_ENTIRELY_MOUNTAIN || lst == LST_PARTIAL_MOUNTAIN) {
			/* If there's a mountain here, build infantry. */
			type = House_GetInfantrySquad(houseID);
		} else {
			/* Otherwise, build a random vehicle. */
			const int r = Tools_RandomLCG_Range(0, nfactories - 1);

			type = StructureAI_PickNextToBuild(factory[r]);
			if (type == UNIT_INVALID)
				continue;
		}

		const tile32 position = Tile_UnpackTile(packed);
		Scenario_Create_Unit(houseID, type, 256, position, 127, ACTION_AREA_GUARD);
		count--;
	}
}

bool
Skirmish_GenHouses(SkirmishData *sd)
{
	const bool is_multiplayer = (g_campaign_selected == CAMPAIGNID_MULTIPLAYER);

	PlayerConfig *pc = g_skirmish.player_config;
	uint16 credits = g_skirmish.credits;
	if (is_multiplayer) {
		pc = g_multiplayer.player_config;
		credits = g_multiplayer.credits;
	}
	
	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_NEUTRAL; houseID++) {
		if (pc[houseID].brain == BRAIN_NONE)
			continue;

		if (pc[houseID].brain == BRAIN_HUMAN) {
			Scenario_Create_House(houseID, BRAIN_HUMAN, credits, 0, UNIT_MAX_PER_HOUSE_RAISED);
		} else {
			House *h = Scenario_Create_House(houseID, pc[houseID].brain, 1000, 0, UNIT_MAX_PER_HOUSE_RAISED);

			h->flags.isAIActive = true;

			if (!Skirmish_GenStructuresAI(houseID, sd))
				return false;
		}
	}

	return true;
}

static bool
Skirmish_GenUnits(SkirmishData *sd)
{
	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_NEUTRAL; houseID++) {
		if (g_skirmish.player_config[houseID].brain == BRAIN_NONE)
			continue;

		if (g_skirmish.player_config[houseID].brain == BRAIN_HUMAN) {
			if (!Skirmish_GenUnitsHuman(houseID, sd))
				return false;
		} else {
			Skirmish_GenUnitsAI(houseID);
		}
	}

	return true;
}

static int
Skirmish_GenSandworms(int worm_count)
{
	const enum HouseType houseID = HOUSE_NEUTRAL;
	const enum UnitType type = UNIT_SANDWORM;
	const enum UnitActionType actionType = enhancement_insatiable_sandworms ? ACTION_AREA_GUARD : ACTION_AMBUSH;
	const uint16 acceptableLst
		= (1 << LST_NORMAL_SAND) | (1 << LST_ENTIRELY_DUNE) | (1 << LST_PARTIAL_DUNE)
		| (1 << LST_SPICE) | (1 << LST_THICK_SPICE) | (1 << LST_BLOOM_FIELD);

	/* Create sandworms. */
	for (int count = worm_count; count > 0;) {
		const uint16 packed = Skirmish_PickRandomLocation(acceptableLst, 0);
		if (packed == 0)
			continue;

		const tile32 position = Tile_UnpackTile(packed);
		Scenario_Create_Unit(houseID, type, 256, position, 127, actionType);
		count--;
	}
}

static void
Skirmish_GenReinforcements(void)
{
	const enum UnitType unitType1[9] = {
		UNIT_TROOPERS,      UNIT_QUAD,          UNIT_QUAD,
		UNIT_TANK,          UNIT_LAUNCHER,      UNIT_SIEGE_TANK,
		UNIT_DEVASTATOR,    UNIT_DEVASTATOR,    UNIT_DEVASTATOR,
	};

	const enum UnitType unitType2[9] = {
		UNIT_TROOPERS,      UNIT_TROOPERS,      UNIT_QUAD,
		UNIT_TANK,          UNIT_TANK,          UNIT_SIEGE_TANK,
		UNIT_SIEGE_TANK,    UNIT_SIEGE_TANK,    UNIT_SIEGE_TANK,
	};
	assert(g_campaignID < 9);

	uint8 index = 0;
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL && index < 16; h++) {
		if (g_skirmish.player_config[h].brain == BRAIN_NONE)
			continue;

		enum UnitType type[2];
		type[0] = unitType1[Tools_RandomLCG_Range(0, g_campaignID)];
		type[1] = unitType2[Tools_RandomLCG_Range(0, g_campaignID)];

		for (int i = 0; i < 2; i++) {
			if (type[i] == UNIT_TROOPERS) {
				type[i] = House_GetInfantrySquad(h);
			} else if (type[i] == UNIT_QUAD) {
				type[i] = House_GetLightVehicle(h);
			} else if (type[i] == UNIT_DEVASTATOR) {
				type[i] = House_GetIXVehicle(h);
			} else if (type[i] == UNIT_LAUNCHER && h == HOUSE_ORDOS) {
				type[i] = UNIT_DEVIATOR;
			}
		}

		const bool repeat = (g_skirmish.player_config[h].brain != BRAIN_HUMAN);

		if (h == HOUSE_SARDAUKAR) {
			/* Sardaukar always get four sets of troopers in the enemy base.  That's just how it is! */
			Scenario_Create_Reinforcement(index++, h, UNIT_TROOPERS, 6, 10 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, UNIT_TROOPERS, 6, 10 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, UNIT_TROOPERS, 6, 10 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, UNIT_TROOPERS, 6, 10 * 6, repeat);
		} else if (g_skirmish.player_config[h].brain == BRAIN_HUMAN) {
			/* Players always get reinforcements at home base. */
			Scenario_Create_Reinforcement(index++, h, UNIT_TANK, 7,  6 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, type[0],   7,  6 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, UNIT_TANK, 7, 12 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, type[1],   7, 12 * 6, repeat);
		} else {
			/* Pick random location, but avoid AIR and VISIBLE types. */
			uint8 locationID = Tools_Random_256() & 0x7;
			if (locationID == 4 || locationID == 5) locationID += 2;

			Scenario_Create_Reinforcement(index++, h, type[0], locationID, 15 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, type[1], locationID, 15 * 6, repeat);
		}
	}
}

static void
Skirmish_GenCHOAM(void)
{
	g_starportAvailable[UNIT_CARRYALL] = 2;
	g_starportAvailable[UNIT_TRIKE] = 5;
	g_starportAvailable[UNIT_QUAD] = 5;
	g_starportAvailable[UNIT_TANK] = Tools_RandomLCG_Range(4, 6);
	g_starportAvailable[UNIT_HARVESTER] = 2;
	g_starportAvailable[UNIT_MCV] = 2;

	if (g_campaignID >= 5) g_starportAvailable[UNIT_LAUNCHER] = Tools_RandomLCG_Range(2, 4);
	if (g_campaignID >= 6) g_starportAvailable[UNIT_SIEGE_TANK] = Tools_RandomLCG_Range(3, 5);
	if (g_campaignID >= 7) g_starportAvailable[UNIT_ORNITHOPTER] = 3;
}

static bool
Skirmish_GenerateMap2(bool only_landscape, SkirmishData *sd)
{
	const bool is_skirmish = (g_campaign_selected == CAMPAIGNID_SKIRMISH);
	const MapInfo *mi = &g_mapInfos[0];

	Game_Init();

	const uint32 seed = is_skirmish ? g_skirmish.seed : g_multiplayer.test_seed;
	const LandscapeGeneratorParams *params
		= is_skirmish
		? &g_skirmish.landscape_params : &g_multiplayer.landscape_params;

	Skirmish_GenGeneral(seed);
	Sprites_UnloadTiles();
	Sprites_LoadTiles();
	Tools_RandomLCG_Seed(seed);
	Map_CreateLandscape(seed, params, g_map);

	if (only_landscape)
		return true;

	/* Create initial island. */
	sd->island[0].start = 0;
	sd->island[0].end = 0;
	for (int dy = 0; dy < mi->sizeY; dy++) {
		for (int dx = 0; dx < mi->sizeX; dx++) {
			const int tx = mi->minX + dx;
			const int ty = mi->minY + dy;
			const uint16 packed = Tile_PackXY(tx, ty);

			sd->buildable[sd->island[0].end].x = tx;
			sd->buildable[sd->island[0].end].y = ty;
			sd->buildable[sd->island[0].end].packed = packed;
			sd->buildable[sd->island[0].end].parent = 0;
			sd->island[0].end++;
		}
	}

	memset(sd->islandID, 0, MAP_SIZE_MAX * MAP_SIZE_MAX * sizeof(sd->islandID[0]));
	Skirmish_DivideIsland(HOUSE_INVALID, 0, sd);

	if (sd->nislands_unused == 0)
		return false;

	if (is_skirmish) {
		if (!Skirmish_GenHouses(sd))
			return false;
		if (!Skirmish_GenUnits(sd))
			return false;
	} else {
		if (!Multiplayer_GenHouses(sd))
			return false;
	}

	Skirmish_GenSpiceBlooms();

	if (is_skirmish) {
		Skirmish_GenSandworms(g_skirmish.worm_count);
		Skirmish_GenReinforcements();
	}
	else {
		Skirmish_GenSandworms(g_multiplayer.worm_count);
	}

	Skirmish_GenCHOAM();

#if 0
	/* Debugging. */
	for (int island = 0; island < sd.nislands; island++) {
		if (sd.island[island].used)
			continue;

		for (int i = sd.island[island].start; i < sd.island[island].end; i++) {
			g_map[sd.buildable[i].packed].groundSpriteID = g_veiledSpriteID;
		}
	}
#endif

	return true;
}

bool
Skirmish_GenerateMap1(bool is_playable)
{
	SkirmishData sd;
	assert(g_validateStrictIfZero == 0);

	g_validateStrictIfZero++;

	if (is_playable) {
		sd.island = malloc(sizeof(sd.island[0]));
		assert(sd.island != NULL);

		sd.nislands = 1;
		sd.nislands_unused = 1;
	} else {
		sd.island = NULL;

		sd.nislands = 0;
		sd.nislands_unused = 0;
	}

	const bool ret = Skirmish_GenerateMap2(!is_playable, &sd);

	free(sd.island);
	g_validateStrictIfZero--;

	assert(g_validateStrictIfZero == 0);
	return ret;
}

bool
Skirmish_GenerateMap(enum MapGeneratorMode mode)
{
	assert(g_campaign_selected == CAMPAIGNID_SKIRMISH);

	switch (mode) {
		case MAP_GENERATOR_TRY_TEST_ELSE_STOP:
		case MAP_GENERATOR_TRY_TEST_ELSE_RAND:
		case MAP_GENERATOR_FINAL:
			break;

		case MAP_GENERATOR_TRY_RAND_ELSE_STOP:
		case MAP_GENERATOR_TRY_RAND_ELSE_RAND:
			g_skirmish.seed = MapGenerator_PickRandomSeed();
			break;

		case MAP_GENERATOR_STOP:
		default:
			return MAP_GENERATOR_STOP;
	}

	g_campaignID = 7;
	g_scenarioID = 20;

	bool is_playable = Skirmish_IsPlayable();
	if (is_playable) {
		Campaign_Load();
		Skirmish_Prepare();
	}

	bool success = Skirmish_GenerateMap1(is_playable);

	if (success) {
		/* Save the minimap for the lobby. */
		Video_DrawMinimap(0, 0, 0, MINIMAP_SAVE);
	}

	return success;
}
