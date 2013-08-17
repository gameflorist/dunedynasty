/** @file src/table/locale.c */

#include "locale.h"

const LanguageInfo g_table_languageInfo[LANGUAGE_MAX] = {
	{ /* LANGUAGE_ENGLISH */
		/* name                 */ "ENG",
		/* suffix               */ "ENG",
		/* sample_prefix        */ 0,
		/* noun_before_adj      */ false
	},
	{ /* LANGUAGE_FRENCH */
		/* name                 */ "FRE",
		/* suffix               */ "FRE",
		/* sample_prefix        */ 'F',
		/* noun_before_adj      */ true
	},
	{ /* LANGUAGE_GERMAN */
		/* name                 */ "GER",
		/* suffix               */ "GER",
		/* sample_prefix        */ 'G',
		/* noun_before_adj      */ false
	},
	{ /* LANGUAGE_ITALIAN */
		/* name                 */ "ITA",
		/* suffix               */ "FRE",
		/* sample_prefix        */ 0,
		/* noun_before_adj      */ true
	},
	{ /* LANGUAGE_SPANISH */
		/* name                 */ "SPA",
		/* suffix               */ "GER",
		/* sample_prefix        */ 0,
		/* noun_before_adj      */ true
	}
};
