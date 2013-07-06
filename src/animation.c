/** @file src/animation.c %Animation routines. */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

#include "animation.h"

#include "audio/audio.h"
#include "binheap.h"
#include "map.h"
#include "sprites.h"
#include "structure.h"
#include "timer/timer.h"
#include "tools/coord.h"
#include "tools/random_general.h"

typedef struct Animation {
	/* Heap key. */
	int64_t tickNext;                       /*!< Which tick this Animation should be called again. */

	enum StructureLayout tileLayout;        /*!< Tile layout of the Animation. */
	enum HouseType houseID;                 /*!< House of the item being animated. */
	uint8 current;                          /*!< At which command we currently are in the Animation. */
	uint8 iconGroup;                        /*!< Which iconGroup the sprites of the Animation belongs. */
	const AnimationCommandStruct *commands; /*!< List of commands for this Animation. */
	tile32 tile;                            /*!< Top-left tile of Animation. */
} Animation;

static BinHeap s_animations;

/**
 * Stop with this Animation.
 * @param animation The Animation to stop.
 * @param parameter Not used.
 */
static void Animation_Func_Stop(Animation *animation, int16 parameter)
{
	const uint16 *layout = g_table_structure_layoutTiles[animation->tileLayout];
	uint16 layoutTileCount = g_table_structure_layoutTileCount[animation->tileLayout];
	uint16 packed = Tile_PackTile(animation->tile);
	VARIABLE_NOT_USED(parameter);

	g_map[packed].hasAnimation = false;
	animation->commands = NULL;

	for (int i = 0; i < layoutTileCount; i++) {
		uint16 position = packed + (*layout++);
		Tile *t = &g_map[position];

		if (animation->tileLayout != 0) {
			t->groundSpriteID = g_mapSpriteID[position];
		}

		if (Map_IsPositionUnveiled(position)) {
			t->overlaySpriteID = 0;
		}

		Map_Update(position, 0, false);
	}
}


/**
 * Abort this Animation.
 * @param animation The Animation to abort.
 * @param parameter Not used.
 */
static void Animation_Func_Abort(Animation *animation, int16 parameter)
{
	uint16 packed = Tile_PackTile(animation->tile);
	VARIABLE_NOT_USED(parameter);

	g_map[packed].hasAnimation = false;
	animation->commands = NULL;

	Map_Update(packed, 0, false);
}

/**
 * Pause the animation for a few ticks.
 * @param animation The Animation to pause.
 * @param parameter How many ticks it should pause.
 * @note Delays are randomly delayed with [0..3] ticks.
 */
static void Animation_Func_Pause(Animation *animation, int16 parameter)
{
	assert(parameter >= 0);

	animation->tickNext = Timer_GetTicks() + parameter + (Tools_Random_256() % 4);
}

/**
 * Set the overlay sprite of the tile.
 * @param animation The Animation for which we change the overlay sprite.
 * @param parameter The SpriteID to which the overlay sprite is set.
 */
static void Animation_Func_SetOverlaySprite(Animation *animation, int16 parameter)
{
	uint16 packed = Tile_PackTile(animation->tile);
	assert(parameter >= 0);

	if (!Map_IsPositionUnveiled(packed)) return;

	Tile *t = &g_map[packed];
	t->overlaySpriteID = g_iconMap[g_iconMap[animation->iconGroup] + parameter];
	t->houseID = animation->houseID;

	Map_Update(packed, 0, false);
}

/**
 * Rewind the animation.
 * @param animation The Animation to rewind.
 * @param parameter Not used.
 */
static void Animation_Func_Rewind(Animation *animation, int16 parameter)
{
	VARIABLE_NOT_USED(parameter);

	animation->current = 0;
}

/**
 * Set the ground sprite of the tile.
 * @param animation The Animation for which we change the ground sprite.
 * @param parameter The offset in the iconGroup to which the ground sprite is set.
 */
static void Animation_Func_SetGroundSprite(Animation *animation, int16 parameter)
{
	const uint16 *layout = g_table_structure_layoutTiles[animation->tileLayout];
	uint16 layoutTileCount = g_table_structure_layoutTileCount[animation->tileLayout];
	uint16 packed = Tile_PackTile(animation->tile);

	uint16 specialMap[1];
	uint16 *iconMap;

	iconMap = &g_iconMap[g_iconMap[animation->iconGroup] + layoutTileCount * parameter];

	/* Some special case for turrets */
	if ((parameter > 1) &&
		(animation->iconGroup == ICM_ICONGROUP_BASE_DEFENSE_TURRET ||
		 animation->iconGroup == ICM_ICONGROUP_BASE_ROCKET_TURRET)) {
		Structure *s = Structure_Get_ByPackedTile(packed);
		assert(s != NULL);
		assert(layoutTileCount == 1);

		specialMap[0] = s->rotationSpriteDiff + g_iconMap[g_iconMap[animation->iconGroup]] + 2;
		iconMap = specialMap;
	}

	for (int i = 0; i < layoutTileCount; i++) {
		uint16 position = packed + (*layout++);
		uint16 spriteID = *iconMap++;
		Tile *t = &g_map[position];

		if (t->groundSpriteID == spriteID) continue;
		t->groundSpriteID = spriteID;
		t->houseID = animation->houseID;

		if (Map_IsPositionUnveiled(position)) {
			t->overlaySpriteID = 0;
		}

		Map_Update(position, 0, false);
	}
}

