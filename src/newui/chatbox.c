/* chatbox.c */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

#include "chatbox.h"

#include "editbox.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../net/net.h"
#include "../video/video.h"

enum {
	MAX_CHAT_LINES = 9 + 1
};

typedef struct ChatEntry {
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

static void
ChatBox_AddEntrySplit(const char *name, const char *start, const char *end)
{
	ChatEntry *c = &s_history[s_historyTail];

	if (name != NULL) {
		snprintf(c->name, sizeof(c->name), "%s", name);
	}
	else {
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

void
ChatBox_AddEntry(const char *name, const char *msg)
{
	const int w = 100;
	int len = 0;

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x11);
	if (name != NULL && name[0] != '\0') {
		len += Font_GetStringWidth(name);
		len += Font_GetCharWidth(' ');
	}

	const char *start = skip_spaces(msg);

	while (*start != '\0') {
		const char *cur = start;
		const char *end = start;

		for (;;) {
			cur = takeuntil_space(cur, w, &len);
			if (isspace(*cur)) {
				end = cur;
			}
			else {
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

		ChatBox_AddEntrySplit(name, start, end);
		start = skip_spaces(end);
		name = NULL;
		len = 0;
	}
}

static void
ChatBox_DrawHistory(int x, int y, int w, int h)
{
	Prim_DrawBorder(x, y, w, h, 1, false, true, 0);
	x += 2;
	y += 2;

	for (unsigned int i = s_historyHead;
			i != s_historyTail;
			i = (i + 1) % MAX_CHAT_LINES) {
		const ChatEntry *c = &s_history[i];
		int dx = 0;

		if (c->name[0] != '\0') {
			GUI_DrawText_Wrapper("%s", x, y, 15, 0, 0x11, c->name);
			dx += Font_GetStringWidth(c->name);
			dx += Font_GetCharWidth(' ');
		}

		GUI_DrawText_Wrapper("%s", x + dx, y, 31, 0, 0x11, c->msg);

		y += 7;
	}
}

void
ChatBox_Draw(const char *buf, bool draw_cursor)
{
	ChatBox_DrawHistory(200, 90, 100 + 4, 63 + 2 + 3);

	Prim_DrawBorder(200, 159, 100 + 4, 11, 1, false, true, 0);
	EditBox_Draw(buf, 202, 161, 100, 7, 4, 31, 0x11, draw_cursor);
}
