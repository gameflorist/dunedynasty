#ifndef NET_NET_H
#define NET_NET_H

#include <stdbool.h>
#include "enumeration.h"

enum {
	DEFAULT_PORT = 1234
};

enum NetHostType {
	HOSTTYPE_NONE,
	HOSTTYPE_DEDICATED_SERVER,
	HOSTTYPE_CLIENT_SERVER,
	HOSTTYPE_DEDICATED_CLIENT,
};

extern enum HouseFlag g_human_houses;
extern enum NetHostType g_host_type;

extern void Net_Initialise(void);
extern void Net_Synchronise(void);
extern bool Net_CreateServer(int port);
extern bool Net_ConnectToServer(const char *hostname, int port);

extern void Server_SendMessages(void);
extern void Server_RecvMessages(void);
extern void Client_SendMessages(void);
extern void Client_RecvMessages(void);

#endif
