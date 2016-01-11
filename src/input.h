

#ifndef _INPUT_H
#define _INPUT_H
#include "template.h"
namespace Tmpl8 {

	class Input
	{
	public:
		Input();
		~Input();

		static bool GetKey( unsigned int c );
		static bool GetKeyDown( unsigned int c );
		static bool GetMouseDown( unsigned int c ) { return instance->mousedown[c]; }
		static bool isTyping( ) { return instance->m_typing; }
		static Input* instance;
		bool isKeyDown[128];
		bool isKeyDownNow[128];
		bool mousedown[5];
		bool m_typing;
		int rx;
		int ry;
		int mx, my;
		static enum Keys
		{
			W = 17,
			A = 30,
			S = 31,
			D = 32,
			UP = 72,
			DOWN = 80,
			LEFT = 75,
			RIGHT = 77,
			SPACE = 57,
			CTRL = 29,
			SHIFT = 42,
			TAB = 15,

			CONSOLE = 41,
			RETURN = 28,
			BACKSPACE = 14,
			ESCAPE = 1,

			E = 18,
			R = 19,
			T = 20,
			Y = 21,
			U = 22,
			F = 33,

			F1 = 59,
			F2 = 60,
			F3 = 61,
			F4 = 62

		};

		static enum MouseKeys
		{
			MOUSE_LEFT = 1,
			MOUSE_MIDDLE,
			MOUSE_RIGHT,
			SCROLL_UP,
			SCROLL_DOWN
		};
	};


};

#endif