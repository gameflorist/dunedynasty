/** @file src/saveload/info.c Load/save routines for Info. */

#include "saveload.h"
#include "../file.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../map.h"
#include "../newui/strategicmap.h"
#include "../opendune.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../structure.h"
#include "../timer/timer.h"

/* These were originally global variables. */
static uint16 s_playerCreditsNoSilo;
static uint16 s_structureActiveID;
static uint16 s_houseMissileCountdown;
static uint16 s_houseMissileID;
static uint16 s_starportID;

static uint32 SaveLoad_SelectionType(void *object, uint32 value, bool loading)
{
	VARIABLE_NOT_USED(object);

	if (loading) {
		g_selectionTypeNew = (uint16)value;
		return 0;
	}

	return g_selectionType;
}

#if 0
static uint32 SaveLoad_UnitSelected(void *object, uint32 value, bool loading)
{
	VARIABLE_NOT_USED(object);

	Unit_UnselectAll();

	if (loading) {
		if ((uint16)value != 0xFFFF && value < UNIT_INDEX_MAX) {
			Unit_AddSelected(Unit_Get_ByIndex((uint16)value));
		}
		return 0;
	}

	if (Unit_AnySelected()) {
		const Unit *u = Unit_FirstSelected(NULL);
		return u->o.index;
	} else {
		return 0xFFFF;
	}
}
#endif

static uint32 SaveLoad_UnitActive(void *object, uint32 value, bool loading)
{
	VARIABLE_NOT_USED(object);

	if (loading) {
		if ((uint16)value != 0xFFFF && value < UNIT_INDEX_MAX) {
			g_unitActive = Unit_Get_ByIndex((uint16)value);
		} else {
			g_unitActive = NULL;
		}
		return 0;
	}

	if (g_unitActive != NULL) {
		return g_unitActive->o.index;
	} else {
		return 0xFFFF;
	}
}

static uint32 SaveLoad_TickScenarioStart(void *object, uint32 value, bool loading)
{
	VARIABLE_NOT_USED(object);

	if (loading) {
		g_tickScenarioStart = g_timerGame - value;
		return 0;
	}

	return g_timerGame - g_tickScenarioStart;
}

static const SaveLoadDesc s_saveInfo[] = {
	SLD_GSLD   (g_scenario,  g_saveScenario),
	SLD_GENTRY (SLDT_UINT16, s_playerCreditsNoSilo),
	SLD_GENTRY (SLDT_UINT16, g_viewportPosition),
	SLD_GENTRY (SLDT_UINT16, g_selectionRectanglePosition),
	SLD_GCALLB (SLDT_INT8,   g_selectionType, &SaveLoad_SelectionType),
	SLD_GENTRY2(SLDT_INT8,   g_structureActiveType, SLDT_UINT16),
	SLD_GENTRY (SLDT_UINT16, g_structureActivePosition),
	SLD_GENTRY (SLDT_UINT16, s_structureActiveID),
	SLD_EMPTY  (SLDT_UINT16), /* was SaveLoad_UnitSelected. */
	SLD_GCALLB (SLDT_UINT16, g_unitActive, &SaveLoad_UnitActive),
	SLD_GENTRY (SLDT_UINT16, g_activeAction),
	SLD_GENTRY (SLDT_UINT32, g_strategicRegionBits),
	SLD_GENTRY (SLDT_UINT16, g_scenarioID),
	SLD_GENTRY (SLDT_UINT16, g_campaignID),
	SLD_GENTRY (SLDT_UINT32, g_hintsShown1),
	SLD_GENTRY (SLDT_UINT32, g_hintsShown2),
	SLD_GCALLB (SLDT_UINT32, g_tickScenarioStart, &SaveLoad_TickScenarioStart),
	SLD_GENTRY (SLDT_UINT16, s_playerCreditsNoSilo),
	SLD_GARRAY (SLDT_INT16,  g_starportAvailable, UNIT_MAX),
	SLD_GENTRY (SLDT_UINT16, s_houseMissileCountdown),
	SLD_GENTRY (SLDT_UINT16, s_houseMissileID),
	SLD_GENTRY (SLDT_UINT16, s_starportID),
	SLD_END
};

