/** @file src/string.h String definitions. */

#ifndef STRING_H
#define STRING_H

#include "enumeration.h"

extern const char * const g_languageSuffixes[];
extern const char * const g_gameSubtitle[];

extern uint16 String_Decompress(char *source, char *dest);
extern const char *String_GenerateFilename(const char *name);
extern char *String_Get_ByIndex(uint16 stringID);
extern char *String_GetMentatString(enum HouseType houseID, int entry);
extern void String_TranslateSpecial(char *source, char *dest);
extern void String_ReloadMentatText(void);
extern void String_Init(void);
extern void String_Uninit(void);

#endif /* STRING_H */
