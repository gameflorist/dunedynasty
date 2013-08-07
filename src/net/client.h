#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "types.h"

struct Object;

extern void Client_Send_IssueUnitAction(uint8 actionID, uint16 encoded, const struct Object *o);

#endif
