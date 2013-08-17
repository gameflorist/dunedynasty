/* client.c */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "client.h"

#include "message.h"
#include "net.h"
#include "../explosion.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../map.h"
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

		if (!t->isUnveiled && s->isUnveiled)
			Map_UnveilTile(packed, g_playerHouseID);

		(*buf) += sizeof(Tile);
	}
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

void
Client_ProcessMessage(const unsigned char *buf, int count)
{
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

			case SCMSG_UPDATE_STRUCTURES:
				Client_Recv_UpdateStructures(&buf);
				break;

			case SCMSG_UPDATE_UNITS:
				Client_Recv_UpdateUnits(&buf);
				break;

			case SCMSG_UPDATE_EXPLOSIONS:
				Client_Recv_UpdateExplosions(&buf);
				break;

			case SCMSG_MAX:
			case SCMSG_INVALID:
			default:
				break;
		}

		count -= (buf - buf0);
	}
}
