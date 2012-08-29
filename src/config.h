/* $Id$ */

/** @file src/config.h Configuration and options load and save definitions. */

#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

MSVC_PACKED_BEGIN
/**
 * This is the layout of decoded dune.cfg.
 */
typedef struct DuneCfg {
	/* 0000(1)   */ PACK uint8  graphicDrv;                 /*!< Graphic mode to use. */
	/* 0001(1)   */ PACK uint8  musicDrv;                   /*!< Index into music drivers array. */
	/* 0002(1)   */ PACK uint8  soundDrv;                   /*!< Index into sound drivers array. */
	/* 0003(1)   */ PACK uint8  voiceDrv;                   /*!< Index into digitized sound drivers array. */
	/* 0004(1)   */ PACK bool   useMouse;                   /*!< Use Mouse. */
	/* 0005(1)   */ PACK bool   useXMS;                     /*!< Use Extended Memory. */
	/* 0006(1)   */ PACK uint8  variable_0006;              /*!< ?? */
	/* 0007(1)   */ PACK uint8  variable_0007;              /*!< ?? */
	/* 0008(1)   */ PACK uint8  language;                   /*!< @see Language. */
	/* 0009(1)   */ PACK uint8  checksum;                   /*!< Used to check validity on config data. See Config_Read(). */
} GCC_PACKED DuneCfg;
MSVC_PACKED_END
assert_compile(sizeof(DuneCfg) == 0xA);

typedef struct GameCfg {
	int gameSpeed;
	bool hints;
	bool autoScroll;
	int autoScrollDelay;
} GameCfg;

extern GameCfg g_gameConfig;
extern DuneCfg g_config;

extern void GameOptions_Load(void);
extern void GameOptions_Save(void);

#endif /* CONFIG_H */
