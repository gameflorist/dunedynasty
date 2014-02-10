#ifndef CHATBOX_H
#define CHATBOX_H

#include <stdbool.h>

enum ChatType {
	CHATTYPE_CHAT,
	CHATTYPE_LOG
};

extern void ChatBox_ClearHistory(void);
extern void ChatBox_ResetTimestamps(void);
extern void ChatBox_AddChat(int peerID, const char *name, const char *msg);
extern void ChatBox_AddLog(enum ChatType type, const char *msg);
extern void ChatBox_Draw(const char *buf, bool draw_cursor);
extern void ChatBox_DrawInGame(const char *buf);

#endif
