/* skirmish.c */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "skirmish.h"

#include "../map.h"
#include "../opendune.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../tile.h"
#include "../tools.h"

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

static void
Skirmish_DivideIsland(int island, SkirmishData *sd)
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
	}

	sd->island[island].used = true;
	sd->nislands_unused--;
	free(orig);
}

static bool
Skirmish_GenerateMapInner(bool generate_houses, SkirmishData *sd)
{
	const MapInfo *mi = &g_mapInfos[0];

	if (generate_houses) {
		Campaign_Load();
	}

	Game_Init();

	Skirmish_GenGeneral();
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
	Skirmish_DivideIsland(0, sd);

	if (sd->nislands_unused == 0)
		return false;

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
	return true;
}

bool
Skirmish_GenerateMap(bool newseed)
{
	const bool generate_houses = Skirmish_IsPlayable();

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
	return ret;
}
