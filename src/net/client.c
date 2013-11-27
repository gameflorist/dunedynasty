/* client.c */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../os/math.h"

#include "client.h"

#include "message.h"
#include "net.h"
#include "../audio/audio.h"
#include "../enhancement.h"
#include "../explosion.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../map.h"
#include "../mods/multiplayer.h"
#include "../newui/actionpanel.h"
#include "../newui/chatbox.h"
#include "../newui/menu.h"
#include "../object.h"
#include "../opendune.h"
#include "../pool/house.h"
#include "../pool/pool.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../structure.h"
#include "../tools/random_starport.h"

#if 0
#define CLIENT_LOG(FORMAT,...)	\
	do { fprintf(stderr, "%s:%d " FORMAT "\n", __FUNCTION__, __LINE__, __VA_ARGS__); } while (false)
#else
#define CLIENT_LOG(...)
#endif

/*--------------------------------------------------------------*/

void
Client_ResetCache(void)
{
	memset(g_client2server_message_buf, 0, MAX_CLIENT_MESSAGE_LEN);
	g_client2server_message_len = 0;
}

/*--------------------------------------------------------------*/

static unsigned char *
Client_GetBuffer(enum ClientServerMsg msg)
{
	assert(msg < CSMSG_MAX);

	int len = 1 + Net_GetLength_ClientServerMsg(msg);
	if (g_client2server_message_len + len >= MAX_CLIENT_MESSAGE_LEN)
		return NULL;

	unsigned char *buf = g_client2server_message_buf + g_client2server_message_len;
	g_client2server_message_len += len;

	Net_Encode_ClientServerMsg(&buf, msg);
	return buf;
}

static void
Client_Send_ObjectIndex(enum ClientServerMsg msg, const Object *o)
{
	unsigned char *buf = Client_GetBuffer(msg);
	if (buf == NULL)
		return;

	Net_Encode_ObjectIndex(&buf, o);
}

/*--------------------------------------------------------------*/

void
Client_Send_RepairUpgradeStructure(const Object *o)
{
	Client_Send_ObjectIndex(CSMSG_REPAIR_UPGRADE_STRUCTURE, o);
}

void
Client_Send_SetRallyPoint(const Object *o, uint16 packed)
{
	unsigned char *buf = Client_GetBuffer(CSMSG_SET_RALLY_POINT);
	if (buf == NULL)
		return;

	Net_Encode_ObjectIndex(&buf, o);
	Net_Encode_uint16(&buf, packed);
}

void
Client_Send_PurchaseResumeItem(const Object *o, uint8 objectType)
{
	unsigned char *buf = Client_GetBuffer(CSMSG_PURCHASE_RESUME_ITEM);
	if (buf == NULL)
		return;

	Net_Encode_ObjectIndex(&buf, o);
	Net_Encode_uint8(&buf, objectType);
}

void
Client_Send_PauseCancelItem(const Object *o, uint8 objectType)
{
	unsigned char *buf = Client_GetBuffer(CSMSG_PAUSE_CANCEL_ITEM);
	if (buf == NULL)
		return;

	Net_Encode_ObjectIndex(&buf, o);
	Net_Encode_uint8(&buf, objectType);
}

void
Client_Send_EnterPlacementMode(const Object *o)
{
	Client_Send_ObjectIndex(CSMSG_ENTER_LEAVE_PLACEMENT_MODE, o);
}

void
Client_Send_LeavePlacementMode(const Object *o)
{
	unsigned char *buf = Client_GetBuffer(CSMSG_ENTER_LEAVE_PLACEMENT_MODE);
	if (buf == NULL)
		return;

	if (o != NULL) {
		Net_Encode_ObjectIndex(&buf, o);
	}
	else {
		Net_Encode_uint16(&buf, STRUCTURE_INDEX_INVALID);
	}
}

void
Client_Send_PlaceStructure(uint16 packed)
{
	unsigned char *buf = Client_GetBuffer(CSMSG_PLACE_STRUCTURE);
	if (buf == NULL)
		return;

	Net_Encode_uint16(&buf, packed);
}

void
Client_Send_SendStarportOrder(const struct Object *o)
{
	Client_Send_ObjectIndex(CSMSG_ACTIVATE_STRUCTURE_ABILITY, o);
}

void
Client_Send_ActivateSuperweapon(const struct Object *o)
{
	Client_Send_ObjectIndex(CSMSG_ACTIVATE_STRUCTURE_ABILITY, o);
}

