/* message.c */

#include <assert.h>

#include "message.h"

#include "net.h"
#include "../object.h"

static const struct {
	unsigned char symbol;
	int len;
} s_table_csmsg[CSMSG_MAX] = {
	{ 'x', 0 }, /* CSMSG_DISCONNECT */
	{ 'r', 2 }, /* CSMSG_REPAIR_UPGRADE_STRUCTURE */
	{ 'g', 4 }, /* CSMSG_SET_RALLY_POINT */
	{ '+', 3 }, /* CSMSG_PURCHASE_RESUME_ITEM */
	{ '-', 3 }, /* CSMSG_PAUSE_CANCEL_ITEM */
	{ 'b', 2 }, /* CSMSG_ENTER_LEAVE_PLACEMENT_MODE */
	{ 'p', 2 }, /* CSMSG_PLACE_STRUCTURE */
	{ 's', 2 }, /* CSMSG_ACTIVATE_STRUCTURE_ABILITY */
	{ 'w', 2 }, /* CSMSG_LAUNCH_DEATHHAND */
	{ 'u', 5 }, /* CSMSG_ISSUE_UNIT_ACTION */
	{ 'n', MAX_NAME_LEN }, /* CSMSG_PREFERRED_NAME */
	{'\'', MAX_CHAT_LEN + 2 }, /* CSMSG_CHAT */
};

static unsigned char s_table_scmsg[SCMSG_MAX] = {
	'X', /* SCMSG_DISCONNECT */
	'L', /* SCMSG_UPDATE_LANDSCAPE */
	'F', /* SCMSG_UPDATE_FOG_OF_WAR */
	'H', /* SCMSG_UPDATE_HOUSE */
	'C', /* SCMSG_UPDATE_CHOAM */
	'S', /* SCMSG_UPDATE_STRUCTURES */
	'U', /* SCMSG_UPDATE_UNITS */
	'E', /* SCMSG_UPDATE_EXPLOSIONS */
	'*', /* SCMSG_SCREEN_SHAKE */
	'M', /* SCMSG_STATUS_MESSAGE */
	'<', /* SCMSG_PLAY_SOUND */
	'>', /* SCMSG_PLAY_SOUND_AT_TILE */
	'V', /* SCMSG_PLAY_VOICE */
	'!', /* SCMSG_PLAY_BATTLE_MUSIC */
	'I', /* SCMSG_IDENTITY */
	'N', /* SCMSG_CLIENT_LIST */
	'"', /* SCMSG_CHAT */
};

unsigned char g_server_broadcast_message_buf[MAX_SERVER_BROADCAST_MESSAGE_LEN];
unsigned char g_server2client_message_buf[HOUSE_MAX][MAX_SERVER_TO_CLIENT_MESSAGE_LEN];
unsigned char g_client2server_message_buf[MAX_CLIENT_MESSAGE_LEN];
int g_server2client_message_len[HOUSE_MAX];
int g_client2server_message_len;

/*--------------------------------------------------------------*/

void
Net_Encode_uint8(unsigned char **buf, uint8 val)
{
	(*buf)[0] = val;
	(*buf)++;
}

uint8
Net_Decode_uint8(const unsigned char **buf)
{
	const uint8 ret = (*buf)[0];
	(*buf)++;
	return ret;
}

void
Net_Encode_uint16(unsigned char **buf, uint16 val)
{
	(*buf)[0] = val;
	(*buf)[1] = val >> 8;
	(*buf) += 2;
}

uint16
Net_Decode_uint16(const unsigned char **buf)
{
	const uint16 ret = ((*buf)[1] << 8) | (*buf)[0];
	(*buf) += 2;
	return ret;
}

void
Net_Encode_uint32(unsigned char **buf, uint32 val)
{
	(*buf)[0] = val;
	(*buf)[1] = val >> 8;
	(*buf)[2] = val >> 16;
	(*buf)[3] = val >> 24;
	(*buf) += 4;
}

uint32
Net_Decode_uint32(const unsigned char **buf)
{
	const uint32 ret = ((*buf)[3] << 24) | ((*buf)[2] << 16) | ((*buf)[1] << 8) | (*buf)[0];
	(*buf) += 4;
	return ret;
}

void
Net_Encode_ObjectIndex(unsigned char **buf, const Object *o)
{
	Net_Encode_uint16(buf, o->index);
}

uint16
Net_Decode_ObjectIndex(const unsigned char **buf)
{
	return Net_Decode_uint16(buf);
}

int
Net_GetLength_ClientServerMsg(enum ClientServerMsg msg)
{
	return (msg < CSMSG_MAX) ? s_table_csmsg[msg].len : 0;
}

void
Net_Encode_ClientServerMsg(unsigned char **buf, enum ClientServerMsg msg)
{
	assert(msg < CSMSG_MAX);

	Net_Encode_uint8(buf, s_table_csmsg[msg].symbol);
}

enum ClientServerMsg
Net_Decode_ClientServerMsg(unsigned char c)
{
	for (enum ClientServerMsg msg = CSMSG_DISCONNECT; msg < CSMSG_MAX; msg++) {
		if (s_table_csmsg[msg].symbol == c)
			return msg;
	}

	return CSMSG_INVALID;
}

void
Net_Encode_ServerClientMsg(unsigned char **buf, enum ServerClientMsg msg)
{
	assert(msg < SCMSG_MAX);

	Net_Encode_uint8(buf, s_table_scmsg[msg]);
}

enum ServerClientMsg
Net_Decode_ServerClientMsg(unsigned char c)
{
	for (enum ServerClientMsg msg = SCMSG_DISCONNECT; msg < SCMSG_MAX; msg++) {
		if (s_table_scmsg[msg] == c)
			return msg;
	}

	return SCMSG_INVALID;
}
