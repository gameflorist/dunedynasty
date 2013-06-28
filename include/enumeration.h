#ifndef ENUMERATION_H
#define ENUMERATION_H

enum Brain {
	BRAIN_NONE,
	BRAIN_HUMAN,
	BRAIN_CPU_ENEMY,
	BRAIN_CPU_ALLY,
};

enum HouseType {
	HOUSE_HARKONNEN = 0,
	HOUSE_ATREIDES  = 1,
	HOUSE_ORDOS     = 2,
	HOUSE_FREMEN    = 3,
	HOUSE_SARDAUKAR = 4,
	HOUSE_MERCENARY = 5,

	HOUSE_MAX       = 6,
	HOUSE_INVALID   = 0xFF
};

enum HouseFlag {
	FLAG_HOUSE_HARKONNEN    = 1 << HOUSE_HARKONNEN, /* 0x01 */
	FLAG_HOUSE_ATREIDES     = 1 << HOUSE_ATREIDES,  /* 0x02 */
	FLAG_HOUSE_ORDOS        = 1 << HOUSE_ORDOS,     /* 0x04 */
	FLAG_HOUSE_FREMEN       = 1 << HOUSE_FREMEN,    /* 0x08 */
	FLAG_HOUSE_SARDAUKAR    = 1 << HOUSE_SARDAUKAR, /* 0x10 */
	FLAG_HOUSE_MERCENARY    = 1 << HOUSE_MERCENARY, /* 0x20 */

	FLAG_HOUSE_ALL  = FLAG_HOUSE_MERCENARY | FLAG_HOUSE_SARDAUKAR | FLAG_HOUSE_FREMEN
	                | FLAG_HOUSE_ORDOS | FLAG_HOUSE_ATREIDES | FLAG_HOUSE_HARKONNEN
};

enum MentatID {
	MENTAT_RADNOR,
	MENTAT_CYRIL,
	MENTAT_AMMON,
	MENTAT_BENE_GESSERIT,
	MENTAT_CUSTOM,
	MENTAT_MAX
};

enum StructureType {
	STRUCTURE_SLAB_1x1          = 0,
	STRUCTURE_SLAB_2x2          = 1,
	STRUCTURE_PALACE            = 2,
	STRUCTURE_LIGHT_VEHICLE     = 3,
	STRUCTURE_HEAVY_VEHICLE     = 4,
	STRUCTURE_HIGH_TECH         = 5,
	STRUCTURE_HOUSE_OF_IX       = 6,
	STRUCTURE_WOR_TROOPER       = 7,
	STRUCTURE_CONSTRUCTION_YARD = 8,
	STRUCTURE_WINDTRAP          = 9,
	STRUCTURE_BARRACKS          = 10,
	STRUCTURE_STARPORT          = 11,
	STRUCTURE_REFINERY          = 12,
	STRUCTURE_REPAIR            = 13,
	STRUCTURE_WALL              = 14,
	STRUCTURE_TURRET            = 15,
	STRUCTURE_ROCKET_TURRET     = 16,
	STRUCTURE_SILO              = 17,
	STRUCTURE_OUTPOST           = 18,

	STRUCTURE_MAX       = 19,
	STRUCTURE_INVALID   = 0xFF
};

