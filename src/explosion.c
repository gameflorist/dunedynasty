/** @file src/explosion.c Explosion routines. */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os/math.h"

#include "explosion.h"

#include "animation.h"
#include "binheap.h"
#include "enhancement.h"
#include "house.h"
#include "map.h"
#include "net/server.h"
#include "opendune.h"
#include "pool/pool.h"
#include "pool/pool_structure.h"
#include "pool/pool_unit.h"
#include "scenario.h"
#include "sprites.h"
#include "structure.h"
#include "tile.h"
#include "timer/timer.h"
#include "unit.h"
#include "video/video_a5.h"
#include "tools/coord.h"
#include "tools/encoded_index.h"
#include "tools/random_general.h"
#include "tools/random_lcg.h"

static BinHeap s_explosions;

extern const ExplosionCommandStruct * const g_table_explosion[EXPLOSIONTYPE_MAX];

static void
Explosion_Update(const Explosion *e)
{
	Map_UpdateAround(24, e->position, NULL, 0);
}

/**
 * Handle damage to a tile, removing spice, removing concrete, stuff like that.
 * @param e The Explosion to handle damage on.
 * @param parameter Unused parameter.
 */
static void Explosion_Func_TileDamage(Explosion *e, uint16 parameter)
{
	static const int16 craterIconMapIndex[] = { -1, 2, 1 };

	uint16 packed = Tile_PackTile(e->position);
	uint16 type;
	Tile *t;
	int16 iconMapIndex;
	uint16 overlaySpriteID;
	uint16 *iconMap;
	VARIABLE_NOT_USED(parameter);

	type = Map_GetLandscapeType(packed);

	if (type == LST_STRUCTURE || type == LST_DESTROYED_WALL) return;

	t = &g_map[packed];

	if (type == LST_CONCRETE_SLAB) {
		t->groundSpriteID = g_mapSpriteID[packed];
	}

	if (g_table_landscapeInfo[type].craterType == 0) return;

	overlaySpriteID = t->overlaySpriteID;
	iconMapIndex = craterIconMapIndex[g_table_landscapeInfo[type].craterType];
	iconMap = &g_iconMap[g_iconMap[iconMapIndex]];

	if (iconMap[0] <= overlaySpriteID && overlaySpriteID <= iconMap[10]) {
		/* There already is a crater; make it bigger */
		overlaySpriteID -= iconMap[0];
		if (overlaySpriteID < 4) overlaySpriteID += 2;
	} else {
		/* Randomly pick 1 of the 2 possible craters */
		overlaySpriteID = Tools_Random_256() & 1;
	}

	/* Reduce spice if there is any */
	Map_ChangeSpiceAmount(packed, -1);

	/* Boom a bloom if there is one */
	if (t->groundSpriteID == g_bloomSpriteID) {
		Map_Bloom_ExplodeSpice(packed, FLAG_HOUSE_ALL);
		return;
	}

	/* Update the tile with the crater */
	t->overlaySpriteID = overlaySpriteID + iconMap[0];
}

/**
 * Play a voice for a Explosion.
 * @param e The Explosion to play the voice on.
 * @param voiceID The voice to play.
 */
static void Explosion_Func_PlayVoice(Explosion *e, uint16 voiceID)
{
	Server_Send_PlaySoundAtTile(FLAG_HOUSE_ALL, voiceID, e->position);
}

/**
 * Shake the screen.
 * @param e The Explosion.
 * @param parameter Unused parameter.
 */
static void Explosion_Func_ScreenShake(Explosion *e, uint16 parameter)
{
	VARIABLE_NOT_USED(parameter);

	Server_Send_ScreenShake(Tile_PackTile(e->position));
}

/**
 * Check if there is a bloom at the location, and make it explode if needed.
 * @param e The Explosion to perform the explosion on.
 * @param parameter Unused parameter.
 */
static void Explosion_Func_BloomExplosion(Explosion *e, uint16 parameter)
{
	uint16 packed = Tile_PackTile(e->position);
	VARIABLE_NOT_USED(parameter);

	if (g_map[packed].groundSpriteID != g_bloomSpriteID) return;

	Map_Bloom_ExplodeSpice(packed, FLAG_HOUSE_ALL);
}

