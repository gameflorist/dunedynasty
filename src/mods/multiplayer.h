#ifndef MODS_MULTIPLAYER_H
#define MODS_MULTIPLAYER_H

#include "enumeration.h"

typedef struct Multiplayer {
	int client[HOUSE_MAX];
} Multiplayer;

extern Multiplayer g_multiplayer;

#endif
