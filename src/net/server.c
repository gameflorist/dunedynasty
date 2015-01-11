/* server.c */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../os/common.h"
#include "../os/math.h"
#include "../os/strings.h"
#include "enum_string.h"

#include "server.h"

#include "message.h"
#include "net.h"
#include "../audio/audio.h"
#include "../enhancement.h"
#include "../explosion.h"
#include "../newui/actionpanel.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../map.h"
#include "../mods/multiplayer.h"
#include "../newui/chatbox.h"
#include "../newui/menu.h"
#include "../newui/menubar.h"
#include "../newui/viewport.h"
#include "../opendune.h"
#include "../pool/pool_house.h"
#include "../pool/pool_structure.h"
#include "../pool/pool_unit.h"
#include "../shape.h"
#include "../string.h"
#include "../structure.h"
#include "../timer/timer.h"
#include "../tools/coord.h"
#include "../tools/encoded_index.h"
#include "../tools/random_starport.h"
#include "../unit.h"

#if 0
#define SERVER_LOG(FORMAT,...)	\
	do { fprintf(stderr, "%s:%d " FORMAT "\n", __FUNCTION__, __LINE__, __VA_ARGS__); } while (false)
#else
#define SERVER_LOG(...)
#endif

typedef struct StructureDelta {
	uint8       type;
	uint8       linkedID;
	ObjectFlags flags;
	uint8       houseID;
	tile32      position;
	uint16      hitpoints;

	uint8       creatorHouse;
	uint16      rotationSprite;
	uint8       objectType;
	uint8       upgradeLevel;
	uint8       upgradeTime;
	uint16      countDown;
	uint16      rallyPoint;

	uint8       buildQueueCount[OBJECTTYPE_MAX];
} StructureDelta;

typedef struct UnitDelta {
	uint8   type;
	ObjectFlags flags;
	uint8   houseID;
	tile32  position;
	uint16  hitpoints;

	uint8   actionID;
	uint8   nextActionID;
	uint8   amount;
	uint8   deviated;
	uint8   deviatedHouse;
	int8    orientation0_current;
	int8    orientation1_current;
	uint8   wobbleIndex;
	uint8   spriteOffset;
} UnitDelta;

static Tile s_mapCopy[MAP_SIZE_MAX * MAP_SIZE_MAX];
static int64_t s_choamLastUpdate;
static StructureDelta s_structureCopy[STRUCTURE_INDEX_MAX_HARD];
static UnitDelta s_unitCopy[UNIT_INDEX_MAX];
static int s_explosionLastCount;

static void Server_ReturnToLobbyNow(bool win);

/*--------------------------------------------------------------*/

void
Server_RestockStarport(enum UnitType unitType)
{
	if (g_starportAvailable[unitType] != 0 && g_starportAvailable[unitType] < 10) {
		if (g_starportAvailable[unitType] == -1) {
			g_starportAvailable[unitType] = 1;
		} else {
			g_starportAvailable[unitType]++;
		}

		s_choamLastUpdate = 0;
	}
}

static void
Server_DepleteStarport(enum UnitType unitType)
{
	g_starportAvailable[unitType]--;

	if (g_starportAvailable[unitType] <= 0)
		g_starportAvailable[unitType] = -1;

	s_choamLastUpdate = 0;
}

/*--------------------------------------------------------------*/

static bool
Server_CanEncodeFixedWidthBuffer(unsigned char **buf, size_t len)
{
	const unsigned char * const end
		= g_server_broadcast_message_buf + MAX_SERVER_BROADCAST_MESSAGE_LEN;

	return (*buf + len <= end);
}

static int
Server_MaxElementsToEncode(unsigned char **buf,
		size_t header_len, size_t element_len)
{
	const unsigned char * const end
		= g_server_broadcast_message_buf + MAX_SERVER_BROADCAST_MESSAGE_LEN;

	if (*buf + header_len + element_len <= end) {
		return (end - *buf - header_len) / element_len;
	} else {
		return 0;
	}
}

static void
Server_InitStructureDelta(const Structure *s, StructureDelta *d)
{
	const Object *o = &s->o;

	memset(d, 0, sizeof(StructureDelta));

	d->type         = o->type;
	d->linkedID     = o->linkedID;
	d->flags        = o->flags;
	d->houseID      = o->houseID;
	d->position     = o->position;
	d->hitpoints    = o->hitpoints;

	d->creatorHouse     = s->creatorHouseID;
	d->rotationSprite   = s->rotationSpriteDiff;
	d->objectType       = s->objectType;
	d->upgradeLevel     = s->upgradeLevel;
	d->upgradeTime      = s->upgradeTimeLeft;
	d->countDown        = s->countDown;
	d->rallyPoint       = s->rallyPoint;

	for (uint16 objectType = 0; objectType < OBJECTTYPE_MAX; objectType++) {
		d->buildQueueCount[objectType]
			= BuildQueue_Count(&s->queue, objectType);
	}
}

static void
Server_InitUnitDelta(const Unit *u, UnitDelta *d)
{
	const Object *o = &u->o;

	memset(d, 0, sizeof(UnitDelta));

	d->type         = o->type;
	d->flags.all    = o->flags.all;
	d->houseID      = o->houseID;
	d->position     = o->position;
	d->hitpoints    = o->hitpoints;

	d->actionID             = u->actionID;
	d->nextActionID         = u->nextActionID;
	d->amount               = u->amount;
	d->deviated             = u->deviated;
	d->deviatedHouse        = u->deviatedHouse;
	d->orientation0_current = u->orientation[0].current;
	d->orientation1_current = u->orientation[1].current;
	d->wobbleIndex          = u->wobbleIndex;
	d->spriteOffset         = u->spriteOffset;
}

void
Server_ResetCache(void)
{
	memset(g_server_broadcast_message_buf, 0, MAX_SERVER_BROADCAST_MESSAGE_LEN);

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		memset(g_server2client_message_buf[h], 0, MAX_SERVER_TO_CLIENT_MESSAGE_LEN);
		g_server2client_message_len[h] = 0;
	}

	memset(s_mapCopy, 0, sizeof(s_mapCopy));
	memset(s_structureCopy, 0, sizeof(s_structureCopy));
	memset(s_unitCopy, 0, sizeof(s_unitCopy));
	s_choamLastUpdate = 0;
	s_explosionLastCount = 0;
}

/*--------------------------------------------------------------*/

