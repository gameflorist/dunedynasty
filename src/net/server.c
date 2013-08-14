/* server.c */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "enum_string.h"

#include "server.h"

#include "message.h"
#include "net.h"
#include "../audio/audio.h"
#include "../enhancement.h"
#include "../newui/actionpanel.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../map.h"
#include "../newui/viewport.h"
#include "../opendune.h"
#include "../pool/house.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../shape.h"
#include "../string.h"
#include "../structure.h"
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

static Tile s_mapCopy[MAP_SIZE_MAX * MAP_SIZE_MAX];

/*--------------------------------------------------------------*/

void
Server_ResetCache(void)
{
	memset(g_server_broadcast_message_buf, 0, MAX_SERVER_BROADCAST_MESSAGE_LEN);

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		memset(g_server2client_message_buf[h], 0, MAX_SERVER_TO_CLIENT_MESSAGE_LEN);
		g_server2client_message_len[h] = 0;
	}

	memset(s_mapCopy, 0, sizeof(s_mapCopy));
}

/*--------------------------------------------------------------*/

void
Server_Send_UpdateLandscape(unsigned char **buf)
{
	const unsigned char * const end
		= g_server_broadcast_message_buf + MAX_SERVER_BROADCAST_MESSAGE_LEN;

	const int header_len  = 1 + 2;
	const int element_len = 2 + sizeof(Tile);

	if (*buf + header_len + element_len >= end)
		return;

	Net_Encode_ServerClientMsg(buf, SCMSG_UPDATE_LANDSCAPE);

	unsigned char *buf_count = *buf;
	uint16 count = 0;

	(*buf) += 2; /* count */

	for (uint16 packed = 0; packed < MAP_SIZE_MAX * MAP_SIZE_MAX; packed++) {
		if (*buf + element_len >= end)
			break;

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

	if (count == 0) {
		*buf = buf_count - 1;
	}
	else {
		SERVER_LOG("tiles changed=%d, %lu bytes",
				count, *buf - buf_count + 1);

		Net_Encode_uint16(&buf_count, count);
	}
}

void
Server_Send_StatusMessage1(enum HouseFlag houses, uint8 priority,
		uint16 str)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str, STR_NULL, STR_NULL);
	}
}

void
Server_Send_StatusMessage2(enum HouseFlag houses, uint8 priority,
		uint16 str1, uint16 str2)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str1, str2, STR_NULL);
	}
}

void
Server_Send_StatusMessage3(enum HouseFlag houses, uint8 priority,
		uint16 str1, uint16 str2, uint16 str3)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str1, str2, str3);
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
}

void
Server_Send_PlayVoice(enum HouseFlag houses, enum VoiceID voiceID)
{
	if (voiceID == VOICE_INVALID)
		return;

	if (houses & (1 << g_playerHouseID)) {
		Audio_PlayVoice(voiceID);
	}
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
	}
	else {
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
	}
	else {
		h->credits -= credits;
		u->o.linkedID = h->starportLinkedID & 0xFF;
		h->starportLinkedID = u->o.index;

		h->starportCount[objectType]++;
		BuildQueue_Add(&h->starportQueue, objectType, credits);

		g_starportAvailable[objectType]--;
		if (g_starportAvailable[objectType] <= 0)
			g_starportAvailable[objectType] = -1;
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
		Structure_Server_RestockStarport(objectType);

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
				}
				else {
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

	if (h->starportTimeLeft == 0) {
		h->starportTimeLeft = g_table_houseInfo[h->index].starportDeliveryTime;
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
	}
	else if ((s->objectType == objectType) && s->o.flags.s.onHold) {
		s->o.flags.s.repairing = false;
		s->o.flags.s.onHold    = false;
		s->o.flags.s.upgrading = false;
	}
	else {
		bool can_build = false;

		if (s->o.type == STRUCTURE_STARPORT) {
			can_build = false;
		}
		else if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
			can_build
				= (objectType < STRUCTURE_MAX)
				&& Structure_GetAvailable_ConstructionYard(s, objectType);
		}
		else if (objectType < UNIT_MAX) {
			const StructureInfo *si = &g_table_structureInfo[s->o.type];

			for (int i = 0; i < 8; i++) {
				if (si->buildableUnits[i] == objectType) {
					can_build = Structure_GetAvailable_Factory(s, i);
					break;
				}
			}
		}

		if (can_build) {
			Structure_Server_BuildObject(s, objectType);
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
	}
	else if (s->objectType == objectType && s->o.linkedID != 0xFF) {
		if (s->o.flags.s.onHold) {
			Server_Recv_CancelItem(s);
		}
		else {
			s->o.flags.s.onHold = true;
		}
	}
}