void
Client_Send_LaunchDeathhand(uint16 packed)
{
	unsigned char *buf = Client_GetBuffer(CSMSG_LAUNCH_DEATHHAND);
	if (buf == NULL)
		return;

	Net_Encode_uint16(&buf, packed);
}

void
Client_Send_EjectRepairFacility(const Object *o)
{
	Client_Send_ObjectIndex(CSMSG_ACTIVATE_STRUCTURE_ABILITY, o);
}

void
Client_Send_IssueUnitAction(uint8 actionID, uint16 encoded, const Object *o)
{
	unsigned char *buf = Client_GetBuffer(CSMSG_ISSUE_UNIT_ACTION);
	if (buf == NULL)
		return;

	Net_Encode_uint8 (&buf, actionID);
	Net_Encode_uint16(&buf, encoded);
	Net_Encode_ObjectIndex(&buf, o);
}

void
Client_Send_BuildQueue(void)
{
	if (g_host_type == HOSTTYPE_DEDICATED_SERVER)
		return;

	PoolFindStruct find;

	/* For each structure, find any with non-empty build queues */
	find.houseID = g_playerHouseID;
	find.type    = 0xFFFF;
	find.index   = 0xFFFF;

	Structure *s;
	while ((s = Structure_Find(&find)) != NULL) {
		const bool start_next = (s->objectType == 0xFFFF) && (s->o.linkedID == 0xFF);
		if (!start_next)
			continue;

		if (s->o.type == STRUCTURE_STARPORT) {
			continue;
		}

		/* Wait until the active structure is placed before a
		 * construction yard begins producing the next structure.
		 * Note: if you have multiple construction yards, only one
		 * should have linkedID=0xFF and a non-empty build queue.
		 */
		if (s->o.type == STRUCTURE_CONSTRUCTION_YARD
				&& g_playerHouse->structureActiveID != STRUCTURE_INDEX_INVALID) {
			continue;
		}

		const uint16 objectType = BuildQueue_RemoveHead(&s->queue);
		if (objectType != 0xFFFF)
			Client_Send_PurchaseResumeItem(&s->o, objectType);
	}
}

bool
Client_Send_PrefName(const char *name)
{
	if (g_host_type == HOSTTYPE_DEDICATED_SERVER)
		return false;

	size_t len = MAX_NAME_LEN;
	while (isspace(*name)) {
		name++;
		len--;
	}

	if (*name == '\0')
		return false;

	unsigned char *buf = Client_GetBuffer(CSMSG_PREFERRED_NAME);
	if (buf == NULL)
		return false;

	memcpy(buf, name, len);
	memset(buf + len, 0, MAX_NAME_LEN + 1 - len);

	return true;
}

void
Client_Send_PrefHouse(enum HouseType houseID)
{
	if (g_host_type == HOSTTYPE_DEDICATED_SERVER)
		return;

	unsigned char *buf = Client_GetBuffer(CSMSG_PREFERRED_HOUSE);

	Net_Encode_uint8(&buf, houseID);
}

void
Client_Send_Chat(const char *msg)
{
	const size_t len = strlen(msg) + 1;

	if (len <= 0 || len >= MAX_CHAT_LEN)
		return;

	unsigned char *buf = Client_GetBuffer(CSMSG_CHAT);

	Net_Encode_uint8(&buf, FLAG_HOUSE_ALL);
	memcpy(buf, msg, len);
}

/*--------------------------------------------------------------*/

static void
Client_Recv_UpdateLandscape(const unsigned char **buf)
{
	const int count = Net_Decode_uint16(buf);

	for (int i = 0; i < count; i++) {
		const uint16 packed = Net_Decode_uint16(buf);
		const Tile *s = (const Tile *)(*buf);
		Tile *t = &g_map[packed];

		t->groundSpriteID   = s->groundSpriteID;
		t->overlaySpriteID  = s->overlaySpriteID;
		t->houseID          = s->houseID;
		t->hasUnit          = s->hasUnit;
		t->hasStructure     = s->hasStructure;
		t->index            = s->index;

		(*buf) += sizeof(Tile);
	}
}