void
Server_Send_UpdateLandscape(unsigned char **buf)
{
	const size_t header_len  = 1 + 2;
	const size_t element_len = 2 + sizeof(Tile);
	const int max = Server_MaxElementsToEncode(buf, header_len, element_len);

	if (max <= 0)
		return;

	Net_Encode_ServerClientMsg(buf, SCMSG_UPDATE_LANDSCAPE);

	unsigned char *buf_count = *buf; (*buf) += 2;
	uint16 count = 0;

	for (uint16 packed = 65;
			packed < MAP_SIZE_MAX * MAP_SIZE_MAX - 65 && count < max;
			packed++) {
		Tile d = g_map[packed];
		d.hasAnimation = 0;
		d.hasExplosion = 0;

		if (memcmp(&s_mapCopy[packed], &d, sizeof(Tile)) == 0)
			continue;

		s_mapCopy[packed] = d;

		Net_Encode_uint16(buf, packed);
		memcpy(*buf, &d, sizeof(Tile));
		(*buf) += sizeof(Tile);

		count++;
	}

	SERVER_LOG("tiles changed=%d, %lu bytes",
			count, *buf - buf_count + 1);

	Net_Encode_uint16(&buf_count, count);
}

void
Server_Send_UpdateFogOfWar(enum HouseType houseID, unsigned char **buf)
{
	if (!enhancement_fog_of_war)
		return;

	const size_t header_len  = 1 + 2;
	const size_t element_len = 2;
	const int max = Server_MaxElementsToEncode(buf, header_len, element_len);

	if (max <= 0)
		return;

	Net_Encode_ServerClientMsg(buf, SCMSG_UPDATE_FOG_OF_WAR);

	unsigned char *buf_count = *buf; (*buf) += 2;
	uint16 count = 0;

	for (uint16 packed = 65;
			packed < MAP_SIZE_MAX * MAP_SIZE_MAX - 65 && count < max;
			packed++) {
		FogOfWarTile *f = &g_mapVisible[packed];

		if (f->cause[houseID] == UNVEILCAUSE_UNCHANGED)
			continue;

		if (f->cause[houseID] < UNVEILCAUSE_STRUCTURE_VISION) {
			uint16 encoded = packed;

			/* Short unveil. */
			if (f->cause[houseID] == UNVEILCAUSE_EXPLOSION)
				encoded |= 0x8000;

			Net_Encode_uint16(buf, encoded);

			count++;
		}

		f->cause[houseID] = UNVEILCAUSE_UNCHANGED;
	}

	SERVER_LOG("unveiled tiles=%d, %lu bytes",
			count, *buf - buf_count + 1);

	Net_Encode_uint16(&buf_count, count);
}

void
Server_Send_UpdateHouse(enum HouseType houseID, unsigned char **buf)
{
	const size_t len = 1 + 23 + (UNIT_MCV - UNIT_CARRYALL + 1) * 1;
	if (!Server_CanEncodeFixedWidthBuffer(buf, len))
		return;

	const House *h = House_Get_ByIndex(houseID);

	Net_Encode_ServerClientMsg(buf, SCMSG_UPDATE_HOUSE);

	/* 23 bytes. */
	Net_Encode_uint32(buf, h->structuresBuilt);     /* XXX - client. */
	Net_Encode_uint16(buf, h->credits);
	Net_Encode_uint16(buf, h->creditsStorage);      /* XXX - client. */
	Net_Encode_uint16(buf, h->powerProduction);     /* XXX - client. */
	Net_Encode_uint16(buf, h->powerUsage);          /* XXX - client. */
	Net_Encode_uint16(buf, h->windtrapCount);       /* XXX - client. */
	Net_Encode_uint16(buf, h->starportTimeLeft);
	Net_Encode_uint16(buf, h->starportLinkedID);
	Net_Encode_uint16(buf, h->structureActiveID);
	Net_Encode_uint16(buf, h->houseMissileID);
	Net_Encode_uint8 (buf, h->houseMissileCountdown);

	for (enum UnitType u = UNIT_CARRYALL; u <= UNIT_MCV; u++) {
		Net_Encode_uint8(buf, h->starportCount[u]);
	}
}

void
Server_Send_UpdateCHOAM(unsigned char **buf)
{
	if (s_choamLastUpdate == g_tickHouseStarportRecalculatePrices)
		return;

	const size_t len = 1 + 2 + (UNIT_MCV - UNIT_CARRYALL + 1) * 1;
	if (!Server_CanEncodeFixedWidthBuffer(buf, len))
		return;

	const uint16 seed = Random_Starport_GetInitialSeed();

	Net_Encode_ServerClientMsg(buf, SCMSG_UPDATE_CHOAM);
	Net_Encode_uint16(buf, seed);

	for (enum UnitType u = UNIT_CARRYALL; u <= UNIT_MCV; u++) {
		Net_Encode_uint8(buf, g_starportAvailable[u]);
	}

	s_choamLastUpdate = g_tickHouseStarportRecalculatePrices;
}

void
Server_Send_UpdateStructures(unsigned char **buf)
{
	const size_t header_len  = 1 + 1;
	const size_t element_len = 2 + 13 + 10 + OBJECTTYPE_MAX;
	const int max = Server_MaxElementsToEncode(buf, header_len, element_len);

	if (max <= 0)
		return;

	Net_Encode_ServerClientMsg(buf, SCMSG_UPDATE_STRUCTURES);

	unsigned char *buf_count = *buf; (*buf) += 1;
	uint8 count = 0;

	for (int i = 0;
			i < STRUCTURE_INDEX_MAX_HARD && count < max;
			i++) {
		const Structure *s = Structure_Get_ByIndex(i);
		StructureDelta d;

		Server_InitStructureDelta(s, &d);
		if (memcmp(&s_structureCopy[i], &d, sizeof(StructureDelta)) == 0)
			continue;

		memcpy(&s_structureCopy[i], &d, sizeof(StructureDelta));

		Net_Encode_ObjectIndex(buf, &s->o);

		/* 13 bytes. */
		Net_Encode_uint8 (buf, d.type);
		Net_Encode_uint8 (buf, d.linkedID);
		Net_Encode_uint32(buf, d.flags.all);
		Net_Encode_uint8 (buf, d.houseID);
		Net_Encode_uint16(buf, d.position.x);
		Net_Encode_uint16(buf, d.position.y);
		Net_Encode_uint16(buf, d.hitpoints);

		/* 10 bytes. */
		Net_Encode_uint8 (buf, d.creatorHouse);
		Net_Encode_uint16(buf, d.rotationSprite);
		Net_Encode_uint8 (buf, d.objectType);
		Net_Encode_uint8 (buf, d.upgradeLevel);
		Net_Encode_uint8 (buf, d.upgradeTime);
		Net_Encode_uint16(buf, d.countDown);
		Net_Encode_uint16(buf, d.rallyPoint);

		/* 32 bytes */
		for (uint16 objectType = 0; objectType < OBJECTTYPE_MAX; objectType++) {
			Net_Encode_uint8(buf, d.buildQueueCount[objectType]);
		}

		count++;
	}

	SERVER_LOG("structures changed=%d, %lu bytes",
			count, *buf - buf_count + 1);

	Net_Encode_uint8(&buf_count, count);
}

