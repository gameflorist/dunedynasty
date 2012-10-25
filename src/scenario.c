/* $Id$ */

/** @file src/scenario.c %Scenario handling routines. */

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "os/common.h"
#include "os/math.h"
#include "os/strings.h"

#include "scenario.h"

#include "enhancement.h"
#include "file.h"
#include "gfx.h"
#include "house.h"
#include "ini.h"
#include "map.h"
#include "opendune.h"
#include "pool/house.h"
#include "pool/pool.h"
#include "pool/structure.h"
#include "pool/unit.h"
#include "sprites.h"
#include "string.h"
#include "structure.h"
#include "team.h"
#include "tile.h"
#include "timer/timer.h"
#include "unit.h"
#include "gui/gui.h"

Campaign *g_campaign_list;
int g_campaign_total;
int g_campaign_selected;
Scenario g_scenario;

static void *s_scenarioBuffer = NULL;

/*--------------------------------------------------------------*/

Campaign *
Campaign_Alloc(const char *dir_name)
{
	Campaign *camp;

	g_campaign_list = realloc(g_campaign_list, (g_campaign_total + 1) * sizeof(g_campaign_list[0]));
	g_campaign_total++;

	camp = &g_campaign_list[g_campaign_total - 1];
	if (dir_name == NULL) { /* Dune II */
		camp->dir_name[0] = '\0';
	}
	else {
		snprintf(camp->dir_name, sizeof(camp->dir_name), "%s/", dir_name);
	}

	return camp;
}

static void
Campaign_AddFileInPAK(const char *filename, int parent)
{
	FileInfo *fi;

	if (FileHash_FindIndex(filename) != (unsigned int)-1)
		return;

	fi = FileHash_Store(filename);
	fi->filename = strdup(filename);
	fi->parentIndex = parent;
	fi->flags.inPAKFile = true;
}

static void
Campaign_ReadCPSTweaks(char *source, const char *key, char *value, size_t size,
		const unsigned int *def, unsigned int *dest)
{
	unsigned int tmp[HOUSE_MAX];

	Ini_GetString("CPS", key, NULL, value, size, source);

	if (sscanf(value, "%u,%u,%u,%u,%u,%u",
				&tmp[HOUSE_HARKONNEN], &tmp[HOUSE_ATREIDES], &tmp[HOUSE_ORDOS],
				&tmp[HOUSE_FREMEN], &tmp[HOUSE_SARDAUKAR], &tmp[HOUSE_MERCENARY]) < 6) {
		memcpy(dest, def, HOUSE_MAX * sizeof(dest[0]));
	}
	else {
		for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
			dest[houseID] = min(tmp[houseID], HOUSE_MAX);
		}
	}
}

static void
Campaign_ReadMetaData(Campaign *camp)
{
	if (camp->dir_name[0] == '\0') /* Dune II */
		return;

	if (!File_Exists_Ex(SEARCHDIR_CAMPAIGN_DIR, "META.INI"))
		return;

	char value[120];

	char *source = GFX_Screen_Get_ByIndex(3);
	memset(source, 0, 32000);
	File_ReadBlockFile_Ex(SEARCHDIR_CAMPAIGN_DIR, "META.INI", source, GFX_Screen_GetSize_ByIndex(3));

	camp->intermission = Ini_GetInteger("CAMPAIGN", "Intermission", 0, source);

	/* Read CPS tweaks. */
	Campaign_ReadCPSTweaks(source, "FAME.CPS",    value, sizeof(value), g_campaign_list[0].fame_cps, camp->fame_cps);
	Campaign_ReadCPSTweaks(source, "MAPMACH.CPS", value, sizeof(value), g_campaign_list[0].mapmach_cps, camp->mapmach_cps);
	Campaign_ReadCPSTweaks(source, "MISC.CPS",    value, sizeof(value), g_campaign_list[0].misc_cps, camp->misc_cps);

	/* Add PAK file entries. */
	int i = snprintf(value, sizeof(value), "%s", camp->dir_name);
	char *keys = source + strlen(source) + 5000;
	*keys = '\0';
	Ini_GetString("PAK", NULL, NULL, keys, 2000, source);

	FileInfo *fi;
	unsigned int parent;

	for (char *key = keys; *key != '\0'; key += strlen(key) + 1) {
		/* Shortcut for all regions and scenarios. */
		if (strcasecmp(key, "Scenarios") == 0) {
			Ini_GetString("PAK", "Scenarios", NULL, value + i, sizeof(value) - i, source);

			/* Handle white-space. */
			int j = i;
			while ((value[j] != '\0') && isspace(value[j])) j++;
			if (i != j) memmove(value + i, value + j, strlen(value + j) + 1);

			/* Already indexed this file. */
			parent = FileHash_FindIndex(value);
			if (parent != (unsigned int)-1)
				continue;

			fi = FileHash_Store(value);
			fi->filename = strdup(value);
			parent = FileHash_FindIndex(value);

			for (int h = 0; h < 3; h++) {
				if (camp->house[h] == HOUSE_INVALID)
					continue;

				snprintf(value + i, sizeof(value) - i, "REGION%c.INI", g_table_houseInfo[camp->house[h]].name[0]);
				Campaign_AddFileInPAK(value, parent);

				for (int scen = 0; scen <= 22; scen++) {
					snprintf(value + i, sizeof(value) - i, "SCEN%c%03d.INI", g_table_houseInfo[camp->house[h]].name[0], scen);
					Campaign_AddFileInPAK(value, parent);
				}
			}
		}

		/* PAK file content lists. */
		else {
			snprintf(value + i, sizeof(value) - i, "%s", key);

			/* Already indexed this file. */
			parent = FileHash_FindIndex(value);
			if (parent != (unsigned int)-1)
				continue;

			fi = FileHash_Store(value);
			fi->filename = strdup(value);
			parent = FileHash_FindIndex(value);

			/* Get content list: file1,file2,... */
			Ini_GetString("PAK", key, NULL, value + i, sizeof(value) - i, source);

			char *start = value + i;
			char *end = start + strlen(start);
			while (start < end) {
				while (isspace(*start)) start++;

				char *sep = strchr(start, ',');
				if (sep == NULL) {
					sep = end;
				}
				else {
					*sep = '\0';
				}

				memmove(value + i, start, sep - start + 1);
				String_Trim(value + i);

				if (strlen(value + i) > 0)
					Campaign_AddFileInPAK(value, parent);

				start = sep + 1;
			}
		}
	}
}

