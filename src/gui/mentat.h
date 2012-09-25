/* $Id$ */

/** @file src/gui/mentat.h Mentat gui definitions. */

#ifndef GUI_MENTAT_H
#define GUI_MENTAT_H

#include <inttypes.h>
#include <stdbool.h>

extern bool g_disableOtherMovement;
extern bool g_interrogation;

struct Widget;

extern void GUI_Mentat_LoadHelpSubjects(bool init);
extern void GUI_Mentat_Draw(bool force);
extern void GUI_Mentat_HelpListLoop(int key);
extern void GUI_Mentat_Display(const char *wsaFilename, uint8 houseID);
extern void GUI_Mentat_Animation(uint16 speakingMode);
extern void GUI_Mentat_Create_HelpScreen_Widgets(void);
extern uint16 GUI_Mentat_Loop(const char *wsaFilename, char *pictureDetails, char *text, bool arg0C, struct Widget *w);
extern uint16 GUI_Mentat_SplitText(char *str, uint16 maxWidth);

#endif /* GUI_MENTAT_H */