void
Server_Send_UpdateUnits(unsigned char **buf)
{
	const size_t header_len  = 1 + 1;
	const size_t element_len = 2 + 12 + 9;
	const int max = Server_MaxElementsToEncode(buf, header_len, element_len);

	if (max <= 0)
		return;

	Net_Encode_ServerClientMsg(buf, SCMSG_UPDATE_UNITS);

	unsigned char *buf_count = *buf; (*buf) += 1;
	uint8 count = 0;

	for (int i = 0;
			i < UNIT_INDEX_MAX && count < max;
			i++) {
		const Unit *u = Unit_Get_ByIndex(i);
		UnitDelta d;

		Server_InitUnitDelta(u, &d);
		if (memcmp(&s_unitCopy[i], &d, sizeof(UnitDelta)) == 0)
			continue;

		memcpy(&s_unitCopy[i], &d, sizeof(UnitDelta));

		Net_Encode_ObjectIndex(buf, &u->o);

		/* 12 bytes. */
		Net_Encode_uint8 (buf, d.type);
		Net_Encode_uint32(buf, d.flags.all);
		Net_Encode_uint8 (buf, d.houseID);
		Net_Encode_uint16(buf, d.position.x);
		Net_Encode_uint16(buf, d.position.y);
		Net_Encode_uint16(buf, d.hitpoints);

		/* 9 bytes. */
		Net_Encode_uint8 (buf, d.actionID);
		Net_Encode_uint8 (buf, d.nextActionID);
		Net_Encode_uint8 (buf, d.amount);
		Net_Encode_uint8 (buf, d.deviated);
		Net_Encode_uint8 (buf, d.deviatedHouse);
		Net_Encode_uint8 (buf, d.orientation0_current);
		Net_Encode_uint8 (buf, d.orientation1_current);
		Net_Encode_uint8 (buf, d.wobbleIndex);
		Net_Encode_uint8 (buf, d.spriteOffset);

		count++;
	}

	SERVER_LOG("units changed=%d, %lu bytes",
			count, *buf - buf_count + 1);

	Net_Encode_uint8(&buf_count, count);
}

void
Server_Send_UpdateExplosions(unsigned char **buf)
{
	const int num = Explosion_Get_NumActive();
	if (num <= 1 && num == s_explosionLastCount)
		return;

	const size_t len = 2 + (num - 1) * 7;
	if (!Server_CanEncodeFixedWidthBuffer(buf, len))
		return;

	Net_Encode_ServerClientMsg(buf, SCMSG_UPDATE_EXPLOSIONS);
	Net_Encode_uint8(buf, num);

	for (int i = 1; i < num; i++) {
		const Explosion *e = Explosion_Get_ByIndex(i);

		Net_Encode_uint16(buf, e->spriteID);
		Net_Encode_uint16(buf, e->position.x);
		Net_Encode_uint16(buf, e->position.y);
		Net_Encode_uint8 (buf, e->houseID);
	}

	s_explosionLastCount = num;
}

static void
Server_BufferGameEvent(enum HouseFlag houses, int len, const unsigned char *src)
{
	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		if (!(houses & (1 << houseID)))
			continue;

		if (g_server2client_message_len[houseID] + len >= MAX_SERVER_TO_CLIENT_MESSAGE_LEN)
			continue;

		memcpy(g_server2client_message_buf[houseID] + g_server2client_message_len[houseID],
				src, len);

		g_server2client_message_len[houseID] += len;
	}
}

void
Server_Send_ScreenShake(uint16 packed)
{
	GFX_ScreenShake_Start(packed, 1);

	if (g_client_houses != 0) {
		unsigned char src[3];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_SCREEN_SHAKE);
		Net_Encode_uint16(&buf, packed);

		Server_BufferGameEvent(g_client_houses, sizeof(src), src);
	}
}

void
Server_Send_StatusMessage1(enum HouseFlag houses, uint8 priority,
		uint16 str)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str, STR_NULL, STR_NULL);
	}

	houses &= g_client_houses;
	if (houses) {
		unsigned char src[8];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_STATUS_MESSAGE);
		Net_Encode_uint8 (&buf, priority);
		Net_Encode_uint16(&buf, str);
		Net_Encode_uint16(&buf, STR_NULL);
		Net_Encode_uint16(&buf, STR_NULL);

		Server_BufferGameEvent(houses, sizeof(src), src);
	}
}

void
Server_Send_StatusMessage2(enum HouseFlag houses, uint8 priority,
		uint16 str1, uint16 str2)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str1, str2, STR_NULL);
	}

	houses &= g_client_houses;
	if (houses) {
		unsigned char src[8];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_STATUS_MESSAGE);
		Net_Encode_uint8 (&buf, priority);
		Net_Encode_uint16(&buf, str1);
		Net_Encode_uint16(&buf, str2);
		Net_Encode_uint16(&buf, STR_NULL);

		Server_BufferGameEvent(houses, sizeof(src), src);
	}
}

void
Server_Send_StatusMessage3(enum HouseFlag houses, uint8 priority,
		uint16 str1, uint16 str2, uint16 str3)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str1, str2, str3);
	}

	houses &= g_client_houses;
	if (houses) {
		unsigned char src[8];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_STATUS_MESSAGE);
		Net_Encode_uint8 (&buf, priority);
		Net_Encode_uint16(&buf, str1);
		Net_Encode_uint16(&buf, str2);
		Net_Encode_uint16(&buf, str3);

		Server_BufferGameEvent(houses, sizeof(src), src);
	}
}

void
Server_Send_PlaySound(enum HouseFlag houses, enum SoundID soundID)
{
	if (soundID == SOUND_INVALID)
		return;

	if (houses & (1 << g_playerHouseID)) {
		Audio_PlaySound(soundID);
	}

	houses &= g_client_houses;
	if (houses) {
		unsigned char src[2];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_PLAY_SOUND);
		Net_Encode_uint8(&buf, soundID);

		Server_BufferGameEvent(houses, sizeof(src), src);
	}
}

void
Server_Send_PlaySoundAtTile(enum HouseFlag houses,
		enum SoundID soundID, tile32 position)
{
	if (soundID == SOUND_INVALID)
		return;

