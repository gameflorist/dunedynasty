/* skirmish.c */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../os/common.h"
#include "../os/math.h"

#include "skirmish.h"

#include "../ai.h"
#include "../enhancement.h"
#include "../gui/gui.h"
#include "../map.h"
#include "../opendune.h"
#include "../pool/pool.h"
#include "../pool/structure.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../tile.h"
#include "../timer/timer.h"
#include "../tools/coord.h"
#include "../tools/random_general.h"
#include "../tools/random_lcg.h"

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

typedef struct {
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

/*--------------------------------------------------------------*/

bool
Skirmish_IsPlayable(void)
{
	bool found_human = false;
	bool found_enemy = false;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_skirmish.brain[h] == BRAIN_HUMAN) {
			found_human = true;
		}
		else if (g_skirmish.brain[h] == BRAIN_CPU_ENEMY) {
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
	Structure *s;

	find.houseID = HOUSE_INVALID;
	find.type = 0xFFFF;
	find.index = STRUCTURE_INDEX_INVALID;

	*dist_ally = 0xFFFF;
	*dist_enemy = 0xFFFF;
	while ((s = Structure_Find(&find)) != NULL) {
		if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL)
			continue;

		const uint16 dist = Tile_GetDistancePacked(Tile_PackTile(s->o.position), packed);
		if (House_AreAllied(houseID, s->o.houseID)) {
			*dist_ally = min(dist, *dist_ally);
		}
		else {
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
	for (enum HouseType h1 = HOUSE_HARKONNEN; h1 < HOUSE_MAX; h1++) {
		if (g_skirmish.brain[h1] == BRAIN_NONE)
			continue;

		if (g_skirmish.brain[h1] == BRAIN_HUMAN)
			playerHouseID = h1;

		g_table_houseAlliance[h1][h1] = HOUSEALLIANCE_ALLIES;

		for (enum HouseType h2 = h1 + 1; h2 < HOUSE_MAX; h2++) {
			if (g_skirmish.brain[h2] == BRAIN_NONE)
				continue;

			if ((g_skirmish.brain[h1] == BRAIN_CPU_ENEMY && g_skirmish.brain[h2] == BRAIN_CPU_ENEMY) ||
			    (g_skirmish.brain[h1] != BRAIN_CPU_ENEMY && g_skirmish.brain[h2] != BRAIN_CPU_ENEMY)) {
				g_table_houseAlliance[h1][h2] = HOUSEALLIANCE_ALLIES;
				g_table_houseAlliance[h2][h1] = HOUSEALLIANCE_ALLIES;
			}
			else {
				g_table_houseAlliance[h1][h2] = HOUSEALLIANCE_ENEMIES;
				g_table_houseAlliance[h2][h1] = HOUSEALLIANCE_ENEMIES;
			}
		}
	}

	/* Make Fremen and saboteur superweapons usable for houses. */
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		HouseInfo *hi = &g_table_houseInfo[h];

		if ((hi->specialWeapon != HOUSE_WEAPON_FREMEN) &&
		    (hi->specialWeapon != HOUSE_WEAPON_SABOTEUR))
			continue;

		enum HouseType h2;
		enum HouseType *hp;

		if (hi->specialWeapon == HOUSE_WEAPON_FREMEN) {
			hp = &(hi->superWeapon.fremen.owner);
		}
		else {
			hp = &(hi->superWeapon.saboteur.owner);
		}

		if (g_table_houseAlliance[h][*hp] == HOUSEALLIANCE_ENEMIES) {
			*hp = h;
			continue;
		}
		else {
			h2 = *hp;
		}

		for (enum HouseType h1 = HOUSE_HARKONNEN; h1 < HOUSE_MAX; h1++) {
			if (g_table_houseAlliance[h][h1] == HOUSEALLIANCE_ALLIES) {
				g_table_houseAlliance[h1][h2] = HOUSEALLIANCE_ALLIES;
				g_table_houseAlliance[h2][h1] = HOUSEALLIANCE_ALLIES;
			}
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

static void
Skirmish_GenGeneral(void)
{
	memset(&g_scenario, 0, sizeof(Scenario));
	g_scenario.winFlags = 3;
	g_scenario.loseFlags = 1;
	g_scenario.mapSeed = g_skirmish.seed;
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
		}
		else {
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
		}
		else if (area > 0 && houseID != HOUSE_INVALID) {
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

static bool
Skirmish_GenStructuresAI(enum HouseType houseID, SkirmishData *sd)
{
	uint16 tech_level = 0;
	uint16 structure = 0;
	int structure_count = 0;
	int structure_threshold = 100;

	int cpu_count = 0;
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_skirmish.brain[h] == BRAIN_CPU_ENEMY || g_skirmish.brain[h] == BRAIN_CPU_ALLY)
			cpu_count++;
	}
	assert(cpu_count != 0);

	const int max_structure_count = 60 / cpu_count;

	/* First pass finds out what to build. */
	tech_level = 0;
	structure = 0;
	structure_count = 0;
	for (structure = 0; (structure < lengthof(buildorder)) && (tech_level <= g_campaignID); structure++) {
		if (buildorder[structure].type == STRUCTURE_INVALID) {
			tech_level++;
		}
		else if (!buildorder[structure].availableToAlly && (g_skirmish.brain[houseID] == BRAIN_CPU_ALLY)) {
		}
		else if ((buildorder[structure].availableHouse & (1 << houseID)) == 0) {
		}
		else {
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

			if ((dist_ally > 16 && dist_ally != 0xFFFF) ||
			    (dist_enemy < 24))
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
			}
			else if (!buildorder[structure].availableToAlly && (g_skirmish.brain[houseID] == BRAIN_CPU_ALLY)) {
				structure++;
				structure_count--;
				continue;
			}
			else if (buildorder[structure].priority >= structure_threshold) {
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
			}
			else {
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

static bool
Skirmish_GenUnitsHuman(enum HouseType houseID, SkirmishData *sd)
{
	const int delta[7] = {
		0, -4, 4,
		-MAP_SIZE_MAX * 3 - 2, -MAP_SIZE_MAX * 3 + 2,
		 MAP_SIZE_MAX * 3 - 2,  MAP_SIZE_MAX * 3 + 2,
	};

	const MapInfo *mi = &g_mapInfos[0];

	/* Pick a tile that is not too close to the edge, and not too
	 * close to the enemy.
	 */
	int r;
	for (int attempts = 0; attempts < 100; attempts++) {
		const int island = Skirmish_PickRandomIsland(sd);
		if (island < 0)
			return false;

		r = Tools_RandomLCG_Range(sd->island[island].start, sd->island[island].end - 1);
		if (!(mi->minX + 4 <= sd->buildable[r].x && sd->buildable[r].x < mi->minX + mi->sizeX - 4))
			continue;

		if (!(mi->minY + 3 <= sd->buildable[r].y && sd->buildable[r].y < mi->minY + mi->sizeY - 3))
			continue;

		PoolFindStruct find;
		find.houseID = HOUSE_INVALID;
		find.type = 0xFFFF;
		find.index = STRUCTURE_INDEX_INVALID;

		Structure *s = Structure_Find(&find);
		for (; s != NULL; s = Structure_Find(&find)) {
			if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL)
				continue;

			if (House_AreAllied(g_playerHouseID, s->o.houseID))
				continue;

			const uint16 dist = Tile_GetDistancePacked(Tile_PackTile(s->o.position), sd->buildable[r].packed);
			if (dist < 24)
				break;
		}

		if (s == NULL) {
			break;
		}
		else {
			r = -1;
		}
	}

	if (r < 0)
		return false;

	for (int i = 0; i < 7; i++) {
		const uint16 packed = sd->buildable[r].packed + delta[i];
		const tile32 position = Tile_UnpackTile(packed);

		enum UnitType type = (i == 0) ? UNIT_MCV : House_GetLightVehicle(houseID);

		enum LandscapeType lst = Map_GetLandscapeType(packed);

		/* If there's a structure or a bloom here, tough luck. */
		if (lst == LST_STRUCTURE || lst == LST_BLOOM_FIELD)
			continue;

		/* If there's a mountain here, build infantry instead. */
		if (lst == LST_ENTIRELY_MOUNTAIN || lst == LST_PARTIAL_MOUNTAIN)
			type = House_GetInfantrySquad(houseID);

		Scenario_Create_Unit(houseID, type, 256, position, 127, g_table_unitInfo[type].o.actionsPlayer[3]);
	}

	return true;
}

static void
Skirmish_GenUnitsAI(enum HouseType houseID)
{
	const uint16 unacceptableLst = (1 << LST_WALL) | (1 << LST_STRUCTURE) | (1 << LST_BLOOM_FIELD);

	Structure *factory[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	unsigned int nfactories = 0;

	PoolFindStruct find;
	find.houseID = houseID;
	find.type = 0xFFFF;
	find.index = STRUCTURE_INDEX_INVALID;

	Structure *s;
	while ((nfactories < lengthof(factory)) && ((s = Structure_Find(&find)) != NULL)) {
		/* Do not produce from hi-tech. */
		if (s->o.type == STRUCTURE_LIGHT_VEHICLE ||
		    s->o.type == STRUCTURE_HEAVY_VEHICLE ||
		    s->o.type == STRUCTURE_WOR_TROOPER ||
		    s->o.type == STRUCTURE_BARRACKS) {
			factory[nfactories] = s;
			nfactories++;
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

		/* If there's a mountain here, build infantry. */
		const enum LandscapeType lst = Map_GetLandscapeType(packed);
		enum UnitType type;

		if (lst == LST_ENTIRELY_MOUNTAIN || lst == LST_PARTIAL_MOUNTAIN) {
			type = House_GetInfantrySquad(houseID);
		}

		/* Otherwise, build a random vehicle. */
		else {
			const int r = Tools_RandomLCG_Range(0, nfactories - 1);

			type = StructureAI_PickNextToBuild(factory[r]);
			if (type == UNIT_INVALID)
				continue;
		}

		const enum UnitActionType actionType = ((Tools_Random_256() & 0x3) == 0) ? ACTION_AMBUSH : ACTION_AREA_GUARD;
		const tile32 position = Tile_UnpackTile(packed);
		Scenario_Create_Unit(houseID, type, 256, position, 127, actionType);
		count--;
	}
}

static void
Skirmish_GenSandworms(void)
{
	const enum HouseType houseID = (g_playerHouseID == HOUSE_FREMEN) ? HOUSE_ATREIDES : HOUSE_FREMEN;
	const enum UnitType type = UNIT_SANDWORM;
	const enum UnitActionType actionType = enhancement_insatiable_sandworms ? ACTION_AREA_GUARD : ACTION_AMBUSH;
	const uint16 acceptableLst
		= (1 << LST_NORMAL_SAND) | (1 << LST_ENTIRELY_DUNE) | (1 << LST_PARTIAL_DUNE)
		| (1 << LST_SPICE) | (1 << LST_THICK_SPICE) | (1 << LST_BLOOM_FIELD);

	/* Create two sandworms. */
	for (int count = 2; count > 0;) {
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
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX && index < 16; h++) {
		if (g_skirmish.brain[h] == BRAIN_NONE)
			continue;

		enum UnitType type[2];
		type[0] = unitType1[Tools_RandomLCG_Range(0, g_campaignID)];
		type[1] = unitType2[Tools_RandomLCG_Range(0, g_campaignID)];

		for (int i = 0; i < 2; i++) {
			if (type[i] == UNIT_TROOPERS)
				type[i] = House_GetInfantrySquad(h);
			else if (type[i] == UNIT_QUAD)
				type[i] = House_GetLightVehicle(h);
			else if (type[i] == UNIT_DEVASTATOR)
				type[i] = House_GetIXVehicle(h);
			else if (type[i] == UNIT_LAUNCHER && h == HOUSE_ORDOS)
				type[i] = UNIT_DEVIATOR;
		}

		const bool repeat = (g_skirmish.brain[h] != BRAIN_HUMAN);

		/* Sardaukar always get four sets of troopers in the enemy base.  That's just how it is! */
		if (h == HOUSE_SARDAUKAR) {
			Scenario_Create_Reinforcement(index++, h, UNIT_TROOPERS, 6, 10 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, UNIT_TROOPERS, 6, 10 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, UNIT_TROOPERS, 6, 10 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, UNIT_TROOPERS, 6, 10 * 6, repeat);
		}

		/* Players always get reinforcements at home base. */
		else if (g_skirmish.brain[h] == BRAIN_HUMAN) {
			Scenario_Create_Reinforcement(index++, h, UNIT_TANK, 7,  6 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, type[0],   7,  6 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, UNIT_TANK, 7, 12 * 6, repeat);
			Scenario_Create_Reinforcement(index++, h, type[1],   7, 12 * 6, repeat);
		}

		/* Pick random location, but avoid AIR and VISIBLE types. */
		else {
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
Skirmish_GenerateMapInner(bool generate_houses, SkirmishData *sd)
{
	const MapInfo *mi = &g_mapInfos[0];

	if (generate_houses) {
		Campaign_Load();
		Skirmish_Prepare();
	}

	Game_Init();

	Skirmish_GenGeneral();
	Sprites_UnloadTiles();
	Sprites_LoadTiles();
	Tools_RandomLCG_Seed(g_skirmish.seed);
	Map_CreateLandscape(g_skirmish.seed);

	if (!generate_houses)
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

	/* Spawn players. */
	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		if (g_skirmish.brain[houseID] == BRAIN_NONE)
			continue;

		if (g_skirmish.brain[houseID] == BRAIN_HUMAN) {
			Scenario_Create_House(houseID, g_skirmish.brain[houseID], 1500, 0, 25);
		}
		else {
			House *h = Scenario_Create_House(houseID, g_skirmish.brain[houseID], 1000, 0, 25);

			h->flags.isAIActive = true;

			if (!Skirmish_GenStructuresAI(houseID, sd))
				return false;
		}
	}

	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		if (g_skirmish.brain[houseID] == BRAIN_NONE)
			continue;

		if (g_skirmish.brain[houseID] == BRAIN_HUMAN) {
			if (!Skirmish_GenUnitsHuman(houseID, sd))
				return false;
		}
		else {
			Skirmish_GenUnitsAI(houseID);
		}
	}

	Skirmish_GenSpiceBlooms();
	Skirmish_GenSandworms();
	Skirmish_GenReinforcements();
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

	Game_Prepare();
	GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
	Scenario_CentreViewport(g_playerHouseID);
	g_tickScenarioStart = g_timerGame;
	return true;
}

bool
Skirmish_GenerateMap(bool newseed)
{
	const bool generate_houses = Skirmish_IsPlayable();
	assert(g_validateStrictIfZero == 0);

	g_campaignID = 7;
	g_scenarioID = 20;
	g_validateStrictIfZero++;

	if (newseed) {
		/* DuneMaps only supports 15 bit maps seeds, so there. */
		g_skirmish.seed = rand() & 0x7FFF;
	}

	SkirmishData sd;

	if (generate_houses) {
		sd.island = malloc(sizeof(sd.island[0]));
		assert(sd.island != NULL);

		sd.nislands = 1;
		sd.nislands_unused = 1;
	}
	else {
		sd.island = NULL;

		sd.nislands = 0;
		sd.nislands_unused = 0;
	}

	const bool ret = Skirmish_GenerateMapInner(generate_houses, &sd);

	free(sd.island);
	g_validateStrictIfZero--;

	assert(g_validateStrictIfZero == 0);
	return ret;
}
