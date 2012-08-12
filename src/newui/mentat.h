#ifndef NEWUI_MENTAT_H
#define NEWUI_MENTAT_H

#include "../house.h"

enum BriefingState {
	MENTAT_SHOW_CONTENTS,
	MENTAT_SHOW_TEXT,
	MENTAT_IDLE
};

enum BriefingEntry {
	MENTAT_BRIEFING_ORDERS  = 0,
	MENTAT_BRIEFING_WIN     = 1,
	MENTAT_BRIEFING_LOSE    = 2,
	MENTAT_BRIEFING_ADVICE  = 3
};

typedef struct MentatState {
	enum BriefingState state;

	char buf[1024];
	char *desc;
	char *text;
	int lines0;
	int lines;
} MentatState;

extern MentatState g_mentat_state;

extern void Mentat_DrawBackground(enum HouseType houseID);
extern void Mentat_Draw(enum HouseType houseID);

extern void MentatBriefing_InitText(enum HouseType houseID, int campaignID, enum BriefingEntry entry, MentatState *mentat);
extern void MentatBriefing_DrawText(MentatState *mentat);
extern void MentatBriefing_AdvanceText(MentatState *mentat);

extern bool MentatHelp_Tick(enum HouseType houseID, MentatState *mentat);

#endif
