/* $Id$ */

/** @file src/config.h Configuration and options load and save definitions. */

#ifndef CONFIG_H
#define CONFIG_H

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
	bool scrollAlongScreenEdge;
	int scrollSpeed;

	/* "Right-click orders" control scheme:
	 * Left  -> select, selection box.
	 * Right -> order, pan.
	 *
	 * "Left-click orders" (Dune 2000) control scheme:
	 * Left  -> select/order, selection box.
	 * Right -> deselect, pan.
	 */
	bool leftClickOrders;
	bool holdControlToZoom;
} GameCfg;

extern GameCfg g_gameConfig;

extern void Config_GetCampaign(void);
extern void Config_SaveCampaignCompletion(void);
extern void GameOptions_Load(void);
extern void GameOptions_Save(void);

#endif /* CONFIG_H */
