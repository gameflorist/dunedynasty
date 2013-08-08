/* message.c */

#include <assert.h>

#include "message.h"

#include "../object.h"

static const struct {
	unsigned char symbol;
	int len;
} s_table_csmsg[CSMSG_MAX] = {
	{ 'x', 0 }, /* CSMSG_DISCONNECT */
	{ 'r', 2 }, /* CSMSG_REPAIR_UPGRADE_STRUCTURE */
	{ 'g', 4 }, /* CSMSG_SET_RALLY_POINT */
	{ 'b', 2 }, /* CSMSG_ENTER_LEAVE_PLACEMENT_MODE */
	{ 'p', 2 }, /* CSMSG_PLACE_STRUCTURE */
	{ 'u', 5 }, /* CSMSG_ISSUE_UNIT_ACTION */
};

unsigned char g_server2client_message_buf[MAX_SERVER_MESSAGE_LEN];
unsigned char g_client2server_message_buf[MAX_CLIENT_MESSAGE_LEN];
int g_server2client_message_len;
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