static uint16
ObjectInfo_FlagsToUint16(const ObjectInfo *oi)
{
	uint16 flags = 0;

	/* Note: Read/write flags as they appear in the original executable. */
	if (oi->flags.hasShadow)            flags |= 0x0001;
	if (oi->flags.factory)              flags |= 0x0002;
	/* 0x0004 unused. */
	if (oi->flags.notOnConcrete)        flags |= 0x0008;
	if (oi->flags.busyStateIsIncoming)  flags |= 0x0010;
	if (oi->flags.blurTile)             flags |= 0x0020;
	if (oi->flags.hasTurret)            flags |= 0x0040;
	if (oi->flags.conquerable)          flags |= 0x0080;
	if (oi->flags.canBePickedUp)        flags |= 0x0100;
	if (oi->flags.noMessageOnDeath)     flags |= 0x0200;
	if (oi->flags.tabSelectable)        flags |= 0x0400;
	if (oi->flags.scriptNoSlowdown)     flags |= 0x0800;
	if (oi->flags.targetAir)            flags |= 0x1000;
	if (oi->flags.priority)             flags |= 0x2000;

	return flags;
}

static uint16
UnitInfo_FlagsToUint16(const UnitInfo *ui)
{
	uint16 flags = 0;

	/* Note: Read/write flags as they appear in the original executable. */
	/* 0x0001 unused. */
	if (ui->flags.isBullet)         flags |= 0x0002;
	if (ui->flags.explodeOnDeath)   flags |= 0x0004;
	if (ui->flags.sonicProtection)  flags |= 0x0008;
	if (ui->flags.canWobble)        flags |= 0x0010;
	if (ui->flags.isTracked)        flags |= 0x0020;
	if (ui->flags.isGroundUnit)     flags |= 0x0040;
	if (ui->flags.mustStayInMap)    flags |= 0x0080;
	/* 0x0100 unused. */
	/* 0x0200 unused. */
	if (ui->flags.firesTwice)       flags |= 0x0400;
	if (ui->flags.impactOnSand)     flags |= 0x0800;
	if (ui->flags.isNotDeviatable)  flags |= 0x1000;
	if (ui->flags.hasAnimationSet)  flags |= 0x2000;
	if (ui->flags.notAccurate)      flags |= 0x4000;
	if (ui->flags.isNormalUnit)     flags |= 0x8000;

	return flags;
}

static void
Campaign_ReadHouseIni(void)
{
	char *source;
	char *key;
	char *keys;
	char buffer[120];

	memcpy(g_table_houseInfo, g_table_houseInfo_original, sizeof(g_table_houseInfo_original));

	if (!File_Exists_Ex(SEARCHDIR_CAMPAIGN_DIR, "HOUSE.INI"))
		return;

	source = GFX_Screen_Get_ByIndex(3);
	memset(source, 0, 32000);

	File_ReadBlockFile_Ex(SEARCHDIR_CAMPAIGN_DIR, "HOUSE.INI", source, GFX_Screen_GetSize_ByIndex(3));

	keys = source + strlen(source) + 5000;
	*keys = '\0';

	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		const char *category = g_table_houseInfo_original[houseID].name;
		const HouseInfo *original = &g_table_houseInfo_original[houseID];
		HouseInfo *hi = &g_table_houseInfo[houseID];

		Ini_GetString(category, NULL, NULL, keys, 2000, source);

		for (key = keys; *key != '\0'; key += strlen(key) + 1) {
			/* Weakness, LemonFactor, Decay, Recharge, Frigate, Special, Voice. */
			if (strcasecmp(key, "Weakness") == 0) {
				hi->toughness = Ini_GetInteger(category, key, original->toughness, source);
			}
			else if (strcasecmp(key, "LemonFactor") == 0) {
				hi->degradingChance = Ini_GetInteger(category, key, original->degradingChance, source);
			}
			else if (strcasecmp(key, "Decay") == 0) {
				hi->degradingAmount = Ini_GetInteger(category, key, original->degradingAmount, source);
			}
			else if (strcasecmp(key, "Recharge") == 0) {
				hi->specialCountDown = Ini_GetInteger(category, key, original->specialCountDown, source);
			}
			else if (strcasecmp(key, "Frigate") == 0) {
				hi->starportDeliveryTime = Ini_GetInteger(category, key, original->starportDeliveryTime, source);
			}
			else if (strcasecmp(key, "Special") == 0) {
				Ini_GetString(category, key, NULL, buffer, sizeof(buffer), source);

				char *buf = buffer;
				while (*buf == ' ' || *buf == '\t') buf++;

				     if (buf[0] == 'M') hi->specialWeapon = HOUSE_WEAPON_MISSILE;
				else if (buf[0] == 'F') hi->specialWeapon = HOUSE_WEAPON_FREMEN;
				else if (buf[0] == 'S') hi->specialWeapon = HOUSE_WEAPON_SABOTEUR;
			}
			else if (strcasecmp(key, "Voice") == 0) {
				Ini_GetString(category, key, NULL, buffer, sizeof(buffer), source);

				char *buf = buffer;
				while (*buf == ' ' || *buf == '\t') buf++;

				if ('a' <= buf[0] && buf[0] <= 'z')
					buf[0] += 'A' - 'a';

				     if (buf[0] == 'H') hi->sampleSet = SAMPLESET_HARKONNEN;
				else if (buf[0] == 'A') hi->sampleSet = SAMPLESET_ATREIDES;
				else if (buf[0] == 'O') hi->sampleSet = SAMPLESET_ORDOS;
			}
		}
	}
}

