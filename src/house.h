/** @file src/house.h %House management definitions. */

#ifndef HOUSE_H
#define HOUSE_H

#include <inttypes.h>
#include "enumeration.h"
#include "types.h"
#include "table/sound.h"

/**
 * Types of special %House Weapons available in the game.
 */
enum HouseWeapon {
	HOUSE_WEAPON_MISSILE  = 1,
	HOUSE_WEAPON_FREMEN   = 2,
	HOUSE_WEAPON_SABOTEUR = 3,

	HOUSE_WEAPON_INVALID = 0xFF
};

enum HouseAlliance {
	HOUSEALLIANCE_BRAIN,    /* Alliance depends on scenario's brain setting. */
	HOUSEALLIANCE_ALLIES,
	HOUSEALLIANCE_ENEMIES,
};

/**
 * Flags for House structure
 */
typedef struct {
	BIT_U8 used:1;                                      /*!< The House is in use (no longer free in the pool). */
	BIT_U8 human:1;                                     /*!< The House is controlled by a human. */
	BIT_U8 doneFullScaleAttack:1;                       /*!< The House did his one time attack the human with everything we have. */
	BIT_U8 isAIActive:1;                                /*!< The House has been seen by the human, and everything now becomes active (Team attack, house missiles, rebuilding, ..). */
	BIT_U8 radarActivated:1;                            /*!< The radar is activated. */
	BIT_U8 unused_0020:3;                               /*!< Unused */
} HouseFlags;

/**
 * A %House as stored in the memory.
 */
typedef struct House {
	uint8  index;                                           /*!< The index of the House in the array. */
	uint16 harvestersIncoming;                              /*!< How many harvesters are waiting to be delivered. Only happens when we run out of Units to do it immediately. */
	HouseFlags flags;                                       /*!< General flags of the House. */
	uint16 unitCount;                                       /*!< Amount of units owned by House. */
	uint16 unitCountMax;                                    /*!< Maximum amount of units this House is allowed to have. */
	uint16 unitCountEnemy;                                  /*!< Amount of units owned by enemy. */
	uint16 unitCountAllied;                                 /*!< Amount of units owned by allies. */
	uint32 structuresBuilt;                                 /*!< The Nth bit active means the Nth structure type is built (one or more). */
	uint16 credits;                                         /*!< Amount of credits the House currently has. */
	uint16 creditsStorage;                                  /*!< Amount of credits the House can store. */
	uint16 powerProduction;                                 /*!< Amount of power the House produces. */
	uint16 powerUsage;                                      /*!< Amount of power the House requires. */
	uint16 windtrapCount;                                   /*!< Amount of windtraps the House currently has. */
	uint16 creditsQuota;                                    /*!< Quota house has to reach to win the mission. */
	tile32 palacePosition;                                  /*!< Position of the Palace. */
	uint16 timerUnitAttack;                                 /*!< Timer to count down when next 'unit approaching' message can be showed again. */
	uint16 timerSandwormAttack;                             /*!< Timer to count down when next 'sandworm approaching' message can be showed again. */
	uint16 timerStructureAttack;                            /*!< Timer to count down when next 'base is under attack' message can be showed again. */
	uint16 starportTimeLeft;                                /*!< How much time is left before starport transport arrives. */
	uint16 starportLinkedID;                                /*!< If there is a starport delivery, this indicates the first unit of the linked list. Otherwise it is 0xFFFF. */
	uint16 ai_structureRebuild[5][2];                       /*!< An array for the AI which stores the type and position of a destroyed structure, for rebuilding. */
} House;

/**
 * Static information per House type.
 */
typedef struct HouseInfo {
	const char *name;                                       /*!< Pointer to name of house. */
	uint16 toughness;                                       /*!< How though the House is. Gives probability of deviation and chance of retreating. */
	uint16 degradingChance;                                 /*!< On Unit create, this is the chance a Unit will be set to 'degrading'. */
	uint16 degradingAmount;                                 /*!< Amount of damage dealt to degrading Structures. */
	uint8  minimapColor;                                    /*!< The color used on the minimap. */
	uint16 specialCountDown;                                /*!< Time between activation of Special Weapon. */
	uint16 starportDeliveryTime;                            /*!< Time it takes for a starport delivery. */
	char prefixChar;                                        /*!< Char used as prefix for some filenames. */
	uint16 specialWeapon;                                   /*!< Which Special Weapon this House has. @see HouseWeapon. */
	uint16 musicWin;                                        /*!< Music played when you won a mission. */
	uint16 musicLose;                                       /*!< Music played when you lose a mission. */
	uint16 musicBriefing;                                   /*!< Music played during initial briefing of mission. */
	const char *voiceFilename;                              /*!< Pointer to filename with the voices of the house. */

	enum MentatID mentat;
	enum SampleSet sampleSet;

	union {
		void *deathhand; /* placeholder. */

		struct {
			enum HouseType owner;
			enum UnitType unit75; /* 75% */
			enum UnitType unit25; /* 25% */
		} fremen;

		struct {
			enum HouseType owner; /* e.g. uncontrollable saboteurs. */
			enum UnitType unit;
		} saboteur;
	} superWeapon;
} HouseInfo;

extern const HouseInfo g_table_houseInfo_original[HOUSE_MAX];
extern HouseInfo g_table_houseInfo[HOUSE_MAX];
extern enum HouseAlliance g_table_houseAlliance[HOUSE_MAX][HOUSE_MAX];
extern const enum HouseType g_table_houseRemap6to3[HOUSE_MAX];

extern House *g_playerHouse;
extern enum HouseType g_playerHouseID;
extern uint16 g_houseMissileCountdown;
extern uint16 g_playerCreditsNoSilo;
extern uint16 g_playerCredits;

extern void GameLoop_House(void);
extern uint8 House_StringToType(const char *name);
extern void House_EnsureHarvesterAvailable(uint8 houseID);
extern bool House_AreAllied(uint8 houseID1, uint8 houseID2);
extern bool House_UpdateRadarState(House *h);
extern void House_UpdateCreditsStorage(uint8 houseID);
extern void House_CalculatePowerAndCredit(struct House *h);
extern const char *House_GetWSAHouseFilename(uint8 houseID);

extern enum UnitType House_GetInfantrySquad(enum HouseType houseID);
extern enum UnitType House_GetLightVehicle(enum HouseType houseID);
extern enum UnitType House_GetIXVehicle(enum HouseType houseID);

#endif /* HOUSE_H */
