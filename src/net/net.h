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
	MAX_CHAT_LEN = 60,
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
	char name[MAX_NAME_LEN + 1];
} PeerData;

extern char g_net_name[MAX_NAME_LEN + 1];
extern char g_host_addr[MAX_ADDR_LEN + 1];
extern char g_host_port[MAX_PORT_LEN + 1];
extern char g_join_addr[MAX_ADDR_LEN + 1];
extern char g_join_port[MAX_PORT_LEN + 1];

extern enum HouseFlag g_client_houses;
extern enum NetHostType g_host_type;
extern int g_local_client_id;
extern PeerData g_peer_data[MAX_CLIENTS];

extern PeerData *Net_NewPeerData(int peerID);
extern PeerData *Net_GetPeerData(int peerID);
extern const char *Net_GetClientName(enum HouseType houseID);
extern enum HouseType Net_GetClientHouse(int peerID);

extern void Net_Initialise(void);
extern bool Net_CreateServer(const char *addr, int port, const char *name);
extern bool Net_ConnectToServer(const char *hostname, int port, const char *name);
extern void Net_Disconnect(void);
extern void Net_Synchronise(void);

extern void Net_Send_Chat(const char *buf);
extern void Server_Recv_Chat(int peerID, enum HouseFlag houses, const char *buf);
extern void Server_SendMessages(void);
extern void Server_RecvMessages(void);
extern void Client_SendMessages(void);
extern enum NetEvent Client_RecvMessages(void);

#endif