	if (houses & (1 << g_playerHouseID)) {
		Audio_PlaySoundAtTile(soundID, position);
	}

	houses &= g_client_houses;
	if (houses) {
		unsigned char src[6];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_PLAY_SOUND_AT_TILE);
		Net_Encode_uint8 (&buf, soundID);
		Net_Encode_uint16(&buf, position.x);
		Net_Encode_uint16(&buf, position.y);

		Server_BufferGameEvent(houses, sizeof(src), src);
	}
}

void
Server_Send_PlayVoice(enum HouseFlag houses, enum VoiceID voiceID)
{
	Server_Send_PlayVoiceAtTile(houses, voiceID, 0);
}

void
Server_Send_PlayVoiceAtTile(enum HouseFlag houses,
		enum VoiceID voiceID, uint16 packed)
{
	if (voiceID == VOICE_INVALID)
		return;

	if (houses & (1 << g_playerHouseID)) {
		Audio_PlayVoiceAtTile(voiceID, packed);
	}

	houses &= g_client_houses;
	if (houses) {
		unsigned char src[4];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_PLAY_VOICE);
		Net_Encode_uint8 (&buf, voiceID);
		Net_Encode_uint16(&buf, packed);

		Server_BufferGameEvent(houses, sizeof(src), src);
	}
}

void
Server_Send_PlayBattleMusic(enum HouseFlag houses)
{
	if (houses & (1 << g_playerHouseID)) {
		if (g_musicInBattle == 0)
			g_musicInBattle = 1;
	}

	houses &= g_client_houses;
	if (houses) {
		unsigned char src[1];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_PLAY_BATTLE_MUSIC);

		Server_BufferGameEvent(houses, sizeof(src), src);
	}
}

void
Server_Send_WinLose(enum HouseType houseID, bool win)
{
	if (g_host_type != HOSTTYPE_NONE) {
		char chat_log[MAX_CHAT_LEN + 1];

		snprintf(chat_log, sizeof(chat_log), "%s %s",
				Net_GetClientName(houseID),
				win ? "won" : "lost");

		Server_Recv_Chat(0, FLAG_HOUSE_ALL, chat_log);

		g_multiplayer.state[houseID] = (win ? MP_HOUSE_WON : MP_HOUSE_LOST);
		g_client_houses &= ~(1 << houseID);

		if (!win) {
			House_Server_ReassignToAI(houseID);
		}
	}

	if (houseID == g_playerHouseID) {
		MenuBar_DisplayWinLose(win);
		Server_ReturnToLobbyNow(win);
	} else {
		unsigned char src[2];
		unsigned char *buf = src;

		Net_Encode_ServerClientMsg(&buf, SCMSG_WIN_LOSE);
		Net_Encode_uint8(&buf, win ? 'W' : 'L');

		assert(buf - src == sizeof(src));
		Server_BufferGameEvent(1 << houseID, sizeof(src), src);
	}
}

void
Server_Send_ClientList(unsigned char **buf)
{
	if (!g_sendClientList)
		return;

	const size_t len = 1 + 1 + MAX_CLIENTS * 16;
	if (!Server_CanEncodeFixedWidthBuffer(buf, len))
		return;

	Net_Encode_ServerClientMsg(buf, SCMSG_CLIENT_LIST);

	unsigned char *buf_count = *buf; (*buf) += 1;
	uint8 count = 0;

	for (int i = 0; i < MAX_CLIENTS; i++) {
		const PeerData *data = &g_peer_data[i];
		if (data->id == 0)
			continue;

		const size_t name_len = strlen(data->name);

		Net_Encode_uint8(buf, data->id);
		Net_Encode_uint8(buf, name_len);
		memcpy(*buf, data->name, name_len);
		(*buf) += name_len;

		count++;
	}

	*buf_count = count;

	g_sendClientList = false;
}

void
Server_Send_Scenario(unsigned char **buf)
{
	if (!g_sendScenario || lobby_map_generator_mode != MAP_GENERATOR_STOP)
		return;

	const size_t len = 1 + 7 + MAX_CLIENTS;
	if (!Server_CanEncodeFixedWidthBuffer(buf, len))
		return;

	Net_Encode_ServerClientMsg(buf, SCMSG_SCENARIO);
	Net_Encode_uint16(buf, g_multiplayer.credits);
	Net_Encode_uint32(buf, g_multiplayer.next_seed);
	Net_Encode_uint32(buf, g_multiplayer.seed_mode);
	Net_Encode_uint32(buf, g_multiplayer.landscape_params.min_spice_fields);
	Net_Encode_uint32(buf, g_multiplayer.landscape_params.max_spice_fields);
	Net_Encode_uint8 (buf, enhancement_fog_of_war);
	Net_Encode_uint8 (buf, enhancement_insatiable_sandworms);

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		Net_Encode_uint8(buf, g_multiplayer.client[h]);
	}

	g_sendScenario = false;
}

/*--------------------------------------------------------------*/

static void
Server_ReturnToLobbyNow(bool win)
{
	/* XXX - assume two teams for now.  Ideally the server enters
	 * observer mode, or runs the game from the lobby, or both.
	 */
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_multiplayer.state[h] != MP_HOUSE_PLAYING)
			continue;

		if (House_AreAllied(g_playerHouseID, h)) {
			Server_Send_WinLose(h, win);
		} else {
			Server_Send_WinLose(h, !win);
		}
	}
}

void
Server_Recv_ReturnToLobby(enum HouseType houseID, bool log_message)
{
	PeerData *data = Net_GetPeerData(g_multiplayer.client[houseID]);
	data->state = CLIENTSTATE_IN_LOBBY;

	if (g_multiplayer.state[houseID] == MP_HOUSE_PLAYING) {
		g_multiplayer.state[houseID] = MP_HOUSE_LOST;
		g_client_houses &= ~(1 << houseID);
		House_Server_ReassignToAI(houseID);

		if (log_message) {
			char chat_log[MAX_CHAT_LEN + 1];

			snprintf(chat_log, sizeof(chat_log), "%s surrendered",
					Net_GetClientName(houseID));

			Server_Recv_Chat(0, FLAG_HOUSE_ALL, chat_log);
		}
	}

	if (houseID == g_playerHouseID)
		Server_ReturnToLobbyNow(false);
}

/*--------------------------------------------------------------*/

static bool
Server_PlayerCanControlStructure(enum HouseType houseID, const Structure *s)
{
	return (s->o.houseID == houseID
			&& s->o.flags.s.allocated
			&& s->o.flags.s.used);
}

