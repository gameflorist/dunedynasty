/* $Id$ */

/** @file src/scenario.h %Scenario handling definitions. */

#ifndef SCENARIO_H
#define SCENARIO_H

#include "enumeration.h"

typedef struct Campaign {
	char name[32];
	char dir_name[128];
	enum HouseType house[3];
	bool intermission;

	/* Emblem tweaks. */
	unsigned int fame_cps[HOUSE_MAX];
	unsigned int mapmach_cps[HOUSE_MAX];
	unsigned int misc_cps[HOUSE_MAX];
} Campaign;

/**
 * Information about reinforcements in the scenario.
 */
typedef struct Reinforcement {
	uint16 unitID;                                          /*!< The Unit which is already created and ready to join the game. */
	uint16 locationID;                                      /*!< The location where the Unit will appear. */
	uint16 timeLeft;                                        /*!< In how many ticks the Unit will appear. */
	uint16 timeBetween;                                     /*!< In how many ticks the Unit will appear again if repeat is set. */
	uint16 repeat;                                          /*!< If non-zero, the Unit will appear every timeBetween ticks. */
} Reinforcement;

/**
 * Information about the current loaded scenario.
 */
typedef struct Scenario {
	uint16 score;                                           /*!< Base score. */
	uint16 winFlags;                                        /*!< BASIC/WinFlags. */
	uint16 loseFlags;                                       /*!< BASIC/LoseFlags. */
	uint32 mapSeed;                                         /*!< MAP/Seed. */
	uint16 mapScale;                                        /*!< BASIC/MapScale. 0 is 62x62, 1 is 32x32, 2 is 21x21. */
	uint16 timeOut;                                         /*!< BASIC/TimeOut. */
	char   pictureBriefing[14];                             /*!< BASIC/BriefPicture. */
	char   pictureWin[14];                                  /*!< BASIC/WinPicture. */
	char   pictureLose[14];                                 /*!< BASIC/LosePicture. */
	uint16 killedAllied;                                    /*!< Number of units lost by "You". */
	uint16 killedEnemy;                                     /*!< Number of units lost by "Enemy". */
	uint16 destroyedAllied;                                 /*!< Number of structures lost by "You". */
	uint16 destroyedEnemy;                                  /*!< Number of structures lost by "Enemy". */
	uint16 harvestedAllied;                                 /*!< Total amount of spice harvested by "You". */
	uint16 harvestedEnemy;                                  /*!< Total amount of spice harvested by "Enemy". */
	Reinforcement reinforcement[16];                        /*!< Reinforcement information. */
} Scenario;

extern Campaign *g_campaign_list;
extern int g_campaign_total;
extern int g_campaign_selected;

extern Scenario g_scenario;

extern Campaign *Campaign_Alloc(const char *dir_name);
extern void Campaign_ReadProfileIniExtensions(char *source);
extern void Campaign_Load(void);
extern bool Scenario_Load(uint16 scenarioID, uint8 houseID);

#endif /* SCENARIO_H */
