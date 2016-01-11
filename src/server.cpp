

#include "server.h"
#include "packet.h"
#include "log.h"
#include "player.h"
#include "game.h"
#include <SDL.h>
using namespace Tmpl8;

extern DWORD ThreadFunc( LPVOID param );

unsigned int packetsReceived = 0;
unsigned int bytesReceived = 0;

bool Network::StartServer()
{
	m_socketDesc = RakNet::SocketDescriptor( m_serverPort, 0 );

	m_peer->Startup( MAX_CLIENTS, &m_socketDesc, 1 );
	m_peer->SetMaximumIncomingConnections( MAX_CLIENTS );
	RakNet::ConnectionAttemptResult error = m_peer->Connect( m_masterServerAdress.c_str(), m_masterServerPort, 0, 0 );
	if( error != 0 )
	{
		Log::Get()->Print( "ERROR: Could not connect to MS. Port already in use?" );
		return false;
	}
	server = new Server( this );
	CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)&ThreadFunc, (LPVOID)1, 0, NULL );
	m_state = STATE_SERVER;
	return true;
}

Server::Server( Network* net)
{
	m_network = net;
	m_scoreRed = 0;
	m_scoreBlue = 0;
}

Server::~Server()
{
}

void Server::Main()
{
	RakNet::Packet* packet;
	RakNet::RakPeerInterface* peer = m_network->m_peer;
	unsigned char packetIdentifier;
	char buffer[256];
	Timer timer0;

	Log::Get()->Print("Server started. Listening on port %i..", m_network->GetServerPort() );
	snapshotTimer.reset();
	while( m_network->GetStatus( ) != Network::STATE_QUIT )
	{
		for( packet = peer->Receive(); packet; peer->DeallocatePacket( packet ), packet = peer->Receive() )
		{
			packetsReceived++;
			bytesReceived += sizeof(packet->data);
			packetIdentifier = m_network->GetPacketIdentifier( packet );
			switch( packetIdentifier )
			{
			case ID_PLAYER_INPUT:
				ReceiveInput( packet );
				break;
			case ID_PLAYER_ACTION:
				ReceiveAction( packet );
				break;
			case ID_PLAYER_CONNECT:
				AcceptClient( packet );
				break;
			case ID_MESSAGE:
					m_network->ReadMsg( packet, buffer, 256 );
					Log::Get()->Print( "%s: %s\n", m_network->FindClient(packet->guid).name, buffer );
					SendChat( buffer, m_network->FindClient( packet->guid ).name );
					break;
			case ID_COMMAND:
				ReadCmd( packet, buffer, 256 );
			//	Log::Get()->Print( "%s: %s\n", m_network->FindClient( packet->guid ).name, buffer );
			//	SendChat( buffer, m_network->FindClient( packet->guid ).name );
				break;
			case ID_NAME_CHANGE:
				AcceptNameChange( packet );
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				Log::Get()->Print( "Server is full\n" );
				break;
			case ID_PLAYER_QUIT:
			case ID_CONNECTION_LOST:
				OnClientQuit( packet->guid, packetIdentifier );
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				Log::Get()->Print( "Succesfully registered to the masterserver\n" );
				m_serverAdress = packet->guid;
				UpdateMS();
				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				Log::Get()->Print( "Connection attempt failed\n" );
				break;
			}
		}
		if( snapshotTimer.milliseconds() > 50 )
		{
			for(unsigned int i = 0; i < MAX_CLIENTS; i++)
			{
				Client& c = m_network->m_clients[i];
				if( c.idx != -1 && Player::s_players[c.idx] )
				{
					m_network->SendCorrection( Player::s_players[c.idx]->GetMatrix().GetTranslation(), Player::s_players[c.idx]->GetPitch(), c.guid );
					Player::s_players[c.idx]->SetAngle(0);
				}
			}
			snapshotTimer.reset();

			if( msPingTimer.seconds() > 5 )
			{
				msPingTimer.reset();
				UpdateMS();
			}
		}
	}
}

void Server::SendChat( char* msg, char* name )
{
	std::string m( name );
	m += ": " + std::string( msg );
	m_network->SendMsg( m.c_str(), m_network->m_peer->GetMyGUID(), false, true );
}

