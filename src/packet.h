

#ifndef _PACKET_H
#define _PACKET_H

#ifdef IGNORE
#undef IGNORE
#endif

#include "network.h"
#include "..\lib\RakNet_PC-4.0802\Source\GetTime.h"
namespace Tmpl8
{
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

	struct PacketPlayerInfo
	{
		PacketBase info;
		RakNet::RakNetGUID guid;
		char name[MAX_NAMELENGTH];
		int id;
		float pitch;
		float3 pos;
		bool disconnect;
		bool flag;
	};

	struct PacketPlayerUpdate
	{
		PacketBase info;
		RakNet::RakNetGUID guid;
		float3 pos;
		float angle;
		int ping;
	};

	struct PacketPlayerInput
	{
		PacketBase info;
		bool forward, backward, left, right;
		bool jump;
		float pitch;
		float yaw;
	};

	struct PacketPlayerAction
	{
		PacketBase info;
		RakNet::RakNetGUID guid;
		PlayerAction action;
	};

	struct PacketFlagAction
	{
		PacketBase info;
		enum Action {
			STEAL, DROP, RETURN, CAPTURE
		} flag;
		RakNet::RakNetGUID guid;
	};

	struct PacketNameChange
	{
		PacketBase info;
		RakNet::RakNetGUID guid;
		char name[MAX_NAMELENGTH];
	};

	struct PacketScore
	{
		PacketBase info;
		int red;
		int blue;
	};

#pragma pack(pop)
};

#endif