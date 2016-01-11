

#include "server.h"
#include "network.h"
#include "packet.h"
#include "log.h"
#include "game.h"
#include "player.h" 
#include "command.h"

using namespace Tmpl8;

Network* Network::m_instance = nullptr;

extern unsigned int packetsReceived;
extern unsigned int bytesReceived;

extern unsigned int packetsReceivedc;
extern unsigned int bytesReceivedc;



DWORD ThreadFunc( LPVOID a_Param )
{
	if( a_Param )
		Network::Get()->server->Main();
	else
		Network::Get()->LoopClient();

	return EXIT_SUCCESS;
}

Network::Network()
{
	m_peer = nullptr;
	server = nullptr;
	m_numPlayers = 0;
	m_scoreBlue = 0;
	m_scoreRed = 0;
	m_state = STATE_DISCONNECTED;
	m_ms = false;

	m_localName = "unnamed";
	m_masterServerAdress = "107.161.22.27";
	m_masterServerPort = 8232;
	m_serverPort = 8233;
}

Network::~Network()
{
	if( server ) delete server;
}

void Network::Init( bool server )
{
	m_peer = RakNet::RakPeerInterface::GetInstance();
	m_state = STATE_DISCONNECTED;
	m_numPlayers = 0;
	m_isServer = server;
	for( uint i = 0; i < MAX_CLIENTS; i++ )
	{
		m_clients[i].idx = -1;
		//m_clients[i].matrix = matrix();
	}
}

void Network::SendMsg( const char* msg, RakNet::AddressOrGUID dest, bool debug, bool broadcast )
{
	RakNet::BitStream bsOut;
	bsOut.Write( ( RakNet::MessageID ) ID_MESSAGE );
	std::string out( ( debug ? "DEBUG: " : "" ) );
	out = out + msg;
	bsOut.Write( out.c_str() );
	m_peer->Send( &bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, dest, broadcast );
}

void Network::ReadMsg( RakNet::Packet* packet, char* buffer, int bufferSize )
{
	RakNet::RakString rs;
	RakNet::BitStream bsIn( packet->data, packet->length, false );
	bsIn.IgnoreBytes( sizeof( RakNet::MessageID ) );
	bsIn.Read( rs );
	strncpy( buffer, rs.C_String(), bufferSize );
	buffer[bufferSize - 1] = '\0';
}

void Network::CloseNetworking()
{
	if( m_peer == NULL ) return;
	m_state = STATE_QUIT;
	Disconnect();
	m_peer->Shutdown( 500, 0, LOW_PRIORITY );
	RakNet::RakPeerInterface::DestroyInstance( m_peer );
	m_peer = NULL;
}

void Network::Disconnect()
{
	if( m_state == STATE_CONNECTED || m_state == STATE_SERVER || m_state == STATE_BROWSER || m_state == STATE_CONNECTING )
	{
		if( m_state == STATE_CONNECTING )
		{
			m_peer->CancelConnectionAttempt( m_serverAdress.systemAddress );
			m_peer->CloseConnection( m_serverAdress, true );
			LOG( "Connection canceled" );
		}
		else if( m_state == STATE_CONNECTED )
		{
			RakNet::BitStream bsOut;
			bsOut.Write( ( RakNet::MessageID ) ID_PLAYER_QUIT );
			m_peer->Send( &bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, m_serverAdress, false );
			m_peer->CloseConnection( m_serverAdress, true );
			LOG( "Connection cancel: %s", m_serverAdress.systemAddress.ToString() );
		}
		else if( m_state == STATE_BROWSER )
		{
			m_peer->CloseConnection( m_serverAdress, true );
			LOG( "Connection cancel: %s", m_serverAdress.systemAddress.ToString() );
		}
		m_state = STATE_DISCONNECTED;
		m_numPlayers = 0;
		Log::Get()->Print( "Disconnected" );
	}
	//	else
	//	Log::Get()->Print( "Disconnect: You're not connected to any server!" );
}