/**
 * Set the animation of a Explosion.
 * @param e The Explosion to change.
 * @param animationMapID The animation map to use.
 */
static void Explosion_Func_SetAnimation(Explosion *e, uint16 animationMapID)
{
	uint16 packed = Tile_PackTile(e->position);

	if (Structure_Get_ByPackedTile(packed) != NULL) return;

	animationMapID += Tools_Random_256() & 0x1;
	animationMapID += g_table_landscapeInfo[Map_GetLandscapeType(packed)].isSand ? 0 : 2;

	assert(animationMapID < 16);
	Animation_Start(g_table_animation_map[animationMapID], e->position, 0, HOUSE_HARKONNEN, 3);
}

/**
 * Set position at the left of a row.
 * @param e The Explosion to change.
 * @param row Row number.
 */
static void Explosion_Func_MoveYPosition(Explosion *e, uint16 row)
{
	e->position.y += (int16)row;
}

/**
 * Stop performing an explosion.
 * @param e The Explosion to end.
 * @param parameter Unused parameter.
 */
static void Explosion_Func_Stop(Explosion *e, uint16 parameter)
{
	uint16 packed = Tile_PackTile(e->position);
	VARIABLE_NOT_USED(parameter);

	g_map[packed].hasExplosion = false;

	Explosion_Update(e);

	e->commands = NULL;
}

/**
 * Set timeout for next the activity of \a e.
 * @param e The Explosion to change.
 * @param value The new timeout value.
 */
static void Explosion_Func_SetTimeout(Explosion *e, uint16 value)
{
	e->timeOut = Timer_GetTicks() + value;
}

/**
 * Set timeout for next the activity of \a e to a random value up to \a value.
 * @param e The Explosion to change.
 * @param value The maximum amount of timeout.
 */
static void Explosion_Func_SetRandomTimeout(Explosion *e, uint16 value)
{
	e->timeOut = Timer_GetTicks() + Tools_RandomLCG_Range(0, value);
}

/**
 * Set the SpriteID of the Explosion.
 * @param e The Explosion to change.
 * @param spriteID The new SpriteID for the Explosion.
 */
static void Explosion_Func_SetSpriteID(Explosion *e, uint16 spriteID)
{
	e->spriteID = spriteID;

	Explosion_Update(e);
}

/**
 * Stop any Explosion at position \a packed.
 * @param packed A packed position where no activities should take place (any more).
 */
static void Explosion_StopAtPosition(uint16 packed)
{
	if (!g_map[packed].hasExplosion) return;

	for (int i = 1; i < s_explosions.num_elem; i++) {
		Explosion *e = (Explosion *)BinHeap_GetElem(&s_explosions, i);

		if (e->commands == NULL) continue;
		if (Tile_PackTile(e->position) != packed) continue;

		Explosion_Func_Stop(e, 0);
		break;
	}
}

/*--------------------------------------------------------------*/

/* Toxic cloud of the Death Hand.  For a few seconds, ground units in the
 * cloud take damage every second, and gas puffs are drawn over it.
 */

#define TOXIC_MAX_CLOUDS 8
#define TOXIC_SECONDS 10
#define TOXIC_INTERVAL_SECONDS 2
#define TOXIC_DAMAGE 10     /* Per unit, every TOXIC_INTERVAL_SECONDS. */

typedef struct ToxicCloud {
	bool active;
	tile32 position;
	uint16 radius;          /* 1/16 tile units, like the explosion radius. */
	uint8 houseID;          /* The house that fired it, which colours the gas. */
	int64_t end;
	int64_t nextDamage;
	int64_t nextPuff;
} ToxicCloud;

static ToxicCloud s_toxic[TOXIC_MAX_CLOUDS];
static FILE *s_testLog;

/* Test only: how far the Death Hand impacts land from the target. */
static tile32 s_testTarget;
static bool s_testTargetSet;
static double s_testErrorSum;
static int s_testImpacts;

static void
Explosion_TestLog(const char *fmt, ...)
{
	va_list args;

	if (s_testLog == NULL)
		return;

	va_start(args, fmt);
	vfprintf(s_testLog, fmt, args);
	va_end(args);
	fflush(s_testLog);
}

