/* $Id$ */

/** @file src/table/sound.c Sound file tables. */

#include <stdio.h>

#include "sound.h"

#define D2TM_ADLIB_PREFIX   "d2tm_adlib"
#define D2TM_MT32_PREFIX    "d2tm_mt32"
#define D2TM_SC55_PREFIX    "d2tm_sc55"
#define DUNE2000_PREFIX     "dune2000"
#define DUNE2_SMD_PREFIX    "dune2_smd"
#define FED2K_MT32_PREFIX   "fed2k_mt32"

#define ADD_MUSIC_FROM_DUNE2_ADLIB(FILENAME,TRACK)  { true, MUSICSET_DUNE2_ADLIB, FILENAME, TRACK, 1.0f }
#define ADD_MUSIC_FROM_DUNE2_C55(FILENAME,TRACK)    { true, MUSICSET_DUNE2_C55,   FILENAME, TRACK, 1.0f }
#define ADD_MUSIC_FROM_FED2K_MT32(FILENAME)         { true, MUSICSET_FED2K_MT32,  FED2K_MT32_PREFIX "/" FILENAME, 0, 0.65f }
#define ADD_MUSIC_FROM_D2TM_ADLIB(FILENAME,VOLUME)  { true, MUSICSET_D2TM_ADLIB,  D2TM_ADLIB_PREFIX "/" FILENAME, 0, VOLUME }
#define ADD_MUSIC_FROM_D2TM_MT32(FILENAME)          { true, MUSICSET_D2TM_MT32,   D2TM_MT32_PREFIX  "/" FILENAME, 0, 0.65f }
#define ADD_MUSIC_FROM_D2TM_SC55(FILENAME,VOLUME)   { true, MUSICSET_D2TM_SC55,   D2TM_SC55_PREFIX  "/" FILENAME, 0, VOLUME }
#define ADD_MUSIC_FROM_DUNE2_SMD(FILENAME,VOLUME)   { true, MUSICSET_DUNE2_SMD,   DUNE2_SMD_PREFIX  "/" FILENAME, 0, VOLUME }
#define ADD_MUSIC_FROM_DUNE2000(FILENAME,VOLUME)    { true, MUSICSET_DUNE2000,    DUNE2000_PREFIX   "/" FILENAME, 0, VOLUME }

const enum MusicID g_table_music_cutoffs[21] = {
	MUSIC_LOGOS,
	MUSIC_INTRO,
	MUSIC_CUTSCENE,
	MUSIC_CREDITS,
	MUSIC_MAIN_MENU,
	MUSIC_STRATEGIC_MAP,
	MUSIC_BRIEFING_HARKONNEN,
	MUSIC_BRIEFING_ATREIDES,
	MUSIC_BRIEFING_ORDOS,
	MUSIC_WIN_HARKONNEN,
	MUSIC_WIN_ATREIDES,
	MUSIC_WIN_ORDOS,
	MUSIC_LOSE_HARKONNEN,
	MUSIC_LOSE_ATREIDES,
	MUSIC_LOSE_ORDOS,
	MUSIC_END_GAME_HARKONNEN,
	MUSIC_END_GAME_ATREIDES,
	MUSIC_END_GAME_ORDOS,
	MUSIC_IDLE1,
	/* Note: MUSIC_BONUS not a cut-off, so you use is as idle music. */
	MUSIC_ATTACK1,
	MUSICID_MAX
};

MusicSetInfo g_table_music_set[NUM_MUSIC_SETS] = {
	{ true, "dune2_adlib" },
	{ true, "dune2_c55" },
	{ true, FED2K_MT32_PREFIX },
	{ true, D2TM_ADLIB_PREFIX },
	{ true, D2TM_MT32_PREFIX },
	{ true, D2TM_SC55_PREFIX },
	{ true, DUNE2_SMD_PREFIX },
	{ true, DUNE2000_PREFIX },
};

