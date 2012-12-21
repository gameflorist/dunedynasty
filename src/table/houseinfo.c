/** @file src/table/houseinfo.c HouseInfo file table. */

#include "sound.h"
#include "../house.h"

const HouseInfo g_table_houseInfo_original[HOUSE_MAX] = {
	{ /* 0 */
		/* name                 */ "Harkonnen",
		/* toughness            */ 200,
		/* degradingChance      */ 85,
		/* degradingAmount      */ 3,
		/* minimapColor         */ 144,
		/* specialCountDown     */ 600,
		/* starportDeliveryTime */ 10,
		/* prefixChar           */ 'H',
		/* specialWeapon        */ HOUSE_WEAPON_MISSILE,
		/* musicWin             */ MUSIC_WIN_HARKONNEN,
		/* musicLose            */ MUSIC_LOSE_HARKONNEN,
		/* musicBriefing        */ MUSIC_BRIEFING_HARKONNEN,
		/* voiceFilename        */ "nhark.voc",
		/* mentat               */ MENTAT_RADNOR,
		/* sampleSet            */ SAMPLESET_HARKONNEN,
		/* superWeapon          */ { .deathhand = 0 },
	},

	{ /* 1 */
		/* name                 */ "Atreides",
		/* toughness            */ 77,
		/* degradingChance      */ 0,
		/* degradingAmount      */ 1,
		/* minimapColor         */ 160,
		/* specialCountDown     */ 300,
		/* starportDeliveryTime */ 10,
		/* prefixChar           */ 'A',
		/* specialWeapon        */ HOUSE_WEAPON_FREMEN,
		/* musicWin             */ MUSIC_WIN_ATREIDES,
		/* musicLose            */ MUSIC_LOSE_ATREIDES,
		/* musicBriefing        */ MUSIC_BRIEFING_ATREIDES,
		/* voiceFilename        */ "nattr.voc",
		/* mentat               */ MENTAT_CYRIL,
		/* sampleSet            */ SAMPLESET_ATREIDES,
		/* superWeapon          */ { .fremen = { .owner = HOUSE_FREMEN, .unit75 = UNIT_TROOPERS, .unit25 = UNIT_TROOPER } },
	},

	{ /* 2 */
		/* name                 */ "Ordos",
		/* toughness            */ 128,
		/* degradingChance      */ 10,
		/* degradingAmount      */ 2,
		/* minimapColor         */ 176,
		/* specialCountDown     */ 300,
		/* starportDeliveryTime */ 10,
		/* prefixChar           */ 'O',
		/* specialWeapon        */ HOUSE_WEAPON_SABOTEUR,
		/* musicWin             */ MUSIC_WIN_ORDOS,
		/* musicLose            */ MUSIC_LOSE_ORDOS,
		/* musicBriefing        */ MUSIC_BRIEFING_ORDOS,
		/* voiceFilename        */ "nordo.voc",
		/* mentat               */ MENTAT_AMMON,
		/* sampleSet            */ SAMPLESET_ORDOS,
		/* superWeapon          */ { .saboteur = { .owner = HOUSE_ORDOS, .unit = UNIT_SABOTEUR } },
	},

	{ /* 3 */
		/* name                 */ "Fremen",
		/* toughness            */ 10,
		/* degradingChance      */ 0,
		/* degradingAmount      */ 1,
		/* minimapColor         */ 192,
		/* specialCountDown     */ 300,
		/* starportDeliveryTime */ 10, /* was 0. */
		/* prefixChar           */ 'A',
		/* specialWeapon        */ HOUSE_WEAPON_FREMEN,
		/* musicWin             */ MUSIC_WIN_ATREIDES,
		/* musicLose            */ MUSIC_LOSE_ATREIDES,
		/* musicBriefing        */ MUSIC_BRIEFING_ATREIDES,
		/* voiceFilename        */ "afremen.voc",
		/* mentat               */ MENTAT_CYRIL,
		/* sampleSet            */ SAMPLESET_ATREIDES,
		/* superWeapon          */ { .fremen = { .owner = HOUSE_FREMEN, .unit75 = UNIT_TROOPERS, .unit25 = UNIT_TROOPER } },
	},

	{ /* 4 */
		/* name                 */ "Sardaukar",
		/* toughness            */ 10,
		/* degradingChance      */ 0,
		/* degradingAmount      */ 1,
		/* minimapColor         */ 208,
		/* specialCountDown     */ 600,
		/* starportDeliveryTime */ 10, /* was 0. */
		/* prefixChar           */ 'H',
		/* specialWeapon        */ HOUSE_WEAPON_MISSILE,
		/* musicWin             */ MUSIC_WIN_HARKONNEN,
		/* musicLose            */ MUSIC_LOSE_HARKONNEN,
		/* musicBriefing        */ MUSIC_BRIEFING_HARKONNEN,
		/* voiceFilename        */ "asard.voc",
		/* mentat               */ MENTAT_RADNOR,
		/* sampleSet            */ SAMPLESET_HARKONNEN,
		/* superWeapon          */ { .deathhand = 0 },
	},

	{ /* 5 */
		/* name                 */ "Mercenary",
		/* toughness            */ 0,
		/* degradingChance      */ 0,
		/* degradingAmount      */ 1,
		/* minimapColor         */ 224,
		/* specialCountDown     */ 300,
		/* starportDeliveryTime */ 10, /* was 0. */
		/* prefixChar           */ 'O',
		/* specialWeapon        */ HOUSE_WEAPON_SABOTEUR,
		/* musicWin             */ MUSIC_WIN_ORDOS,
		/* musicLose            */ MUSIC_LOSE_ORDOS,
		/* musicBriefing        */ MUSIC_BRIEFING_ORDOS,
		/* voiceFilename        */ "amerc.voc",
		/* mentat               */ MENTAT_AMMON,
		/* sampleSet            */ SAMPLESET_ORDOS,
		/* superWeapon          */ { .saboteur = { .owner = HOUSE_MERCENARY, .unit = UNIT_SABOTEUR } },
	}
};

HouseInfo g_table_houseInfo[HOUSE_MAX];
enum HouseAlliance g_table_houseAlliance[HOUSE_MAX][HOUSE_MAX];

const enum HouseType g_table_houseRemap6to3[HOUSE_MAX] = {
	HOUSE_HARKONNEN, HOUSE_ATREIDES, HOUSE_ORDOS,
	HOUSE_ATREIDES, HOUSE_HARKONNEN, HOUSE_ORDOS
};
