/* net.c */

#include <assert.h>
#include <enet/enet.h>
#include <stdio.h>
#include <string.h>

#include "net.h"

#include "client.h"
#include "message.h"
#include "server.h"
#include "../house.h"
#include "../opendune.h"
#include "../pool/house.h"

#if 0
#define NET_LOG(FORMAT,...)	\
	do { fprintf(stderr, "%s:%d " FORMAT "\n", __FUNCTION__, __LINE__, __VA_ARGS__); } while (false)
#else
#define NET_LOG(...)
#endif

enum HouseFlag g_client_houses;
enum NetHostType g_host_type;
static ENetHost *s_host;
static ENetPeer *s_peer;

/*--------------------------------------------------------------*/

void
Net_Initialise(void)
{
	enet_initialize();
	atexit(enet_deinitialize);
}

static bool
Server_ConnectClient(ENetPacket *packet, ENetPeer *peer)
{
	bool success = false;

	if (packet->dataLength == 2 && packet->data[0] == '=') {
		const enum HouseType houseID = packet->data[1];

		if ((HOUSE_HARKONNEN <= houseID && houseID < HOUSE_MAX)
				&& !(g_client_houses & (1 << houseID))) {
			House *h = House_Get_ByIndex(houseID);

			peer->data = (void *)houseID;

			g_client_houses |= (1 << houseID);
			h->flags.human = true;

			success = true;
		}
	}

	return success;
}

static void
Server_DisconnectClient(ENetPeer *peer)
{
	const enum HouseType houseID = (enum HouseType)peer->data;
	House *h = House_Get_ByIndex(houseID);

	g_client_houses &= ~(1 << houseID);
	h->flags.human = false;
	h->flags.isAIActive = true;

	enet_peer_disconnect(peer, 0);
}

static void
Server_Synchronise(void)
{
	int clients_connected = 0;

	g_client_houses = 0;

	while (clients_connected == 0) {
		ENetEvent event;
		if (enet_host_service(s_host, &event, 1000) == 0)
			continue;

		switch (event.type) {
			case ENET_EVENT_TYPE_RECEIVE:
				{
					const bool success
						= Server_ConnectClient(event.packet, event.peer);

					if (success) {
						clients_connected++;
						enet_peer_send(event.peer, 0, event.packet);
					}
					else {
						enet_packet_destroy(event.packet);
						enet_peer_disconnect(event.peer, 0);
					}
				}
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
			case ENET_EVENT_TYPE_CONNECT:
			case ENET_EVENT_TYPE_NONE:
			default:
				break;
		}
	}

	enet_host_service(s_host, NULL, 0); /* XXX */

	Server_ResetCache();
}

static void
Client_Synchronise(void)
{
	unsigned char buf[2];
	buf[0] = '=';
	buf[1] = g_playerHouseID;

	ENetPacket *packet
		= enet_packet_create(&buf, 2, ENET_PACKET_FLAG_RELIABLE);

	enet_peer_send(s_peer, 0, packet);

	Client_ResetCache();
}

void
Net_Synchronise(void)
{
	switch (g_host_type) {
		case HOSTTYPE_DEDICATED_SERVER:
		case HOSTTYPE_CLIENT_SERVER:
			Server_Synchronise();
			break;

		case HOSTTYPE_DEDICATED_CLIENT:
			Client_Synchronise();
			break;

		default:
			return;
	}
}

bool
Net_CreateServer(int port)
{
	const int max_clients = HOUSE_MAX;

	if (g_host_type == HOSTTYPE_NONE && s_host == NULL && s_peer == NULL) {
		ENetAddress address;
		address.host = ENET_HOST_ANY;
		address.port = port;

		s_host = enet_host_create(&address, max_clients, 2, 0, 0);
		if (s_host != NULL) {
			ENetEvent event;

			NET_LOG("%s", "Created server.");

			if (enet_host_service(s_host, &event, 10000) > 0) {
				if (event.type == ENET_EVENT_TYPE_CONNECT) {
					NET_LOG("A new client connected from %x:%u.",
							event.peer->address.host, event.peer->address.port);

					g_host_type = HOSTTYPE_CLIENT_SERVER;
					return true;
				}
			}

			/* Timeout. */
			enet_host_destroy(s_host);
			s_host = NULL;
		}
	}

	return false;
}

bool
Net_ConnectToServer(const char *hostname, int port)
{
	if (g_host_type == HOSTTYPE_NONE && s_host == NULL && s_peer == NULL) {
		ENetAddress address;
		enet_address_set_host(&address, hostname);
		address.port = port;

		s_host = enet_host_create(NULL, 1, 2, 57600/8, 14400/8);
		if (s_host != NULL) {
			s_peer = enet_host_connect(s_host, &address, 2, 0);
			if (s_peer != NULL) {
				ENetEvent event;

				if (enet_host_service(s_host, &event, 5000) > 0
						&& event.type == ENET_EVENT_TYPE_CONNECT) {
					NET_LOG("Connected to server %s:%d\n", hostname, port);
					enet_host_service(s_host, &event, 0);

					g_host_type = HOSTTYPE_DEDICATED_CLIENT;
					return true;
				}

				/* Timeout. */
				enet_peer_reset(s_peer);
				s_peer = NULL;
			}

			/* Error creating peer. */
			enet_host_destroy(s_host);
			s_host = NULL;
		}
	}

	return false;
}

