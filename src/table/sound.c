/** @file src/table/sound.c Sound file tables. */

#include <stdio.h>
#include "../os/common.h"

#include "sound.h"

#define D2TM_ADLIB_PREFIX   		"d2tm_adlib"
#define D2TM_SC55_PREFIX    		"d2tm_sc55"
#define DUNE2000_PREFIX     		"dune2000"
#define EMPEROR_ATREIDES_PREFIX 	"emperor_atreides"
#define EMPEROR_HARKONNEN_PREFIX 	"emperor_harkonnen"
#define EMPEROR_ORDOS_PREFIX 		"emperor_ordos"
#define DUNE2_SMD_PREFIX    		"dune2_smd"
#define SCDB_MIX_PREFIX  			"scdb_mix"
#define SHAIWA_MT32_PREFIX  		"fed2k_mt32"
#define RCBLANKE_SC55_PREFIX    	"rcblanke_sc55"
#define DUNE2_AMIGA_PREFIX    		"dune2_amiga"
#define DUNE2_PCSPEAKER_PREFIX    	"dune2_pcspeaker"
#define DUNE1992_ADLIB_PREFIX    	"dune1992_adlib"
#define DUNE1992_SCDB_PREFIX    	"dune1992_scdb"
#define DUNE1992_SPICEOPERA_PREFIX	"dune1992_spiceopera"
#define DUNE1984_OST_PREFIX    		"dune1984_ost"
#define DUNE2021_OST_PREFIX    		"dune2021_ost"
#define DUNE2021_SKETCHBOOK_PREFIX	"dune2021_sketchbook"
#define DUNE_PART_TWO_OST_PREFIX	"dune_part_two_ost"

#define ADD_MUSIC_LIST(TABLE,SONGNAME)  { 0, 0, 0, SONGNAME, lengthof(TABLE), TABLE }
#define ADD_MUSIC_FROM_DUNE2_ADLIB(FILENAME,TRACK)  			{ MUSIC_ENABLE, MUSICSET_DUNE2_ADLIB,   		NULL,   FILENAME, TRACK, 1.0f }
#define ADD_MUSIC_FROM_DUNE2_MIDI(FILENAME,TRACK)   			{ MUSIC_ENABLE, MUSICSET_DUNE2_MIDI,    		NULL,   FILENAME, TRACK, 1.0f }
#define ADD_MUSIC_FROM_FLUIDSYNTH(FILENAME,TRACK)   			{ MUSIC_ENABLE, MUSICSET_FLUIDSYNTH,    		NULL,   FILENAME, TRACK, 1.0f }
#define ADD_MUSIC_FROM_SCDB_MIX(FILENAME)        				{ MUSIC_WANT,   MUSICSET_SCDB_MIX,   			NULL,   "music/" SCDB_MIX_PREFIX "/" FILENAME, 0, 0.50f }
#define ADD_MUSIC_FROM_SHAIWA_MT32(FILENAME)        			{ MUSIC_WANT,   MUSICSET_SHAIWA_MT32,   		NULL,   "music/" SHAIWA_MT32_PREFIX "/" FILENAME, 0, 0.65f }
#define ADD_MUSIC_FROM_RCBLANKE_SC55(FILENAME)      			{ MUSIC_WANT,   MUSICSET_RCBLANKE_SC55, 		NULL,   "music/" RCBLANKE_SC55_PREFIX"/"FILENAME, 0, 0.80f }
#define ADD_MUSIC_FROM_D2TM_ADLIB(FILENAME)  					{ MUSIC_WANT,   MUSICSET_D2TM_ADLIB,    		NULL,   "music/" D2TM_ADLIB_PREFIX  "/" FILENAME, 0, 0.40f }
#define ADD_MUSIC_FROM_D2TM_SC55(FILENAME,VOLUME)   			{ MUSIC_WANT,   MUSICSET_D2TM_SC55,     		NULL,   "music/" D2TM_SC55_PREFIX   "/" FILENAME, 0, VOLUME }
#define ADD_MUSIC_FROM_DUNE2_PCSPEAKER(FILENAME)      			{ MUSIC_WANT,   MUSICSET_DUNE2_PCSPEAKER, 		NULL,   "music/" DUNE2_PCSPEAKER_PREFIX   "/"   FILENAME, 0, 0.50f }
#define ADD_MUSIC_FROM_DUNE2_SMD(FILENAME,SONGNAME) 			{ MUSIC_WANT,   MUSICSET_DUNE2_SMD,     		SONGNAME,"music/"DUNE2_SMD_PREFIX   "/" FILENAME, 0, 0.50f }
#define ADD_MUSIC_FROM_DUNE2_AMIGA(FILENAME,SONGNAME)			{ MUSIC_WANT,   MUSICSET_DUNE2_AMIGA, 			SONGNAME,"music/"DUNE2_AMIGA_PREFIX   "/"   FILENAME, 0, 0.50f }
#define ADD_MUSIC_FROM_DUNE2000(FILENAME,SONGNAME)  			{ MUSIC_WANT,   MUSICSET_DUNE2000,      		SONGNAME,"music/"DUNE2000_PREFIX    "/" FILENAME, 0, 1.00f }
#define ADD_MUSIC_FROM_EMPEROR_ATREIDES(FILENAME,SONGNAME)		{ MUSIC_WANT,   MUSICSET_EMPEROR_ATREIDES,		SONGNAME,"music/emperor/" FILENAME, 0, 0.70f }
#define ADD_MUSIC_FROM_EMPEROR_HARKONNEN(FILENAME,SONGNAME)		{ MUSIC_WANT,   MUSICSET_EMPEROR_HARKONNEN,		SONGNAME,"music/emperor/" FILENAME, 0, 0.70f }
#define ADD_MUSIC_FROM_EMPEROR_ORDOS(FILENAME,SONGNAME)			{ MUSIC_WANT,   MUSICSET_EMPEROR_ORDOS,			SONGNAME,"music/emperor/" FILENAME, 0, 0.70f }
#define ADD_MUSIC_FROM_DUNE1992_ADLIB(FILENAME,SONGNAME)		{ MUSIC_WANT,   MUSICSET_DUNE1992_ADLIB,      	SONGNAME,"music/"DUNE1992_ADLIB_PREFIX    "/" FILENAME, 0, 1.00f }
#define ADD_MUSIC_FROM_DUNE1992_SCDB(FILENAME,SONGNAME)			{ MUSIC_WANT,   MUSICSET_DUNE1992_SCDB,      	SONGNAME,"music/"DUNE1992_SCDB_PREFIX    "/" FILENAME, 0, 1.00f }
#define ADD_MUSIC_FROM_DUNE1992_SPICEOPERA(FILENAME,SONGNAME)  	{ MUSIC_WANT,   MUSICSET_DUNE1992_SPICEOPERA,	SONGNAME,"music/"DUNE1992_SPICEOPERA_PREFIX    "/" FILENAME, 0, 0.80f }
#define ADD_MUSIC_FROM_DUNE1984_OST(FILENAME,SONGNAME)  		{ MUSIC_WANT,   MUSICSET_DUNE1984_OST,      	SONGNAME,"music/"DUNE1984_OST_PREFIX    "/" FILENAME, 0, 1.00f }
#define ADD_MUSIC_FROM_DUNE2021_OST(FILENAME,SONGNAME)  		{ MUSIC_WANT,   MUSICSET_DUNE2021_OST,      	SONGNAME,"music/"DUNE2021_OST_PREFIX    "/" FILENAME, 0, 1.00f }
#define ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK(FILENAME,SONGNAME)	{ MUSIC_WANT,   MUSICSET_DUNE2021_SKETCHBOOK,	SONGNAME,"music/"DUNE2021_SKETCHBOOK_PREFIX    "/" FILENAME, 0, 1.00f }
#define ADD_MUSIC_FROM_DUNE_PART_TWO_OST(FILENAME,SONGNAME)		{ MUSIC_WANT,   MUSICSET_DUNE_PART_TWO_OST,		SONGNAME,"music/"DUNE_PART_TWO_OST_PREFIX    "/" FILENAME, 0, 1.00f }

