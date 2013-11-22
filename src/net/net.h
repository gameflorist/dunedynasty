#ifndef NET_NET_H
#define NET_NET_H

#include <stdbool.h>
#include "enumeration.h"
#include "types.h"

enum NetEvent {
	NETEVENT_NORMAL,
	NETEVENT_DISCONNECT,
};

enum {
	MAX_CLIENTS = HOUSE_MAX,
	MAX_NAME_LEN = 14,
	MAX_ADDR_LEN = 1023,
	MAX_PORT_LEN = 5,
	DEFAULT_PORT = 10700
};

#define DEFAULT_PORT_STR "10700"

enum NetHostType {
	HOSTTYPE_NONE,
	HOSTTYPE_DEDICATED_SERVER,
	HOSTTYPE_CLIENT_SERVER,
	HOSTTYPE_DEDICATED_CLIENT,
};

typedef struct PeerData {
	int id;
	void *peer;
} PeerData;

extern enum HouseFlag g_client_houses;
extern enum NetHostType g_host_type;
extern int g_local_client_id;

extern void Net_Initialise(void);
extern bool Net_CreateServer(const char *addr, int port);
extern bool Net_ConnectToServer(const char *hostname, int port);
extern void Net_Disconnect(void);
extern void Net_Synchronise(void);

extern void Server_SendMessages(void);
extern void Server_RecvMessages(void);
extern void Client_SendMessages(void);
extern enum NetEvent Client_RecvMessages(void);

#endif