void Server::OnClientQuit( const RakNet::RakNetGUID guid, const int reason )
{
	Client client = m_network->FindClient( guid );
	char reasonStr[MAX_NAMELENGTH];
	switch( reason )
	{
	case ID_CONNECTION_TIMEOUT:
		SDL_strlcpy( reasonStr, "timeout", MAX_NAMELENGTH );
		break;
	case ID_PLAYER_QUIT:
		SDL_strlcpy( reasonStr, "quit", MAX_NAMELENGTH );
		break;
	default:
		SDL_strlcpy( reasonStr, "unknown", MAX_NAMELENGTH );
		break;
	}
	if( client.idx >= 0 )
	{
#if DEBUG_MESSAGES
		Log::Get()->Print( "Client %i disconnected: %s (%s)\n", client.idx, client.name, reasonStr );
#else
		Log::Get()->Print( "%s has disconnected (%s)\n", client.name, reasonStr );
#endif

		GameEvent gameEvent;
		gameEvent.type = GameEvent::PLAYER_QUIT;
		gameEvent.id0 = client.idx;
		Game::FireEvent( gameEvent );

		Game::RemovePlayer( client.idx );
		BroadcastClient( client, true );
		m_network->m_clients[client.idx].idx = -1;
		UpdateMS();
#if DEBUG_MESSAGES
		Log::Get()->Print( "Server::OnClientQuit (completed)" );
#endif
	}
}

void Server::ReceiveInput( const RakNet::Packet* packet )
{
	Client c = m_network->FindClient( packet->guid );
	if( c.idx == -1 )
	{
		Log::Get()->Print( "ERROR: Input received from non-existing client?" );
		return;
	}
	PacketPlayerInput* input = (PacketPlayerInput*)packet->data;
	
	Player* p = Player::s_players[c.idx];
	if( !p ) return;
	p->m_input.forward = input->forward;
	p->m_input.backward = input->backward;
	p->m_input.left = input->left;
	p->m_input.right = input->right;
	p->transform.Rotate( float3( 0, -input->pitch * (180/PI), 0 ) );
	p->SetAngle( p->GetPitch() - input->pitch );
	p->SetRotation( p->GetRotation() + input->pitch );
	p->SetYaw( p->GetYaw() - input->yaw );
	p->SetRotYaw( p->GetRotYaw() + input->yaw );
//	printf( "%f \n", p->GetRotation( )  * ( 180 / PI ) );
}

void Network::SendCorrection( float3 pos, float angle, RakNet::RakNetGUID guid )
{
	PacketPlayerUpdate packet;
	packet.guid = guid;
	packet.pos = pos;
	packet.angle = angle;
	packet.ping = GetPing( guid );
	SendPacket( (const char*)&packet, sizeof( packet ), ID_PLAYER_UPDATE, m_peer->GetMyGUID(), true );
}

void Server::ReceiveAction( const RakNet::Packet* packet )
{
	Client c = m_network->FindClient( packet->guid );
	if( c.idx == -1 )
	{
		Log::Get()->Print( "ERROR: Input received from non-existing client?" );
		return;
	}
	PacketPlayerAction* input = (PacketPlayerAction*)packet->data;
	Player* p = Player::s_players[c.idx];
	p->m_input.fire = (input->action == SHOOT);
	p->m_input.jump = (input->action == JUMP);

	SendAction( input->action, packet->guid );
}

void Server::SendAction( PlayerAction action, int pid )
{
	RakNet::RakNetGUID guid = m_network->FindClient( pid ).guid;
	SendAction( action, guid );
}
void Server::SendAction( PlayerAction action, RakNet::RakNetGUID guid )
{
	PacketPlayerAction packet;
	packet.guid = guid;
	packet.action = action;
	m_network->SendPacket( (const char*)&packet, sizeof( packet ), ID_PLAYER_ACTION, 
		( action == DEATH ) ? m_serverAdress : guid,
		true );
}