static void
Server_Recv_RepairUpgradeStructure(enum HouseType houseID, const unsigned char *buf)
{
	const uint16 objectID = Net_Decode_ObjectIndex(&buf);

	SERVER_LOG("objectID=%d", objectID);

	if (objectID >= STRUCTURE_INDEX_MAX_SOFT)
		return;

	Structure *s = Structure_Get_ByIndex(objectID);
	if (!Server_PlayerCanControlStructure(houseID, s))
		return;

	if (!Structure_Server_SetRepairingState(s, -1))
		Structure_Server_SetUpgradingState(s, -1);
}

static void
Server_Recv_SetRallyPoint(enum HouseType houseID, const unsigned char *buf)
{
	const uint16 objectID = Net_Decode_ObjectIndex(&buf);
	const uint16 packed   = Net_Decode_uint16(&buf);

	SERVER_LOG("objectID=%d, packed=%d", objectID, packed);

	if (objectID >= STRUCTURE_INDEX_MAX_SOFT)
		return;

	Structure *s = Structure_Get_ByIndex(objectID);
	if (!Server_PlayerCanControlStructure(houseID, s)
			|| !Structure_SupportsRallyPoints(s->o.type))
		return;

	if (Tile_IsOutOfMap(packed)) {
		s->rallyPoint = 0xFFFF;
	} else {
		s->rallyPoint = packed;
	}
}

static void
Server_Recv_CancelItem(Structure *s)
{
	Structure_Server_CancelBuild(s);
	s->state = STRUCTURE_STATE_IDLE;

	/* Reset high-tech factory animation in case we have the
	 * open/close roof animations.
	 */
	if (s->o.type == STRUCTURE_HIGH_TECH)
		Structure_UpdateMap(s);
}

static void
Server_Recv_PurchaseItemStarport(Structure *s, uint8 objectType)
{
	if (objectType >= UNIT_MAX || g_starportAvailable[objectType] <= 0)
		return;

	const int credits = Random_Starport_CalculateUnitPrice(objectType);
	House *h = House_Get_ByIndex(s->o.houseID);
	if (h->credits <= credits)
		return;

	/* Attempt to create a unit. */
	Unit *u;
	g_validateStrictIfZero++;
	{
		tile32 tile;
		tile.x = 0xFFFF;
		tile.y = 0xFFFF;
		u = Unit_Create(UNIT_INDEX_INVALID, objectType, s->o.houseID, tile, 0);
	}
	g_validateStrictIfZero--;

	if (u == NULL) {
		Server_Send_StatusMessage1(1 << h->index, 2,
				STR_UNABLE_TO_CREATE_MORE);
		Server_Send_PlaySound(1 << h->index, EFFECT_ERROR_OCCURRED);
	} else {
		h->credits -= credits;
		u->o.linkedID = h->starportLinkedID & 0xFF;
		h->starportLinkedID = u->o.index;

		h->starportCount[objectType]++;
		BuildQueue_Add(&h->starportQueue, objectType, credits);

		Server_DepleteStarport(objectType);
	}
}

static void
Server_Recv_CancelItemStarport(Structure *s, uint8 objectType)
{
	if (g_starportAvailable[objectType] == 0)
		return;

	House *h = House_Get_ByIndex(s->o.houseID);

	int credits;
	if (BuildQueue_RemoveTail(&h->starportQueue, objectType, &credits)) {
		h->credits += credits;
		h->starportCount[objectType]--;
		Server_RestockStarport(objectType);

		/* We create units as soon as they are purchased (due to unit limits),
		 * so we need to free them too!
		 */
		uint8 *prev = NULL;
		uint16 unitID = h->starportLinkedID;
		while (unitID != 0xFF) {
			Unit *u = Unit_Get_ByIndex(unitID);

			if (u->o.type == objectType) {
				if (prev == NULL) {
					h->starportLinkedID
						= (u->o.linkedID == 0xFF)
						? UNIT_INDEX_INVALID : u->o.linkedID;
				} else {
					*prev = u->o.linkedID;
				}

				Unit_Free(u);
				break;
			}

			prev = &u->o.linkedID;
			unitID = u->o.linkedID;
		}
	}
}

static void
Server_Recv_SendStarportOrder(Structure *s)
{
	House *h = House_Get_ByIndex(s->o.houseID);

	if (BuildQueue_IsEmpty(&h->starportQueue))
		return;

	const uint16 delivery_time
		= g_table_houseInfo[h->index].starportDeliveryTime;

	if (h->starportTimeLeft == 0 || h->starportTimeLeft == delivery_time) {
		h->starportTimeLeft = delivery_time;
		memset(h->starportCount, 0, sizeof(h->starportCount));
		BuildQueue_Free(&h->starportQueue);
	}
}

static void
Server_Recv_PurchaseResumeItem(enum HouseType houseID, const unsigned char *buf)
{
	const uint16 objectID   = Net_Decode_ObjectIndex(&buf);
	const uint8  objectType = Net_Decode_uint8(&buf);

	SERVER_LOG("objectID=%d, objectType=%d", objectID, objectType);

	if (objectID >= STRUCTURE_INDEX_MAX_SOFT)
		return;

	Structure *s = Structure_Get_ByIndex(objectID);
	if (!Server_PlayerCanControlStructure(houseID, s))
		return;

	if (s->o.type == STRUCTURE_STARPORT) {
		Server_Recv_PurchaseItemStarport(s, objectType);
		return;
	}

	if (objectType == 0xFF) {
		Server_Recv_CancelItem(s);
	} else if ((s->objectType == objectType) && s->o.flags.s.onHold) {
		s->o.flags.s.repairing = false;
		s->o.flags.s.onHold    = false;
		s->o.flags.s.upgrading = false;
	} else {
		bool can_build = false;

		if (s->o.type == STRUCTURE_STARPORT) {
			can_build = false;
		} else if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
			can_build
				= (objectType < STRUCTURE_MAX)
				&& Structure_GetAvailable_ConstructionYard(s, objectType);
		} else if (objectType < UNIT_MAX) {
			const StructureInfo *si = &g_table_structureInfo[s->o.type];

			for (int i = 0; i < 8; i++) {
				if (si->buildableUnits[i] == objectType) {
					can_build = Structure_GetAvailable_Factory(s, i);
					break;
				}
			}
		}

		if (!can_build)
			return;

		if ((s->o.linkedID == 0xFF) && BuildQueue_IsEmpty(&s->queue)) {
			Structure_Server_BuildObject(s, objectType);
		} else {
			BuildQueue_Add(&s->queue, objectType, 0);
		}
	}
}

