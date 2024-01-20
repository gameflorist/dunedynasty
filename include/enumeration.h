#ifndef ENUMERATION_H
#define ENUMERATION_H

#include "enum_house.h"
#include "enum_structure.h"
#include "enum_unit.h"

enum Brain {
	BRAIN_NONE,
	BRAIN_HUMAN,
	BRAIN_CPU,
};

enum PlayerTeam {
	TEAM_NONE,
	TEAM_1,
	TEAM_2,
	TEAM_3,
	TEAM_4,
	TEAM_5,
	TEAM_6,
	TEAM_MAX,
};

enum MatchType {
	MATCHTYPE_SKIRMISH,
	MATCHTYPE_MULTIPLAYER,
};

typedef struct PlayerConfig {
	enum Brain brain;
	enum PlayerTeam team;
	enum HouseType houseID;
	enum MatchType matchType;
} PlayerConfig;

enum TileUnveilCause {
	UNVEILCAUSE_UNCHANGED,

	/* Short reveal. */
	UNVEILCAUSE_EXPLOSION,

	/* Long reveals. */
	UNVEILCAUSE_STRUCTURE_PLACED,
	UNVEILCAUSE_STRUCTURE_SCRIPT,
	UNVEILCAUSE_UNIT_UPDATE,
	UNVEILCAUSE_UNIT_SCRIPT,
	UNVEILCAUSE_BULLET_FIRED,

	/* Client side. */
	UNVEILCAUSE_STRUCTURE_VISION,
	UNVEILCAUSE_UNIT_VISION,
	UNVEILCAUSE_INITIALISATION,
	UNVEILCAUSE_SHORT,
	UNVEILCAUSE_LONG,
};

enum MentatID {
	MENTAT_RADNOR,
	MENTAT_CYRIL,
	MENTAT_AMMON,
	MENTAT_BENE_GESSERIT,
	MENTAT_CUSTOM,
	MENTAT_MAX
};

#endif
