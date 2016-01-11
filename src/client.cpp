

#include "network.h"
#include "packet.h"
#include "log.h"
#include "player.h"
#include "game.h"
#include "command.h" 

using namespace Tmpl8;

unsigned int packetsReceivedc = 0;
unsigned int bytesReceivedc = 0;

extern DWORD ThreadFunc( LPVOID param );

bool Network::StartClient()
{
	m_peer->Startup( MAX_CLIENTS, &m_socketDesc, 1 );
	ConnectToMS();
	CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)&ThreadFunc, (LPVOID)0, 0, NULL );
	return true;
}

void Network::LoopClient()
{
	RakNet::Packet* packet;
	RakNet::RakPeerInterface* peer = m_peer;
	unsigned char packetIdentifier;
	char buffer[256];
	Timer timer0;

	while( m_state != STATE_QUIT )
	{
		for( packet = peer->Receive(); packet; peer->DeallocatePacket( packet ), packet = peer->Receive() )
		{

			packetsReceivedc++;
			bytesReceivedc += sizeof( packet->data );

			packetIdentifier = GetPacketIdentifier( packet );
			switch( packetIdentifier )
			{
			case ID_PLAYER_UPDATE:
				if( !IsConnected() ) break;
				TakeCorrection( packet );
				break;
			case ID_PLAYER_ACTION:
				if( !IsConnected( ) ) break;
				ReceiveAction( packet );
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				Log::Get()->Print( "Connection accepted\n" );
				m_serverAdress = packet->guid;
				if( !m_ms )
				{
					m_state = STATE_BROWSER;
					m_ms = true;
					RequestServerList();
				} else
				{
					Game::EnterGame();
					IdentifyMyself();
					m_state = STATE_CONNECTED;
					m_ms = false;
				}
				break;
			case ID_FLAG_INTERACTION:
				if( !IsConnected( ) ) break;
				TakeFlagInteraction( packet );
				break;
			case ID_MESSAGE:
				if( !IsConnected( ) ) break;
				ReadMsg( packet, buffer, 256 );
				Audio::Get()->PlaySound( "assets/sound/menu.ogg" );
				Log::Get()->Print( "%s\n", buffer );
				break;
			case ID_PLAYER_ADD:
				if( !IsConnected( ) ) break;
				OnClientJoin( packet );
				break;
			case ID_PLAYER_QUIT:
				if( !IsConnected( ) ) break;
				OnClientExit( packet );
				break;
			case ID_NAME_CHANGE:
				if( !IsConnected( ) ) break;
				TakeNameChange( packet );
				break;
			case ID_SCORE:
				if( !IsConnected( ) ) break;
				TakeScore( packet );
				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				if( m_state == STATE_BROWSER ) break;
				Log::Get()->Print( "ERROR: Connection Attempt Failed" );
				m_state = STATE_DISCONNECTED;
				ConnectToMS();
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				Log::Get()->Print( "Server is full\n" );
				m_state = STATE_DISCONNECTED;
				break;
			case ID_CONNECTION_LOST:
				Log::Get()->Print( "Lost the connection.\n" );
				Command::Run( "disconnect" );
				//Disconnect();
				//ConnectToMS();
				break;
			case ID_RECEIVE_LIST:
#ifdef DEBUG_MESSAGES
				Log::Get()->Print( "Received list from MS" );
#endif
				ReceiveServerList( packet );
				break;
			}
		}
	}
}

void Network::IdentifyMyself()
{
	/* Client announces his name to the server */
	PacketPlayerInfo packet;
	packet.guid = m_peer->GetMyGUID();
	strncpy( packet.name, m_localName.c_str(), MAX_NAMELENGTH );
	packet.name[MAX_NAMELENGTH - 1] = '\0';
	const int size = sizeof( packet );

	SendPacket( (const char*)&packet, size, ID_PLAYER_CONNECT, m_serverAdress );
}

void Network::OnClientJoin( RakNet::Packet* packet )
{
	PacketPlayerInfo info = *(PacketPlayerInfo*)packet->data;
	int idx = info.id;
	strncpy( m_clients[idx].name, info.name, MAX_NAMELENGTH );
	m_clients[idx].name[MAX_NAMELENGTH - 1] = '\0';
	m_clients[idx].idx = idx;
	m_clients[idx].guid = info.guid;
	m_clients[idx].ping = -1;

	if( info.guid == m_peer->GetMyGUID() )
	{
		GameEvent gameEvent;
		gameEvent.type = GameEvent::PLAYER_JOIN;
		gameEvent.id0 = idx;
		PacketPlayerInfo* playerInfo = new PacketPlayerInfo();
		playerInfo->pitch = info.pitch;
		playerInfo->pos = info.pos;
		playerInfo->name[0] = 'y'; // Means it is the local player, yes this is ugly
		gameEvent.data = playerInfo;
		Game::FireEvent( gameEvent );
		return;
	}
#if DEBUG_MESSAGES
	Log::Get()->Print( "%i %s has joined the game", idx, m_clients[idx].name );
#else
	Log::Get()->Print( "%s has joined the game", m_clients[idx].name );
#endif


	GameEvent gameEvent;
	gameEvent.type = GameEvent::PLAYER_JOIN;
	gameEvent.id0 = idx;
	PacketPlayerInfo* playerInfo = new PacketPlayerInfo();
	playerInfo->pitch = info.pitch;
	playerInfo->pos = info.pos;
	playerInfo->name[0] = 'n'; // Means it is NOT the local player, yes this is ugly
	gameEvent.data = playerInfo;
	Game::FireEvent( gameEvent );

	//Game::SpawnPlayer( idx );
	//Player* p = Player::s_players[idx];
	//p->SetTranslation( info.pos );
	//p->Rotate( float3( 0, -( info.angle ) * ( 180 / PI ), 0 ) );
	//p->SetRotation( info.angle );
	if( info.flag )
	{
		GameEvent gameEvent;
		gameEvent.type = GameEvent::PLAYER_FLAG;
		gameEvent.id0 = idx;
		Game::FireEvent( gameEvent );
	}
}

