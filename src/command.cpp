#include "game.h"
#include "command.h"
#include "log.h"
#include "network.h"

using namespace Tmpl8;


void Command::Run( const char* command )
{
	s_game->RunCommand( command );
}


void Command::Connect( std::vector<std::string> args )
{
	if( args.size() != 1 )
	{
		Log::Get()->Print( "Incorrect usage of connect: 'connect IP' " );
		return;
	}
	if( Network::Get()->IsServer() )
	{
		return;
	}
	Network::Get()->ConnectToGame( RakNet::SystemAddress( args[0].c_str() ) );
}

void Command::Disconnect()
{
	Network::Get()->Disconnect();
	Command::s_game->ExitGame();
}

void Command::Localconnect()
{
	s_game->RunCommand( "connect 127.0.0.1" );
}

void Command::Name( std::vector<std::string> args )
{
	if( args.size() == 0 )
	{
		Log::Get()->Print( "Incorrect usage of name. Try: 'name sample_name' " );
	}
	else
	{
		Network::Get()->SendNameChange( args.at( 0 ) );
	}
}

void Command::Kill()
{
	Network::Get()->SendCmd( "kill" );
}

void Command::Quit()
{
#if DEBUG_MESSAGES
	Log::Get()->Print( "Command::Quit()" );
#endif
	Command::s_game->UnloadScene();
	SDL_Event e;
	e.type = SDL_QUIT;
	SDL_PushEvent( (SDL_Event*)&e );
}

void Command::UnrecognisedCommand( const char* c )
{
	Log::Get()->Print( "Unrecognised command: %s", c );
}