

#include "game.h"
#include "scene.h"
#include "renderer.h"
#include "scenegraph.h"
#include "input.h"
#include "gameobject.h"
#include "log.h"
#include "..\lib\SDL\include\SDL.h"
//#include "..\lib\SDL\include\SDL_keyboard.h"

using namespace Tmpl8;

Input* Input::instance = nullptr;

void Game::KeyDown( unsigned int c )
{
//	printf("%i\n", c);
	if( c >= 127 ) return;
	if( Input::instance ) 
	{
		Input::instance->isKeyDown[c] = true;
		Input::instance->isKeyDownNow[c] = true;
	}
	switch (c)
	{
	case Input::Keys::SHIFT:
		if( !mouseCapture) SDL_WM_GrabInput( SDL_GRAB_ON ), mouseCapture = true, SDL_ShowCursor( 0 );
		else SDL_WM_GrabInput( SDL_GRAB_OFF ), mouseCapture = false, 	SDL_ShowCursor( 1 );
		break;
	case Input::Keys::F1:
		Game::s_FreeCam = !Game::s_FreeCam;
		Log::Get()->Print("Freecam set to %i", Game::s_FreeCam);
		break;
	case Input::Keys::F2:
		Renderer::s_backfaceCulling = !Renderer::s_backfaceCulling;
		Log::Get()->Print("BackfaceCulling set to %i", Renderer::s_backfaceCulling);
		break;
	case Input::Keys::F3:
		Renderer::s_wireframe = !Renderer::s_wireframe;
		Log::Get()->Print("Wireframe set to %i", Renderer::s_wireframe);
		break;
	case Input::Keys::F4:
		Renderer::s_enableAnimations = !Renderer::s_enableAnimations;
		Log::Get()->Print("EnableAnimations set to %i", Renderer::s_enableAnimations);
		break;
	case 2:
		
		break;
	case 3:
		SDL_WM_GrabInput( SDL_GRAB_OFF );
		break;
	case Input::Keys::RETURN:
		if( m_textInput )
		{
			m_textInput = false;
			m_inputType = INPUT_NONE;
			ParseText( m_inputStr );
		}
		break;
	case Input::Keys::BACKSPACE:
		if( m_textInput )
		{
			if(m_inputStr.size() > 0 ) m_inputStr.pop_back();
			//printf( "backspaced\n" );
		}
		break;
	case Input::Keys::ESCAPE:
		
		break;
	default:
		break;
	}
}

void Game::KeyIn( SDL_Event e )
{
	if( inputmgr.m_typing = m_textInput )
	{
		if( e.key.keysym.sym >= 97 && e.key.keysym.sym <= 127 || e.key.keysym.sym >= 32 && e.key.keysym.sym <= 64 )
			m_inputStr += (char)(e.key.keysym.sym);
	}
}

bool Input::GetKey( unsigned int c )
{
	if( c >= 127 ) return false;
	return instance->isKeyDown[ c ];
}
bool Input::GetKeyDown( unsigned int c )
{
	if( c >= 127 ) return false;
	return instance->isKeyDownNow[ c ];
}
void Game::KeyUp( unsigned int c )
{
	if( c >= 127 ) return;
	if( Input::instance ) Input::instance->isKeyDown[c] = false;
}

void Game::MouseMove( unsigned int x, unsigned int y ) 
{
//	if( !mouseCapture ) return;
	Input::instance->mx = x;
	Input::instance->my = y;
}

void Game::MouseDown( unsigned int c )
{
	if( Input::instance ) Input::instance->mousedown[c] = true;
//	printf("%i\n", c );
}

void Game::MouseMoveRel( int x, int y ) 
{
	if( !mouseCapture ) return;
	if( Input::instance ) 
	{
		Input::instance->rx = x;
		Input::instance->ry = y;
	}	
}

Input::Input()
{
	instance = this;
	m_typing = false;
	for(unsigned int i = 0; i < 128; i++)
			instance->isKeyDown[i] = false;
	for(unsigned int i = 0; i < 128; i++)
			instance->isKeyDownNow[i] = false;
	for(unsigned int i = 0; i < 5; i++)
		instance->mousedown[i] = false;
	rx = 0;
	ry = 0;
	mx = 0;
	my = 0;
}
Input::~Input()
{

}