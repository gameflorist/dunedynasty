/** @file src/house.c %House management routines. */

#include <stdio.h>
#include "os/math.h"
#include "os/strings.h"
#include "os/sleep.h"

#include "house.h"

#include "audio/audio.h"
#include "enhancement.h"
#include "gfx.h"
#include "gui/gui.h"
#include "gui/widget.h"
#include "map.h"
#include "newui/menubar.h"
#include "opendune.h"
#include "pool/pool.h"
#include "pool/house.h"
#include "pool/structure.h"
#include "pool/unit.h"
#include "scenario.h"
#include "string.h"
#include "structure.h"
#include "table/strings.h"
#include "tile.h"
#include "timer/timer.h"
#include "tools.h"
#include "unit.h"
#include "video/video.h"
#include "wsa.h"


House *g_playerHouse = NULL;
enum HouseType g_playerHouseID = HOUSE_INVALID;
uint16 g_houseMissileCountdown = 0;
uint16 g_playerCreditsNoSilo = 0;
uint16 g_playerCredits = 0; /*!< Credits shown to player as 'current'. */
int64_t g_tickHousePowerMaintenance = 0;

static int64_t s_tickHouseHouse = 0;
static int64_t s_tickHouseStarport = 0;
static int64_t s_tickHouseReinforcement = 0;
static int64_t s_tickHouseMissileCountdown = 0;
static int64_t s_tickHouseStarportAvailability = 0;

/**
 * Loop over all houses, preforming various of tasks.
 */
