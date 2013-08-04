/** @file src/saveload/house.c Load/save routines for House. */

#include <string.h>

#include "saveload.h"
#include "../house.h"
#include "../pool/house.h"
#include "../pool/pool.h"
#include "../pool/structure.h"
#include "../pool/unit.h"

static const SaveLoadDesc s_saveHouse[] = {
	SLD_ENTRY2(House, SLDT_UINT16, index,           SLDT_UINT8),
	SLD_ENTRY (House, SLDT_UINT16, harvestersIncoming),
	SLD_ENTRY2(House, SLDT_UINT16, flags,           SLDT_HOUSEFLAGS),
	SLD_ENTRY (House, SLDT_UINT16, unitCount),
	SLD_ENTRY (House, SLDT_UINT16, unitCountMax),
	SLD_ENTRY (House, SLDT_UINT16, unitCountEnemy),
	SLD_ENTRY (House, SLDT_UINT16, unitCountAllied),
	SLD_ENTRY (House, SLDT_UINT32, structuresBuilt),
	SLD_ENTRY (House, SLDT_UINT16, credits),
	SLD_ENTRY (House, SLDT_UINT16, creditsStorage),
	SLD_ENTRY (House, SLDT_UINT16, powerProduction),
	SLD_ENTRY (House, SLDT_UINT16, powerUsage),
	SLD_ENTRY (House, SLDT_UINT16, windtrapCount),
	SLD_ENTRY (House, SLDT_UINT16, creditsQuota),
	SLD_ENTRY (House, SLDT_UINT16, palacePosition.x),
	SLD_ENTRY (House, SLDT_UINT16, palacePosition.y),
	SLD_EMPTY (       SLDT_UINT16),
	SLD_ENTRY (House, SLDT_UINT16, timerUnitAttack),
	SLD_ENTRY (House, SLDT_UINT16, timerSandwormAttack),
	SLD_ENTRY (House, SLDT_UINT16, timerStructureAttack),
	SLD_ENTRY (House, SLDT_UINT16, starportTimeLeft),
	SLD_ENTRY (House, SLDT_UINT16, starportLinkedID),
	SLD_ARRAY (House, SLDT_UINT16, ai_structureRebuild, 10),
	SLD_END
};

static const SaveLoadDesc s_saveHouse2[] = {
	SLD_ENTRY2(House, SLDT_UINT16, index,           SLDT_UINT8),
	SLD_ENTRY (House, SLDT_UINT16, creditsStorageNoSilo),
	SLD_ENTRY (House, SLDT_UINT16, houseMissileCountdown),
	SLD_ENTRY (House, SLDT_UINT16, houseMissileID),
	SLD_ENTRY (House, SLDT_UINT16, starportID),
	SLD_END
};

/**
 * Load all Houses from a file.
 * @param fp The file to load from.
 * @param length The length of the data chunk.
 * @return True if and only if all bytes were read successful.
 */
bool House_Load(FILE *fp, uint32 length)
{
	while (length > 0) {
		House *h;
		House hl;

		memset(&hl, 0, sizeof(hl));

		/* Read the next House from disk */
		if (!SaveLoad_Load(s_saveHouse, fp, &hl)) return false;

		length -= SaveLoad_GetLength(s_saveHouse);

		/* Create the House in the pool */
		h = House_Allocate(hl.index);
		if (h == NULL) return false;

		/* Copy over the data */
		*h = hl;

		/* Extra data. */
		h->starportLinkedID = UNIT_INDEX_INVALID;
		h->houseMissileID   = UNIT_INDEX_INVALID;

		/* See if it is a human house */
		if (h->flags.human) {
			g_playerHouseID = h->index;
			g_playerHouse = h;

			if (h->starportLinkedID != 0xFFFF && h->starportTimeLeft == 0) h->starportTimeLeft = 1;

			Info_Load_PlayerHouseGlobals(h);
		}
	}

	Scenario_Load_OldStats();

	if (length != 0) return false;

	return true;
}

/**
 * Load all Houses from a file.
 * @param fp The file to load from.
 * @param length The length of the data chunk.
 * @return True if and only if all bytes were read successful.
 */
bool House_LoadOld(FILE *fp, uint32 length)
{
	while (length > 0) {
		House hl;

		/* Read the next House from disk */
		if (!SaveLoad_Load(s_saveHouse, fp, &hl)) return false;

		/* See if it is a human house */
		if (hl.flags.human) {
			g_playerHouseID = hl.index;
			break;
		}

		length -= SaveLoad_GetLength(s_saveHouse);
	}
	if (length == 0) return false;

	return true;
}

/**
 * Save all Houses to a file.
 * @param fp The file to save to.
 * @return True if and only if all bytes were written successful.
 */
bool House_Save(FILE *fp)
{
	PoolFindStruct find;

	find.houseID = HOUSE_INVALID;
	find.type    = 0xFFFF;
	find.index   = 0xFFFF;

	while (true) {
		House *h;

		h = House_Find(&find);
		if (h == NULL) break;

		if (!SaveLoad_Save(s_saveHouse, fp, h)) return false;
	}

	return true;
}

/*--------------------------------------------------------------*/

bool
House_Load2(FILE *fp, uint32 length)
{
	uint32 bytes_read = 0;

	while (bytes_read < length) {
		House hl;

		if (!SaveLoad_Load(s_saveHouse2, fp, &hl))
			return false;

		bytes_read += SaveLoad_GetLength(s_saveHouse2);

		House *h = House_Get_ByIndex(hl.index);
		if (h == NULL)
			return false;

		h->houseMissileID = hl.houseMissileID;
		h->houseMissileCountdown = hl.houseMissileCountdown;
	}

	return (bytes_read == length);
}

bool
House_Save2(FILE *fp)
{
	PoolFindStruct find;

	find.houseID = HOUSE_INVALID;
	find.type    = 0xFFFF;
	find.index   = 0xFFFF;

	while (true) {
		House *h = House_Find(&find);
		if (h == NULL)
			break;

		if (!SaveLoad_Save(s_saveHouse2, fp, h))
			return false;
	}

	return true;
}