void Network::SendPacket( const char* packet, const int size, unsigned char type, RakNet::AddressOrGUID dest, bool broadcast )
{
	Packet* p = (Packet*)packet;
	p->info.timeStamp = RakNet::GetTime();
	p->info.useTimeStamp = ID_TIMESTAMP;
	p->info.typeID = type;
	PacketReliability reliability;
	switch( type )
	{
	case ID_PLAYER_UPDATE:
		reliability = UNRELIABLE_SEQUENCED;
		break;
	case ID_MESSAGE:
	case ID_FLAG_INTERACTION:
		reliability = RELIABLE_ORDERED;
		break;
	case ID_PLAYER_ACTION:
	case ID_PLAYER_INPUT:
	case ID_PLAYER_ADD:
	case ID_PLAYER_CONNECT:
	case ID_PLAYER_DISCONNECT:
	case ID_PLAYER_QUIT:
	case ID_CONNECTION_TIMEOUT:
	case ID_REQUEST_LIST:
	case ID_RECEIVE_LIST:
	case ID_UPDATE_MS:
	case ID_NAME_CHANGE:
		reliability = RELIABLE;
		break;
	}

	static unsigned int totalbytes = 0;
	static unsigned int numPackets = 0;
	static Timer measureTimer;
	totalbytes += size;
	numPackets++;
	//int waitt = IsServer() ? 130 : 120;
	//if( measureTimer.seconds() >= waitt )
	//{
	//	LOG( "Measurement: %i bytes, %i packets sent", totalbytes, numPackets );
	//	LOG( "Measurement: %i bytes, %i packets received (S)", bytesReceived, packetsReceived );
	//	LOG( "Measurement: %i bytes, %i packets received (C)", bytesReceivedc, packetsReceivedc );
	//	measureTimer.reset();
	//	Disconnect();
	//}
	m_peer->Send( (const char*)packet, size, HIGH_PRIORITY, reliability, 0, dest, broadcast );
}

int Network::GetPing( RakNet::AddressOrGUID target )
{
	return m_peer->GetAveragePing( target );
}

void Network::Ping( RakNet::AddressOrGUID target )
{
	m_peer->Ping( ( const RakNet::SystemAddress ) target.systemAddress );
}

void Network::OnClientExit( RakNet::Packet* packet )
{
	PacketPlayerInfo info = *(PacketPlayerInfo*)packet->data;
	int idx = FindClient( info.guid ).idx;
	if( idx == -1 ) return;
	m_clients[idx].idx = -1;
	Log::Get()->Print( "%s has left the game.", info.name );
	GameEvent gameEvent;
	gameEvent.type = GameEvent::PLAYER_QUIT;
	gameEvent.id0 = idx;
	Game::FireEvent( gameEvent );
}

unsigned char Network::GetPacketIdentifier( RakNet::Packet* packet )
{
	if( (unsigned char)packet->data[0] == ID_TIMESTAMP )
	{
		return (unsigned char)packet->data[sizeof(unsigned char)+sizeof( RakNet::Time )];
	}
	else
		return (unsigned char)packet->data[0];
}


int Network::AssignClient( const RakNet::RakNetGUID guid )
{
	// Check if client already exists on server
	Client client = FindClient( guid );

	// If not, find empty slot
	if( client.idx == -1 )
	{
		for( unsigned char i = 0; i < MAX_CLIENTS; i++ )
		{
			if( m_clients[i].idx == -1 )
			{
				m_clients[i].guid = guid;
				return i;
			}
		}
	}
	return client.idx;
}

Client& Network::FindClient( RakNet::RakNetGUID guid )
{
	for( uint i = 0; i < MAX_CLIENTS; i++ )
	{
		Client& client = m_clients[i];
		if( client.guid == guid )
			return client;
	}

	static Client empty;
	empty.idx = -1;
	return empty;
}


void Network::SendCmd( const char* cmd, RakNet::AddressOrGUID to, bool broadcast )
{
	RakNet::BitStream bsOut;
	bsOut.Write( ( RakNet::MessageID ) ID_COMMAND );
	bsOut.Write( cmd );
	m_peer->Send( &bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, to, broadcast );
}

void Network::SetMasterServerAddr( std::string addr )
{
	std::string ip, port;
	size_t split = addr.find( ":" );
	if( split != std::string::npos )
	{
		ip = addr.substr( 0, split );
		port = addr.substr( addr.find( ":" ) + 1, addr.length() );

		m_masterServerAdress = ip;
		m_masterServerPort = atoi( port.c_str() );
	}
	else
	{
		m_masterServerAdress = addr;
	}

	if( GetStatus() == Network::STATE_BROWSER )
	{
		ConnectToMS();
	}
}