MusicSetInfo g_table_music_set[NUM_MUSIC_SETS] = {
	{ true, "dune2_adlib",  			"AdLib" },
	{ true, "dune2_midi",   			"MIDI" },
	{ true, "fluidsynth",   			"FluidSynth" },
	{ true, SCDB_MIX_PREFIX,     		"SCDB Mix" },
	{ true, SHAIWA_MT32_PREFIX,     	"ShaiWa MT-32" },
	{ true, RCBLANKE_SC55_PREFIX,   	"RCBlanke SC-55" },
	{ true, D2TM_ADLIB_PREFIX,  		"D2TM AdLib" },
	{ true, D2TM_SC55_PREFIX,   		"D2TM SC-55" },
	{ true, DUNE2_PCSPEAKER_PREFIX,   	"PC Speaker" },
	{ true, DUNE2_SMD_PREFIX,   		"Sega Mega Drive" },
	{ true, DUNE2_AMIGA_PREFIX,   		"Amiga" },
	{ true, DUNE2000_PREFIX,    		"Dune 2000" },
	{ true, EMPEROR_ATREIDES_PREFIX,    "Emperor (Atreides)" },
	{ true, EMPEROR_HARKONNEN_PREFIX,   "Emperor (Harkonnen)" },
	{ true, EMPEROR_ORDOS_PREFIX,   	"Emperor (Ordos)" },
	{ true, DUNE1992_ADLIB_PREFIX,    	"Dune Adlib" },
	{ true, DUNE1992_SCDB_PREFIX,    	"Dune SCDB Mix" },
	{ true, DUNE1992_SPICEOPERA_PREFIX,	"Dune Spice Opera" },
	{ true, DUNE1984_OST_PREFIX,    	"Dune 1984 OST" },
	{ true, DUNE2021_OST_PREFIX,    	"Dune: Part One OST" },
	{ true, DUNE2021_SKETCHBOOK_PREFIX,	"Dune: Part One Sketchbook" },
	{ true, DUNE_PART_TWO_OST_PREFIX,	"Dune: Part Two OST" },
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
	ADD_MUSIC_FROM_SCDB_MIX  	("01 - Westwood Studios Logo"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_00_4"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("westwood_logo"),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER("Westwood Associates Logo [dune0-4]"),
	ADD_MUSIC_FROM_DUNE2_AMIGA	("01_Westwood Studios", "Westwood Studios Logo"),
};

static MusicInfo s_table_music_intro[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune0.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune0.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune0.C55", 2),
	ADD_MUSIC_FROM_SCDB_MIX  	("02 - Introduction"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_00_2"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("intro"),
	ADD_MUSIC_FROM_D2TM_SC55    ("intro", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER("Intro [dune0-2]"),

	/* Sega Mega Drive intro music doesn't match. */
	{ 0, MUSICSET_DUNE2_SMD,    NULL, "music/" DUNE2_SMD_PREFIX "/01_intro", 0, 0.50f },
};

static MusicInfo s_table_music_cutscene[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune16.ADL", 8),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune16.C55", 8),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune16.C55", 8),
	ADD_MUSIC_FROM_SCDB_MIX  	("16 - Emperor Theme"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_16_24"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("emperors_theme"),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER("The Long Sleep (Emperor's Theme) [dune16-8]"),
};

static MusicInfo s_table_music_credits[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  		("dune20.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   		("dune20.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   		("dune20.C55", 2),
	ADD_MUSIC_FROM_SCDB_MIX  			("25 - Credits Theme"),
	ADD_MUSIC_FROM_SHAIWA_MT32  		("dune2_mt32_20_22"),
	ADD_MUSIC_FROM_RCBLANKE_SC55		("credits"),
	ADD_MUSIC_FROM_DUNE2_SMD    		("20_credits", NULL),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("22 - Cryogenia", "Cryogenia"),
	ADD_MUSIC_FROM_DUNE1984_OST			("17 - Take My Hand", "Take My Hand"),
};

static MusicInfo s_table_music_main_menu[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  		("dune7.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   		("dune7.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   		("dune7.C55", 6),
	ADD_MUSIC_FROM_SCDB_MIX  			("03 - Main Menu"),
	ADD_MUSIC_FROM_SHAIWA_MT32  		("dune2_mt32_07_13"),
	ADD_MUSIC_FROM_RCBLANKE_SC55		("title"),
	ADD_MUSIC_FROM_D2TM_ADLIB   		("menu"),
	ADD_MUSIC_FROM_D2TM_SC55    		("menu", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Hope Fades (Main Menu) [dune7-6]"),
	ADD_MUSIC_FROM_DUNE2_SMD    		("12_chosendestiny", "Chosen Destiny"),
	ADD_MUSIC_FROM_DUNE2_AMIGA			("05_Intro Part 1", "Intro Part 1"),
	ADD_MUSIC_FROM_DUNE2000     		("OPTIONS", "Options"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES		("AT_Menu1", "Atreides Menu"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN	("HK_Menu1", "Harkonnen Menu"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS		("OR_Menu1", "Ordos Menu"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB     	("Spice Opera", "Spice Opera"),
	ADD_MUSIC_FROM_DUNE1992_SCDB     	("06 - Worm Suit (Spice Opera)", "Worm Suit (Spice Opera)"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("11 - Spice Opera", "Spice Opera"),
	ADD_MUSIC_FROM_DUNE1984_OST     	("02 - Main Title", "Main Title"),
	ADD_MUSIC_FROM_DUNE2021_OST     	("01 - Dream of Arrakis", "Dream of Arrakis"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK	("05 - Paul's Dream", "Paul's Dream"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST	("01 - Beginnings Are Such Delicate Times", "Beginnings Are Such Delicate Times"),

	/* Dune 2000 battle summary as alternative menu music. */
	{ 0, MUSICSET_DUNE2000,     "Score", "music/" DUNE2000_PREFIX "/SCORE", 0, 1.0f },
};

static MusicInfo s_table_music_strategic_map[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune16.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune16.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune16.C55", 7),
	ADD_MUSIC_FROM_SCDB_MIX  	("05 - Map Screen"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_16_23"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("conquest_map"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("nextconq"),
	ADD_MUSIC_FROM_D2TM_SC55    ("nextconq", 1.0f),
	ADD_MUSIC_FROM_DUNE2_SMD    ("11_evasiveaction", "Evasive Action"),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Destructive Minds (Strategic Map) [dune16-7]"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES   	("AT_Map1", "Atreides Map"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN   	("HK_Map1", "Harkonnen Map"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("OR_Map1", "Ordos Map"),
};

static MusicInfo s_table_music_briefing_harkonnen[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune7.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune7.C55", 2),	
	ADD_MUSIC_FROM_SCDB_MIX  	("04.3 - Mentat (Harkonnen)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_07_09"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("mentat_harkonnen"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentath"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentath", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Arid Sands (Harkonnen Briefing) [dune7-2]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("04_radnorsscheme", "Radnor's Scheme"),
};

static MusicInfo s_table_music_briefing_atreides[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune7.C55", 3),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune7.C55", 3),
	ADD_MUSIC_FROM_SCDB_MIX  	("04.1 - Mentat (Atreides) - The Council"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_07_10"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("mentat_atreides"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentata"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentata", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("The Council (Atreides Briefing) [dune7-3]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("02_cyrilscouncil", "Cyril's Council"),
};

static MusicInfo s_table_music_briefing_ordos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune7.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune7.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune7.C55", 4),
	ADD_MUSIC_FROM_SCDB_MIX  	("04.2 - Mentat (Ordos) - Disturbed Thoughts"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_07_11"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("mentat_ordos"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("mentato"),
	ADD_MUSIC_FROM_D2TM_SC55    ("mentato", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Disturbed Thoughts (Ordos Briefing) [dune7-4]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("03_ammonsadvice", "Ammon's Advice"),
};

static MusicInfo s_table_music_win_harkonnen[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune8.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune8.C55", 3),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune8.C55", 3),
	ADD_MUSIC_FROM_SCDB_MIX  	("15.3 - Win Theme (Harkonnen)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_08_11"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("victory_harkonnen"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win2", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Victory 2 (Harkonnen Victory) [dune8-3]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("15_harkonnenrules", "Harkonnen Rules"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS("HK_Score1", "Harkonnen Score"),
};

static MusicInfo s_table_music_win_atreides[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune8.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune8.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune8.C55", 2),
	ADD_MUSIC_FROM_SCDB_MIX  	("15.1 - Win Theme (Atreides)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_08_10"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("victory_atreides"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win1", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Victory (Atreides Victory) [dune8-2]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("13_conquest", "Conquest"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES   	("AT_Score1a", "Atreides Score"),
};

static MusicInfo s_table_music_win_ordos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune17.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune17.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune17.C55", 4),
	ADD_MUSIC_FROM_SCDB_MIX  	("15.2 - Win Theme (Ordos)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_17_21"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("victory_ordos"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("win2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("win3", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Abuse (Ordos Victory) [dune17-4]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("14_slitherin", "Slitherin"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("OR_Score1", "Ordos Score"),
};

static MusicInfo s_table_music_lose_harkonnen[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune1.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune1.C55", 4),
	ADD_MUSIC_FROM_SCDB_MIX  	("17.3 - Defeat (Harkonnen)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_01_4"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("defeat_harkonnen"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose1", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Harkonnen Defeat [dune1-4]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("18_harkonnendirge", "Harkonnen Dirge"),
};

static MusicInfo s_table_music_lose_atreides[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 5),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune1.C55", 5),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune1.C55", 5),
	ADD_MUSIC_FROM_SCDB_MIX  	("17.1 - Defeat (Atreides)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_01_5"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("defeat_atreides"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose2", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Atreides Defeat [dune1-5]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("16_atreidesdirge", "Atreides Dirge"),
};

static MusicInfo s_table_music_lose_ordos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune1.C55", 3),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune1.C55", 3),
	ADD_MUSIC_FROM_SCDB_MIX  	("17.2 - Defeat (Ordos)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_01_6"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("defeat_ordos"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("lose3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("lose3", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Ordos Defeat [dune1-3]"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("17_ordosdirge", "Ordos Dirge"),
};

static MusicInfo s_table_music_end_game_harkonnen[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune19.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune19.C55", 4),
	ADD_MUSIC_FROM_SCDB_MIX  	("24.3 - Ending (Harkonnen)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_19_23"),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Evil Harkonnen (Harkonnen Ending) [dune19-4]"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ending_harkonnen"),
	ADD_MUSIC_FROM_DUNE2_AMIGA	("10_Ending", "Ending"),
};

static MusicInfo s_table_music_end_game_atreides[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 2),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune19.C55", 2),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune19.C55", 2),
	ADD_MUSIC_FROM_SCDB_MIX  	("24.1 - Ending (Atreides)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_19_21"),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Noble Atreides (Atreides Ending) [dune19-2]"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ending_atreides"),
	ADD_MUSIC_FROM_DUNE2_AMIGA	("10_Ending", "Ending"),
};

static MusicInfo s_table_music_end_game_ordos[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune19.ADL", 3),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune19.C55", 3),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune19.C55", 3),
	ADD_MUSIC_FROM_SCDB_MIX  	("24.2 - Ending (Ordos)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_19_22"),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Insidious Ordos (Ordos Ending) [dune19-3]"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ending_ordos"),
	ADD_MUSIC_FROM_DUNE2_AMIGA	("10_Ending", "Ending"),
};

static MusicInfo s_table_music_idle1[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune1.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune1.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune1.C55", 6),	
	ADD_MUSIC_FROM_SCDB_MIX  	("06 - The Building of a Dynasty"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_01_7"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient02"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace2", 1.0f),
};

static MusicInfo s_table_music_idle2[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune2.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune2.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune2.C55", 6),
	ADD_MUSIC_FROM_SCDB_MIX  	("07 - Dark Technology"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_02_8"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient03"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace5"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace5", 1.0f),
};

static MusicInfo s_table_music_idle3[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune3.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune3.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune3.C55", 6),
	ADD_MUSIC_FROM_SCDB_MIX  	("08 - Rulers of Arrakis"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_03_9"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient04"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace4"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace4", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("RISEHARK", "Rise of Harkonnen"),
};

static MusicInfo s_table_music_idle4[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune4.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune4.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune4.C55", 6),
	ADD_MUSIC_FROM_SCDB_MIX  	("09 - Desert of Doom"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_04_10"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient05"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace1", 1.0f),
};

static MusicInfo s_table_music_idle5[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune5.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune5.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune5.C55", 6),
	ADD_MUSIC_FROM_SCDB_MIX  	("10 - Faithful Warriors"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_05_11"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient06"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace9"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace9", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("UNDERCON", "Under Construction"),
};

static MusicInfo s_table_music_idle6[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune6.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune6.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune6.C55", 6),
	ADD_MUSIC_FROM_SCDB_MIX  	("11 - Spice Melange"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_06_12"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient07"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace8"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace8", 1.0f),
	ADD_MUSIC_FROM_DUNE2000     ("ATREGAIN", "The Atreides Gain"),
};

static MusicInfo s_table_music_idle7[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune9.ADL", 4),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune9.C55", 4),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune9.C55", 4),
	ADD_MUSIC_FROM_SCDB_MIX  	("12 - The Prophecy (part 1)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_09_13"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient08"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace7"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace7", 1.0f),
};

static MusicInfo s_table_music_idle8[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune9.ADL", 5),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune9.C55", 5),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune9.C55", 5),
	ADD_MUSIC_FROM_SCDB_MIX  	("13 - The Prophecy (part 2)"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_09_14"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient09"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace6"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace6", 1.0f),
};

static MusicInfo s_table_music_idle9[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune18.ADL", 6),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune18.C55", 6),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune18.C55", 6),
	ADD_MUSIC_FROM_SCDB_MIX  	("14 - For Those Fallen"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_18_24"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("ambient10"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("peace3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("peace3", 1.0f),
};

static MusicInfo s_table_music_idle_other[] = {
	ADD_MUSIC_FROM_DUNE2_SMD    ("05_thelegotune", "The LEGO Tune"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("06_turbulence", "Turbulence"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("07_spicetrip", "Spice Trip"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("08_commandpost", "Command Post"),
	ADD_MUSIC_FROM_DUNE2_SMD    ("09_trenching", "Trenching"),
	ADD_MUSIC_FROM_DUNE2_AMIGA	("08_In-Game BGM 1", "Background Music 1"),
	ADD_MUSIC_FROM_DUNE2_AMIGA	("09_In-Game BGM 2", "Background Music 2"),
	ADD_MUSIC_FROM_DUNE2000     ("AMBUSH",   "The Ambush"),
	ADD_MUSIC_FROM_DUNE2000     ("ENTORDOS", "Enter the Ordos"),
	ADD_MUSIC_FROM_DUNE2000     ("FREMEN",   "The Fremen"),
	ADD_MUSIC_FROM_DUNE2000     ("LANDSAND", "Land of Sand"),
	ADD_MUSIC_FROM_DUNE2000     ("PLOTTING", "Plotting"),
	ADD_MUSIC_FROM_DUNE2000     ("ROBOTIX",  "Robotix"),
	ADD_MUSIC_FROM_DUNE2000     ("SOLDAPPR", "The Soldiers Approach"),
	ADD_MUSIC_FROM_DUNE2000     ("SPICESCT", "Spice Scouting"),
	ADD_MUSIC_FROM_DUNE2000     ("WAITGAME", "The Waiting Game"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT02)Sand_Excursion", "Sand Excursion"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT03)Assembling_the_Troops", "Assembling the Troops"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT04)The_Spice_Must_Flow", "The Spice Must Flow"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT05)The_Overseer", "The Overseer"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT06)Battle_of_the_Atreides", "Battle of the Atreides"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT08)Infiltrating_HK", "Infiltrating HK"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT09)Unsuspected_Attack", "Unsuspected Attack"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT10)Fremen_Alliance", "Fremen Alliance"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT11)Assassination_Attempt", "Assassination Attempt"),
	ADD_MUSIC_FROM_EMPEROR_ATREIDES     	("(AT12)Fight_in_the_Dunes", "Fight in the Dunes"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK02)Surrounded", "Surrounded"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK03)Tribute_to_Evil", "Tribute to Evil"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK04)Harkonnen_Force", "Harkonnen Force"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK06)Unstoppable", "Unstoppable"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK07)Dark_Alliance", "Dark Alliance"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK08)War_for_the_Spice", "War for the Spice"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK09)Defenders_of_Arrakis", "Defenders of Arrakis"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK10)House_Harkonnen", "House Harkonnen"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK11)Invincible", "Invincible"),
	ADD_MUSIC_FROM_EMPEROR_HARKONNEN     	("(HK12)Victory_is_Inevitable", "Victory is Inevitable"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR02)The_Strategist", "The Strategist"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR03)House_Ordos", "House Ordos"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR04)Ghola", "Ghola"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR05)Executronic", "Executronic"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR06)Deception", "Deception"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR07)Sabotage", "Sabotage"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR08)Dream_of_the_Executrix", "Dream of the Executrix"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR10)Ordos_Control", "Ordos Control"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR11)The_Specimen", "The Specimen"),
	ADD_MUSIC_FROM_EMPEROR_ORDOS     	("(OR12)Infiltrators", "Infiltrators"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB	("Chani's Eyes", "Chani's Eyes"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB	("Dune Variation", "Dune Variation"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB	("Free Men", "Free Men"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB	("Sietch Tuek", "Sietch Tuek"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB	("Sign of the Worm", "Sign of the Worm"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB	("Too", "Too"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB	("Wake Up", "Wake Up"),
	ADD_MUSIC_FROM_DUNE1992_ADLIB	("Water", "Water"),
	ADD_MUSIC_FROM_DUNE1992_SCDB	("02 - Morning (Chani's Eyes)", "Morning (Chani's Eyes)"),
	ADD_MUSIC_FROM_DUNE1992_SCDB	("01 - Arrakis (Dune Theme)", "Arrakis (Dune Theme)"),
	ADD_MUSIC_FROM_DUNE1992_SCDB	("03 - Bagdad (Free Men)", "Bagdad (Free Men)"),
	ADD_MUSIC_FROM_DUNE1992_SCDB	("07 - Sietch", "Sietch"),
	ADD_MUSIC_FROM_DUNE1992_SCDB	("05 - Worm Intro (Sign of the Worm)", "Worm Intro (Sign of the Worm)"),
	ADD_MUSIC_FROM_DUNE1992_SCDB	("08 - War Song (Too)", "War Song (Too)"),
	ADD_MUSIC_FROM_DUNE1992_SCDB	("04 - Sequence (Wake Up)", "Sequence (Wake Up)"),
	ADD_MUSIC_FROM_DUNE1992_SCDB	("09 - Water (Water)", "Water"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("01 - Dune Theme", "Dune Theme"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("02 - Emotion Control", "Emotion Control"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("03 - Ecolove", "Ecolove"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("04 - Water Of Life", "Water"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("05 - Revelation", "Revelation"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("06 - Free Men", "Free Men"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("07 - Wake Up", "Wake Up"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("08 - Too", "Chani's Eyes"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("09 - Chani's Eyes", "Sign of the Worm"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("10 - Sign Of The Worm", "Too"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("12 - Dune Variation", "Dune Variation"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("13 - Arrakis (PC)", "Arrakis"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("14 - Bagdad (PC)", "Bagdad"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("15 - Morning (PC)", "Morning"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("16 - Sequence (PC)", "Sequence"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("17 - Sietch (PC)", "Sietch"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("18 - War Song (PC)", "War Song"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("19 - Water (PC)", "Water"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("20 - Worm Intro (PC)", "Worm Intro"),
	ADD_MUSIC_FROM_DUNE1992_SPICEOPERA	("21 - Dune Theme (overclocked)", "Dune Theme (overclocked)"),
	ADD_MUSIC_FROM_DUNE1984_OST ("03 - Robot Fight", "Robot Fight"),
	ADD_MUSIC_FROM_DUNE1984_OST ("04 - Leto's Theme", "Leto's Theme"),
	ADD_MUSIC_FROM_DUNE1984_OST ("05 - The Box", "The Box"),
	ADD_MUSIC_FROM_DUNE1984_OST ("07 - Trip to Arrakis", "Trip to Arrakis"),
	ADD_MUSIC_FROM_DUNE1984_OST ("08 - First Attack", "First Attack"),
	ADD_MUSIC_FROM_DUNE1984_OST ("09 - Prophecy Theme", "Prophecy Theme"),
	ADD_MUSIC_FROM_DUNE1984_OST ("10 - Dune (Desert Theme)", "Dune (Desert Theme)"),
	ADD_MUSIC_FROM_DUNE1984_OST ("11 - Paul Meets Chani", "Paul Meets Chani"),
	ADD_MUSIC_FROM_DUNE1984_OST ("13 - Paul Takes The Water Of Life", "Paul Takes The Water Of Life"),
	ADD_MUSIC_FROM_DUNE1984_OST ("14 - Big Battle", "Big Battle"),
	ADD_MUSIC_FROM_DUNE1984_OST ("15 - Paul Kills Feyd", "Paul Kills Feyd"),
	ADD_MUSIC_FROM_DUNE1984_OST ("16 - Final Dream", "Final Dream"),
	ADD_MUSIC_FROM_DUNE2021_OST ("01 - Dream of Arrakis", "Dream of Arrakis"),
	ADD_MUSIC_FROM_DUNE2021_OST ("02 - Herald of the Change", "Herald of the Change"),
	ADD_MUSIC_FROM_DUNE2021_OST ("03 - Bene Gesserit", "Bene Gesserit"),
	ADD_MUSIC_FROM_DUNE2021_OST ("04 - Gom Jabbar", "Gom Jabbar"),
	ADD_MUSIC_FROM_DUNE2021_OST ("05 - The One", "The One"),
	ADD_MUSIC_FROM_DUNE2021_OST ("06 - Leaving Caladan", "Leaving Caladan"),
	ADD_MUSIC_FROM_DUNE2021_OST ("07 - Arrakeen", "Arrakeen"),
	ADD_MUSIC_FROM_DUNE2021_OST ("08 - Ripples in the Sand", "Ripples in the Sand"),
	ADD_MUSIC_FROM_DUNE2021_OST ("09 - Visions of Chani", "Visions of Chani"),
	ADD_MUSIC_FROM_DUNE2021_OST ("10 - Night on Arrakis", "Night on Arrakis"),
	ADD_MUSIC_FROM_DUNE2021_OST ("11 - Armada", "Armada"),
	ADD_MUSIC_FROM_DUNE2021_OST ("12 - Burning Palms", "Burning Palms"),
	ADD_MUSIC_FROM_DUNE2021_OST ("13 - Stranded", "Stranded"),
	ADD_MUSIC_FROM_DUNE2021_OST ("14 - Blood for Blood", "Blood for Blood"),
	ADD_MUSIC_FROM_DUNE2021_OST ("15 - The Fall", "The Fall"),
	ADD_MUSIC_FROM_DUNE2021_OST ("16 - Holy War", "Holy War"),
	ADD_MUSIC_FROM_DUNE2021_OST ("17 - Sanctuary", "Sanctuary"),
	ADD_MUSIC_FROM_DUNE2021_OST ("18 - Premonition", "Premonition"),
	ADD_MUSIC_FROM_DUNE2021_OST ("19 - Ornithopter", "Ornithopter"),
	ADD_MUSIC_FROM_DUNE2021_OST ("20 - Sandstorm", "Sandstorm"),
	ADD_MUSIC_FROM_DUNE2021_OST ("21 - Stillsuits", "Stillsuits"),
	ADD_MUSIC_FROM_DUNE2021_OST ("22 - My Road Leads into the Desert", "My Road Leads into the Desert"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("01 - Song of the Sisters", "Song of the Sisters"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("02 - I See You in My Dreams", "I See You in My Dreams"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("03 - House Atreides", "House Atreides"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("04 - The Shortening of the Way", "The Shortening of the Way"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("05 - Paul's Dream", "Paul's Dream"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("06 - Moon over Caladan", "Moon over Caladan"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("07 - Shai-hulud", "Shai-hulud"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("08 - Mind-killer", "Mind-killer"),
	ADD_MUSIC_FROM_DUNE2021_SKETCHBOOK ("09 - Grains of Sand", "Grains of Sand"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("02 - Eclipse", "Eclipse"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("03 - The Sietch", "The Sietch"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("04 - Water of Life", "Water of Life"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("05 - A Time of Quiet Between the Storms", "A Time of Quiet Between the Storms"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("06 - Harvester Attack", "Harvester Attack"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("07 - Worm Ride", "Worm Ride"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("08 - Ornithopter Attack", "Ornithopter Attack"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("09 - Each Man Is a Little War", "Each Man Is a Little War"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("10 - Harkonnen Arena", "Harkonnen Arena"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("11 - Spice", "Spice"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("12 - Seduction", "Seduction"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("13 - Never Lose Me", "Never Lose Me"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("14 - Travel South", "Travel South"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("15 - Paul Drinks", "Paul Drinks"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("16 - Resurrection", "Resurrection"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("17 - Arrival", "Arrival"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("18 - Southern Messiah", "Southern Messiah"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("19 - The Emperor", "The Emperor"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("20 - Worm Army", "Worm Army"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("21 - Gurney Battle", "Gurney Battle"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("22 - You Fought Well", "You Fought Well"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("23 - Kiss the Ring", "Kiss the Ring"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("24 - Only I Will Remain", "Only I Will Remain"),
	ADD_MUSIC_FROM_DUNE_PART_TWO_OST ("25 - Lisan al Gaib", "Lisan al Gaib"),
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
	ADD_MUSIC_FROM_SCDB_MIX  	("18 - Into the Heat"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_10_17"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack01"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack5"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack4", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Into the Heat (Attack Music 1) [dune10-7]"),
	ADD_MUSIC_FROM_DUNE2000     ("ARAKATAK", "Attack on Arrakis"),
};

static MusicInfo s_table_music_attack2[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune11.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune11.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune11.C55", 7),
	ADD_MUSIC_FROM_SCDB_MIX  	("19 - Epic War"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_11_18"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack02"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack3"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack5", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Epic War (Attack Music 2) [dune11-7]"),
};

static MusicInfo s_table_music_attack3[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune12.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune12.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune12.C55", 7),
	ADD_MUSIC_FROM_SCDB_MIX  	("20 - Humans Fall"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_12_19"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack03"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack6"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack2", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Humans Fall (Attack Music 3) [dune12-7]"),
	ADD_MUSIC_FROM_DUNE2000     ("HARK_BAT", "Harkonnen Battle"),
};

static MusicInfo s_table_music_attack4[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune13.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune13.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune13.C55", 7),
	ADD_MUSIC_FROM_SCDB_MIX  	("21 - Adrenaline Rush"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_13_20"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack04"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack2"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack3", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Adrenaline Rush (Attack Music 4) [dune13-7]"),
};

static MusicInfo s_table_music_attack5[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune14.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune14.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune14.C55", 7),
	ADD_MUSIC_FROM_SCDB_MIX  	("22 - Only the Strongest Survives"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_14_21"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack05"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack4"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack1", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Only the Strongest Survives (Attack Music 5) [dune14-7]"),
	ADD_MUSIC_FROM_DUNE2000     ("FIGHTPWR", "Fight for Power"),
};

static MusicInfo s_table_music_attack6[] = {
	ADD_MUSIC_FROM_DUNE2_ADLIB  ("dune15.ADL", 7),
	ADD_MUSIC_FROM_DUNE2_MIDI   ("dune15.C55", 7),
	ADD_MUSIC_FROM_FLUIDSYNTH   ("dune15.C55", 7),
	ADD_MUSIC_FROM_SCDB_MIX  	("23 - Marching Towards the End"),
	ADD_MUSIC_FROM_SHAIWA_MT32  ("dune2_mt32_15_22"),
	ADD_MUSIC_FROM_RCBLANKE_SC55("attack06"),
	ADD_MUSIC_FROM_D2TM_ADLIB   ("attack1"),
	ADD_MUSIC_FROM_D2TM_SC55    ("attack6", 1.0f),
	ADD_MUSIC_FROM_DUNE2_PCSPEAKER		("Marching Towards the End (Attack Music 6) [dune15-7]"),
};

// static MusicInfo s_table_music_attack_other[] = {}; // unused

MusicList g_table_music[MUSICID_MAX] = {
	ADD_MUSIC_LIST(s_table_music_stop,  NULL),
	ADD_MUSIC_LIST(s_table_music_logos, "Title Screen"),
	ADD_MUSIC_LIST(s_table_music_intro, "Introduction"),
	ADD_MUSIC_LIST(s_table_music_main_menu, "Main Menu: Hope Fades"),
	ADD_MUSIC_LIST(s_table_music_strategic_map, "Strategic Map: Destructive Minds"),
	ADD_MUSIC_LIST(s_table_music_cutscene,  "Cutscene: The Long Sleep"),
	ADD_MUSIC_LIST(s_table_music_credits,   "Credits"),
	ADD_MUSIC_LIST(s_table_music_briefing_harkonnen,"Briefing Harkonnen: Arid Sands"),
	ADD_MUSIC_LIST(s_table_music_briefing_atreides, "Briefing Atreides: The Council"),
	ADD_MUSIC_LIST(s_table_music_briefing_ordos,    "Briefing Ordos"),
	ADD_MUSIC_LIST(s_table_music_win_harkonnen, "Victory Harkonnen"),
	ADD_MUSIC_LIST(s_table_music_win_atreides,  "Victory Atreides"),
	ADD_MUSIC_LIST(s_table_music_win_ordos,     "Victory Ordos"),
	ADD_MUSIC_LIST(s_table_music_lose_harkonnen,"Defeat Harkonnen"),
	ADD_MUSIC_LIST(s_table_music_lose_atreides, "Defeat Atreides"),
	ADD_MUSIC_LIST(s_table_music_lose_ordos,    "Defeat Ordos"),
	ADD_MUSIC_LIST(s_table_music_end_game_harkonnen,"Ending Harkonnen: Evil Harkonnen"),
	ADD_MUSIC_LIST(s_table_music_end_game_atreides, "Ending Atreides: Noble Atriedes"),
	ADD_MUSIC_LIST(s_table_music_end_game_ordos,    "Ending Ordos: Insidious Ordos"),
	ADD_MUSIC_LIST(s_table_music_idle1, "Idle 1: The Building of a Dynasty"),
	ADD_MUSIC_LIST(s_table_music_idle2, "Idle 2: Dark Technology"),
	ADD_MUSIC_LIST(s_table_music_idle3, "Idle 3: Rulers of Arrakis"),
	ADD_MUSIC_LIST(s_table_music_idle4, "Idle 4: Desert of Doom"),
	ADD_MUSIC_LIST(s_table_music_idle5, "Idle 5: Faithful Warriors"),
	ADD_MUSIC_LIST(s_table_music_idle6, "Idle 6: Spice Melange"),
	ADD_MUSIC_LIST(s_table_music_idle7, "Idle 7: The Prophecy, Part I"),
	ADD_MUSIC_LIST(s_table_music_idle8, "Idle 8: The Prophecy, Part II"),
	ADD_MUSIC_LIST(s_table_music_idle9, "Idle 9: For Those Fallen"),
	ADD_MUSIC_LIST(s_table_music_idle_other, "Idle (Others)"),
	ADD_MUSIC_LIST(s_table_music_bonus, "Bonus: Choose Your House"),
	ADD_MUSIC_LIST(s_table_music_attack1, "Attack 1: Into the Heat"),
	ADD_MUSIC_LIST(s_table_music_attack2, "Attack 2: Epic War"),
	ADD_MUSIC_LIST(s_table_music_attack3, "Attack 3: Humans Fall"),
	ADD_MUSIC_LIST(s_table_music_attack4, "Attack 4: Adrenaline Rush"),
	ADD_MUSIC_LIST(s_table_music_attack5, "Attack 5: Only the Strongest Survives"),
	ADD_MUSIC_LIST(s_table_music_attack6, "Attack 6: Marching Towards the End"),
	// ADD_MUSIC_LIST(s_table_music_attack_other, "Attack (Others)"),
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
