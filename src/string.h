/** @file src/string.h String definitions. */

#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include "enumeration.h"

/**
 * Types of Language available in the game.
 */
typedef enum Language {
	LANGUAGE_ENGLISH     = 0,
	LANGUAGE_FRENCH      = 1,
	LANGUAGE_GERMAN      = 2,
	LANGUAGE_ITALIAN     = 3,
	LANGUAGE_SPANISH     = 4,

	LANGUAGE_MAX         = 5,
	LANGUAGE_INVALID     = 0xFF
} Language;

extern const char * const g_languageSuffixes[LANGUAGE_MAX];
extern const char * const g_gameSubtitle[3];

extern uint16 String_Decompress(const char *source, char *dest);
extern const char *String_GenerateFilename(const char *name);
extern char *String_Get_ByIndex(uint16 stringID);
extern char *String_GetMentatString(enum HouseType houseID, int entry);
extern void String_TranslateSpecial(char *source, char *dest);
extern void String_ReloadCampaignStrings(void);
extern void String_Init(void);
extern void String_Uninit(void);
extern void String_Trim(char *string);
extern void String_GetBool(const char *str, bool *value);

#endif /* STRING_H */
