// Template for GP1, version 1
// IGAD/NHTV - Jacco Bikker - 2006-2013

#pragma once
#include "template.h"
#include "input.h"
#include <string>
#include <vector>
#include "SDL.h"
#include <queue>

namespace Tmpl8
{
	class Surface;
	class Renderer;
	class SceneNode;
	class Server;
	struct Client;
	class Mesh;

	struct ConsoleCommand
	{
		std::string command;
		std::vector<std::string> args;
	};

#define MENU_NONE 00
#define MENU_REFRESH 01
#define MENU_SERVER 02


	struct GameEvent
	{
		enum EventType
		{
			SPAWNPARTICLE,
			PLAYER_HIT,
			PLAYER_SHOT,
			PLAYER_JUMP,
			PLAYER_SUICIDE,
			PLAYER_DEATH,
			PLAYER_FLAG,
			PLAYER_JOIN,
			PLAYER_QUIT,
			COMMAND,
		} type;
		
		int id0;
		union {
			void* data;
			int id1;
		};
		ConsoleCommand ce;
	};

	class Game
	{
	public:
		void SetTarget( Surface* a_Surface ) { m_surface = a_Surface; }
		void Init( bool server = false, bool headless = false );

		void Tick( float a_DT );
		void LoadScene();
		void UnloadScene();
		void ServerBrowser();
		void KeyDown( unsigned int code );
		void KeyIn( SDL_Event k );
		void KeyUp( unsigned int code );
		void MouseMove( unsigned int x, unsigned int y );
		void MouseMoveRel( int x, int y );
		void MouseUp( unsigned int button ) {}
		void MouseDown( unsigned int button );
		void ParseText( std::string t );
		void RunCommand( const std::string c, std::vector<std::string> args );
		void RunCommand( const char* c );
		void ProcessEvents( );
		void ScoreBoard();
		void DrawCrosshair();
		void LoadConfig( const char* file );

		Renderer* m_renderer;
		Input inputmgr;
		bool mouseCapture;
		bool m_textInput;
		static bool s_FreeCam;
		static void SpawnPlayer( const unsigned char a_id );
		static void RemovePlayer( const unsigned char a_id );
		static void EnterGame();
		static void ExitGame();
		static void FireEvent( GameEvent gameEvent );
		static class Corpse* SpawnBody();
		static class Particle* SpawnParticle( float3 pos );

		std::string m_inputStr;
		~Game();
	private:
		Surface* m_surface;
		Surface* m_background;
		bool inGame;
		bool m_server;
		bool m_headless;
		bool m_sceneInitialised;
		bool m_scoreboard;
		int m_menuSelected;
		std::queue<GameEvent> m_events;

		enum InputType
		{
			INPUT_NONE,
			INPUT_CONSOLE,
			INPUT_CHAT
		} m_inputType;
	};
	
}; // namespace Tmpl8