enum StructureFlag {
	FLAG_STRUCTURE_SLAB_1x1             = 1 << STRUCTURE_SLAB_1x1,          /* 0x____01 */
	FLAG_STRUCTURE_SLAB_2x2             = 1 << STRUCTURE_SLAB_2x2,          /* 0x____02 */
	FLAG_STRUCTURE_PALACE               = 1 << STRUCTURE_PALACE,            /* 0x____04 */
	FLAG_STRUCTURE_LIGHT_VEHICLE        = 1 << STRUCTURE_LIGHT_VEHICLE,     /* 0x____08 */
	FLAG_STRUCTURE_HEAVY_VEHICLE        = 1 << STRUCTURE_HEAVY_VEHICLE,     /* 0x____10 */
	FLAG_STRUCTURE_HIGH_TECH            = 1 << STRUCTURE_HIGH_TECH,         /* 0x____20 */
	FLAG_STRUCTURE_HOUSE_OF_IX          = 1 << STRUCTURE_HOUSE_OF_IX,       /* 0x____40 */
	FLAG_STRUCTURE_WOR_TROOPER          = 1 << STRUCTURE_WOR_TROOPER,       /* 0x____80 */
	FLAG_STRUCTURE_CONSTRUCTION_YARD    = 1 << STRUCTURE_CONSTRUCTION_YARD, /* 0x__01__ */
	FLAG_STRUCTURE_WINDTRAP             = 1 << STRUCTURE_WINDTRAP,          /* 0x__02__ */
	FLAG_STRUCTURE_BARRACKS             = 1 << STRUCTURE_BARRACKS,          /* 0x__04__ */
	FLAG_STRUCTURE_STARPORT             = 1 << STRUCTURE_STARPORT,          /* 0x__08__ */
	FLAG_STRUCTURE_REFINERY             = 1 << STRUCTURE_REFINERY,          /* 0x__10__ */
	FLAG_STRUCTURE_REPAIR               = 1 << STRUCTURE_REPAIR,            /* 0x__20__ */
	FLAG_STRUCTURE_WALL                 = 1 << STRUCTURE_WALL,              /* 0x__40__ */
	FLAG_STRUCTURE_TURRET               = 1 << STRUCTURE_TURRET,            /* 0x__80__ */
	FLAG_STRUCTURE_ROCKET_TURRET        = 1 << STRUCTURE_ROCKET_TURRET,     /* 0x01____ */
	FLAG_STRUCTURE_SILO                 = 1 << STRUCTURE_SILO,              /* 0x02____ */
	FLAG_STRUCTURE_OUTPOST              = 1 << STRUCTURE_OUTPOST,           /* 0x04____ */

	FLAG_STRUCTURE_NONE                 = 0,
	FLAG_STRUCTURE_NEVER                = 0xFFFF                            /*!< Special flag to mark that certain buildings can never be built on a Construction Yard. */
};

enum UnitType {
	UNIT_CARRYALL           = 0,
	UNIT_ORNITHOPTER        = 1,
	UNIT_INFANTRY           = 2,
	UNIT_TROOPERS           = 3,
	UNIT_SOLDIER            = 4,
	UNIT_TROOPER            = 5,
	UNIT_SABOTEUR           = 6,
	UNIT_LAUNCHER           = 7,
	UNIT_DEVIATOR           = 8,
	UNIT_TANK               = 9,
	UNIT_SIEGE_TANK         = 10,
	UNIT_DEVASTATOR         = 11,
	UNIT_SONIC_TANK         = 12,
	UNIT_TRIKE              = 13,
	UNIT_RAIDER_TRIKE       = 14,
	UNIT_QUAD               = 15,
	UNIT_HARVESTER          = 16,
	UNIT_MCV                = 17,
	UNIT_MISSILE_HOUSE      = 18,
	UNIT_MISSILE_ROCKET     = 19,
	UNIT_MISSILE_TURRET     = 20,
	UNIT_MISSILE_DEVIATOR   = 21,
	UNIT_MISSILE_TROOPER    = 22,
	UNIT_BULLET             = 23,
	UNIT_SONIC_BLAST        = 24,
	UNIT_SANDWORM           = 25,
	UNIT_FRIGATE            = 26,

	UNIT_MAX        = 27,
	UNIT_INVALID    = 0xFF
};

