/** @file src/unit.h %Unit definitions. */

#ifndef UNIT_H
#define UNIT_H

#include <inttypes.h>
#include "enumeration.h"
#include "object.h"

enum {
	MAX_SELECTABLE_UNITS  = 32
};

/**
 * Directional information
 */
typedef struct dir24 {
	int8 speed;                                             /*!< Speed of direction change. */
	int8 target;                                            /*!< Target direction. */
	int8 current;                                           /*!< Current direction. */
} dir24;

/**
 * A Unit as stored in the memory.
 */
typedef struct Unit {
	Object o;                                               /*!< Common to Unit and Structures. */
	tile32 currentDestination;                              /*!< Where the Unit is currently going to. */
	uint16 originEncoded;                                   /*!< Encoded index, indicating the origin. */
	uint8  actionID;                                        /*!< Current action. */
	uint8  nextActionID;                                    /*!< Next action. */
	uint16 fireDelay;                                       /*!< Delay between firing. In Dune2 this is an uint8. */
	uint16 distanceToDestination;                           /*!< How much distance between where we are now and where currentDestination is. */
	uint16 targetAttack;                                    /*!< Target to attack (encoded index). */
	uint16 targetMove;                                      /*!< Target to move to (encoded index). */
	uint8  amount;                                          /*!< Meaning depends on type:
	                                                         * - Sandworm : units to eat before disappearing.
	                                                         * - Harvester : harvested spice.
	                                                         */
	uint8  deviated;                                        /*!< Strength of deviation. Zero if unit is not deviated. */
	uint8  deviatedHouse;                                   /*!< Which house it is deviated to. Only valid if 'deviated' is non-zero. */
	tile32 targetLast;                                      /*!< The last position of the Unit. Carry-alls will return the Unit here. */
	tile32 targetPreLast;                                   /*!< The position before the last position of the Unit. */
	dir24  orientation[2];                                  /*!< Orientation of the unit. [0] = base, [1] = top (turret, etc). */
	uint8  speedPerTick;                                    /*!< Every tick this amount is added; if over 255 Unit is moved. */
	uint8  speedRemainder;                                  /*!< Remainder of speedPerTick. */
	uint8  speed;                                           /*!< The amount to move when speedPerTick goes over 255. */
	uint8  movingSpeed;                                     /*!< The speed of moving as last set. */
	uint8  wobbleIndex;                                     /*!< At which wobble index the Unit currently is. */
	 int8  spriteOffset;                                    /*!< Offset of the current sprite for Unit. */
	uint8  blinkCounter;                                    /*!< If non-zero, it indicates how many more ticks this unit is blinking. */
	uint8  team;                                            /*!< If non-zero, unit is part of team. Value 1 means team 0, etc. */
	uint16 timer;                                           /*!< Timer used in animation, to count down when to do the next step. */
	uint8  route[14];                                       /*!< The current route the Unit is following. */

	tile32 lastPosition;
	bool permanentFollow;
	bool detonateAtTarget;
	bool deviationDecremented;
	enum SquadID squadID;
	enum SquadID aiSquad;
} Unit;

/**
 * Static information per Unit type.
 */
