#ifndef NET_SERVER_H
#define NET_SERVER_H

#include "enumeration.h"
#include "types.h"

extern void Server_Send_StatusMessage1(enum HouseFlag houses, uint8 priority, uint16 str1);
extern void Server_Send_StatusMessage2(enum HouseFlag houses, uint8 priority, uint16 str1, uint16 str2);
extern void Server_Send_StatusMessage3(enum HouseFlag houses, uint8 priority, uint16 str1, uint16 str2, uint16 str3);

#endif