static void
Campaign_ReadProfileIni(void)
{
	struct {
		char type; /* unit, structure, or objects. */
		const char *category;
	} scandata[] = {
		/* Dune II. */
		{ 'O', "Construct" },
		{ 'U', "Combat" },

		/* Dune Dynasty extensions. */
		{ 'O', "Availability" },
		{ 'S', "Factory" },
		{ 'S', "StructureInfo" },
		{ 'U', "UnitObjectInfo" },
		{ 'U', "UnitInfo" }
	};

	memcpy(g_table_structureInfo, g_table_structureInfo_original, sizeof(g_table_structureInfo_original));
	memcpy(g_table_unitInfo, g_table_unitInfo_original, sizeof(g_table_unitInfo_original));

	if (!File_Exists_Ex(SEARCHDIR_CAMPAIGN_DIR, "PROFILE.INI"))
		return;

	char *source = GFX_Screen_Get_ByIndex(3);
	memset(source, 0, 32000);
	File_ReadBlockFile_Ex(SEARCHDIR_CAMPAIGN_DIR, "PROFILE.INI", source, GFX_Screen_GetSize_ByIndex(3));

	char *keys = source + strlen(source) + 5000;
	char buffer[120];

	for (unsigned int x = 0; x < lengthof(scandata); x++) {
		const char *category = scandata[x].category;

		*keys = '\0';
		Ini_GetString(category, NULL, NULL, keys, 2000, source);

		for (char *key = keys; *key != '\0'; key += strlen(key) + 1) {
			ObjectInfo *oi = NULL;
			StructureInfo *si = NULL;
			UnitInfo *ui = NULL;

			if (scandata[x].type == 'U' || scandata[x].type == 'O') {
				const uint8 type = Unit_StringToType(key);

				if (type != UNIT_INVALID) {
					ui = &g_table_unitInfo[type];
					oi = &g_table_unitInfo[type].o;
				}
			}

			if (scandata[x].type == 'S' || (scandata[x].type == 'O' && oi == NULL)) {
				const uint8 type = Structure_StringToType(key);

				if (type != STRUCTURE_INVALID) {
					si = &g_table_structureInfo[type];
					oi = &g_table_structureInfo[type].o;
				}
			}

			if (oi == NULL)
				continue;

			Ini_GetString(category, key, NULL, buffer, sizeof(buffer), source);

			switch (x) {
				case 0: /* Construct: buildCredits, buildTime, hitpoints, fogUncoverRadius, availableCampaign, priorityBuild, priorityTarget, sortPriority. */
					{
						ObjectInfo ot;
						uint16 sortPriority;    /* (uint8) concrete/concrete4 are 100/101 respectively. */

						const int count = sscanf(buffer, "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu",
								&ot.buildCredits, &ot.buildTime, &ot.hitpoints, &ot.fogUncoverRadius,
								&ot.availableCampaign, &ot.priorityBuild, &ot.priorityTarget, &sortPriority);
						if (count < 7) {
							fprintf(stderr, "[%s] %s=%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu\n", category, key,
									oi->buildCredits, oi->buildTime, oi->hitpoints, oi->fogUncoverRadius,
									oi->availableCampaign, oi->priorityBuild, oi->priorityTarget, oi->sortPriority);
							break;
						}

						oi->buildCredits      = ot.buildCredits;
						oi->buildTime         = ot.buildTime;
						oi->hitpoints         = ot.hitpoints;
						oi->fogUncoverRadius  = ot.fogUncoverRadius;
						oi->availableCampaign = ot.availableCampaign;
						oi->priorityBuild     = ot.priorityBuild;
						oi->priorityTarget    = ot.priorityTarget;

						if (count >= 8) {
							if ((si != &g_table_structureInfo[STRUCTURE_SLAB_1x1]) &&
							    (si != &g_table_structureInfo[STRUCTURE_SLAB_2x2])) {
								oi->sortPriority = sortPriority;
							}
						}
					}
					break;

				case 1: /* Combat: fireDistance, damage, fireDelay, movingSpeed. */
					{
						UnitInfo ut;

						const int count = sscanf(buffer, "%hu,%hu,%hu,%hu",
								&ut.fireDistance, &ut.damage, &ut.fireDelay, &ut.movingSpeed);
						if (count < 4) {
							fprintf(stderr, "[%s] %s=%hu,%hu,%hu,%hu\n", category, key,
									ui->damage, ui->movingSpeed, ui->fireDelay, ui->fireDistance);
							break;
						}

						ui->damage       = ut.damage;
						ui->movingSpeed  = ut.movingSpeed;
						ui->fireDelay    = ut.fireDelay;
						ui->fireDistance = ut.fireDistance;
					}
					break;

				case 2: /* Availability: availableHouse, structuresRequired, upgradeLevelRequired. */
					{
						uint16 availableHouse;          /* (uint8) enum HouseFlag. */
						uint32 structuresRequired;      /* 0xFFFFFFFF or StructureFlag. */
						uint16 upgradeLevelRequired;    /* (uint8) 0 .. 3. */

						const int count = sscanf(buffer, "%hX,%X,%hu",
								&availableHouse, &structuresRequired, &upgradeLevelRequired);
						if (count < 3) {
							fprintf(stderr, "[%s] %s=0x%02hX,0x%06X,%hu\n", category, key,
									oi->availableHouse, oi->structuresRequired, oi->upgradeLevelRequired);
							break;
						}

						oi->availableHouse = availableHouse;
						oi->structuresRequired = structuresRequired;
						oi->upgradeLevelRequired = upgradeLevelRequired;
					}
					break;

				case 3: /* Factory: buildableUnits[1..8], upgradeCampaign[1..3]. */
					{
						int16 buildableUnits[8];    /* -1 or enum UnitType. */
						uint16 upgradeCampaign[3];  /* 0 .. 9. */

						const int count = sscanf(buffer, "%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hu,%hu,%hu",
								&buildableUnits[0], &buildableUnits[1], &buildableUnits[2], &buildableUnits[3],
								&buildableUnits[4], &buildableUnits[5], &buildableUnits[6], &buildableUnits[7],
								&upgradeCampaign[0], &upgradeCampaign[1], &upgradeCampaign[2]);
						if (count < 11) {
							fprintf(stderr, "[%s] %s=%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hu,%hu,%hu\n", category, key,
									si->buildableUnits[0], si->buildableUnits[1], si->buildableUnits[2], si->buildableUnits[3],
									si->buildableUnits[4], si->buildableUnits[5], si->buildableUnits[6], si->buildableUnits[7],
									si->upgradeCampaign[0], si->upgradeCampaign[1], si->upgradeCampaign[2]);
							break;
						}

						for (int i = 0; i < 8; i++) {
							si->buildableUnits[i] = (0 <= buildableUnits[i] && buildableUnits[i] < UNIT_MAX) ? buildableUnits[i] : -1;
						}

						for (int i = 0; i < 3; i++) {
							si->upgradeCampaign[i] = upgradeCampaign[i];
						}
					}
					break;

				case 4: /* StructureInfo: objectFlags, spawnChance, enterFilter, creditsStorage, powerUsage. */
					{
						StructureInfo st;
						uint16 flags;

						const int count = sscanf(buffer, "%hX,%hu,%X,%hu,%hd",
								&flags, &st.o.spawnChance, &st.enterFilter, &st.creditsStorage, &st.powerUsage);
						if (count < 5) {
							fprintf(stderr, "[%s] %s=0x%04hX,%hu,0x%06X,%hu,%hd\n", category, key,
									ObjectInfo_FlagsToUint16(oi), si->o.spawnChance,
									si->enterFilter, si->creditsStorage, si->powerUsage);
							break;
						}

						oi->flags.hasShadow             = (flags & 0x0001) ? 1 : 0;
						oi->flags.factory               = (flags & 0x0002) ? 1 : 0;
						oi->flags.notOnConcrete         = (flags & 0x0008) ? 1 : 0;
						oi->flags.busyStateIsIncoming   = (flags & 0x0010) ? 1 : 0;
						/* oi->flags.blurTile           = (flags & 0x0020) ? 1 : 0; */
						/* oi->flags.hasTurret          = (flags & 0x0040) ? 1 : 0; */
						oi->flags.conquerable           = (flags & 0x0080) ? 1 : 0;
						/* oi->flags.canBePickedUp      = (flags & 0x0100) ? 1 : 0; */
						/* oi->flags.noMessageOnDeath   = (flags & 0x0200) ? 1 : 0; */
						/* oi->flags.tabSelectable      = (flags & 0x0400) ? 1 : 0; */
						/* oi->flags.scriptNoSlowdown   = (flags & 0x0800) ? 1 : 0; */
						/* oi->flags.targetAir          = (flags & 0x1000) ? 1 : 0; */
						/* oi->flags.priority           = (flags & 0x2000) ? 1 : 0; */

						si->enterFilter = st.enterFilter;
						si->creditsStorage = st.creditsStorage;
						si->powerUsage = st.powerUsage;
					}
					break;

				case 5: /* UnitObjectInfo: objectFlags, spawnChance, actionsPlayer[1..4]. */
					{
						ObjectInfo ot;
						uint16 flags;

						const int count = sscanf(buffer, "%hX,%hu,%hu,%hu,%hu,%hu",
								&flags, &ot.spawnChance,
								&ot.actionsPlayer[0], &ot.actionsPlayer[1], &ot.actionsPlayer[2], &ot.actionsPlayer[3]);
						if (count < 6) {
							fprintf(stderr, "[%s] %s=0x%04hX,%hu,%hu,%hu,%hu,%hu\n", category, key,
									ObjectInfo_FlagsToUint16(oi), oi->spawnChance,
									oi->actionsPlayer[0], oi->actionsPlayer[1], oi->actionsPlayer[2], oi->actionsPlayer[3]);
							break;
						}

						oi->flags.hasShadow             = (flags & 0x0001) ? 1 : 0;
						/* oi->flags.factory            = (flags & 0x0002) ? 1 : 0; */
						/* oi->flags.notOnConcrete      = (flags & 0x0008) ? 1 : 0; */
						/* oi->flags.busyStateIsIncoming= (flags & 0x0010) ? 1 : 0; */
						oi->flags.blurTile              = (flags & 0x0020) ? 1 : 0;
						oi->flags.hasTurret             = (flags & 0x0040) ? 1 : 0;
						/* oi->flags.conquerable        = (flags & 0x0080) ? 1 : 0; */
						oi->flags.canBePickedUp         = (flags & 0x0100) ? 1 : 0;
						oi->flags.noMessageOnDeath      = (flags & 0x0200) ? 1 : 0;
						/* oi->flags.tabSelectable      = (flags & 0x0400) ? 1 : 0; */
						/* oi->flags.scriptNoSlowdown   = (flags & 0x0800) ? 1 : 0; */
						oi->flags.targetAir             = (flags & 0x1000) ? 1 : 0;
						oi->flags.priority              = (flags & 0x2000) ? 1 : 0;
						oi->spawnChance = ot.spawnChance;

						for (int i = 0; i < 4; i++) {
							oi->actionsPlayer[i] = (ot.actionsPlayer[i] < ACTION_MAX) ? ot.actionsPlayer[i] : ACTION_ATTACK;
						}
					}
					break;

				case 6: /* UnitInfo: unitFlags, movementType, turningSpeed, explosionType, bulletType, bulletSound. */
					{
						uint16 flags;
						uint16 movementType;    /* enum UnitMovementType. */
						uint16 turningSpeed;    /* (uint8). */
						int16 explosionType;    /* -1 or 0 .. 19. */
						int16 bulletType;       /* UNIT_MISSILE_HOUSE .. UNIT_SANDWORM. */
						int16 bulletSound;      /* -1 or enum SoundID. */

						const int count = sscanf(buffer, "%hX,%hu,%hu,%hd,%hd,%hd",
								&flags, &movementType, &turningSpeed, &explosionType, &bulletType, &bulletSound);
						if (count < 6) {
							fprintf(stderr, "[%s] %s=0x%04hX,%hu,%hu,%hd,%hd,%hd\n", category, key,
									UnitInfo_FlagsToUint16(ui), ui->movementType, ui->turningSpeed,
									ui->explosionType, ui->bulletType, ui->bulletSound);
							break;
						}

						ui->flags.isBullet          = (flags & 0x0002) ? 1 : 0;
						ui->flags.explodeOnDeath    = (flags & 0x0004) ? 1 : 0;
						ui->flags.sonicProtection   = (flags & 0x0008) ? 1 : 0;
						ui->flags.canWobble         = (flags & 0x0010) ? 1 : 0;
						ui->flags.isTracked         = (flags & 0x0020) ? 1 : 0;
						ui->flags.isGroundUnit      = (flags & 0x0040) ? 1 : 0;
						ui->flags.mustStayInMap     = (flags & 0x0080) ? 1 : 0;
						ui->flags.firesTwice        = (flags & 0x0400) ? 1 : 0;
						ui->flags.impactOnSand      = (flags & 0x0800) ? 1 : 0;
						ui->flags.isNotDeviatable   = (flags & 0x1000) ? 1 : 0;
						ui->flags.hasAnimationSet   = (flags & 0x2000) ? 1 : 0;
						ui->flags.notAccurate       = (flags & 0x4000) ? 1 : 0;
						ui->flags.isNormalUnit      = (flags & 0x8000) ? 1 : 0;

						ui->movementType = (movementType < MOVEMENT_MAX) ? movementType : MOVEMENT_FOOT;
						ui->turningSpeed = turningSpeed;
						ui->explosionType = (0 <= explosionType && explosionType < 20) ? explosionType : -1;
						ui->bulletType = (UNIT_MISSILE_HOUSE <= bulletType && bulletType <= UNIT_SANDWORM) ? bulletType : -1;
						ui->bulletSound = (0 <= bulletSound && bulletSound < SOUNDID_MAX) ? bulletSound : -1;
					}
					break;

				default:
					assert(false);
			}
		}
	}
}