enum UnitFlag {
	FLAG_UNIT_CARRYALL          = 1 << UNIT_CARRYALL,           /* 0x______01 */
	FLAG_UNIT_ORNITHOPTER       = 1 << UNIT_ORNITHOPTER,        /* 0x______02 */
	FLAG_UNIT_INFANTRY          = 1 << UNIT_INFANTRY,           /* 0x______04 */
	FLAG_UNIT_TROOPERS          = 1 << UNIT_TROOPERS,           /* 0x______08 */
	FLAG_UNIT_SOLDIER           = 1 << UNIT_SOLDIER,            /* 0x______10 */
	FLAG_UNIT_TROOPER           = 1 << UNIT_TROOPER,            /* 0x______20 */
	FLAG_UNIT_SABOTEUR          = 1 << UNIT_SABOTEUR,           /* 0x______40 */
	FLAG_UNIT_LAUNCHER          = 1 << UNIT_LAUNCHER,           /* 0x______80 */
	FLAG_UNIT_DEVIATOR          = 1 << UNIT_DEVIATOR,           /* 0x____01__ */
	FLAG_UNIT_TANK              = 1 << UNIT_TANK,               /* 0x____02__ */
	FLAG_UNIT_SIEGE_TANK        = 1 << UNIT_SIEGE_TANK,         /* 0x____04__ */
	FLAG_UNIT_DEVASTATOR        = 1 << UNIT_DEVASTATOR,         /* 0x____08__ */
	FLAG_UNIT_SONIC_TANK        = 1 << UNIT_SONIC_TANK,         /* 0x____10__ */
	FLAG_UNIT_TRIKE             = 1 << UNIT_TRIKE,              /* 0x____20__ */
	FLAG_UNIT_RAIDER_TRIKE      = 1 << UNIT_RAIDER_TRIKE,       /* 0x____40__ */
	FLAG_UNIT_QUAD              = 1 << UNIT_QUAD,               /* 0x____80__ */
	FLAG_UNIT_HARVESTER         = 1 << UNIT_HARVESTER,          /* 0x__01____ */
	FLAG_UNIT_MCV               = 1 << UNIT_MCV,                /* 0x__02____ */
	FLAG_UNIT_MISSILE_HOUSE     = 1 << UNIT_MISSILE_HOUSE,      /* 0x__04____ */
	FLAG_UNIT_MISSILE_ROCKET    = 1 << UNIT_MISSILE_ROCKET,     /* 0x__08____ */
	FLAG_UNIT_MISSILE_TURRET    = 1 << UNIT_MISSILE_TURRET,     /* 0x__10____ */
	FLAG_UNIT_MISSILE_DEVIATOR  = 1 << UNIT_MISSILE_DEVIATOR,   /* 0x__20____ */
	FLAG_UNIT_MISSILE_TROOPER   = 1 << UNIT_MISSILE_TROOPER,    /* 0x__40____ */
	FLAG_UNIT_BULLET            = 1 << UNIT_BULLET,             /* 0x__80____ */
	FLAG_UNIT_SONIC_BLAST       = 1 << UNIT_SONIC_BLAST,        /* 0x01______ */
	FLAG_UNIT_SANDWORM          = 1 << UNIT_SANDWORM,           /* 0x02______ */
	FLAG_UNIT_FRIGATE           = 1 << UNIT_FRIGATE,            /* 0x04______ */

	FLAG_UNIT_NONE              = 0
};

enum UnitActionType {
	ACTION_ATTACK       = 0,
	ACTION_MOVE         = 1,
	ACTION_RETREAT      = 2,
	ACTION_GUARD        = 3,
	ACTION_AREA_GUARD   = 4,
	ACTION_HARVEST      = 5,
	ACTION_RETURN       = 6,
	ACTION_STOP         = 7,
	ACTION_AMBUSH       = 8,
	ACTION_SABOTAGE     = 9,
	ACTION_DIE          = 10,
	ACTION_HUNT         = 11,
	ACTION_DEPLOY       = 12,
	ACTION_DESTRUCT     = 13,

	ACTION_MAX      = 14,
	ACTION_INVALID  = 0xFF
};

typedef enum UnitActionType ActionType;

enum UnitDisplayMode {
	DISPLAYMODE_SINGLE_FRAME        = 0,
	DISPLAYMODE_UNIT                = 1, /* Ground: N,NE,E,SE,S.  Air: N,NE,E. */
	DISPLAYMODE_ROCKET              = 2, /* N,NNE,NE,ENE,E. */
	DISPLAYMODE_INFANTRY_3_FRAMES   = 3, /* N,E,S; 3 frames per direction. */
	DISPLAYMODE_INFANTRY_4_FRAMES   = 4, /* N,E,S; 4 frames per direction. */
	DISPLAYMODE_ORNITHOPTER         = 5, /* N,NE,E; 3 frames per direction. */
};

enum UnitMovementType {
	MOVEMENT_FOOT       = 0,
	MOVEMENT_TRACKED    = 1,
	MOVEMENT_HARVESTER  = 2,
	MOVEMENT_WHEELED    = 3,
	MOVEMENT_WINGER     = 4,
	MOVEMENT_SLITHER    = 5,

	MOVEMENT_MAX        = 6,
	MOVEMENT_INVALID    = 0xFF
};

#endif
