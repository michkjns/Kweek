

#ifndef _SERVER_H
#define _SERVER_H

#include "../raknet/Source/RakPeerInterface.h"
#include "../raknet/Source/MessageIdentifiers.h"
#include "../raknet/Source/BitStream.h"
#include "../raknet/Source/RakNetTypes.h"
#include <vector>
#include <string>

#define SERVER_ADDR "84.104.132.138"//"127.0.0.1"
#define SERVER_PORT 8232
#define MAX_CLIENTS 8
#define MAX_NAMELENGTH 16
#pragma warning (disable : 4530)

typedef unsigned int SystemAddress;
typedef unsigned int uint;

enum MessageIds
{
	ID_MESSAGE = ID_USER_PACKET_ENUM + 1,  // Message packet ID 1
	ID_CONNECTION_TIMEOUT = 143,
	ID_CLIENT_UPDATE = 146,
	ID_REQUEST_LIST,
	ID_RECEIVE_LIST,
};

struct Client
{
	char name[MAX_NAMELENGTH];
	int numPlayers;
	RakNet::SystemAddress ip;
	RakNet::Time lastPing;
};

class Server
{
public:
	bool Init();
	bool Run();

	void ReadMsg( RakNet::Packet* packet, char* buffer, int bufferSize );
	void SendMsg( const char* msg, RakNet::AddressOrGUID dest, bool debug = false, bool broadcast = true );
	void UpdateClientInfo( RakNet::Packet* packet );
	void SendPacket( const char* packet, const int size, unsigned char type, RakNet::AddressOrGUID adress, bool broadcast = false );
	void SendList( RakNet::SystemAddress addr );
	void PrintList();

protected:
	RakNet::SocketDescriptor m_sockDesc;
	RakNet::RakPeerInterface* m_peer;
	std::vector<Client> m_clients;
	unsigned char GetPacketIdentifier( RakNet::Packet* packet );

};

#endif