void
Campaign_Load(void)
{
	Campaign *camp = &g_campaign_list[g_campaign_selected];

	Campaign_ReadMetaData(camp);
	Campaign_ReadHouseIni();
	Campaign_ReadProfileIni();
	String_ReloadMentatText();
}

/*--------------------------------------------------------------*/

static void Scenario_Load_General(void)
{
	g_scenario.winFlags          = Ini_GetInteger("BASIC", "WinFlags",    0,                            s_scenarioBuffer);
	g_scenario.loseFlags         = Ini_GetInteger("BASIC", "LoseFlags",   0,                            s_scenarioBuffer);
	g_scenario.mapSeed           = Ini_GetInteger("MAP",   "Seed",        0,                            s_scenarioBuffer);
	g_scenario.timeOut           = Ini_GetInteger("BASIC", "TimeOut",     0,                            s_scenarioBuffer);
	g_viewportPosition           = Ini_GetInteger("BASIC", "TacticalPos", g_viewportPosition,           s_scenarioBuffer);
	g_selectionRectanglePosition = Ini_GetInteger("BASIC", "CursorPos",   g_selectionRectanglePosition, s_scenarioBuffer);
	g_scenario.mapScale          = Ini_GetInteger("BASIC", "MapScale",    0,                            s_scenarioBuffer);

	Ini_GetString("BASIC", "BriefPicture", "HARVEST.WSA",  g_scenario.pictureBriefing, 14, s_scenarioBuffer);
	Ini_GetString("BASIC", "WinPicture",   "WIN1.WSA",     g_scenario.pictureWin,      14, s_scenarioBuffer);
	Ini_GetString("BASIC", "LosePicture",  "LOSTBILD.WSA", g_scenario.pictureLose,     14, s_scenarioBuffer);

	g_selectionPosition = g_selectionRectanglePosition;
	Map_MoveDirection(0, 0);
}