void Server::FlagInteraction( int flag, int pid )
{
	PacketFlagAction p;
	p.flag = ( PacketFlagAction::Action )flag;
	Client& c = m_network->FindClient( pid );
	switch( flag )
	{
	case PacketFlagAction::Action::STEAL:
		Log::Get()->Print( "%s picked up the flag!", c.name );
		break;
	case PacketFlagAction::Action::CAPTURE:
		Log::Get()->Print( "%s scored a capture!", c.name );
		OnScore( pid % 2 );
		break;
	case PacketFlagAction::Action::RETURN:
	case PacketFlagAction::Action::DROP:
	default:
		break;
	}

	p.guid = c.guid;
	m_network->SendPacket( (const char*)&p, sizeof( p ), ID_FLAG_INTERACTION, m_serverAdress, true );
}

void Server::OnScore( int team )
{
	( team == 0 ) ? m_scoreRed++ : m_scoreBlue++;
	SendScore();
}

void Server::SendScore()
{
	PacketScore p;
	p.blue = m_scoreBlue;
	p.red = m_scoreRed;
	m_network->SendPacket( (const char*)&p, sizeof( p ), ID_SCORE, m_serverAdress, true );

	m_network->m_scoreBlue = m_scoreBlue;
	m_network->m_scoreRed = m_scoreRed;
}

void Server::AcceptClient( const RakNet::Packet* packet )
{
#if DEBUG_MESSAGES
	Log::Get()->Print( "Server::AcceptClient" );
#endif
	int idx = AssignClient( packet->guid );
	assert( idx != -1 );
	Client& client = m_network->m_clients[idx];
	client.idx = idx;
	client.guid = packet->guid;

	SDL_strlcpy( client.name, ( (PacketPlayerInfo*)packet->data )->name, MAX_NAMELENGTH );

#if DEBUG_MESSAGES
	Log::Get()->Print( "Client %i accepted: %s\n", client.idx, client.name );
#else
	Log::Get()->Print( "%s has joined the game", client.name );
#endif

	GameEvent gameEvent;
	PacketPlayerInfo* playerInfo = new PacketPlayerInfo( );
	gameEvent.type = GameEvent::PLAYER_JOIN;
	gameEvent.id0 = idx;
	playerInfo->pitch = 0;
	playerInfo->pos = float3(0,0,0);
	playerInfo->flag = false;
	gameEvent.data = playerInfo;

	Game::FireEvent( gameEvent );
	//Game::SpawnPlayer( client.idx );
//	BroadcastClient( client, false );

	PacketScore p;
	p.blue = m_scoreBlue;
	p.red = m_scoreRed;
	m_network->SendPacket( (const char*)&p, sizeof( p ), ID_SCORE, client.guid );

	m_network->m_scoreBlue = m_scoreBlue;
	m_network->m_scoreRed = m_scoreRed;

	UpdateMS();
}

void Server::BroadcastClient( const Client& client, bool disconnect )
{
	PacketPlayerInfo packet;
	SDL_strlcpy( packet.name, client.name, MAX_NAMELENGTH );
	packet.guid = client.guid;
	packet.id = client.idx;
	packet.pitch = disconnect ? 0 : Player::s_players[client.idx]->GetRotation();
	packet.pos = disconnect ? float3(0,0,0) : Player::s_players[client.idx]->transform.GetTranslation();
	packet.flag = false;

	const int size = sizeof( PacketPlayerInfo );
	if( disconnect )
		m_network->SendPacket( (const char*)&packet, size, ID_PLAYER_QUIT, m_network->m_peer->GetMyGUID(), true );
	else
	{
		m_network->SendPacket( (const char*)&packet, size, ID_PLAYER_ADD, m_network->m_peer->GetMyGUID(), true );

		for( unsigned int i = 0; i < 8; i++ )
		{ // Notify new client of the existing clients
			if( m_network->m_clients[i].idx != -1 && m_network->m_clients[i].idx != client.idx )
			{
				PacketPlayerInfo packet;
				SDL_strlcpy( packet.name, m_network->m_clients[i].name, MAX_NAMELENGTH );
				packet.guid = m_network->m_clients[i].guid;
				packet.id = i;
				packet.pitch = Player::s_players[i]->GetRotation();
				packet.pos = Player::s_players[i]->transform.GetTranslation();
				packet.flag = Player::s_players[i]->HasFlag();
				const int size = sizeof( packet );
				m_network->SendPacket( (const char*)&packet, size, ID_PLAYER_ADD, client.guid, false );
			}
		}
	}
}

