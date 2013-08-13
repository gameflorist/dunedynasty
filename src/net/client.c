/* client.c */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "client.h"

#include "message.h"
#include "net.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../newui/actionpanel.h"
#include "../object.h"
#include "../opendune.h"
#include "../pool/pool.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../structure.h"

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

/*--------------------------------------------------------------*/

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