static void
Server_Recv_EnterLeavePlacementMode(enum HouseType houseID, const unsigned char *buf)
{
	House *h = House_Get_ByIndex(houseID);
	const uint16 objectID = Net_Decode_ObjectIndex(&buf);

	SERVER_LOG("objectID=%d", objectID);

	/* If the construction yard was destroyed during placement mode,
	 * and the placement was aborted, then destroy the structure.
	 */
	if (objectID == STRUCTURE_INDEX_INVALID) {
		if (h->structureActiveID == STRUCTURE_INDEX_INVALID)
			return;

		Structure *s = Structure_Get_ByIndex(h->structureActiveID);
		if (s != NULL)
			Structure_Free(s);

		h->structureActiveID = STRUCTURE_INDEX_INVALID;
	}
	else if (objectID < STRUCTURE_INDEX_MAX_SOFT) {
		Structure *s = Structure_Get_ByIndex(objectID);
		if ((s->o.type != STRUCTURE_CONSTRUCTION_YARD)
				|| !Server_PlayerCanControlStructure(houseID, s))
			return;

		if (s->countDown == 0
				&& s->o.linkedID != STRUCTURE_INVALID
				&& h->structureActiveID == STRUCTURE_INDEX_INVALID) {
			h->structureActiveID = s->o.linkedID;
			s->o.linkedID = STRUCTURE_INVALID;
		}
		else if (s->o.linkedID == STRUCTURE_INVALID
				&& h->structureActiveID != STRUCTURE_INDEX_INVALID) {
			s->o.linkedID = h->structureActiveID;
			h->structureActiveID = STRUCTURE_INDEX_INVALID;
		}
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
	}
	else if (s->o.type == STRUCTURE_STARPORT) {
		Server_Recv_SendStarportOrder(s);
	}
	else if (s->o.type == STRUCTURE_REPAIR) {
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
	}
	else {
		u->deviationDecremented = false;
	}

	Object_Script_Variable4_Clear(&u->o);
	u->targetAttack = 0;
	u->targetMove = 0;
	u->route[0] = 0xFF;

	Unit_SetAction(u, actionID);
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
	}
	else if (actionID == ACTION_INVALID
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
	}
	else {
		u->deviationDecremented = false;
	}

	Object_Script_Variable4_Clear(&u->o);
	u->targetAttack = 0;
	u->targetMove = 0;
	u->route[0] = 0xFF;
	u->detonateAtTarget = detonateAtTarget;

	Unit_SetAction(u, actionID);

	Unit *target = NULL;
	if (actionID == ACTION_MOVE) {
		Unit_SetDestination(u, encoded);

		if (enhancement_targetted_sabotage && u->detonateAtTarget) {
			target = Tools_Index_GetUnit(u->targetMove);
		}
		else if (enhancement_permanent_follow_mode) {
			u->permanentFollow = (Tools_Index_GetType(u->targetMove) == IT_UNIT);
		}
	}
	else if (actionID == ACTION_HARVEST) {
		u->targetMove = encoded;
	}
	else {
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
	}
	else if (Tools_Index_GetType(encoded) == IT_NONE) {
		Server_Recv_IssueUnitActionUntargetted(u, actionID);
	}
	else if (Tools_Index_IsValid_Defensive(encoded)) {
		Server_Recv_IssueUnitActionTargetted(u, actionID, encoded);
	}
}

void
Server_ProcessMessage(enum HouseType houseID,
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

			case CSMSG_MAX:
			case CSMSG_INVALID:
				assert(false);
				break;
		}

		buf += len;
		count -= len;
	}
}