void Network::SendInput( bool forward, bool backward, bool left, bool right, float pitch, float yaw )
{
	if( m_isServer ) return;
	PacketPlayerInput packet;
	packet.forward = forward;
	packet.backward = backward;
	packet.left = left;
	packet.right = right;
	packet.pitch = pitch;
	packet.yaw = yaw;
	const int size = sizeof( packet );
	SendPacket( (const char*)&packet, size, ID_PLAYER_INPUT, m_serverAdress );
}

void Network::SendAction( PlayerAction action )
{
	if( m_isServer ) return;
	PacketPlayerAction packet;
	packet.action = action;
	SendPacket( (const char*)&packet, sizeof( packet ), ID_PLAYER_ACTION, m_serverAdress );
}

void Network::TakeCorrection( RakNet::Packet* packet )
{ 
	if( m_state == Network::STATE_QUIT ) return;
	PacketPlayerUpdate* updatePacket = (PacketPlayerUpdate*)packet->data;
	if( updatePacket->guid == m_peer->GetMyGUID() )
	{
		if( Player::s_localPlayer ) Player::s_localPlayer->TakeTransform( updatePacket->pos, updatePacket->angle );
		FindClient( m_peer->GetMyGUID() ).ping = updatePacket->ping;
	}
	else
	{
		Client& c = FindClient( updatePacket->guid );
		c.ping = updatePacket->ping;
		if( c.idx == -1 )
		{
#ifdef DEBUG_MESSAGES
			Log::Get()->Print("Invalid player id, package dismissed");
#endif
			return;
		}

		if( Player::s_players[c.idx] ) Player::s_players[c.idx]->TakeTransform( updatePacket->pos, updatePacket->angle );
	}
}

void Network::ReceiveAction( RakNet::Packet* packet )
{
	PacketPlayerAction* action = (PacketPlayerAction*)packet->data;
	Client c = FindClient( action->guid );
	if( c.idx == -1 )
	{
#ifdef DEBUG_MESSAGES
		Log::Get()->Print( "ERROR: Action rec. from non-existing client" );
#endif
		return;
	}
//	Player* p = Player::s_players[c.idx]; 
	//p->m_input.fire = ( action->action == SHOOT );
	//p->m_input.jump = ( action->action == JUMP );

	GameEvent gameEvent;

	switch( action->action )
	{
	case JUMP:
		gameEvent.type = GameEvent::PLAYER_JUMP;
		gameEvent.id0 = c.idx;
		Game::FireEvent( gameEvent );
		break;
	case SHOOT:
		gameEvent.type = GameEvent::PLAYER_SHOT;
		gameEvent.id0 = c.idx;
		Game::FireEvent( gameEvent );
		break;
	case DEATH:
		gameEvent.type = GameEvent::PLAYER_DEATH;
		gameEvent.id0 = c.idx;
		Game::FireEvent( gameEvent );
		break;
	default:
		break;
	}	
}

void Network::ReceiveInput( RakNet::Packet* packet )
{
	Client c = FindClient( packet->guid );
	if( c.idx == -1 )
	{
#ifdef DEBUG_MESSAGES
		Log::Get()->Print( "ERROR: Input rec. from non-existing client" );
#endif
		return;
	}
	PacketPlayerInput* input = (PacketPlayerInput*)packet->data;
	Player* p = Player::s_players[c.idx]; 
	p->m_input.forward = input->forward;
	p->m_input.backward = input->backward;
	p->m_input.left = input->left;
	p->m_input.right = input->right;
	p->m_input.jump = input->jump;
	p->transform.Rotate( float3( 0, -input->pitch * (180/PI), 0 ) );
}

bool Network::IsConnected()
{
	return (m_state == STATE_CONNECTED );
}

void Network::RequestServerList()
{
	Log::Get()->Print("Refreshing server list..");
	m_state = Network::STATE_BROWSER;
	Packet p;
	SendPacket( (const char*)&p, sizeof(p), ID_REQUEST_LIST, m_serverAdress );
}