static void
Server_Recv_PauseCancelItem(enum HouseType houseID, const unsigned char *buf)
{
	const uint16 objectID   = Net_Decode_ObjectIndex(&buf);
	const uint8  objectType = Net_Decode_uint8(&buf);

	SERVER_LOG("objectID=%d, objectType=%d", objectID, objectType);

	if (objectID >= STRUCTURE_INDEX_MAX_SOFT)
		return;

	Structure *s = Structure_Get_ByIndex(objectID);
	if (!Server_PlayerCanControlStructure(houseID, s))
		return;

	if (s->o.type == STRUCTURE_STARPORT) {
		Server_Recv_CancelItemStarport(s, objectType);
	} else if (s->objectType == objectType && s->o.linkedID != 0xFF) {
		if (s->o.flags.s.onHold
				|| (s->o.type == STRUCTURE_CONSTRUCTION_YARD
					&& s->countDown == 0)) {
			const uint16 nextType = BuildQueue_RemoveHead(&s->queue);

			if (nextType == 0xFFFF) {
				Server_Recv_CancelItem(s);
			} else if (nextType != s->objectType) {
				Structure_Server_BuildObject(s, nextType);
			}
		} else {
			s->o.flags.s.onHold = true;
		}
	} else {
		BuildQueue_RemoveTail(&s->queue, objectType, NULL);
	}
}

static void
Server_Recv_EnterLeavePlacementMode(enum HouseType houseID, const unsigned char *buf)
{
	House *h = House_Get_ByIndex(houseID);
	const uint16 objectID = Net_Decode_ObjectIndex(&buf);

	SERVER_LOG("objectID=%d", objectID);

	if (objectID < STRUCTURE_INDEX_MAX_SOFT) {
		Structure *s = Structure_Get_ByIndex(objectID);
		if ((s->o.type != STRUCTURE_CONSTRUCTION_YARD)
				|| !Server_PlayerCanControlStructure(houseID, s))
			return;

		if (s->countDown == 0
				&& s->o.linkedID != STRUCTURE_INVALID
				&& h->structureActiveID == STRUCTURE_INDEX_INVALID) {
			h->constructionYardPosition = Tile_PackTile(s->o.position);
			h->structureActiveID = s->o.linkedID;
			s->o.linkedID = STRUCTURE_INVALID;
		} else if (s->o.linkedID == STRUCTURE_INVALID
				&& h->structureActiveID != STRUCTURE_INDEX_INVALID) {
		}
	} else if (h->structureActiveID != STRUCTURE_INDEX_INVALID) {
		/* Return the active structure back to the construction yard.
		 * If the construction yard was captured or destroyed during
		 * placement mode, and the placement was aborted, then destroy
		 * the structure.
		 */
		Structure *s = Structure_Get_ByPackedTile(h->constructionYardPosition);
		if (s != NULL
				&& s->o.type == STRUCTURE_CONSTRUCTION_YARD
				&& s->o.linkedID == STRUCTURE_INVALID
				&& Server_PlayerCanControlStructure(houseID, s)) {
			s->o.linkedID = h->structureActiveID;
		} else {
			Structure_Free(Structure_Get_ByIndex(h->structureActiveID));
		}

		h->structureActiveID = STRUCTURE_INDEX_INVALID;
	}
}

static void
Server_Recv_PlaceStructure(enum HouseType houseID, const unsigned char *buf)
{
	House *h = House_Get_ByIndex(houseID);
	const uint16 objectID = h->structureActiveID;
	const uint16 packed = Net_Decode_uint16(&buf);

	SERVER_LOG("objectID=%d, packed=%d", objectID, packed);

	if ((h->structureActiveID == STRUCTURE_INDEX_INVALID)
			|| Tile_IsOutOfMap(packed))
		return;

	Structure *s = Structure_Get_ByIndex(objectID);
	Viewport_Server_Place(h, s, packed);
}

static void
Server_Recv_ActivateStructureAbility(enum HouseType houseID, const unsigned char *buf)
{
	const uint16 objectID = Net_Decode_ObjectIndex(&buf);

	SERVER_LOG("objectID=%d", objectID);

	if (objectID >= STRUCTURE_INDEX_MAX_SOFT)
		return;

	Structure *s = Structure_Get_ByIndex(objectID);
	if (!Server_PlayerCanControlStructure(houseID, s))
		return;

	if (s->o.type == STRUCTURE_PALACE) {
		if (s->countDown == 0)
			Structure_Server_ActivateSpecial(s);
	} else if (s->o.type == STRUCTURE_STARPORT) {
		Server_Recv_SendStarportOrder(s);
	} else if (s->o.type == STRUCTURE_REPAIR) {
		if (s->o.linkedID != 0xFF)
			Structure_Server_SetState(s, STRUCTURE_STATE_READY);
	}
}

static void
Server_Recv_LaunchDeathhand(enum HouseType houseID, const unsigned char *buf)
{
	House *h = House_Get_ByIndex(houseID);
	const uint16 packed = Net_Decode_uint16(&buf);

	SERVER_LOG("packed=%d", packed);

	if (h->houseMissileID == UNIT_INDEX_INVALID)
		return;

	Unit_Server_LaunchHouseMissile(h, packed);
}

/*--------------------------------------------------------------*/

static bool
Server_PlayerCanControlUnit(enum HouseType houseID, const Unit *u)
{
	/* Note: u->o.flags.s.inTransport is badly named and isn't what you think. */
	return (Unit_GetHouseID(u) == houseID
			&& u->o.flags.s.used
			&& u->o.flags.s.allocated
			&& !u->o.flags.s.isNotOnMap);
}

static void
Server_Recv_IssueUnitActionUntargetted(Unit *u, enum UnitActionType actionID)
{
	const UnitInfo *ui = &g_table_unitInfo[u->o.type];

	if (ui->o.actionsPlayer[0] != actionID
	 && ui->o.actionsPlayer[1] != actionID
	 && ui->o.actionsPlayer[2] != actionID
	 && ui->o.actionsPlayer[3] != actionID) {
		return;
	}

	if (u->deviated != 0) {
		Unit_Deviation_Decrease(u, 5);
		if (u->deviated == 0)
			return;
	}

	if (g_table_actionInfo[actionID].selectionType == SELECTIONTYPE_TARGET) {
		u->deviationDecremented = true;
		return;
	} else {
		u->deviationDecremented = false;
	}

	Object_Script_Variable4_Clear(&u->o);
	u->targetAttack = 0;
	u->targetMove = 0;
	u->route[0] = 0xFF;

	Unit_Server_SetAction(u, actionID);
}

