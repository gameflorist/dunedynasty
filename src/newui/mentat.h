#ifndef NEWUI_MENTAT_H
#define NEWUI_MENTAT_H

#include <inttypes.h>
#include "enumeration.h"
#include "../file.h"
#include "../house.h"

enum BriefingState {
	MENTAT_SHOW_CONTENTS,
	MENTAT_PAUSE_DESCRIPTION,
	MENTAT_SHOW_DESCRIPTION,
	MENTAT_SHOW_TEXT,
	MENTAT_IDLE,
	MENTAT_SECURITY_INCORRECT
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
	int desc_lines;
	int64_t desc_timer;

	char *text;
	int lines0;
	int lines;

	int speaking_mode;
	int64_t speaking_timer;

	void *wsa;
	int wsa_frame;
	int64_t wsa_timer;

	char security_prompt[128];
	int security_question;
	int security_lives;
} MentatState;

struct Widget;

extern int movingEyesSprite;
extern int movingMouthSprite;
extern int otherSprite;
extern MentatState g_mentat_state;

extern void Mentat_LoadHelpSubjects(struct Widget *scrollbar, bool init, enum SearchDirectory dir, enum HouseType houseID, int campaignID, bool skip_advice);

extern enum MentatID Mentat_InitFromString(const char *str, enum HouseType houseID);

extern void Mentat_GetEyePositions(enum MentatID mentatID, int *left, int *top, int *right, int *bottom);
extern void Mentat_GetMouthPositions(enum MentatID mentatID, int *left, int *top, int *right, int *bottom);
extern void Mentat_DrawBackground(enum MentatID mentatID);
extern void Mentat_Draw(enum MentatID mentatID);

extern void MentatBriefing_SplitText(MentatState *mentat);
extern void MentatBriefing_InitText(enum HouseType houseID, int campaignID, enum BriefingEntry entry, MentatState *mentat);
extern void MentatBriefing_DrawText(const MentatState *mentat);
extern void MentatBriefing_AdvanceText(MentatState *mentat);

extern void MentatBriefing_InitWSA(enum HouseType houseID, int scenarioID, enum BriefingEntry entry, MentatState *mentat);
extern void MentatBriefing_DrawWSA(MentatState *mentat);

extern void MentatSecurity_Initialise(enum HouseType houseID, MentatState *mentat);
extern void MentatSecurity_PrepareQuestion(bool pick_new_question, MentatState *mentat);
extern void MentatSecurity_Draw(MentatState *mentat);
extern bool MentatSecurity_CorrectLoop(MentatState *mentat, int64_t blink_start);

extern void MentatHelp_Draw(enum MentatID mentatID, MentatState *mentat);
extern void MentatHelp_TickPauseDescription(MentatState *mentat);
extern void MentatHelp_TickShowDescription(MentatState *mentat);
extern bool MentatHelp_Tick(MentatState *mentat);

#endif