static void
Client_Recv_UpdateFogOfWar(const unsigned char **buf)
{
	const int count = Net_Decode_uint16(buf);

	for (int i = 0; i < count; i++) {
		const uint16 encoded = Net_Decode_uint16(buf);
		const uint16 packed  = encoded & 0x3FFF;

		const enum TileUnveilCause cause
			= (encoded & 0x8000) ? UNVEILCAUSE_SHORT : UNVEILCAUSE_LONG;

		Map_UnveilTile(g_playerHouseID, cause, packed);
	}
}

static void
Client_Recv_UpdateHouse(const unsigned char **buf)
{
	House *h = g_playerHouse;
	const uint8 oldMissileCountdown = h->houseMissileCountdown;

	h->structuresBuilt      = Net_Decode_uint32(buf);
	h->credits              = Net_Decode_uint16(buf);
	h->creditsStorage       = Net_Decode_uint16(buf);
	h->powerProduction      = Net_Decode_uint16(buf);
	h->powerUsage           = Net_Decode_uint16(buf);
	h->windtrapCount        = Net_Decode_uint16(buf);
	h->starportTimeLeft     = Net_Decode_uint16(buf);
	h->starportLinkedID     = Net_Decode_uint16(buf);
	h->structureActiveID    = Net_Decode_uint16(buf);
	h->houseMissileID       = Net_Decode_uint16(buf);
	h->houseMissileCountdown= Net_Decode_uint8 (buf);

	for (enum UnitType u = UNIT_CARRYALL; u <= UNIT_MCV; u++) {
		h->starportCount[u] = Net_Decode_uint8(buf);
	}

	House_Client_UpdateRadarState();

	if ((h->houseMissileCountdown != oldMissileCountdown)
	 && (h->houseMissileCountdown > 0)) {
		House_Client_TickMissileCountdown();
	}
}

static void
Client_Recv_UpdateCHOAM(const unsigned char **buf)
{
	const uint16 seed = Net_Decode_uint16(buf);

	for (enum UnitType u = UNIT_CARRYALL; u <= UNIT_MCV; u++) {
		g_starportAvailable[u] = (int8)Net_Decode_uint8(buf);
	}

	Random_Starport_Seed(seed);
	g_factoryWindowTotal = -1;
}

static void
Client_Recv_UpdateStructures(const unsigned char **buf)
{
	const int count = Net_Decode_uint8(buf);

	for (int i = 0; i < count; i++) {
		const uint16 index = Net_Decode_ObjectIndex(buf);
		Structure *s = Structure_Get_ByIndex(index);
		Object *o = &s->o;

		o->index        = index;
		o->type         = Net_Decode_uint8 (buf);
		o->linkedID     = Net_Decode_uint8 (buf);
		o->flags.all    = Net_Decode_uint32(buf);
		o->houseID      = Net_Decode_uint8 (buf);
		o->position.x   = Net_Decode_uint16(buf);
		o->position.y   = Net_Decode_uint16(buf);
		o->hitpoints    = Net_Decode_uint16(buf);

		s->creatorHouseID       = Net_Decode_uint8 (buf);
		s->rotationSpriteDiff   = Net_Decode_uint16(buf);
		s->objectType           = Net_Decode_uint8 (buf);
		s->upgradeLevel         = Net_Decode_uint8 (buf);
		s->upgradeTimeLeft      = Net_Decode_uint8 (buf);
		s->countDown            = Net_Decode_uint16(buf);
		s->rallyPoint           = Net_Decode_uint16(buf);

		if (s->objectType == 0xFF)
			s->objectType = 0xFFFF;
	}

	Structure_Recount();
}

static void
Client_Recv_UpdateUnits(const unsigned char **buf)
{
	const int count = Net_Decode_uint8(buf);
	bool recount = false;

	for (int i = 0; i < count; i++) {
		const uint16 index = Net_Decode_ObjectIndex(buf);
		Unit *u = Unit_Get_ByIndex(index);
		Object *o = &u->o;
		const bool was_used = o->flags.s.used;

		o->index        = index;
		o->type         = Net_Decode_uint8 (buf);
		o->flags.all    = Net_Decode_uint32(buf);
		o->houseID      = Net_Decode_uint8 (buf);
		o->position.x   = Net_Decode_uint16(buf);
		o->position.y   = Net_Decode_uint16(buf);
		o->hitpoints    = Net_Decode_uint16(buf);

		u->actionID     = Net_Decode_uint8(buf);
		u->nextActionID = Net_Decode_uint8(buf);
		u->amount       = Net_Decode_uint8(buf);
		u->deviated     = Net_Decode_uint8(buf);
		u->deviatedHouse= Net_Decode_uint8(buf);
		u->orientation[0].current   = Net_Decode_uint8(buf);
		u->orientation[1].current   = Net_Decode_uint8(buf);
		u->wobbleIndex  = Net_Decode_uint8(buf);
		u->spriteOffset = Net_Decode_uint8(buf);

		/* XXX -- Smooth animation not yet implemented. */
		u->lastPosition = o->position;

		recount = recount || (was_used != o->flags.s.used);
	}

	if (recount)
		Unit_Recount();
}

