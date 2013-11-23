#ifndef CHATBOX_H
#define CHATBOX_H

#include <stdbool.h>

extern void ChatBox_ClearHistory(void);
extern void ChatBox_AddEntry(const char *name, const char *msg);
extern void ChatBox_Draw(const char *buf, bool draw_cursor);

#endif
