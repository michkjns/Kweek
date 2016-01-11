#ifndef _PACKET_H
#define _PACKET_H

#include "server.h"
#include "../raknet/Source/GetTime.h"

#pragma pack(push, 1)

	struct PacketBase
	{
		unsigned char useTimeStamp; // Assign ID_TIMESTAMP to this
		RakNet::Time timeStamp; // use RakNet::GetTime()
		unsigned char typeID; // ID_...
	};
	struct Packet
	{
		PacketBase info;
	};

#pragma pack(pop)



#endif