MusicInfo g_table_music[MUSICID_MAX] = {
	/* 0: MUSIC_STOP */
	{ false, MUSICSET_DUNE2_ADLIB, NULL, 0, 0.0f },

	/* These are kept for historical reasons, even though are silent. */

	/* 1: MUSIC_23 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 8),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune1.C55", 8),

	/* 3: MUSIC_35 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune0.C55", 3),

	/* 5: MUSIC_37 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 5),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune0.C55", 5),

	/* MUSIC_LOGOS */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune0.C55", 4),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_00_4"),

	/* MUSIC_INTRO */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune0.C55", 2),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_00_2"),
	ADD_MUSIC_FROM_D2TM_MT32    ("intro"),
	ADD_MUSIC_FROM_D2TM_SC55    ("intro", 1.0f),

	/* MUSIC_CUTSCENE */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune16.ADL", 8),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune16.C55", 8),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_16_24"),

	/* MUSIC_CREDITS */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune20.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune20.C55", 2),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_20_22"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("20_credits", 1.0f),

	/* MUSIC_MAIN_MENU */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune7.C55", 6),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_07_13"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("menu", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("menu"),
	ADD_MUSIC_FROM_D2TM_SC55    ("menu", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("12_chosendestiny", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("OPTIONS", 1.0f),

	/* MUSIC_STRATEGIC_MAP */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune16.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune16.C55", 7),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_16_23"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("nextconq", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("nextconq"),
	ADD_MUSIC_FROM_D2TM_SC55    ("nextconq", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("11_evasiveaction", 1.0f),

	/* MUSIC_BRIEFING_HARKONNEN */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune7.C55", 2),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_07_09"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentath", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("mentath"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentath", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("04_radnorsscheme", 1.0f),

	/* MUSIC_BRIEFING_ATREIDES */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune7.C55", 3),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_07_10"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentata", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("mentata"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentata", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("02_cyrilscouncil", 1.0f),

	/* MUSIC_BRIEFING_ORDOS */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune7.C55", 4),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_07_11"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentato", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("mentato"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentato", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("03_ammonsadvice", 1.0f),

	/* MUSIC_WIN_HARKONNEN */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune8.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune8.C55", 3),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_08_11"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win3", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("win2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win2", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("15_harkonnenrules", 1.0f),

	/* MUSIC_WIN_ATREIDES */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune8.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune8.C55", 2),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_08_10"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win1", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("win1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win1", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("13_conquest", 1.0f),

	/* MUSIC_WIN_ORDOS */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune17.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune17.C55", 4),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_17_21"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win2", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("win3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win3", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("14_slitherin", 1.0f),

	/* MUSIC_LOSE_HARKONNEN */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune1.C55", 4),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_01_4"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose2", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("lose1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose1", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("18_harkonnendirge", 1.0f),

	/* MUSIC_LOSE_ATREIDES */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 5),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune1.C55", 5),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_01_5"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose1", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("lose2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose2", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("16_atreidesdirge", 1.0f),

	/* MUSIC_LOSE_ORDOS */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune1.C55", 3),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_01_6"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose3", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("lose3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose3", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("17_ordosdirge", 1.0f),

	/* MUSIC_END_GAME_HARKONNEN */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune19.C55", 4),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_19_23"),

	/* MUSIC_END_GAME_ATREIDES */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune19.C55", 2),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_19_21"),

	/* MUSIC_END_GAME_ORDOS */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune19.C55", 3),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_19_22"),

	/* MUSIC_IDLE1 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune1.C55", 6),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_01_7"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace2", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace2", 1.0f),

	/* MUSIC_IDLE2 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune2.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune2.C55", 6),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_02_8"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace5", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace5", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("SPICESCT", 1.0f),

	/* MUSIC_IDLE3 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune3.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune3.C55", 6),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_03_9"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace4", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace4", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("07_spicetrip", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("RISEHARK", 1.0f),

	/* MUSIC_IDLE4 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune4.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune4.C55", 6),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_04_10"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace1", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace4"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace1", 1.0f),

	/* MUSIC_IDLE5 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune5.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune5.C55", 6),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_05_11"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace9", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace5"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace9", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("06_turbulence", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("UNDERCON", 1.0f),

	/* MUSIC_IDLE6 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune6.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune6.C55", 6),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_06_12"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace8", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace6"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace8", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("ATREGAIN", 1.0f),

	/* MUSIC_IDLE7 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune9.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune9.C55", 4),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_09_13"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace7", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace7"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace7", 1.0f),

	/* MUSIC_IDLE8 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune9.ADL", 5),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune9.C55", 5),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_09_14"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace6", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace8"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace6", 1.0f),

	/* MUSIC_IDLE9 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune18.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune18.C55", 6),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_18_24"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace3", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace9"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace3", 1.0f),

	/* MUSIC_IDLE_OTHER */
	ADD_MUSIC_FROM_DUNE2_SMD    ("05_thelegotune", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("08_commandpost", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("09_trenching", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("AMBUSH", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("ENTORDOS", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("FREMEN", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("LANDSAND", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("PLOTTING", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("ROBOTIX", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("SOLDAPPR", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("WAITGAME", 1.0f),

	/* MUSIC_BONUS (disabled by default). */
	{ false, MUSICSET_DUNE2_ADLIB, "dune1.ADL", 2, 0.0f },
	{ false, MUSICSET_DUNE2_C55,   "dune1.C55", 2, 0.0f },
	{ false, MUSICSET_FED2K_MT32,  FED2K_MT32_PREFIX "/dune2_mt32_01_3", 0, 0.65f },
	{ false, MUSICSET_DUNE2_SMD,   DUNE2_SMD_PREFIX  "/10_starport", 0, 1.0f },

	/* MUSIC_ATTACK1 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune10.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune10.C55", 7),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_10_17"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack5", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack4", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("ARAKATAK", 1.0f),

	/* MUSIC_ATTACK2 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune11.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune11.C55", 7),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_11_18"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack3", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack5", 1.0f),

	/* MUSIC_ATTACK3 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune12.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune12.C55", 7),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_12_19"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack6", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack2", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("HARK_BAT", 1.0f),

	/* MUSIC_ATTACK4 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune13.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune13.C55", 7),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_13_20"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack2", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack4"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack3", 1.0f),

	/* MUSIC_ATTACK5 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune14.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune14.C55", 7),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_14_21"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack4", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack5"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack1", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("FIGHTPWR", 1.0f),

	/* MUSIC_ATTACK6 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune15.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_C55    ("dune15.C55", 7),
	ADD_MUSIC_FROM_FED2K_MT32   ("dune2_mt32_15_22"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack1", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack6"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack6", 1.0f),
};

/** Available voices. */
const SoundData g_table_voices[SAMPLEID_MAX] = {
	{"+VSCREAM1.VOC",  11}, /*   0 */
	{"+EXSAND.VOC",    10}, /*   1 */
	{"+ROCKET.VOC",    11}, /*   2 */
	{"+BUTTON.VOC",    10}, /*   3 */
	{"+VSCREAM5.VOC",  11}, /*   4 */
	{"+CRUMBLE.VOC",   15}, /*   5 */
	{"+EXSMALL.VOC",    9}, /*   6 */
	{"+EXMED.VOC",     10}, /*   7 */
	{"+EXLARGE.VOC",   14}, /*   8 */
	{"+EXCANNON.VOC",  11}, /*   9 */
	{"+GUNMULTI.VOC",   9}, /*  10 */
	{"+GUN.VOC",       10}, /*  11 */
	{"+EXGAS.VOC",     10}, /*  12 */
	{"+EXDUD.VOC",     10}, /*  13 */
	{"+VSCREAM2.VOC",  11}, /*  14 */
	{"+VSCREAM3.VOC",  11}, /*  15 */
	{"+VSCREAM4.VOC",  11}, /*  16 */
	{"+%cAFFIRM.VOC",  15}, /*  17 */
	{"+%cREPORT1.VOC", 15}, /*  18 */
	{"+%cREPORT2.VOC", 15}, /*  19 */
	{"+%cREPORT3.VOC", 15}, /*  20 */
	{"+%cOVEROUT.VOC", 15}, /*  21 */
	{"+%cMOVEOUT.VOC", 15}, /*  22 */
	{"?POPPA.VOC",     15}, /*  23 */
	{"?SANDBUG.VOC",   15}, /*  24 */
	{"+STATICP.VOC",   10}, /*  25 */
	{"+WORMET3P.VOC",  16}, /*  26 */
	{"+MISLTINP.VOC",  10}, /*  27 */
	{"+SQUISH2.VOC",   12}, /*  28 */
	{"%cENEMY.VOC",    20}, /*  29 */
	{"%cHARK.VOC",     20}, /*  30 */
	{"%cATRE.VOC",     20}, /*  31 */
	{"%cORDOS.VOC",    20}, /*  32 */
	{"%cFREMEN.VOC",   20}, /*  33 */
	{"%cSARD.VOC",     20}, /*  34 */
	{"FILLER.VOC",     20}, /*  35 */
	{"%cUNIT.VOC",     20}, /*  36 */
	{"%cSTRUCT.VOC",   20}, /*  37 */
	{"%cONE.VOC",      19}, /*  38 */
	{"%cTWO.VOC",      19}, /*  39 */
	{"%cTHREE.VOC",    19}, /*  40 */
	{"%cFOUR.VOC",     19}, /*  41 */
	{"%cFIVE.VOC",     19}, /*  42 */
	{"%cCONST.VOC",    20}, /*  43 */
	{"%cRADAR.VOC",    20}, /*  44 */
	{"%cOFF.VOC",      20}, /*  45 */
	{"%cON.VOC",       20}, /*  46 */
	{"%cFRIGATE.VOC",  20}, /*  47 */
	{"?%cARRIVE.VOC",  20}, /*  48 */
	{"%cWARNING.VOC",  20}, /*  49 */
	{"%cSABOT.VOC",    20}, /*  50 */
	{"%cMISSILE.VOC",  20}, /*  51 */
	{"%cBLOOM.VOC",    20}, /*  52 */
	{"%cDESTROY.VOC",  20}, /*  53 */
	{"%cDEPLOY.VOC",   20}, /*  54 */
	{"%cAPPRCH.VOC",   20}, /*  55 */
	{"%cLOCATED.VOC",  20}, /*  56 */
	{"%cNORTH.VOC",    20}, /*  57 */
	{"%cEAST.VOC",     20}, /*  58 */
	{"%cSOUTH.VOC",    20}, /*  59 */
	{"%cWEST.VOC",     20}, /*  60 */
	{"?%cWIN.VOC",     20}, /*  61 */
	{"?%cLOSE.VOC",    20}, /*  62 */
	{"%cLAUNCH.VOC",   20}, /*  63 */
	{"%cATTACK.VOC",   20}, /*  64 */
	{"%cVEHICLE.VOC",  20}, /*  65 */
	{"%cREPAIR.VOC",   20}, /*  66 */
	{"%cHARVEST.VOC",  20}, /*  67 */
	{"%cWORMY.VOC",    20}, /*  68 */
	{"%cCAPTURE.VOC",  20}, /*  69 */
	{"%cNEXT.VOC",     20}, /*  70 */
	{"%cNEXT2.VOC",    20}, /*  71 */
	{"/BLASTER.VOC",   10}, /*  72 */
	{"/GLASS6.VOC",    10}, /*  73 */
	{"/LIZARD1.VOC",   10}, /*  74 */
	{"/FLESH.VOC",     10}, /*  75 */
	{"/CLICK.VOC",     10}, /*  76 */
	{"-3HOUSES.VOC",   12}, /*  77 */
	{"-ANDNOW.VOC",    12}, /*  78 */
	{"-ARRIVED.VOC",   12}, /*  79 */
	{"-BATTLE.VOC",    12}, /*  80 */
	{"-BEGINS.VOC",    12}, /*  81 */
	{"-BLDING.VOC",    12}, /*  82 */
	{"-CONTROL2.VOC",  12}, /*  83 */
	{"-CONTROL3.VOC",  12}, /*  84 */
	{"-CONTROL4.VOC",  12}, /*  85 */
	{"-CONTROLS.VOC",  12}, /*  86 */
	{"-DUNE.VOC",      12}, /*  87 */
	{"-DYNASTY.VOC",   12}, /*  88 */
	{"-EACHHOME.VOC",  12}, /*  89 */
	{"-EANDNO.VOC",    12}, /*  90 */
	{"-ECONTROL.VOC",  12}, /*  91 */
	{"-EHOUSE.VOC",    12}, /*  92 */
	{"-EMPIRE.VOC",    12}, /*  93 */
	{"-EPRODUCE.VOC",  12}, /*  94 */
	{"-ERULES.VOC",    12}, /*  95 */
	{"-ETERRIT.VOC",   12}, /*  96 */
	{"-EMOST.VOC",     12}, /*  97 */
	{"-ENOSET.VOC",    12}, /*  98 */
	{"-EVIL.VOC",      12}, /*  99 */
	{"-HARK.VOC",      12}, /* 100 */
	{"-HOME.VOC",      12}, /* 101 */
	{"-HOUSE2.VOC",    12}, /* 102 */
	{"-INSID.VOC",     12}, /* 103 */
	{"-KING.VOC",      12}, /* 104 */
	{"-KNOWN.VOC",     12}, /* 105 */
	{"-MELANGE.VOC",   12}, /* 106 */
	{"-NOBLE.VOC",     12}, /* 107 */
	{"?NOW.VOC",       12}, /* 108 */
	{"-OFDUNE.VOC",    12}, /* 109 */
	{"-ORD.VOC",       12}, /* 110 */
	{"-PLANET.VOC",    12}, /* 111 */
	{"-PREVAIL.VOC",   12}, /* 112 */
	{"-PROPOSED.VOC",  12}, /* 113 */
	{"-SANDLAND.VOC",  12}, /* 114 */
	{"-SPICE.VOC",     12}, /* 115 */
	{"-SPICE2.VOC",    12}, /* 116 */
	{"-VAST.VOC",      12}, /* 117 */
	{"-WHOEVER.VOC",   12}, /* 118 */
	{"?YOUR.VOC",      12}, /* 119 */
	{"?FILLER.VOC",    12}, /* 120 */
	{"-DROPEQ2P.VOC",  10}, /* 121 */
	{"/EXTINY.VOC",    10}, /* 122 */
	{"-WIND2BP.VOC",   10}, /* 123 */
	{"-BRAKES2P.VOC",  11}, /* 124 */
	{"-GUNSHOT.VOC",   10}, /* 125 */
	{"-GLASS.VOC",     11}, /* 126 */
	{"-MISSLE8.VOC",   10}, /* 127 */
	{"-CLANK.VOC",     10}, /* 128 */
	{"-BLOWUP1.VOC",   10}, /* 129 */
	{"-BLOWUP2.VOC",   11}  /* 130 */
};

/* Map soundID -> sampleID.
 * Sounds mapped to SAMPLE_INVALID are effects.
 */
const enum SampleID g_table_voiceMapping[SOUNDID_MAX] = {
	0xFFFF, /*   0 */
	0xFFFF, /*   1 */
	0xFFFF, /*   2 */
	0xFFFF, /*   3 */
	0xFFFF, /*   4 */
	0xFFFF, /*   5 */
	0xFFFF, /*   6 */
	0xFFFF, /*   7 */
	0xFFFF, /*   8 */
	0xFFFF, /*   9 */
	0xFFFF, /*  10 */
	0xFFFF, /*  11 */
	0xFFFF, /*  12 */
	0xFFFF, /*  13 */
	0xFFFF, /*  14 */
	0xFFFF, /*  15 */
	0xFFFF, /*  16 */
	0xFFFF, /*  17 */
	0xFFFF, /*  18 */
	0xFFFF, /*  19 */
	SAMPLE_PLACEMENT,   /*  20 */
	0xFFFF, /*  21 */
	0xFFFF, /*  22 */
	0xFFFF, /*  23 */
	SAMPLE_DROP_LOAD,   /*  24 */
	0xFFFF, /*  25 */
	0xFFFF, /*  26 */
	0xFFFF, /*  27 */
	0xFFFF, /*  28 */
	0xFFFF, /*  29 */
	SAMPLE_SCREAM1, /*  30 */
	SAMPLE_SCREAM5, /*  31 */
	SAMPLE_SCREAM2, /*  32 */
	SAMPLE_SCREAM3, /*  33 */
	SAMPLE_SCREAM4, /*  34 */
	SAMPLE_SQUISH,  /*  35 */
	0xFFFF, /*  36 */
	0xFFFF, /*  37 */
	SAMPLE_BUTTON,          /*  38 */
	SAMPLE_EXPLODE_GAS,     /*  39: s_explosion07 */
	SAMPLE_EXPLODE_SAND,    /*  40: s_explosion08, s_explosion19 */
	SAMPLE_EXPLODE_MEDIUM,  /*  41: s_explosion05, s_explosion09 */
	SAMPLE_ROCKET,          /*  42: Death Hand, Launcher, Rocket Turret, Deviator */
	0xFFFF, /*  43: Sonic Tank */
	SAMPLE_STRUCTURE_DESTROYED, /*  44 */
	0xFFFF, /*  45 */
	0xFFFF, /*  46 */
	0xFFFF, /*  47 */
	0xFFFF, /*  48 */
	SAMPLE_EXPLODE_MEDIUM,  /*  49: s_explosion03, s_explosion10, s_explosion15, s_explosion16, s_explosion17 */
	SAMPLE_EXPLODE_SMALL,   /*  50: s_explosion02 */
	SAMPLE_EXPLODE_LARGE,   /*  51: s_explosion04, s_explosion06, s_explosion11, s_explosion14 */
	0xFFFF, /*  52 */
	0xFFFF, /*  53 */
	SAMPLE_EXPLODE_TINY,    /*  54: s_explosion18 */
	0xFFFF, /*  55 */
	SAMPLE_EXPLODE_CANNON,  /*  56 */
	SAMPLE_EXPLODE_CANNON,  /*  57: Tank, Siege Tank, Devastator Tank */
	SAMPLE_GUN,             /*  58: Soldier, Soldiers, Saboteur */
	SAMPLE_MACHINE_GUN,     /*  59: Trike, Raider Trike, Quad, Trooper(s) at close range */
	SAMPLE_VOICE_FRAGMENT_CONSTRUCTION_COMPLETE,    /*  60 */
	0xFFFF, /*  61 */
	SAMPLE_RADAR_STATIC,    /*  62 */
	SAMPLE_SANDWORM,        /*  63 */
	SAMPLE_MINI_ROCKET,     /*  64: Ornithopter, Trooper(s) at long range */
	SAMPLE_BLASTER,         /*  65: Harkonnen end game */
	SAMPLE_GLASS,           /*  66: Harkonnen end game */
	SAMPLE_LIZARD,          /*  67: Ordos end game */
	SAMPLE_FLESH,           /*  68: Harkonnen end game */
	SAMPLE_CLICK,           /*  69: Harkonnen end game */
	0xFFFF, /*  70 */
	0xFFFF, /*  71 */
	0xFFFF, /*  72 */
	0xFFFF, /*  73 */
	0xFFFF, /*  74 */
	0xFFFF, /*  75 */
	0xFFFF, /*  76 */
	0xFFFF, /*  77 */
	0xFFFF, /*  78 */
	0xFFFF, /*  79 */
	0xFFFF, /*  80 */
	0xFFFF, /*  81 */
	0xFFFF, /*  82 */
	0xFFFF, /*  83 */
	0xFFFF, /*  84 */
	0xFFFF, /*  85 */
	0xFFFF, /*  86 */
	0xFFFF, /*  87 */
	0xFFFF, /*  88 */
	0xFFFF, /*  89 */
	0xFFFF, /*  90 */
	0xFFFF, /*  91 */
	0xFFFF, /*  92 */
	0xFFFF, /*  93 */
	0xFFFF, /*  94 */
	0xFFFF, /*  95 */
	0xFFFF, /*  96 */
	0xFFFF, /*  97 */
	0xFFFF, /*  98 */
	0xFFFF, /*  99 */
	0xFFFF, /* 100 */
	0xFFFF, /* 101 */
	0xFFFF, /* 102 */
	0xFFFF, /* 103 */
	0xFFFF, /* 104 */
	0xFFFF, /* 105 */
	0xFFFF, /* 106 */
	0xFFFF, /* 107 */
	SAMPLE_INTRO_WIND,  /* 108 */
	0xFFFF, /* 109 */
	SAMPLE_INTRO_HARVESTER, /* 110 */
	0xFFFF, /* 111 */
	SAMPLE_INTRO_FIRE_ORNITHOPTER_TROOPER,  /* 112 */
	SAMPLE_INTRO_BREAK_GLASS,   /* 113 */
	SAMPLE_INTRO_FIRE_LAUNCHER, /* 114 */
	0xFFFF, /* 115 */
	0xFFFF, /* 116 */
	SAMPLE_INTRO_CARRYALL_FRIGATE,  /* 117 */
	SAMPLE_INTRO_TROOPER_EXPLODE1,  /* 118 */
	SAMPLE_INTRO_TROOPER_EXPLODE2   /* 119 */
};

/**
 * Feedback on events and user commands (English audio, viewport message, and sound).
 * @see g_translatedVoice
 */
const Feedback g_feedback[VOICEID_MAX] = {
	{{0x002B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x33, 0x003C}, /*  0 */
	{{0x0031, 0x001D, 0x0024, 0x0037, 0xFFFF}, 0x34, 0xFFFF}, /*  1 */
	{{0x0031, 0x001D, 0x0024, 0x0037, 0x0039}, 0x34, 0xFFFF}, /*  2 */
	{{0x0031, 0x001D, 0x0024, 0x0037, 0x003A}, 0x34, 0xFFFF}, /*  3 */
	{{0x0031, 0x001D, 0x0024, 0x0037, 0x003B}, 0x34, 0xFFFF}, /*  4 */
	{{0x0031, 0x001D, 0x0024, 0x0037, 0x003C}, 0x34, 0xFFFF}, /*  5 */
	{{0x0031, 0x001E, 0x0024, 0x0037, 0xFFFF}, 0x35, 0xFFFF}, /*  6 */
	{{0x0031, 0x001F, 0x0024, 0x0037, 0xFFFF}, 0x36, 0xFFFF}, /*  7 */
	{{0x0031, 0x0020, 0x0024, 0x0037, 0xFFFF}, 0x37, 0xFFFF}, /*  8 */
	{{0x0031, 0x0021, 0x0024, 0x0037, 0xFFFF}, 0x38, 0xFFFF}, /*  9 */
	{{0x0031, 0x0022, 0x0037, 0xFFFF, 0xFFFF}, 0x39, 0xFFFF}, /* 10 */
	{{0x0031, 0x0023, 0x0024, 0x0037, 0xFFFF}, 0x3A, 0xFFFF}, /* 11 */
	{{0x0031, 0x0032, 0x0037, 0xFFFF, 0xFFFF}, 0x3B, 0xFFFF}, /* 12 */
	{{0x001D, 0x0024, 0x0035, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 13 */
	{{0x001E, 0x0024, 0x0035, 0xFFFF, 0xFFFF}, 0x3C, 0xFFFF}, /* 14 */
	{{0x001F, 0x0024, 0x0035, 0xFFFF, 0xFFFF}, 0x3D, 0xFFFF}, /* 15 */
	{{0x0020, 0x0024, 0x0035, 0xFFFF, 0xFFFF}, 0x3E, 0xFFFF}, /* 16 */
	{{0x0021, 0x0024, 0x0035, 0xFFFF, 0xFFFF}, 0x3F, 0xFFFF}, /* 17 */
	{{0x0022, 0x0035, 0xFFFF, 0xFFFF, 0xFFFF}, 0x40, 0xFFFF}, /* 18 */
	{{0x0023, 0x0024, 0x0035, 0xFFFF, 0xFFFF}, 0x41, 0xFFFF}, /* 19 */
	{{0x0032, 0x0035, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 20 */
	{{0x001D, 0x0025, 0x0035, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 21 */
	{{0x001E, 0x0025, 0x0035, 0xFFFF, 0xFFFF}, 0x42, 0xFFFF}, /* 22 */
	{{0x001F, 0x0025, 0x0035, 0xFFFF, 0xFFFF}, 0x43, 0xFFFF}, /* 23 */
	{{0x0020, 0x0025, 0x0035, 0xFFFF, 0xFFFF}, 0x44, 0xFFFF}, /* 24 */
	{{0x0021, 0x0025, 0x0035, 0xFFFF, 0xFFFF}, 0x45, 0xFFFF}, /* 25 */
	{{0x0022, 0x0035, 0xFFFF, 0xFFFF, 0xFFFF}, 0x46, 0xFFFF}, /* 26 */
	{{0x0023, 0x0025, 0x0035, 0xFFFF, 0xFFFF}, 0x47, 0xFFFF}, /* 27 */
	{{0x002C, 0x002E, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 28 */
	{{0x002C, 0x002D, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 29 */
	{{0x001E, 0x0024, 0x0036, 0xFFFF, 0xFFFF}, 0x48, 0xFFFF}, /* 30 */
	{{0x001F, 0x0024, 0x0036, 0xFFFF, 0xFFFF}, 0x49, 0xFFFF}, /* 31 */
	{{0x0020, 0x0024, 0x0036, 0xFFFF, 0xFFFF}, 0x4A, 0xFFFF}, /* 32 */
	{{0x0021, 0x0024, 0x0036, 0xFFFF, 0xFFFF}, 0x4B, 0xFFFF}, /* 33 */
	{{0x0022, 0x0036, 0xFFFF, 0xFFFF, 0xFFFF}, 0x4C, 0xFFFF}, /* 34 */
	{{0x0023, 0x0024, 0x0036, 0xFFFF, 0xFFFF}, 0x4D, 0xFFFF}, /* 35 */
	{{0x0034, 0x0038, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 36 */
	{{0x0031, 0x0044, 0xFFFF, 0xFFFF, 0xFFFF}, 0x4E, 0x0017}, /* 37 */
	{{0x002F, 0x0030, 0xFFFF, 0xFFFF, 0xFFFF}, 0x50, 0xFFFF}, /* 38 */
	{{0x0031, 0x0033, 0x0037, 0xFFFF, 0xFFFF}, 0x51, 0xFFFF}, /* 39 */
	{{0x003D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 40 */
	{{0x003E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 41 */
	{{0x0033, 0x003F, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 42 */
	{{0x0026, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0x002E}, /* 43 */
	{{0x0027, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0x002E}, /* 44 */
	{{0x0028, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0x002E}, /* 45 */
	{{0x0029, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0x002E}, /* 46 */
	{{0x002A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0x002E}, /* 47 */
	{{0x0040, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x5A, 0x0017}, /* 48 */
	{{0x001E, 0x0024, 0x003F, 0xFFFF, 0xFFFF}, 0x9A, 0xFFFF}, /* 49 */
	{{0x001F, 0x0024, 0x003F, 0xFFFF, 0xFFFF}, 0x9B, 0xFFFF}, /* 50 */
	{{0x0020, 0x0024, 0x003F, 0xFFFF, 0xFFFF}, 0x9C, 0xFFFF}, /* 51 */
	{{0x0021, 0x0024, 0x003F, 0xFFFF, 0xFFFF}, 0x9D, 0xFFFF}, /* 52 */
	{{0x0022, 0x0024, 0x003F, 0xFFFF, 0xFFFF}, 0x9E, 0xFFFF}, /* 53 */
	{{0x0023, 0x0024, 0x003F, 0xFFFF, 0xFFFF}, 0x9F, 0xFFFF}, /* 54 */
	{{0x001E, 0x0041, 0x0042, 0xFFFF, 0xFFFF}, 0xA2, 0xFFFF}, /* 55 */
	{{0x001F, 0x0041, 0x0042, 0xFFFF, 0xFFFF}, 0xA3, 0xFFFF}, /* 56 */
	{{0x0020, 0x0041, 0x0042, 0xFFFF, 0xFFFF}, 0xA4, 0xFFFF}, /* 57 */
	{{0x0021, 0x0041, 0x0042, 0xFFFF, 0xFFFF}, 0xA5, 0xFFFF}, /* 58 */
	{{0x0022, 0x0041, 0x0042, 0xFFFF, 0xFFFF}, 0xA6, 0xFFFF}, /* 59 */
	{{0x0023, 0x0041, 0x0042, 0xFFFF, 0xFFFF}, 0xA7, 0xFFFF}, /* 60 */
	{{0x0046, 0x0047, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 61 */
	{{0x001E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 62 */
	{{0x001F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 63 */
	{{0x0020, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 64 */
	{{0x0021, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 65 */
	{{0x0022, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 66 */
	{{0x0023, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 67 */
	{{0x001E, 0x0043, 0x0036, 0xFFFF, 0xFFFF}, 0x93, 0xFFFF}, /* 68 */
	{{0x001F, 0x0043, 0x0036, 0xFFFF, 0xFFFF}, 0x94, 0xFFFF}, /* 69 */
	{{0x0020, 0x0043, 0x0036, 0xFFFF, 0xFFFF}, 0x95, 0xFFFF}, /* 70 */
	{{0x0021, 0x0043, 0x0036, 0xFFFF, 0xFFFF}, 0x96, 0xFFFF}, /* 71 */
	{{0x0022, 0x0043, 0x0036, 0xFFFF, 0xFFFF}, 0x97, 0xFFFF}, /* 72 */
	{{0x0023, 0x0043, 0x0036, 0xFFFF, 0xFFFF}, 0x98, 0xFFFF}, /* 73 */

	{{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x01, 0xFFFF}, /* 74 */
	{{ SAMPLE_INTRO_DUNE,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF},                       /* 75 */
	{{ SAMPLE_INTRO_THE_BUILDING,
	   SAMPLE_INTRO_OF_A_DYNASTY,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x01, 0xFFFF},                       /* 76 */
	{{0x006F, 0x0069, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 77 */
	{{0x0072, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 78 */
	{{0x0065, 0x0073, 0x006A, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 79 */
	{{0x0074, 0x0056, 0x005D, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 80 */
	{{0x0076, 0x0053, 0x0054, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 81 */
	{{0x0068, 0x0071, 0x0059, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 82 */
	{{0x005C, 0x005E, 0x0061, 0x005B, 0xFFFF}, 0x02, 0xFFFF}, /* 83 */
	{{0x0062, 0x0060, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 84 */
	{{0x005A, 0x005F, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 85 */
	{{0x0075, 0x004F, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 86 */
	{{0x004E, 0x004D, 0x0055, 0x006D, 0xFFFF}, 0x01, 0xFFFF}, /* 87 */
	{{0x006B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 88 */
	{{0x0067, 0x006E, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 89 */
	{{0x0063, 0x0064, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 90 */
	{{0x0066, 0x0070, 0xFFFF, 0xFFFF, 0xFFFF}, 0x02, 0xFFFF}, /* 91 */
	{{0x0077, 0x0050, 0x0051, 0xFFFF, 0xFFFF}, 0x01, 0xFFFF}, /* 92 */
	{{0x006C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x01, 0xFFFF}  /* 93 */
};

/** Translated audio feedback of events and user commands. */
const enum SampleID g_translatedVoice[VOICEID_MAX][NUM_SPEECH_PARTS] = {
	{0x002B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /*  0 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  1 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  2 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  3 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  4 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  5 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  6 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  7 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  8 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /*  9 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /* 10 */
	{0x0031, 0x001D, 0xFFFF, 0xFFFF, 0xFFFF}, /* 11 */
	{0x0031, 0x0032, 0xFFFF, 0xFFFF, 0xFFFF}, /* 12 */
	{0x0024, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 13 */
	{0x0037, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 14 */
	{0x0037, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 15 */
	{0x0037, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 16 */
	{0x0037, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 17 */
	{0x0037, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 18 */
	{0x0037, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 19 */
	{0x0035, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 20 */
	{0x0025, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 21 */
	{0x0025, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 22 */
	{0x0025, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 23 */
	{0x0025, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 24 */
	{0x0025, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 25 */
	{0x0025, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 26 */
	{0x0025, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 27 */
	{0x002E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 28 */
	{0x002D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 29 */
	{0x0036, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 30 */
	{0x0036, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 31 */
	{0x0036, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 32 */
	{0x0036, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 33 */
	{0x0036, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 34 */
	{0x0036, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 35 */
	{0x0034, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 36 */
	{0x0031, 0x0044, 0xFFFF, 0xFFFF, 0xFFFF}, /* 37 */
	{0x002F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 38 */
	{0x0031, 0x0033, 0xFFFF, 0xFFFF, 0xFFFF}, /* 39 */
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 40 */
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 41 */
	{0x003F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 42 */
	{0x0026, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 43 */
	{0x0027, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 44 */
	{0x0028, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 45 */
	{0x0029, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 46 */
	{0x002A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 47 */
	{0x0040, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 48 */
	{0x0041, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 49 */
	{0x0041, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 50 */
	{0x0041, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 51 */
	{0x0041, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 52 */
	{0x0041, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 53 */
	{0x0041, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 54 */
	{0x0042, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 55 */
	{0x0042, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 56 */
	{0x0042, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 57 */
	{0x0042, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 58 */
	{0x0042, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 59 */
	{0x0042, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 60 */
	{0x0046, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 61 */
	{0x001E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 62 */
	{0x001F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 63 */
	{0x0020, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 64 */
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 65 */
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 66 */
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 67 */
	{0x0043, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 68 */
	{0x0043, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 69 */
	{0x0043, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 70 */
	{0x0043, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 71 */
	{0x0043, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 72 */
	{0x0043, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 73 */
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 74 */
	{0x0057, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 75 */
	{0x0052, 0x0058, 0xFFFF, 0xFFFF, 0xFFFF}, /* 76 */
	{0x006F, 0x0069, 0xFFFF, 0xFFFF, 0xFFFF}, /* 77 */
	{0x0072, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 78 */
	{0x0065, 0x0073, 0x006A, 0xFFFF, 0xFFFF}, /* 79 */
	{0x0074, 0x0056, 0x005D, 0xFFFF, 0xFFFF}, /* 80 */
	{0x0076, 0x0053, 0x0054, 0xFFFF, 0xFFFF}, /* 81 */
	{0x0068, 0x0071, 0x0059, 0xFFFF, 0xFFFF}, /* 82 */
	{0x005C, 0x005E, 0x0061, 0x005B, 0xFFFF}, /* 83 */
	{0x0062, 0x0060, 0xFFFF, 0xFFFF, 0xFFFF}, /* 84 */
	{0x005A, 0x005F, 0xFFFF, 0xFFFF, 0xFFFF}, /* 85 */
	{0x0075, 0x004F, 0xFFFF, 0xFFFF, 0xFFFF}, /* 86 */
	{0x004E, 0x004D, 0x0055, 0x006D, 0xFFFF}, /* 87 */
	{0x006B, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 88 */
	{0x0067, 0x006E, 0xFFFF, 0xFFFF, 0xFFFF}, /* 89 */
	{0x0063, 0x0064, 0xFFFF, 0xFFFF, 0xFFFF}, /* 90 */
	{0x0066, 0x0070, 0xFFFF, 0xFFFF, 0xFFFF}, /* 91 */
	{0x0077, 0x0050, 0x0051, 0xFFFF, 0xFFFF}, /* 92 */
	{0x006C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}  /* 93 */
};
