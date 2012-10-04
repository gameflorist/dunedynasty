/* $Id$ */

/** @file src/table/sound.c Sound file tables. */

#include <stdio.h>

#include "sound.h"

#define D2TM_ADLIB_PREFIX   "d2tm_adlib"
#define D2TM_MT32_PREFIX    "d2tm_mt32"
#define D2TM_SC55_PREFIX    "d2tm_sc55"
#define DUNE2000_PREFIX     "dune2000"
#define FED2K_MT32_PREFIX   "fed2k_mt32"

#define MIDI_FILE_NOT_AVAILABLE     { false, NULL, 0 }
#define MUSIC_FILE_NOT_AVAILABLE    { false, 0.0f, NULL }

const char *g_music_set_prefix[NUM_MUSIC_SETS] = {
	"dune2_adlib",
	"dune2_c55",
	FED2K_MT32_PREFIX,
	D2TM_ADLIB_PREFIX,
	D2TM_MT32_PREFIX,
	D2TM_SC55_PREFIX,
	DUNE2000_PREFIX,
};

MusicInfo g_table_music[MUSICID_MAX] = {
	{	/* MUSIC_STOP */
		/* dune2_adlib   */ MIDI_FILE_NOT_AVAILABLE,
		/* dune2_c55     */ MIDI_FILE_NOT_AVAILABLE,
		/* fed2k_mt32    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_1: bonus track */
		/* dune2_adlib   */ { true, "dune1.ADL", 2 },
		/* dune2_c55     */ { true, "dune1.C55", 2 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_01_3" },
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_LOSE_ORDOS */
		/* dune2_adlib   */ { true, "dune1.ADL", 3 },
		/* dune2_c55     */ { true, "dune1.C55", 3 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_01_6" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/lose3" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/lose3" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/lose3" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_LOSE_HARKONNEN */
		/* dune2_adlib   */ { true, "dune1.ADL", 4 },
		/* dune2_c55     */ { true, "dune1.C55", 4 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_01_4" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/lose2" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/lose1" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/lose1" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_LOSE_ATREIDES */
		/* dune2_adlib   */ { true, "dune1.ADL", 5 },
		/* dune2_c55     */ { true, "dune1.C55", 5 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_01_5" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/lose1" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/lose2" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/lose2" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_WIN_ORDOS */
		/* dune2_adlib   */ { true, "dune17.ADL", 4 },
		/* dune2_c55     */ { true, "dune17.C55", 4 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_17_21" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/win2" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/win3" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/win3" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_WIN_HARKONNEN */
		/* dune2_adlib   */ { true, "dune8.ADL", 3 },
		/* dune2_c55     */ { true, "dune8.C55", 3 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_08_11" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/win3" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/win2" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/win2" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_WIN_ATREIDES */
		/* dune2_adlib   */ { true, "dune8.ADL", 2 },
		/* dune2_c55     */ { true, "dune8.C55", 2 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_08_10" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/win1" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/win1" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/win1" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_IDLE1 */
		/* dune2_adlib   */ { true, "dune1.ADL", 6 },
		/* dune2_c55     */ { true, "dune1.C55", 6 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_01_7" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace2" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace1" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace2" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/AMBUSH" },
	},

	{	/* MUSIC_IDLE2 */
		/* dune2_adlib   */ { true, "dune2.ADL", 6 },
		/* dune2_c55     */ { true, "dune2.C55", 6 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_02_8" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace5" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace2" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace5" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/SPICESCT" },
	},

	{	/* MUSIC_IDLE3 */
		/* dune2_adlib   */ { true, "dune3.ADL", 6 },
		/* dune2_c55     */ { true, "dune3.C55", 6 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_03_9" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace4" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace3" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace4" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/RISEHARK" },
	},

	{	/* MUSIC_IDLE4 */
		/* dune2_adlib   */ { true, "dune4.ADL", 6 },
		/* dune2_c55     */ { true, "dune4.C55", 6 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_04_10" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace1" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace4" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace1" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/ENTORDOS" },
	},

	{	/* MUSIC_IDLE5 */
		/* dune2_adlib   */ { true, "dune5.ADL", 6 },
		/* dune2_c55     */ { true, "dune5.C55", 6 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_05_11" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace9" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace5" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace9" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/UNDERCON" },
	},

	{	/* MUSIC_IDLE6 */
		/* dune2_adlib   */ { true, "dune6.ADL", 6 },
		/* dune2_c55     */ { true, "dune6.C55", 6 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_06_12" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace8" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace6" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace8" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/ATREGAIN" },
	},

	{	/* MUSIC_IDLE7 */
		/* dune2_adlib   */ { true, "dune9.ADL", 4 },
		/* dune2_c55     */ { true, "dune9.C55", 4 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_09_13" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace7" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace7" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace7" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/FREMEN" },
	},

	{	/* MUSIC_IDLE8 */
		/* dune2_adlib   */ { true, "dune9.ADL", 5 },
		/* dune2_c55     */ { true, "dune9.C55", 5 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_09_14" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace6" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace8" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace6" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/LANDSAND" },
	},

	{	/* MUSIC_IDLE9 */
		/* dune2_adlib   */ { true, "dune18.ADL", 6 },
		/* dune2_c55     */ { true, "dune18.C55", 6 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_18_24" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/peace3" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/peace9" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/peace3" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/WAITGAME" },
	},

	{	/* MUSIC_ATTACK1 */
		/* dune2_adlib   */ { true, "dune10.ADL", 7 },
		/* dune2_c55     */ { true, "dune10.C55", 7 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_10_17" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/attack5" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/attack1" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/attack4" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/ARAKATAK" },
	},

	{	/* MUSIC_ATTACK2 */
		/* dune2_adlib   */ { true, "dune11.ADL", 7 },
		/* dune2_c55     */ { true, "dune11.C55", 7 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_11_18" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/attack3" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/attack2" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/attack5" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_ATTACK3 */
		/* dune2_adlib   */ { true, "dune12.ADL", 7 },
		/* dune2_c55     */ { true, "dune12.C55", 7 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_12_19" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/attack6" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/attack3" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/attack2" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/HARK_BAT" },
	},

	{	/* MUSIC_ATTACK4 */
		/* dune2_adlib   */ { true, "dune13.ADL", 7 },
		/* dune2_c55     */ { true, "dune13.C55", 7 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_13_20" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/attack2" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/attack4" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/attack3" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_ATTACK5 */
		/* dune2_adlib   */ { true, "dune14.ADL", 7 },
		/* dune2_c55     */ { true, "dune14.C55", 7 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_14_21" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/attack4" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/attack5" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/attack1" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/FIGHTPWR" },
	},

	{	/* MUSIC_ATTACK6 */
		/* dune2_adlib   */ { true, "dune15.ADL", 7 },
		/* dune2_c55     */ { true, "dune15.C55", 7 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_15_22" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/attack1" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/attack6" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/attack6" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_23 */
		/* dune2_adlib   */ { true, "dune1.ADL", 8 },
		/* dune2_c55     */ { true, "dune1.C55", 8 },
		/* fed2k_mt32    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_BRIEFING_HARKONNEN */
		/* dune2_adlib   */ { true, "dune7.ADL", 2 },
		/* dune2_c55     */ { true, "dune7.C55", 2 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_07_09" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/mentath" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/mentath" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/mentath" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_BRIEFING_ATREIDES */
		/* dune2_adlib   */ { true, "dune7.ADL", 3 },
		/* dune2_c55     */ { true, "dune7.C55", 3 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_07_10" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/mentata" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/mentata" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/mentata" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_BRIEFING_ORDOS */
		/* dune2_adlib   */ { true, "dune7.ADL", 4 },
		/* dune2_c55     */ { true, "dune7.C55", 4 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_07_11" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/mentato" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/mentato" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/mentato" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_INTRO */
		/* dune2_adlib   */ { true, "dune0.ADL", 2 },
		/* dune2_c55     */ { true, "dune0.C55", 2 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_00_2" },
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/intro" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/intro" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_MAIN_MENU */
		/* dune2_adlib   */ { true, "dune7.ADL", 6 },
		/* dune2_c55     */ { true, "dune7.C55", 6 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_07_13" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/menu" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/menu" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/menu" },
		/* dune2000      */ { true, 1.0f, DUNE2000_PREFIX "/OPTIONS" },
	},

	{	/* MUSIC_STRATEGIC_MAP */
		/* dune2_adlib   */ { true, "dune16.ADL", 7 },
		/* dune2_c55     */ { true, "dune16.C55", 7 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_16_23" },
		/* d2tm_adlib    */ { true, 1.0f, D2TM_ADLIB_PREFIX "/nextconq" },
		/* d2tm_mt32     */ { true, 1.0f, D2TM_MT32_PREFIX "/nextconq" },
		/* d2tm_sc55     */ { true, 1.0f, D2TM_SC55_PREFIX "/nextconq" },
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_END_GAME_HARKONNEN */
		/* dune2_adlib   */ { true, "dune19.ADL", 4 },
		/* dune2_c55     */ { true, "dune19.C55", 4 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_19_23" },
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_END_GAME_ATREIDES */
		/* dune2_adlib   */ { true, "dune19.ADL", 2 },
		/* dune2_c55     */ { true, "dune19.C55", 2 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_19_21" },
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_END_GAME_ORDOS */
		/* dune2_adlib   */ { true, "dune19.ADL", 3 },
		/* dune2_c55     */ { true, "dune19.C55", 3 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_19_22" },
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_CREDITS */
		/* dune2_adlib   */ { true, "dune20.ADL", 2 },
		/* dune2_c55     */ { true, "dune20.C55", 2 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_20_22" },
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_CUTSCENE */
		/* dune2_adlib   */ { true, "dune16.ADL", 8 },
		/* dune2_c55     */ { true, "dune16.C55", 8 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_16_24" },
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_35 */
		/* dune2_adlib   */ { true, "dune0.ADL", 3 },
		/* dune2_c55     */ { true, "dune0.C55", 3 },
		/* fed2k_mt32    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_LOGOS */
		/* dune2_adlib   */ { true, "dune0.ADL", 4 },
		/* dune2_c55     */ { true, "dune0.C55", 4 },
		/* fed2k_mt32    */ { true, 1.0f, FED2K_MT32_PREFIX "/dune2_mt32_00_4" },
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},

	{	/* MUSIC_37 */
		/* dune2_adlib   */ { true, "dune0.ADL", 5 },
		/* dune2_c55     */ { true, "dune0.C55", 5 },
		/* fed2k_mt32    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_adlib    */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_mt32     */ MUSIC_FILE_NOT_AVAILABLE,
		/* d2tm_sc55     */ MUSIC_FILE_NOT_AVAILABLE,
		/* dune2000      */ MUSIC_FILE_NOT_AVAILABLE,
	},
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
