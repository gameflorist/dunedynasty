/** @file src/table/sound.c Sound file tables. */

#include <stdio.h>
#include "../os/common.h"

#include "sound.h"

#define D2TM_ADLIB_PREFIX   "d2tm_adlib"
#define D2TM_MT32_PREFIX    "d2tm_mt32"
#define D2TM_SC55_PREFIX    "d2tm_sc55"
#define DUNE2000_PREFIX     "dune2000"
#define DUNE2_SMD_PREFIX    "dune2_smd"
#define SHAIWA_MT32_PREFIX  "fed2k_mt32"
#define RCBLANKE_SC55_PREFIX    "rcblanke_sc55"

#define ADD_MUSIC_LIST(TABLE,SONGNAME)  { 0, 0, 0, SONGNAME, lengthof(TABLE), TABLE }
#define ADD_MUSIC_FROM_DUNE2_ADLIB(FILENAME,TRACK)  { MUSIC_ENABLE, MUSICSET_DUNE2_ADLIB,   NULL,   FILENAME, TRACK, 1.0f }
#define ADD_MUSIC_FROM_DUNE2_MIDI(FILENAME,TRACK)   { MUSIC_ENABLE, MUSICSET_DUNE2_MIDI,    NULL,   FILENAME, TRACK, 1.0f }
#define ADD_MUSIC_FROM_FLUIDSYNTH(FILENAME,TRACK)   { MUSIC_ENABLE, MUSICSET_FLUIDSYNTH,    NULL,   FILENAME, TRACK, 1.0f }
#define ADD_MUSIC_FROM_SHAIWA_MT32(FILENAME)        { MUSIC_WANT,   MUSICSET_SHAIWA_MT32,   NULL,   "music/" SHAIWA_MT32_PREFIX "/" FILENAME, 0, 0.65f }
#define ADD_MUSIC_FROM_RCBLANKE_SC55(FILENAME)      { MUSIC_WANT,   MUSICSET_RCBLANKE_SC55, NULL,   "music/" RCBLANKE_SC55_PREFIX"/"FILENAME, 0, 0.80f }
#define ADD_MUSIC_FROM_D2TM_ADLIB(FILENAME,VOLUME)  { MUSIC_WANT,   MUSICSET_D2TM_ADLIB,    NULL,   "music/" D2TM_ADLIB_PREFIX  "/" FILENAME, 0, VOLUME }
#define ADD_MUSIC_FROM_D2TM_MT32(FILENAME)          { MUSIC_WANT,   MUSICSET_D2TM_MT32,     NULL,   "music/" D2TM_MT32_PREFIX   "/" FILENAME, 0, 0.65f }
#define ADD_MUSIC_FROM_D2TM_SC55(FILENAME,VOLUME)   { MUSIC_WANT,   MUSICSET_D2TM_SC55,     NULL,   "music/" D2TM_SC55_PREFIX   "/" FILENAME, 0, VOLUME }
#define ADD_MUSIC_FROM_DUNE2_SMD(FILENAME,SONGNAME) { MUSIC_WANT,   MUSICSET_DUNE2_SMD,     SONGNAME,"music/"DUNE2_SMD_PREFIX   "/" FILENAME, 0, 0.50f }
#define ADD_MUSIC_FROM_DUNE2000(FILENAME,SONGNAME)  { MUSIC_WANT,   MUSICSET_DUNE2000,      SONGNAME,"music/"DUNE2000_PREFIX    "/" FILENAME, 0, 1.00f }

MusicSetInfo g_table_music_set[NUM_MUSIC_SETS] = {
	{ true, "dune2_adlib",  "AdLib" },
	{ true, "dune2_midi",   "MIDI" },
	{ true, "fluidsynth",   "FluidSynth" },
	{ true, SHAIWA_MT32_PREFIX,     "ShaiWa MT-32" },
	{ true, RCBLANKE_SC55_PREFIX,   "RCBlanke SC-55" },
	{ true, D2TM_ADLIB_PREFIX,  "D2TM AdLib" },
	{ true, D2TM_MT32_PREFIX,   "D2TM MT-32" },
	{ true, D2TM_SC55_PREFIX,   "D2TM SC-55" },
	{ true, DUNE2_SMD_PREFIX,   "Sega Mega Drive" },
	{ true, DUNE2000_PREFIX,    "Dune 2000" },
};

