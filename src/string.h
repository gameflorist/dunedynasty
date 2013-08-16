/** @file src/string.h String definitions. */

#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include "enumeration.h"

extern const char * const g_gameSubtitle[3];

extern uint16 String_Decompress(const char *source, char *dest);
extern const char *String_GenerateFilename(const char *name);
extern char *String_Get_ByIndex(uint16 stringID);
extern const char *String_GetMentatString(enum HouseType houseID, int stringID);
extern void String_TranslateSpecial(const char *source, char *dest);
extern void String_ReloadCampaignStrings(void);
extern void String_Init(void);
extern void String_Uninit(void);
extern void String_Trim(char *string);
extern void String_GetBool(const char *str, bool *value);

#endif /* STRING_H */
