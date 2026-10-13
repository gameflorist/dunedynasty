/* net_none.c
 *
 * Networking for builds without a network transport (WITH_ENET=OFF).
 * Single player and skirmish run with g_host_type == HOSTTYPE_NONE,
 * but still pass the local player's commands through
 * the client/server message buffers, so those paths are kept as in
 * net_enet.c.
 */

#include <stddef.h>

#include "net.h"

#include "client.h"
#include "message.h"
#include "server.h"
#include "../house.h"
#include "../mods/multiplayer.h"
#include "../opendune.h"

char g_net_name[MAX_NAME_LEN + 1] = "Name";
char g_host_addr[MAX_ADDR_LEN + 1] = "0.0.0.0";
char g_host_port[MAX_PORT_LEN + 1] = DEFAULT_PORT_STR;
char g_join_addr[MAX_ADDR_LEN + 1] = "localhost";
char g_join_port[MAX_PORT_LEN + 1] = DEFAULT_PORT_STR;
char g_chat_buf[MAX_CHAT_LEN + 1];

bool g_sendClientList;
bool g_sendScenario;
enum HouseFlag g_client_houses;
enum NetHostType g_host_type;

int g_local_client_id;
PeerData g_peer_data[MAX_CLIENTS];

/*--------------------------------------------------------------*/

PeerData *
Net_GetPeerData(int peerID)
{
	if (peerID == 0)
		return NULL;

	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (g_peer_data[i].id == peerID)
			return &g_peer_data[i];
	}

	return NULL;
}

const char *
Net_GetClientName(enum HouseType houseID)
{
	const uint8 id = g_multiplayer.client[houseID];
	const PeerData *data = Net_GetPeerData(id);

	return (data != NULL) ? data->name : NULL;
}

enum HouseType
Net_GetClientHouse(int peerID)
{
	if (peerID == 0)
		return HOUSE_INVALID;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		if (g_multiplayer.client[h] == peerID)
			return h;
	}

	return HOUSE_INVALID;
}

void
Net_Initialise(void)
{
}

bool
Net_CreateServer(const char *addr, int port, const char *name)
{
	VARIABLE_NOT_USED(addr);
	VARIABLE_NOT_USED(port);
	VARIABLE_NOT_USED(name);
	return false;
}

bool
Net_ConnectToServer(const char *hostname, int port, const char *name)
{
	VARIABLE_NOT_USED(hostname);
	VARIABLE_NOT_USED(port);
	VARIABLE_NOT_USED(name);
	return false;
}

void
Net_Disconnect(void)
{
	g_host_type = HOSTTYPE_NONE;
}

bool
Net_IsPlayable(void)
{
	return false;
}

bool
Net_HasAtLeastTwoPlayers(void)
{
	return false;
}

bool
Net_HasAllPlayersAssigned(void)
{
	return false;
}

bool
Net_HasAtLeastTwoTeams(void)
{
	return false;
}

bool
Net_HasClientRole(void)
{
	return false;
}

/* No transport: multiplayer menus are hidden. */
bool
Net_IsAvailable(void)
{
	return false;
}

bool
Net_HasServerRole(void)
{
	return false;
}

void
Net_Synchronise(void)
{
}

/*--------------------------------------------------------------*/

void
Net_Send_Chat(const char *buf)
{
	VARIABLE_NOT_USED(buf);
}

void
Server_Recv_Chat(int peerID, enum HouseFlag houses, const char *buf)
{
	VARIABLE_NOT_USED(peerID);
	VARIABLE_NOT_USED(houses);
	VARIABLE_NOT_USED(buf);
}

bool
Server_Send_StartGame(void)
{
	return false;
}

void
Server_SendMessages(void)
{
}

void
Server_DisconnectClient(PeerData *data)
{
	VARIABLE_NOT_USED(data);
}

void
Server_RecvMessages(void)
{
	/* Process the local player's commands. */
	Server_ProcessMessage(g_local_client_id, g_playerHouseID,
			g_client2server_message_buf, g_client2server_message_len);
	g_client2server_message_len = 0;
}

void
Client_SendMessages(void)
{
}

enum NetEvent
Client_RecvMessages(void)
{
	House_Client_UpdateRadarState();
	Client_ChangeSelectionMode();
	return NETEVENT_NORMAL;
}
