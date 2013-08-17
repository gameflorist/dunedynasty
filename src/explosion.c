/** @file src/explosion.c Explosion routines. */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "os/math.h"

#include "explosion.h"

#include "animation.h"
#include "binheap.h"
#include "enhancement.h"
#include "house.h"
#include "map.h"
#include "net/server.h"
#include "sprites.h"
#include "structure.h"
#include "tile.h"
#include "timer/timer.h"
#include "tools/coord.h"
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
		Map_Update(packed, 0, false);
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
	Map_Update(packed, 0, false);
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

	/* ENHANCEMENT -- Screenshake even if explosion is only partially unveiled. */
	if (g_dune2_enhanced) {
		const Tile *t = &g_map[Tile_PackTile(e->position)];

		if (!t->isUnveiled)
			return;
	}
	else {
		if (!Map_IsPositionUnveiled(Tile_PackTile(e->position)))
			return;
	}

	GFX_ScreenShake_Start(1);
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

void
Explosion_Init(void)
{
	BinHeap_Init(&s_explosions, sizeof(Explosion));
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
 */
void Explosion_Start(uint16 explosionType, tile32 position)
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

		g_map[packed].hasExplosion = true;

		/* Do not unveil for explosion types 13 (sandworm eat) and 19 (spice bloom). */
		if (enhancement_fog_of_war &&
				!(explosionType == EXPLOSION_SANDWORM_SWALLOW || explosionType == EXPLOSION_SPICE_BLOOM_TREMOR)) {
			Tile_RefreshFogInRadius(position, 1, false);
		}
	}
}

/**
 * Timer tick for explosions.
 */
void Explosion_Tick(void)
{
	const int64_t curr_ticks = Timer_GetTicks();

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
		}
		else {
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
		if (!g_map[packed].isUnveiled)
			continue;

		int x, y;
		if (!Map_IsPositionInViewport(e->position, &x, &y))
			continue;

		Shape_Draw(e->spriteID, x, y, 2, 0xC000);
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