void
Explosion_ToxicCloud_Add(tile32 position, uint16 radius, uint8 houseID)
{
	ToxicCloud *cloud = NULL;

	for (int i = 0; i < TOXIC_MAX_CLOUDS; i++) {
		if (!s_toxic[i].active) {
			cloud = &s_toxic[i];
			break;
		}

		if (cloud == NULL || s_toxic[i].end < cloud->end)
			cloud = &s_toxic[i];
	}

	cloud->active = true;
	cloud->position = position;
	cloud->radius = radius;
	cloud->houseID = houseID;
	cloud->end = g_timerGame + Tools_AdjustToGameSpeed(60 * TOXIC_SECONDS, 1, 0xFFFF, true);
	cloud->nextDamage = g_timerGame + Tools_AdjustToGameSpeed(60 * TOXIC_INTERVAL_SECONDS, 1, 0xFFFF, true);
	cloud->nextPuff = g_timerGame;

	Explosion_TestLog("[toxic] cloud added: radius %u (1/16 tiles) at tile %u,%u, house %u, ends in %d ticks\n",
			radius, Tile_GetPackedX(Tile_PackTile(position)), Tile_GetPackedY(Tile_PackTile(position)),
			houseID, (int)(cloud->end - g_timerGame));

	if (s_testTargetSet) {
		const double error = Tile_GetDistance(position, s_testTarget) / 256.0;

		s_testErrorSum += error;
		s_testImpacts++;
		Explosion_TestLog("[test] impact %d landed %.2f tiles from the target\n", s_testImpacts, error);
	}
}

int
Explosion_ToxicCloud_Count(void)
{
	int count = 0;

	for (int i = 0; i < TOXIC_MAX_CLOUDS; i++) {
		if (s_toxic[i].active)
			count++;
	}

	return count;
}

static void
Explosion_ToxicCloud_Tick(void)
{
	for (int i = 0; i < TOXIC_MAX_CLOUDS; i++) {
		ToxicCloud *cloud = &s_toxic[i];

		if (!cloud->active)
			continue;

		if (g_timerGame >= cloud->end) {
			cloud->active = false;
			Explosion_TestLog("[toxic] cloud expired\n");
			continue;
		}

		/* Gas puffs: the deviator gas explosion is only a visual, the
		 * deviating itself is done elsewhere.  The gas takes the colour of
		 * the house that fired the missile.
		 */
		if (g_timerGame >= cloud->nextPuff) {
			cloud->nextPuff = g_timerGame + Tools_AdjustToGameSpeed(15, 1, 0xFFFF, true);

			for (int p = 0; p < 3; p++)
				Explosion_Start(EXPLOSION_DEVIATOR_GAS, Tile_MoveByRandom(cloud->position, cloud->radius, false), cloud->houseID);
		}

		if (g_timerGame >= cloud->nextDamage) {
			PoolFindStruct find;

			cloud->nextDamage += Tools_AdjustToGameSpeed(60 * TOXIC_INTERVAL_SECONDS, 1, 0xFFFF, true);

			for (Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
					u != NULL;
					u = Unit_FindNext(&find)) {
				const UnitInfo *ui = &g_table_unitInfo[u->o.type];

				if (u->o.flags.s.isNotOnMap || !ui->flags.isNormalUnit || u->o.type == UNIT_SANDWORM)
					continue;

				if (ui->movementType == MOVEMENT_WINGER)
					continue;

				if ((Tile_GetDistance(cloud->position, u->o.position) >> 4) >= cloud->radius)
					continue;

				Explosion_TestLog("[toxic] pulse hits unit %u type %u hp %u\n",
						u->o.index, u->o.type, u->o.hitpoints);

				/* Infantry can't survive the gas at all. */
				Unit_Damage(u, (ui->movementType == MOVEMENT_FOOT) ? u->o.hitpoints : TOXIC_DAMAGE, 0);
			}
		}
	}
}

/*--------------------------------------------------------------*/

