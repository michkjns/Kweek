

#include "game.h"
#include "surface.h"
#include "renderer.h"
#include "scenegraph.h"
#include "player.h"
#include "scene.h"
#include "input.h"
#include "..\lib\SDL\include\SDL.h"
#include "log.h"
#include "network.h" 
#include "server.h"
#include "physics.h"
#include "command.h"
#include "flag.h"
#include "packet.h"
#include "particle.h"

using namespace Tmpl8;
Game* Command::s_game = nullptr;

CRITICAL_SECTION g_Crit;

void Game::Init( bool isServer, bool isHeadless )
{
	//	isServer = true;
	LoadConfig( "config.cfg" );
	Command::s_game = this;
	InitializeCriticalSection( &g_Crit );
	m_server = isServer;
	m_headless = isHeadless;
	m_renderer = new Renderer();
	m_renderer->Init( m_surface );
	m_textInput = false;
	m_sceneInitialised = false;
	m_inputType = INPUT_NONE;
	Network::Get()->Init( isServer );
	physx::Physics::Get()->Init();
	inGame = false;
	mouseCapture = false;
	m_menuSelected = 0;
	m_background = new Surface( "assets/bg.jpg" );


	if( isServer )
	{
		Network::Get()->StartServer();
		LoadScene();
	}
	else
	{
		Network::Get()->StartClient();
		Audio::Get()->LoadMusic( "assets/sound/main.mp3" );
		Audio::Get()->PlayMusic();
	}
}

void Game::Tick( float dt )
{
	if( !m_headless )
		m_surface->Clear( 0 );

	EnterCriticalSection( &g_Crit );
	ProcessEvents();
	LeaveCriticalSection( &g_Crit );

	EnterCriticalSection( &g_Crit );
	auto iter = GameObject::s_entities.begin();
	{
		while( iter != GameObject::s_entities.end() ) if( !( *iter )->m_kill )
		{
			( *iter )->Tick( dt );
			iter++;
		}
		else
		{
			if( ( *iter )->m_destroy ) delete *iter;
			iter = GameObject::s_entities.erase( iter );
		}
	}
	LeaveCriticalSection( &g_Crit );

	if( !m_headless && m_sceneInitialised )
	{
		m_renderer->Render();
		DrawCrosshair();
	}
	physx::Physics::Get()->StepPhysX();

	if( Input::GetKey( Input::Keys::TAB ) )
	{
		if( Network::Get()->IsConnected() )
		{
			ScoreBoard();
			m_scoreboard = true;
		}
	}
	else m_scoreboard = false;

	if( !m_headless && !m_scoreboard && Network::Get()->GetStatus() == Network::STATE_CONNECTED )
		Log::Get()->Draw( m_surface );

	if( Network::Get()->GetStatus() == Network::STATE_CONNECTING )
	{
		if( !m_headless )
			m_surface->Print( "Connecting..", SCRWIDTH / 2, 100, 0xffffff );
	}

	if( Network::Get()->GetStatus() == Network::STATE_BROWSER/* && Network::Get()->isBrowsingServers()*/ )
	{
		ServerBrowser();
	}

	if( Input::GetKeyDown( Input::CONSOLE ) )
	{
		if( m_inputType == INPUT_NONE )
		{
			m_textInput = inputmgr.m_typing = true;
			m_inputStr.clear();
			m_inputStr = "/";
			m_inputType = INPUT_CONSOLE;
		}
		else if( m_inputType == INPUT_CONSOLE )
		{
			m_inputType = INPUT_NONE;
			m_textInput = inputmgr.m_typing = false;
		};
	}

	if( Input::GetKeyDown( Input::T ) && !Game::s_FreeCam )
	{
		if( m_inputType == INPUT_NONE )
		{
			m_textInput = inputmgr.m_typing = true;
			m_inputStr.clear();
			m_inputStr = "say: ";
			m_inputType = INPUT_CHAT;
		}
	}

	if( m_textInput )
	{
		char buffer[128];
		strcpy_s( buffer, 128, m_inputStr.c_str() );
		m_surface->Print( buffer, 10, SCRHEIGHT - 100, 0xffffff );
	}

	if( Input::GetKeyDown( Input::Keys::ESCAPE ) )
	{
#if DEBUG_MESSAGES_
		Log::Get()->Print( "Keys::ESCAPE" );
#endif
		if( m_textInput )
		{
			m_inputType = INPUT_NONE;
			m_textInput = false;
		}
		else
		{
			switch( Network::Get()->GetStatus() )
			{
			case Network::STATE_CONNECTED:
				Command::Run( "disconnect" );
				break;
			case Network::STATE_CONNECTING:
				Network::Get()->ConnectToMS();
				break;
			case Network::STATE_BROWSER:
			case Network::STATE_SERVER:
			default:
				Command::Run( "quit" );
				break;
			}
		}
	}

	if( !m_headless )
		Log::Get()->Draw( m_surface );


	for( unsigned int i = 0; i < 127; i++ )
		Input::instance->isKeyDownNow[i] = false;
	for( unsigned int i = 0; i < 5; i++ )
		Input::instance->mousedown[i] = false;
	Input::instance->ry = 0;
	Input::instance->rx = 0;
}

