#ifndef TABLE_LOCALE_H
#define TABLE_LOCALE_H

#include <stdbool.h>
#include "enum_language.h"

typedef struct {
	char name[4];
	char suffix[4];
	char sample_prefix;
	bool noun_before_adj;
} LanguageInfo;

extern const LanguageInfo g_table_languageInfo[LANGUAGE_MAX];

#endif
