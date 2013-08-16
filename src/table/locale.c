/** @file src/table/locale.c */

#include "locale.h"

const LanguageInfo g_table_languageInfo[LANGUAGE_MAX] = {
	{ /* LANGUAGE_ENGLISH */
		/* name                 */ "ENG",
		/* suffix               */ "ENG",
		/* noun_before_adj      */ false
	},
	{ /* LANGUAGE_FRENCH */
		/* name                 */ "FRE",
		/* suffix               */ "FRE",
		/* noun_before_adj      */ true
	},
	{ /* LANGUAGE_GERMAN */
		/* name                 */ "GER",
		/* suffix               */ "GER",
		/* noun_before_adj      */ false
	},
	{ /* LANGUAGE_ITALIAN */
		/* name                 */ "ITA",
		/* suffix               */ "FRE",
		/* noun_before_adj      */ true
	},
	{ /* LANGUAGE_SPANISH */
		/* name                 */ "SPA",
		/* suffix               */ "GER",
		/* noun_before_adj      */ true
	}
};
