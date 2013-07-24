/** @file src/table/actioninfo.c ActionInfo file table. */

#include <stdio.h>
#include "enum_string.h"
#include "types.h"

#include "../gui/gui.h"
#include "../unit.h"
#include "sound.h"

const ActionInfo g_table_actionInfo[ACTION_MAX] = {
	{ /* 0 */
		/* stringID      */ STR_ATTACK,
		/* name          */ "Attack",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_TARGET,
		/* soundID       */ SAMPLE_INFANTRY_OUT
	},

	{ /* 1 */
		/* stringID      */ STR_MOVE,
		/* name          */ "Move",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_TARGET,
		/* soundID       */ SAMPLE_MOVING_OUT
	},

	{ /* 2 */
		/* stringID      */ STR_RETREAT,
		/* name          */ "Retreat",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_INFANTRY_OUT
	},

	{ /* 3 */
		/* stringID      */ STR_GUARD,
		/* name          */ "Guard",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_INFANTRY_OUT
	},

	{ /* 4 */
		/* stringID      */ STR_AREA_GUARD,
		/* name          */ "Area Guard",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_ACKNOWLEDGED
	},

	{ /* 5 */
		/* stringID      */ STR_HARVEST,
		/* name          */ "Harvest",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_TARGET,
		/* soundID       */ SAMPLE_ACKNOWLEDGED
	},

	{ /* 6 */
		/* stringID      */ STR_RETURN,
		/* name          */ "Return",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_INFANTRY_OUT
	},

	{ /* 7 */
		/* stringID      */ STR_STOP2,
		/* name          */ "Stop",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_INFANTRY_OUT
	},

	{ /* 8 */
		/* stringID      */ STR_AMBUSH,
		/* name          */ "Ambush",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_ACKNOWLEDGED
	},

	{ /* 9 */
		/* stringID      */ STR_SABOTAGE,
		/* name          */ "Sabotage",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_ACKNOWLEDGED
	},

	{ /* 10 */
		/* stringID      */ STR_DIE,
		/* name          */ "Die",
		/* switchType    */ 1,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_INVALID
	},

	{ /* 11 */
		/* stringID      */ STR_HUNT,
		/* name          */ "Hunt",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_ACKNOWLEDGED
	},

	{ /* 12 */
		/* stringID      */ STR_DEPLOY,
		/* name          */ "Deploy",
		/* switchType    */ 0,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_ACKNOWLEDGED
	},

	{ /* 13 */
		/* stringID      */ STR_DESTRUCT,
		/* name          */ "Destruct",
		/* switchType    */ 1,
		/* selectionType */ SELECTIONTYPE_UNIT,
		/* soundID       */ SAMPLE_ACKNOWLEDGED
	}
};
