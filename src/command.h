#ifndef _COMMAND_H
#define _COMMAND_H

#include <vector>
#include <string>

namespace Tmpl8
{
	class Command
	{
	public:

		static void Run( const char* command );
		///////////////////////////////////////////
	protected:
		static void Connect( std::vector<std::string> args );
		static void Disconnect();
		static void Kill();
		static void Localconnect();
		static void Name( std::vector<std::string> args );
		static void Quit();
		static void UnrecognisedCommand( const char* );


		static class Game* s_game;
		friend class Game;
	};
};

#endif