void Game::LoadScene()
{
	Audio::Get()->StopMusic();

	Player::s_Mesh = Renderer::Instance->LoadMDL( ASSETS "assets/player.mdl", float3( 0, -3.0f, 0 ), 1.15f );
	Camera::s_mainCamera->transform.m_mat = matrix();
	if( m_server )
	{
		// Top-down spectator camera
		Camera::s_mainCamera->Translate( float3( 0, 185, 0 ) );
		Camera::s_mainCamera->Rotate( float3( 0, 0, -90 ) );
		Camera::s_mainCamera->Rotate( float3( 0, 90, 0 ) );
	}
	SceneNode* floor = new SceneNode();
	SceneNode* bigWalls = new SceneNode();
	SceneNode* smallWalls = new SceneNode();
	SceneNode* skyline = new SceneNode();

	floor->SetMesh( m_renderer->LoadObj( ASSETS "assets/levelFloor.obj", 2.00f ) );
	bigWalls->SetMesh( m_renderer->LoadObj( ASSETS "assets/levelBigWalls.obj", 2.00f ) );
	smallWalls->SetMesh( m_renderer->LoadObj( ASSETS "assets/levelSmallWalls.obj", 2.00f ) );
	skyline->SetMesh( m_renderer->LoadObj( ASSETS "assets/levelSkyline.obj", 2.00f ) );

	SceneGraph::Get()->GetRoot()->AddNode( floor );
	SceneGraph::Get()->GetRoot()->AddNode( bigWalls );
	SceneGraph::Get()->GetRoot()->AddNode( smallWalls );
	SceneGraph::Get()->GetRoot()->AddNode( skyline );


	GameObject* rflag = new Flag();
	Flag::redFlag = (Flag*)rflag;
	rflag->Rotate( float3( 0, 90, 0 ) );
	rflag->Translate( float3( -103, 30, -54 ) );
	rflag->SetMesh( Renderer::Instance->LoadObj( ASSETS "assets/red_flag.obj", 1.0f ) );
	rflag->AddCollider( false, 1.5f, 4.f, 1.5f );
	//( ( physx::PxRigidDynamic* )rflag->m_collider->m_actor )->set

	( (Flag*)rflag )->type = Flag::RED;
	( (Flag*)rflag )->m_startPosition = rflag->transform.GetTranslation();
	SceneGraph::Get()->GetRoot()->AddNode( rflag );


	GameObject* bflag = new Flag();
	Flag::blueFlag = (Flag*)bflag;
	bflag->Rotate( float3( 0, -90, 0 ) );
	bflag->Translate( float3( 103, 30, 54 ) );
	bflag->SetMesh( Renderer::Instance->LoadObj( ASSETS "assets/blue_flag.obj", 1.0f ) );
	bflag->AddCollider( false, 1.5f, 4.f, 1.5f );
	//( ( physx::PxRigidDynamic* )bflag->m_collider->m_actor )->

	( (Flag*)bflag )->m_startPosition = bflag->transform.GetTranslation();
	( (Flag*)bflag )->type = Flag::BLUE;
	SceneGraph::Get()->GetRoot()->AddNode( bflag );

	m_sceneInitialised = true;
	if( !m_server ) SDL_WM_GrabInput( SDL_GRAB_ON ), mouseCapture = true, SDL_ShowCursor( 0 );



	Mesh* collisionMap = Renderer::Instance->LoadObj( "assets/levelCollision.obj", 2.0f );
	Renderer::Instance->m_scene->LoadCollisionMesh( collisionMap );
}