int Server::AssignClient( const RakNet::RakNetGUID guid )
{
	// Check if client already exists on server
	Client client = m_network->FindClient( guid );

	// If not, find empty slot
	if( client.idx == -1 )
	{
		for( unsigned char i = 0; i < MAX_CLIENTS; i++ )
		{
			if( m_network->m_clients[i].idx == -1 )
			{
				return i;
			}
		}
		// Server full!
	}
	return client.idx;
}

void Server::AcceptNameChange( const RakNet::Packet* packet )
{
	PacketNameChange* p = ( PacketNameChange* )packet->data;
	Client& c = m_network->FindClient( packet->guid );
	if( c.idx == -1 )
	{
#ifdef DEBUG_MESSAGES
		Log::Get( )->Print( "Warning: Namechange from an unexisting client, ignored");
#endif
		return;
	}
	Log::Get()->Print( "%s changed his/her name to %s\n", c.name, p->name );
	strcpy_s( c.name, p->name );
	PacketNameChange out;
	strcpy_s( out.name, p->name );
	out.guid = packet->guid;
	m_network->SendPacket( (const char*)&out, sizeof( out ), ID_NAME_CHANGE, m_serverAdress, true );
}

void Server::ReadCmd( RakNet::Packet* packet, char* buffer, int bufferSize )
{
	RakNet::RakString rs;
	RakNet::BitStream bsIn( packet->data, packet->length, false );
	bsIn.IgnoreBytes( sizeof( RakNet::MessageID ) );
	bsIn.Read( rs );
	strncpy( buffer, rs.C_String(), bufferSize );
	buffer[bufferSize - 1] = '\0';
	std::string cmd( buffer );
	Client& c = m_network->FindClient( packet->guid );
	int pid = c.idx;
	if( pid == -1 ) return;
	if( cmd.compare( "kill" ) == 0 )
	{
		char buffer[32];
		sprintf( buffer, "%s suicided", c.name );
		Log::Get()->Print( buffer );
		m_network->SendMsg( buffer, m_network->m_peer->GetMyGUID(), false, true );
		GameEvent ge;
		ge.type = GameEvent::PLAYER_SUICIDE;
		ge.id0 = pid;
		Game::FireEvent( ge );
	}

}

void Server::UpdateMS()
{
	RakNet::BitStream bsOut;
	bsOut.Write( ( RakNet::MessageID ) ID_UPDATE_MS );
	bsOut.Write( m_network->GetNumPlayers() );
	bsOut.Write( "Unnamed Server" );

	m_network->m_peer->Send( &bsOut, HIGH_PRIORITY, RELIABLE, 0, m_serverAdress, false );
	UpdateStats();
}

void Server::UpdateStats()
{
	return;
	system( "cls" );
	printf( "			Unnamed Server - %i/8 players \n\n", m_network->GetNumPlayers() );
	printf( "RED TEAM: %i\n", m_scoreRed );
	for( int i = 0; i < m_network->GetNumPlayers(); i++ )
	{
		Client c = m_network->m_clients[i];
		if( Player::s_players[c.idx]->GetTeam() == 0 )
			printf( " %i %s - Score: %i - Health: %i -ping: %i \n", c.idx, c.name, (int)Player::s_players[c.idx]->GetScore(), (int)Player::s_players[c.idx]->GetHealth(), m_network->GetPing( c.guid ) );
	}
	printf( "\n\nBLUE TEAM: %i\n", m_scoreBlue );
	for( int i = 0; i < m_network->GetNumPlayers(); i++ )
	{
		Client c = m_network->m_clients[i];
		if( Player::s_players[c.idx]->GetTeam() == 1 )
			printf( " %i %s - Score: %i - Health: %i - ping: %i \n", c.idx, c.name, (int)Player::s_players[c.idx]->GetScore(), (int)Player::s_players[c.idx]->GetHealth(), m_network->GetPing( c.guid ) );
	}
}