typedef struct UnitInfo {
	ObjectInfo o;                                           /*!< Common to UnitInfo and StructureInfo. */
	uint16 indexStart;                                      /*!< At Unit create, between this and indexEnd (including) a free index is picked. */
	uint16 indexEnd;                                        /*!< At Unit create, between indexStart and this (including) a free index is picked. */
	struct {
		BIT_U8 isBullet:1;                                  /*!< If true, Unit is a bullet / missile. */
		BIT_U8 explodeOnDeath:1;                            /*!< If true, Unit exploses when dying. */
		BIT_U8 sonicProtection:1;                           /*!< If true, Unit receives no damage of a sonic blast. */
		BIT_U8 canWobble:1;                                 /*!< If true, Unit will wobble around while moving on certain tiles. */
		BIT_U8 isTracked:1;                                 /*!< If true, Unit is tracked-based (and leaves marks in sand). */
		BIT_U8 isGroundUnit:1;                              /*!< If true, Unit is ground-based. */
		BIT_U8 mustStayInMap:1;                             /*!< Unit cannot leave the map and bounces off the border (air-based units). */
		BIT_U8 firesTwice:1;                                /*!< If true, Unit fires twice. */
		BIT_U8 impactOnSand:1;                              /*!< If true, hitting sand (as bullet / missile) makes an impact (crater-like). */
		BIT_U8 isNotDeviatable:1;                           /*!< If true, Unit can't be deviated. */
		BIT_U8 hasAnimationSet:1;                           /*!< If true, the Unit has two set of sprites for animation. */
		BIT_U8 notAccurate:1;                               /*!< If true, Unit is a bullet and is not very accurate at hitting the target (rockets). */
		BIT_U8 isNormalUnit:1;                              /*!< If true, Unit is a normal unit (not a bullet / missile, nor a sandworm / frigate). */
	} flags;                                                /*!< General flags of the UnitInfo. */
	uint16 dimension;                                       /*!< The dimension of the Unit Sprite. */
	uint16 movementType;                                    /*!< MovementType of Unit. */
	uint16 animationSpeed;                                  /*!< Speed of sprite animation of Unit. */
	uint16 movingSpeedFactor;                               /*!< Factor speed of movement of Unit, where 256 is full speed. */
	uint8  turningSpeed;                                    /*!< Speed of orientation change of Unit. */
	uint16 groundSpriteID;                                  /*!< SpriteID for north direction. */
	uint16 turretSpriteID;                                  /*!< SpriteID of the turret for north direction. */
	uint16 actionAI;                                        /*!< Default action for AI units. */
	uint16 displayMode;                                     /*!< How to draw the Unit. */
	uint16 destroyedSpriteID;                               /*!< SpriteID of burning Unit for north direction. Can be zero if no such animation. */
	uint16 fireDelay;                                       /*!< Time between firing at Normal speed. */
	uint16 fireDistance;                                    /*!< Maximal distance this Unit can fire from. */
	uint16 damage;                                          /*!< Damage this Unit does to other Units. */
	uint16 explosionType;                                   /*!< Type of the explosion of Unit. */
	uint8  bulletType;                                      /*!< Type of the bullets of Unit. */
	uint16 bulletSound;                                     /*!< Sound for the bullets. */
} UnitInfo;

/**
 * Static information per Action type.
 */
typedef struct ActionInfo {
	uint16 stringID;                                        /*!< StringID of Action name. */
	const char *name;                                       /*!< Name of Action. */
	uint16 switchType;                                      /*!< When going to new mode, how do we handle it? 0: queue if needed, 1: change immediately, 2: run via subroutine. */
	uint16 selectionType;                                   /*!< Selection type attached to this action. */
	uint16 soundID;                                         /*!< The sound played when unit is a Foot unit. */
} ActionInfo;

struct Team;
struct Structure;

extern const char * const g_table_movementTypeName[MOVEMENT_MAX];

extern const uint16 g_table_actionsAI[4];
extern const ActionInfo g_table_actionInfo[ACTION_MAX];
extern const UnitInfo g_table_unitInfo_original[UNIT_MAX];
extern UnitInfo g_table_unitInfo[UNIT_MAX];

extern Unit *g_unitActive;
extern Unit *g_unitHouseMissile;
extern int16 g_starportAvailable[UNIT_MAX];

extern Unit *Unit_FirstSelected(int *iter);
extern Unit *Unit_NextSelected(int *iter);
extern bool Unit_IsSelected(const Unit *unit);
extern bool Unit_AnySelected(void);
extern void Unit_AddSelected(Unit *unit);
extern void Unit_Unselect(const Unit *unit);
extern void Unit_UnselectAll(void);
extern Unit *Unit_GetForActionPanel(void);
extern tile32 Unit_GetNextDestination(const Unit *u);