static void Scenario_Load_House(uint8 houseID)
{
	const char *houseName = g_table_houseInfo[houseID].name;
	char *houseType;
	char buf[128];
	char *b;
	House *h;

	/* Get the type of the House (CPU / Human) */
	Ini_GetString(houseName, "Brain", "NONE", buf, 127, s_scenarioBuffer);
	for (b = buf; *b != '\0'; b++) if (*b >= 'a' && *b <= 'z') *b += 'A' - 'a';
	houseType = strstr("HUMAN$CPU", buf);
	if (houseType == NULL) return;

	/* Create the house */
	h = House_Allocate(houseID);

	h->credits      = Ini_GetInteger(houseName, "Credits",  0, s_scenarioBuffer);
	h->creditsQuota = Ini_GetInteger(houseName, "Quota",    0, s_scenarioBuffer);
	h->unitCountMax = Ini_GetInteger(houseName, "MaxUnit", 39, s_scenarioBuffer);

	/* ENHANCEMENT -- "MaxUnits" instead of MaxUnit. */
	if (enhancement_fix_scenario_typos || enhancement_raise_scenario_unit_cap) {
		if (h->unitCountMax == 0)
			h->unitCountMax = Ini_GetInteger(houseName, "MaxUnits", 39, s_scenarioBuffer);
	}

	/* For 'Brain = Human' we have to set a few additional things */
	if (*houseType != 'H') return;

	h->flags.human = true;

	g_playerHouseID       = houseID;
	g_playerHouse         = h;
	g_playerCreditsNoSilo = h->credits;
}