static void
Client_Recv_UpdateExplosions(const unsigned char **buf)
{
	const int count = Net_Decode_uint8(buf);
	Explosion_Set_NumActive(count);

	for (int i = 1; i < count; i++) {
		Explosion *e = Explosion_Get_ByIndex(i);

		e->spriteID     = Net_Decode_uint16(buf);
		e->position.x   = Net_Decode_uint16(buf);
		e->position.y   = Net_Decode_uint16(buf);
	}
}

static void
Client_Recv_ScreenShake(const unsigned char **buf)
{
	const uint16 packed = Net_Decode_uint16(buf);

	GFX_ScreenShake_Start(packed, 1);
}

static void
Client_Recv_StatusMessage(const unsigned char **buf)
{
	const uint8  priority   = Net_Decode_uint8 (buf);
	const uint16 str1       = Net_Decode_uint16(buf);
	const uint16 str2       = Net_Decode_uint16(buf);
	const uint16 str3       = Net_Decode_uint16(buf);

	GUI_DrawStatusBarTextWrapper(priority, str1, str2, str3);
}

static void
Client_Recv_PlaySound(const unsigned char **buf)
{
	const uint8 soundID = Net_Decode_uint8(buf);

	Audio_PlaySound(soundID);
}

static void
Client_Recv_PlaySoundAtTile(const unsigned char **buf)
{
	const uint8 soundID = Net_Decode_uint8(buf);
	tile32 position;

	position.x = Net_Decode_uint16(buf);
	position.y = Net_Decode_uint16(buf);

	Audio_PlaySoundAtTile(soundID, position);
}

static void
Client_Recv_PlayVoice(const unsigned char **buf)
{
	const uint8 voiceID = Net_Decode_uint8(buf);

	Audio_PlayVoice(voiceID);
}

static void
Client_Recv_Identity(const unsigned char **buf)
{
	const uint8 clientID = Net_Decode_uint8(buf);

	if (g_local_client_id == 0 && clientID > 0)
		g_local_client_id = clientID;
}

static void
Client_Recv_ClientList(const unsigned char **buf)
{
	const uint8 count = Net_Decode_uint8(buf);
	int i;

	for (i = 0; i < count; i++) {
		const uint8 peerID = Net_Decode_uint8(buf);
		const size_t len = Net_Decode_uint8(buf);

		if (i >= MAX_CLIENTS)
			continue;

		PeerData *data = &g_peer_data[i];

		data->id = peerID;
		snprintf(data->name, min(sizeof(data->name), len + 1), "%s", *buf);

		if (peerID == g_local_client_id)
			strncpy(g_net_name, data->name, sizeof(g_net_name));

		(*buf) += len;
	}

	for (; i < MAX_CLIENTS; i++) {
		PeerData *data = &g_peer_data[i];
		data->id = 0;
	}

	enum HouseFlag houses = 0;
	for (i = 0; i < count; i++) {
		if (i >= MAX_CLIENTS)
			break;

		const PeerData *data = &g_peer_data[i];
		const enum HouseType houseID = Net_GetClientHouse(data->id);
		if (houseID != HOUSE_INVALID)
			houses |= (1 << houseID);
	}

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (houses & (1 << h))
			continue;

		g_multiplayer.client[h] = 0;
	}
}

static void
Client_Recv_Scenario(const unsigned char **buf)
{
	const uint32 old_seed = g_multiplayer.seed;

	g_multiplayer.seed = Net_Decode_uint32(buf);
	enhancement_fog_of_war = Net_Decode_uint8(buf);

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		g_multiplayer.client[h] = Net_Decode_uint8(buf);

		if (g_local_client_id != 0
		 && g_local_client_id == g_multiplayer.client[h]) {
			g_playerHouseID = h;
			g_playerHouse = House_Get_ByIndex(h);
		}
	}

	Multiplayer_GenerateMap(false);

	if (g_multiplayer.seed != old_seed)
		Lobby_ResetRadarAnimation();
}