/* Debug tests, run with a mission start such as
 *   --house atreides --mission 5 --test-deathhand
 *   --house atreides --mission 5 --test-deathhand-cy
 *
 * They fire a real Death Hand missile and log what happens to deathhand_test.log,
 * then quit.  The first puts tanks and infantry at various distances from the
 * target, the second targets the player's construction yard.
 */

#define TEST_UNITS 7
#define TEST_MISSILES 1

void
Explosion_DebugTestDeathHand(void)
{
	static int state = 0;
	static int64_t stateTick;
	static uint16 unitIndex[TEST_UNITS];
	static const enum UnitType unitType[TEST_UNITS] = {
		UNIT_TANK, UNIT_TANK, UNIT_TANK, UNIT_TANK, UNIT_TANK, UNIT_INFANTRY, UNIT_INFANTRY
	};
	static const int unitDistance[TEST_UNITS] = { 0, 1, 3, 5, 12, 0, 3 }; /* tiles from the target. */
	static const int unitRow[TEST_UNITS] = { 0, 0, 0, 0, 0, 2, 2 };       /* tiles south of the target. */
	static uint16 structureIndex = 0xFFFF;
	static int64_t nextReport;
	static int64_t impactTick;
	static bool screenshotTaken;
	const bool structureTest = (g_debugTestDeathHand == 2);

	if (state == 0) {
		s_testLog = fopen("deathhand_test.log", "w");
		stateTick = g_timerGame;
		state = 1;
		Explosion_TestLog("[test] started (%s)\n", structureTest ? "construction yard" : "units");
		return;
	}

	/* Wait a couple of seconds for the game to settle. */
	if (state == 1) {
		PoolFindStruct find;
		tile32 target;

		if (g_timerGame - stateTick < 120)
			return;

		if (structureTest) {
			const Structure *cy = Structure_FindFirst(&find, g_playerHouseID, STRUCTURE_CONSTRUCTION_YARD);

			if (cy == NULL) {
				Explosion_TestLog("[test] the player has no construction yard\n");
				state = 99;
				return;
			}

			structureIndex = cy->o.index;

			/* The yard is 2x2 tiles, its position is the top left one. */
			target = cy->o.position;
			target.x += 256;
			target.y += 256;

			Explosion_TestLog("[test] construction yard %u at tile %u,%u: %u hitpoints (max %u), targeting its centre\n",
					cy->o.index, Tile_GetPackedX(Tile_PackTile(cy->o.position)), Tile_GetPackedY(Tile_PackTile(cy->o.position)),
					cy->o.hitpoints, g_table_structureInfo[cy->o.type].o.hitpoints);
		} else {
			const Unit *anchor = Unit_FindFirst(&find, g_playerHouseID, UNIT_INVALID);
			const Structure *anchorS = Structure_FindFirst(&find, g_playerHouseID, STRUCTURE_INVALID);
			tile32 base;

			if (anchor != NULL) {
				base = anchor->o.position;
			} else if (anchorS != NULL) {
				base = anchorS->o.position;
			} else {
				Explosion_TestLog("[test] no player unit or structure to anchor on\n");
				state = 99;
				return;
			}

			const int bx = Tile_GetPackedX(Tile_PackTile(base)) + 8;
			const int by = Tile_GetPackedY(Tile_PackTile(base));

			for (int i = 0; i < TEST_UNITS; i++) {
				const uint16 packed = Tile_PackXY(bx + unitDistance[i], by + unitRow[i]);
				Unit *u = Unit_Create(UNIT_INDEX_INVALID, unitType[i], g_playerHouseID, Tile_Center(Tile_UnpackTile(packed)), 0);

				unitIndex[i] = (u != NULL) ? u->o.index : UNIT_INDEX_INVALID;
				Explosion_TestLog("[test] %s %d at %d tiles from target (tile %d,%d): %s (unit %u)\n",
						(unitType[i] == UNIT_TANK) ? "tank" : "infantry", i, unitDistance[i],
						bx + unitDistance[i], by + unitRow[i],
						(u != NULL) ? "created" : "FAILED to create", unitIndex[i]);
			}

			target = Tile_Center(Tile_UnpackTile(Tile_PackXY(bx, by)));
		}

		/* Fire the real missiles from 24 tiles away, a typical launch distance. */
		const int tx = Tile_GetPackedX(Tile_PackTile(target));
		const int ty = Tile_GetPackedY(Tile_PackTile(target));
		const tile32 start = Tile_Center(Tile_UnpackTile(Tile_PackXY(tx, (ty >= 24) ? ty - 24 : 0)));
		const uint16 encoded = Tools_Index_Encode(Tile_PackTile(target), IT_TILE);

		/* Several missiles at the same target, to measure the accuracy. */
		s_testTarget = target;
		s_testTargetSet = true;

		/* Show the impact, for the screenshot. */
		Map_CentreViewport(TILE_SIZE * tx + TILE_SIZE / 2, TILE_SIZE * ty + TILE_SIZE / 2);

		for (int m = 0; m < TEST_MISSILES; m++) {
			Unit *missile = Unit_CreateBullet(start, UNIT_MISSILE_HOUSE, g_playerHouseID, 500, encoded);
			Explosion_TestLog("[test] missile %d %s\n", m, (missile != NULL) ? "launched" : "FAILED to launch");
		}

		stateTick = g_timerGame;
		nextReport = g_timerGame;
		state = 2;
		return;
	}

	if (state == 99) {
		fclose(s_testLog);
		s_testLog = NULL;
		exit(1);
	}

	if (Explosion_ToxicCloud_Count() > 0 && impactTick == 0)
		impactTick = g_timerGame;

	/* A screenshot of the cloud, saved to the personal data directory. */
	if (impactTick != 0 && !screenshotTaken && g_timerGame - impactTick > 150) {
		screenshotTaken = true;
		VideoA5_CaptureScreenshot();
		Explosion_TestLog("[test] screenshot requested\n");
	}

	/* Report every second until the cloud has gone. */
	if (g_timerGame >= nextReport) {
		nextReport = g_timerGame + 60;

		Explosion_TestLog("[test] t=%d clouds=%d", (int)((g_timerGame - stateTick) / 60), Explosion_ToxicCloud_Count());

		if (structureTest) {
			const Structure *s = (structureIndex != 0xFFFF) ? Structure_Get_ByIndex(structureIndex) : NULL;

			if (s != NULL && s->o.flags.s.used && s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
				Explosion_TestLog(" construction yard hp: %u\n", s->o.hitpoints);
			} else {
				Explosion_TestLog(" construction yard: destroyed\n");
			}
		} else {
			Explosion_TestLog(" hp:");
			for (int i = 0; i < TEST_UNITS; i++) {
				const Unit *u = (unitIndex[i] != UNIT_INDEX_INVALID) ? Unit_Get_ByIndex(unitIndex[i]) : NULL;

				if (u != NULL && u->o.flags.s.used && u->o.type == unitType[i]) {
					Explosion_TestLog(" %u", u->o.hitpoints);
				} else {
					Explosion_TestLog(" dead");
				}
			}
			Explosion_TestLog("\n");
		}
	}

	/* Done a few seconds after the cloud expired, or after a minute and a half. */
	if ((impactTick != 0 && Explosion_ToxicCloud_Count() == 0 && g_timerGame - impactTick > 60 * 8)
			|| g_timerGame - stateTick > 60 * 90) {
		Explosion_TestLog("[test] finished: %d impacts, mean error %.2f tiles\n",
				s_testImpacts, (s_testImpacts > 0) ? s_testErrorSum / s_testImpacts : -1.0);
		fclose(s_testLog);
		s_testLog = NULL;
		exit(0);
	}
}