void GameLoop_House(void)
{
	PoolFindStruct find;
	House *h = NULL;
	bool tickHouse                = false;
	bool tickPowerMaintenance     = false;
	bool tickStarport             = false;
	bool tickReinforcement        = false;
	bool tickMissileCountdown     = false;
	bool tickStarportAvailability = false;

	if (g_debugScenario) return;

	if (s_tickHouseHouse <= g_timerGame) {
		tickHouse = true;
		s_tickHouseHouse = g_timerGame + 900;
	}

	if (g_tickHousePowerMaintenance <= g_timerGame) {
		tickPowerMaintenance = true;
		g_tickHousePowerMaintenance = g_timerGame + 10800;
	}

	if (s_tickHouseStarport <= g_timerGame) {
		tickStarport = true;
		s_tickHouseStarport = g_timerGame + 180;
	}

	if (s_tickHouseReinforcement <= g_timerGame) {
		tickReinforcement = true;
		s_tickHouseReinforcement = g_timerGame + (g_debugGame ? 60 : 600);
	}

	if (s_tickHouseMissileCountdown <= g_timerGame) {
		tickMissileCountdown = true;
		s_tickHouseMissileCountdown = g_timerGame + 60;
	}

	if (s_tickHouseStarportAvailability <= g_timerGame) {
		tickStarportAvailability = true;
		s_tickHouseStarportAvailability = g_timerGame + 1800;
	}

	if (tickMissileCountdown && g_houseMissileCountdown != 0) {
		g_houseMissileCountdown--;

		const enum VoiceID voiceID = VOICE_MISSILE_LAUNCHED + g_houseMissileCountdown - 1;

		/* Don't queue up the countdown numbers. */
		if (voiceID >= VOICE_ONE) {
			const bool narrator_speaking = Audio_Poll();

			if (!narrator_speaking) {
				Audio_PlayVoice(voiceID);
			}
			else if (voiceID == VOICE_FIVE) {
				Audio_PlayEffect(EFFECT_COUNT_DOWN_TICK);
			}
		}
		else if (voiceID >= VOICE_MISSILE_LAUNCHED) {
			Audio_PlayVoice(voiceID);
		}

		if (g_houseMissileCountdown == 0) Unit_LaunchHouseMissile(Map_FindLocationTile(4, g_playerHouseID));
	}

	if (tickStarportAvailability) {
		uint16 type;

		/* Pick a random unit to increase starport availability */
		type = Tools_RandomLCG_Range(0, UNIT_MAX - 1);

		/* Increase how many of this unit is available via starport by one */
		if (g_starportAvailable[type] != 0 && g_starportAvailable[type] < 10) {
			if (g_starportAvailable[type] == -1) {
				g_starportAvailable[type] = 1;
			} else {
				g_starportAvailable[type]++;
			}
		}
	}

	if (tickReinforcement) {
		Unit *nu = NULL;
		int i;

		for (i = 0; i < 16; i++) {
			uint16 locationID;
			bool deployed;
			Unit *u;

			if (g_scenario.reinforcement[i].unitID == UNIT_INDEX_INVALID) continue;
			if (g_scenario.reinforcement[i].timeLeft == 0) continue;
			if (--g_scenario.reinforcement[i].timeLeft != 0) continue;

			u = Unit_Get_ByIndex(g_scenario.reinforcement[i].unitID);

			locationID = g_scenario.reinforcement[i].locationID;
			deployed   = false;

			if (locationID >= 4) {
				if (nu == NULL) {
					nu = Unit_Create(UNIT_INDEX_INVALID, UNIT_CARRYALL, u->o.houseID, Tile_UnpackTile(Map_FindLocationTile(Tools_Random_256() & 3, u->o.houseID)), 100);

					if (nu != NULL) {
						nu->o.flags.s.byScenario = true;
						Unit_SetDestination(nu, Tools_Index_Encode(Map_FindLocationTile(locationID, u->o.houseID), IT_TILE));
					}
				}

				if (nu != NULL) {
					u->o.linkedID = nu->o.linkedID;
					nu->o.linkedID = (uint8)u->o.index;
					nu->o.flags.s.inTransport = true;
					g_scenario.reinforcement[i].unitID = UNIT_INDEX_INVALID;
					deployed = true;
				} else {
					/* Failed to create carry-all, try again in a short moment */
					g_scenario.reinforcement[i].timeLeft = 1;
				}
			} else {
				deployed = Unit_SetPosition(u, Tile_UnpackTile(Map_FindLocationTile(locationID, u->o.houseID)));
			}

			if (deployed && g_scenario.reinforcement[i].repeat != 0) {
				tile32 tile;
				tile.tile = 0xFFFFFFFF;

				g_validateStrictIfZero++;
				u = Unit_Create(UNIT_INDEX_INVALID, u->o.type, u->o.houseID, tile, 0);
				g_validateStrictIfZero--;

				if (u != NULL) {
					g_scenario.reinforcement[i].unitID = u->o.index;
					g_scenario.reinforcement[i].timeLeft = g_scenario.reinforcement[i].timeBetween;
				}
			}
		}
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		h = House_Find(&find);
		if (h == NULL) break;

		if (tickHouse) {
			/* ENHANCEMENT -- Originally this code was outside the house loop, which seems very odd.
			 *  This problem is considered to be so bad, that the original code has been removed. */
			if (h->index != g_playerHouseID) {
				if (h->creditsStorage < h->credits) {
					h->credits = h->creditsStorage;
				}
			} else {
				uint16 maxCredits = max(h->creditsStorage, g_playerCreditsNoSilo);
				if (h->credits > maxCredits) {
					h->credits = maxCredits;

					GUI_DisplayText(String_Get_ByIndex(STR_INSUFFICIENT_SPICE_STORAGE_AVAILABLE_SPICE_IS_LOST), 1);
				}
			}

			if (h->index == g_playerHouseID) {
				if (h->creditsStorage > g_playerCreditsNoSilo) {
					g_playerCreditsNoSilo = 0;
				}

				if (g_playerCreditsNoSilo == 0 && g_campaignID > 1 && h->credits != 0) {
					if (h->creditsStorage != 0 && ((h->credits * 256 / h->creditsStorage) > 200)) {
						GUI_DisplayText(String_Get_ByIndex(STR_SPICE_STORAGE_CAPACITY_LOW_BUILD_SILOS), 0);
					}
				}

				if (h->credits < 100 && g_playerCreditsNoSilo != 0) {
					GUI_DisplayText(String_Get_ByIndex(STR_CREDITS_ARE_LOW_HARVEST_SPICE_FOR_MORE_CREDITS), 0);
				}
			}
		}

		if (tickHouse) House_EnsureHarvesterAvailable((uint8)h->index);

		if (tickStarport && h->starportLinkedID != UNIT_INDEX_INVALID) {
			Unit *u = NULL;

			h->starportTimeLeft--;
			if ((int16)h->starportTimeLeft < 0) h->starportTimeLeft = 0;

			if (h->starportTimeLeft == 0) {
				Structure *s;

				s = Structure_Get_ByIndex(g_structureIndex);
				if (s->o.type == STRUCTURE_STARPORT && s->o.houseID == h->index) {
					u = Unit_CreateWrapper((uint8)h->index, UNIT_FRIGATE, Tools_Index_Encode(s->o.index, IT_STRUCTURE));

					if (u != NULL) {
						u->o.linkedID = (uint8)h->starportLinkedID;
						h->starportLinkedID = UNIT_INDEX_INVALID;
						u->o.flags.s.inTransport = true;

						Audio_PlayVoice(VOICE_FRIGATE_HAS_ARRIVED);
					}
				} else {
					PoolFindStruct find2;

					find2.houseID = h->index;
					find2.index   = 0xFFFF;
					find2.type    = STRUCTURE_STARPORT;

					while (true) {
						s = Structure_Find(&find2);
						if (s == NULL) break;
						if (s->o.linkedID != 0xFF) continue;

						u = Unit_CreateWrapper((uint8)h->index, UNIT_FRIGATE, Tools_Index_Encode(s->o.index, IT_STRUCTURE));

						if (u != NULL) {
							u->o.linkedID = (uint8)h->starportLinkedID;
							h->starportLinkedID = 0xFFFF;
							u->o.flags.s.inTransport = true;

							Audio_PlayVoice(VOICE_FRIGATE_HAS_ARRIVED);
						}
					}
				}

				h->starportTimeLeft = (u != NULL) ? g_table_houseInfo[h->index].starportDeliveryTime : 1;
			}
		}

		if (tickHouse) {
			House_CalculatePowerAndCredit(h);
			Structure_CalculateHitpointsMax(h);

			if (h->timerUnitAttack != 0) h->timerUnitAttack--;
			if (h->timerSandwormAttack != 0) h->timerSandwormAttack--;
			if (h->timerStructureAttack != 0) h->timerStructureAttack--;
			if (h->harvestersIncoming > 0 && Unit_CreateWrapper((uint8)h->index, UNIT_HARVESTER, 0) != NULL) h->harvestersIncoming--;
		}

		if (tickPowerMaintenance) {
			uint16 powerMaintenanceCost = (h->powerUsage / 32) + 1;
			h->credits -= min(h->credits, powerMaintenanceCost);
		}
	}
}