static void
Server_Recv_IssueUnitActionTargetted(Unit *u,
		enum UnitActionType actionID, uint16 encoded)
{
	const UnitInfo *ui = &g_table_unitInfo[u->o.type];

	actionID = Unit_GetSimilarAction(ui->o.actionsPlayer, actionID);

	/* ENHANCEMENT -- Targetted sabotage is ACTION_MOVE + detonateAtTarget. */
	bool detonateAtTarget = false;
	if (enhancement_targetted_sabotage && actionID == ACTION_SABOTAGE) {
		actionID = ACTION_MOVE;
		detonateAtTarget = true;
	} else if (actionID == ACTION_INVALID
			|| g_table_actionInfo[actionID].selectionType != SELECTIONTYPE_TARGET) {
		return;
	}

	/* Since the deviation counter is decremented when you press the
	 * command button, generic orders have a habit of by-passing that
	 * logic.
	 */
	if ((u->deviated != 0) && (!u->deviationDecremented)) {
		Unit_Deviation_Decrease(u, 5);
		if (u->deviated == 0)
			return;
	} else {
		u->deviationDecremented = false;
	}

	Object_Script_Variable4_Clear(&u->o);
	u->targetAttack = 0;
	u->targetMove = 0;
	u->route[0] = 0xFF;
	u->detonateAtTarget = detonateAtTarget;

	Unit_Server_SetAction(u, actionID);

	Unit *target = NULL;
	if (actionID == ACTION_MOVE) {
		Unit_SetDestination(u, encoded);

		if (enhancement_targetted_sabotage && u->detonateAtTarget) {
			target = Tools_Index_GetUnit(u->targetMove);
		} else if (enhancement_permanent_follow_mode) {
			u->permanentFollow = (Tools_Index_GetType(u->targetMove) == IT_UNIT);
		}
	} else if (actionID == ACTION_HARVEST) {
		u->targetMove = encoded;
	} else {
		Unit_SetTarget(u, encoded);
		target = Tools_Index_GetUnit(u->targetAttack);
	}

	if (target != NULL)
		target->blinkCounter = 8;
}

static void
Server_Recv_IssueUnitAction(enum HouseType houseID, const unsigned char *buf)
{
	const uint8  actionID = Net_Decode_uint8(&buf);
	const uint16 encoded  = Net_Decode_uint16(&buf);
	const uint16 objectID = Net_Decode_ObjectIndex(&buf);

	SERVER_LOG("actionID=%d, encoded=%x, objectID=%d",
			actionID, encoded, objectID);

	if ((actionID >= ACTION_MAX && actionID != ACTION_CANCEL)
			|| (objectID >= UNIT_INDEX_MAX))
		return;

	Unit *u = Unit_Get_ByIndex(objectID);
	if (!Server_PlayerCanControlUnit(houseID, u))
		return;

	if (actionID == ACTION_CANCEL) {
		u->deviationDecremented = false;
	} else if (Tools_Index_GetType(encoded) == IT_NONE) {
		Server_Recv_IssueUnitActionUntargetted(u, actionID);
	} else if (Tools_Index_IsValid_Defensive(encoded)) {
		Server_Recv_IssueUnitActionTargetted(u, actionID, encoded);
	}
}

void
Server_Recv_PrefName(int peerID, const char *name)
{
	PeerData *data = Net_GetPeerData(peerID);
	int len = MAX_NAME_LEN;

	while ((len > 0) && (*name != '\0')) {
		if (!isspace(*name))
			break;
		name++;
		len--;
	}

	if ((len > 0) && (*name != '\0')) {
		char new_name[MAX_NAME_LEN + 1];
		char chat_log[MAX_CHAT_LEN + 1];

		if (strncmp(name, data->name, sizeof(data->name)) == 0)
			return;

		snprintf(new_name, len + 1, "%s", name);

		bool retry = true;
		for (int attempts = 10; attempts > 0 && retry; attempts--) {
			retry = false;

			for (int i = 0; i < MAX_CLIENTS; i++) {
				const PeerData *other = &g_peer_data[i];
				if (other->id == 0 || other->id == peerID)
					continue;

				if (strncasecmp(other->name, new_name, sizeof(new_name)) == 0) {
					retry = true;
					break;
				}
			}

			if (retry) {
				len = strlen(new_name);

				for (; len > 0; len--) {
					if (!isdigit(new_name[len - 1]))
						break;
				}

				if (len == MAX_NAME_LEN)
					len--;

				int nth = atoi(new_name + len) + 1;
				snprintf(new_name + len, sizeof(new_name) - len, "%d", nth);
			}
		}

		/* Should not happen since we only have 6 clients. */
		if (retry)
			return;

		if (data->name[0] == '\0') {
			snprintf(chat_log, sizeof(chat_log), "%s joined",
					new_name);
		} else {
			snprintf(chat_log, sizeof(chat_log), "%s is now %s",
					data->name, new_name);
		}

		memcpy(data->name, new_name, sizeof(data->name));

		if (peerID == g_local_client_id)
			memcpy(g_net_name, new_name, sizeof(g_net_name));

		g_sendClientList = true;
		Server_Recv_Chat(0, FLAG_HOUSE_ALL, chat_log);
	}
}

void
Server_Recv_PrefHouse(int peerID, enum HouseType houseID)
{
	const PeerData *data = Net_GetPeerData(peerID);
	const enum HouseType old_house = Net_GetClientHouse(peerID);

	if (old_house == houseID)
		return;

	if (houseID < HOUSE_MAX) {
		if (!Multiplayer_IsHouseAvailable(houseID))
			return;

		g_multiplayer.client[houseID] = data->id;

		if (g_local_client_id != 0
		 && g_local_client_id == g_multiplayer.client[houseID]) {
			g_playerHouseID = houseID;
			g_playerHouse = House_Get_ByIndex(houseID);
		}
	}

	if (old_house != HOUSE_INVALID) {
		g_multiplayer.client[old_house] = 0;
	}

	lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_RAND;
}

static void
Server_Recv_PrefHouseBuf(int peerID, const unsigned char *buf)
{
	if (g_inGame)
		return;

	enum HouseType houseID = Net_Decode_uint8(&buf);

	Server_Recv_PrefHouse(peerID, houseID);
}

