#ifndef MAPGENERATOR_SKIRMISH_H
#define MAPGENERATOR_SKIRMISH_H

#include <stdbool.h>

extern bool Skirmish_IsPlayable(void);
extern void Skirmish_Prepare(void);
extern bool Skirmish_GenerateMap(bool newseed);

#endif