/*--------------------------------------------------------------*/

void
Server_SendMessages(void)
{
	if (g_host_type != HOSTTYPE_DEDICATED_SERVER
	 && g_host_type != HOSTTYPE_CLIENT_SERVER)
		return;

	unsigned char *buf = g_server_broadcast_message_buf;

	Server_Send_UpdateCHOAM(&buf);
	Server_Send_UpdateLandscape(&buf);
	Server_Send_UpdateStructures(&buf);
	Server_Send_UpdateUnits(&buf);
	Server_Send_UpdateExplosions(&buf);

	unsigned char * const buf_start_client_specific = buf;

	for (size_t i = 0; i < s_host->peerCount; i++) {
		ENetPeer *peer = &s_host->peers[i];
		enum HouseType houseID = (enum HouseType)peer->data;

		buf = buf_start_client_specific;

		Server_Send_UpdateHouse(houseID, &buf);
		Server_Send_UpdateFogOfWar(houseID, &buf);

		if (buf + g_server2client_message_len[houseID]
				< g_server_broadcast_message_buf + MAX_SERVER_BROADCAST_MESSAGE_LEN) {
			memcpy(buf, g_server2client_message_buf[houseID],
					g_server2client_message_len[houseID]);
			buf += g_server2client_message_len[houseID];
			g_server2client_message_len[houseID] = 0;
		}

		const int len = buf - g_server_broadcast_message_buf;

		NET_LOG("packet size=%d, num outgoing packets=%lu",
				len, enet_list_size(&s_host->peers[0].outgoingReliableCommands));

		ENetPacket *packet
			= enet_packet_create(g_server_broadcast_message_buf, len,
					ENET_PACKET_FLAG_RELIABLE);
		enet_peer_send(peer, 0, packet);
	}
}

void
Server_RecvMessages(void)
{
	if (g_host_type == HOSTTYPE_DEDICATED_CLIENT)
		return;

	/* Process the local player's commands. */
	if (g_host_type == HOSTTYPE_NONE
	 || g_host_type == HOSTTYPE_CLIENT_SERVER) {
		Server_ProcessMessage(g_playerHouseID,
				g_client2server_message_buf, g_client2server_message_len);
		g_client2server_message_len = 0;

		if (g_host_type == HOSTTYPE_NONE)
			return;
	}

	ENetEvent event;
	while (enet_host_service(s_host, &event, 0) > 0) {
		switch (event.type) {
			case ENET_EVENT_TYPE_RECEIVE:
				{
					ENetPacket *packet = event.packet;
					enum HouseType houseID = (enum HouseType)event.peer->data;
					Server_ProcessMessage(houseID,
							packet->data, packet->dataLength);
					enet_packet_destroy(packet);
				}
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				Server_DisconnectClient(event.peer);
				break;

			case ENET_EVENT_TYPE_CONNECT:
			case ENET_EVENT_TYPE_NONE:
			default:
				break;
		}
	}
}

void
Client_SendMessages(void)
{
	if (g_host_type == HOSTTYPE_DEDICATED_SERVER)
		return;

	Client_Send_BuildQueue();

	if (g_host_type != HOSTTYPE_DEDICATED_CLIENT)
		return;

	if (g_client2server_message_len <= 0)
		return;

	NET_LOG("packet size=%d, num outgoing packets=%lu",
			g_client2server_message_len,
			enet_list_size(&s_host->peers[0].outgoingReliableCommands));

	ENetPacket *packet
		= enet_packet_create(
				g_client2server_message_buf, g_client2server_message_len,
				ENET_PACKET_FLAG_RELIABLE);

	enet_peer_send(s_peer, 0, packet);
	g_client2server_message_len = 0;
}

void
Client_RecvMessages(void)
{
	if (g_host_type == HOSTTYPE_DEDICATED_SERVER) {
		return;
	}
	else if (g_host_type != HOSTTYPE_DEDICATED_CLIENT) {
		House_Client_UpdateRadarState();
		Client_ChangeSelectionMode();
		return;
	}

	ENetEvent event;
	while (enet_host_service(s_host, &event, 0) > 0) {
		switch (event.type) {
			case ENET_EVENT_TYPE_RECEIVE:
				{
					ENetPacket *packet = event.packet;
					Client_ProcessMessage(packet->data, packet->dataLength);
					enet_packet_destroy(packet);
				}
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				g_gameMode = GM_QUITGAME;
				break;

			case ENET_EVENT_TYPE_CONNECT:
			case ENET_EVENT_TYPE_NONE:
			default:
				break;
		}
	}
}