void Game::UnloadScene()
{
	m_sceneInitialised = false;
	for( uint i = 0; i < 8; i++ )
		RemovePlayer( i );

	for( unsigned int i = 0; i < GameObject::s_entities.size(); i++ )
	{
		GameObject::s_entities[i]->m_kill = true;
		GameObject::s_entities[i]->m_destroy = true;
	}

	m_renderer->ClearSceneGraph();
	SDL_WM_GrabInput( SDL_GRAB_OFF ), mouseCapture = false, SDL_ShowCursor( 1 );
}

void Game::SpawnPlayer( const unsigned char idx )
{
	Player* player = new Player();
	player->SetId( idx );
	player->JoinTeam( idx % 2 );
	//	player->Translate( Player::s_spawnLocations[player->GetTeam()] );
	player->SetPosition( Player::s_spawnLocations[player->GetTeam()] );
	if( Network::Get()->IsServer() )
	{
		float angle = player->GetTeam() == 0 ? -90.0f * PI : 90.f * PI;
		player->transform.Rotate( float3( 0, -(angle)* ( 180.f / PI ), 0 ) );
		player->SetRotation( angle );
	}
	Player::s_players[idx] = player;
	SceneGraph::Get()->GetRoot()->AddNode( player );
	Audio::Get()->PlaySound( "assets/sound/spawn.ogg" );
	Network::Get()->IncrementNumPlayers();
}

void Game::RemovePlayer( const unsigned char idx )
{
	SceneNode* p = Player::s_players[idx];
	if( p != nullptr )
	{
		SceneNode* sg = SceneGraph::Get()->GetRoot();
		sg->RemoveNode( p );
		( (Player*)p )->m_kill = true;
		( (Player*)p )->m_destroy = true;
		Player::s_players[idx] = nullptr;
		Network::Get()->DecrementNumPlayers();
	}
}

void Game::EnterGame()
{
	EnterCriticalSection( &g_Crit );
	Command::s_game->LoadScene();
	LeaveCriticalSection( &g_Crit );
}

void Game::ExitGame() // I should rename this LeaveGame()
{
	Command::s_game->KeyUp( Input::Keys::ESCAPE );
	Command::s_game->UnloadScene();
	Command::s_game->m_sceneInitialised = false;
	Network::Get()->ConnectToMS();
	SDL_WM_GrabInput( SDL_GRAB_OFF );
	//Network::Get()->::STATE_BROWSER;
}

void Game::ServerBrowser()
{
	const int numServers = Network::Get()->m_serverList.size();
	unsigned int color = 0xffffff;
	bool menuHover = false;
	m_background->CopyTo( m_surface, 0, 0 );
	if( inputmgr.my >= 90 && inputmgr.my <= 110 )
	{
		if( m_menuSelected != MENU_REFRESH ) Audio::Get()->PlaySound( "assets/sound/menu.ogg" );
		m_menuSelected = MENU_REFRESH;
		menuHover = true;
		color = 0xff0000;
		if( Input::GetMouseDown( Input::MOUSE_LEFT ) )
		{
			Network::Get()->RequestServerList();
		}
	}

	if( Input::GetMouseDown( 2 ) ) LoadScene();

	m_surface->Print( "Refresh", SCRWIDTH / 3, 100, color );

	for( int i = 0; i < numServers; i++ )
	{
		AvailableServer s = Network::Get()->m_serverList[i];
		color = 0xffffff;
		if( inputmgr.my >= ( 120 + i * 30 ) - 5 && inputmgr.my <= ( 120 + i * 30 ) + 5 )
		{
			if( m_menuSelected != MENU_SERVER + i ) Audio::Get()->PlaySound( "assets/sound/menu.ogg" );
			m_menuSelected = MENU_SERVER + i;
			menuHover = true;
			color = 0xff0000;
			if( Input::GetMouseDown( Input::MOUSE_LEFT ) )
			{
				std::string ip( s.addr.ToString() );
				ip = ip.substr( 0, ip.length() - ip.find( ":" ) );
				Network::Get()->ConnectToGame( RakNet::SystemAddress( ip.c_str(), 8233 ) );
			}
		}
		char buffer[128];
		sprintf( buffer, "%s - %s - %i/8 players", s.addr.ToString(), s.name.C_String(), s.numPlayers );
		m_surface->Print( buffer, SCRWIDTH / 3, 120 + i * 30, color );
	}

	if( !menuHover ) m_menuSelected = MENU_NONE;
}

