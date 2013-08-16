#ifndef TABLE_LOCALE_H
#define TABLE_LOCALE_H

#include "enum_language.h"

typedef struct {
	char name[4];
	char suffix[4];
} LanguageInfo;

extern const LanguageInfo g_table_languageInfo[LANGUAGE_MAX];

#endif
