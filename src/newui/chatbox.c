/* chatbox.c */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

#include "chatbox.h"

#include "editbox.h"
#include "../common_a5.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../net/net.h"
#include "../opendune.h"
#include "../timer/timer.h"
#include "../video/video.h"

enum {
	CHAT_IN_GAME_DURATION   = 60 * 10,
	CHAT_FADEOUT_DURATION   = 60 * 1,
	MAX_CHAT_LINES = 9 + 1
};

typedef struct ChatEntry {
	enum ChatType type;
	int64_t timeout;
	unsigned char col;
	char name[MAX_NAME_LEN + 1];
	char msg[MAX_CHAT_LEN + 1];
} ChatEntry;

static ChatEntry s_history[MAX_CHAT_LINES];
static unsigned int s_historyHead = 0;
static unsigned int s_historyTail = 0;

void
ChatBox_ClearHistory(void)
{
	s_historyTail = s_historyHead;
}

void
ChatBox_ResetTimestamps(void)
{
	for (unsigned int i = 0; i < MAX_CHAT_LINES; i++) {
		s_history[i].timeout = 0;
	}
}

static int
ChatBox_CountLines(int64_t curr_ticks)
{
	int count = 0;

	for (unsigned int i = s_historyHead;
			i != s_historyTail;
			i = (i + 1) % MAX_CHAT_LINES) {
		if (s_history[i].timeout >= curr_ticks)
			count++;
	}

	return count;
}

static void
ChatBox_AddEntrySplit(enum ChatType type, unsigned char col,
		const char *name, const char *start, const char *end)
{
	ChatEntry *c = &s_history[s_historyTail];

	c->type = type;
	c->timeout = Timer_GetTimer(TIMER_GUI) + CHAT_IN_GAME_DURATION;
	c->col = col;

	if (name != NULL) {
		snprintf(c->name, sizeof(c->name), "%s", name);
	} else {
		c->name[0] = '\0';
	}

	memcpy(c->msg, start, end - start);
	c->msg[end - start] = '\0';

	s_historyTail = (s_historyTail + 1) % MAX_CHAT_LINES;

	if (s_historyTail == s_historyHead)
		s_historyHead = (s_historyHead + 1) % MAX_CHAT_LINES;
}

static const char *
skip_spaces(const char *s)
{
	while (isspace(*s))
		s++;
	return s;
}

static const char *
takeuntil_space(const char *start, int w, int *len)
{
	const char *p;

	for (p = start; *p && !isspace(*p); p++) {
		int char_len = Font_GetCharWidth(*p);
		if (*len + char_len > w)
			break;
		*len += char_len;
	}

	return p;
}

static const char *
takewhile_space(const char *start, int *len)
{
	const char *p;

	for (p = start; *p && isspace(*p); p++)
		*len += Font_GetCharWidth(*p);

	return p;
}

static void
ChatBox_AddEntry(enum ChatType type,
		int peerID, const char *name, const char *msg)
{
	const int w = 100;
	const char *start;
	unsigned char col;
	int len = 0;

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x11);
	if (name != NULL && name[0] != '\0') {
		len += Font_GetStringWidth(name);
		len += Font_GetCharWidth(' ');
	}

	if (type == CHATTYPE_CHAT) {
		const enum HouseType houseID = Net_GetClientHouse(peerID);

		start = skip_spaces(msg);
		col = (houseID == HOUSE_INVALID) ? 15 : (144 + 16 * houseID + 2);
	} else {
		start = msg;
		col = 0;
	}

	while (*start != '\0') {
		const char *cur = start;
		const char *end = start;

		for (;;) {
			cur = takeuntil_space(cur, w, &len);
			if (isspace(*cur)) {
				end = cur;
			} else {
				/* Note: if name != NULL, then we can split at the
				 * space between the use name and the message.
				 */
				if (*cur == '\0' || (start == end && name == NULL))
					end = cur;
				break;
			}

			cur = takewhile_space(cur, &len);
			if (*cur == '\0' || len >= w) {
				/* Too long to next word or at last word. */
				break;
			}
		}

		ChatBox_AddEntrySplit(type, col, name, start, end);
		start = skip_spaces(end);
		name = NULL;
		len = 0;
	}
}

void
ChatBox_AddChat(int peerID, const char *name, const char *msg)
{
	const enum ChatType type = (peerID == 0) ? CHATTYPE_LOG : CHATTYPE_CHAT;
	ChatBox_AddEntry(type, peerID, name, msg);
}

void
ChatBox_AddLog(enum ChatType type, const char *msg)
{
	assert(type != CHATTYPE_CHAT);
	ChatBox_AddEntry(type, 0, NULL, msg);
}

static void
ChatBox_DrawHistory(int x, int y, int style, int64_t curr_ticks)
{
	const int h = (style == 0x11) ? 7 : 10;

	for (unsigned int i = s_historyHead;
			i != s_historyTail;
			i = (i + 1) % MAX_CHAT_LINES) {
		const ChatEntry *c = &s_history[i];

		if (c->timeout < curr_ticks)
			continue;

		int dx = 0;

		unsigned char fg;
		unsigned char alpha;

		if ((style == 0x22) && (c->timeout - curr_ticks < CHAT_FADEOUT_DURATION)) {
			alpha = 0xFF * (c->timeout - curr_ticks) / CHAT_FADEOUT_DURATION;
		} else {
			alpha = 0xFF;
		}

		if (c->type == CHATTYPE_CHAT) {
			if (c->name[0] != '\0') {
				fg = (style == 0x11) ? (144 + 2) : c->col;

				GUI_DrawText_Wrapper(NULL, 0, 0, fg, 0, style);
				GUI_DrawTextAlpha(c->name, x + dx, y, alpha);

				dx += Font_GetStringWidth(c->name);
				dx += Font_GetCharWidth(' ');
			}
		}

		fg = (style == 0x22) ? 15
			: (c->type == CHATTYPE_CHAT) ? 31
			: (c->type == CHATTYPE_LOG) ? 228
			: (144 + 16 * 1 + 3);

		GUI_DrawText_Wrapper(NULL, 0, 0, fg, 0, style);
		GUI_DrawTextAlpha(c->msg, x + dx, y, alpha);

		y += h;
	}
}

void
ChatBox_Draw(const char *buf, bool draw_cursor)
{
	Prim_DrawBorder(200, 90, 100 + 4, 63 + 5, 1, false, true, 0);
	ChatBox_DrawHistory(202, 92, 0x11, 0);

	Prim_DrawBorder(200, 159, 100 + 4, 11, 1, false, true, 0);
	EditBox_Draw(buf, 202, 161, 100, 7, 4, 31, 0x11, draw_cursor);
}

void
ChatBox_DrawInGame(const char *buf)
{
	const enum ScreenDivID divID = A5_SaveTransform();
	const ScreenDiv *div = &g_screenDiv[SCREENDIV_HUD];
	const int x = 8;
	const int y = div->height - 16;

	A5_UseTransform(SCREENDIV_HUD);

	const int64_t curr_ticks = Timer_GetTicks();
	ChatBox_DrawHistory(x, y - 10 * ChatBox_CountLines(curr_ticks), 0x22, curr_ticks);

	if (g_isEnteringChat) {
		int col = 144 + 16 * g_playerHouseID + 2;
		int dx = 0;

		GUI_DrawText_Wrapper("%s", x, y, col, 0, 0x22, g_net_name);
		dx += Font_GetStringWidth(g_net_name);
		dx += Font_GetCharWidth(' ');

		EditBox_Draw(buf, x + dx, y, 100, 10, 6, 15, 0x22, true);
	}

	A5_UseTransform(divID);
}