/**
 * Forward the current Animation with the given amount of steps.
 * @param animation The Animation to forward.
 * @param parameter With what value you want to forward the Animation.
 * @note Forwarding with 1 is just the next instruction, making this command a NOP.
 */
static void Animation_Func_Forward(Animation *animation, int16 parameter)
{
	animation->current += parameter - 1;
}

/**
 * Set the IconGroup of the Animation.
 * @param animation The Animation to change.
 * @param parameter To what value IconGroup should change.
 */
static void Animation_Func_SetIconGroup(Animation *animation, int16 parameter)
{
	assert(parameter >= 0);

	animation->iconGroup = (uint8)parameter;
}

/**
 * Play a Voice on the tile of animation.
 * @param animation The Animation which gives the position the voice plays at.
 * @param parameter The VoiceID to play.
 */
static void Animation_Func_PlayVoice(Animation *animation, int16 parameter)
{
	Audio_PlaySoundAtTile(parameter, animation->tile);
}

void
Animation_Init(void)
{
	BinHeap_Init(&s_animations, sizeof(Animation));
}

void
Animation_Uninit(void)
{
	BinHeap_Free(&s_animations);
}

/**
 * Start an Animation.
 * @param commands List of commands for the Animation.
 * @param tile The tile to do the Animation on.
 * @param layout The layout of tiles for the Animation.
 * @param houseID The house of the item being Animation.
 * @param iconGroup In which IconGroup the sprites of the Animation belongs.
 */
void Animation_Start(const AnimationCommandStruct *commands, tile32 tile, uint16 tileLayout, uint8 houseID, uint8 iconGroup)
{
	uint16 packed = Tile_PackTile(tile);
	Animation_Stop_ByTile(packed);

	Animation *animation = BinHeap_Push(&s_animations, Timer_GetTicks());
	if (animation != NULL) {
		animation->tileLayout = tileLayout;
		animation->houseID    = houseID;
		animation->current    = 0;
		animation->iconGroup  = iconGroup;
		animation->commands   = commands;
		animation->tile       = tile;

		g_map[packed].houseID = houseID;
		g_map[packed].hasAnimation = true;
	}
}

/**
 * Stop an Animation on a tile, if any.
 * @param packed The tile to check for animation on.
 */
void Animation_Stop_ByTile(uint16 packed)
{
	if (!g_map[packed].hasAnimation) return;

	for (int i = 1; i < s_animations.num_elem; i++) {
		Animation *animation = (Animation *)BinHeap_GetElem(&s_animations, i);

		if (animation->commands == NULL) continue;
		if (Tile_PackTile(animation->tile) != packed) continue;

		Animation_Func_Stop(animation, 0);
		break;
	}
}

/**
 * Check all Animations if they need changing.
 */
void Animation_Tick(void)
{
	const int64_t curr_ticks = Timer_GetTicks();

	Animation *animation = BinHeap_GetMin(&s_animations);
	while ((animation != NULL) && (animation->tickNext <= curr_ticks)) {
		while (animation->commands != NULL) {
			const AnimationCommandStruct *commands = animation->commands + animation->current;
			int16 parameter = commands->parameter;
			assert((parameter & 0x0800) == 0 || (parameter & 0xF000) != 0); /* Validate if the compiler sign-extends correctly */

			animation->current++;

			switch (commands->command) {
				case ANIMATION_STOP:
				default:                           Animation_Func_Stop(animation, parameter); break;

				case ANIMATION_ABORT:              Animation_Func_Abort(animation, parameter); break;
				case ANIMATION_SET_OVERLAY_SPRITE: Animation_Func_SetOverlaySprite(animation, parameter); break;
				case ANIMATION_PAUSE:              Animation_Func_Pause(animation, parameter); break;
				case ANIMATION_REWIND:             Animation_Func_Rewind(animation, parameter); break;
				case ANIMATION_PLAY_VOICE:         Animation_Func_PlayVoice(animation, parameter); break;
				case ANIMATION_SET_GROUND_SPRITE:  Animation_Func_SetGroundSprite(animation, parameter); break;
				case ANIMATION_FORWARD:            Animation_Func_Forward(animation, parameter); break;
				case ANIMATION_SET_ICONGROUP:      Animation_Func_SetIconGroup(animation, parameter); break;
			}

			if (animation->tickNext > curr_ticks)
				break;
		}

		if (animation->commands == NULL) {
			BinHeap_Pop(&s_animations);
		}
		else {
			BinHeap_UpdateMin(&s_animations);
		}

		animation = BinHeap_GetMin(&s_animations);
	}
}