static void
Client_Recv_Chat(const unsigned char **buf)
{
	const char *name = NULL;
	char msg[MAX_CHAT_LEN + 1];

	const uint8 peerID = Net_Decode_uint8(buf);
	const int msg_len = snprintf(msg, sizeof(msg), "%s", *buf);
	(*buf) += msg_len;

	const PeerData *data = Net_GetPeerData(peerID);
	if (data != NULL)
		name = data->name;

	ChatBox_AddEntry(name, msg);
}

void
Client_ChangeSelectionMode(void)
{
	static bool l_houseMissileWasActive; /* XXX */

	if ((g_playerHouse->structureActiveID != STRUCTURE_INDEX_INVALID)
			&& (g_structureActive == NULL)) {
		ActionPanel_BeginPlacementMode();
		return;
	}
	else if ((g_playerHouse->structureActiveID == STRUCTURE_INDEX_INVALID)
			&& (g_selectionType == SELECTIONTYPE_PLACE)) {
		g_structureActive = NULL;
		g_structureActiveType = 0xFFFF;

		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
		g_selectionState = 0; /* Invalid. */
		return;
	}

	if ((g_playerHouse->houseMissileID != UNIT_INDEX_INVALID)
			&& (g_selectionType != SELECTIONTYPE_TARGET)) {
		l_houseMissileWasActive = true;
		GUI_ChangeSelectionType(SELECTIONTYPE_TARGET);
		return;
	}
	else if ((g_playerHouse->houseMissileID == UNIT_INDEX_INVALID)
			&& (l_houseMissileWasActive)) {
		l_houseMissileWasActive = false;
		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
		return;
	}
}

enum NetEvent
Client_ProcessMessage(const unsigned char *buf, int count)
{
	enum NetEvent ret = NETEVENT_NORMAL;

	while (count > 0) {
		const enum ServerClientMsg msg = Net_Decode_ServerClientMsg(buf[0]);
		const unsigned char * const buf0 = buf+1;

		buf++;
		count--;

		switch (msg) {
			case SCMSG_DISCONNECT:
				break;

			case SCMSG_UPDATE_LANDSCAPE:
				Client_Recv_UpdateLandscape(&buf);
				break;

			case SCMSG_UPDATE_FOG_OF_WAR:
				Client_Recv_UpdateFogOfWar(&buf);
				break;

			case SCMSG_UPDATE_HOUSE:
				Client_Recv_UpdateHouse(&buf);
				Client_ChangeSelectionMode();
				break;

			case SCMSG_UPDATE_CHOAM:
				Client_Recv_UpdateCHOAM(&buf);
				break;

			case SCMSG_UPDATE_STRUCTURES:
				Client_Recv_UpdateStructures(&buf);
				break;

			case SCMSG_UPDATE_UNITS:
				Client_Recv_UpdateUnits(&buf);
				break;

			case SCMSG_UPDATE_EXPLOSIONS:
				Client_Recv_UpdateExplosions(&buf);
				break;

			case SCMSG_SCREEN_SHAKE:
				Client_Recv_ScreenShake(&buf);
				break;

			case SCMSG_STATUS_MESSAGE:
				Client_Recv_StatusMessage(&buf);
				break;

			case SCMSG_PLAY_SOUND:
				Client_Recv_PlaySound(&buf);
				break;

			case SCMSG_PLAY_SOUND_AT_TILE:
				Client_Recv_PlaySoundAtTile(&buf);
				break;

			case SCMSG_PLAY_VOICE:
				Client_Recv_PlayVoice(&buf);
				break;

			case SCMSG_PLAY_BATTLE_MUSIC:
				if (g_musicInBattle == 0)
					g_musicInBattle = 1;
				break;

			case SCMSG_IDENTITY:
				Client_Recv_Identity(&buf);
				break;

			case SCMSG_CLIENT_LIST:
				Client_Recv_ClientList(&buf);
				break;

			case SCMSG_SCENARIO:
				Client_Recv_Scenario(&buf);
				break;

			case SCMSG_START_GAME:
				ret = NETEVENT_START_GAME;
				break;

			case SCMSG_CHAT:
				Client_Recv_Chat(&buf);
				break;

			case SCMSG_MAX:
			case SCMSG_INVALID:
			default:
				break;
		}

		count -= (buf - buf0);
	}

	return ret;
}