static void Scenario_Load_Houses(void)
{
	House *h;
	uint8 houseID;

	for (houseID = 0; houseID < HOUSE_MAX; houseID++) {
		Scenario_Load_House(houseID);
	}

	h = g_playerHouse;
	/* In case there was no unitCountMax in the scenario, calculate
	 *  it based on values used for the AI controlled houses. */
	if (h->unitCountMax == 0) {
		PoolFindStruct find;
		uint8 max;
		House *h;

		find.houseID = HOUSE_INVALID;
		find.index   = 0xFFFF;
		find.type    = 0xFFFF;

		max = 80;
		while ((h = House_Find(&find)) != NULL) {
			/* Skip the human controlled house */
			if (h->flags.human) continue;
			max -= h->unitCountMax;
		}

		h->unitCountMax = max;
	}

	if (enhancement_raise_scenario_unit_cap) {
		/* Leave some room for reinforcements. */
		int free_units = 70;
		int num_houses = 0;

		for (houseID = 0; houseID < HOUSE_MAX; houseID++) {
			House *h = House_Get_ByIndex(houseID);

			if (h->unitCountMax > 0) {
				free_units -= h->unitCountMax;
				num_houses++;
			}
		}

		const int inc = free_units / num_houses;
		if (inc <= 0)
			return;

		for (houseID = 0; houseID < HOUSE_MAX; houseID++) {
			House *h = House_Get_ByIndex(houseID);

			if (h->unitCountMax > 0)
				h->unitCountMax += inc;
		}
	}
}

