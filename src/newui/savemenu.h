#ifndef NEWUI_SAVEMENU_H
#define NEWUI_SAVEMENU_H

#include "types.h"

extern char g_savegameDesc[5][51];

extern int SaveMenu_Savegame_Click(uint16 key);
extern void SaveMenu_InitSaveLoad(bool save);
extern int SaveMenu_SaveLoad_Click(bool save);

#endif