void Network::ReceiveServerList( RakNet::Packet* packet )
{
#ifdef DEBUG_MESSAGES
	Log::Get()->Print("Received server list");
#endif
	m_serverList.clear();
	m_state = STATE_BROWSER;
	int numServers;
	int numPlayers;
	RakNet::RakString serverName;
	RakNet::BitStream bsIn( packet->data, packet->length, false );
	RakNet::SystemAddress serverAddress;
	bsIn.IgnoreBytes( sizeof( RakNet::MessageID ) );
	bsIn.Read( numServers );
	for( int i = 0; i < numServers; i++)
	{
		bsIn.Read( serverAddress );
		bsIn.Read( serverName );
		bsIn.Read( numPlayers );
		//printf("%s - %s - %i/8 players", serverAddress.ToString(), serverName.C_String(), numPlayers );
		AvailableServer s;
		s.addr = serverAddress;
		s.name = serverName;
		s.numPlayers = numPlayers;
		m_serverList.push_back( s );
	}
}

bool Network::ConnectToGame( RakNet::AddressOrGUID addr )
{
	Disconnect();
	Log::Get()->Print("Connecting to %s..", addr.ToString());
	m_state = STATE_CONNECTING;

	RakNet::ConnectionAttemptResult error = m_peer->Connect( addr.ToString(), m_serverPort, 0, 0 );
	if( error != 0 )
	{
		Log::Get()->Print( "ERROR: Connection attempt failed." );
		return false;
	}

	return true;
}

bool Network::ConnectToMS()
{
	Disconnect();
	Log::Get()->Print( "Connecting to ms" );
	m_state = STATE_CONNECTING;
	m_ms = false;
	RakNet::ConnectionAttemptResult error = m_peer->Connect(m_masterServerAdress.c_str()
		,m_masterServerPort, 0, 0 );
	//m_serverAdress = RakNetGUID( MS_ADDR );
	if( error != 0 )
	{
		Log::Get()->Print( "ERROR: Connection attempt failed. (%i)", error );
		return false;
	}

	return true;
}

void Network::SendNameChange( std::string name )
{
	m_localName = name;

	if( m_state == Network::STATE_CONNECTED )
	{
		int pid = Player::s_localPlayer->GetId();
		PacketNameChange packet;
		strncpy( packet.name, name.c_str(), MAX_NAMELENGTH );
		packet.name[MAX_NAMELENGTH - 1] = '\0';
		SendPacket( (const char*)&packet, sizeof( packet ), ID_NAME_CHANGE, m_serverAdress );
	}
	else
	{
		Log::Get()->Print( "Your name is now: %s", m_localName.c_str() );
	}
}

void Network::TakeNameChange( RakNet::Packet* packet )
{
	PacketNameChange* p = (PacketNameChange*)packet->data;

	Client& c = FindClient( p->guid );
	if( c.idx == -1 )
	{
#ifdef DEBUG_MESSAGES
		Log::Get()->Print( "Warning: Namechange from an unexisting client, ignored" );
#endif
		return;
	}
//	else if( c.guid == m_peer->GetMyGUID() ) return;

	Log::Get()->Print( "%s changed his/her name to %s\n", c.name, p->name );
	strcpy_s( c.name, p->name );

}

void Network::TakeFlagInteraction( RakNet::Packet* packet )
{
	PacketFlagAction* p = (PacketFlagAction*)packet->data;
	Client& c = FindClient( p->guid );
	int pid = c.idx;
	if( pid == -1 )
	{
		if( p->guid == m_peer->GetMyGUID() )
		{
			pid = Player::s_localPlayer->GetId();
			switch( p->flag )
			{
			case PacketFlagAction::Action::STEAL:
				Log::Get()->Print( "You've picked up the flag!", c.name );
				break;
			case PacketFlagAction::Action::CAPTURE:
				Log::Get()->Print( "You've scored for your team!", c.name );
				break;
			case PacketFlagAction::Action::RETURN:
			case PacketFlagAction::Action::DROP:
			default:
				break;
			}
		}
		else
		{
			Log::Get()->Print( "error picked up the flag!", c.name );
			return;
		}
	}
	else
	{
		switch( p->flag )
		{
		case PacketFlagAction::Action::STEAL:
			Log::Get()->Print( "%s picked up the flag!", c.name );
			break;
		case PacketFlagAction::Action::CAPTURE:
			Log::Get()->Print( "%s scored a capture!", c.name );
			break;
		case PacketFlagAction::Action::RETURN:
		case PacketFlagAction::Action::DROP:
		default:
			break;
		}
	}

	GameEvent gameEvent;
	gameEvent.id0 = pid;
	gameEvent.type = GameEvent::PLAYER_FLAG;
	gameEvent.id1 = p->flag;
	Game::FireEvent( gameEvent );

}

void Network::SendCmd( const char* cmd )
{
	SendCmd( cmd, m_serverAdress );
}

void Network::TakeScore( RakNet::Packet* packet )
{
	PacketScore* p = ( PacketScore*)packet->data;

	m_scoreBlue = p->blue;
	m_scoreRed = p->red;

}

int Network::GetScore( int team )
{
	return ( team == 0 ) ? m_scoreRed : m_scoreBlue;
}