static const SaveLoadDesc s_saveInfoOld[] = {
	SLD_EMPTY2(SLDT_UINT8,  250),
	SLD_GENTRY(SLDT_UINT16, g_scenarioID),
	SLD_GENTRY(SLDT_UINT16, g_campaignID),
	SLD_END
};

static const SaveLoadDesc s_saveInfo2[] = {
	SLD_ENTRY (Unit, SLDT_UINT16, o.index),
	SLD_END
};

/**
 * Load all kinds of important info from a file.
 * @param fp The file to load from.
 * @param length The length of the data chunk.
 * @return True if and only if all bytes were read successful.
 */
bool Info_Load(FILE *fp, uint32 length)
{
	if (SaveLoad_GetLength(s_saveInfo) != length) return false;
	if (!SaveLoad_Load(s_saveInfo, fp, NULL)) return false;

	g_selectionPosition = g_selectionRectanglePosition;
	Map_MoveDirection(0, 0);

	Sprites_LoadTiles();

	Map_CreateLandscape(g_scenario.mapSeed);

	return true;
}

/**
 * Load all kinds of important info from a file.
 * @param fp The file to load from.
 * @param length The length of the data chunk.
 * @return True if and only if all bytes were read successful.
 */
bool Info_LoadOld(FILE *fp, uint32 length)
{
	VARIABLE_NOT_USED(length);

	if (!SaveLoad_Load(s_saveInfoOld, fp, NULL)) return false;

	return true;
}

void
Info_Load_PlayerHouseGlobals(House *h)
{
	h->creditsStorageNoSilo  = s_playerCreditsNoSilo;
	h->structureActiveID     = s_structureActiveID;
	h->houseMissileCountdown = s_houseMissileCountdown;
	h->houseMissileID        = s_houseMissileID;
	h->starportID            = s_starportID;

	g_structureActive
		= (s_structureActiveID != STRUCTURE_INDEX_INVALID)
		? Structure_Get_ByIndex(s_structureActiveID) : NULL;
}

/**
 * Save all kinds of important info to the savegame.
 * @param fp The file to save to.
 * @return True if and only if all bytes were written successful.
 */
bool Info_Save(FILE *fp)
{
	const uint16 savegameVersion = 0x0290;

	if (g_playerHouse != NULL) {
		s_playerCreditsNoSilo   = g_playerHouse->creditsStorageNoSilo;
		s_structureActiveID     = g_playerHouse->structureActiveID;
		s_houseMissileCountdown = g_playerHouse->houseMissileCountdown;
		s_houseMissileID        = g_playerHouse->houseMissileID;
		s_starportID            = g_playerHouse->starportID;
	}
	else {
		s_playerCreditsNoSilo   = 0;
		s_structureActiveID     = STRUCTURE_INDEX_INVALID;
		s_houseMissileCountdown = 0;
		s_houseMissileID        = UNIT_INDEX_INVALID;
		s_starportID            = STRUCTURE_INDEX_INVALID;
	}

	if (!fwrite_le_uint16(savegameVersion, fp)) return false;

	Scenario_Save_OldStats();
	if (!SaveLoad_Save(s_saveInfo, fp, NULL)) return false;

	return true;
}

/*--------------------------------------------------------------*/

bool
Info_Load2(FILE *fp, uint32 length)
{
	Unit_UnselectAll();

	while (length > 0) {
		Unit ul;
		if (!SaveLoad_Load(s_saveInfo2, fp, &ul))
			return false;

		length -= SaveLoad_GetLength(s_saveInfo2);

		Unit *u = Unit_Get_ByIndex(ul.o.index);
		if (u == NULL)
			return false;

		Unit_AddSelected(u);
	}

	return true;
}

bool
Info_Save2(FILE *fp)
{
	int iter;

	Unit *u = Unit_FirstSelected(&iter);
	while (u != NULL) {
		if (!SaveLoad_Save(s_saveInfo2, fp, u))
			return false;

		u = Unit_NextSelected(&iter);
	}

	return true;
}