/**
 * Convert the name of a house to the type value of that house, or
 *  HOUSE_INVALID if not found.
 */
uint8 House_StringToType(const char *name)
{
	uint8 index;
	if (name == NULL) return HOUSE_INVALID;

	for (index = 0; index < 6; index++) {
		if (strcasecmp(g_table_houseInfo[index].name, name) == 0) return index;
	}

	return HOUSE_INVALID;
}

/**
 * Gives a harvester to the given house if it has a refinery and no harvesters.
 *
 * @param houseID The index of the house to give a harvester to.
 */
void House_EnsureHarvesterAvailable(uint8 houseID)
{
	PoolFindStruct find;
	Structure *s;

	find.houseID = houseID;
	find.type    = 0xFFFF;
	find.index   = 0xFFFF;

	while (true) {
		s = Structure_Find(&find);
		if (s == NULL) break;
		/* ENHANCEMENT -- Dune2 checked the wrong type to skip. LinkedID is a structure for a Construction Yard */
		if (!g_dune2_enhanced && s->o.type == STRUCTURE_HEAVY_VEHICLE) continue;
		if (g_dune2_enhanced && s->o.type == STRUCTURE_CONSTRUCTION_YARD) continue;
		if (s->o.linkedID == UNIT_INVALID) continue;
		if (Unit_Get_ByIndex(s->o.linkedID)->o.type == UNIT_HARVESTER) return;
	}

	find.houseID = houseID;
	find.type    = UNIT_CARRYALL;
	find.index   = 0xFFFF;

	while (true) {
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;
		if (u->o.linkedID == UNIT_INVALID) continue;
		if (Unit_Get_ByIndex(u->o.linkedID)->o.type == UNIT_HARVESTER) return;
	}

	if (Unit_IsTypeOnMap(houseID, UNIT_HARVESTER)) return;

	find.houseID = houseID;
	find.type    = STRUCTURE_REFINERY;
	find.index   = 0xFFFF;

	s = Structure_Find(&find);
	if (s == NULL) return;

	if (Unit_CreateWrapper(houseID, UNIT_HARVESTER, Tools_Index_Encode(s->o.index, IT_STRUCTURE)) == NULL) return;

	if (houseID != g_playerHouseID) return;

	GUI_DisplayText(String_Get_ByIndex(STR_HARVESTER_IS_HEADING_TO_REFINERY), 0);
}