void
Explosion_Init(void)
{
	BinHeap_Init(&s_explosions, sizeof(Explosion));
	memset(s_toxic, 0, sizeof(s_toxic));
}

void
Explosion_Uninit(void)
{
	BinHeap_Free(&s_explosions);
}

/**
 * Start a Explosion on a tile.
 * @param explosionType Type of Explosion.
 * @param position The position to use for init.
 * @param houseID House from which the explosion originates. Determines deviator gas color.
 */
void Explosion_Start(uint16 explosionType, tile32 position, uint8 houseID)
{
	if (explosionType > EXPLOSION_SPICE_BLOOM_TREMOR) return;

	uint16 packed = Tile_PackTile(position);
	Explosion_StopAtPosition(packed);

	Explosion *e = BinHeap_Push(&s_explosions, Timer_GetTicks());
	if (e != NULL) {
		e->commands = g_table_explosion[explosionType];
		e->current  = 0;
		e->spriteID = 0;
		e->position = position;
		e->houseID = houseID;

		g_map[packed].hasExplosion = true;

		/* Do not unveil for sandworm eat and spice bloom explosion types.
		 * In multiplayer, only reveal nearby explosions.
		 */
		if (enhancement_fog_of_war
				&& explosionType != EXPLOSION_SANDWORM_SWALLOW
				&& explosionType != EXPLOSION_SPICE_BLOOM_TREMOR) {
			const enum HouseFlag houses
				= (g_campaign_selected == CAMPAIGNID_MULTIPLAYER)
				? Map_FindHousesInRadius(position, 9)
				: FLAG_HOUSE_ALL;

			Tile_RefreshFogInRadius(houses, UNVEILCAUSE_EXPLOSION,
					position, 1, false);
		}
	}
}

