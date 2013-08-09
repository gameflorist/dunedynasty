/* server.c */

#include <assert.h>
#include <stdio.h>
#include "enum_string.h"

#include "server.h"

#include "message.h"
#include "../audio/audio.h"
#include "../enhancement.h"
#include "../gui/gui.h"
#include "../house.h"
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
#include "../unit.h"

#if 0
#define SERVER_LOG(FORMAT,...)	\
	do { fprintf(stderr, "%s:%d " FORMAT "\n", __FUNCTION__, __LINE__, __VA_ARGS__); } while (false)
#else
#define SERVER_LOG(...)
#endif

/*--------------------------------------------------------------*/

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
Server_ProcessMessages(void)
{
	const unsigned char *buf = g_client2server_message_buf;
	int count = g_client2server_message_len;

	enum HouseType houseID = g_playerHouseID;

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

	g_client2server_message_len = 0;
}