void Game::ParseText( std::string t )
{
	if( t.find( "/" ) <= 1 )
	{
		RunCommand( t.substr( 1, t.length() ).c_str() );
	}
	else
	if( t.find( "say: " ) <= 1 )
	{
		Network::Get()->SendMsg( t.substr( 5, t.length() ).c_str(), Network::Get()->GetServerAdress(), false, false );
	}
}

void Game::RunCommand( const std::string c, std::vector<std::string> args )
{
	ConsoleCommand cc;
	cc.command = c;
	cc.args = args;
	GameEvent ge;
	ge.type = GameEvent::COMMAND;
	ge.ce = cc;
	FireEvent( ge );
}

void Game::RunCommand( const char* c )
{
	std::string t( c );
	bool end = false;
	std::vector<std::string> args;
	int space = t.find_first_of( " " );
	if( space == std::string::npos ) space = t.length(), end = true;
	std::string base = t.substr( 0, space );
	if( !end )
		t = t.substr( base.length() + 1 );
	while( !end )
	{
		space = t.find_first_of( " " );
		if( space == std::string::npos ) space = t.length(), end = true;
		std::string arg = t.substr( 0, space );
		if( !end )	t = t.substr( space + 1, t.length() );
		args.push_back( arg );
	}
	RunCommand( base, args );
}

void Game::FireEvent( GameEvent gameEvent )
{
	EnterCriticalSection( &g_Crit );
	Command::s_game->m_events.push( gameEvent );
	LeaveCriticalSection( &g_Crit );
}

void Game::ProcessEvents()
{
	while( m_events.size() > 0 )
	{
		GameEvent ge = m_events.front();
		m_events.pop();
		PacketPlayerInfo* info = (PacketPlayerInfo*)ge.data;
		switch( ge.type )
		{
		case GameEvent::COMMAND:
			ge.ce.command == "connect" ? Command::Connect( ge.ce.args ) :
				ge.ce.command == "disconnect" ? Command::Disconnect() :
				ge.ce.command == "kill" ? Command::Kill() :
				ge.ce.command == "lanconnect" ? Command::Localconnect() :
				ge.ce.command == "name" ? Command::Name( ge.ce.args ) :
				ge.ce.command == "quit" ? Command::Quit() :
				Command::UnrecognisedCommand( ge.ce.command.c_str() );
			break;
		case GameEvent::SPAWNPARTICLE:
			SpawnParticle( *(float3*)ge.data );
			delete (float3*)ge.data;
			break;
		case GameEvent::PLAYER_SUICIDE:
		case GameEvent::PLAYER_DEATH:
			Player::s_players[ge.id0]->Kill();
			break;
		case GameEvent::PLAYER_SHOT:
			Player::s_players[ge.id0]->FireWeapon();
			break;
		case GameEvent::PLAYER_JUMP:
			Player::s_players[ge.id0]->Jump();
			break;
		case GameEvent::PLAYER_FLAG:
			ge.id1 == PacketFlagAction::STEAL ? Player::s_players[ge.id0]->TakeFlag() :
				ge.id1 == PacketFlagAction::CAPTURE ? Player::s_players[ge.id0]->CaptureFlag() :
				0;
			break;
		case GameEvent::PLAYER_JOIN:
			SpawnPlayer( ge.id0 );

			if( info->name[0] == 'y' ) // If the player is the local player
			{
				Player::s_localPlayer = Player::s_players[ge.id0];
				Player::s_localPlayer->SetAsLocal();
				Camera::s_mainCamera->transform.Rotate( float3( 0, -( info->pitch ) * ( 180 / PI ), 0 ) );
			}
			if( Network::Get()->IsServer() )
			{
				Network::Get()->server->BroadcastClient( Network::Get()->FindClient( ge.id0 ) );
			}
			else
			{
				Player::s_players[ge.id0]->SetTranslation( info->pos );
				Player::s_players[ge.id0]->Rotate( float3( 0, -( info->pitch ) * ( 180 / PI ), 0 ) );
				Player::s_players[ge.id0]->SetRotation( info->pitch );
				//	Player::s_players[ge.id0]->TakeTransform( info->pos, info->angle );
			}

			delete info;
			break;
		case GameEvent::PLAYER_QUIT:
			RemovePlayer( ge.id0 );
			break;
		}
	}
}