/**
 * Timer tick for explosions.
 */
void Explosion_Tick(void)
{
	const int64_t curr_ticks = Timer_GetTicks();

	Explosion_ToxicCloud_Tick();

	Explosion *e = BinHeap_GetMin(&s_explosions);
	while ((e != NULL) && (e->timeOut <= curr_ticks)) {
		while (e->commands != NULL) {
			uint16 parameter = e->commands[e->current].parameter;
			uint16 command   = e->commands[e->current].command;

			e->current++;

			switch (command) {
				default:
				case EXPLOSION_STOP:               Explosion_Func_Stop(e, parameter); break;

				case EXPLOSION_SET_SPRITE:         Explosion_Func_SetSpriteID(e, parameter); break;
				case EXPLOSION_SET_TIMEOUT:        Explosion_Func_SetTimeout(e, parameter); break;
				case EXPLOSION_SET_RANDOM_TIMEOUT: Explosion_Func_SetRandomTimeout(e, parameter); break;
				case EXPLOSION_MOVE_Y_POSITION:    Explosion_Func_MoveYPosition(e, parameter); break;
				case EXPLOSION_TILE_DAMAGE:        Explosion_Func_TileDamage(e, parameter); break;
				case EXPLOSION_PLAY_VOICE:         Explosion_Func_PlayVoice(e, parameter); break;
				case EXPLOSION_SCREEN_SHAKE:       Explosion_Func_ScreenShake(e, parameter); break;
				case EXPLOSION_SET_ANIMATION:      Explosion_Func_SetAnimation(e, parameter); break;
				case EXPLOSION_BLOOM_EXPLOSION:    Explosion_Func_BloomExplosion(e, parameter); break;
			}

			if (e->timeOut > curr_ticks)
				break;
		}

		/* Push explosion with updated time before pop. */
		if (e->commands == NULL) {
			BinHeap_Pop(&s_explosions);
		} else {
			BinHeap_UpdateMin(&s_explosions);
		}

		e = BinHeap_GetMin(&s_explosions);
	}
}

void
Explosion_Draw(void)
{
	for (int i = 1; i < s_explosions.num_elem; i++) {
		const Explosion *e = (const Explosion *)BinHeap_GetElem(&s_explosions, i);

		if (e->spriteID == 0) continue;

		const uint16 packed = Tile_PackTile(e->position);
		if (!Map_IsUnveiledToHouse(g_playerHouseID, packed))
			continue;

		int x, y;
		if (!Map_IsPositionInViewport(e->position, &x, &y))
			continue;

		Shape_DrawRemap(e->spriteID, e->houseID, x, y, 2, 0xC000);
	}
}

uint8
Explosion_Get_NumActive(void)
{
	return min(s_explosions.num_elem, 0xFF);
}

void
Explosion_Set_NumActive(int num)
{
	if (s_explosions.max_elem < num) {
		BinHeap_Resize(&s_explosions, num);
	}

	s_explosions.num_elem = num;
}

Explosion *
Explosion_Get_ByIndex(int i)
{
	assert(1 <= i && i < s_explosions.num_elem);

	return (Explosion *)BinHeap_GetElem(&s_explosions, i);
}