extern void GameLoop_Unit(void);
extern uint8 Unit_GetHouseID(const Unit *u);
extern uint8 Unit_StringToType(const char *name);
extern uint8 Unit_ActionStringToType(const char *name);
extern uint8 Unit_MovementStringToType(const char *name);
extern struct Unit *Unit_Create(uint16 index, uint8 typeID, uint8 houseID, tile32 position, int8 orientation);
extern bool Unit_IsTypeOnMap(uint8 houseID, uint8 typeID);
extern void Unit_SetAction(Unit *u, ActionType action);
extern uint16 Unit_AddToTeam(Unit *u, struct Team *t);
extern uint16 Unit_RemoveFromTeam(Unit *u);
extern struct Team *Unit_GetTeam(Unit *u);
extern void Unit_Sort(void);
extern Unit *Unit_Get_ByPackedTile(uint16 packed);
extern uint16 Unit_IsValidMovementIntoStructure(Unit *unit, struct Structure *s);
extern void Unit_SetDestination(Unit *u, uint16 destination);
extern uint16 Unit_FindClosestRefinery(Unit *unit);
extern bool Unit_SetPosition(Unit *u, tile32 position);
extern void Unit_Remove(Unit *u);
extern bool Unit_StartMovement(Unit *unit);
extern void Unit_SetTarget(Unit* unit, uint16 encoded);
extern bool Unit_Deviation_Decrease(Unit* unit, uint16 amount);
extern void Unit_RefreshFog(Unit *unit, bool unveil);
extern void Unit_RemoveFog(Unit *unit);
extern bool Unit_Deviate(Unit *unit, uint16 probability, uint8 houseID);
extern bool Unit_Move(Unit *unit, uint16 distance);
extern bool Unit_Damage(Unit *unit, uint16 damage, uint16 range);
extern void Unit_UntargetEncodedIndex(uint16 encoded);
extern void Unit_UntargetMe(Unit *unit);
extern void Unit_SetOrientation(Unit *unit, int8 orientation, bool rotateInstantly, uint16 level);
extern void Unit_Select(Unit *unit);
extern Unit *Unit_CreateWrapper(uint8 houseID, enum UnitType type, uint16 location);
extern uint16 Unit_FindTargetAround(uint16 packed);
extern bool Unit_IsTileOccupied(Unit *unit);
extern void Unit_SetSpeed(Unit *unit, uint16 speed);
extern Unit *Unit_CreateBullet(tile32 position, enum UnitType type, uint8 houseID, uint16 damage, uint16 target);
extern void Unit_DisplayStatusText(const Unit *unit);
extern void Unit_DisplayGroupStatusText(void);
extern void Unit_Hide(Unit *unit);
extern Unit *Unit_CallUnitByType(enum UnitType type, uint8 houseID, uint16 target, bool createCarryall);
extern void Unit_EnterStructure(Unit *unit, struct Structure *s);
extern int16 Unit_GetTileEnterScore(Unit *unit, uint16 packed, uint16 direction);
extern void Unit_RemovePlayer(Unit *unit);
extern void Unit_UpdateMap(uint16 type, Unit *unit);
extern void Unit_RemoveFromTile(Unit *unit, uint16 packed);
extern void Unit_AddToTile(Unit *unit, uint16 packed);
extern void Unit_LaunchHouseMissile(const struct Structure *s, uint16 packed);
extern void Unit_HouseUnitCount_Remove(Unit *unit);
extern void Unit_HouseUnitCount_Add(Unit *unit, uint8 houseID);

extern uint16 Unit_GetTargetUnitPriority(Unit *unit, Unit *target);
extern uint16 Unit_GetTargetStructurePriority(Unit *unit, struct Structure *s);
extern uint16 Unit_FindBestTargetEncoded(Unit *unit, uint16 mode);
extern Unit *Unit_FindBestTargetUnit(Unit *u, uint16 mode);
extern Unit *Unit_Sandworm_FindBestTarget(Unit *unit);

#endif /* UNIT_H */
