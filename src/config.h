/* $Id$ */

/** @file src/config.h Configuration and options load and save definitions. */

#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

enum WindowMode {
	WM_WINDOWED,
	WM_FULLSCREEN,
	WM_FULLSCREEN_WINDOW
};

typedef struct GameCfg {
	enum WindowMode windowMode;
	unsigned int language;
	int gameSpeed;
	bool hints;
	bool autoScroll;
	int scrollSpeed;
} GameCfg;

extern GameCfg g_gameConfig;

extern void GameOptions_Load(void);
extern void GameOptions_Save(void);

#endif /* CONFIG_H */