static void Scenario_Load_Unit(const char *key, char *settings)
{
	uint8 houseType, unitType, actionType;
	int8 orientation;
	uint16 hitpoints;
	tile32 position;
	Unit *u;
	char *split;

	VARIABLE_NOT_USED(key);

	/* The value should have 6 values separated by a ',' */
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* First value is the House type */
	houseType = House_StringToType(settings);
	if (houseType == HOUSE_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Second value is the Unit type */
	unitType = Unit_StringToType(settings);
	if (unitType == UNIT_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Third value is the Hitpoints in percent (in base 256) */
	hitpoints = atoi(settings);

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Fourth value is the position on the map */
	position = Tile_UnpackTile(atoi(settings));

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Fifth value is orientation */
	orientation = (int8)((uint8)atoi(settings));

	/* Sixth value is the current state of the unit */
	settings = split + 1;
	actionType = Unit_ActionStringToType(settings);
	if (actionType == ACTION_INVALID) return;


	u = Unit_Allocate(UNIT_INDEX_INVALID, unitType, houseType);
	if (u == NULL) return;
	u->o.flags.s.byScenario = true;

	u->o.hitpoints   = hitpoints * g_table_unitInfo[unitType].o.hitpoints / 256;
	u->o.position    = position;
	u->orientation[0].current = orientation;
	u->actionID     = actionType;
	u->nextActionID = ACTION_INVALID;

	/* In case the above function failed and we are passed campaign 2, don't add the unit */
	if (!Map_IsValidPosition(Tile_PackTile(u->o.position)) && g_campaignID > 2) {
		Unit_Free(u);
		return;
	}

	/* XXX -- There is no way this is ever possible, as the beingBuilt flag is unset by Unit_Allocate() */
	if (!u->o.flags.s.isNotOnMap) Unit_SetAction(u, u->actionID);

	u->o.seenByHouses = 0x00;

	Unit_HouseUnitCount_Add(u, u->o.houseID);

	Unit_SetOrientation(u, u->orientation[0].current, true, 0);
	Unit_SetOrientation(u, u->orientation[0].current, true, 1);
	Unit_SetSpeed(u, 0);
}

static void Scenario_Load_Structure(const char *key, char *settings)
{
	uint8 index, houseType, structureType;
	uint16 hitpoints, position;
	char *split;

	/* 'GEN' marked keys are Slabs and Walls, where the number following indicates the position on the map */
	if (strncasecmp(key, "GEN", 3) == 0) {
		/* Position on the map is in the key */
		position = atoi(key + 3);

		/* The value should have two values separated by a ',' */
		split = strchr(settings, ',');
		if (split == NULL) return;
		*split = '\0';
		/* First value is the House type */
		houseType = House_StringToType(settings);
		if (houseType == HOUSE_INVALID) return;

		/* Second value is the Structure type */
		settings = split + 1;
		structureType = Structure_StringToType(settings);
		if (structureType == STRUCTURE_INVALID) return;

		Structure_Create(STRUCTURE_INDEX_INVALID, structureType, houseType, position);
		return;
	}

	/* The key should start with 'ID', followed by the index */
	index = atoi(key + 2);

	/* The value should have four values separated by a ',' */
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* First value is the House type */
	houseType = House_StringToType(settings);
	if (houseType == HOUSE_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Second value is the Structure type */
	structureType = Structure_StringToType(settings);
	if (structureType == STRUCTURE_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Third value is the Hitpoints in percent (in base 256) */
	/* ENHANCEMENT -- Dune2 ignores the % hitpoints read from the scenario */
	if (enhancement_read_scenario_structure_health) {
		if (enhancement_fix_scenario_typos) {
			hitpoints = clamp(0, atoi(settings), 256);
		}
		else {
			hitpoints = atoi(settings);
		}
	}
	else {
		hitpoints = 256;
	}

	/* Fourth value is the position of the structure */
	settings = split + 1;
	position = atoi(settings);

	/* Ensure nothing is already on the tile */
	/* XXX -- DUNE2 BUG? -- This only checks the top-left corner? Not really a safety, is it? */
	if (Structure_Get_ByPackedTile(position) != NULL) return;

	{
		Structure *s;

		/* ENHANCEMENT -- Atreides shouldn't get WOR since they can't do anything with it. */
		if (enhancement_fix_scenario_typos) {
			if (houseType == HOUSE_ATREIDES && structureType == STRUCTURE_WOR_TROOPER)
				structureType = STRUCTURE_BARRACKS;
		}

		s = Structure_Create(index, structureType, houseType, position);
		if (s == NULL) return;

		s->o.hitpoints = hitpoints * g_table_structureInfo[s->o.type].o.hitpoints / 256;
		s->o.flags.s.degrades = false;
		s->state = STRUCTURE_STATE_IDLE;
	}
}

static void Scenario_Load_Map(const char *key, char *settings)
{
	Tile *t;
	uint16 packed;
	uint16 value;
	char *s;
	char posY[3];

	if (*key != 'C') return;

	memcpy(posY, key + 4, 2);
	posY[2] = '\0';

	packed = Tile_PackXY(atoi(posY), atoi(key + 6)) & 0xFFF;
	t = &g_map[packed];

	s = strtok(settings, ",\r\n");
	value = atoi(s);
	t->houseID        = value & 0x07;
	t->isUnveiled     = (value & 0x08) != 0 ? true : false;
	t->hasUnit        = (value & 0x10) != 0 ? true : false;
	t->hasStructure   = (value & 0x20) != 0 ? true : false;
	t->hasAnimation   = (value & 0x40) != 0 ? true : false;
	t->hasExplosion = (value & 0x80) != 0 ? true : false;

	s = strtok(NULL, ",\r\n");
	t->groundSpriteID = atoi(s) & 0x01FF;
	if (g_mapSpriteID[packed] != t->groundSpriteID) g_mapSpriteID[packed] |= 0x8000;

	if (!t->isUnveiled) t->overlaySpriteID = g_veiledSpriteID;
}

static void Scenario_Load_Map_Bloom(uint16 packed, Tile *t)
{
	if (enhancement_fix_scenario_typos) {
		/* SCENA005: spice bloom is found in rock. */
		if (packed == 1364 && Map_GetLandscapeType(packed) == 4) {
			packed = 1360;
			t = &g_map[packed];
		}
	}

	t->groundSpriteID = g_bloomSpriteID;
	g_mapSpriteID[packed] |= 0x8000;
}

static void Scenario_Load_Map_Field(uint16 packed, Tile *t)
{
	Map_Bloom_ExplodeSpice(packed, HOUSE_INVALID);

	/* Show where a field started in the preview mode by making it an odd looking sprite */
	if (g_debugScenario) {
		t->groundSpriteID = 0x01FF;
	}
}

static void Scenario_Load_Map_Special(uint16 packed, Tile *t)
{
	t->groundSpriteID = g_bloomSpriteID + 1;
	g_mapSpriteID[packed] |= 0x8000;
}

static void Scenario_Load_Reinforcement(const char *key, char *settings)
{
	uint8 index, houseType, unitType, locationID;
	uint16 timeBetween;
	tile32 position;
	bool repeat;
	Unit *u;
	char *split;

	index = atoi(key);

	/* The value should have 4 values separated by a ',' */
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* First value is the House type */
	houseType = House_StringToType(settings);
	if (houseType == HOUSE_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Second value is the Unit type */
	unitType = Unit_StringToType(settings);
	if (unitType == UNIT_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Third value is the location of the reinforcement */
	     if (strcasecmp(settings, "NORTH")     == 0) locationID = 0;
	else if (strcasecmp(settings, "EAST")      == 0) locationID = 1;
	else if (strcasecmp(settings, "SOUTH")     == 0) locationID = 2;
	else if (strcasecmp(settings, "WEST")      == 0) locationID = 3;
	else if (strcasecmp(settings, "AIR")       == 0) locationID = 4;
	else if (strcasecmp(settings, "VISIBLE")   == 0) locationID = 5;
	else if (strcasecmp(settings, "ENEMYBASE") == 0) locationID = 6;
	else if (strcasecmp(settings, "HOMEBASE")  == 0) locationID = 7;
	else return;

	/* Fourth value is the time between reinforcement */
	settings = split + 1;
	timeBetween = atoi(settings) * 6 + 1;
	repeat = (settings[strlen(settings) - 1] == '+') ? true : false;
	/* ENHANCEMENT -- Dune2 makes a mistake in reading the '+', causing repeat to be always false */
	if (!enhancement_repeat_reinforcements) repeat = false;

	position.s.x = 0xFFFF;
	position.s.y = 0xFFFF;
	u = Unit_Create(UNIT_INDEX_INVALID, unitType, houseType, position, 0);
	if (u == NULL) return;

	g_scenario.reinforcement[index].unitID      = u->o.index;
	g_scenario.reinforcement[index].locationID  = locationID;
	g_scenario.reinforcement[index].timeLeft    = timeBetween;
	g_scenario.reinforcement[index].timeBetween = timeBetween;
	g_scenario.reinforcement[index].repeat      = repeat ? 1 : 0;
}

static void Scenario_Load_Team(const char *key, char *settings)
{
	uint8 houseType, teamActionType, movementType;
	uint16 minMembers, maxMembers;
	char *split;

	VARIABLE_NOT_USED(key);

	/* The value should have 5 values separated by a ',' */
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* First value is the House type */
	houseType = House_StringToType(settings);
	if (houseType == HOUSE_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Second value is the teamAction type */
	teamActionType = Team_ActionStringToType(settings);
	if (teamActionType == TEAM_ACTION_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Third value is the movement type */
	movementType = Unit_MovementStringToType(settings);
	if (movementType == MOVEMENT_INVALID) return;

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Fourth value is minimum amount of members in team */
	minMembers = atoi(settings);

	/* Find the next value in the ',' separated list */
	settings = split + 1;
	split = strchr(settings, ',');
	if (split == NULL) return;
	*split = '\0';

	/* Fifth value is maximum amount of members in team */
	maxMembers = atoi(settings);

	Team_Create(houseType, teamActionType, movementType, minMembers, maxMembers);
}

/**
 * Initialize a unit count of the starport.
 * @param key Unit type to set.
 * @param settings Count to set.
 */
static void Scenario_Load_Choam(const char *key, char *settings)
{
	uint8 unitType;

	unitType = Unit_StringToType(key);
	if (unitType == UNIT_INVALID) return;

	g_starportAvailable[unitType] = atoi(settings);
}

static void Scenario_Load_MapParts(const char *key, void (*ptr)(uint16 packed, Tile *t))
{
	char *s;
	char buf[128];

	Ini_GetString("MAP", key, '\0', buf, 127, s_scenarioBuffer);

	s = strtok(buf, ",\r\n");
	while (s != NULL) {
		uint16 packed;
		Tile *t;

		packed = atoi(s);
		t = &g_map[packed];

		(*ptr)(packed, t);

		s = strtok(NULL, ",\r\n");
	}
}

static void Scenario_Load_Chunk(const char *category, void (*ptr)(const char *key, char *settings))
{
	char *buffer = g_readBuffer;

	Ini_GetString(category, NULL, NULL, g_readBuffer, g_readBufferSize, s_scenarioBuffer);
	while (true) {
		char buf[127];

		if (*buffer == '\0') break;

		Ini_GetString(category, buffer, NULL, buf, 127, s_scenarioBuffer);

		(*ptr)(buffer, buf);
		buffer += strlen(buffer) + 1;
	}
}

static void Scenario_CentreViewport(uint8 houseID)
{
	PoolFindStruct find;

	find.houseID = houseID;
	find.type = STRUCTURE_CONSTRUCTION_YARD;
	find.index = 0xFFFF;

	Structure *s = Structure_Find(&find);
	if (s != NULL) {
		Map_CentreViewport((s->o.position.s.x >> 4) + TILE_SIZE, (s->o.position.s.y >> 4) + TILE_SIZE);
	}
}

bool Scenario_Load(uint16 scenarioID, uint8 houseID)
{
	char filename[14];
	int i;

	if (houseID >= HOUSE_MAX) return false;

	g_scenarioID = scenarioID;

	/* Load scenario file */
	snprintf(filename, sizeof(filename), "SCEN%c%03d.INI", g_table_houseInfo[houseID].name[0], scenarioID);
	if (!File_Exists_Ex(SEARCHDIR_CAMPAIGN_DIR, filename))
		return false;

	s_scenarioBuffer = File_ReadWholeFile_Ex(SEARCHDIR_CAMPAIGN_DIR, filename);

	memset(&g_scenario, 0, sizeof(Scenario));

	Scenario_Load_General();
	Sprites_LoadTiles();
	Map_CreateLandscape(g_scenario.mapSeed);

	for (i = 0; i < 16; i++) {
		g_scenario.reinforcement[i].unitID = UNIT_INDEX_INVALID;
	}

	Scenario_Load_Houses();

	Scenario_Load_Chunk("UNITS", &Scenario_Load_Unit);
	Scenario_Load_Chunk("STRUCTURES", &Scenario_Load_Structure);
	Scenario_Load_Chunk("MAP", &Scenario_Load_Map);
	Scenario_Load_Chunk("REINFORCEMENTS", &Scenario_Load_Reinforcement);
	Scenario_Load_Chunk("TEAMS", &Scenario_Load_Team);
	Scenario_Load_Chunk("CHOAM", &Scenario_Load_Choam);

	Scenario_Load_MapParts("Bloom", Scenario_Load_Map_Bloom);
	Scenario_Load_MapParts("Field", Scenario_Load_Map_Field);
	Scenario_Load_MapParts("Special", Scenario_Load_Map_Special);

	Scenario_CentreViewport(houseID);
	g_tickScenarioStart = g_timerGame;

	free(s_scenarioBuffer); s_scenarioBuffer = NULL;
	return true;
}