void Game::ScoreBoard()
{
	char buffer[128];
	sprintf( buffer, "Unnamed Server - %i/8 players", Network::Get()->GetNumPlayers() );
	m_surface->Print( buffer, SCRWIDTH / 3 + 50, 50, 0xffffff );
	sprintf( buffer, "RED TEAM :: Score: %i", Network::Get()->GetScore( 0 ) );
	m_surface->Print( buffer, SCRWIDTH / 2 - 50, 100, 0xffffff );
	for( int i = 0; i < Network::Get()->GetNumPlayers(); i++ )
	{
		Client c = Network::Get()->FindClient( i );
		if( Player::s_players[c.idx]->GetTeam() == 0 )
		{
			sprintf( buffer, "%s - Score: %i - ping: %i\n", c.name, (int)Player::s_players[c.idx]->GetScore(), c.ping );
			m_surface->Print( buffer, SCRWIDTH / 3 + 50, 110 + i * 20, 0xffffff );
		}
	}

	sprintf( buffer, "BLUE TEAM	:: Score: %i", Network::Get()->GetScore( 1 ) );
	m_surface->Print( buffer, SCRWIDTH / 2 - 50, SCRHEIGHT / 2, 0xffffff );
	for( int i = 0; i < Network::Get()->GetNumPlayers(); i++ )
	{
		Client c = Network::Get()->FindClient( i );
		if( Player::s_players[c.idx]->GetTeam() == 1 )
		{
			sprintf( buffer, "%s - Score: %i - ping: %i\n", c.name, (int)Player::s_players[c.idx]->GetScore(), c.ping );
			m_surface->Print( buffer, SCRWIDTH / 3 + 50, SCRHEIGHT / 2 + 20 + i * 20, 0xffffff );
		}
	}
}

void Game::DrawCrosshair()
{
	for( int y = -1; y < 2; y++ )
		m_surface->GetBuffer()[SCRWIDTH / 2 + ( SCRHEIGHT / 2 + y )* m_surface->GetPitch()] = 0xffffff;
	for( int x = -1; x < 2; x++ )
		m_surface->GetBuffer()[SCRWIDTH / 2 + SCRHEIGHT / 2 * m_surface->GetPitch() + x] = 0xffffff;
}

Corpse* Game::SpawnBody()
{
	Corpse* c = nullptr;
	EnterCriticalSection( &g_Crit );
	c = new Corpse();
	LeaveCriticalSection( &g_Crit );
	return c;
}

Particle* Game::SpawnParticle( float3 pos )
{
	Particle* c = nullptr;
	EnterCriticalSection( &g_Crit );
	c = new Particle();
	LeaveCriticalSection( &g_Crit );
	c->SetTranslation( pos );
	return c;
}

void Game::LoadConfig( const char* fileName )
{
	FILE* file = fopen( fileName, "r" );
	if( file != NULL )
	{
		while( 1 )
		{
			char lineHeader[128];
			char buffer[24];
			int res = fscanf( file, "%s = ", lineHeader );
			if( res == EOF ) break;
			if( strcmp( lineHeader, "name" ) == 0 )
			{
				fscanf( file, "%s\n", buffer );
				Network::Get()->SendNameChange( std::string( buffer ) );
			}
			else if( strcmp( lineHeader, "masterserver" ) == 0 )
			{
				fscanf( file, "%s\n", buffer );
				Network::Get()->SetMasterServerAddr( std::string( buffer ) );
			}

		}
#ifdef DEBUG_MESSAGES
		Log::Get()->Print( "%s loaded", fileName );
#endif
	fclose( file );
	}
	else Log::Get()->Print( "Failed to load %s", fileName );
}

Game::~Game()
{
	DeleteCriticalSection( &g_Crit );
	Network::Get()->CloseNetworking();
	delete m_renderer;
	delete Log::Get();
	delete Network::Get();
	delete m_background;
	for( uint i = 0; i < Collider::s_colliders.size(); i++ ) { delete Collider::s_colliders[i]; }
	delete physx::Physics::Get();
}
