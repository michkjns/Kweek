

#include "server.h"
#include "packet.h"
#include <stdio.h>
using namespace std;

Server g_server;

bool Server::Init()
{
	printf( "Masterserver Initializing..\n" );
	m_peer = RakNet::RakPeerInterface::GetInstance();
	m_sockDesc = RakNet::SocketDescriptor( SERVER_PORT, 0 );
	m_peer->Startup( MAX_CLIENTS, &m_sockDesc, 1 );
	m_peer->SetMaximumIncomingConnections( MAX_CLIENTS );

	return true;
}

void Server::ReadMsg( RakNet::Packet* a_Packet, char* a_Buffer, int a_BufferSize )
{
	RakNet::RakString rs;
	RakNet::BitStream bsIn( a_Packet->data, a_Packet->length, false );
	bsIn.IgnoreBytes( sizeof( RakNet::MessageID ) );
	bsIn.Read( rs );
	strncpy( a_Buffer, rs.C_String(), a_BufferSize );
	a_Buffer[a_BufferSize - 1] = '\0';
}

void Server::SendPacket( const char* a_Packet, const int a_Size, unsigned char a_Type, RakNet::AddressOrGUID a_Adress, bool broadcast )
{
	Packet* packet = (Packet*)a_Packet;
	packet->info.timeStamp = RakNet::GetTime();
	packet->info.useTimeStamp = ID_TIMESTAMP;
	packet->info.typeID = a_Type;

	m_peer->Send( (const char*)packet, a_Size, HIGH_PRIORITY, RELIABLE_ORDERED, 0, a_Adress, broadcast );
}

unsigned char Server::GetPacketIdentifier( RakNet::Packet* a_Packet )
{
	if( (unsigned char)a_Packet->data[0] == ID_TIMESTAMP )
	{
		return (unsigned char)a_Packet->data[sizeof(unsigned char)+sizeof( RakNet::Time )];
	}
	else
		return (unsigned char)a_Packet->data[0];
}

bool Server::Run()
{
	char buffer[256];
	RakNet::Packet* packet;
	unsigned char packetIdentifier;
	while( 1 )
	{
		sleep( 5 );
		for( packet = m_peer->Receive(); packet; m_peer->DeallocatePacket( packet ), packet = m_peer->Receive() )
		{
			packetIdentifier = GetPacketIdentifier( packet );
			switch( packetIdentifier )
			{
			case ID_CLIENT_UPDATE:
				UpdateClientInfo( packet );
				PrintList();
				break;
			case ID_REQUEST_LIST:
				printf("req. rec.\n");
				SendList( packet->systemAddress );
				break;
			case ID_CONNECTION_LOST:
				printf("connection lost\n");
				break;
			case ID_CONNECTION_TIMEOUT:
				printf("timeout\n");
				break;
			case ID_MESSAGE:
				ReadMsg( packet, buffer, 256 ); //  read string
				printf( "%s\n", buffer );
				break;
			}
		}

		for( std::vector<Client>::iterator i = m_clients.begin(); i != m_clients.end();)
		{
			//printf("Time out!\n");
			if( RakNet::GetTime() - (*i).lastPing > 10000 )
			{
				i = m_clients.erase( i );
				//printf("Time out\n");
				continue;
			}
			 ++i;
		}

	}

	return true;
}
#include <string>
void Server::UpdateClientInfo( RakNet::Packet* packet )
{
	int numPlayers;
	RakNet::RakString rs;
	RakNet::BitStream bsIn( packet->data, packet->length, false );
	bsIn.IgnoreBytes( sizeof( RakNet::MessageID ) );
	bsIn.Read( numPlayers );
	bsIn.Read( rs );
	//strncpy( a_Buffer, rs.C_String(), a_BufferSize );
	//a_Buffer[a_BufferSize - 1] = '\0';

	//See if server is already registered
	for( unsigned int i = 0; i < m_clients.size(); i++)
	{
		if( m_clients[i].ip == packet->systemAddress)
		{
			m_clients[i].numPlayers = numPlayers;
			m_clients[i].lastPing = RakNet::GetTime();
			return;
		}
	}

	//Otherwise register the server as new
	Client c;
	c.ip = packet->systemAddress;
	c.lastPing = RakNet::GetTime();
	//strcpy( c.name, rs.C_String() );
    std::string n( rs.C_String() );
	printf("%s\n", rs.C_String() );
	strcpy( c.name, n.c_str() );
	c.numPlayers = numPlayers;
	m_clients.push_back( c );
}

void Server::SendList( RakNet::SystemAddress addr )
{
	RakNet::BitStream bsOut;
	bsOut.Write( ( RakNet::MessageID ) ID_RECEIVE_LIST );
	bsOut.Write( m_clients.size() );
	for( int i = 0; i < m_clients.size(); i++)
	{
		bsOut.Write( m_clients[i].ip );
		bsOut.Write( m_clients[i].name );
		bsOut.Write( m_clients[i].numPlayers );
	}

	m_peer->Send( &bsOut, HIGH_PRIORITY, RELIABLE, 0, addr, false );
}

void Server::PrintList()
{
    system("clear");
	for( int i = 0; i < m_clients.size(); i++ )
	{
		printf("%s - %i/8 players\n", m_clients[i].name, m_clients[i].numPlayers );
	}
}

int main( int argc, char **argv )
{
	assert( g_server.Init() );
	printf( "Server Initialized\n" );

	bool result = g_server.Run();

	assert( result );

	return 0;
}