/**
 * Checks if two houses are allied.
 *
 * @param houseID1 The index of the first house.
 * @param houseID2 The index of the second house.
 * @return True if and only if the two houses are allies of eachother.
 */
bool House_AreAllied(uint8 houseID1, uint8 houseID2)
{
	if (houseID1 == HOUSE_INVALID || houseID2 == HOUSE_INVALID) return false;

	if (houseID1 == houseID2) return true;

	if (houseID1 == HOUSE_FREMEN || houseID2 == HOUSE_FREMEN) {
		return (houseID1 == HOUSE_ATREIDES || houseID2 == HOUSE_ATREIDES);
	}

	return (houseID1 != g_playerHouseID && houseID2 != g_playerHouseID);
}

/**
 * Updates the radar state for the given house.
 * @param h The house.
 * @return True if and only if the radar has been activated.
 */
bool House_UpdateRadarState(House *h)
{
	if (h == NULL || h->index != g_playerHouseID) return false;

#if 0
	void *wsa = NULL;
	bool activate = h->flags.radarActivated;
	uint16 frame;
	uint16 frameCount;

	if (h->flags.radarActivated) {
		/* Deactivate radar */
		if ((h->structuresBuilt & (1 << STRUCTURE_OUTPOST)) == 0 || h->powerProduction < h->powerUsage) activate = false;
	} else {
		/* Activate radar */
		if ((h->structuresBuilt & (1 << STRUCTURE_OUTPOST)) != 0 && h->powerProduction >= h->powerUsage) activate = true;
	}
#else
	const bool activate = ((h->structuresBuilt & FLAG_STRUCTURE_OUTPOST) && (h->powerProduction >= h->powerUsage));
#endif

	if (h->flags.radarActivated == activate) return false;

#if 0
	wsa = WSA_LoadFile("STATIC.WSA", GFX_Screen_Get_ByIndex(SCREEN_1), GFX_Screen_GetSize_ByIndex(SCREEN_1), true);
	frameCount = WSA_GetFrameCount(wsa);

	GUI_Mouse_Hide_Safe();

	while (Driver_Voice_IsPlaying()) sleepIdle();
#endif

	Audio_PlaySound(SOUND_RADAR_STATIC);
	Audio_PlayVoice(activate ? VOICE_RADAR_ACTIVATED : VOICE_RADAR_DEACTIVATED);

#if 0
	frameCount = WSA_GetFrameCount(wsa);

	for (frame = 0; frame < frameCount; frame++) {
		WSA_DisplayFrame(wsa, activate ? frameCount - frame : frame, 256, 136, SCREEN_0);
		GUI_PaletteAnimate();

		Timer_Sleep(3);
	}

	h->flags.radarActivated = activate;

	WSA_Unload(wsa);

	g_viewport_forceRedraw = true;

	GUI_Mouse_Show_Safe();

	GUI_Widget_Viewport_RedrawMap(0);
#else
	MenuBar_StartRadarAnimation(activate);
	h->flags.radarActivated = activate;
#endif

	return activate;
}