static MusicInfo s_table_music_stop[] = {
	/* 0: MUSIC_STOP */
	{ 0, MUSICSET_DUNE2_ADLIB, NULL, NULL, 0, 0.0f },

	/* These are kept for historical reasons, even though are silent. */

	/* 1: MUSIC_23 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 8),

	/* 2: MUSIC_35 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 3),

	/* 3: MUSIC_37 */
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 5),
};

static MusicInfo s_table_music_logos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune0.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune0.C55", 4),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_00_4"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("westwood_logo"),
};

static MusicInfo s_table_music_intro[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune0.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune0.C55", 2),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_00_2"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("intro"),
	ADD_MUSIC_FROM_D2TM_MT32    ("intro"),
	ADD_MUSIC_FROM_D2TM_SC55    ("intro", 1.0f),

	/* Sega Mega Drive intro music doesn't match. */
	{ 0, MUSICSET_DUNE2_SMD,    NULL, "music/" DUNE2_SMD_PREFIX "/01_intro", 0, 0.50f },
};

static MusicInfo s_table_music_cutscene[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune16.ADL", 8),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune16.C55", 8),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune16.C55", 8),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_16_24"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("emperors_theme"),
};

static MusicInfo s_table_music_credits[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune20.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune20.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune20.C55", 2),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_20_22"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("credits"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("20_credits", NULL),
};

static MusicInfo s_table_music_main_menu[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune7.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune7.C55", 6),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_07_13"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("title"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("menu", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("menu"),
	ADD_MUSIC_FROM_D2TM_SC55    ("menu", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("12_chosendestiny", "Chosen Destiny"),
	ADD_MUSIC_FROM_DUNE2000     ("OPTIONS", "Options"),

	/* Dune 2000 battle summary as alternative menu music. */
	{ 0, MUSICSET_DUNE2000,     "Score", "music/" DUNE2000_PREFIX "/SCORE", 0, 1.0f },
};

static MusicInfo s_table_music_strategic_map[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune16.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune16.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune16.C55", 7),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_16_23"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("conquest_map"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("nextconq", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("nextconq"),
	ADD_MUSIC_FROM_D2TM_SC55    ("nextconq", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("11_evasiveaction", "Evasive Action"),
};

static MusicInfo s_table_music_briefing_harkonnen[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune7.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune7.C55", 2),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_07_09"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("mentat_harkonnen"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentath", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("mentath"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentath", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("04_radnorsscheme", "Radnor's Scheme"),
};

static MusicInfo s_table_music_briefing_atreides[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune7.C55", 3),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune7.C55", 3),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_07_10"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("mentat_atreides"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentata", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("mentata"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentata", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("02_cyrilscouncil", "Cyril's Council"),
};

static MusicInfo s_table_music_briefing_ordos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune7.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune7.C55", 4),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_07_11"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("mentat_ordos"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentato", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("mentato"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentato", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("03_ammonsadvice", "Ammon's Advice"),
};

static MusicInfo s_table_music_win_harkonnen[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune8.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune8.C55", 3),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune8.C55", 3),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_08_11"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("victory_harkonnen"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win3", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("win2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win2", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("15_harkonnenrules", "Harkonnen Rules"),
};

static MusicInfo s_table_music_win_atreides[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune8.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune8.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune8.C55", 2),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_08_10"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("victory_atreides"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win1", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("win1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win1", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("13_conquest", "Conquest"),
};

static MusicInfo s_table_music_win_ordos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune17.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune17.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune17.C55", 4),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_17_21"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("victory_ordos"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win2", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("win3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win3", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("14_slitherin", "Slitherin"),
};

static MusicInfo s_table_music_lose_harkonnen[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune1.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune1.C55", 4),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_01_4"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("defeat_harkonnen"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose2", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("lose1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose1", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("18_harkonnendirge", "Harkonnen Dirge"),
};

static MusicInfo s_table_music_lose_atreides[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 5),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune1.C55", 5),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune1.C55", 5),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_01_5"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("defeat_atreides"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose1", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("lose2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose2", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("16_atreidesdirge", "Atreides Dirge"),
};

static MusicInfo s_table_music_lose_ordos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune1.C55", 3),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune1.C55", 3),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_01_6"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("defeat_ordos"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose3", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("lose3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose3", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("17_ordosdirge", "Ordos Dirge"),
};

static MusicInfo s_table_music_end_game_harkonnen[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune19.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune19.C55", 4),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_19_23"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ending_harkonnen"),
};

static MusicInfo s_table_music_end_game_atreides[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune19.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune19.C55", 2),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_19_21"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ending_atreides"),
};

static MusicInfo s_table_music_end_game_ordos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune19.C55", 3),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune19.C55", 3),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_19_22"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ending_ordos"),
};

static MusicInfo s_table_music_idle1[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune1.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune1.C55", 6),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_01_7"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient02"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace2", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace2", 1.0f),
};

static MusicInfo s_table_music_idle2[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune2.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune2.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune2.C55", 6),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_02_8"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient03"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace5", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace5", 1.0f),
};

static MusicInfo s_table_music_idle3[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune3.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune3.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune3.C55", 6),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_03_9"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient04"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace4", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace4", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("RISEHARK", "Rise of Harkonnen"),
};

static MusicInfo s_table_music_idle4[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune4.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune4.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune4.C55", 6),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_04_10"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient05"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace1", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace4"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace1", 1.0f),
};

static MusicInfo s_table_music_idle5[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune5.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune5.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune5.C55", 6),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_05_11"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient06"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace9", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace5"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace9", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("UNDERCON", "Under Construction"),
};

static MusicInfo s_table_music_idle6[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune6.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune6.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune6.C55", 6),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_06_12"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient07"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace8", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace6"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace8", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("ATREGAIN", "The Atreides Gain"),
};

static MusicInfo s_table_music_idle7[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune9.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune9.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune9.C55", 4),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_09_13"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient08"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace7", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace7"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace7", 1.0f),
};

static MusicInfo s_table_music_idle8[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune9.ADL", 5),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune9.C55", 5),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune9.C55", 5),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_09_14"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient09"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace6", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace8"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace6", 1.0f),
};

static MusicInfo s_table_music_idle9[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune18.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune18.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune18.C55", 6),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_18_24"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient10"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace3", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("peace9"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace3", 1.0f),
};

static MusicInfo s_table_music_idle_other[] = {
	ADD_MUSIC_FROM_DUNE2_SMD    ("05_thelegotune", "The LEGO Tune"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("06_turbulence", "Turbulence"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("07_spicetrip", "Spice Trip"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("08_commandpost", "Command Post"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("09_trenching", "Trenching"),
	ADD_MUSIC_FROM_DUNE2000     ("AMBUSH",   "The Ambush"),
	ADD_MUSIC_FROM_DUNE2000     ("ENTORDOS", "Enter the Ordos"),
	ADD_MUSIC_FROM_DUNE2000     ("FREMEN",   "The Fremen"),
	ADD_MUSIC_FROM_DUNE2000     ("LANDSAND", "Land of Sand"),
	ADD_MUSIC_FROM_DUNE2000     ("PLOTTING", "Plotting"),
	ADD_MUSIC_FROM_DUNE2000     ("ROBOTIX",  "Robotix"),
	ADD_MUSIC_FROM_DUNE2000     ("SOLDAPPR", "The Soldiers Approach"),
	ADD_MUSIC_FROM_DUNE2000     ("SPICESCT", "Spice Scouting"),
	ADD_MUSIC_FROM_DUNE2000     ("WAITGAME", "The Waiting Game"),
};

static MusicInfo s_table_music_bonus[] = { /* Disabled by default. */
	{ MUSIC_FOUND, MUSICSET_DUNE2_ADLIB, NULL, "dune1.ADL", 2, 0.0f },
	{ MUSIC_FOUND, MUSICSET_DUNE2_MIDI,  NULL, "dune1.C55", 2, 0.0f },
	{ MUSIC_FOUND, MUSICSET_FLUIDSYNTH,  NULL, "dune1.C55", 2, 0.0f },
	{ 0, MUSICSET_SHAIWA_MT32,  NULL, "music/" SHAIWA_MT32_PREFIX "/dune2_mt32_01_3", 0, 0.65f },
	{ 0, MUSICSET_RCBLANKE_SC55,NULL, "music/" RCBLANKE_SC55_PREFIX "/ambient01", 0, 0.80f },
	{ 0, MUSICSET_DUNE2_SMD,    "Starport", "music/" DUNE2_SMD_PREFIX  "/10_starport", 0, 0.50f },
};

static MusicInfo s_table_music_attack1[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune10.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune10.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune10.C55", 7),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_10_17"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack01"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack5", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack4", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("ARAKATAK", "Attack on Arrakis"),
};

static MusicInfo s_table_music_attack2[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune11.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune11.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune11.C55", 7),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_11_18"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack02"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack3", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack5", 1.0f),
};

static MusicInfo s_table_music_attack3[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune12.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune12.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune12.C55", 7),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_12_19"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack03"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack6", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack2", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("HARK_BAT", "Harkonnen Battle"),
};

static MusicInfo s_table_music_attack4[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune13.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune13.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune13.C55", 7),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_13_20"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack04"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack2", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack4"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack3", 1.0f),
};

static MusicInfo s_table_music_attack5[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune14.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune14.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune14.C55", 7),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_14_21"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack05"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack4", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack5"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack1", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("FIGHTPWR", "Fight for Power"),
};

static MusicInfo s_table_music_attack6[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune15.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune15.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune15.C55", 7),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_15_22"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack06"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack1", 1.0f),
	ADD_MUSIC_FROM_D2TM_MT32    ("attack6"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack6", 1.0f),
};

MusicList g_table_music[MUSICID_MAX] = {
	ADD_MUSIC_LIST(s_table_music_stop,  NULL),
	ADD_MUSIC_LIST(s_table_music_logos, "Title Screen"),
	ADD_MUSIC_LIST(s_table_music_intro, "Introduction"),
	ADD_MUSIC_LIST(s_table_music_main_menu, "Hope Fades"),
	ADD_MUSIC_LIST(s_table_music_strategic_map, "Destructive Minds"),
	ADD_MUSIC_LIST(s_table_music_cutscene,  "The Long Sleep"),
	ADD_MUSIC_LIST(s_table_music_credits,   "Credits"),
	ADD_MUSIC_LIST(s_table_music_briefing_harkonnen,"Arid Sands"),
	ADD_MUSIC_LIST(s_table_music_briefing_atreides, "The Council"),
	ADD_MUSIC_LIST(s_table_music_briefing_ordos,    "Ordos Briefing"),
	ADD_MUSIC_LIST(s_table_music_win_harkonnen, "Victory 2 (Harkonnen)"),
	ADD_MUSIC_LIST(s_table_music_win_atreides,  "Victory (Atreides)"),
	ADD_MUSIC_LIST(s_table_music_win_ordos,     "Abuse (Ordos)"),
	ADD_MUSIC_LIST(s_table_music_lose_harkonnen,"Death (Harkonnen)"),
	ADD_MUSIC_LIST(s_table_music_lose_atreides, "Death (Atreides)"),
	ADD_MUSIC_LIST(s_table_music_lose_ordos,    "Death (Ordos)"),
	ADD_MUSIC_LIST(s_table_music_end_game_harkonnen,"Evil Harkonnens"),
	ADD_MUSIC_LIST(s_table_music_end_game_atreides, "Noble Atreides"),
	ADD_MUSIC_LIST(s_table_music_end_game_ordos,    "Insidious Ordos"),
	ADD_MUSIC_LIST(s_table_music_idle1, "Idle 1: The Building of a Dynasty"),
	ADD_MUSIC_LIST(s_table_music_idle2, "Idle 2: Dark Technology"),
	ADD_MUSIC_LIST(s_table_music_idle3, "Idle 3: Rulers of Arrakis"),
	ADD_MUSIC_LIST(s_table_music_idle4, "Idle 4: Desert of Doom"),
	ADD_MUSIC_LIST(s_table_music_idle5, "Idle 5: Faithful Warriors"),
	ADD_MUSIC_LIST(s_table_music_idle6, "Idle 6: Spice Melange"),
	ADD_MUSIC_LIST(s_table_music_idle7, "Idle 7: The Prophecy, Part I"),
	ADD_MUSIC_LIST(s_table_music_idle8, "Idle 8: The Prophecy, Part II"),
	ADD_MUSIC_LIST(s_table_music_idle9, "Idle 9: For Those Fallen"),
	ADD_MUSIC_LIST(s_table_music_idle_other, "Idle"),
	ADD_MUSIC_LIST(s_table_music_bonus, "Bonus: Choose Your House"),
	ADD_MUSIC_LIST(s_table_music_attack1, "Into the Heat"),
	ADD_MUSIC_LIST(s_table_music_attack2, "Epic War"),
	ADD_MUSIC_LIST(s_table_music_attack3, "Humans Fall"),
	ADD_MUSIC_LIST(s_table_music_attack4, "Adrenaline Rush"),
	ADD_MUSIC_LIST(s_table_music_attack5, "Only the Strongest Survives"),
	ADD_MUSIC_LIST(s_table_music_attack6, "Marching Towards the End"),
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
	{"%cMERC.VOC",     20}, /*  35 */
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

	/* The countdown ticking sound 0x002E repeats five times already. */
	{{0x0026, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 43 */
	{{0x0027, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 44 */
	{{0x0028, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 45 */
	{{0x0029, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x00, 0xFFFF}, /* 46 */
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
	{{ SAMPLE_VOICE_FRAGMENT_FREMEN,
	   SAMPLE_VOICE_FRAGMENT_APPROACHING,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID}, 0x00, 0xFFFF},                        /* 65 */
	{{ SAMPLE_VOICE_FRAGMENT_SARDAUKAR,
	   SAMPLE_VOICE_FRAGMENT_DEPLOYED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF},                       /* 66 */
	{{ SAMPLE_VOICE_FRAGMENT_MERCENARY,
	   SAMPLE_VOICE_FRAGMENT_HAS_ARRIVED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF},                       /* 67 */

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
	{{0x006C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0x01, 0xFFFF}, /* 93 */

	{{ SAMPLE_VOICE_FRAGMENT_ENEMY,
	   SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	   SAMPLE_VOICE_FRAGMENT_CAPTURED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF },                      /* 94 */
	{{ SAMPLE_VOICE_FRAGMENT_HARKONNEN,
	   SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	   SAMPLE_VOICE_FRAGMENT_CAPTURED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF },                      /* 95 */
	{{ SAMPLE_VOICE_FRAGMENT_ATREIDES,
	   SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	   SAMPLE_VOICE_FRAGMENT_CAPTURED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF },                      /* 96 */
	{{ SAMPLE_VOICE_FRAGMENT_ORDOS,
	   SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	   SAMPLE_VOICE_FRAGMENT_CAPTURED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF },                      /* 97 */
	{{ SAMPLE_VOICE_FRAGMENT_FREMEN,
	   SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	   SAMPLE_VOICE_FRAGMENT_CAPTURED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF },                      /* 98 */
	{{ SAMPLE_VOICE_FRAGMENT_SARDAUKAR,
	   SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	   SAMPLE_VOICE_FRAGMENT_CAPTURED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF },                      /* 99 */
	{{ SAMPLE_VOICE_FRAGMENT_MERCENARY,
	   SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	   SAMPLE_VOICE_FRAGMENT_CAPTURED,
	   SAMPLE_INVALID,
	   SAMPLE_INVALID }, 0x00, 0xFFFF },                      /* 100 */
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
	{0x006C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, /* 93 */

	{ SAMPLE_VOICE_FRAGMENT_ENEMY,
	  SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	  SAMPLE_VOICE_FRAGMENT_CAPTURED,
	  SAMPLE_INVALID,
	  SAMPLE_INVALID },                       /* 94 */
	{ SAMPLE_VOICE_FRAGMENT_HARKONNEN,
	  SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	  SAMPLE_VOICE_FRAGMENT_CAPTURED,
	  SAMPLE_INVALID,
	  SAMPLE_INVALID },                       /* 95 */
	{ SAMPLE_VOICE_FRAGMENT_ATREIDES,
	  SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	  SAMPLE_VOICE_FRAGMENT_CAPTURED,
	  SAMPLE_INVALID,
	  SAMPLE_INVALID },                       /* 96 */
	{ SAMPLE_VOICE_FRAGMENT_ORDOS,
	  SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	  SAMPLE_VOICE_FRAGMENT_CAPTURED,
	  SAMPLE_INVALID,
	  SAMPLE_INVALID },                       /* 97 */
	{ SAMPLE_VOICE_FRAGMENT_FREMEN,
	  SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	  SAMPLE_VOICE_FRAGMENT_CAPTURED,
	  SAMPLE_INVALID,
	  SAMPLE_INVALID },                       /* 98 */
	{ SAMPLE_VOICE_FRAGMENT_SARDAUKAR,
	  SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	  SAMPLE_VOICE_FRAGMENT_CAPTURED,
	  SAMPLE_INVALID,
	  SAMPLE_INVALID },                       /* 99 */
	{ SAMPLE_VOICE_FRAGMENT_MERCENARY,
	  SAMPLE_VOICE_FRAGMENT_STRUCTURE,
	  SAMPLE_VOICE_FRAGMENT_CAPTURED,
	  SAMPLE_INVALID,
	  SAMPLE_INVALID },                       /* 100 */
};