void
Server_ProcessMessage(int peerID, enum HouseType houseID,
		const unsigned char *buf, int count)
{
	while (count > 0) {
		const enum ClientServerMsg msg = Net_Decode_ClientServerMsg(buf[0]);
		const int len = Net_GetLength_ClientServerMsg(msg);

		buf++;
		count--;

		if ((msg >= CSMSG_MAX) || (count < len)) {
			SERVER_LOG("msg=%d, len=%d", msg, len);
			break;
		}

		switch (msg) {
			case CSMSG_DISCONNECT:
				assert(false);
				break;

			case CSMSG_RETURN_TO_LOBBY:
				Server_Recv_ReturnToLobby(houseID, true);
				break;

			case CSMSG_REPAIR_UPGRADE_STRUCTURE:
				Server_Recv_RepairUpgradeStructure(houseID, buf);
				break;

			case CSMSG_SET_RALLY_POINT:
				Server_Recv_SetRallyPoint(houseID, buf);
				break;

			case CSMSG_PURCHASE_RESUME_ITEM:
				Server_Recv_PurchaseResumeItem(houseID, buf);
				break;

			case CSMSG_PAUSE_CANCEL_ITEM:
				Server_Recv_PauseCancelItem(houseID, buf);
				break;

			case CSMSG_ENTER_LEAVE_PLACEMENT_MODE:
				Server_Recv_EnterLeavePlacementMode(houseID, buf);
				break;

			case CSMSG_PLACE_STRUCTURE:
				Server_Recv_PlaceStructure(houseID, buf);
				break;

			case CSMSG_ACTIVATE_STRUCTURE_ABILITY:
				Server_Recv_ActivateStructureAbility(houseID, buf);
				break;

			case CSMSG_LAUNCH_DEATHHAND:
				Server_Recv_LaunchDeathhand(houseID, buf);
				break;

			case CSMSG_ISSUE_UNIT_ACTION:
				Server_Recv_IssueUnitAction(houseID, buf);
				break;

			case CSMSG_PREFERRED_NAME:
				Server_Recv_PrefName(peerID, (const char *)buf);
				break;

			case CSMSG_PREFERRED_HOUSE:
				Server_Recv_PrefHouseBuf(peerID, buf);
				break;

			case CSMSG_CHAT:
				Server_Recv_Chat(peerID, buf[0], (const char *)buf + 1);
				break;

			case CSMSG_MAX:
			case CSMSG_INVALID:
				assert(false);
				break;
		}

		buf += len;
		count -= len;
	}
}

/*--------------------------------------------------------------*/

static void
Server_Console_Help(const char *msg)
{
	static const char *help[] = {
		"Commands",
		" /list",
		" /kick <id | name>",
		" /credits <N>",
		" /seed <N>",
		" /spice <min> <max>",
	};
	VARIABLE_NOT_USED(msg);

	for (unsigned int i = 0; i < lengthof(help); i++) {
		ChatBox_AddLog(CHATTYPE_CONSOLE, help[i]);
	}
}

static void
Server_Console_Kick(const char *msg)
{
	PeerData *data = NULL;
	int peerID = 0;

	if (msg[0] == '\0')
		return;

	if (sscanf(msg, "%d", &peerID) == 1) {
		data = Net_GetPeerData(peerID);
	} else {
		for (peerID = 0; peerID < MAX_CLIENTS; peerID++) {
			data = &g_peer_data[peerID];
			if (data->id == 0)
				continue;

			if (strcasecmp(msg, data->name) == 0)
				break;
		}
	}

	if (data != NULL && data->peer != NULL)
		Server_DisconnectClient(data);
}

static void
Server_Console_List(const char *msg)
{
	char chat_log[MAX_CHAT_LEN + 1];
	VARIABLE_NOT_USED(msg);

	for (int i = 0; i < MAX_CLIENTS; i++) {
		const PeerData *data = &g_peer_data[i];
		if (data->id == 0)
			continue;

		snprintf(chat_log, sizeof(chat_log), "%d: %s", data->id, data->name);
		ChatBox_AddLog(CHATTYPE_CONSOLE, chat_log);
	}
}

static void
Server_Console_Credits(const char *msg)
{
	char chat_log[MAX_CHAT_LEN + 1];
	unsigned int credits;

	if (sscanf(msg, "%u", &credits) == 1) {
		g_multiplayer.credits = clamp(1000, credits, 10000);
		lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_STOP;
	}

	snprintf(chat_log, sizeof(chat_log), "Set to %d credits",
			g_multiplayer.credits);

	Server_Recv_Chat(0, FLAG_HOUSE_ALL, chat_log);
}

static void
Server_Console_Seed(const char *msg)
{
	unsigned int seed;

	if (sscanf(msg, "%u", &seed) == 1) {
		g_multiplayer.test_seed = seed;
		lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_STOP;
	}
}

static void
Server_Console_Spice(const char *msg)
{
	char chat_log[MAX_CHAT_LEN + 1];
	unsigned int spice1;
	unsigned int spice2;
	int count;

	count = sscanf(msg, "%u %u", &spice1, &spice2);
	if (count == 1 || count == 2) {
		if (count == 1) {
			spice1 = min(spice1, 255);
			spice2 = spice1;
		} else {
			spice1 = min(spice1, 255);
			spice2 = min(spice2, 255);
		}

		g_multiplayer.landscape_params.min_spice_fields = min(spice1, spice2);
		g_multiplayer.landscape_params.max_spice_fields = max(spice1, spice2);
		lobby_map_generator_mode = MAP_GENERATOR_TRY_TEST_ELSE_STOP;
	}

	snprintf(chat_log, sizeof(chat_log), "Set to [%d..%d] spice",
			g_multiplayer.landscape_params.min_spice_fields,
			g_multiplayer.landscape_params.max_spice_fields);

	Server_Recv_Chat(0, FLAG_HOUSE_ALL, chat_log);
}

bool
Server_ProcessCommand(const char *msg)
{
	static const struct {
		const char *str;
		void (*proc)(const char *msg);
	} command[] = {
		{ "/help",      Server_Console_Help },
		{ "/list",      Server_Console_List },
		{ "/kick",      Server_Console_Kick },

		/* Lobby only commands below this point. */
		{ NULL,         NULL },
		{ "/credits",   Server_Console_Credits },
		{ "/seed",      Server_Console_Seed },
		{ "/spice",     Server_Console_Spice },
	};

	for (unsigned int i = 0; i < lengthof(command); i++) {
		if (command[i].str == NULL) {
			if (g_inGame)
				return false;

			continue;
		}

		const size_t len = strlen(command[i].str);

		if (strncmp(msg, command[i].str, len) == 0
				&& (msg[len] == '\0' || msg[len] == ' ')) {

			msg = msg + len;
			while (*msg != '\0' && isspace(*msg))
				msg++;

			command[i].proc(msg);
			return true;
		}
	}

	return false;
}