/**
 * Update the CreditsStorage by walking over all structures and checking what
 *  they can hold.
 * @param houseID The house to check the storage for.
 */
void House_UpdateCreditsStorage(uint8 houseID)
{
	PoolFindStruct find;
	uint32 creditsStorage;

	uint16 oldValidateStrictIfZero = g_validateStrictIfZero;
	g_validateStrictIfZero = 0;

	find.houseID = houseID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	creditsStorage = 0;
	while (true) {
		const StructureInfo *si;
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;

		si = &g_table_structureInfo[s->o.type];
		creditsStorage += si->creditsStorage;
	}

	if (creditsStorage > 32000) creditsStorage = 32000;

	House_Get_ByIndex(houseID)->creditsStorage = creditsStorage;

	g_validateStrictIfZero = oldValidateStrictIfZero;
}

/**
 * Calculate the power usage and production, and the credits storage.
 *
 * @param h The house to calculate the numbers for.
 */
void House_CalculatePowerAndCredit(House *h)
{
	PoolFindStruct find;

	if (h == NULL) return;

	h->powerUsage      = 0;
	h->powerProduction = 0;
	h->creditsStorage  = 0;

	find.houseID = h->index;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		const StructureInfo *si;
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;
		/* ENHANCEMENT -- Only count structures that are placed on the map, not ones we are building. */
		if (g_dune2_enhanced && s->o.flags.s.isNotOnMap) continue;

		si = &g_table_structureInfo[s->o.type];

		h->creditsStorage += si->creditsStorage;

		/* Positive values means usage */
		if (si->powerUsage >= 0) {
			h->powerUsage += si->powerUsage;
			continue;
		}

		/* Negative value and full health means everything goes to production */
		if (s->o.hitpoints >= si->o.hitpoints) {
			h->powerProduction += -si->powerUsage;
			continue;
		}

		/* Negative value and partial health, calculate how much should go to production (capped at 50%) */
		/* ENHANCEMENT -- The 50% cap of Dune2 is silly and disagress with the GUI. If your hp is 10%, so should the production. */
		if (!g_dune2_enhanced && s->o.hitpoints <= si->o.hitpoints / 2) {
			h->powerProduction += (-si->powerUsage) / 2;
			continue;
		}
		h->powerProduction += (-si->powerUsage) * s->o.hitpoints / si->o.hitpoints;
	}

	/* Check if we are low on power */
	if (h->index == g_playerHouseID && h->powerUsage > h->powerProduction) {
		GUI_DisplayText(String_Get_ByIndex(STR_INSUFFICIENT_POWER_WINDTRAP_IS_NEEDED), 1);
	}

	/* If there are no buildings left, you lose your right on 'credits without storage' */
	if (h->index == g_playerHouseID && h->structuresBuilt == 0 && g_validateStrictIfZero == 0) {
		g_playerCreditsNoSilo = 0;
	}
}

const char *House_GetWSAHouseFilename(uint8 houseID)
{
	static const char *houseWSAFileNames[HOUSE_MAX] = {
		"FHARK.WSA", "FARTR.WSA", "FORDOS.WSA",
		"FFREMN.WSA", "FSARD.WSA", "FMERC.WSA"
	};

	if (houseID >= HOUSE_MAX) return NULL;
	return houseWSAFileNames[houseID];
}
