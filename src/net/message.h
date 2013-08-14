#ifndef NET_MESSAGE_H
#define NET_MESSAGE_H

#include "enumeration.h"
#include "types.h"

enum {
	MAX_SERVER_BROADCAST_MESSAGE_LEN = 32768,
	MAX_SERVER_TO_CLIENT_MESSAGE_LEN = 1024,
	MAX_CLIENT_MESSAGE_LEN = 32768
};

enum ClientServerMsg {
	CSMSG_DISCONNECT,

	CSMSG_REPAIR_UPGRADE_STRUCTURE,
	CSMSG_SET_RALLY_POINT,
	CSMSG_PURCHASE_RESUME_ITEM,
	CSMSG_PAUSE_CANCEL_ITEM,
	CSMSG_ENTER_LEAVE_PLACEMENT_MODE,
	CSMSG_PLACE_STRUCTURE,
	CSMSG_ACTIVATE_STRUCTURE_ABILITY,
	CSMSG_LAUNCH_DEATHHAND,
	CSMSG_ISSUE_UNIT_ACTION,

	CSMSG_MAX,
	CSMSG_INVALID
};

enum ServerClientMsg {
	SCMSG_DISCONNECT,

	SCMSG_UPDATE_LANDSCAPE,

	SCMSG_MAX,
	SCMSG_INVALID
};

struct Object;

extern unsigned char g_server_broadcast_message_buf[MAX_SERVER_BROADCAST_MESSAGE_LEN];
extern unsigned char g_server2client_message_buf[HOUSE_MAX][MAX_SERVER_TO_CLIENT_MESSAGE_LEN];
extern unsigned char g_client2server_message_buf[MAX_CLIENT_MESSAGE_LEN];
extern int g_server2client_message_len[HOUSE_MAX];
extern int g_client2server_message_len;

extern void   Net_Encode_uint8 (unsigned char **buf, uint8 val);
extern uint8  Net_Decode_uint8 (const unsigned char **buf);
extern void   Net_Encode_uint16(unsigned char **buf, uint16 val);
extern uint16 Net_Decode_uint16(const unsigned char **buf);
extern void   Net_Encode_uint32(unsigned char **buf, uint32 val);
extern uint32 Net_Decode_uint32(const unsigned char **buf);

extern void   Net_Encode_ObjectIndex(unsigned char **buf, const struct Object *o);
extern uint16 Net_Decode_ObjectIndex(const unsigned char **buf);

extern int Net_GetLength_ClientServerMsg(enum ClientServerMsg msg);
extern void Net_Encode_ClientServerMsg(unsigned char **buf, enum ClientServerMsg msg);
extern enum ClientServerMsg Net_Decode_ClientServerMsg(unsigned char c);

extern void Net_Encode_ServerClientMsg(unsigned char **buf, enum ServerClientMsg msg);
extern enum ServerClientMsg Net_Decode_ServerClientMsg(unsigned char c);

#endif
